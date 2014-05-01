/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/issue_dma.h
 * @brief         Code to DMA packet data to the NFP
 */

#ifndef _BLOCKS__VNIC_PCI_IN_ISSUE_DMA_H_
#define _BLOCKS__VNIC_PCI_IN_ISSUE_DMA_H_

extern void issue_dma_setup_shared();

void issue_dma_setup();

/** Parameters list to be filled out as extended */
int issue_dma();


#endif /* !_BLOCKS__VNIC_PCI_IN_ISSUE_DMA_H_ */

