/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
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
 * State variables for PCI.IN queue controller accesses
 */
static __xread struct qc_xfers tx_ap_xfers;

static SIGNAL tx_ap_s0;
static SIGNAL tx_ap_s1;
static SIGNAL tx_ap_s2;
static SIGNAL tx_ap_s3;

__shared __gpr struct qc_bitmask active_bmsk;
__shared __gpr struct qc_bitmask pending_bmsk;

/*
 * State variables for PCI.OUT queue controller accesses
 */
static __xread struct qc_xfers rx_ap_xfers;

static SIGNAL rx_ap_s0;
static SIGNAL rx_ap_s1;
static SIGNAL rx_ap_s2;
static SIGNAL rx_ap_s3;

__shared __gpr struct qc_bitmask pci_out_active_bmsk;
__remote volatile SIGNAL nfd_out_cache_bmsk_sig;

extern __shared __gpr struct qc_bitmask cfg_queue_bmsk;

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
    struct nfp_em_filter_status tmp_status;
    __cls struct event_cls_filter *tmp_event_filter;

#ifdef NFD_VNIC_SIM
    /* Set QC to generate events including 8bit queue numbers.
     * The configurator performs this job on hardware. */
    set_Qctl8bitQnum();
#endif

    /* Zero bitmasks */
    init_bitmasks(&active_bmsk);
    init_bitmasks(&pending_bmsk);

    init_bitmasks(&cfg_queue_bmsk);

    /* Configure nfd_in autopush filters */
    init_bitmask_filters(&tx_ap_xfers, &tx_ap_s0, &tx_ap_s1,
                         &tx_ap_s2, &tx_ap_s3,
                         (NFD_IN_Q_EVENT_DATA<<6) | NFD_IN_Q_START,
                         NFP_EVENT_TYPE_FIFO_NOT_EMPTY,
                         NFD_IN_Q_EVENT_START);

    /* Configure nfd_out autopush filters */
    init_bitmask_filters(&rx_ap_xfers, &rx_ap_s0, &rx_ap_s1, &rx_ap_s2,
                         &rx_ap_s3,(NFD_OUT_Q_EVENT_DATA<<6) | NFD_OUT_Q_START,
                         NFP_EVENT_TYPE_FIFO_ABOVE_WM,
                         NFD_OUT_Q_EVENT_START);


    /* XXX temporarily setup a general last event filter to see what events
     * we are triggering. */
    tmp_status.__raw = 0; /* bitmask32 requires no further settings */
    tmp_event_filter = event_cls_filter_handle(8);
    event_cls_filter_setup(tmp_event_filter, NFP_EM_FILTER_MASK_TYPE_LASTEV,
                           ((4 | PCIE_ISL)<<18), 0xFC0000, tmp_status);

    /* XXX set all QC queues to a safe state! */
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

    queue = NFD_BUILD_NATQ(cfg_msg->vnic, queue);
    bmsk_queue = NFD_NATQ2BMQ(queue);

    txq.watermark    = NFP_QC_STS_HI_WATERMARK_4;
    txq.event_data   = NFD_IN_Q_EVENT_DATA;
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
        if (cfg_msg->vnic != NFD_MAX_VFS) {
            queue_data[bmsk_queue].requester_id = (cfg_msg->vnic +
                                                   NFD_CFG_VF_OFFSET);
        }
        queue_data[bmsk_queue].spare0 = 0;
        queue_data[bmsk_queue].up = 1;
        queue_data[bmsk_queue].ring_base_hi = ring_base[1] & 0xFF;
        queue_data[bmsk_queue].ring_base_lo = ring_base[0];

        txq.event_type   = NFP_QC_STS_LO_EVENT_TYPE_NOT_EMPTY;
        txq.size         = ring_sz - 8; /* XXX add define for size shift */
        qc_init_queue(PCIE_ISL, (queue<<1) | NFD_IN_Q_START, &txq);
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
        qc_init_queue(PCIE_ISL, (queue<<1) | NFD_IN_Q_START, &txq);
    }
}


/**
 * Use API provided by shared/qc to update queue state
 */
void
service_qc()
{
    struct check_queues_consts c;

    /* Check nfd_in bitmasks */
    check_bitmask_filters(&active_bmsk, &cfg_queue_bmsk, &tx_ap_xfers,
                          &tx_ap_s0, &tx_ap_s1, &tx_ap_s2, &tx_ap_s3,
                          NFD_IN_Q_EVENT_START);

    /* Check nfd_out bitmasks */
    pci_out_active_bmsk.bmsk_lo = 0;
    pci_out_active_bmsk.bmsk_hi = 0;
    check_bitmask_filters(&pci_out_active_bmsk, &cfg_queue_bmsk, &rx_ap_xfers,
                          &rx_ap_s0, &rx_ap_s1, &rx_ap_s2, &rx_ap_s3,
                          NFD_OUT_Q_EVENT_START);

    if (pci_out_active_bmsk.bmsk_lo | pci_out_active_bmsk.bmsk_hi) {
        __xwrite unsigned int update[2];

        update[0] = pci_out_active_bmsk.bmsk_lo;
        update[1] = pci_out_active_bmsk.bmsk_hi;

        mem_bitset(update, NFD_OUT_ACTIVE_BMSK_LINK, sizeof update);
        send_interthread_sig(NFD_OUT_CACHE_ME, 0,
                             __signal_number(&nfd_out_cache_bmsk_sig,
                                             NFD_OUT_CACHE_ME));
    }

    /* Check queues */
    c.pcie_isl =       PCIE_ISL;
    c.max_retries =    NFD_IN_MAX_RETRIES;
    c.batch_sz =       NFD_IN_BATCH_SZ;
    c.base_queue_num = NFD_IN_Q_START;
    c.pending_test =   NFD_IN_PENDING_TEST;
    c.event_data =     NFD_IN_Q_EVENT_DATA;
    c.event_type =     NFP_QC_STS_LO_EVENT_TYPE_NOT_EMPTY;
    check_queues(&queue_data, &active_bmsk, &pending_bmsk, &c);
}
