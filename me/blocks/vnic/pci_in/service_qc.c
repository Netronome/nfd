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
 * @file          blocks/vnic/pci_in/service_qc.c
 * @brief         Code to maintain and access the TX.W mask from the queue
 *                controller
 */

#include <assert.h>
#include <nfp.h>

#include <nfp6000/nfp_event.h>

#include <vnic/pci_in.h>

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
__shared __gpr struct qc_bitmask active_bmsk;
__shared __gpr struct qc_bitmask pending_bmsk;

extern __shared __gpr struct qc_bitmask cfg_queue_bmsk;

/*
 * Variables for PCI.OUT queue controller accesses
 */
__remote volatile SIGNAL nfd_out_cache_bmsk_sig;
NFD_OUT_ACTIVE_BMSK_DECLARE;


__shared __lmem struct nfd_in_queue_info queue_data[NFD_IN_MAX_QUEUES];


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
    init_bitmasks(&active_bmsk);
    init_bitmasks(&pending_bmsk);

    init_bitmasks(&cfg_queue_bmsk);

    /* Configure autopush filters */
    init_bitmask_filters(&qc_ap_xfers, &qc_ap_s0, &qc_ap_s1,
                         &qc_ap_s2, &qc_ap_s3, &qc_ap_s4, &qc_ap_s5,
                         &qc_ap_s6, &qc_ap_s7, NFD_EVENT_DATA<<6,
                         NFD_EVENT_FILTER_START);

}


/**
 * Change the configuration of the queues and rings associated with a vNIC
 * @param cfg_msg       configuration information concerning the change
 *
 * This method performs changes to the local state for a vNIC.  The 'cfg_msg'
 * struct is used in conjunction with 'nfd_cfg_proc_msg' and internal nfd_cfg
 * state to determine a particular queue to change each time this method is
 * called.  See nfd_cfg.h for further information.
 */
__intrinsic void
service_qc_vnic_setup(struct nfd_cfg_msg *cfg_msg)
{
    struct qc_queue_config txq;
    unsigned int queue;
    unsigned char ring_sz;
    unsigned int ring_base[2];
    __gpr unsigned int bmsk_queue;

    nfd_cfg_proc_msg(cfg_msg, &queue, &ring_sz, ring_base, NFD_CFG_PCI_IN0);

    if (cfg_msg->error || !cfg_msg->interested) {
        return;
    }

    queue = NFD_VID2NATQ(cfg_msg->vid, queue);
    bmsk_queue = NFD_NATQ2BMQ(queue);

    txq.watermark    = NFP_QC_STS_HI_WATERMARK_4;
    txq.event_data   = NFD_EVENT_DATA;
    txq.ptr          = 0;

    if (cfg_msg->up_bit && !queue_data[bmsk_queue].up) {
        /* Up the queue:
         * - Set ring size and requester ID info
         * - (Re)clear queue pointers in case something changed them
         *   while down */
        queue_data[bmsk_queue].tx_w = 0;
        queue_data[bmsk_queue].tx_s = 0;
        queue_data[bmsk_queue].ring_sz_msk = ((1 << ring_sz) - 1);
        queue_data[bmsk_queue].requester_id = 0;
        if (NFD_VID_IS_VF(cfg_msg->vid)) {
            queue_data[bmsk_queue].requester_id = (cfg_msg->vid +
                                                   NFD_CFG_VF_OFFSET);
        }
        queue_data[bmsk_queue].spare0 = 0;
        queue_data[bmsk_queue].up = 1;
        queue_data[bmsk_queue].ring_base_hi = ring_base[1] & 0xFF;
        queue_data[bmsk_queue].ring_base_lo = ring_base[0];

        txq.event_type   = NFP_QC_STS_LO_EVENT_TYPE_NOT_EMPTY;
        txq.size         = ring_sz - 8; /* XXX add define for size shift */
        qc_init_queue(PCIE_ISL, NFD_NATQ2QC(queue, NFD_IN_TX_QUEUE), &txq);
    } else if (!cfg_msg->up_bit && queue_data[bmsk_queue].up) {
        /* Down the queue:
         * - Prevent it issuing events
         * - Clear active_msk bit
         * - Clear pending_msk bit
         * - Clear the proc bitmask bit?
         * - Clear tx_w and tx_s
         * - Try to count pending packets? Host responsibility? */

        /* Clear active and pending bitmask bits */
        clear_queue(&bmsk_queue, &active_bmsk);
        clear_queue(&bmsk_queue, &pending_bmsk);

        /* Clear queue LM state */
        queue_data[bmsk_queue].tx_w = 0;
        queue_data[bmsk_queue].tx_s = 0;
        queue_data[bmsk_queue].up = 0;

        /* Set QC queue to safe state (known size, no events, zeroed ptrs) */
        txq.event_type   = NFP_QC_STS_LO_EVENT_TYPE_NEVER;
        txq.size         = 0;
        qc_init_queue(PCIE_ISL, NFD_NATQ2QC(queue, NFD_IN_TX_QUEUE), &txq);
    }
}


/**
 * Use API provided by shared/qc to update queue state
 */
void
service_qc()
{
    struct check_queues_consts c;
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

    /* Check queues */
    c.pcie_isl =       PCIE_ISL;
    c.max_retries =    NFD_IN_MAX_RETRIES;
    c.batch_sz =       NFD_IN_BATCH_SZ;
    c.queue_type =     NFD_IN_TX_QUEUE;
    c.pending_test =   NFD_IN_PENDING_TEST;
    c.event_data =     NFD_EVENT_DATA;
    c.event_type =     NFP_QC_STS_LO_EVENT_TYPE_NOT_EMPTY;
    check_queues(&queue_data, &active_bmsk, &pending_bmsk, &c);
}
