/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/issue_dma.h
 * @brief         Code to DMA packet data from the NFP
 */

#ifndef _BLOCKS__VNIC_PCI_OUT_ISSUE_DMA_H_
#define _BLOCKS__VNIC_PCI_OUT_ISSUE_DMA_H_

extern void issue_dma_setup_shared();

extern void issue_dma_setup();

/** Parameters list to be filled out as extended */
extern void issue_dma();


#endif /* !_BLOCKS__VNIC_PCI_OUT_ISSUE_DMA_H_ */

