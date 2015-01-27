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
#include <pkt/pkt.h>
#include <std/reg_utils.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_out.h>
#include <vnic/shared/nfd.h>
#include <vnic/utils/qc.h>



#define NFD_OUT_RING_LINK(_isl)                                         \
do {                                                                    \
    nfd_out_ring_info[_isl].addr_hi =                                   \
        ((unsigned long long) NFD_EMEM_LINK(_isl) >> 32);               \
    nfd_out_ring_info[_isl].sp0 = 0;                                    \
    nfd_out_ring_info[_isl].rnum = NFD_RING_LINK(_isl, nfd_out, 0);     \
} while(0)

__shared __lmem struct nfd_ring_info nfd_out_ring_info[NFD_MAX_ISL];


/* XXX point unused islands at a small "stray" ring? */
__intrinsic void
nfd_out_send_init()
{
#ifdef NFD_PCIE0_EMEM
    NFD_OUT_RING_LINK(0);
#endif

#ifdef NFD_PCIE1_EMEM
    NFD_OUT_RING_LINK(1);
#endif

#ifdef NFD_PCIE2_EMEM
    NFD_OUT_RING_LINK(2);
#endif

#ifdef NFD_PCIE3_EMEM
    NFD_OUT_RING_LINK(3);
#endif
}


__intrinsic unsigned int
nfd_out_map_queue(unsigned int vnic, unsigned int queue)
{
    return NFD_BUILD_QID(vnic, queue);
}


/*
 * XXX this code assumes that credits are forced to start at CTM addr 0.
 */
__intrinsic void
__nfd_out_get_credit(unsigned int pcie_isl, unsigned int bmsk_queue,
                     unsigned int num, __xrw unsigned int *data,
                     sync_t sync, SIGNAL_PAIR *sigpair)
{
    unsigned int addr_hi;
    unsigned int addr_lo;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);
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


__intrinsic unsigned int
nfd_out_get_credit(unsigned int pcie_isl, unsigned int bmsk_queue,
                   unsigned int num)
{
    __xrw unsigned int data;
    SIGNAL_PAIR sig;
    unsigned int ret;

    __nfd_out_get_credit(pcie_isl, bmsk_queue, num, &data, ctx_swap, &sig);
    ret = data;

    return ret;
}


__intrinsic void
nfd_out_fill_desc(__gpr struct nfd_out_input *desc, void *pkt_info,
                  unsigned int nbi, unsigned int ctm_split,
                  unsigned int pkt_start, unsigned int meta_len)
{
    ctassert(__is_in_reg_or_lmem(pkt_info) || __is_read_reg(pkt_info));

    /* Address details */
    desc->cpp.isl = ((struct nbi_meta_pkt_info *) pkt_info)->isl;
    desc->cpp.pktnum = ((struct nbi_meta_pkt_info *) pkt_info)->pnum;
    desc->cpp.mu_addr = ((struct nbi_meta_pkt_info *) pkt_info)->muptr;
    desc->cpp.split = ctm_split;
    desc->cpp.nbi = nbi;
    desc->cpp.bls = ((struct nbi_meta_pkt_info *) pkt_info)->bls;

    /* Length and offset details */
    desc->rxd.data_len = (((struct nbi_meta_pkt_info *) pkt_info)->len +
                          meta_len);
    desc->rxd.meta_len = meta_len;
    desc->cpp.offset = pkt_start - meta_len;
}


__intrinsic void
nfd_out_dummy_vlan(__gpr struct nfd_out_input *desc, unsigned int vlan,
                   unsigned int flags)
{
    desc->rxd.vlan  = vlan;
    desc->rxd.flags = flags;
}


/* XXX selecting between the PCIe islands becomes ugly if not CT const.
 * This can be optimised for specific use cases, depending on what assumptions
 * the calling applications is able to make. */
__intrinsic void
__nfd_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
               __xrw struct nfd_out_input desc_out[2],
               __gpr struct nfd_out_input *desc,
               sync_t sync, SIGNAL_PAIR *sigpair)
{
    unsigned int desc_sz = sizeof(struct nfd_out_input);
    mem_ring_addr_t raddr;
    unsigned int rnum;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done);
    try_ctassert(pcie_isl < NFD_MAX_ISL);

    raddr = nfd_out_ring_info[pcie_isl].addr_hi << 24;
    rnum = nfd_out_ring_info[pcie_isl].rnum;

    /* Complete the basic descriptor */
    desc->rxd.dd = 1;
    desc->rxd.queue = bmsk_queue;
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
nfd_out_send_test(__xrw struct nfd_out_input desc_out[2])
{
    int result;

    result = desc_out[0].cpp.__raw[0];
    return (result & (1 << 31)) ? (result << 2) : -1;
}


__intrinsic int
nfd_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
             __gpr struct nfd_out_input *desc)
{
    __xrw struct nfd_out_input data[2];
    SIGNAL_PAIR sigpair;

    __nfd_out_send(pcie_isl, bmsk_queue, data, desc, sig_done, &sigpair);
    wait_for_all(&sigpair);

    return nfd_out_send_test(data);

}
