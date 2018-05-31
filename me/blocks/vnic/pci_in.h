/*
 * Copyright (C) 2012-2018,  Netronome Systems, Inc.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file          blocks/vnic/pci_in.h
 * @brief         Interface to PCI.IN
 */
#ifndef _BLOCKS__VNIC_PCI_IN_H_
#define _BLOCKS__VNIC_PCI_IN_H_

#include <nfp_chipres.h>
#include <pkt/pkt.h>

#include <nfd_user_cfg.h>

#include <nfd_common.h>
#include "shared/nfd_net.h"
#include "shared/nfd_api_common.h"


/** @cond DOXYGEN_SHOULD_SKIP_THIS */

#ifndef NFD_IN_DATA_OFFSET
#define NFD_IN_DATA_OFFSET          64
#endif

#ifndef NFD_IN_WQ_SZ
#error "NFD_IN_WQ_SZ must be defined by the user"
#endif

#ifndef NFD_IN_NUM_WQS
#define NFD_IN_NUM_WQS         8
#endif

#ifndef NFD_IN_BLM_REG_BLS
#error "NFD_IN_BLM_REG_BLS must be defined by the user"
#endif

#ifndef NFD_IN_BLM_REG_POOL
#error "NFD_IN_BLM_REG_POOL must be defined by the user"
#endif

#ifndef NFD_IN_BLM_REG_SIZE
#error "NFD_IN_BLM_REG_SIZE must be defined by the user"
#endif

#ifndef NFD_IN_BLM_JUMBO_BLS
#error "NFD_IN_BLM_JUMBO_BLS must be defined by the user"
#endif

#ifndef NFD_IN_BLM_JUMBO_POOL
#error "NFD_IN_BLM_JUMBO_POOL must be defined by the user"
#endif

#ifndef NFD_IN_BLM_JUMBO_SIZE
#error "NFD_IN_BLM_JUMBO_SIZE must be defined by the user"
#endif

#ifndef NFD_IN_BLM_RADDR
#error "NFD_IN_BLM_RADDR must be defined by the user"
#endif


#define NFD_IN_MAX_QUEUES   64


#ifndef NFD_IN_NUM_SEQRS
#define NFD_IN_NUM_SEQRS 1
#endif

#ifndef NFD_IN_SEQR_QSHIFT
#define NFD_IN_SEQR_QSHIFT 0
#endif

#if (NFD_IN_NUM_SEQRS < 1 || NFD_IN_NUM_SEQRS > 64 || \
    (NFD_IN_NUM_SEQRS & (NFD_IN_NUM_SEQRS - 1)) != 0)
#error "NFD_IN_NUM_SEQRS must be a power of 2 between 1 and 64"
#endif

/** @endcond */


/* Define the default maximum length in bytes of prepended chained metadata.
 * Assume one 32-bit word is used to encode the metadata types in a chain and
 * that each metadata value for a corresponding metadata type is 4 bytes
 * long. */
#ifndef NFD_IN_MAX_META_LEN
#define NFD_IN_MAX_META_LEN (4 * 32 / NFP_NET_META_FIELD_SIZE + 4)
#endif

/* Maximum length of a single meta data item that will be fetched using
 * nfd_in_metadata_pop() */
#ifndef NFD_IN_MAX_META_ITEM_LEN
#define NFD_IN_MAX_META_ITEM_LEN 4
#endif


/**
 * Determine the input sequencer for a packet given its queue number.
 *
 * This macro computes the sequencer number of a particular packet given
 * its queue number.  Sequencers start at 0 and are numbered contiguously
 * and sequentially.  The sequencers for separate PCI.IN blocks are distinct
 * meaning that sequencer 1 for pcie0 is a separate sequence space from
 * sequencer 1 from pcie2, for example.  It is the application's
 * responsibility to map these <pcie, sequencer> tuples to appropriate
 * reorder contexts in GRO or the NBI traffic manager.
 */
#define NFD_IN_SEQR_NUM(_qnum) \
    (((_qnum) >> NFD_IN_SEQR_QSHIFT) & (NFD_IN_NUM_SEQRS - 1))


/** @cond DOXYGEN_SHOULD_SKIP_THIS */

#ifdef NFD_IN_WQ_SHARED

#define NFD_IN_RINGS_DECL_IND2(_isl, _emem)                             \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_in_ring_nums0 _emem##_queues   \
                     global NFD_IN_NUM_WQS NFD_IN_NUM_WQS)              \
    _NFP_CHIPRES_ASM(.declare_resource nfd_in_ring_nums_res0            \
                     global NFD_IN_NUM_WQS nfd_in_ring_nums0)
#define NFD_IN_RINGS_DECL_IND1(_isl, _emem)     \
    NFD_IN_RINGS_DECL_IND2(_isl, _emem)
#define NFD_IN_RINGS_DECL_IND0(_isl)            \
    NFD_IN_RINGS_DECL_IND1(_isl, NFD_IN_WQ_SHARED)
#define NFD_IN_RINGS_DECL(_isl) NFD_IN_RINGS_DECL_IND0(_isl)

#define NFD_IN_RING_NUM_ALLOC_IND(_isl, _num)               \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_in_ring_num0##_num \
                     nfd_in_ring_nums_res0 global 1)
#define NFD_IN_RING_NUM_ALLOC(_isl, _num) NFD_IN_RING_NUM_ALLOC_IND(_isl, _num)

#else /* !NFD_IN_WQ_SHARED */

#define NFD_IN_RINGS_DECL_IND2(_isl, _emem)                             \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_in_ring_nums##_isl _emem##_queues \
                     global NFD_IN_NUM_WQS NFD_IN_NUM_WQS)              \
    _NFP_CHIPRES_ASM(.declare_resource nfd_in_ring_nums_res##_isl       \
                     global NFD_IN_NUM_WQS nfd_in_ring_nums##_isl)
#define NFD_IN_RINGS_DECL_IND1(_isl, _emem)     \
    NFD_IN_RINGS_DECL_IND2(_isl, _emem)
#define NFD_IN_RINGS_DECL_IND0(_isl)            \
    NFD_IN_RINGS_DECL_IND1(_isl, NFD_PCIE##_isl##_EMEM)
#define NFD_IN_RINGS_DECL(_isl) NFD_IN_RINGS_DECL_IND0(_isl)

#define NFD_IN_RING_NUM_ALLOC_IND(_isl, _num)                           \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_in_ring_num##_isl##_num        \
                     nfd_in_ring_nums_res##_isl global 1)
#define NFD_IN_RING_NUM_ALLOC(_isl, _num) NFD_IN_RING_NUM_ALLOC_IND(_isl, _num)

#endif /* NFD_IN_WQ_SHARED */

#define NFD_IN_ISSUED_LSO_RING_DECL_IND2(_isl, _emem, _num)             \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_in_issued_lso_ring_num##_isl##_num \
                     _emem##_queues global 1 1)
#define NFD_IN_ISSUED_LSO_RING_DECL_IND1(_isl, _emem, _num)     \
    NFD_IN_ISSUED_LSO_RING_DECL_IND2(_isl, _emem, _num)
#define NFD_IN_ISSUED_LSO_RING_DECL_IND0(_isl, _num)            \
    NFD_IN_ISSUED_LSO_RING_DECL_IND1(_isl, NFD_PCIE##_isl##_EMEM, _num)
#define NFD_IN_ISSUED_LSO_RING_DECL(_isl, _num)                 \
    NFD_IN_ISSUED_LSO_RING_DECL_IND0(_isl, _num)

#ifdef NFD_PCIE0_EMEM
    NFD_IN_RINGS_DECL(0);
    NFD_IN_RING_NUM_ALLOC(0, 0);
    NFD_IN_ISSUED_LSO_RING_DECL(0, 0);
    NFD_IN_ISSUED_LSO_RING_DECL(0, 1);
#endif

#ifdef NFD_PCIE1_EMEM
    NFD_IN_RINGS_DECL(1);
    NFD_IN_RING_NUM_ALLOC(1, 0);
    NFD_IN_ISSUED_LSO_RING_DECL(1, 0);
    NFD_IN_ISSUED_LSO_RING_DECL(1, 1);
#endif

#ifdef NFD_PCIE2_EMEM
    NFD_IN_RINGS_DECL(2);
    NFD_IN_RING_NUM_ALLOC(2, 0);
    NFD_IN_ISSUED_LSO_RING_DECL(2, 0);
    NFD_IN_ISSUED_LSO_RING_DECL(2, 1);
#endif

#ifdef NFD_PCIE3_EMEM
    NFD_IN_RINGS_DECL(3);
    NFD_IN_RING_NUM_ALLOC(3, 0);
    NFD_IN_ISSUED_LSO_RING_DECL(3, 0);
    NFD_IN_ISSUED_LSO_RING_DECL(3, 1);
#endif

/** @endcond */


/*
 * PCI.IN TX descriptor format
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+-------------+-------------------------------+---------------+
 *    0  |E|   offset    |            dma_len            |  dma_addr_hi  |
 *       +-+-------------+-------------------------------+---------------+
 *    1  |                          dma_addr_lo                          |
 *       +---------------+---------------+---+---------------------------+
 *    2  |     flags     |   lso_hdrlen  |sp0|           mss             |
 *       +---------------+---------------+---|---------------------------+
 *    3  |           data_len            |        vlan [L4/L3 off]       |
 *       +-------------------------------+-------------------------------+
 *
 *      E -> End of packet
 */
/**
 * Host-to-NFD (TX) packet descriptor
 */
struct nfd_in_tx_desc {
    union {
        struct {
            unsigned int eop:1;         /**< Last descriptor of a packet */
            unsigned int offset:7;      /**< Offset of packet start from dma
                                         * address.  (packet metadat length) */
            unsigned int dma_len:16;    /**< Length to DMA for this desc */
            unsigned int dma_addr_hi:8; /**< Bits[39:32] of the host address */

            unsigned int dma_addr_lo:32; /**< Bits[31:0] of the host address */

            unsigned int flags:8;       /**< Flags for the packet */
            unsigned int lso_hdrlen:8;  /**< LSO, TCP payload offset */
            unsigned int sp0:2;
            unsigned int mss:14;        /**< Info for Large Segment Offload */

            unsigned short data_len;    /**< Length of the entire packet */
            union {
                struct {
                    unsigned char l4_offset;    /**< LSO, L4 data start */
                    unsigned char l3_offset;    /**< LSO, L3 data start */
                };
                unsigned short vlan;    /**< VLAN to prepend to this packet */
            };
        };
        unsigned int __raw[4];          /**< Direct access to struct words */
    };
};


/*
 * PCI.in Packet descriptor format
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+-------------+-------------------------------+---+-----------+
 *    0  |S|    offset   |            seq_num            |itf|   q_num   |
 *       +-+-+-+---------+-------------------------------+---+-----------+
 *    1  |I|J|S|                       buf_addr                          |
 *       +-+-+-+---------+---------------+-+-+---------------------------+
 *    2  |     flags     |  lso_seq_cnt  |L|S|           mss             |
 *       +---------------+---------------+-+-+---------------------------+
 *    3  |            data_len           |       vlan [L4/L3 off]        |
 *       +-------------------------------+-------------------------------+
 *
 *    itf -> intf
 *    L -> Last packet in a series of LSO packets
 *    S -> sp0, sp1, and sp2 (spare)
 */
/**
 * NFD-to-App (TX) packet descriptor
 */
struct nfd_in_pkt_desc {
    union {
        struct {
            unsigned int sp0:1;         /**< Spare bit (unused) */
            unsigned int offset:7;      /**< Offset of packet data from start
                                         *   of buffer + NFD_IN_DATA_OFFSET.
                                         *   aka (prepended metadata length) */
            unsigned int seq_num:16;    /**< Sequence number of the packet */
            unsigned int intf:2;        /**< PCIe num the packet arrived on */
            unsigned int q_num:6;       /**< Queue num the packet arrived on */

            unsigned int invalid:1;     /**< Packet invalid, recommend drop */
            unsigned int jumbo:1;       /**< buf_addr from jumbo pool */
            unsigned int sp2:1;         /**< Spare bit (unused) */
            unsigned int buf_addr:29;   /**< Bits [39:11] of the MU buffer */

            unsigned int flags:8;       /**< Flags for the packet */
            unsigned int lso_seq_cnt:8; /**< LSO index/count for this series */
            unsigned int lso_end:1;     /**< Last packet in a series of LSO packets */
            unsigned int sp1:1;         /**< Spare bit (unused) */
            unsigned int mss:14;        /**< Info for Large Segment Offload */

            unsigned short data_len;    /**< Length of the entire packet */
            union {
                struct {
                    unsigned char l4_offset;    /**< LSO, L4 data start */
                    unsigned char l3_offset;    /**< LSO, L3 data start */
                };
                unsigned short vlan;    /**< VLAN to prepend to this packet */
            };
        };
        unsigned int __raw[4];          /**< Direct access to struct words */
    };
};


/**
 * Prepare ME data structures required to receive packets from NFD
 *
 * This method should be called from a single context, during initialization.
 */
__intrinsic void nfd_in_recv_init(void);


/**
 * Receive a packet from PCI.IN
 * @param pcie_isl      PCIe island
 * @param workq         Work queue from the given island to access
 * @param nfd_in_meta   PCI.IN descriptor for the packet
 * @param sync          Type of synchronization
 * @param sig           Signal to report completion
 */
__intrinsic void __nfd_in_recv(unsigned int pcie_isl, unsigned int workq,
                               __xread struct nfd_in_pkt_desc *nfd_in_meta,
                               sync_t sync, SIGNAL *sig);

/**
 * Receive a packet from PCI.IN
 * @param pcie_isl      PCIe island
 * @param workq         Work queue from the given island to access
 *                      (must be 0)
 * @param nfd_in_meta   PCI.IN descriptor for the packet
 */
__intrinsic void nfd_in_recv(unsigned int pcie_isl, unsigned int workq,
                             __xread struct nfd_in_pkt_desc *nfd_in_meta);


/**
 * Increment packet and byte counts for PCI.IN queues.
 * @param pcie_isl      PCIe island
 * @param queue         TX queue number
 * @param byte_count    The bytes count to add
 * @param sync          Type of synchronization
 * @param sig           Signal to report completion
 *
 * This function uses the stats engine to log the packet and byte
 * counts per TX queue.  The values are accumulated in the nfd_in_cntrsX
 * memory (where X is the PCIe unit number) and needs to be pushed to the
 * CFG BAR using the "__nfd_in_push_pkt_cnt" function.
 */
__intrinsic void __nfd_in_cnt_pkt(unsigned int pcie_isl,
                                  unsigned int queue,
                                  unsigned int byte_count,
                                  sync_t sync, SIGNAL *sig);

/**
 * Push packet and bytes counts for PCI.IN queue into the CFG BAR.
 * @param pcie_isl      PCIe island
 * @param queue         TX queue number
 * @param sync          Type of synchronization
 * @param sig           Signal to report completion
 *
 * This function updates the per TX queue packets and bytes counter
 * in the CFG BAR. It reads and clears the packet and byte
 * counts from the relevant nfd_in_cntrsX memory (where X is the
 * PCIe unit number) and updates the CFG BAR counters using the read values.
 */
__intrinsic void __nfd_in_push_pkt_cnt(unsigned int pcie_isl,
                                       unsigned int queue,
                                       sync_t sync, SIGNAL *sig);


/**
 * Retrieve metadata from prepended packet metadata chain.
 * @param meta_val      Retrieved metadata
 * @param meta_len      Length of metadata unretrieved before calling
 * @param meta_info     Encodes types of metadata in metadata chain
 * @param pkt_buf_ptr   Pointer to start of packet buffer
 *
 * The application firmware initialises meta_len to the length of metadata
 * prepended to the packet and meta_info to zero. The meta_info is then
 * updated by this function as metadata is retrieved.
 * Returns the meta type if there is more data, zero if meta_len is zero, else
 * -1 to indicate an error.  The user can set NFD_IN_MAX_META_LEN to specify
 * the maximum amount of prepend metadata that they expect.  If meta_len is
 * larger than this value, it is caught as an error.
 * "meta_val" must be sized to hold NFD_IN_MAX_META_ITEM_LEN (default 4B).  The
 * user inspects the type returned to decide how to process the metadata, and
 * to determine the amount of data associated with this type.
 *
 * @note the user must subtract the data length of the meta type from meta_len
 * before invoking this function again.
 */
__intrinsic int nfd_in_metadata_pop(void *meta_val,
                                    unsigned int *meta_len,
                                    unsigned int *meta_info,
                                    __mem40 void *pkt_buf_ptr);


/**
 * Retrieve metadata from cache of prepended packet metadata chain.
 * @param meta_val          Retrieved metadata
 * @param meta_len          Length of metadata unretrieved before calling
 * @param meta_info         Encodes types of metadata in metadata chain
 * @param meta_cache        Transfer registers in which metadata chain cached
 * @param meta_cache_ptr    Pointer to the location in the cache to use next
 * @param meta_cache_len    Size of metadata cache in bytes
 * @param pkt_buf_ptr       Pointer to start of packet buffer
 *
 * The application firmware initialises meta_len to the length of metadata
 * prepended to the packet and meta_info to zero. The meta_info is then
 * updated by this function as metadata is retrieved.
 * Returns the meta type if there is more data, zero if meta_len is zero, else
 * -1 to indicate an error.  The user can set NFD_IN_MAX_META_LEN to specify
 * the maximum amount of prepend metadata that they expect.  If meta_len is
 * larger than this value, it is caught as an error.
 * "meta_val" must be sized to hold NFD_IN_MAX_META_ITEM_LEN (default 4B).  The
 * user inspects the type returned to decide how to process the metadata, and
 * to determine the amount of data associated with this type.
 *
 * @note the user must subtract the data length of the meta type from "meta_len"
 * and must add the data length to "meta_cache_ptr" before invoking this
 * function again.
 */
__intrinsic int nfd_in_metadata_pop_cache(unsigned int *meta_val,
                                          unsigned int *meta_len,
                                          unsigned int *meta_info,
                                          __xread unsigned int *meta_cache,
                                          unsigned int *meta_cache_ptr,
                                          const unsigned int meta_cache_len,
                                          __mem40 void *pkt_buf_ptr);


/**
 * Populate a nbi_meta_pkt_info structure from the NFD meta data.
 * @param pkt_info     nbi_meta_pkt_info struct for the packet
 * @param nfd_in_meta  PCI.IN descriptor for the packet
 *
 * "pkt_info->isl", "pkt_info->pnum", and "pkt_info->split" are set to zero
 * as PCI.IN returns an "MU only" packet.
 */
__intrinsic void nfd_in_fill_meta(void *pkt_info,
                                  __xread struct nfd_in_pkt_desc *nfd_in_meta);


/**
 * Map an NFD bitmask queue to a vnic, queue number pair.
 * @param type      vNIC type as seen by the host
 * @param vnic      vNIC within type as seen by the host
 * @param queue     Queue number within the vNIC
 * @param nfd_queue Queue number within NFD numbering system
 */
__intrinsic void nfd_in_map_queue(unsigned int *type, unsigned int *vnic,
                                  unsigned int *queue, unsigned int nfd_queue);

/**
 * Get the length of the packet excluding metadata.
 * @param nfd_in_meta   PCI.IN descriptor for the packet
 * @return              The number of bytes of packet data
 */
__intrinsic unsigned int nfd_in_pkt_len(
    __xread struct nfd_in_pkt_desc *nfd_in_meta);


/**
 * Get the sequence number for a PCI.IN sourced packet.
 * @param nfd_in_meta   PCI.IN descriptor for the packet
 * @return              The sequence number of the packet
 */
__intrinsic unsigned int nfd_in_get_seqn(
    __xread struct nfd_in_pkt_desc *nfd_in_meta);

#endif /* !_BLOCKS__VNIC_PCI_IN_H_ */

