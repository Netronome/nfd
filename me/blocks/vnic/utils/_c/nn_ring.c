/*
 * Copyright (C) 2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/utils/_c/nn_ring.c
 * @brief         NFP next neighbour ring interface
 */

#include <vnic/utils/nn_ring.h>

#include <nfp6000/nfp_me.h>

__intrinsic void
nn_ring_init_send(unsigned int self)
{
    struct nfp_mecsr_ctx_enables cfg;

    /*
     * Perform read modify write to update nn_send_configuration
     */
    cfg.__raw = local_csr_read(NFP_MECSR_CTX_ENABLES);
    if (self) {
        cfg.nn_send_configuration = 1;
    } else {
        cfg.nn_send_configuration = 0;
    }
    local_csr_write(NFP_MECSR_CTX_ENABLES, cfg.__raw);
}

__intrinsic void
nn_ring_init_receive(unsigned int self, unsigned int empty_assert)
{
    struct nfp_mecsr_ctx_enables cfg;

    /*
     * Reset put and get pointers
     * These are "owned" by the receive ME
     */
    local_csr_write(NFP_MECSR_NN_PUT, 0);
    local_csr_write(NFP_MECSR_NN_GET, 0);


    /*
     * Perform a read modify write to update nn_receive_configuration
     * and nn_ring_empty threshold. See the data book for threshold values.
     */
    cfg.__raw = local_csr_read(NFP_MECSR_CTX_ENABLES);
    if (self) {
        cfg.nn_receive_configuration = 1;
    } else {
        cfg.nn_receive_configuration = 0;
    }
    cfg.nn_ring_empty = empty_assert;
    local_csr_write(NFP_MECSR_CTX_ENABLES, cfg.__raw);
}


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
