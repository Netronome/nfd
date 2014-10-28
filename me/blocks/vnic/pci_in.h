/*
 * Copyright (C) 2012-2013 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in.h
 * @brief         Interface to PCI.IN
 */
#ifndef _BLOCKS__VNIC_PCI_IN_H_
#define _BLOCKS__VNIC_PCI_IN_H_

#include <pkt/pkt.h>

#ifndef NFD_IN_DATA_OFFSET
#define NFD_IN_DATA_OFFSET          64
#endif

#ifndef NFD_IN_WQ_SZ
#define NFD_IN_WQ_SZ           (16 * 1024)
#endif

#ifndef NFD_IN_NUM_WQS
#define NFD_IN_NUM_WQS         8
#endif

/* XXX expose to the APP */
#define NFD_IN_BLM_BLS          0
#define NFD_IN_BLM_POOL         BLM_NBI8_BLQ0_EMU_QID
#define NFD_IN_BLM_RADDR        __LoadTimeConstant("__addr_emem1")


#define NFD_IN_MAX_QUEUES   64


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
struct nfd_in_tx_desc {
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
struct nfd_in_pkt_desc {
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
__intrinsic void nfd_in_recv_init();


/**
 * Receive a packet from PCI.IN
 * @param pcie_isl      PCIe island to access
 * @param workq         work queue from the given island to access
 * @param nfd_in_meta   PCI.IN descriptor for the packet
 * @param sync          type of synchronization
 * @param sig           signal to report completion
 */
__intrinsic void __nfd_in_recv(unsigned int pcie_isl, unsigned int workq,
                               __xread struct nfd_in_pkt_desc *nfd_in_meta,
                               sync_t sync, SIGNAL *sig);

__intrinsic void nfd_in_recv(unsigned int pcie_isl, unsigned int workq,
                             __xread struct nfd_in_pkt_desc *nfd_in_meta);


/**
 * Populate a nfd_in_pkt_desc struct from the NFD meta data
 * @param desc      PCI.IN descriptor for the packet
 * @param pkt_info  nbi_meta_pkt_info struct for the packet
 *
 * "pkt_info->isl", "pkt_info->pnum", and "pkt_info->split" are set to zero
 * as PCI.IN returns an "MU only" packet.
 */
__intrinsic void nfd_in_fill_meta(void *pkt_info,
                                  __xread struct nfd_in_pkt_desc *nfd_in_meta);


/**
 * Map an NFD bitmask queue to a vnic, queue number pair
 * @param vnic      vNIC as seen by the host
 * @param queue     queue number within the vNIC
 * @param nfd_queue queue number within NFD "bitmask" numbering system
 */
void nfd_in_map_queue(unsigned int *vnic, unsigned int *queue,
                      unsigned int nfd_queue);

__intrinsic unsigned int nfd_in_pkt_len(
    __xread struct nfd_in_pkt_desc *nfd_in_meta);


#endif /* !_BLOCKS__VNIC_PCI_IN_H_ */

