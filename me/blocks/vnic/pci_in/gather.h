/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/gather.h
 * @brief         Code to gather TX descriptors from pending queues
 */
#ifndef _BLOCKS__VNIC_PCI_IN_GATHER_H_
#define _BLOCKS__VNIC_PCI_IN_GATHER_H_

extern void  dummy_init_gather_shared();

extern void dummy_init_gather();

__intrinsic void dummy_gather_vnic_setup(void *cfg_msg_in,
                                         unsigned int queue_size);

/** Parameters list to be filled out as extended */
extern int gather();


#endif /* !_BLOCKS__VNIC_PCI_IN_GATHER_H_ */
