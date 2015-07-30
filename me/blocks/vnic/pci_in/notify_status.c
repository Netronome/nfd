/*
 * Copyright (C) 2015 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/notify_status.c
 * @brief         Display the state of the notify block
 */

#include <nfp.h>

#include <nfp/me.h>
#include <std/reg_utils.h>

#include <vnic/pci_in/notify_status.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_in.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/qc.h>


/**
 * Notify state
 */
extern __shared __gpr unsigned int data_dma_seq_compl;
extern __shared __gpr unsigned int data_dma_seq_served;

/**
 * Xfers to display state
 */
static __xwrite struct nfd_in_notify_status status_notify = {0, 0};

SIGNAL status_throttle;


void
notify_status_setup()
{
    /* Fix the transfer registers used */
    __assign_relative_register(&status_notify, STATUS_NOTIFY_START);

    set_alarm(NFD_IN_DBG_GATHER_INTVL, &status_throttle);
}


void
notify_status()
{
    unsigned int bmsk_queue;

    if (signal_test(&status_throttle))
    {
        __implicit_read(&status_notify, sizeof status_notify);

        /*
         * Collect the notify state from various sources
         */
        status_notify.dma_compl = data_dma_seq_compl;
        status_notify.dma_served = data_dma_seq_served;

        /*
         * Reset the alarm
         */
        set_alarm(NFD_IN_DBG_NOTIFY_INTVL, &status_throttle);
    }
}
