/*
 * Copyright (C) 2012-2013 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in.h
 * @brief         Interface to PCI.IN
 */
#ifndef _BLOCKS__VNIC_PCI_IN_H_
#define _BLOCKS__VNIC_PCI_IN_H_

#include <pkt/pkt.h>

#include "nfd_user_cfg.h"

#include <vnic/shared/nfcc_chipres.h>

#ifndef NFD_IN_DATA_OFFSET
#define NFD_IN_DATA_OFFSET          64
#endif

#ifndef NFD_IN_WQ_SZ
#error "NFD_IN_WQ_SZ must be defined by the user"
#endif

#ifndef NFD_IN_NUM_WQS
#define NFD_IN_NUM_WQS         8
#endif

#ifndef NFD_IN_BLM_BLS
#error "NFD_IN_BLM_BLS must be defined by the user"
#endif

#ifndef NFD_IN_BLM_POOL
#error "NFD_IN_BLM_POOL must be defined by the user"
#endif

#ifndef NFD_IN_BLM_RADDR
#error "NFD_IN_BLM_RADDR must be defined by the user"
#endif


#define NFD_IN_MAX_QUEUES   64


#ifdef NFD_IN_WQ_SHARED

#define NFD_IN_RINGS_DECL_IND2(_isl, _emem)                             \
    _emem##_queues_DECL                                                 \
    ASM(.alloc_resource nfd_in_ring_nums0 _emem##_queues global         \
        NFD_IN_NUM_WQS NFD_IN_NUM_WQS)                                  \
    ASM(.declare_resource nfd_in_ring_nums_res0 global NFD_IN_NUM_WQS   \
        nfd_in_ring_nums0)
#define NFD_IN_RINGS_DECL_IND1(_isl, _emem)    \
    NFD_IN_RINGS_DECL_IND2(_isl, _emem)
#define NFD_IN_RINGS_DECL_IND0(_isl)                \
    NFD_IN_RINGS_DECL_IND1(_isl, NFD_IN_WQ_SHARED)
#define NFD_IN_RINGS_DECL(_isl) NFD_IN_RINGS_DECL_IND0(_isl)

#define NFD_IN_RING_NUM_ALLOC_IND(_isl, _num)                           \
    ASM(.alloc_resource nfd_in_ring_num0##_num nfd_in_ring_nums_res0    \
        global 1)
#define NFD_IN_RING_NUM_ALLOC(_isl, _num) NFD_IN_RING_NUM_ALLOC_IND(_isl, _num)

#else /* !NFD_IN_WQ_SHARED */

#define NFD_IN_RINGS_DECL_IND2(_isl, _emem)                             \
    _emem##_queues_DECL                                                 \
    ASM(.alloc_resource nfd_in_ring_nums##_isl _emem##_queues global    \
        NFD_IN_NUM_WQS NFD_IN_NUM_WQS)                                  \
    ASM(.declare_resource nfd_in_ring_nums_res##_isl global NFD_IN_NUM_WQS  \
        nfd_in_ring_nums##_isl)
#define NFD_IN_RINGS_DECL_IND1(_isl, _emem)    \
    NFD_IN_RINGS_DECL_IND2(_isl, _emem)
#define NFD_IN_RINGS_DECL_IND0(_isl)                    \
    NFD_IN_RINGS_DECL_IND1(_isl, NFD_PCIE##_isl##_EMEM)
#define NFD_IN_RINGS_DECL(_isl) NFD_IN_RINGS_DECL_IND0(_isl)

#define NFD_IN_RING_NUM_ALLOC_IND(_isl, _num)                           \
    ASM(.alloc_resource nfd_in_ring_num##_isl##_num nfd_in_ring_nums_res##_isl \
        global 1)
#define NFD_IN_RING_NUM_ALLOC(_isl, _num) NFD_IN_RING_NUM_ALLOC_IND(_isl, _num)

#endif /* NFD_IN_WQ_SHARED */


#ifdef NFD_PCIE0_EMEM
    NFD_IN_RINGS_DECL(0);
    NFD_IN_RING_NUM_ALLOC(0, 0);
#endif

#ifdef NFD_PCIE1_EMEM
    NFD_IN_RINGS_DECL(1);
    NFD_IN_RING_NUM_ALLOC(1, 0);
#endif

#ifdef NFD_PCIE2_EMEM
    NFD_IN_RINGS_DECL(2);
    NFD_IN_RING_NUM_ALLOC(2, 0);
#endif

#ifdef NFD_PCIE3_EMEM
    NFD_IN_RINGS_DECL(3);
    NFD_IN_RING_NUM_ALLOC(3, 0);
#endif


/**
 * PCI.in TX descriptor format
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+-------------+-------------------------------+---------------+
 *    0  |E|   offset    |            dma_len            |  dma_addr_hi  |
 *       +-+-------------+-------------------------------+---------------+
 *    1  |                          dma_addr_lo                          |
 *       +---------------+---------------+-------------------------------+
 *    2  |     flags     |   l4_offsets  |               lso             |
 *       +---------------+---------------+-------------------------------+
 *    3  |           data_len            |              vlan             |
 *       +-------------------------------+-------------------------------+
 *
 *      E -> End of packet
 */
struct nfd_in_tx_desc {
    union {
        struct {
            unsigned int eop:1;
            unsigned int offset:7;
            unsigned int dma_len:16;
            unsigned int dma_addr_hi:8;

            unsigned int dma_addr_lo:32;

            unsigned int flags:8;
            unsigned int l4_offset:8;
            unsigned int lso:16;

            unsigned int data_len:16;
            unsigned int vlan:16;
        };
        unsigned int __raw[4];
    };
};


/**
 * PCI.in Packet descriptor format
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+-------------+-----------------------------+---+-------------+
 *    0  |S|    offset   |           reserved          |itf|    q_num    |
 *       +-+-------------+-----------------------------+---+-------------+
 *    1  |                           buf_addr                            |
 *       +---------------+---------------+-------------------------------+
 *    2  |     flags     |   l4_offset   |               lso             |
 *       +---------------+---------------+-------------------------------+
 *    3  |            data_len           |              vlan             |
 *       +-------------------------------+-------------------------------+
 *
 *      S -> sp0 (spare)
 *    itf -> intf
 */
struct nfd_in_pkt_desc {
    union {
        struct {
            unsigned int sp0:1;
            unsigned int offset:7;
            unsigned int reserved:16;
            unsigned int intf:2;
            unsigned int q_num:6;

            unsigned int buf_addr:32;

            unsigned int flags:8;
            unsigned int l4_offset:8;
            unsigned int lso:16;

            unsigned int data_len:16;
            unsigned int vlan:16;
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
__intrinsic void nfd_in_map_queue(unsigned int *vnic, unsigned int *queue,
                                  unsigned int nfd_queue);

__intrinsic unsigned int nfd_in_pkt_len(
    __xread struct nfd_in_pkt_desc *nfd_in_meta);


/**
 * Get the dst_q sequence number (if NFD is configured to add it)
 * @param nfd_in_meta   PCI.IN descriptor for the packet
 */
__intrinsic unsigned int nfd_in_get_seqn(
    __xread struct nfd_in_pkt_desc *nfd_in_meta);

#endif /* !_BLOCKS__VNIC_PCI_IN_H_ */

