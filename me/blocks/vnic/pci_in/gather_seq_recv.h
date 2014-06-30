/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/gather_seq_recv.h
 * @brief         Receive gather_dma_seq_compl and expose it to local ME
 */
#ifndef _BLOCKS__VNIC_PCI_IN_GATHER_SEQ_RECV_H_
#define _BLOCKS__VNIC_PCI_IN_GATHER_SEQ_RECV_H_


/**
 * Check for sequence number reflects from "distr_seqn" and copy to shared
 * registers.  Recompute data_dma_seq_safe if necessary.
 */
extern void gather_seq_recv();

#endif /* !_BLOCKS__VNIC_PCI_IN_GATHER_SEQ_RECV_H_ */
