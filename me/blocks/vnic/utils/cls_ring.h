/*
 * Copyright (C) 2012-2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/utils/cls_ring.h
 * @brief         NFP CLS ring interface
 */
#ifndef _NFP__CLS_RING_H_
#define _NFP__CLS_RING_H_

#include <nfp.h>
#include <types.h>


__intrinsic void cls_ring_setup(unsigned int rnum, __cls void *base,
                                size_t size);

__intrinsic void cls_ring_put(unsigned int rnum, __xwrite void *data,
                              size_t size, SIGNAL *put_sig);

__intrinsic void cls_ring_get(unsigned int rnum, __xread void *data,
                              size_t size, SIGNAL *get_sig);

#endif /* !_NFP__CLS_RING_H_ */
