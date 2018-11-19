/*
 * Copyright (C) 2014-2019,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/pci_in/service_qc.c
 * @brief         Code to maintain and access the TX.W mask from the queue
 *                controller
 */

#if defined(__NFP_IS_6XXX)
    #include <nfp6000/nfp_event.h>
#elif defined(__NFP_IS_38XX)
    #include <nfp3800/nfp_event.h>
#else
    #error "Unsupported chip type"
#endif

#include <assert.h>
#include <nfp.h>

#include <vnic/nfd_common.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/qc.h>


/**
 * QC event update variables
 */
static __xread struct qc_xfers qc_ap_xfers;

static SIGNAL qc_ap_s0;
static SIGNAL qc_ap_s1;
static SIGNAL qc_ap_s2;
static SIGNAL qc_ap_s3;
static SIGNAL qc_ap_s4;
static SIGNAL qc_ap_s5;
static SIGNAL qc_ap_s6;
static SIGNAL qc_ap_s7;


/**
 * State variables for PCI.IN queue controller accesses
 */
extern __shared __gpr struct qc_bitmask active_bmsk;
extern __shared __gpr struct qc_bitmask cfg_queue_bmsk;

/*
 * Variables for PCI.OUT queue controller accesses
 */
__remote volatile SIGNAL nfd_out_cache_bmsk_sig;
NFD_OUT_ACTIVE_BMSK_DECLARE;


/* XXX nfd_cfg_internal.c defines this currently */
__intrinsic void send_interthread_sig(unsigned int dst_me, unsigned int ctx,
                                      unsigned int sig_no);


/* XXX rename */
/**
 * Initialise the PCI.IN queue controller queues
 */
void
service_qc_setup ()
{
#ifdef NFD_VNIC_SIM
    /* Set QC to generate events including 8bit queue numbers.
     * The configurator performs this job on hardware. */
    set_Qctl8bitQnum();
#endif

    /* Zero bitmasks */
    init_bitmasks(&cfg_queue_bmsk);

    /* Configure autopush filters */
    init_bitmask_filters(&qc_ap_xfers, &qc_ap_s0, &qc_ap_s1,
                         &qc_ap_s2, &qc_ap_s3, &qc_ap_s4, &qc_ap_s5,
                         &qc_ap_s6, &qc_ap_s7, NFD_EVENT_DATA<<6,
                         NFD_EVENT_FILTER_START);

}


/**
 * Use API provided by shared/qc to update queue state
 */
void
service_qc()
{
    __shared __gpr struct qc_bmsk_updates updates[3];

    /* Check event filters */
    check_bitmask_filters(updates, &qc_ap_xfers, &qc_ap_s0, &qc_ap_s1,
                         &qc_ap_s2, &qc_ap_s3, &qc_ap_s4, &qc_ap_s5,
                         &qc_ap_s6, &qc_ap_s7, NFD_EVENT_FILTER_START);

    /* Copy over PCI.IN bitmasks */
    active_bmsk.bmsk_lo |= updates[NFD_IN_TX_QUEUE].bmsk_lo;
    active_bmsk.bmsk_hi |= updates[NFD_IN_TX_QUEUE].bmsk_hi;

    /* Copy over config bitmasks */
    cfg_queue_bmsk.bmsk_lo |= updates[NFD_CFG_QUEUE].bmsk_lo;
    cfg_queue_bmsk.bmsk_hi |= updates[NFD_CFG_QUEUE].bmsk_hi;

    /* Send FL bitmasks to PCI.OUT */
    if (updates[NFD_OUT_FL_QUEUE].bmsk_lo | updates[NFD_OUT_FL_QUEUE].bmsk_hi) {
        __xwrite unsigned int update_wr[2];

        update_wr[0] = updates[NFD_OUT_FL_QUEUE].bmsk_lo;
        update_wr[1] = updates[NFD_OUT_FL_QUEUE].bmsk_hi;

        mem_bitset(update_wr, NFD_OUT_ACTIVE_BMSK_LINK, sizeof update_wr);
        send_interthread_sig(NFD_OUT_CACHE_ME, 0,
                             __signal_number(&nfd_out_cache_bmsk_sig,
                                             NFD_OUT_CACHE_ME));
    }
}
