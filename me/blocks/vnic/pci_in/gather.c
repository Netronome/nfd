/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/gather.c
 * @brief         Code to gather TX descriptors from pending queues
 */

#include <assert.h>
#include <nfp.h>

#include <vnic/pci_in/gather.h>

#include <vnic/pci_in.h>
#include <vnic/pci_in_cfg.h>
#include <vnic/pci_in/pci_in_internal.h>
#include <vnic/shared/qc.h>
#include <vnic/utils/pcie.h>
#include <vnic/utils/nn_ring.h>


/*
 * State variables for PCI.IN gather
 */
/* XXX who should conceptually own the pending_bmsk? service_qc or gather? */
extern __shared __gpr struct qc_bitmask pending_bmsk;
extern __shared __lmem struct tx_queue_info queue_data[MAX_TX_QUEUES];

/* XXX assume hi bits 0 to DMA into CLS */
static __shared __gpr unsigned int desc_ring_base;

__shared __gpr unsigned int dma_seq_issued = 0;
extern __shared __gpr unsigned int gather_dma_seq_compl;

#define DESC_RING_SZ (MAX_TX_BATCH_SZ * DESC_BATCH_Q_SZ *       \
                      sizeof(struct nfd_pci_in_tx_desc))
__export __shared __cls __align(DESC_RING_SZ) struct nfd_pci_in_tx_desc
    desc_ring[MAX_TX_BATCH_SZ * DESC_BATCH_Q_SZ];

static __gpr struct nfp_pcie_dma_cmd descr_tmp;


void
gather_setup_shared()
{
    struct pcie_dma_cfg_one cfg;

    /*
     * Initialise the NN ring
     */
    nn_ring_init_send(0);

    /*
     * Initialise the CLS TX descriptor ring
     */
    desc_ring_base = ((unsigned int) &desc_ring) & 0xFFFFFFFF;

    /*
     * Set up TX_GATHER_CFG_REG DMA Config Register
     */
    cfg.__raw = 0;
#ifdef NFD_VNIC_NO_HOST
    /* Use signal_only for seqn num generation
     * Don't actually DMA data */
    cfg.signal_only = 1;
#else
    cfg.signal_only = 0;
#endif
    cfg.end_pad     = 0;
    cfg.start_pad   = 0;
    /* Ordering settings? */
    cfg.target_64   = 0;
    cfg.cpp_target  = 15;
    pcie_dma_cfg_set_one(PCIE_ISL, TX_GATHER_CFG_REG, cfg);
}


void
gather_setup()
{
    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, and PCIe address will be
     * overwritten per transaction.
     * For dma_mode, we technically only want to overwrite the "source"
     * field, i.e. 12 of the 16 bits.
     */
    descr_tmp.rid_override = 1;
    descr_tmp.trans_class = 0;
    descr_tmp.cpp_token = 0;    /* CLS doesn't offer write swap token */
    descr_tmp.dma_cfg_index = TX_GATHER_CFG_REG;
    descr_tmp.cpp_addr_hi = 0;
}


int
gather()
{
    int ret;
    __gpr unsigned int queue = 0;
    __gpr unsigned int tx_r_update_tmp;
    __gpr unsigned int tx_s_cp;
    __gpr int tx_r_correction;

    /*
     * Before looking for a batch of work, we need to be able to issue a DMA
     * and we need space in the CLS and NN rings. The CLS ring is sized to hold
     * more batches than the NN ring, so checking for !nn_ring_full is enough.
     */
    if ((dma_seq_issued < (TX_GATHER_MAX_IN_FLIGHT + gather_dma_seq_compl)) &&
        (!nn_ring_full())) {
        ret = select_queue(&queue, &pending_bmsk);

        /* No work to do. */
        if (ret) {
            return 0;
        }

        /*
         * Compute increase, strategy:
         * Round tx_s up to the next MAX_TX_BATCH_SZ multiple (64B multiple)
         * Compute the batch size that would produce this tx_s
         * Subtract this tx_s from tx_w. If positive or zero, leave batch size
         * unchanged.  If negative, correct tx_s and batch size by adding the
         * negative error.
         */
        ctassert(__is_log2(MAX_TX_BATCH_SZ));
        tx_s_cp = MAX_TX_BATCH_SZ + queue_data[queue].tx_s;
        tx_s_cp &= ~(MAX_TX_BATCH_SZ - 1);
        tx_r_correction = queue_data[queue].tx_w - tx_s_cp;
        tx_r_update_tmp = (MAX_TX_BATCH_SZ -
                           (queue_data[queue].tx_s & (MAX_TX_BATCH_SZ - 1)));
        if (tx_r_correction < 0) {
            tx_r_update_tmp += tx_r_correction;
        }

        /*
         * Check if there is a batch to handle
         */
        if(tx_r_update_tmp != 0) {
            /*
             * There is. Put a message on the work_ring.
             */
            struct batch_desc batch;
            unsigned int dma_cmd_sz;
            unsigned int pcie_addr_off;
            unsigned int desc_ring_off;
            __xwrite struct nfp_pcie_dma_cmd descr;

            /*
             * Increment dma_seq_issued upfront to avoid ambiguity
             * about sequence number zero
             */
            dma_seq_issued++;

            /*
             * Compute desc_ring and PCIe offsets
             * PCIe offset depends on tx_s, the total packets serviced on the
             * queue. desc_ring offset depends on the batches processed, each
             * batch having it's own slot in the ring.
             */
            dma_cmd_sz = sizeof(struct nfp_pcie_dma_cmd);
            pcie_addr_off = (queue_data[queue].tx_s &
                             queue_data[queue].ring_sz_msk);
            pcie_addr_off = pcie_addr_off * dma_cmd_sz;
            desc_ring_off = ((dma_seq_issued * MAX_TX_BATCH_SZ *
                             sizeof(struct nfd_pci_in_tx_desc)) &
                             (DESC_RING_SZ - 1));

            /*
             * Populate the batch message
             */
            batch.__raw = 0;
            batch.queue = queue;
            batch.num = tx_r_update_tmp;
            nn_ring_put(batch.__raw);

            /*
             * Prepare the "DMA"
             * Filling the descriptor can possibly be optimised either
             * by doing it all by hand, or playing with initialisation time.
             */
            descr_tmp.pcie_addr_hi = queue_data[queue].ring_base_hi;
            descr_tmp.pcie_addr_lo = (queue_data[queue].ring_base_lo |
                                      pcie_addr_off);
            descr_tmp.cpp_addr_lo =  desc_ring_base | desc_ring_off;
            descr_tmp.rid = queue_data[queue].requester_id;
            /* Can replace with ld_field instruction if 8bit seqn is enough */
            pcie_dma_set_event(&descr_tmp, TX_GATHER_EVENT_TYPE,
                               dma_seq_issued);
            descr_tmp.length = ((tx_r_update_tmp *
                                 sizeof(struct nfd_pci_in_tx_desc)) - 1);
            descr = descr_tmp;

            /*
             * Update tx_s before swapping
             */
            queue_data[queue].tx_s += tx_r_update_tmp;

            /*
             * Issue the DMA
             */
            pcie_dma_enq(PCIE_ISL, &descr, TX_GATHER_DMA_QUEUE);

        } else {
            /*
             * No batch to processes.
             * Clear pending_bmsk so we don't check it again
             * unless something resets the bitmask
             */
            clear_queue(queue, &pending_bmsk);
        }
    }

    return 0;
}
