/*
 * Copyright (C) 2014-2018,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/utils/nn_ring.h
 * @brief         NFP next neighbour ring interface
 */
#ifndef _BLOCKS__VNIC_UTILS_NN_RING_H_
#define _BLOCKS__VNIC_UTILS_NN_RING_H_

#include <nfp.h>
#include <nfp/me.h>


/**
 * The desired NN ring mode (CtxEnables:NNreceiveConfig) must be known
 * at compile time by NFCC to generate correct delays between ring accesses.
 * Therefore, the commandline option "-Qnn_mode" should be used to configure
 * the desired mode rather than "_init_csr" or code that modifies "CtxEnables"
 * directly.
 */

/**
 * Configure NN put and get pointers at init time.
 * The pointers on the receiving ME determine the behaviour of the NN ring.
 */
#define NN_RING_ZERO_PTRS               \
    __asm { .init_csr mecsr:NNPut 0 }   \
    __asm { .init_csr mecsr:NNGut 0 }


/**
 * Configure the NN ring empty assert setting in CtxEnables.  This should
 * be set on the receiving ME.
 * See the databook for a description of possible values.
 */
#define NN_RING_EMPTY_ASSERT_SET(_x)                        \
    __asm .init_csr mecsr:CtxEnables.NextNeighborEmptyAssert #_x


/**
 * Test NN_EMPTY status signal
 *
 * Indicates whether the NN ring from which this ME receives work is empty.
 * The threshold at which the ring tests empty depends on the configuration set
 * by nn_ring_init_receive.
 */
__intrinsic nn_ring_empty()
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
__intrinsic int nn_ring_full()
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

