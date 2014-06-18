/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/qc.h
 * @brief         An API to manage queue controller accesses in NFD
 */
#ifndef _BLOCKS__VNIC_SHARED_QC_H_
#define _BLOCKS__VNIC_SHARED_QC_H_


/* XXX Host code may need access to the map_xxx_to_yyy methods
 * for debugging purposes. */

/**
 * Map from natural queue number to bitmask number
 * Compute which bit in a bitmask corresponds to a given natural queue number
 */
__intrinsic int
map_natural_to_bitmask(int natural_queue)
{
    return (((natural_queue & 31) << 1) | (natural_queue >> 5));
}

/**
 * Map from bitmask number to natural number
 * Given an offset in a queue bitmask, obtain the natural queue number
 */
__intrinsic int
map_bitmask_to_natural(int bitmask_queue)
{
    return (bitmask_queue>>1) | ((bitmask_queue << 5) & 32);
}


#if defined (__NFP_LANG_MICROC)

#include <nfp.h>
#include <types.h>
#include <std/event.h>
#include <vnic/utils/qcntl.h>

struct qc_xfers {
    volatile unsigned int x0;
    volatile unsigned int x1;
    volatile unsigned int x2;
    volatile unsigned int x3;
};

/**
 * A 64-bit bitmask structure
 * @param bmsk_lo       low 32 bits of the reference bitmask
 * @param bmsk_hi       high 32 bits of the reference bitmask
 * @param proc          copy of the bitmask in process
 * @param curr          number (0 or 1) of the bitmask currently in 'proc'
 *
 * 'bmsk_lo' and 'bmsk_hi' hold the reference bitmask.  One of these is copied
 * into 'proc' as necessary.  The copy in 'proc' is modified as the bitmask is
 * processed to track the progress.  'curr' lists the bitmask that was last
 * copied into 'proc'.
 */
struct qc_bitmask {
    unsigned int bmsk_lo;
    unsigned int bmsk_hi;
    unsigned int proc;
    unsigned int curr;
};

/**
 * A shared type PCI.IN and PCI.OUT queue data
 * PCI.IN and PCI.OUT maintain separate structs as they each require a few
 * specific fields. These structs are cast to a queue_info struct to update
 * their write and serviced pointers in a generic manner.
 */
struct queue_info {
    unsigned int wptr;
    unsigned int sptr;
    unsigned int ring_sz_msk;
    unsigned int dummy[5];
};

struct check_queues_consts {
    unsigned int pcie_isl;
    unsigned int max_retries;
    unsigned int batch_sz;
    unsigned int base_queue_num;
    unsigned int pending_test;
    unsigned int event_data;
    unsigned int event_type;
};

/**
 * Select a queue to service
 * @param queue         Next queue to service. Value in bitmask ordering.
 * @param bmsk          Structure containing reference bitmasks, processing
 *                      bitmask, and index of current bitmask.
 *
 * This code returns a queue to service from the queues set in bmsk->bmsk_lo/hi
 * using the "ffs" command. Actual processing works on a copy of
 * bmsk->bmsk_lo/hi
 * that is stored in bmsk->proc. Each time that an entry is located in
 * bmsk->proc, it is marked zero in that mask. Once the mask is empty, it is
 * refreshed from bmsk->bmsk_hi/lo.
 * The bitmasks are 64bit, but the ffs command supports 32bit values, so
 * the code alternates between the upper and lower 32bits.
 * The method return 0 if a queue to service is found, or 1 if no suitable
 * queue is found.
 */
__intrinsic int select_queue(__gpr unsigned int *queue,
                             __shared __gpr struct qc_bitmask *bmsk);

/**
 * Clear the bit for a queue in the bitmask provided
 *
 * @param queue         The queue bit to clear
 * @param bmsk          The bitmask to work on
 */
__intrinsic void clear_queue(unsigned int queue,
                             __shared __gpr struct qc_bitmask *bmsk);

/**
 * Configure a group of queue controller queues
 *
 * @param pcie_isl      PCIe Island to configure
 * @param cfg           Configuration to apply to the queues
 * @param start_queue   First queue to configure
 * @param stride        Value to increment queue number by each step
 * @param num_queues    Number of queues to configure
 *
 * Configuring 64 queues takes approximately 4000 cycles.
 */
__intrinsic void init_qc_queues(unsigned char pcie_isl,
                                struct qc_queue_config *cfg,
                                unsigned char start_queue,
                                unsigned char stride,
                                unsigned char num_queues);

/**
 * Configure a set of CLS event filters to monitor queue controller queues
 *
 * @param pcie_isl      PCIe Island to configure
 * @param xfers         An array of 4 read xfers for autopush value
 * @param sigs          An array of 4 singles for autopush
 * @param event_data    Basic pattern to match
 * @param start_handle  First event filter and autopush signal to use
 *
 * The 'event_data' is specialised for each bitmask by OR'ing in bits [6:5].
 * Bits [4:1] should be zero to match 16 queues with the bitmask.
 * Four event filters and autopush signals will used from 'start_handle'.
 * Setting up the four event filters and autopush signals takes approximately
 * 800 cycles.
 */
__intrinsic void init_bitmask_filters(__xread struct qc_xfers *xfers,
                                      volatile SIGNAL *s0, volatile SIGNAL *s1,
                                      volatile SIGNAL *s2, volatile SIGNAL *s3,
                                      unsigned int event_data,
                                      unsigned int start_handle);

/**
 * Zero a bitmask struct
 *
 * @param bmsk            A struct holding bitmask data
 */
__intrinsic void init_bitmasks(__gpr struct qc_bitmask *bmsk);


/**
 * Check whether any autopushes have returned, and reset if necessary
 * @param bmsk          active bitmask to update with new data
 * @param xfers         array of transfer registers to use for updates
 * @param sigX          signals to test
 * @param start_handle  first autopush handle, needed to reset filters
 *
 * 64 single spaced queues (0, 2, 4, ...) are packed into 64 bits in bmsk
 * bmsk[0] contains ( 0, 32,  1, 33,  2, 34, ..., 14, 46, 15, 47)
 * bmsk[1] contains (16, 48, 17, 49, 18, 50, ..., 30, 62, 31, 63)
 */
__intrinsic void check_bitmask_filters(__shared __gpr struct qc_bitmask *bmsk,
                                       __xread struct qc_xfers *xfers,
                                       volatile SIGNAL *s0, volatile SIGNAL *s1,
                                       volatile SIGNAL *s2, volatile SIGNAL *s3,
                                       unsigned int start_handle);

/**
 * Check active queues for new work, updating pending_bmsk and write pointers
 * @param queue_info_struct     An LM structure holding per queue data that
 *                              may be cast to a "struct queue_info" array
 * @param active_bmsk           An active bitmask as maintained by qc.X methods
 * @param pending_bmsk          A bitmask of queues that have outstanding work
 * @param queue_sz_msk          Used to mask the write pointer for the queues
 * @param c                     A struct of compile time configuration constants
 *                              the checks
 *
 * This method uses the active mask to narrow down likely queues to check for
 * outstanding work.  When queues with outstanding work are found, the pending
 * mask is updated.  If a queue is marked active but found to have no pending
 * work, the method attempts to mark it as inactive.
 * Per queue write pointers are updated by this method, and a serviced pointer
 * is taken as input to the method.  These are stored in the queue_info_struct.
 * The method requires a range of configuration data that allows it to be used
 * for either PCI.IN or PCI.OUT.  This data is stored in the "c" struct.
 * The queue size is treated as a separate parameter so that it can be
 * configured at runtime.
 */
__intrinsic int check_queues(void *queue_info_struct,
                             __shared __gpr struct qc_bitmask *active_bmsk,
                             __shared __gpr struct qc_bitmask *pending_bmsk,
                             struct check_queues_consts *c);

/**
 * XXX Configure the queue controller to use 8bit queue numbers in events.
 * This method must be called from the PCIe island to be configured.
 * The configurator will ultimately set this value, then this method will be
 * removed.
 */
__intrinsic void set_Qctl8bitQnum();

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_SHARED_QC_H_ */
