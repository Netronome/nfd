/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc_status.c
 * @brief         Display the state of the cache_desc block
 */

#include <nfp.h>

#include <std/reg_utils.h>

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
    __gpr struct rx_queue_info info_tmp;

    __implicit_write(&status_queue_sel);

    /* Fix the transfer registers used */
    __assign_relative_register(&status_queue_info, STATUS_Q_INFO_START);
    __assign_relative_register(&status_queue_sel, STATUS_Q_SEL_START);

    reg_zero(&info_tmp, sizeof info_tmp);
    status_queue_info = info_tmp;
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
