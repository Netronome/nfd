/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc_status.c
 * @brief         Display the state of the cache_desc block
 */

#include <nfp.h>

#include <vnic/pci_out/cache_desc_status.h>

#include <vnic/pci_out/pci_out_internal.h>
#include <vnic/shared/qc.h>

/**
 * Per queue state to show
 */
extern __shared __lmem struct rx_queue_info queue_data[64];

/**
 * Xfers to display state
 */
static __xread unsigned int status_queue_sel = 0;
static __xwrite struct rx_queue_info status_queue_info;


void
cache_desc_status_setup()
{
    __implicit_write(&status_queue_sel);

    /* Fix the transfer registers used */
    __assign_relative_register(&status_queue_info, STATUS_Q_INFO_START);
    __assign_relative_register(&status_queue_sel, STATUS_Q_SEL_START);

    /* XXX replace with mr_zero type command */
    status_queue_info.fl_w = 0;
    status_queue_info.fl_s = 0;
    status_queue_info.ring_sz_msk = 0;
    status_queue_info.requester_id = 0;
    status_queue_info.ring_base_addr = 0;
    status_queue_info.rx_w = 0;
    status_queue_info.dummy[0] = 0;
    status_queue_info.dummy[1] = 0;
}


void
cache_desc_status()
{
    unsigned int bmsk_queue;

    __implicit_read(&status_queue_info, sizeof status_queue_info);

    /*
     * Convert the natural queue number in the request
     * to a bitmask queue number
     */
    bmsk_queue = map_natural_to_bitmask(status_queue_sel);
    __implicit_write(&status_queue_sel);

    /*
     * Copy the queue info from LM into the status struct
     */
    status_queue_info = queue_data[bmsk_queue];
}
