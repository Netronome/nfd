/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc_status.h
 * @brief         Display the state of the cache_desc block
 */
#ifndef _BLOCKS__VNIC_PCI_IN_GATHER_STATUS_H_
#define _BLOCKS__VNIC_PCI_IN_GATHER_STATUS_H_

#define STATUS_Q_INDEP_START    16
#define STATUS_Q_INFO_START     24
#define STATUS_Q_SEL_START      31


extern void cache_desc_status_setup();

extern void cache_desc_status();

#endif /* !_BLOCKS__VNIC_PCI_OUT_CACHE_DESC_STATUS_H_ */

