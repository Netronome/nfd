/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file          blocks/vnic/pci_out/cache_desc_status.c
 * @brief         Display the state of the cache_desc block
 */

#include <nfp.h>

#include <nfp/me.h>
#include <std/reg_utils.h>

#include <vnic/pci_out/cache_desc_status.h>

#include <vnic/nfd_common.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/qc.h>


/**
 * Per queue state to show
 */
extern __shared __lmem struct nfd_out_queue_info *queue_data;


/**
 * cache_desc state
 */
extern __shared __gpr struct qc_bitmask active_bmsk;
extern __shared __gpr struct qc_bitmask urgent_bmsk;

extern __gpr unsigned int fl_cache_dma_seq_issued;
extern __gpr unsigned int fl_cache_dma_seq_compl;
extern __gpr unsigned int fl_cache_dma_seq_served;


/**
 * desc_dma state
 */
extern __shared __gpr struct qc_bitmask cached_bmsk;
extern __shared __gpr struct qc_bitmask pending_bmsk;

extern __shared __gpr unsigned int desc_dma_issued;
extern __shared __gpr unsigned int desc_dma_compl;
extern __shared __gpr unsigned int desc_dma_served;
extern __shared __gpr unsigned int desc_dma_pkts_served;


#define _ZERO_ARRAY     {0, 0, 0, 0, 0, 0, 0, 0}

/**
 * Xfers to display state
 */
static __xread unsigned int status_queue_sel = 0;
static __xwrite struct nfd_out_queue_info status_queue_info = _ZERO_ARRAY;
static __xwrite struct nfd_out_cache_desc_status status_cache_desc = _ZERO_ARRAY;
static __xwrite struct nfd_out_desc_dma_status status_desc_dma = _ZERO_ARRAY;

SIGNAL status_throttle;


void
cache_desc_status_setup()
{
    __implicit_write(&status_queue_sel);

    /* Fix the transfer registers used */
    __assign_relative_register(&status_cache_desc, STATUS_Q_CACHE_START);
    __assign_relative_register(&status_desc_dma, STATUS_Q_DMA_START);
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
        __implicit_read(&status_desc_dma, sizeof status_desc_dma);
        __implicit_read(&status_queue_info, sizeof status_queue_info);

        /*
         * Convert the natural queue number in the request to a bitmask queue
         * number
         */
        bmsk_queue = NFD_NATQ2BMQ(status_queue_sel);
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

        status_cache_desc.reserved = 0;

        /*
         * Collect desc_dma data
         */
        status_desc_dma.cached_bmsk_hi = cached_bmsk.bmsk_hi;
        status_desc_dma.cached_bmsk_lo = cached_bmsk.bmsk_lo;
        status_desc_dma.pending_bmsk_hi = pending_bmsk.bmsk_hi;
        status_desc_dma.pending_bmsk_lo = pending_bmsk.bmsk_lo;

        status_desc_dma.desc_dma_issued = desc_dma_issued;
        status_desc_dma.desc_dma_compl = desc_dma_compl;
        status_desc_dma.desc_dma_served = desc_dma_served;

        status_desc_dma.desc_dma_pkts_served = desc_dma_pkts_served;

        /*
         * Reset the alarm
         */
        set_alarm(NFD_OUT_DBG_CACHE_DESC_INTVL, &status_throttle);
    }
}
