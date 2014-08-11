/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/issue_dma.c
 * @brief         Code to DMA packet data from the NFP
 */

/* XXX probably better to name the PCI.OUT and PCI.IN files differently */

#include <assert.h>
#include <nfp.h>

#include <nfp6000/nfp_pcie.h>

#include <vnic/pci_out/issue_dma.h>

#include <vnic/pci_out.h>
#include <vnic/pci_out_cfg.h>
#include <vnic/pci_out/pci_out_internal.h>
#include <vnic/shared/qc.h>
#include <vnic/utils/cls_ring.h>
#include <vnic/utils/nn_ring.h>
#include <vnic/utils/pcie.h>

/* XXX move somewhere shared? */
struct _dma_desc_batch {
    struct nfp_pcie_dma_cmd pkt0;
    struct nfp_pcie_dma_cmd pkt1;
    struct nfp_pcie_dma_cmd pkt2;
    struct nfp_pcie_dma_cmd pkt3;
};

/* XXX Further data will be required when checking for follow on packets
 * volatile set temporarily */
volatile __shared __lmem unsigned int ring_rids[MAX_RX_QUEUES];

__shared __gpr unsigned int data_dma_msg_served = 0; /* XXX necessary? */

__shared __gpr unsigned int data_dma_seq_issued = 0;
__shared __gpr unsigned int data_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_served = 0;

__visible volatile __xread unsigned int rx_data_compl_reflect_xread;
__visible volatile SIGNAL rx_data_compl_reflect_sig;

static __gpr struct nfp_pcie_dma_cmd descr_tmp;
static __xwrite struct _dma_desc_batch dma_out_main;
static __xwrite struct _dma_desc_batch dma_out_res;
static SIGNAL data_sig0, data_sig1, data_sig2, data_sig3;


NN_RING_ZERO_PTRS;
NN_RING_EMPTY_ASSERT_SET(0);


void
issue_dma_setup_shared()
{
    /* XXX complete non-per-queue data */
    unsigned int queue;
    unsigned int vnic;
    unsigned int vnic_q;
    struct nfp_pcie_dma_cfg cfg_tmp;
    __xwrite struct nfp_pcie_dma_cfg cfg;

    ctassert(__is_log2(MAX_VNICS));
    ctassert(__is_log2(MAX_VNIC_QUEUES));

    /*
     * Set requester IDs
     */
    for (queue = 0, vnic = 0; vnic < MAX_VNICS; vnic++) {
        for (vnic_q = 0; vnic_q < MAX_VNIC_QUEUES; vnic_q++, queue++) {
            unsigned int bmsk_queue;

            bmsk_queue = map_natural_to_bitmask(queue);
            ring_rids[bmsk_queue] = vnic;
        }
    }

    /*
     * Setup the DMA configuration registers
     */
    cfg_tmp.__raw = 0;
    /* Signal only configuration for null messages */
    cfg_tmp.signal_only_odd = 1;
    cfg_tmp.target_64_odd = 1;
    cfg_tmp.cpp_target_odd = 7;
    /* Regular configuration */
#ifdef NFD_VNIC_NO_HOST
    /* Use signal_only for seqn num generation
     * Don't actually DMA data */
    cfg_tmp.signal_only_even = 0;
#else
    cfg_tmp.signal_only_even = 1;
#endif
    cfg_tmp.end_pad_even = 0;
    cfg_tmp.start_pad_even = 0;
    cfg_tmp.cpp_target_even = 7;
    cfg = cfg_tmp;

    pcie_dma_cfg_set_pair(PCIE_ISL, RX_DATA_CFG_REG, &cfg);
}

void
issue_dma_setup()
{
    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, and PCIe address will be
     * overwritten per transaction.
     * For dma_mode, we technically only want to overwrite the "source"
     * field, i.e. 12 of the 16 bits.
     */
    descr_tmp.length = sizeof(struct nfd_pci_out_rx_desc) - 1;
    descr_tmp.rid_override = 1;
    descr_tmp.trans_class = 0;
    descr_tmp.cpp_token = RX_DATA_DMA_TOKEN;
    descr_tmp.dma_cfg_index = RX_DATA_CFG_REG;
}

/** Parameters list to be filled out as extended */
void
issue_dma()
{
    static __xread struct pci_out_data_batch in_batch;
    struct pci_out_data_batch_msg msg;
    unsigned int n_bat;

    SIGNAL get_sig;

    /* Dummy code, check nn_ring for message, fetch batch,
     * issue signal only DMA per batch */

    if (!nn_ring_empty()) {
        /* We have a message! */
        msg.__raw = nn_ring_get();
        n_bat = msg.num;

        cls_ring_get(RX_DATA_BATCH_RING_NUM, &in_batch, sizeof in_batch,
                     &get_sig);
        wait_for_all(&get_sig);
        __implicit_read(&in_batch, sizeof in_batch);

        /*
         * Increment batch_issued upfront to avoid ambiguity about sequence
         * number zero
         */
        data_dma_seq_issued++;

        descr_tmp.rid = 0; /* XXX Get RID from cls message */

        pcie_dma_set_event(&descr_tmp, RX_DATA_EVENT_TYPE, data_dma_seq_issued);

        descr_tmp.dma_cfg_index = RX_DATA_CFG_REG_SIG_ONLY;

        dma_out_main.pkt0 = descr_tmp;
        __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt0, RX_DATA_DMA_QUEUE,
                       sig_done, &data_sig0);

        descr_tmp.dma_cfg_index = RX_DATA_CFG_REG;

        wait_for_all(&data_sig0);

    }
}

