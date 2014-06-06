/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/issue_dma.c
 * @brief         Code to DMA packet data from the NFP
 */

/* XXX probably better to name the PCI.OUT and PCI.IN files differently */

#include <assert.h>
#include <nfp.h>

#include <vnic/pci_out/issue_dma.h>

#include <vnic/pci_out_cfg.h>
#include <vnic/shared/qc.h>

/* XXX Further data will be required when checking for follow on packets
 * volatile set temporarily */
volatile __shared __lmem unsigned int ring_rids[MAX_RX_QUEUES];

void
issue_dma_setup_shared()
{
    /* XXX complete non-per-queue data */
    unsigned int queue;
    unsigned int vnic;
    unsigned int vnic_q;

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
