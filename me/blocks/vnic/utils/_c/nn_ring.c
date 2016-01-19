/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/utils/_c/nn_ring.c
 * @brief         NFP next neighbour ring interface
 */

#include <nfp/me.h>

#include <vnic/utils/nn_ring.h>

#include <nfp6000/nfp_me.h>


__intrinsic void
nn_ring_put(unsigned int val)
{
    __asm alu[*n$index++, --, B, val];
}


__intrinsic unsigned int
nn_ring_get()
{
    unsigned int result;

    __asm alu[result,--,B,*n$index++];

    return result;
}

__intrinsic unsigned int
nn_ring_read()
{
    unsigned int result;

    __asm alu[result,--,B,*n$index];

    return result;
}
