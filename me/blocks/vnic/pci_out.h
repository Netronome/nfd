/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out.h
 * @brief         Interface to PCI.OUT
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_H_
#define _BLOCKS__VNIC_PCI_OUT_H_

#include <nfd_net.h>

#define NFD_OUT_MAX_QUEUES              64

/*
 * RX descriptor format
 *
 * "data_len" and "queue" are used by PCI.OUT when DMA'ing packet data.
 * "queue" is a "bitmask numbered" value, seen as a reserved field by the host.
 */
struct nfd_out_rx_desc {
    union {
        struct {
            unsigned int dd:1;          /* Descriptor done, must be set */
            unsigned int meta_len:7;    /* Length of meta data prepended */
            unsigned int queue:8;       /* Queue, bitmask numbered */
            unsigned int data_len:16;   /* Length of frame + meta data */

            unsigned int vlan:16;       /* VLAN if stripped */
            unsigned int flags:16;      /* RX flags */
        };
        unsigned int __raw[2];
    };
};


/*
 * CPP address descriptor format
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
            unsigned int isl:6;         /* CTM island, zero for MU only pkts */
            unsigned int ctm_only:1;    /* The packet is entirely in CTM */
            unsigned int pktnum:9;      /* CTM packet number */
            unsigned int split:2;       /* Split length allocated to the pkt */
            unsigned int reserved:1;    /* Must be zero from application */
            unsigned int offset:13;     /* Offset where data starts in NFP */

            unsigned int nbi:1;         /* NBI that received the pkt */
            unsigned int bls:2;         /* NBI buffer list */
            unsigned int mu_addr:29;    /* Pkt MU address */
        };
        unsigned int __raw[2];
    };
};

/*
 * PCI.OUT descriptors consist of a CPP descriptor and an RX descriptor
 */
struct nfd_out_input {
    struct nfd_out_cpp_desc cpp;
    struct nfd_out_rx_desc rxd;
};


/*
 * Format of PCI.OUT CTM atomic data
 */
#define NFD_OUT_ATOMICS_SZ          8
#define NFD_OUT_ATOMICS_CREDIT      0
#define NFD_OUT_ATOMICS_SENT        4


#if defined(__NFP_LANG_MICROC)

#include <pkt/pkt.h>

#include "nfd_user_cfg.h"

#include <vnic/shared/nfcc_chipres.h>


#define NFD_OUT_RINGS_DECL_IND2(_isl, _emem)                            \
    _emem##_queues_DECL                                                 \
    ASM(.alloc_resource nfd_out_ring_num##_isl##0 _emem##_queues global 1 1)
#define NFD_OUT_RINGS_DECL_IND1(_isl, _emem)    \
    NFD_OUT_RINGS_DECL_IND2(_isl, _emem)
#define NFD_OUT_RINGS_DECL_IND0(_isl)                       \
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


/**
 * Prepare ME data structures required to send packets to NFD
 *
 * This method should be called from a single context, during initialisation.
 */
__intrinsic void nfd_out_send_init();


/**
 * Map a vnic, queue number pair to a bitmask queue
 * @param vnic      vNIC as seen by the host
 * @param queue     queue number within the vNIC
 *
 * This method returns a "bitmask" queue number (the numbering system used
 * internally in PCI.OUT and PCI.IN) from a vNIC, queue number pair.
 */
__intrinsic unsigned int nfd_out_map_queue(unsigned int vnic,
                                           unsigned int queue);


/**
 * Request credits from specified bitmask queue
 * @param bmsk_queue    Queue, bitmask numbered
 * @param num           Number of credits to request
 *
 * This method returns the number of credits available before the request.
 * It is the user's responsibility to verify that this is non-zero if one
 * credit was requested, and greater than or equal to num if more than one
 * credit was requested.
 */

__intrinsic void __nfd_out_get_credit(unsigned int pcie_isl,
                                      unsigned int bmsk_queue, unsigned int num,
                                      __xrw unsigned int *data,
                                      sync_t sync, SIGNAL_PAIR *sigpair);

__intrinsic unsigned int nfd_out_get_credit(unsigned int pcie_isl,
                                            unsigned int bmsk_queue,
                                            unsigned int num);


/**
 * Packets and Bytes count for PCI.OUT queues.
 * @param pcie_isl      PCIe island to access
 * @param bmsk_queue    Rx queue number
 * @param byte_count    The bytes count to add
 * @param sync          type of synchronization
 * @param sig           signal to report completion
 *
 * This function uses the stats engine pkt and byte counters
 * to log the packet and bytes count per Rx queue.
 * The values are accumulated in the nfd_out_cntrsX memory and needs
 * to be pushed to the CFG BAR using the "__nfd_out_push_pkt_cnt" function.
 */
__intrinsic void __nfd_out_cnt_pkt(unsigned int pcie_isl,
                                   unsigned int bmsk_queue,
                                   unsigned int byte_count,
                                   sync_t sync, SIGNAL *sig);

/**
 * Push Packets and Bytes count for PCI.OUT queue into the CFG BAR.
 * @param pcie_isl      PCIe island to access
 * @param bmsk_queue    Rx queue number
 * @param sync          type of synchronization
 * @param sig           signal to report completion
 *
 * This function updates the per Rx Q packets and bytes counter
 * in the CFG BAR. It reads and clears the packets and bytes
 * count from the relevant nfd_in_cntrsX memory and updates the
 * CFG BAR counters using the read values.
 */
__intrinsic void __nfd_out_push_pkt_cnt(unsigned int pcie_isl,
                                        unsigned int bmsk_queue,
                                        sync_t sync, SIGNAL *sig);

/**
 * Populate the address fields of the CPP descriptor for a packet
 * @param desc          PCI.OUT descriptor to fill
 * @param pkt_info      Up to date nbi_meta_pkt_info struct for the packet
 * @param nbi           NBI that received the packet
 * @param ctm_split     CTM split length for the packet
 * @param pkt_start     Start address of packet data
 * @param meta_len      Amount of prepended meta data
 *
 * If both NBIs share BLM pools, set "nbi" to zero.  For MU only packets,
 * "pkt_info->isl" must be zero.  "ctm_split" must be encoded as for the
 * "NbiDmaBPCfg" register (see DB).
 */
__intrinsic void nfd_out_fill_desc(__gpr struct nfd_out_input *desc,
                                   void *pkt_info,
                                   unsigned int nbi, unsigned int ctm_split,
                                   unsigned int pkt_start,
                                   unsigned int meta_len);

/**
 * Set "ctm_only" correctly based on other fields in "desc"
 *
 * This function will parse "desc" and compute whether the whole packet
 * is held in the CTM buffer (if any).  If so, it will set "ctm_only" to 1.
 * Accessing the necessary data from "desc" may be less efficient than
 * setting "ctm_only" based on other context specific knowledge.  "ctm_only"
 * may be set directly in these cases.
 */
__intrinsic void nfd_out_check_ctm_only(__gpr struct nfd_out_input *desc);


__intrinsic void nfd_out_dummy_vlan(__gpr struct nfd_out_input *desc,
                                    unsigned int vlan, unsigned int flags);


/**
 * Enqueue descriptor(s) to PCI.OUT ring
 * @param pcie_isl      PCIe island to send the descriptors to
 * @param bmsk_queue    Queue to send the packets (bitmask numbered)
 * @param desc_out      Write transfer registers to hold the descriptors
 * @param sync          Type of synchronisation to use
 * @param sigpair       Signal pair for the ring put
 *
 * This method selects the appropriate MU ring to enqueue the packet to.  It
 * also tests whether the packet requires one descriptor or two and manages
 * signalling with "sop" and "eop" appropriately.
 * */
__intrinsic void __nfd_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
                                __xrw struct nfd_out_input *desc_out,
                                __gpr struct nfd_out_input *desc,
                                sync_t sync, SIGNAL_PAIR *sigpair);


/**
 * Test result of "__nfd_out_send"
 * @param desc_out      Write transfer registers that were used in send
 *
 * This helper function tests for error codes from the hardware queue engine
 * that indicate whether a "__nfd_out_send" call failed.  "desc_out" must have
 * been used in the "__nfd_out_send" call of interest, immediately before.
 */
__intrinsic int nfd_out_send_test(__xrw struct nfd_out_input *desc_out);


/**
 * Enqueue descriptor(s) to PCI.OUT ring
 * @param pcie_isl      PCIe island to send the descriptors to
 * @param bmsk_queue    Queue to send the packets (bitmask numbered)
 * @param desc_out      Write transfer registers to hold the descriptors
 *
 * See "__pci_out_send" above.
 */
__intrinsic int nfd_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
                             __gpr struct nfd_out_input *desc);

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_PCI_OUT_H_ */
