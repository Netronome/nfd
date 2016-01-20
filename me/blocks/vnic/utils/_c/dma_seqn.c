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
 * @file          blocks/vnic/utils/_c/dma_seqn.c
 * @brief         Helper functions for handling DMA sequence numbers and events
 *
 */

#include <assert.h>
#include <nfp.h>

#include <std/event.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/utils/dma_seqn.h>


#define DMA_SEQN_EXT_TYPE_msk   0x3
#define DMA_SEQN_EXT_TYPE_shf   14
#define DMA_SEQN_SEQN_msk       0xff
#define DMA_SEQN_SEQN_shf       16
#define DMA_SEQN_EXTRACT_shf    6


__intrinsic void
dma_seqn_ap_setup(unsigned int filter_num, unsigned int ap_num,
                  unsigned int type, unsigned int ext_type,
                  volatile __xread unsigned int *xfer,
                  SIGNAL *sig)
{
    unsigned int ctx = ctx();
    unsigned int meid = __MEID;
    struct nfp_em_filter_status filter_status;
    __cls struct event_cls_filter *event_filter_handle;

    /* Strict match type, ext_type, and provider, match all seqn */
    unsigned int event_mask = NFP_EVENT_MATCH(0xFF, 0x3, 0xF);
    unsigned int pcie_provider = NFP_EVENT_PROVIDER_NUM(
        meid>>4, NFP_EVENT_PROVIDER_INDEX_PCIE);
    unsigned int event_match;

    ctassert(__is_ct_const(filter_num));
    ctassert(__is_ct_const(ap_num));
    ctassert(__is_ct_const(type));
    ctassert(__is_ct_const(ext_type));

    /* We extend the HW event type by using 2 bits of the source field. */
    event_match = NFP_EVENT_MATCH(pcie_provider,
                                  (ext_type & DMA_SEQN_EXT_TYPE_msk),
                                  type);

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

    /* Mark the signal used on this thread. */
    __implicit_write(sig);
}


__intrinsic void
dma_seqn_advance(volatile __xread unsigned int *xfer, __gpr unsigned int *compl)
{
    unsigned int seqn_inc;

    seqn_inc = *xfer >> DMA_SEQN_EXTRACT_shf;
    seqn_inc = (seqn_inc - *compl) & DMA_SEQN_SEQN_msk;
    *compl += seqn_inc;
}



__intrinsic void
dma_seqn_advance_save(volatile __xread unsigned int *xfer,
                      __gpr unsigned int *compl,
                      __gpr unsigned int *seqn_inc)
{
    *seqn_inc = *xfer >> DMA_SEQN_EXTRACT_shf;
    *seqn_inc = (*seqn_inc - *compl) & DMA_SEQN_SEQN_msk;
    *compl += *seqn_inc;
}



__intrinsic void
dma_seqn_set_event(void *cmd, unsigned int type, unsigned int ext_type,
                   unsigned int source)
{
    unsigned int dma_mode;
    struct nfp_pcie_dma_cmd *cmd_ptr;

    dma_mode = source & DMA_SEQN_SEQN_msk;
    dma_mode = ((dma_mode << 2) |
                (((type & 0xF) << 12) | (ext_type & DMA_SEQN_EXT_TYPE_msk)));

    cmd_ptr = cmd;
    cmd_ptr->mode_sel = NFP_PCIE_DMA_CMD_DMA_MODE_2;
    cmd_ptr->dma_mode = dma_mode;
}


__intrinsic unsigned int
dma_seqn_init_event(unsigned int type, unsigned int ext_type)
{
    unsigned int event;

    ctassert(__is_ct_const(type));
    ctassert(__is_ct_const(ext_type));

    event = NFP_PCIE_DMA_CMD_MODE_SEL(2);
    event |= NFP_PCIE_DMA_CMD_DMA_MODE(((type & 0xF) << 12) |
                                       (ext_type & DMA_SEQN_EXT_TYPE_msk));

    return event;
}


__intrinsic unsigned int
dma_seqn_set_seqn(unsigned int cpp_hi_part, unsigned int source)
{
    unsigned int event;

    event = source & DMA_SEQN_SEQN_msk;
    event = cpp_hi_part | (event << DMA_SEQN_SEQN_shf);

    return event;
}
