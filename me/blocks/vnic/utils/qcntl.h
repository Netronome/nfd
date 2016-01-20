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
 * @file          blocks/vnic/utils/qcntl.h
 * @brief         Interface for event bus and event managers
 *
 */
#ifndef _BLOCKS__VNIC_UTILS_QCNTL_H_
#define _BLOCKS__VNIC_UTILS_QCNTL_H_

#include <nfp.h>
#include <types.h>

#include <nfp6000/nfp_qc.h>

/**
 * Queue configuration struct, ideally containing constants
 * @param watermark         watermark to use, if event_type set watermark
 * @param size              queue size
 * @param event_data        6 bits of data to include in events
 * @param event_type        when to generate events for this queue
 * @param ptr               value to reset the read and write pointers to
 *
 *  Depending on the queue controller configuration, either all 6 'event_data'
 *  bits are included in events, or the top 4 bits are included.
 */
struct qc_queue_config {
    enum pcie_qc_watermark watermark;
    enum pcie_qc_q_size size;
    unsigned int event_data;
    enum pcie_qc_event event_type;
    unsigned int ptr;
};

/**
 * Select QC pointer
 */
enum qc_ptr_type {
    QC_RPTR = 0,
    QC_WPTR
};


/**
 * Read QC configuration and status for one queue
 * @param pcie_isl      which PCIe island to address (0..3)
 * @param queue         which QC queue to address
 * @param ptr           which pointer to read
 * @param value         read transfer register for output data
 * @param sync          type of synchronisation (sig_done or ctx_swap)
 * @param sig           signal to use
 *
 * The read pointer is contained in ConfigStatusLow, and the write pointer is
 * contained in ConfigStatusHigh.  To use the output, 'value' can be allocated
 * to the '__raw' entry in the appropriate struct, or cast to the desired type.
 */
__intrinsic void  __qc_read(unsigned char pcie_isl, unsigned int queue,
                            enum qc_ptr_type ptr,
                            __xread unsigned int *value,
                            sync_t sync, SIGNAL *sig);
__intrinsic unsigned int qc_read(unsigned char pcie_isl, unsigned int queue,
                                 enum qc_ptr_type ptr);


/**
 * Write QC configuration and status, or pointer only, for one queue
 * @param pcie_isl      which PCIe island to address (0..3)
 * @param queue         which QC queue to address
 * @param value         unsigned int value to write
 * @param offset        address offset within the queue to write
 * @param sync          type of synchronisation (sig_done or ctx_swap)
 * @param sig           signal to use
 *
 * The specific offset and value written determines whether only the
 * pointer or the configuration is written.  See the data book for more
 * information.
 */
__intrinsic void __qc_write(unsigned char pcie_isl, unsigned int queue,
                            __xwrite unsigned int *value,
                            unsigned int offset, sync_t sync, SIGNAL *sig);


/**
 * Initialise a queue controller queue using the supplied configuration
 * @param pcie_isl      which PCIe island to address (0..3)
 * @param queue         which QC queue to address
 * @param cfg           a struct containing configuration settings, ideally
 *                      all compile time constants
 *
 * This method rewrites the queue configuration for a given PCIe island and
 * queue ConfigStatusLow and ConfigStatusHigh are written in parallel, using
 * separate signals for each write.  The method swaps on these signals
 * before returning.
 */
__intrinsic void qc_init_queue(unsigned char pcie_isl, unsigned int queue,
                               struct qc_queue_config *cfg);

/**
 * Rewrite queue event configuration, triggering queue flags to be retested
 * @param pcie_isl      which PCIe island to address (0..3)
 * @param queue         which QC queue to address
 * @param event_data    6 bits of data to include in events.
 * @param event_type    when to generate events for this queue
 * @param xfer          transfer register to use for the ping
 * @param sync          type of synchronisation (sig_done or ctx_swap)
 * @param sig           signal to use
 *
 * If an FPC maintains state about a QC queue and relies on the event system
 * for notifications about changes in state, then pinging a queue after marking
 * the queue not active ensures that an event is issued if it has become active
 * since it was last checked.  This makes it safe to mark queues not active
 * without creating a race condition between queue data from the CPP bus and
 * the event bus.  Depending on the queue controller configuration, either
 * all 6 'event_data' bits are included, or just the top 4 bits are included.
 */
__intrinsic void __qc_ping_queue(unsigned char pcie_isl, unsigned int queue,
                                 unsigned int event_data,
                                 enum pcie_qc_event event_type,
                                 __xwrite unsigned int *xfer,
                                 sync_t sync, SIGNAL *sig);

__intrinsic void qc_ping_queue(unsigned char pcie_isl, unsigned int queue,
                               unsigned int event_data,
                               enum pcie_qc_event event_type);

/**
 * Add specified value to given queue pointer
 * @param pcie_isl      which PCIe island to address (0..3)
 * @param queue         which QC queue to address
 * @param ptr           specify read or write pointer (QC_RPTR or QC_WPTR)
 * @param value         the value to add to the pointer
 * @param xfer          transfer register to use for the read
 * @param sync          type of synchronisation (sig_done or ctx_swap)
 * @param sig           signal to use
 *
 * Trigger hardware add to pointer operation.  This may result in an event on
 * the event bus depending on the state of the queue flags after the add, and
 * the queue configuration.
 */
__intrinsic void __qc_add_to_ptr(unsigned char pcie_isl, unsigned int queue,
                                 enum qc_ptr_type ptr,unsigned int value,
                                 __xread unsigned int *xfer,
                                 sync_t sync, SIGNAL *sig);

__intrinsic void qc_add_to_ptr(unsigned char pcie_isl, unsigned int queue,
                               enum qc_ptr_type ptr, unsigned int value);

#endif /* !_BLOCKS__VNIC_UTILS_QCNTL_H_ */
