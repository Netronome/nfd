/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/precache_bufs.h
 * @brief         Fill the local (LM) cache of MU buffers
 */
#ifndef _BLOCKS__VNIC_PCI_IN_PRECACHE_BUFS_H_
#define _BLOCKS__VNIC_PCI_IN_PRECACHE_BUFS_H_

extern void precache_bufs_setup();

extern void precache_bufs();

__intrinsic unsigned int precache_bufs_use();

unsigned int precache_bufs_avail();

#endif /* !_BLOCKS__VNIC_PCI_IN_PRECACHE_BUFS_H_ */
