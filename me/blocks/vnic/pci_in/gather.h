/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/gather.h
 * @brief         Code to gather TX descriptors from pending queues
 */
#ifndef _BLOCKS__VNIC_PCI_IN_GATHER_H_
#define _BLOCKS__VNIC_PCI_IN_GATHER_H_


/**
 * Perform shared initialisation of the gather block.
 */
extern void  dummy_init_gather_shared();


/**
 * Perform per context initialisation (for CTX 1 to 7)
 */
extern void dummy_init_gather();

__intrinsic void dummy_gather_vnic_setup(void *cfg_msg_in,
                                         unsigned int queue_size);

/**
 * Examine pending bitmasks and queue state to determine whether there are
 * any outstanding packets to process.  If there are, form a work batch
 * containing packets from the first queue with packets.  A batch may contain
 * up to MAX_TX_BATCH_SZ packets from a single queue.
 *
 * A batch message is placed in the next-neighbour ring for the ME, and the
 * descriptors are DMA'ed into the next slot in the CLS "desc_ring".
 */
extern int gather();


#endif /* !_BLOCKS__VNIC_PCI_IN_GATHER_H_ */
