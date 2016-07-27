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
 * @file          blocks/vnic/pci_in/issue_dma_status.c
 * @brief         Display the state of the issue_dma block
 */

#include <nfp.h>

#include <nfp/me.h>
#include <std/reg_utils.h>

#include <vnic/pci_in/issue_dma_status.h>

#include <vnic/nfd_common.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/qc.h>


/**
 * Issue_dma state
 */
extern __shared __gpr unsigned int gather_dma_seq_compl;
extern __shared __gpr unsigned int gather_dma_seq_serv;
extern __shared __gpr unsigned int data_dma_seq_issued;
extern __shared __gpr unsigned int data_dma_seq_compl;
extern __shared __gpr unsigned int data_dma_seq_served;
extern __shared __gpr unsigned int data_dma_seq_safe;
extern __shared __gpr unsigned int jumbo_dma_seq_issued;
extern __shared __gpr unsigned int jumbo_dma_seq_compl;
extern __shared __gpr unsigned int jumbo_cnt;


/**
 * Per queue state to show
 */
extern __shared __lmem struct nfd_in_dma_state queue_data[NFD_IN_MAX_QUEUES];


#define _ZERO_ARRAY     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0}

/**
 * Xfers to display state
 */
static __xread unsigned int status_queue_sel = 0;
static __xwrite struct nfd_in_dma_state status_queue_info = {0, 0, 0, 0,
                                                             0, 0, 0, 0};
static __xwrite struct nfd_in_issue_dma_status status_issued = _ZERO_ARRAY;

SIGNAL status_throttle;


void
issue_dma_status_setup()
{
    __implicit_write(&status_queue_sel);

    /* Fix the transfer registers used */
    __assign_relative_register(&status_queue_info, STATUS_QUEUE_START1);
    __assign_relative_register(&status_issued, STATUS_ISSUE_DMA_START);
    __assign_relative_register(&status_queue_sel, STATUS_Q_SEL_START1);

    set_alarm(NFD_IN_DBG_ISSUE_DMA_INTVL, &status_throttle);
}


void
issue_dma_status()
{
    unsigned int bmsk_queue;

    if (signal_test(&status_throttle))
    {
        unsigned int resv_dma_avail;

        __implicit_read(&status_queue_info, sizeof status_queue_info);
        __implicit_read(&status_issued, sizeof status_issued);

        /*
         * Convert the natural queue number in the request to a bitmask queue
         * number
         */
        bmsk_queue = NFD_NATQ2BMQ(status_queue_sel);
        __implicit_write(&status_queue_sel);

        /*
         * Collect the independent data from various sources
         */
        status_issued.gather_dma_seq_compl = gather_dma_seq_compl;
        status_issued.gather_dma_seq_serv = gather_dma_seq_serv;
        status_issued.bufs_avail = precache_bufs_avail();
        resv_dma_avail = (NFD_IN_JUMBO_MAX_IN_FLIGHT + jumbo_dma_seq_compl -
                          jumbo_dma_seq_issued);
        status_issued.resv_dma_avail = resv_dma_avail;
        status_issued.data_dma_seq_issued = data_dma_seq_issued;
        status_issued.data_dma_seq_compl = data_dma_seq_compl;
        status_issued.data_dma_seq_served = data_dma_seq_served;
        status_issued.data_dma_seq_safe = data_dma_seq_safe;
        status_issued.jumbo_cnt = jumbo_cnt;

        /*
         * Copy the queue info from LM into the status struct
         */
        status_queue_info = queue_data[bmsk_queue];

        /*
         * Reset the alarm
         */
        set_alarm(NFD_IN_DBG_ISSUE_DMA_INTVL, &status_throttle);
    }
}
