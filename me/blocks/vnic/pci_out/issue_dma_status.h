/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/issue_dma_status.h
 * @brief         Display the state of the issue_dma blocks
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_ISSUE_DMA_STATUS_H_
#define _BLOCKS__VNIC_PCI_OUT_ISSUE_DMA_STATUS_H_

#define STATUS_ISSUE_DMA_START   24

struct nfd_out_issue_dma_status {
    unsigned int data_dma_seq_started;
    unsigned int data_dma_seq_issued;
    unsigned int data_dma_seq_compl;
    unsigned int data_dma_seq_served;
    unsigned int data_dma_seq_safe;
};


#if defined (__NFP_LANG_MICROC)

/**
 * Initialise the issue_dma status output
 */
extern void issue_dma_status_setup();

/**
 * Show issue_dma status in transfer registers
 */
extern void issue_dma_status();

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_PCI_OUT_ISSUE_DMA_STATUS_H_ */
