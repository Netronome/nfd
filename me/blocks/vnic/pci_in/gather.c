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
 * Temporary header includes
 */
#include <nfp/cls.h>            /* TEMP */
#include <nfp/mem_ring.h>       /* TEMP */
#include <nfp6000/nfp_me.h>     /* TEMP */
#include <nfp6000/nfp_cls.h>    /* TEMP */


/*
 * State variables for PCI.IN gather
 */
/* XXX who should conceptually own the pending_bmsk? service_qc or gather? */
extern __shared __gpr struct qc_bitmask pending_bmsk;
extern __shared __lmem struct tx_queue_info queue_data[64]; /* XXX use #define*/

/* XXX assume hi bits 0 to DMA into CLS */
static __shared __gpr unsigned int desc_ring_base;

__shared __gpr unsigned int dma_seq_issued;
extern __shared __gpr unsigned int gather_dma_seq_compl;

#define DESC_RING_SZ (MAX_TX_BATCH_SZ * DESC_BATCH_Q_SZ *       \
                      sizeof(struct nfd_pci_in_tx_desc))
__export __shared __cls __align(DESC_RING_SZ) struct nfd_pci_in_tx_desc
    desc_ring[MAX_TX_BATCH_SZ * DESC_BATCH_Q_SZ];

static __gpr struct nfp_pcie_dma_cmd descr_tmp;

__shared __gpr mem_ring_addr_t gather_dmas_addr; /* TEMP */
MEM_JOURNAL_DECLARE(gather_dmas, 1024); /* TEMP */


void
dummy_init_gather_shared()
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

    gather_dmas_addr = MEM_JOURNAL_CONFIGURE(gather_dmas, 1); /* TEMP */

    /*
     * Initialise DMA sequence tracking
     */
    dma_seq_issued = 0;

    /*
     * Set up TX_GATHER_CFG_REG DMA Config Register
     */
    cfg.__raw = 0;
    cfg.signal_only = 1; /* TEMP use signal_only for seqn num generation */
    cfg.end_pad     = 0;
    cfg.start_pad   = 0;
    /* Ordering settings? */
    cfg.target_64   = 0;
    cfg.cpp_target  = 15;
    pcie_dma_cfg_set_one(PCIE_ISL, TX_GATHER_CFG_REG, cfg);
}

static __intrinsic void dummy_dma(unsigned int dma_mode, __xwrite void *descr)
{
    SIGNAL_PAIR journal_sig;
    SIGNAL dma_sig;

    __mem_ring_journal(1, gather_dmas_addr, descr,
                       sizeof(struct nfp_pcie_dma_cmd),
                       sizeof(struct nfp_pcie_dma_cmd),
                       sig_done, &journal_sig);

    /* Fake a DMA ... using a DMA (signal only) */
    __pcie_dma_enq(PCIE_ISL, descr, TX_GATHER_DMA_QUEUE, sig_done, &dma_sig);

    /* Wait on both writes at once */
    wait_for_all_single(&journal_sig.even, &dma_sig);
}

void
dummy_init_gather()
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

__intrinsic void
dummy_gather_vnic_setup(void *cfg_msg_in, unsigned int queue_size)
{
    struct pci_in_cfg_msg *cfg_msg;
    struct qc_queue_config txq;

    /* XXX make this code maintain state and re-enter
     * to support case where data is read from emem */
    unsigned int start_queue;
    unsigned int end_queue;
    unsigned int queue;

    ctassert(__is_log2(MAX_VNICS));
    ctassert(__is_log2(MAX_VNIC_QUEUES));

    txq.watermark    = PCIE_QC_WM_4;
    txq.size         = queue_size - 8; /* XXX add define for size shift */
    txq.event_data   = TXQ_EVENT_DATA;
    txq.event_type   = PCIE_QC_EVENT_NOT_EMPTY;
    txq.ptr          = 0;

    cfg_msg = cfg_msg_in;
    start_queue = cfg_msg->vnic * MAX_VNIC_QUEUES;
    end_queue = start_queue + MAX_VNIC_QUEUES;

    /*
     * XXX Set up per queue data to dummy values
     */
    for (queue = start_queue; queue < end_queue; queue++) {
        unsigned int bmsk_queue = map_natural_to_bitmask(queue);

        queue_data[bmsk_queue].tx_w = 0;
        queue_data[bmsk_queue].tx_s = 0;
        queue_data[bmsk_queue].ring_sz_msk = ((1 << queue_size) - 1);
        queue_data[bmsk_queue].requester_id = cfg_msg->vnic;
        /* TEMP */
        queue_data[bmsk_queue].ring_base_hi = 0;
        queue_data[bmsk_queue].ring_base_lo = (0x47<<24) | (queue<<16);
    }

    init_qc_queues(PCIE_ISL, &txq, TXQ_START + start_queue * 2, 2,
                   MAX_VNIC_QUEUES);

    /* Indicate completion */
    cfg_msg->msg_valid = 0;
}

int
gather()
{
    int ret;
    __gpr unsigned int queue = 0;
    __gpr unsigned int tx_r_update_tmp;

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
         * Compute increase.
         */
        tx_r_update_tmp = queue_data[queue].tx_w - queue_data[queue].tx_s;
        if (tx_r_update_tmp > MAX_TX_BATCH_SZ) {
            tx_r_update_tmp = MAX_TX_BATCH_SZ;
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
            descr_tmp.length = (tx_r_update_tmp *
                                sizeof(struct nfd_pci_in_tx_desc));
            descr = descr_tmp;

            /*
             * Update tx_s before swapping
             */
            queue_data[queue].tx_s += tx_r_update_tmp;

            /*
             * Issue the DMA TEMP: out until DMAs available
             */
            /* pcie_dma_enq(PCIE_ISL, &descr, NFP_PCIE_DMA_FROMPCI_HI); */

            /* TEMP:
             * write descriptor to the CLS buffer and
             * trigger user event */
            dummy_dma(descr_tmp.dma_mode, &descr);

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
