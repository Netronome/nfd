/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out.h
 * @brief         Interface to PCI.OUT
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_H_
#define _BLOCKS__VNIC_PCI_OUT_H_


/* XXX rename */
#define RX_PCI_OUT_RING_SZ  (4 * 16 * 1024 *1024)

/*
 * RX descriptor format
 *
 * "data_len" and "queue" are used by PCI.OUT when DMA'ing packet data.
 * "queue" is a "bitmask numbered" value, seen as a reserved field by the host.
 */
struct nfd_pci_out_rx_desc {
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
struct nfd_pci_out_cpp_desc {
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
struct nfd_pci_out_input {
    struct nfd_pci_out_cpp_desc cpp;
    struct nfd_pci_out_rx_desc rxd;
};


/**
 * Map a vnic, queue number pair to a bitmask queue
 * @param vnic      vNIC as seen by the host
 * @param queue     queue number within the vNIC
 *
 * This method returns a "bitmask" queue number (the numbering system used
 * internally in PCI.OUT and PCI.IN) from a vNIC, queue number pair.
 */
extern unsigned int pci_out_map_queue(unsigned int vnic, unsigned int queue);


/**
 * Request credits from specified bitmask queue
 * @param bmsk_queue    Queue, bitmask numbered
 * @param num           Number of credits to request
 *
 * This method returns the actual number of credits allocated.
 */
extern unsigned int pci_out_get_credit(unsigned int bmsk_queue,
                                       unsigned int num);


/**
 * Populate the address fields of the CPP descriptor for a packet
 * @param desc      PCI.OUT descriptor to fill
 * @param isl       CTM island for the packet
 * @param pktnum    CTM packet number
 * @param mu_addr   MU address for the packet
 * @param nbi       NBI that received the packet
 * @param bls       Buffer pool for the MU address
 *
 * If both NBIs share BLM pools, set "nbi" to zero.
 */
__intrinsic void pci_out_fill_addr(__gpr struct nfd_pci_out_input *desc,
                                   unsigned int isl, unsigned int pktnum,
                                   unsigned int mu_addr, unsigned int nbi,
                                   unsigned int bls);


/**
 * Populate the address fields of the CPP descriptor for a packet
 * @param desc      PCI.OUT descriptor to fill
 * @param mu_addr   MU address for the packet
 * @param nbi       NBI that received the packet
 * @param bls       Buffer pool for the MU address
 *
 * If both NBIs share BLM pools, set "nbi" to zero.
 */
__intrinsic void pci_out_fill_addr_mu_only(__gpr struct nfd_pci_out_input *desc,
                                           unsigned int mu_addr,
                                           unsigned int nbi, unsigned int bls);


/**
 * Compute start offset and data_len for the packet
 * @param desc          PCI.OUT descriptor to fill
 * @param pkt_start     Start address of packet data
 * @param pkt_len       Length of the packet
 * @param meta_len      Amount of prepended meta data
 */
__intrinsic void pci_out_fill_size(__gpr struct nfd_pci_out_input *desc,
                                   unsigned int pkt_start, unsigned int pkt_len,
                                   unsigned int meta_len);


__intrinsic void pci_out_dummy_vlan(__gpr struct nfd_pci_out_input *desc,
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
__intrinsic void __pci_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
                                __xrw struct nfd_pci_out_input desc_out[2],
                                __gpr struct nfd_pci_out_input *desc,
                                sync_t sync, SIGNAL_PAIR *sigpair);


/**
 * Enqueue descriptor(s) to PCI.OUT ring
 * @param pcie_isl      PCIe island to send the descriptors to
 * @param bmsk_queue    Queue to send the packets (bitmask numbered)
 * @param desc_out      Write transfer registers to hold the descriptors
 *
 * See "__pci_out_send" above.
 */
__intrinsic int pci_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
                             __gpr struct nfd_pci_out_input *desc);

#endif /* !_BLOCKS__VNIC_PCI_OUT_H_ */
