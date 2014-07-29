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
 */
struct nfd_pci_out_rx_desc {
    union {
        struct {
            unsigned int dd:1;
            unsigned int offset:7;
            unsigned int queue:8;
            unsigned int data_len:16;

            unsigned int vlan:16;
            unsigned int spare:12;
            unsigned int flags:4;
        };
        unsigned int __raw[2];
    };
};


/*
 * CPP address descriptor format
 */
struct nfd_pci_out_cpp_desc {
    union {
        struct {
            unsigned int isl:6;
            unsigned int down:1;
            unsigned int pktnum:9;
            unsigned int sop:1;
            unsigned int eop:1;
            unsigned int offset:14;

            unsigned int split:2;
            unsigned int sp1:1;
            unsigned int mu_addr:29;
        };
        unsigned int __raw[2];
    };
};


struct nfd_pci_out_input {
    struct nfd_pci_out_cpp_desc cpp;
    struct nfd_pci_out_rx_desc rxd;
};


extern unsigned int pci_out_map_queue(unsigned int vnic, unsigned int queue);


extern unsigned int pci_out_get_credit(unsigned int bmsk_queue,
                                       unsigned int num);


__intrinsic void pci_out_fill_addr(__gpr struct nfd_pci_out_input *desc,
                                   unsigned int isl, unsigned int pktnum,
                                   unsigned int mu_addr);


__intrinsic void pci_out_fill_addr_mu_only(__gpr struct nfd_pci_out_input *desc,
                                           unsigned int mu_addr);


__intrinsic void pci_out_fill_size(__gpr struct nfd_pci_out_input *desc,
                                   unsigned int pkt_start, unsigned int pkt_len,
                                   unsigned int meta_len);


__intrinsic void pci_out_dummy_vlan(__gpr struct nfd_pci_out_input *desc,
                                    unsigned int vlan, unsigned int flags);


__intrinsic void __pci_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
                                __xrw struct nfd_pci_out_input desc_out[2],
                                __gpr struct nfd_pci_out_input *desc,
                                sync_t sync, SIGNAL_PAIR *sigpair);


__intrinsic int pci_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
                             __gpr struct nfd_pci_out_input *desc);

#endif /* !_BLOCKS__VNIC_PCI_OUT_H_ */
