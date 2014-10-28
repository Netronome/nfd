/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out.h
 * @brief         Interface to PCI.OUT
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_H_
#define _BLOCKS__VNIC_PCI_OUT_H_

#include <pkt/pkt.h>


/* XXX rename */
/* #define NFD_OUT_RING_SZ  (4 * 16 * 1024 *1024) */

/* Only support up to 256 credits and 64 queues in the PCI.OUT input ring
 * so that the ring fits easily in the direct access memory */
#define NFD_OUT_RING_SZ  (16 * 64 * 256)

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
            unsigned int offset:7;      /* Offset where packet starts */
            unsigned int queue:8;       /* Queue, bitmask numbered */
            unsigned int data_len:16;   /* Length of frame + meta data */

            unsigned int vlan:16;       /* VLAN if stripped */
            unsigned int spare:12;
            unsigned int flags:4;       /* RX flags */
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
 * "sop" must be set for the first descriptor for a packet, and "eop" for the
 * final descriptor for a packet.  If "sop" is set, the descriptor will transfer
 * the first 4kB of the packet data (possibly as two DMAs).  If "sop" is not
 * set, the descriptor will transfer the balance of the packet data, also
 * possibly requiring two DMAs.
 */
struct nfd_out_cpp_desc {
    union {
        struct {
            unsigned int isl:6;         /* CTM island, zero for MU only pkts */
            unsigned int down:1;        /* Queue down, must be zero */
            unsigned int pktnum:9;      /* CTM packet number */
            unsigned int bls:2;         /* NBI buffer list */
            unsigned int sop:1;         /* Set for start of packet */
            unsigned int eop:1;         /* Set for end of packet */
            unsigned int offset:12;     /* Offset where data starts in NFP */

            unsigned int split:2;       /* Split length allocated to the pkt */
            unsigned int nbi:1;         /* NBI that received the pkt */
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
extern unsigned int nfd_out_map_queue(unsigned int vnic, unsigned int queue);


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

extern unsigned int nfd_out_get_credit(unsigned int pcie_isl,
                                       unsigned int bmsk_queue,
                                       unsigned int num);


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
                                __xrw struct nfd_out_input desc_out[2],
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
__intrinsic int nfd_out_send_test(__xrw struct nfd_out_input desc_out[2]);


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

#endif /* !_BLOCKS__VNIC_PCI_OUT_H_ */
