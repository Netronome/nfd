/*
 * Copyright (C) 2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/utils/nn_ring.h
 * @brief         NFP next neighbour ring interface
 */
#ifndef _BLOCKS__VNIC_UTILS_NN_RING_H_
#define _BLOCKS__VNIC_UTILS_NN_RING_H_

#include <nfp.h>
#include <nfp/me.h>

/**
 * Configure CTX enables to put to next neighbour or self
 * @param self          use own next neighbour registers
 *
 * If self tests true, configure ring to the same ME, else configure ring
 * to the next neighbour.  This method must be called from the ME that
 * will put onto ring.
 */
__intrinsic void nn_ring_init_send(unsigned int self);

/**
 * Configure next neighbour ring behaviour
 * @param self          use own next neighbour registers
 * @param empty_assert  threshold for the NN_EMPTY status signal
 *
 * If self tests true, configure ring to the same ME, else configure ring
 * to the next neighbour.  The NN_put and NN_get CSRs are also reset to zero.
 * This method must be called from the ME that will get from the ring.  See
 * the data book for possible 'empty_assert' values.
 */
__intrinsic void nn_ring_init_receive(unsigned int self,
                                      unsigned int empty_assert);

/**
 * Test NN_EMPTY status signal
 *
 * Indicates whether the NN ring from which this ME receives work is empty.
 * The threshold at which the ring tests empty depends on the configuration set
 * by nn_ring_init_receive.
 */
static __inline nn_ring_empty()
{
    return inp_state_test(inp_state_nn_empty);
}


/**
 * Test NN_FULL status signal
 *
 * Indicates whether the NN ring to which this ME puts work is full.
 * NN_FULL asserts when the ring contains 96 entries.  See the data book for
 * an explanation of how many long words of data can safely be added to a
 * ring after it tests !NN_FULL.
 */
static __inline int nn_ring_full()
{
    return inp_state_test(inp_state_nn_full);
}

/**
 * Put one register onto the ring, incrementing the put counter
 * @param val           value to add to the ring
 *
 * It is the user's responsibility to ensure sufficient space is available
 * in the ring before calling this method.
 */
__intrinsic void nn_ring_put(unsigned int val);

/**
 * Get one register from the ring, incrementing the get counter
 *
 * It is the user's responsibility to ensure sufficient space data is available
 * on the ring before calling this method.
 */
__intrinsic unsigned int nn_ring_get();

/**
 * Read one register from the ring, without incrementing the get counter
 *
 * It is the user's responsibility to ensure sufficient space data is available
 * on the ring before calling this method.
 */
__intrinsic unsigned int nn_ring_read();

#endif /* !_BLOCKS__VNIC_UTILS_NN_RING_H_ */

