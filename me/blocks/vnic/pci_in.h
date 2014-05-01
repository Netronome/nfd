/*
 * Copyright (C) 2012-2013 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in.h
 * @brief         Interface to PCI.IN
 */
#ifndef _BLOCKS__VNIC_PCI_IN_H_
#define _BLOCKS__VNIC_PCI_IN_H_

/**
 * PCI.in TX descriptor format
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+-----+---+-+-----------------+---------+-----+---------------+
 *    0  |E| sp0 |        dma_len        |   sp1   |dst_q|    dma_addr   |
 *       +-+-----+---+-+-----------------+---------+-----+---------------+
 *    1  |                          dma_addr_lo                          |
 *       +-+-------------+---------------+-------------------------------+
 *    2  |0|  offset     |      sp2      |            data_len           |
 *       +-+-------------+---------------+-----------------------+-------+
 *    3  |             VLAN              |          sp3          | flags |
 *       +-------------------------------+-----------------------+-------+
 *
 *      sp0 - sp3 -> spare
 *      E -> End of packet
 */
struct nfd_pci_in_tx_desc {
    unsigned int eop:1;
    unsigned int sp0:3;
    unsigned int dma_len:12;
    unsigned int sp1:5;
    unsigned int dst_q:3;
    unsigned int dma_addr_hi:8;
    unsigned int dma_addr_lo:32;
    unsigned int valid:1;
    unsigned int offset:7;
    unsigned int sp2:8;
    unsigned int data_len:16;
    unsigned int vlan:16;
    unsigned int sp3:12;
    unsigned int flags:4;
};


#endif /* !_BLOCKS__VNIC_PCI_IN_H_ */

