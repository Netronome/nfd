/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/issue_dma_status.c
 * @brief         Display the state of the issue_dma block
 */

#include <nfp.h>

#include <nfp/me.h>
#include <std/reg_utils.h>

#include <vnic/pci_in/issue_dma_status.h>

#include <vnic/pci_in_cfg.h>
#include <vnic/pci_in/pci_in_internal.h>
#include <vnic/pci_in/precache_bufs.h>
#include <vnic/shared/qc.h>


/**
 * Issue_dma state
 */
extern __shared __gpr unsigned int gather_dma_seq_compl;
extern __shared __gpr unsigned int gather_dma_seq_serv;
extern __shared __gpr unsigned int data_dma_seq_issued;
extern __shared __gpr unsigned int data_dma_seq_compl;
extern __shared __gpr unsigned int data_dma_seq_served;
extern __shared __gpr unsigned int data_dma_seq_safe;


/**
 * Per queue state to show
 */
extern __shared __lmem struct tx_dma_state queue_data[MAX_TX_QUEUES];


#define _ZERO_ARRAY     {0, 0, 0, 0, 0, 0, 0, 0}

/**
 * Xfers to display state
 */
static __xread unsigned int status_queue_sel = 0;
static __xwrite struct tx_dma_state status_queue_info = {0, 0, 0, 0};
static __xwrite struct tx_issue_dma_status status_issued = _ZERO_ARRAY;

SIGNAL status_throttle;


void
issue_dma_status_setup()
{
    __implicit_write(&status_queue_sel);

    /* Fix the transfer registers used */
    __assign_relative_register(&status_queue_info, STATUS_QUEUE_START);
    __assign_relative_register(&status_issued, STATUS_ISSUE_DMA_START);
    __assign_relative_register(&status_queue_sel, STATUS_Q_SEL_START);

    set_alarm(TX_DBG_ISSUE_DMA_INTVL, &status_throttle);
}


void
issue_dma_status()
{
    unsigned int bmsk_queue;

    if (signal_test(&status_throttle))
    {
        __implicit_read(&status_queue_info, sizeof status_queue_info);
        __implicit_read(&status_issued, sizeof status_issued);

        /*
         * Convert the natural queue number in the request to a bitmask queue
         * number
         */
        bmsk_queue = map_natural_to_bitmask(status_queue_sel);
        __implicit_write(&status_queue_sel);

        /*
         * Collect the independent data from various sources
         */
        status_issued.gather_dma_seq_compl = gather_dma_seq_compl;
        status_issued.gather_dma_seq_serv = gather_dma_seq_serv;
        status_issued.bufs_avail = precache_bufs_avail();
        status_issued.data_dma_seq_issued = data_dma_seq_issued;
        status_issued.data_dma_seq_compl = data_dma_seq_compl;
        status_issued.data_dma_seq_served = data_dma_seq_served;
        status_issued.data_dma_seq_safe = data_dma_seq_safe;

        /*
         * Copy the queue info from LM into the status struct
         */
        status_queue_info = queue_data[bmsk_queue];

        /*
         * Reset the alarm
         */
        set_alarm(TX_DBG_ISSUE_DMA_INTVL, &status_throttle);
    }
}
