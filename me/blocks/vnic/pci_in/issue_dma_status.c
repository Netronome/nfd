/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/issue_dma_status.c
 * @brief         Display the state of the issue_dma block
 */

#include <nfp.h>

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

/**
 * Xfers to display state
 */
static __xread unsigned int status_queue_sel = 0;
static __xwrite struct tx_dma_state status_queue;
static __xwrite struct tx_issue_dma_status status_issued;



void
issue_dma_status_setup()
{
    __gpr struct tx_dma_state state_tmp;
    __gpr struct tx_issue_dma_status issued_tmp;

    /* Fix the transfer registers used */
    __assign_relative_register(&status_queue, STATUS_QUEUE_START);
    __assign_relative_register(&status_issued, STATUS_ISSUE_DMA_START);
    __assign_relative_register(&status_queue_sel, STATUS_Q_SEL_START);

    __implicit_write(&status_queue_sel);

    reg_zero(&state_tmp, sizeof state_tmp);
    status_queue = state_tmp;

    reg_zero(&issued_tmp, sizeof issued_tmp);
    status_issued = issued_tmp;
}


void
issue_dma_status()
{
    unsigned int bmsk_queue;

    __implicit_read(&status_queue, sizeof status_queue);
    __implicit_read(&status_issued, sizeof status_issued);

    /*
     * Convert the natural queue number in the request
     * to a bitmask queue number
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
    status_queue = queue_data[bmsk_queue];
}
