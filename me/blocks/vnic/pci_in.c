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

#include <vnic/pci_in.h>
#include <vnic/pci_in_cfg.h>
#include <vnic/shared/nfd_shared.h>
#include <vnic/shared/qc.h>


__shared __lmem struct nfd_ring_info nfd_in_ring_info[NFD_MAX_ISL];


__intrinsic void
nfd_in_recv_init()
{
#if _nfp_has_island("pcie0")
    NFD_RING_INIT(0, nfd_in, NFD_NUM_WQS);
#endif

#if _nfp_has_island("pcie1")
    NFD_RING_INIT(1, nfd_in, NFD_NUM_WQS);
#endif

#if _nfp_has_island("pcie2")
    NFD_RING_INIT(2, nfd_in, NFD_NUM_WQS);
#endif

#if _nfp_has_island("pcie3")
    NFD_RING_INIT(3, nfd_in, NFD_NUM_WQS);
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


void
nfd_in_map_queue(unsigned int *vnic, unsigned int *queue,
                 unsigned int nfd_queue)
{
    unsigned int natural_queue;

    natural_queue = map_bitmask_to_natural(nfd_queue);
    *queue = natural_queue & (MAX_VNIC_QUEUES - 1);
    *vnic = natural_queue / MAX_VNIC_QUEUES;
}


__intrinsic unsigned int
nfd_in_pkt_len(__xread struct nfd_in_pkt_desc *nfd_in_meta)
{
    return nfd_in_meta->data_len - nfd_in_meta->offset;
}


