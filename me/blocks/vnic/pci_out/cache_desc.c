/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc.c
 * @brief         Code to cache FL descriptors from pending queues
 */

#include <assert.h>
#include <nfp.h>

#include <vnic/pci_out/cache_desc.h>

#include <vnic/pci_out/pci_out_internal.h>
#include <vnic/shared/qc.h>

__shared __lmem struct rx_queue_info queue_data[MAX_VNICS * MAX_VNIC_QUEUES];

void
cache_desc_setup_shared()
{
    /* XXX complete non-per-queue data */
}


__intrinsic void
cache_desc_vnic_setup(void *cfg_msg_in, unsigned int queue_size)
{
    struct pci_out_cfg_msg *cfg_msg;

    /* XXX make this code maintain state and re-enter
     * to support case where data is read from emem */
    unsigned int start_queue;
    unsigned int end_queue;
    unsigned int queue;

    ctassert(__is_log2(MAX_VNICS));
    ctassert(__is_log2(MAX_VNIC_QUEUES));

    cfg_msg = cfg_msg_in;
    start_queue = cfg_msg->vnic * MAX_VNIC_QUEUES;
    end_queue = start_queue + MAX_VNIC_QUEUES;

    /*
     * XXX Set up per queue data to dummy values
     */
    for (queue = start_queue; queue < end_queue; queue++) {
        unsigned int bmsk_queue = map_natural_to_bitmask(queue);

        queue_data[bmsk_queue].fl_w = 0;
        queue_data[bmsk_queue].fl_s = 0;
        queue_data[bmsk_queue].requester_id = cfg_msg->vnic;
        /* TEMP */
        queue_data[bmsk_queue].spare0 = 0;
        queue_data[bmsk_queue].ring_base_hi = 0;
        queue_data[bmsk_queue].ring_base_lo = (0x56<<24) | (queue<<16);
        queue_data[bmsk_queue].ring_sz_msk = ((1 << queue_size) - 1);
        queue_data[bmsk_queue].rx_w = 0;
    }

    /* Indicate completion */
    cfg_msg->msg_valid = 0;
}
