/*
 * Copyright (C) 2012-2016,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/utils/ctm_ring.h
 * @brief         NFP CTM ring interface
 */
#ifndef _NFP__CTM_RING_H_
#define _NFP__CTM_RING_H_

#include <nfp.h>
#include <types.h>


__intrinsic void ctm_ring_setup(unsigned int rnum, __ctm void *base,
                                size_t size);

__intrinsic void ctm_ring_put(unsigned int isl, unsigned int rnum,
                              __xwrite void *data,
                              size_t size, SIGNAL *put_sig);

__intrinsic void ctm_ring_get(unsigned int isl, unsigned int rnum,
                              __xread void *data, size_t size, SIGNAL *get_sig);

#endif /* !_NFP__CTM_RING_H_ */
