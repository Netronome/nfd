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
 * @file          blocks/vnic/utils/cls_ring.h
 * @brief         NFP CLS ring interface
 */
#ifndef _NFP__CLS_RING_H_
#define _NFP__CLS_RING_H_

#include <nfp.h>
#include <types.h>


enum cls_state_e {
    cls_ring0_status = 0,
    cls_ring1_status = 1,
    cls_ring2_status = 2,
    cls_ring3_status = 3,
    cls_ring4_status = 4,
    cls_ring5_status = 5,
    cls_ring6_status = 6,
    cls_ring7_status = 7,
    cls_ring8_status = 8,
    cls_ring9_status = 9,
    cls_ring10_status = 10,
    cls_ring11_status = 11,
    cls_ring12_status = 12,
    cls_ring13_status = 13,
    cls_ring14_status = 14,
    cls_ring15_status = 15
};


__intrinsic void cls_ring_setup(unsigned int rnum, __cls void *base,
                                size_t size);

__intrinsic int cls_state_test(int state);

__intrinsic int cls_state_test2(int state1, int state2);

#define CLS_RING_FULL_IND(x) cls_state_test(cls_ring##x##_status)
#define CLS_RING_FULL(x) CLS_RING_FULL_IND(x)

#define CLS_RING_EITHER_FULL_IND(x, y) \
    cls_state_test2(cls_ring##x##_status, cls_ring##y##_status)
#define CLS_RING_EITHER_FULL(x, y) CLS_RING_EITHER_FULL_IND(x, y)

__intrinsic void cls_ring_put(unsigned int rnum, __xwrite void *data,
                              size_t size, SIGNAL *put_sig);

__intrinsic void cls_ring_get(unsigned int rnum, __xread void *data,
                              size_t size, SIGNAL *get_sig);

#endif /* !_NFP__CLS_RING_H_ */
