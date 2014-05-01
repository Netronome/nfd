/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/send_desc.c
 * @brief         Code to transmit RX descriptors to the host
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_SEND_DESC_H_
#define _BLOCKS__VNIC_PCI_OUT_SEND_DESC_H_

extern void send_desc_setup_shared();

__intrinsic void send_desc_vnic_setup(void *cfg_msg_in,
                                      unsigned int queue_size);

#endif /* !_BLOCKS__VNIC_PCI_OUT_SEND_DESC_H_ */
