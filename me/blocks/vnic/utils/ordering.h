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
 * @file          blocks/vnic/utils/ordering.h
 * @brief         NFP ordering primatives
 */
#ifndef _BLOCKS__VNIC_UTILS_ORDERING_H_
#define _BLOCKS__VNIC_UTILS_ORDERING_H_

#include <nfp.h>

__intrinsic void reorder_start(unsigned int start_ctx, SIGNAL *sig);

__intrinsic void reorder_done(unsigned int start_ctx, unsigned int end_ctx,
                              SIGNAL *sig);

__intrinsic unsigned int reorder_get_next_ctx(unsigned int start_ctx,
                                              unsigned int end_ctx);

/* NB: "next_ctx" as obtained from "reorder_get_next_ctx" above */
__intrinsic void reorder_done_opt(unsigned int *next_ctx, SIGNAL *sig);

__intrinsic void reorder_self(SIGNAL *sig);

__intrinsic void reorder_future_sig(SIGNAL *sig, unsigned int cycles);

__intrinsic void reorder_test_swap(SIGNAL *sig);

#endif /* !_BLOCKS__VNIC_UTILS_ORDERING_H_ */
