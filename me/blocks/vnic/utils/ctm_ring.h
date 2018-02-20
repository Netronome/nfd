/*
 * Copyright (C) 2012-2018,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/utils/ctm_ring.h
 * @brief         NFP CTM ring interface
 */
#ifndef _NFP__CTM_RING_H_
#define _NFP__CTM_RING_H_

#include <nfp.h>
#include <types.h>


__intrinsic void ctm_ring_setup(unsigned int rnum, __ctm40 void *base,
                                size_t size);

__intrinsic void ctm_ring_put(unsigned int isl, unsigned int rnum,
                              __xwrite void *data,
                              size_t size, SIGNAL *put_sig);

__intrinsic void ctm_ring_get(unsigned int isl, unsigned int rnum,
                              __xread void *data, size_t size, SIGNAL *get_sig);

#endif /* !_NFP__CTM_RING_H_ */
