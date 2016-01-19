/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
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
