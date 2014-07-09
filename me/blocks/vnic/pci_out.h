/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out.h
 * @brief         Interface to PCI.OUT
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_H_
#define _BLOCKS__VNIC_PCI_OUT_H_


/*
 * RX and freelist descriptor format
 */
struct nfd_pci_out_rx_desc {
    union {
        /* Freelist descriptor */
        struct {
            unsigned int dd:1;
            unsigned int spare:23;
            unsigned int dma_addr_hi:8;

            unsigned int dma_addr_lo;
        } fld;
        /* RX descriptor */
        struct {
            unsigned int dd:1;
            unsigned int offset:7;
            unsigned int reserved:8;
            unsigned int data_len:16;

            unsigned int vlan:16;
            unsigned int spare:12;
            unsigned int flags:4;
        } rxd;
        unsigned int __raw[2];
    };
};


#endif /* !_BLOCKS__VNIC_PCI_OUT_H_ */
