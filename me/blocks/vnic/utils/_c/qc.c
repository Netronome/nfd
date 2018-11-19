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
 * @file          blocks/vnic/utils/qc.c
 * @brief         An API to manage queue controller accesses in NFD
 */

#if defined(__NFP_IS_6XXX)
    #include <nfp6000/nfp_cls.h>
    #include <nfp6000/nfp_me.h>
#elif defined(__NFP_IS_38XX)
    #include <nfp3800/nfp_cls.h>
    #include <nfp3800/nfp_me.h>
#else
    #error "Unsupported chip type"
#endif

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <std/reg_utils.h>

#include <vnic/nfd_common.h>
#include <vnic/utils/qc.h>


/* Compress quad-spaced queues to a compact bitmask in the low 8 bits
 * "off" specifies the offset within the input to extract
 *
 * bf_compress_quad(0b0011 0110, 0) -> 10
 * bf_compress_quad(0b0011 0110, 1) -> 11
 * bf_compress_quad(0b0011 0110, 2) -> 01
 * bf_compress_quad(0b0011 0110, 3) -> 00
 *
 * The implementation is a variant of the sheep-and-goat algorithm.
 */
static __intrinsic unsigned int bf_compress_quad(unsigned int x, int off)
{
    ctassert(__is_ct_const(off));

    x = (x >> off) & 0x11111111;

    x = (x | x >> 3) & 0x03030303;
    x = (x | x >> 6) & 0x000f000f;

    return (x | x >> 12) & 0x000000ff;
}


/*
 * Select a queue to service
 */
__intrinsic int
select_queue(__gpr unsigned int *queue, __shared __gpr struct qc_bitmask *bmsk)
{
    __gpr unsigned int bmsk_cp;
    /* XXX THSDK-1134 workaround. Provide local structures for pointers */
    __gpr unsigned int queue_cp;

    if (bmsk->proc == 0) {
        /*
         * Select the next non-zero bitmask to service
         * Perhaps a jump table could be used effectively
         * __gpr variables need to be explicitly referenced,
         * and we need to swap masks each time for fairness
         */
        if (bmsk->curr == 0) {
            if (bmsk->bmsk_hi != 0) {
                bmsk->proc = bmsk->bmsk_hi;
                bmsk->curr = 1;
            } else if (bmsk->bmsk_lo != 0) {
                bmsk->proc = bmsk->bmsk_lo;
            } else {
                return 1;
            }
        } else {
            if (bmsk->bmsk_lo != 0) {
                bmsk->proc = bmsk->bmsk_lo;
                bmsk->curr = 0;
            } else if (bmsk->bmsk_hi != 0) {
                bmsk->proc = bmsk->bmsk_hi;
            } else {
                return 1;
            }
        }
    }

    /* bmsk->proc != 0 */
    bmsk_cp = bmsk->proc;
    queue_cp = *queue;

    __asm ffs[queue_cp, bmsk_cp];
    __asm alu[--, queue_cp, OR, 0];
    __asm alu[bmsk_cp, bmsk_cp, and~, 1, <<indirect];

    bmsk->proc = bmsk_cp;
    *queue = queue_cp | (bmsk->curr << 5);

    return 0;
}

__intrinsic void
clear_queue(__gpr unsigned int *queue, __shared __gpr struct qc_bitmask *bmsk)
{
    if (*queue & 32) {
        bmsk->bmsk_hi &= ~(1 << (*queue & 31));
    } else {
        bmsk->bmsk_lo &= ~(1 << *queue);
    }
}

__intrinsic void
set_queue(__gpr unsigned int *queue, __shared __gpr struct qc_bitmask *bmsk)
{
    if (*queue & 32) {
        bmsk->bmsk_hi |= (1 << (*queue & 31));
    } else {
        bmsk->bmsk_lo |= (1 << *queue);
    }
}

__intrinsic void
init_qc_queues(unsigned int pcie_isl, struct qc_queue_config *cfg,
               unsigned int start_queue, unsigned int stride,
               unsigned int num_queues)
{
    __gpr unsigned int queue;
    unsigned int end_queue;

    /* ctassert(__is_ct_const(start_queue)); */
    ctassert(__is_ct_const(stride));
    ctassert(__is_ct_const(num_queues));
    try_ctassert((start_queue + (unsigned int) stride * (num_queues-1)) <= 255);
    end_queue = start_queue + stride * (num_queues - 1);

    /* Loop over queue range calling initialisation method */
    for (queue = start_queue; queue <= end_queue; queue += stride) {
        qc_init_queue(pcie_isl, queue, cfg);
    }
}

#define _INIT_ONE_FILTER(num, handle)                                   \
do {                                                                    \
    __implicit_write(s##num);                                           \
    __implicit_write(&(xfers->x##num), sizeof(unsigned int));           \
                                                                        \
    event_filter = event_cls_filter_handle(handle);                     \
    event_cls_filter_setup(event_filter,                                \
                           NFP_EM_FILTER_MASK_TYPE_MASK32,              \
                           (event_match | (num << 9)),                  \
                           event_mask, status);                         \
                                                                        \
    event_cls_autopush_signal_setup(handle, meid,                       \
                                    ctx,                                \
                                    __signal_number(s##num),            \
                                    __xfer_reg_number(&(xfers->x##num))); \
                                                                        \
    event_cls_autopush_filter_reset(                                    \
        handle,                                                         \
        NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,                   \
        handle);                                                        \
} while (0)

__intrinsic void
init_bitmask_filters(__xread struct qc_xfers *xfers,
                     volatile SIGNAL *s0, volatile SIGNAL *s1,
                     volatile SIGNAL *s2, volatile SIGNAL *s3,
                     volatile SIGNAL *s4, volatile SIGNAL *s5,
                     volatile SIGNAL *s6, volatile SIGNAL *s7,
                     unsigned int event_data, unsigned int start_handle)
{
    __cls struct event_cls_filter *event_filter;
    struct nfp_em_filter_status status;
    /* Configure mask to allow both NFP_EVENT_TYPE_FIFO_NOT_EMPTY (0) and
     * NFP_EVENT_TYPE_FIFO_ABOVE_WM (2).  TX and CFG queues use "not empty"
     * and the FL queues use "above watermark".
     */
    unsigned int event_mask = NFP_EVENT_MATCH(0xFF, 0xFE0, 0xD);
    unsigned int event_match;
    unsigned int meid = __MEID;
    unsigned int ctx = ctx();
    unsigned int pcie_provider = NFP_EVENT_PROVIDER_NUM(
        (meid>>4), NFP_EVENT_PROVIDER_INDEX_PCIE);

    ctassert(__is_ct_const(start_handle));
    ctassert(start_handle + 8 < 16);

    status.__raw = 0; /* bitmask32 requires no further settings */
    event_match = NFP_EVENT_MATCH(pcie_provider, event_data, 0);

    _INIT_ONE_FILTER(0, start_handle);
    _INIT_ONE_FILTER(1, start_handle + 1);
    _INIT_ONE_FILTER(2, start_handle + 2);
    _INIT_ONE_FILTER(3, start_handle + 3);
    _INIT_ONE_FILTER(4, start_handle + 4);
    _INIT_ONE_FILTER(5, start_handle + 5);
    _INIT_ONE_FILTER(6, start_handle + 6);
    _INIT_ONE_FILTER(7, start_handle + 7);
}

__intrinsic void
init_bitmasks(__gpr struct qc_bitmask *bmsk)
{
    bmsk->bmsk_lo = 0;
    bmsk->bmsk_hi = 0;
    bmsk->proc = 0;
    bmsk->curr = 0;
}

/* Test and reset one bitmask filter if necessary
 * @param num       signal number to test, and xfer to use if set
 * @param entry     bmsk_lo or bmsk_hi
 * @param shf       amount to shift queues once compacted
 *
 * The queues are all single spaced, so they must be compressed first.
 * The compressed bitmask must then be shifted into the correct bits of
 * the output bitmask.
 */
#define _CHECK_ONE_FILTER(num, entry, shf)                              \
do {                                                                    \
    if (signal_test(s##num)) {                                          \
        unsigned int new_queues;                                        \
                                                                        \
        new_queues = bf_compress_quad(xfers->x##num, 0);                \
        updates[0].##entry |= new_queues << shf;                        \
                                                                        \
        new_queues = bf_compress_quad(xfers->x##num, 1);                \
        updates[1].##entry |= new_queues << shf;                        \
                                                                        \
        new_queues = bf_compress_quad(xfers->x##num, 2);                \
        updates[2].##entry |= new_queues << shf;                        \
                                                                        \
        __implicit_write(s##num);                                       \
        __implicit_write(&(xfers->x##num), sizeof(unsigned int));       \
        event_cls_autopush_filter_reset(                                \
            start_handle + num,                                         \
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,               \
            start_handle + num);                                        \
    }                                                                   \
} while (0)

__intrinsic void
check_bitmask_filters(__shared __gpr struct qc_bmsk_updates updates[3],
                      __xread struct qc_xfers *xfers,
                      volatile SIGNAL *s0, volatile SIGNAL *s1,
                      volatile SIGNAL *s2, volatile SIGNAL *s3,
                      volatile SIGNAL *s4, volatile SIGNAL *s5,
                      volatile SIGNAL *s6, volatile SIGNAL *s7,
                      unsigned int start_handle)
{
    reg_zero(updates, sizeof(struct qc_bmsk_updates[3]));

    _CHECK_ONE_FILTER(0, bmsk_lo, 0);
    _CHECK_ONE_FILTER(1, bmsk_lo, 8);
    _CHECK_ONE_FILTER(2, bmsk_lo, 16);
    _CHECK_ONE_FILTER(3, bmsk_lo, 24);
    _CHECK_ONE_FILTER(4, bmsk_hi, 0);
    _CHECK_ONE_FILTER(5, bmsk_hi, 8);
    _CHECK_ONE_FILTER(6, bmsk_hi, 16);
    _CHECK_ONE_FILTER(7, bmsk_hi, 24);
}


__intrinsic void
set_Qctl8bitQnum()
{
    /*
     * XPB address of ECC SRAMTimingControl10.Qctl8bitQnum
     * We want to do a read-modify-write setting bit 31 only.
     */
    unsigned int addr = 0x110018;
    __xrw unsigned int ECCSramCntl10;
    SIGNAL sig;

    __asm ct[xpb_read, ECCSramCntl10, addr, 0, 1], ctx_swap[sig];
    ECCSramCntl10 = ECCSramCntl10 | (1<<31);
    __asm ct[xpb_write, ECCSramCntl10, addr, 0, 1], ctx_swap[sig];
}
