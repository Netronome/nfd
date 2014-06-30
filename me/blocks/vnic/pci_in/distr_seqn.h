/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/distr_seqn.h
 * @brief         Code to compute and distribute DMA sequence numbers
 */
#ifndef _BLOCKS__VNIC_PCI_IN_DISTR_SEQN_H_
#define _BLOCKS__VNIC_PCI_IN_DISTR_SEQN_H_

/**
 * The distr_seqn block declares three sequence numbers: "gather_dma_seq_compl",
 * "data_dma_seq_compl", and "data_dma_seq_served".  The "compl" sequence
 * numbers are reconstructed from DMA events in this block.  The "served"
 * sequence number is taken from the "notify" block.  It is pushed if it has
 * changed.
 */

/**
 * Initialise DMA sequence number distribution state
 *
 * This method sets up the shared transfer registers and autopushes
 * necessary for monitoring and distributing DMA sequence numbers.
 */
extern void distr_seqn_setup();

/**
 * Distribute DMA sequence numbers
 *
 * Checks for new sequence number events, updates internal state and
 * pushes the sequence number to listening MEs.
 */
extern void  distr_seqn();

#endif /* !_BLOCKS__VNIC_PCI_IN_DISTR_SEQN_H_ */
