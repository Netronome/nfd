/*
 * Copyright (C) 2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/utils/dma_seqn.h
 * @brief         Helper functions for handling DMA sequence numbers and events
 *
 */
#ifndef _BLOCKS__VNIC_UTILS_DMA_SEQN_H_
#define _BLOCKS__VNIC_UTILS_DMA_SEQN_H_

#include <nfp.h>


/**
 * Configure an event filter and autopush to receive DMA sequence numbers
 * @param filter_num    event filter to configure
 * @param ap_num        autopush to configure
 * @param type          event type to catch
 * @param xfer          read transfer register for sequence events
 * @param sig           autopush signal
 *
 * This function must be called from the ME and context that will receive the
 * autopush as it tests for these values internally.
 *
 */
__intrinsic void dma_seqn_ap_setup(unsigned int filter_num, unsigned int ap_num,
                                   unsigned int type,
                                   volatile __xread unsigned int *xfer,
                                   SIGNAL *sig);


/**
 * Compute the updated "completed" sequence number
 * @param xfer          transfer register containing event
 * @param compl         "completed" sequence number to update
 */
__intrinsic void dma_seqn_advance(volatile __xread unsigned int *xfer,
                                  __gpr unsigned int *compl);

#endif /* !_BLOCKS__VNIC_UTILS_DMA_SEQN_H_ */

