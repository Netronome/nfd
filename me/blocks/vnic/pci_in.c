/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in.c
 * @brief         Interface to PCI.IN
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>
#include <pkt/pkt.h>
#include <std/reg_utils.h>
#include <std/cntrs.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_in.h>
#include <vnic/shared/nfd.h>
#include <vnic/utils/qc.h>

#ifdef NFD_PCIE0_EMEM
    PKTS_CNTRS_DECLARE(nfd_in_cntrs0, NFD_IN_MAX_QUEUES, __imem_n(0));
#endif

#ifdef NFD_PCIE1_EMEM
    PKTS_CNTRS_DECLARE(nfd_in_cntrs1, NFD_IN_MAX_QUEUES, __imem_n(0));
#endif

#ifdef NFD_PCIE2_EMEM
    PKTS_CNTRS_DECLARE(nfd_in_cntrs2, NFD_IN_MAX_QUEUES, __imem_n(1));
#endif

#ifdef NFD_PCIE3_EMEM
    PKTS_CNTRS_DECLARE(nfd_in_cntrs3, NFD_IN_MAX_QUEUES, __imem_n(1));
#endif

#ifdef NFD_IN_WQ_SHARED

#define NFD_IN_RING_LINK(_isl)                                          \
do {                                                                    \
    nfd_in_ring_info[_isl].addr_hi =                                    \
        ((unsigned long long) NFD_EMEM_SHARED(NFD_IN_WQ_SHARED) >> 32); \
    nfd_in_ring_info[_isl].sp0 = 0;                                     \
    nfd_in_ring_info[_isl].rnum = NFD_RING_LINK(0, nfd_in, 0);          \
} while(0)

#else /* !NFD_IN_WQ_SHARED */

#define NFD_IN_RING_LINK(_isl)                                          \
do {                                                                    \
    nfd_in_ring_info[_isl].addr_hi =                                    \
        ((unsigned long long) NFD_EMEM_LINK(_isl) >> 32);               \
    nfd_in_ring_info[_isl].sp0 = 0;                                     \
    nfd_in_ring_info[_isl].rnum = NFD_RING_LINK(_isl, nfd_in, 0);       \
} while(0)

#endif /* NFD_IN_WQ_SHARED */

__intrinsic uint64_t
swapw64(uint64_t val)
{
    uint32_t tmp;
    tmp = val >> 32;
    return (val << 32) + tmp;
}

__shared __lmem struct nfd_ring_info nfd_in_ring_info[NFD_MAX_ISL];
__shared __lmem struct pkt_cntr_addr nfd_in_cntrs_base[NFD_MAX_ISL];

/* XXX point unused islands at a small "stray" ring? */
__intrinsic void
nfd_in_recv_init()
{
#ifdef NFD_PCIE0_EMEM
    NFD_IN_RING_LINK(0);
    nfd_in_cntrs_base[0] = pkt_cntr_get_addr(nfd_in_cntrs0);
#endif

#ifdef NFD_PCIE1_EMEM
    NFD_IN_RING_LINK(1);
    nfd_in_cntrs_base[1] = pkt_cntr_get_addr(nfd_in_cntrs1);
#endif

#ifdef NFD_PCIE2_EMEM
    NFD_IN_RING_LINK(2);
    nfd_in_cntrs_base[2] = pkt_cntr_get_addr(nfd_in_cntrs2);
#endif

#ifdef NFD_PCIE3_EMEM
    NFD_IN_RING_LINK(3);
    nfd_in_cntrs_base[3] = pkt_cntr_get_addr(nfd_in_cntrs3);
#endif
}


__intrinsic void
__nfd_in_recv(unsigned int pcie_isl, unsigned int workq,
              __xread struct nfd_in_pkt_desc *nfd_in_meta,
              sync_t sync, SIGNAL *sig)
{
    mem_ring_addr_t raddr;
    unsigned int rnum;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);
    try_ctassert(pcie_isl < NFD_MAX_ISL);

    raddr = nfd_in_ring_info[pcie_isl].addr_hi << 24;
    rnum = nfd_in_ring_info[pcie_isl].rnum;

    rnum |= workq;

    __mem_workq_add_thread(rnum, raddr, nfd_in_meta, sizeof(*nfd_in_meta),
                           sizeof(*nfd_in_meta), sync, sig);
}

__intrinsic void
nfd_in_recv(unsigned int pcie_isl, unsigned int workq,
            __xread struct nfd_in_pkt_desc *nfd_in_meta)
{
    SIGNAL sig;

    __nfd_in_recv(pcie_isl, workq, nfd_in_meta, ctx_swap, &sig);
}

__intrinsic void
__nfd_in_cnt_pkt(unsigned int pcie_isl, unsigned int bmsk_queue,
                 unsigned int byte_count, sync_t sync, SIGNAL *sig)
{
    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    pkt_cntr_add(nfd_in_cntrs_base[pcie_isl], bmsk_queue, 0, byte_count,
                 sync, sig);
}

__intrinsic void
__nfd_in_push_pkt_cnt(unsigned int pcie_isl, unsigned int bmsk_queue,
                      sync_t sync, SIGNAL *sig)
{
    unsigned int pkt_count;
    unsigned long long byte_count;
    __xwrite unsigned long long xfer_update[2];

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    pkt_cntr_read_and_clr(nfd_in_cntrs_base[pcie_isl], bmsk_queue, 0,
                          &pkt_count, &byte_count);

    if (pkt_count != 0) {
        xfer_update[0] = swapw64(pkt_count);
        xfer_update[1] = swapw64(byte_count);
        __mem_add64(xfer_update, (NFD_CFG_BAR_ISL(0/*PCIE_ISL*/, 1) +
                    NFP_NET_CFG_RXR_STATS(bmsk_queue)),
                    sizeof xfer_update, sizeof xfer_update, sync, sig);
    }
}

__intrinsic void
nfd_in_fill_meta(void *pkt_info,
                 __xread struct nfd_in_pkt_desc *nfd_in_meta)
{
    ctassert(__is_in_reg_or_lmem(pkt_info));

    /* XXX What is typically done with these values
     * when ejecting a packet from CTM? */
    ((struct nbi_meta_pkt_info *) pkt_info)->isl = 0;   /* Signal MU only */
    ((struct nbi_meta_pkt_info *) pkt_info)->pnum = 0;  /* Signal MU only */
    ((struct nbi_meta_pkt_info *) pkt_info)->split = 0; /* Signal MU only */

    ((struct nbi_meta_pkt_info *) pkt_info)->resv0 = 0;

    ((struct nbi_meta_pkt_info *) pkt_info)->bls = NFD_IN_BLM_BLS;
    ((struct nbi_meta_pkt_info *) pkt_info)->muptr = nfd_in_meta->buf_addr;

    ((struct nbi_meta_pkt_info *) pkt_info)->len = (nfd_in_meta->data_len -
                                                    nfd_in_meta->offset);
}


__intrinsic void
nfd_in_map_queue(unsigned int *vnic, unsigned int *queue,
                 unsigned int nfd_queue)
{
    NFD_EXTRACT_QID(*vnic, *queue, nfd_queue);
}


__intrinsic unsigned int
nfd_in_pkt_len(__xread struct nfd_in_pkt_desc *nfd_in_meta)
{
    return nfd_in_meta->data_len - nfd_in_meta->offset;
}


__intrinsic unsigned int
nfd_in_get_seqn(__xread struct nfd_in_pkt_desc *nfd_in_meta)
{
#ifdef NFD_IN_ADD_SEQN
    /* XXX the sequence number is stored in the reserved field currently */
    return nfd_in_meta->reserved;
#else
    cterror("nfd_in_get_seqn called without NFD_IN_ADD_SEQN defined");
    return 0; /* Avoid missing return warning */
#endif
}
