/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/gather_status.c
 * @brief         Display the state of the gather and service_qc blocks
 */

#include <nfp.h>

#include <vnic/pci_in/gather_status.h>

#include <vnic/pci_in/pci_in_internal.h>
#include <vnic/shared/qc.h>

/**
 * Queue independent state
 */
extern __shared __gpr struct qc_bitmask active_bmsk;
extern __shared __gpr struct qc_bitmask pending_bmsk;
extern __shared __gpr unsigned int  dma_seq_issued;
extern __shared __gpr unsigned int gather_dma_seq_compl;

/**
 * Per queue state to show
 */
extern __shared __lmem struct tx_queue_info queue_data[64];

/**
 * Xfers to display state
 */
static __xread unsigned int status_queue_sel = 0;
static __xwrite struct tx_queue_info status_queue_info;
static __xwrite struct tx_gather_status status_queue_indep;



void
init_gather_status()
{
    __implicit_write(&status_queue_sel);

    /* Fix the transfer registers used */
    __assign_relative_register(&status_queue_indep, STATUS_Q_INDEP_START);
    __assign_relative_register(&status_queue_info, STATUS_Q_INFO_START);
    __assign_relative_register(&status_queue_sel, STATUS_Q_SEL_START);

    /* XXX replace with mr_zero type command */
    status_queue_indep.actv_bmsk_hi = 0;
    status_queue_indep.actv_bmsk_lo = 0;
    status_queue_indep.actv_bmsk_proc = 0;
    status_queue_indep.pend_bmsk_hi = 0;
    status_queue_indep.pend_bmsk_lo = 0;
    status_queue_indep.pend_bmsk_proc = 0;
    status_queue_indep.dma_issued = 0;
    status_queue_indep.dma_compl = 0;

    status_queue_info.tx_w = 0;
    status_queue_info.tx_s = 0;
    status_queue_info.ring_sz_msk = 0;
    status_queue_info.requester_id = 0;
    status_queue_info.ring_base_addr = 0;
    status_queue_info.dummy[0] = 0;
    status_queue_info.dummy[1] = 0;
    status_queue_info.dummy[2] = 0;



}


void
gather_status()
{
    unsigned int bmsk_queue;

    __implicit_read(&status_queue_info, sizeof status_queue_info);
    __implicit_read(&status_queue_indep, sizeof status_queue_indep);

    /*
     * Convert the natural queue number in the request
     * to a bitmask queue number
     */
    bmsk_queue = map_natural_to_bitmask(status_queue_sel);
    __implicit_write(&status_queue_sel);

    /*
     * Collect the independent data from various sources
     */
    status_queue_indep.actv_bmsk_hi = active_bmsk.bmsk_hi;
    status_queue_indep.actv_bmsk_lo = active_bmsk.bmsk_lo;
    status_queue_indep.actv_bmsk_proc = active_bmsk.proc;
    status_queue_indep.pend_bmsk_hi = pending_bmsk.bmsk_hi;
    status_queue_indep.pend_bmsk_lo = pending_bmsk.bmsk_lo;
    status_queue_indep.pend_bmsk_proc = pending_bmsk.proc;
    status_queue_indep.dma_issued = dma_seq_issued;
    status_queue_indep.dma_compl = gather_dma_seq_compl;

    /*
     * Copy the queue info from LM into the status struct
     */
    status_queue_info = queue_data[bmsk_queue];
}
