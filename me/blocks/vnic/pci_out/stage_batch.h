/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/stage_batch.h
 * @brief         Look for a batch and stage the transfer
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_STAGE_BATCH_H_
#define _BLOCKS__VNIC_PCI_OUT_STAGE_BATCH_H_

#include <nfcc_chipres.h>

#include <vnic/pci_out_cfg.h>

extern void stage_batch_setup_rings();

extern void stage_batch_setup_shared();

extern void stage_batch_setup();

extern void stage_batch();

extern void distr_seqn_setup_shared();

extern void distr_seqn();

extern void send_desc_setup_shared();

extern void send_desc_setup();

extern void send_desc();

#endif /* !_BLOCKS__VNIC_PCI_OUT_STAGE_BATCH_H_ */
