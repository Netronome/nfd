/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/distr_seqn.h
 * @brief         Code to compute and distribute DMA sequence numbers
 */
#ifndef _BLOCKS__VNIC_PCI_IN_DISTR_SEQN_H_
#define _BLOCKS__VNIC_PCI_IN_DISTR_SEQN_H_

/**
 * Initialise DMA sequence number distribution state
 *
 * This method sets up the shared transfer registers and autopushes
 * necessary for monitoring and distributing DMA sequence numbers.
 */
extern void init_distr_seqn();

/**
 * Distribute DMA sequence numbers
 *
 * Checks for new sequence number events, updates internal state and
 * pushes the sequence number to listening MEs.
 */
extern void  distr_seqn();

#endif /* !_BLOCKS__VNIC_PCI_IN_DISTR_SEQN_H_ */
