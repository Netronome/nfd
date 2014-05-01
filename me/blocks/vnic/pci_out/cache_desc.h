/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc.c
 * @brief         Code to cache FL descriptors from pending queues
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_CACHE_DESC_H_
#define _BLOCKS__VNIC_PCI_OUT_CACHE_DESC_H_

extern void cache_desc_setup_shared();

__intrinsic void cache_desc_vnic_setup(void *cfg_msg_in,
                                       unsigned int queue_size);

#endif /* !_BLOCKS__VNIC_PCI_OUT_CACHE_DESC_H_ */

