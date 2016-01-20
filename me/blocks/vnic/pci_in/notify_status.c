/*
 * Copyright (C) 2015-2016,  Netronome Systems, Inc.  All rights reserved.
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
extern __shared __gpr unsigned int data_dma_seq_compl0;
extern __shared __gpr unsigned int data_dma_seq_served0;
extern __shared __gpr unsigned int data_dma_seq_compl1;
extern __shared __gpr unsigned int data_dma_seq_served1;

/**
 * Xfers to display state
 */
static __xwrite struct nfd_in_notify_status status_notify0 = {0, 0};
static __xwrite struct nfd_in_notify_status status_notify1 = {0, 0};

SIGNAL status_throttle;


void
notify_status_setup()
{
    /* Fix the transfer registers used */
    __assign_relative_register(&status_notify0, STATUS_NOTIFY_START);
    __assign_relative_register(&status_notify1, (STATUS_NOTIFY_START + 2));

    set_alarm(NFD_IN_DBG_GATHER_INTVL, &status_throttle);
}


void
notify_status()
{
    unsigned int bmsk_queue;

    if (signal_test(&status_throttle))
    {
        __implicit_read(&status_notify0, sizeof status_notify0);
        __implicit_read(&status_notify1, sizeof status_notify1);

        /*
         * Collect the notify state from various sources
         */
        status_notify0.dma_compl = data_dma_seq_compl0;
        status_notify0.dma_served = data_dma_seq_served0;
        status_notify1.dma_compl = data_dma_seq_compl1;
        status_notify1.dma_served = data_dma_seq_served1;

        /*
         * Reset the alarm
         */
        set_alarm(NFD_IN_DBG_NOTIFY_INTVL, &status_throttle);
    }
}
