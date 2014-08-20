/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out.c
 * @brief         Interface to PCI.OUT
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>
#include <std/reg_utils.h>

#include <vnic/pci_out.h>
#include <vnic/shared/nfd_shared.h>
#include <vnic/shared/qc.h>


NFD_RING_DECLARE(0, pci_out, RX_PCI_OUT_RING_SZ);
NFD_RING_DECLARE(1, pci_out, RX_PCI_OUT_RING_SZ);
NFD_RING_DECLARE(2, pci_out, RX_PCI_OUT_RING_SZ);
NFD_RING_DECLARE(3, pci_out, RX_PCI_OUT_RING_SZ);


unsigned int
pci_out_map_queue(unsigned int vnic, unsigned int queue)
{
    unsigned int natural_queue;

    natural_queue = vnic * MAX_VNIC_QUEUES | queue;
    return map_natural_to_bitmask(natural_queue);
}


/*
 * XXX this code assumes that credits are forced to start at CTM addr 0.
 */
__intrinsic void
__pci_out_get_credit(unsigned int pcie_isl, unsigned int bmsk_queue,
                     unsigned int num, __xrw unsigned int *data,
                     sync_t sync, SIGNAL_PAIR *sigpair)
{
    unsigned int addr_hi;
    unsigned int addr_lo;

    ctassert(__is_ct_const(sync));
    ctassert(__is_read_write_reg(data));

    addr_hi = (0x84 | pcie_isl) << 24;
    addr_lo = bmsk_queue * sizeof(unsigned int);

    *data = num;

    if (sync == sig_done) {
        __asm mem[test_subsat, *data, addr_hi, <<8, addr_lo, 1], \
            sig_done[*sigpair];
    } else {
        __asm mem[test_subsat, *data, addr_hi, <<8, addr_lo, 1], \
            sig_done[*sigpair];
        wait_for_all(sigpair);
    }
}


unsigned int
pci_out_get_credit(unsigned int pcie_isl, unsigned int bmsk_queue,
                   unsigned int num)
{
    __xrw unsigned int data;
    SIGNAL_PAIR sig;
    unsigned int ret;

    __pci_out_get_credit(pcie_isl, bmsk_queue, num, &data, ctx_swap, &sig);
    ret = data;

    return ret;
}


/* unsigned int */
/* pci_out_get_credit(unsigned int vnic, unsigned int queue, unsigned int num) */
/* { */
/*     unsigned int bmsk_queue; */

/*     bmsk_queue = pci_out_map_queue(vnic, queue); */
/*     /\* dummy code *\/ */
/*     return num; */
/* } */



__intrinsic void
pci_out_fill_addr(__gpr struct nfd_pci_out_input *desc,
                  unsigned int isl, unsigned int pktnum, unsigned int mu_addr,
                  unsigned int nbi, unsigned int bls)
{
    desc->cpp.isl = isl;
    desc->cpp.pktnum = pktnum;
    desc->cpp.mu_addr = mu_addr;
    desc->cpp.nbi = nbi;
    desc->cpp.bls = bls;
}


__intrinsic void
pci_out_fill_addr_mu_only(__gpr struct nfd_pci_out_input *desc,
                          unsigned int mu_addr, unsigned int nbi,
                          unsigned int bls)
{
    desc->cpp.isl = 0;
    desc->cpp.pktnum = 0;
    desc->cpp.mu_addr = mu_addr;
    desc->cpp.nbi = nbi;
    desc->cpp.bls = bls;
}


__intrinsic void
pci_out_fill_size(__gpr struct nfd_pci_out_input *desc, unsigned int pkt_start,
                  unsigned int pkt_len, unsigned int meta_len)
{
    desc->rxd.data_len = pkt_len + meta_len;
    desc->rxd.offset = meta_len;
    desc->cpp.offset = pkt_start - meta_len;
}

__intrinsic void
pci_out_dummy_vlan(__gpr struct nfd_pci_out_input *desc, unsigned int vlan,
                   unsigned int flags)
{
    desc->rxd.vlan  = vlan;
    desc->rxd.flags = flags;
    desc->rxd.spare = 0;
}


/* XXX selecting between the PCIe islands becomes ugly if not CT const.
 * This can be optimised for specific use cases, depending on what assumptions
 * the calling applications is able to make. */
__intrinsic void
__pci_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
               __xrw struct nfd_pci_out_input desc_out[2],
               __gpr struct nfd_pci_out_input *desc,
               sync_t sync, SIGNAL_PAIR *sigpair)
{
    unsigned int desc_sz = sizeof(struct nfd_pci_out_input);
    mem_ring_addr_t raddr;
    unsigned int rnum;

    ctassert(sync == sig_done);

    switch (pcie_isl) {
    case 0:
        raddr = (unsigned long long) NFD_EMEM(0) >> 8;
        rnum = NFD_RING_ALLOC(0, pci_out, 1);
        break;
    case 1:
        raddr = (unsigned long long) NFD_EMEM(1) >> 8;
        rnum = NFD_RING_ALLOC(1, pci_out, 1);
        break;
    case 2:
        raddr = (unsigned long long) NFD_EMEM(2) >> 8;
        rnum = NFD_RING_ALLOC(2, pci_out, 1);
        break;
    case 3:
        raddr = (unsigned long long) NFD_EMEM(3) >> 8;
        rnum = NFD_RING_ALLOC(3, pci_out, 1);
        break;
    default:
        halt();
    }

    /* Complete the basic descriptor */
    desc->rxd.dd = 1;
    desc->rxd.queue = bmsk_queue;
    desc->cpp.split = 0; /* XXX deal with getting split length */
    desc->cpp.down = 0;
    desc->cpp.sop = 1;

    if (desc->rxd.data_len < 4096) {
        desc->cpp.eop = 1;
        desc_out[0] = *desc;

        __mem_ring_put(rnum, raddr, desc_out, desc_sz, desc_sz,
                       sig_done, sigpair);
    } else {
        desc->cpp.eop = 0;
        desc_out[0] = *desc;

        desc->cpp.sop = 0;
        desc->cpp.eop = 1;
        desc_out[1] = *desc;

        __mem_ring_put(rnum, raddr, desc_out, 2 * desc_sz, 2 * desc_sz,
                       sig_done, sigpair);
    }
}


__intrinsic int
pci_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
             __gpr struct nfd_pci_out_input *desc)
{
    __xrw struct nfd_pci_out_input data[2];
    SIGNAL_PAIR sigpair;
    int result;

    __pci_out_send(pcie_isl, bmsk_queue, data, desc, sig_done, &sigpair);
    wait_for_all(&sigpair);

    result = data[0].cpp.__raw[0];
    return (result & (1 << 31)) ? (result << 2) : -1;
}
