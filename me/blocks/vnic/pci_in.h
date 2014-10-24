/*
 * Copyright (C) 2012-2013 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in.h
 * @brief         Interface to PCI.IN
 */
#ifndef _BLOCKS__VNIC_PCI_IN_H_
#define _BLOCKS__VNIC_PCI_IN_H_

#include <pkt/pkt.h>


/**
 * PCI.in TX descriptor format
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+-----+-----------------------+---------+-----+---------------+
 *    0  |E| sp0 |        dma_len        |   sp1   |dst_q|  dma_addr_hi  |
 *       +-+-----+-----------------------+---------+-----+---------------+
 *    1  |                          dma_addr_lo                          |
 *       +-+-------------+-------+-------+-------------------------------+
 *    2  |0|  offset     |  sp2  | flags |            data_len           |
 *       +-+-------------+-------+-------+-------------------------------+
 *    3  |             VLAN              |               sp3             |
 *       +-------------------------------+-------------------------------+
 *
 *      sp0 - sp3 -> spare
 *      E -> End of packet
 */
struct nfd_pci_in_tx_desc {
    union {
        struct {
            unsigned int eop:1;
            unsigned int sp0:3;
            unsigned int dma_len:12;
            unsigned int sp1:5;
            unsigned int dst_q:3;
            unsigned int dma_addr_hi:8;
            unsigned int dma_addr_lo:32;
            unsigned int valid:1;
            unsigned int offset:7;
            unsigned int sp2:4;
            unsigned int flags:4;
            unsigned int data_len:16;
            unsigned int vlan:16;
            unsigned int sp3:16;
        };
        unsigned int __raw[4];
    };
};


/**
 * PCI.in Packet descriptor format
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +----+------------+---------------------------------------------+
 *    0  |intf|   q_num    |                     sp1                     |
 *       +----+------------+---------------------------------------------+
 *    1  |                           buf_addr                            |
 *       +-+-------------+-------+-------+-------------------------------+
 *    2  |0|  offset     |  sp2  | flags |            data_len           |
 *       +-+-------------+-------+-------+-------------------------------+
 *    3  |             VLAN              |               sp3             |
 *       +-------------------------------+-------------------------------+
 *
 *      sp0 - sp3 -> spare
 * XXX use compact buffer address?
 */
struct nfd_pci_in_pkt_desc {
    union {
        struct {
            unsigned int intf:2;
            unsigned int q_num:6;
            unsigned int sp1:24;
            unsigned int buf_addr:32;
            unsigned int valid:1;
            unsigned int offset:7;
            unsigned int sp2:4;
            unsigned int flags:4;
            unsigned int data_len:16;
            unsigned int vlan:16;
            unsigned int sp3:16;
        };
        unsigned int __raw[4];
    };
};


/**
 * Prepare ME data structures required to receive packets from NFD
 *
 * This method should be called from a single context, during initialisation.
 */
__intrinsic void nfd_pkt_recv_init();


__intrinsic void __nfd_pkt_recv(unsigned int pcie_isl, unsigned int workq,
                                __xread struct nfd_pci_in_pkt_desc *pci_in_meta,
                                sync_t sync, SIGNAL *sig);

/**
 * Populate a nfd_pci_in_pkt_desc struct from the NFD meta data
 * @param desc      PCI.IN descriptor for the packet
 * @param pkt_info  nbi_meta_pkt_info struct for the packet
 *
 * "pkt_info->isl", "pkt_info->pnum", and "pkt_info->split" are set to zero
 * as PCI.IN returns an "MU only" packet.
 */
__intrinsic void nfd_fill_meta(void *pkt_info,
                               __xread struct nfd_pci_in_pkt_desc *pci_in_meta);


void pci_in_map_queue(unsigned int *vnic, unsigned int *queue,
                      unsigned int nfd_queue);

__intrinsic unsigned int pci_in_pkt_len(
    __xread struct nfd_pci_in_pkt_desc *pci_in_meta);


#endif /* !_BLOCKS__VNIC_PCI_IN_H_ */

