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
 * @file          blocks/vnic/pci_in/gather_status.c
 * @brief         Display the state of the gather and service_qc blocks
 */

#include <nfp.h>

#include <nfp/me.h>
#include <std/reg_utils.h>

#include <vnic/pci_in/gather_status.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_in.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/qc.h>

/**
 * Gather state
 */
extern __shared __gpr struct qc_bitmask active_bmsk;
extern __shared __gpr struct qc_bitmask pending_bmsk;
extern __shared __gpr unsigned int  dma_seq_issued;
extern __shared __gpr unsigned int gather_dma_seq_compl;

/**
 * Per queue state to show
 */
extern __shared __lmem struct nfd_in_queue_info queue_data[NFD_IN_MAX_QUEUES];


#define _ZERO_ARRAY     {0, 0, 0, 0, 0, 0, 0, 0}

/**
 * Xfers to display state
 */
static __xread unsigned int status_queue_sel = 0;
static __xwrite struct nfd_in_queue_info status_queue_info = _ZERO_ARRAY;
static __xwrite struct nfd_in_gather_status status_gather = _ZERO_ARRAY;

SIGNAL status_throttle;


void
gather_status_setup()
{
    __implicit_write(&status_queue_sel);

    /* Fix the transfer registers used */
    __assign_relative_register(&status_gather, STATUS_GATHER_START);
    __assign_relative_register(&status_queue_info, STATUS_QUEUE_START);
    __assign_relative_register(&status_queue_sel, STATUS_Q_SEL_START);

    set_alarm(NFD_IN_DBG_GATHER_INTVL, &status_throttle);
}


void
gather_status()
{
    unsigned int bmsk_queue;

    if (signal_test(&status_throttle))
    {
        __implicit_read(&status_gather, sizeof status_gather);
        __implicit_read(&status_queue_info, sizeof status_queue_info);

        /*
         * Convert the natural queue number in the request to a bitmask queue
         * number
         */
        bmsk_queue = NFD_NATQ2BMQ(status_queue_sel);
        __implicit_write(&status_queue_sel);

        /*
         * Collect the independent data from various sources
         */
        status_gather.actv_bmsk_hi = active_bmsk.bmsk_hi;
        status_gather.actv_bmsk_lo = active_bmsk.bmsk_lo;
        status_gather.actv_bmsk_proc = active_bmsk.proc;
        status_gather.pend_bmsk_hi = pending_bmsk.bmsk_hi;
        status_gather.pend_bmsk_lo = pending_bmsk.bmsk_lo;
        status_gather.pend_bmsk_proc = pending_bmsk.proc;
        status_gather.dma_issued = dma_seq_issued;
        status_gather.dma_compl = gather_dma_seq_compl;

        /*
         * Copy the queue info from LM into the status struct
         */
        status_queue_info = queue_data[bmsk_queue];

        /*
         * Reset the alarm
         */
        set_alarm(NFD_IN_DBG_GATHER_INTVL, &status_throttle);
    }
}
