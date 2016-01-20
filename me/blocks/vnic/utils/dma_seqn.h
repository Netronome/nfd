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
 * @param ext_type      extended software type to catch
 * @param xfer          read transfer register for sequence events
 * @param sig           autopush signal
 *
 * This function must be called from the ME and context that will receive the
 * autopush as it tests for these values internally.
 *
 */
__intrinsic void dma_seqn_ap_setup(unsigned int filter_num, unsigned int ap_num,
                                   unsigned int type, unsigned int ext_type,
                                   volatile __xread unsigned int *xfer,
                                   SIGNAL *sig);


/**
 * Compute the updated "completed" sequence number
 * @param xfer          transfer register containing event
 * @param compl         "completed" sequence number to update
 */
__intrinsic void dma_seqn_advance(volatile __xread unsigned int *xfer,
                                  __gpr unsigned int *compl);


/**
 * Compute the updated "completed" sequence number
 * @param xfer          transfer register containing event
 * @param compl         "completed" sequence number to update
 * @param amt           saves the amount by which *compl gets incremented
 */
__intrinsic void dma_seqn_advance_save(volatile __xread unsigned int *xfer,
                                       __gpr unsigned int *compl,
                                       __gpr unsigned int *amt);


/**
 * Populate the mode_sel and dma_mode fields for a DMA completion event
 * @param cmd           struct nfp_pcie_dma_cmd containing partial descriptor
 * @param type          event type
 * @param ext_type      extended software type to use for this event
 * @param source        event source
 *
 * This is function provides the functionality of flowenv's
 * pcie_dma_set_event(), but also accommodates the extended "type" and
 * reduced "source" field that dma_seqn utilises.
 */
__intrinsic void dma_seqn_set_event(void *cmd, unsigned int type,
                                    unsigned int ext_type,
                                    unsigned int source);


/**
 * Initialise a variable with event type information
 * @param type          hardware type to use for this event
 * @param ext_type      extended software type to use for this event
 */
__intrinsic unsigned int dma_seqn_init_event(unsigned int type,
                                             unsigned int ext_type);



/**
 * Set just the sequence number in an otherwise complete CPP_HI descriptor
 * @param cpp_hi_part   partial CPP "hi" descriptor
 * @param source        seqn number to set
 *
 * This method only touches the bits that will code the sequence number,
 * but does require them to be zero beforehand.  dma_seqn_init_event()
 * should be used to setup event information suitable for use with this
 * function.
 */
__intrinsic unsigned int dma_seqn_set_seqn(unsigned int cpp_hi_part,
                                           unsigned int seqn);

#endif /* !_BLOCKS__VNIC_UTILS_DMA_SEQN_H_ */

