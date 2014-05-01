/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/service_qc.c
 * @brief         Code to maintain and access the TX.W mask from the queue
 *                controller
 */

#include <assert.h>
#include <nfp.h>

#include <vnic/pci_in/service_qc.h>

#include <vnic/pci_in_cfg.h>
#include <vnic/pci_in/pci_in_internal.h>
#include <vnic/shared/qc.h>

/**
 * State variables for PCI.IN queue controller accesses
 */
static __xread struct qc_xfers tx_ap_xfers;

static volatile SIGNAL tx_ap_s0;
static volatile SIGNAL tx_ap_s1;
static volatile SIGNAL tx_ap_s2;
static volatile SIGNAL tx_ap_s3;

__shared __gpr struct qc_bitmask active_bmsk;
__shared __gpr struct qc_bitmask pending_bmsk;

__shared __lmem struct tx_queue_info queue_data[64];

/**
 * Initialise the PCI.IN queue controller queues
 * @param max_vnics         Maximum number of VNICs that will be exposed
 * @param max_vnic_queues   Maximum number of queues provided per VNIC
 * @param queue_size        Preferred size of queue controller queues
 *
 * */
void
init_service_qc ()
{
    struct nfp_em_filter_status tmp_status;
    __cls struct event_cls_filter *tmp_event_filter;

    /* XXX Set QC to generate events including 8bit queue numbers.
     * Ultimately the configurator will perform this job. */
    set_Qctl8bitQnum();

    /* Zero bitmasks */
    init_bitmasks(&active_bmsk);
    init_bitmasks(&pending_bmsk);

    /* Configure TXQ autopush filters */
    init_bitmask_filters(&tx_ap_xfers, &tx_ap_s0, &tx_ap_s1, &tx_ap_s2,
                         &tx_ap_s3,(TXQ_EVENT_DATA<<6), TXQ_EVENT_START);


    /* XXX temporarily setup a general last event filter to see what events
     * we are triggering. */
    tmp_status.__raw = 0; /* bitmask32 requires no further settings */
    tmp_event_filter = event_cls_filter_handle(8);
    event_cls_filter_setup(tmp_event_filter, NFP_EM_FILTER_MASK_TYPE_LASTEV,
                           0x140000, 0xFC0000, tmp_status);

    /* XXX set all QC queues to a safe state! */
}


void
service_qc()
{
    struct check_queues_consts c;

    /* Check bitmasks */
    check_bitmask_filters(&active_bmsk, &tx_ap_xfers, &tx_ap_s0, &tx_ap_s1,
              &tx_ap_s2, &tx_ap_s3, TXQ_EVENT_START);

    /* Check queues */
    c.pcie_isl =       PCIE_ISL;
    c.max_retries =    TX_MAX_RETRIES;
    c.batch_sz =       TX_BATCH_SZ;
    c.base_queue_num = TXQ_START;
    c.pending_test =   TX_PENDING_TEST;
    c.event_data =     TXQ_EVENT_DATA;
    c.event_type =     NFP_QC_STS_LO_EVENT_TYPE_NOT_EMPTY;
    check_queues(&queue_data, &active_bmsk, &pending_bmsk, &c);
}
