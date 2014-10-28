/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc_status.c
 * @brief         Display the state of the cache_desc block
 */

#include <nfp.h>

#include <nfp/me.h>
#include <std/reg_utils.h>

#include <vnic/pci_out/cache_desc_status.h>

#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/qc.h>


/**
 * Per queue state to show
 */
extern __shared __lmem struct nfd_out_queue_info queue_data[64];


/**
 * cache_desc state
 */
extern __shared __gpr struct qc_bitmask active_bmsk;
extern __shared __gpr struct qc_bitmask urgent_bmsk;

extern __gpr unsigned int fl_cache_dma_seq_issued;
extern __gpr unsigned int fl_cache_dma_seq_compl;
extern __gpr unsigned int fl_cache_dma_seq_served;


/**
 * stage_batch state
 */
extern __shared __gpr unsigned int batch_issued;
extern __shared __gpr unsigned int batch_safe;

extern __shared __gpr unsigned int data_dma_compl;
extern __shared __gpr unsigned int desc_batch_served;

extern __shared __gpr unsigned int desc_dma_issued;
extern __shared __gpr unsigned int desc_dma_compl;
extern __shared __gpr unsigned int desc_dma_safe;


#define _ZERO_ARRAY     {0, 0, 0, 0, 0, 0, 0, 0}

/**
 * Xfers to display state
 */
static __xread unsigned int status_queue_sel = 0;
static __xwrite struct nfd_out_queue_info status_queue_info = _ZERO_ARRAY;
static __xwrite struct nfd_out_cache_desc_status status_cache_desc = _ZERO_ARRAY;
static __xwrite struct nfd_out_stage_batch_status status_stage = _ZERO_ARRAY;

SIGNAL status_throttle;


void
cache_desc_status_setup()
{
    __implicit_write(&status_queue_sel);

    /* Fix the transfer registers used */
    __assign_relative_register(&status_cache_desc, STATUS_Q_CACHE_START);
    __assign_relative_register(&status_stage, STATUS_Q_STAGE_START);
    __assign_relative_register(&status_queue_info, STATUS_Q_INFO_START);
    __assign_relative_register(&status_queue_sel, STATUS_Q_SEL_START);

    set_alarm(NFD_OUT_DBG_CACHE_DESC_INTVL, &status_throttle);
}


void
cache_desc_status()
{
    unsigned int bmsk_queue;

    if (signal_test(&status_throttle))
    {
        __implicit_read(&status_cache_desc, sizeof status_cache_desc);
        __implicit_read(&status_stage, sizeof status_stage);
        __implicit_read(&status_queue_info, sizeof status_queue_info);

        /*
         * Convert the natural queue number in the request to a bitmask queue
         * number
         */
        bmsk_queue = map_natural_to_bitmask(status_queue_sel);
        __implicit_write(&status_queue_sel);

        /*
         * Copy the queue info from LM into the status struct
         */
        status_queue_info = queue_data[bmsk_queue];

        /*
         * Collect cache_desc data
         */
        status_cache_desc.active_bmsk_hi = active_bmsk.bmsk_hi;
        status_cache_desc.active_bmsk_lo = active_bmsk.bmsk_lo;
        status_cache_desc.urgent_bmsk_hi = urgent_bmsk.bmsk_hi;
        status_cache_desc.urgent_bmsk_lo = urgent_bmsk.bmsk_lo;

        status_cache_desc.fl_cache_issued = fl_cache_dma_seq_issued;
        status_cache_desc.fl_cache_compl = fl_cache_dma_seq_compl;
        status_cache_desc.fl_cache_served = fl_cache_dma_seq_served;


        /*
         * Collect stage_batch data
         */
        status_stage.batch_issued = batch_issued;
        status_stage.batch_safe = batch_safe;
        status_stage.data_dma_compl = data_dma_compl;
        status_stage.desc_batch_served = desc_batch_served;

        status_stage.desc_dma_issued = desc_dma_issued;
        status_stage.desc_dma_compl = desc_dma_compl;
        status_stage.desc_dma_safe = desc_dma_safe;

        /*
         * Reset the alarm
         */
        set_alarm(NFD_OUT_DBG_CACHE_DESC_INTVL, &status_throttle);
    }
}
