/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/notify.h
 * @brief         Code to DMA packet data to the NFP
 */

#ifndef _BLOCKS__VNIC_PCI_IN_NOTIFY_H_
#define _BLOCKS__VNIC_PCI_IN_NOTIFY_H_

#include <vnic/pci_in_cfg.h>
#include <vnic/pci_in/pci_in_internal.h>

#define NFD_WQ_BASE_IND(_isl)       nfd_wq_base_pcie##_isl
#define NFD_WQ_BASE(_isl)           NFD_WQ_BASE_IND(_isl)

/* XXX try to set up an NFD_WQ_NUM(_isl, _q) type macro */
#define NFD_WQ_NUM_IND(_isl, _q)    (NFD_WQS_NUM_PCIE##_isl | _q)
#define NFD_WQ_NUM(_isl, _q)        NFD_WQ_NUM_IND(_isl, _q)

#define NFD_WQS_DECLARE_IND(_isl)                                   \
  __export __emem __align(NFD_NUM_WQS * NFD_WQ_SZ *                 \
                          sizeof(struct nfd_pci_in_issued_desc))    \
      struct nfd_pci_in_issued_desc                                 \
      NFD_WQ_BASE(_isl)[NFD_NUM_WQS][NFD_WQ_SZ]

#define NFD_WQS_DECLARE(_isl) NFD_WQS_DECLARE_IND(_isl)

/**
 * Perform shared configuration for notify
 */
extern void notify_setup_shared();

/**
 * Perform per context initialisation (for CTX 1 to 7)
 */
extern void notify_setup();

/**
 * Dequeue a batch of "issue_dma" messages and process that batch, incrementing
 * TX.R for the queue and adding an output message to one of the PCI.IN work
 * queueus.  An output message is only sent for the final message for a packet
 * (EOP bit set).  A count of the total number of descriptors in the batch is
 * added by the "issue_dma" block.
 */
extern void notify();

#endif /* !_BLOCKS__VNIC_PCI_IN_NOTIFY_H_ */
