/**
 * Copyright (C) 2014-2018,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/pci_out.h
 * @brief         Interface to PCI.OUT
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_H_
#define _BLOCKS__VNIC_PCI_OUT_H_

/* Host code may include this file, so use <> to allow non-local paths */
#include <nfd_net.h>

/** The maximum number of queues a single PCI.OUT block can support */
#define NFD_OUT_MAX_QUEUES              64

/**
 * NFP-to-Host (RX) packet descriptor
 *
 * The fields "data_len" and "queue" are used by PCI.OUT when DMA'ing packet
 * data.  The "queue" field is seen as a reserved field by the host.
 */
struct nfd_out_rx_desc {
    union {
        struct {
            unsigned int dd:1;          /**< Descriptor done, must be set */
            unsigned int meta_len:7;    /**< Length of meta data prepended */
            unsigned int queue:8;       /**< Queue, bitmask numbered */
            unsigned int data_len:16;   /**< Length of frame + meta data */

            unsigned int vlan:16;       /**< VLAN if stripped */
            unsigned int flags:16;      /**< RX flags */
        };
        unsigned int __raw[2];          /**< Direct access to struct words */
    };
};


/**
 * App-to-NFD packet CPP descriptor
 *
 * If the "isl" field is zero, the packet is handled as "MU only".  It is the
 * user's responsibility to free the CTM buffer associated with packets that
 * are submitted as "MU only".
 *
 * "offset" points to the start of the data to be DMA'ed in NFP memory.  The
 * value is shared by DMAs from the MU and the CTM, so it must be consistent
 * between the two buffers.
 *
 * The BLM queue for the MU buffer is selected by queue = ("nbi"<<2) | bls.
 * If BLM queues are shared between the NBIs, "nbi" should be set to zero.
 *
 * Packets that are contained entirely in CTM must be flagged for fastpath
 * processing by setting "ctm_only".
 */
struct nfd_out_cpp_desc {
    union {
        struct {
            unsigned int isl:6;         /**< CTM isl, zero for MU only pkts */
            unsigned int ctm_only:1;    /**< 1 if packet is entirely in CTM */
            unsigned int pktnum:9;      /**< CTM packet number */
            unsigned int split:2;       /**< CTM buffer size of the pkt */
            unsigned int reserved:1;    /**< Must be zero from application */
            unsigned int offset:13;     /**< Offset where data starts in NFP */

            unsigned int nbi:1;         /**< NBI that received the pkt */
            unsigned int bls:2;         /**< NBI buffer list */
            unsigned int mu_addr:29;    /**< Pkt MU address */
        };
        unsigned int __raw[2];          /**< Direct access to struct words */
    };
};


/**
 * App-to-NFD packet descriptor
 */
struct nfd_out_input {
    struct nfd_out_cpp_desc cpp;        /**< NFP portion of the desriptor */
    struct nfd_out_rx_desc rxd;         /**< Host portion of the descriptor */
};


/*
 * Format of PCI.OUT CTM atomic data
 */
/** \cond DOXYGEN_SHOULD_SKIP_THIS */
#define NFD_OUT_ATOMICS_SZ          16
#define NFD_OUT_ATOMICS_SZ_LG2      4
#define NFD_OUT_ATOMICS_CREDIT      0
#define NFD_OUT_ATOMICS_SENT        4
#define NFD_OUT_ATOMICS_DMA_DONE    8
/** \endcond */


#if defined(__NFP_LANG_MICROC)

#include <pkt/pkt.h>

#include <nfd_user_cfg.h>

#include <nfp_chipres.h>
#include "shared/nfd_api_common.h"
#include <nfd_common.h>

/** \cond DOXYGEN_SHOULD_SKIP_THIS */

#define NFD_OUT_RINGS_DECL_IND2(_isl, _emem)                            \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_out_ring_num##_isl##0          \
                     _emem##_queues global 1 1)
#define NFD_OUT_RINGS_DECL_IND1(_isl, _emem)    \
    NFD_OUT_RINGS_DECL_IND2(_isl, _emem)
#define NFD_OUT_RINGS_DECL_IND0(_isl)           \
    NFD_OUT_RINGS_DECL_IND1(_isl, NFD_PCIE##_isl##_EMEM)
#define NFD_OUT_RINGS_DECL(_isl) NFD_OUT_RINGS_DECL_IND0(_isl)

#ifdef NFD_PCIE0_EMEM
    NFD_OUT_RINGS_DECL(0);
#endif

#ifdef NFD_PCIE1_EMEM
    NFD_OUT_RINGS_DECL(1);
#endif

#ifdef NFD_PCIE2_EMEM
    NFD_OUT_RINGS_DECL(2);
#endif

#ifdef NFD_PCIE3_EMEM
    NFD_OUT_RINGS_DECL(3);
#endif

#ifndef NFD_OUT_RING_SZ
#error "NFD_OUT_RING_SZ must be defined by the user"
#endif

/** \endcond */


/* Define the default maximum length in bytes of prepended chained metadata.
 * Assume one 32-bit word is used to encode the metadata types in a chain and
 * that each metadata value for a corresponding metadata type is 4 bytes
 * long. */
#ifndef NFD_OUT_MAX_META_LEN
#define NFD_OUT_MAX_META_LEN (4 * 32 / NFP_NET_META_FIELD_SIZE + 4)
#endif

/* Maximum length of a single meta data item that will be prepended using
 * nfd_out_metadata_push() */
#ifndef NFD_OUT_MAX_META_ITEM_LEN
#define NFD_OUT_MAX_META_ITEM_LEN 4
#endif


/**
 * Prepare ME data structures required to send packets to NFD.
 *
 * This method should be called from a single context, during initialization.
 */
__intrinsic void nfd_out_send_init(void);


/**
 * Map a vNIC, queue number pair to an NFD queue number.
 * @param type      vNIC type as seen by the host
 * @param vnic      vNIC within type as seen by the host
 * @param queue     Queue number within the vNIC
 * @return          The corresponding NFD queue number
 *
 * This method returns an NFD queue number (the numbering system used
 * internally in PCI.OUT and PCI.IN) from a vNIC, queue number pair.
 */
__intrinsic unsigned int nfd_out_map_queue(unsigned int type,
                                           unsigned int vnic,
                                           unsigned int queue);


/**
 * Request credits from specified queue.
 * @param pcie_isl      PCIe island
 * @param queue         Queue to acquire credits for
 * @param num           Number of credits to request
 * @param data          Transfer register to use for the request
 * @param sync          Type of sychronization
 * @param sigpair       Signals to use for the operation
 *
 * This method returns the number of credits available before the request.
 * It is the user's responsibility to verify that this is non-zero if one
 * credit was requested, and greater than or equal to num if more than one
 * credit was requested.
 */

__intrinsic void __nfd_out_get_credit(unsigned int pcie_isl,
                                      unsigned int queue, unsigned int num,
                                      __xrw unsigned int *data,
                                      sync_t sync, SIGNAL_PAIR *sigpair);

/**
 * Request credits from specified queue.
 * @param pcie_isl      PCIe island
 * @param queue         Queue to acquire credits for
 * @param num           Number of credits to request
 *
 * This method returns the number of credits available before the request.
 * It is the user's responsibility to verify that this is non-zero if one
 * credit was requested, and greater than or equal to num if more than one
 * credit was requested.
 */
__intrinsic unsigned int nfd_out_get_credit(unsigned int pcie_isl,
                                            unsigned int queue,
                                            unsigned int num);


/**
 * Packets and Bytes count for PCI.OUT queues.
 * @param pcie_isl      PCIe island
 * @param queue         RX queue number
 * @param byte_count    The bytes count to add
 * @param sync          Type of synchronization
 * @param sig           Signal to report completion
 *
 * This function uses the stats engine to log the packet and byte counts per
 * RX queue.  The values are accumulated in the nfd_out_cntrsX memory (where X
 * is the PCIe unit number) and needs to be pushed to the CFG BAR using the
 * __nfd_out_push_pkt_cnt() function.
 */
__intrinsic void __nfd_out_cnt_pkt(unsigned int pcie_isl,
                                   unsigned int queue,
                                   unsigned int byte_count,
                                   sync_t sync, SIGNAL *sig);

/**
 * Push Packets and Bytes count for PCI.OUT queue into the CFG BAR.
 * @param pcie_isl      PCIe island
 * @param queue         RX queue number
 * @param sync          Type of synchronization
 * @param sig           Signal to report completion
 *
 * This function updates the per RX queue packet and byte counters
 * in the CFG BAR. It reads and clears the packet and byte
 * counts from the relevant nfd_in_cntrsX memory (where X is the
 * PCIe unit number) and updates the CFG BAR counters using the read values.
 */
__intrinsic void __nfd_out_push_pkt_cnt(unsigned int pcie_isl,
                                        unsigned int queue,
                                        sync_t sync, SIGNAL *sig);
/**
 * Push metadata onto prepended packet metadata chain.
 * @param meta_len          Length of metadata currently prepended
 * @param meta_types        Types of metadata being prepended
 * @param meta_val          Metadata to prepend
 * @param meta_type_num     Number of metadata type fields being prepended
 * @param meta_val_len      Length of metadata value in bytes
 * @param meta_val_len_max  Maximum length of metadata value in bytes
 * @param pkt_start_ptr     Pointer to start of packet
 * @param sync              Type of synchronization
 * @param sig               Signal to report completion
 *
 * The application firmware initialises meta_len to zero and it is then
 * updated by this function as metadata is prepended. Returns -1 if the
 * maximum supported metadata length (defined by NFD_OUT_MAX_META_LEN)
 * is exceeded.
 * NFD_OUT_MAX_META_ITEM_LEN is used to allocate sufficient write xfers.
 */
__intrinsic int __nfd_out_metadata_push(void *meta_len,
                                        unsigned int meta_types,
                                        void *meta_val,
                                        unsigned int meta_type_num,
                                        unsigned int meta_val_len,
                                        const unsigned int meta_val_len_max,
                                        __mem40 void *pkt_start_ptr,
                                        sync_t sync, SIGNAL *sig);

/**
 * Push metadata onto prepended packet metadata chain.
 * @param meta_len      Length of metadata currently prepended
 * @param meta_types    Types of metadata being prepended
 * @param meta_val      Metadata to prepend
 * @param meta_type_num Number of metadata type fields being prepended
 * @param meta_val_len  Length of metadata value in bytes
 * @param pkt_start_ptr Pointer to start of packet
 *
 * The application firmware initialises meta_len to zero and it is then
 * updated by this function as metadata is prepended. Returns -1 if the
 * maximum supported metadata length (defined by NFD_OUT_MAX_META_LEN)
 * is exceeded.
 * NFD_OUT_MAX_META_ITEM_LEN is used to allocate sufficient write xfers.
 */
__intrinsic int nfd_out_metadata_push(void *meta_len,
                                      unsigned int meta_types,
                                      void *meta_val,
                                      const unsigned int meta_type_num,
                                      const unsigned int meta_val_len,
                                      __mem40 void *pkt_start_ptr);

/**
 * Populate the buffer address fields of the CPP descriptor for a packet.
 * @param desc          PCI.OUT descriptor to fill
 * @param pkt_info      Up to date nbi_meta_pkt_info struct for the packet
 * @param nbi           NBI that received the packet
 * @param ctm_split     CTM split length for the packet
 * @param pkt_start     Start address of packet data
 * @param meta_len      Amount of prepended meta data
 *
 * If both NBIs share BLM pools, set "nbi" to zero.  For MU only packets,
 * "pkt_info->isl" must be zero.  "ctm_split" must be encoded as for the
 * "NbiDmaBPCfg" register (see the NFP Databook).
 */
__intrinsic void nfd_out_fill_desc(__gpr struct nfd_out_input *desc,
                                   void *pkt_info,
                                   unsigned int nbi, unsigned int ctm_split,
                                   unsigned int pkt_start,
                                   unsigned int meta_len);

/**
 * Set "ctm_only" correctly based on other fields in "desc".
 *
 * @param desc          The descriptor to update.
 *
 * This function will parse "desc" and compute whether the whole packet
 * is held in the CTM buffer (if any).  If so, it will set "ctm_only" to 1.
 * Accessing the necessary data from "desc" may be less efficient than
 * setting "ctm_only" based on other context specific knowledge.  "ctm_only"
 * may be set directly in these cases.
 */
__intrinsic void nfd_out_check_ctm_only(__gpr struct nfd_out_input *desc);


/**
 * Fill the VLAN and flag parameters in the PCI.OUT descriptor.
 *
 * @param desc          The descriptor to modify
 * @param vlan          The VLAN field value
 * @param flags         The flags field value
 */
__intrinsic void nfd_out_dummy_vlan(__gpr struct nfd_out_input *desc,
                                    unsigned int vlan, unsigned int flags);


/**
 * Add descriptor(s) to the PCI.OUT ring.
 * @param pcie_isl      PCIe island
 * @param queue         Queue to send the packets to
 * @param desc_out      Write transfer registers to hold the descriptors
 * @param desc          RX descriptor for the packet
 * @param sync          Type of synchronisation to use
 * @param sig           Signal on send completion
 *
 * */
__intrinsic void __nfd_out_send(unsigned int pcie_isl, unsigned int queue,
                                __xwrite struct nfd_out_input *desc_out,
                                __gpr struct nfd_out_input *desc,
                                sync_t sync, SIGNAL *sig);


/**
 * Enqueue descriptor(s) to PCI.OUT ring
 * @param pcie_isl      PCIe island
 * @param queue         Queue to send the packets to
 * @param desc          RX packet descriptor for the packet
 *
 */
__intrinsic void nfd_out_send(unsigned int pcie_isl, unsigned int queue,
                              __gpr struct nfd_out_input *desc);

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_PCI_OUT_H_ */
