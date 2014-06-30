/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/issue_dma.h
 * @brief         Code to DMA packet data to the NFP
 */

#ifndef _BLOCKS__VNIC_PCI_IN_ISSUE_DMA_H_
#define _BLOCKS__VNIC_PCI_IN_ISSUE_DMA_H_


/**
 * Perform shared configuration for issue_dma
 */
extern void issue_dma_setup_shared();

/**
 * Perform per context initialisation (for CTX 1 to 7)
 */
void issue_dma_setup();

/**
 * Fetch batch messages from the NN ring and process them, issuing up to
 * MAX_TX_BATCH_SZ DMAs, and placing a batch of messages onto the
 * "tx_issued_ring".  Messages are only dequeued from the NN ring when the
 * "gather_dma_seq_compl" sequence number indicates that it is safe to do so.
 * The message processing stalls until "data_dma_seq_safe" and
 * "data_dma_seq_issued" indicate that it is safe to continue.  Two ordering
 * stages ensure that packet DMAs are issued in sequence.
 */
void issue_dma();


#endif /* !_BLOCKS__VNIC_PCI_IN_ISSUE_DMA_H_ */

