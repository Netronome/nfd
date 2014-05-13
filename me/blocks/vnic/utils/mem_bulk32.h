/*
 * Copyright (C) 2012-2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/utils/_c/mem_bulk32.c
 * @brief         NFP memory bulk interface
 */
#ifndef _NFP__MEM_BULK32_H_
#define _NFP__MEM_BULK32_H_

#include <nfp.h>
#include <types.h>

/**
 * Read a multiple of 4B from a memory unit (EMEM, IMEM, or CTM)
 * @param data      pointer to sufficient read transfer registers for the op
 * @param addr      40-bit pointer to the memory start address
 * @param size      size of the read, must be a multiple of 8
 * @param max_size  used to determine largest read, if size is not a constant
 * @param sync      type of synchronisation (sig_done or ctx_swap)
 * @param sig       signal to use
 *
 * This method provides basic bulk reads from NFP memory units.  No special
 * alignment is assumed about address when converting from a 40bit pointer to
 * "src_op" fields in the __asm command, which makes the method general, but
 * suboptimal if it is guaranteed to be 256B aligned.  There is currently
 * limited support for size to be a runtime value, and for reads >32B, see
 * THSDK-1161.  mem_read32() provides a simplified interface where size is
 * assumed to be compile time constant, and the context swaps on an internal
 * signal while waiting for the read to complete.
 */
__intrinsic void __mem_read32(__xread void *data, __dram void *addr,
                              size_t size, const size_t max_size,
                              sync_t sync, SIGNAL *sig);

__intrinsic void mem_read32(__xread void *data, __dram void *addr,
                            const size_t size);


/**
 * Write a multiple of 4B to a memory unit (EMEM, IMEM, or CTM)
 * @param data      pointer to sufficient write transfer registers for the op
 * @param addr      40-bit pointer to the memory start address
 * @param size      size of the write, must be a multiple of 8
 * @param max_size  used to determine largest write, if size is not a constant
 * @param sync      type of synchronisation (sig_done or ctx_swap)
 * @param sig       signal to use
 *
 * This method provides basic bulk writes to NFP memory units.  No special
 * alignment is assumed about address when converting from a 40bit pointer to
 * "src_op" fields in the __asm command, which makes the method general, but
 * suboptimal if it is guaranteed to be 256B aligned.  There is currently
 * limited support for size to be a runtime value, and for reads >32B, see
 * THSDK-1161.  mem_write32() provides a simplified interface where size is
 * assumed to be compile time constant, and the context swaps on an internal
 * signal while waiting for the write to complete.
 */
__intrinsic void __mem_write32(__xwrite void *data, __dram void *addr,
                               size_t size, const size_t max_size,
                               sync_t sync, SIGNAL *sig);

__intrinsic void mem_write32(__xwrite void *data, __dram void *addr,
                             const size_t size);

#endif /* !_NFP__MEM_BULK32_H_ */
