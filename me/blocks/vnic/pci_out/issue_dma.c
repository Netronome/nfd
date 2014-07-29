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

#include <vnic/pci_out_cfg.h>
#include <vnic/shared/qc.h>
#include <vnic/utils/nn_ring.h>
#include <vnic/utils/pcie.h>

/* XXX Further data will be required when checking for follow on packets
 * volatile set temporarily */
volatile __shared __lmem unsigned int ring_rids[MAX_RX_QUEUES];

__shared __gpr unsigned int data_dma_msg_served = 0; /* XXX necessary? */

__shared __gpr unsigned int data_dma_seq_issued = 0;
__shared __gpr unsigned int data_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_served = 0;

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

}

/** Parameters list to be filled out as extended */
int
issue_dma()
{
    return 0;
}

