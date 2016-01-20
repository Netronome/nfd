/*
 * Copyright (C) 2012-2016,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/utils/_c/cls_ring.c
 * @brief         NFP CLS ring interface
 */

#include <assert.h>
#include <nfp.h>
#include <types.h>

#include <nfp/cls.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_me.h>

#include <vnic/utils/cls_ring.h>

/* Override the length field on the NFP6000
 * XXX replace with values from nfp6000/nfp_me.h*/
#define _IND_COUNT(_x)   (((_x - 1) << 8) | (1 << 7))


__intrinsic void
cls_ring_setup(unsigned int rnum, __cls void *base, size_t size)
{
    unsigned int entries = size / 4;
    __xwrite unsigned int cls_ring_base;
    __xwrite unsigned int cls_ring_ptrs = 0;
    __cls char *ring_addr = (__cls char *)NFP_CLS_RINGS_RING_BASE(rnum);
    SIGNAL s1, s2;

    try_ctassert(rnum < 16);

    /* Note, size is in bytes and an entry is 32bits */
    ctassert(__is_ct_const(size));
    ctassert(__is_log2(entries));
    ctassert((entries >= 32) && (entries <= SZ_1K));

    /* NB: if base is linker assigned, this can't be evaluated
     * at compile time */
    cls_ring_base = (NFP_CLS_RINGS_RING_BASE_SIZE(__log2(entries) - 5) |
                     NFP_CLS_RINGS_RING_BASE_BASE(((unsigned int)base) >> 7));

    __cls_write(&cls_ring_base, ring_addr, sizeof(cls_ring_base),
                sizeof(cls_ring_base), sig_done, &s1);
    __cls_write(&cls_ring_ptrs, ring_addr + 0x80, sizeof(cls_ring_ptrs),
                sizeof(cls_ring_ptrs), sig_done, &s2);
    wait_for_all(&s1, &s2);
}


__intrinsic int
cls_state_test(int rstate)
{
    __gpr int full = 1;

    ctassert(__is_ct_const(rstate));

    __asm {
        br_cls_state[__ct_const_val(rstate), match];
        alu[full, --, B, 0];
        match:
    }

    return full;
}


__intrinsic int
cls_state_test2(int rstate1, int rstate2)
{
    __gpr int full = 1;

    ctassert(__is_ct_const(rstate1));
    ctassert(__is_ct_const(rstate2));

    __asm {
        br_cls_state[__ct_const_val(rstate1), match];
        br_cls_state[__ct_const_val(rstate2), match];
        alu[full, --, B, 0];
        match:
    }

    return full;
}


__intrinsic void
cls_ring_put(unsigned int rnum, __xwrite void *data, size_t size,
             SIGNAL *put_sig)
{
    unsigned int ind;
    unsigned int count = size / 4;
    unsigned int ring_addr = rnum << 2;

    ctassert(__is_write_reg(data));
    try_ctassert(rnum < 16);
    ctassert(__is_ct_const(size));
    ctassert(__is_aligned(size, 4));
    ctassert(size <= (16*4));

    if (size <= (8*4)) {
        __asm { cls[put, *data, ring_addr, 0, __ct_const_val(count)], \
                sig_done[*put_sig] }
    } else {
        ind = _IND_COUNT(count);
        __asm {
            alu[--, --, b, ind]
            cls[put, *data, ring_addr, 0, __ct_const_val(count)], \
                indirect_ref, sig_done[*put_sig]
        }
    }
}


__intrinsic void
cls_ring_get(unsigned int rnum, __xread void *data, size_t size,
             SIGNAL *get_sig)
{
    unsigned int ind;
    unsigned int count = size / 4;
    unsigned int ring_addr = rnum << 2;

    ctassert(__is_read_reg(data));
    try_ctassert(rnum < 16);
    ctassert(__is_ct_const(size));
    ctassert(__is_aligned(size, 4));
    ctassert(size <= (16*4));

    if (size <= (8*4)) {
        __asm { cls[ring_get_freely, *data, ring_addr, 0, \
                    __ct_const_val(count)], sig_done[*get_sig] }
    } else {
        ind = _IND_COUNT(count);
        __asm {
            alu[--, --, b, ind]
            cls[ring_get_freely, *data, ring_addr, 0, \
                __ct_const_val(count)], indirect_ref, sig_done[*get_sig]
        }
    }
}
