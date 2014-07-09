/*
 * Copyright (C) 2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/utils/_c/dma_seqn.c
 * @brief         Helper functions for handling DMA sequence numbers and events
 *
 */

#include <assert.h>
#include <nfp.h>

#include <std/event.h>

#include <nfp6000/nfp_cls.h>

#include <vnic/utils/dma_seqn.h>


__intrinsic void
dma_seqn_ap_setup(unsigned int filter_num, unsigned int ap_num,
                  unsigned int type, volatile __xread unsigned int *xfer,
                  volatile SIGNAL *sig)
{
    unsigned int ctx = ctx();
    unsigned int meid = __MEID;
    struct nfp_em_filter_status filter_status;
    __cls struct event_cls_filter *event_filter_handle;

    /* Strict match type and provider, match all seqn */
    unsigned int event_mask = NFP_EVENT_MATCH(0xFF, 0, 0xF);
    unsigned int pcie_provider = NFP_EVENT_PROVIDER_NUM(
        meid>>4, NFP_EVENT_PROVIDER_INDEX_PCIE);
    unsigned int event_match = NFP_EVENT_MATCH(pcie_provider, 0, type);


    ctassert(__is_ct_const(filter_num));
    ctassert(__is_ct_const(ap_num));
    ctassert(__is_ct_const(type));

    /*
     * Set filter status.
     * We can throttle autopush rate by setting non-zero values here,
     * but resetting the autopush inherently throttles the rate as well.
     */
    filter_status.__raw = 0;

    /* Setup filter */
    event_filter_handle = event_cls_filter_handle(filter_num);
    event_cls_filter_setup(event_filter_handle,
                           NFP_EM_FILTER_MASK_TYPE_LASTEV,
                           event_match, event_mask, filter_status);

    /* Setup autopush */
    event_cls_autopush_signal_setup(ap_num, meid, ctx,
                                    __signal_number(sig),
                                    __xfer_reg_number(xfer));
    event_cls_autopush_filter_reset(
        ap_num,
        NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
        ap_num);
}


__intrinsic void
dma_seqn_advance(volatile __xread unsigned int *xfer, unsigned int *compl)
{
    unsigned int seqn_inc;
    struct nfp_event_match event;

    event.__raw = *xfer;
    seqn_inc = (event.source - *compl) & 0xFFF;
    *compl += seqn_inc;
}
