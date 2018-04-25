/*
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
    int vid;
    int vqn;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    pkt_cntr_read_and_clr(nfd_in_cntrs_base[pcie_isl], bmsk_queue, 0,
                          &pkt_count, &byte_count);

    if (pkt_count != 0) {
        xfer_update[0] = swapw64(pkt_count);
        xfer_update[1] = swapw64(byte_count);
        NFD_QID2VID(vid, vqn, bmsk_queue);
        __mem_add64(xfer_update, (nfd_cfg_bar_base(pcie_isl, vid) +
                                  NFP_NET_CFG_TXR_STATS(vqn)),
                    sizeof xfer_update, sizeof xfer_update, sync, sig);
    }
}


__intrinsic int
nfd_in_metadata_pop(void *meta_val,
                    unsigned int *meta_len,
                    unsigned int *meta_info,
                    __mem40 void *pkt_buf_ptr)
{
    __mem40 char *meta_ptr;
    __xread unsigned int meta_data[(NFD_IN_MAX_META_ITEM_LEN + 4) / 4];
    SIGNAL sig_meta;
    int ret = 0;

    ctassert(__is_in_reg_or_lmem(meta_val));
    ctassert(__is_in_reg_or_lmem(meta_len));
    ctassert(__is_in_reg_or_lmem(meta_info));
    ctassert(__is_in_reg_or_lmem(pkt_buf_ptr));

    if (*meta_len > NFD_IN_MAX_META_LEN) {
        ret = -1;
        goto err;
    }

    if (*meta_len == 0) {
        ret = 0;
        goto done;
    }

    meta_ptr = (__mem40 char *)((unsigned long long)pkt_buf_ptr +
                                NFD_IN_DATA_OFFSET -
                                (unsigned long long)(*meta_len));

    /* If this is the first word of metadata being "popped", read
     * meta_info word too. */
    if (*meta_info == 0) {
        __mem_read32(meta_data, meta_ptr, 4 + NFD_IN_MAX_META_ITEM_LEN,
                     4 + NFD_IN_MAX_META_ITEM_LEN, ctx_swap, &sig_meta);
        *meta_info = meta_data[0];
        *meta_len -= 4;
    } else {
        __mem_read32(&meta_data[1], meta_ptr, NFD_IN_MAX_META_ITEM_LEN,
                     NFD_IN_MAX_META_ITEM_LEN, ctx_swap, &sig_meta);
    }

    /* "Pop" NFD_IN_MAX_META_ITEM_LEN bytes of metadata */
    reg_cp(meta_val, &meta_data[1], NFD_IN_MAX_META_ITEM_LEN);

    /* "Pop" type from meta_info */
    ret = *meta_info & NFP_NET_META_FIELD_MASK;
    *meta_info >>= NFP_NET_META_FIELD_SIZE;

done:
err:
    return ret;
}


__intrinsic int
nfd_in_metadata_pop_cache(unsigned int *meta_val,
                          unsigned int *meta_len,
                          unsigned int *meta_info,
                          __xread unsigned int *meta_cache,
                          unsigned int *meta_cache_ptr,
                          const unsigned int meta_cache_len,
                          __mem40 void *pkt_buf_ptr)
{
    __mem40 char *meta_ptr;
    int ret = 0;
    unsigned int i;
    SIGNAL sig_meta;

    ctassert(__is_in_reg_or_lmem(meta_val));
    ctassert(__is_in_reg_or_lmem(meta_len));
    ctassert(__is_in_reg_or_lmem(meta_info));
    ctassert(__is_ct_const(meta_cache_len));
    ctassert(__is_in_reg_or_lmem(pkt_buf_ptr));
    ctassert(meta_cache_len <= NFD_IN_MAX_META_LEN);
    ctassert(meta_cache_len % 4 == 0);

    if (*meta_len > meta_cache_len) {
        ret = -1;
        goto err;
    }

    if (*meta_len == 0) {
        ret = 0;
        goto done;
    }

    /* Cache all metadata in transfer registers */
    if (*meta_info == 0) {
        meta_ptr = (__mem40 char *)((unsigned long long)pkt_buf_ptr +
                                    NFD_IN_DATA_OFFSET -
                                    (unsigned long long)(*meta_len));
        __mem_read32(meta_cache, meta_ptr, *meta_len, meta_cache_len,
                     ctx_swap, &sig_meta);

        /* Set up T_INDEX to access cached metadata */
        *meta_cache_ptr =
            (__ctx() << 7) | (__xfer_reg_number(&meta_cache[1]) << 2);
        *meta_len -= 4;
        *meta_info = meta_cache[0];
    }

    local_csr_write(local_csr_t_index, *meta_cache_ptr);
    __asm nop

    /* "Pop" type from meta_info to return */
    ret = *meta_info & NFP_NET_META_FIELD_MASK;
    *meta_info >>= NFP_NET_META_FIELD_SIZE;

    /* "Pop" NFD_IN_MAX_META_ITEM_LEN bytes of metadata */
    #if NFD_IN_MAX_META_ITEM_LEN == 4
    __asm alu[*meta_val, --, B, *$index++]
    #elif NFD_IN_MAX_META_ITEM_LEN > 4
    __asm alu[meta_val[0], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 8
        __asm alu[meta_val[1], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 12
        __asm alu[meta_val[2], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 16
        __asm alu[meta_val[3], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 20
        __asm alu[meta_val[4], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 24
        __asm alu[meta_val[5], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 28
        __asm alu[meta_val[6], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 32
        __asm alu[meta_val[7], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 36
        __asm alu[meta_val[8], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 40
        __asm alu[meta_val[9], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 44
        __asm alu[meta_val[10], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 48
        __asm alu[meta_val[11], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 52
        __asm alu[meta_val[12], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN >= 56
        __asm alu[meta_val[13], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN == 60
        __asm alu[meta_val[14], --, B, *$index++]
    #endif
    #if NFD_IN_MAX_META_ITEM_LEN > 60
        #error "Maximum supported value of NFD_IN_MAX_META_ITEM_LEN is 60"
    #endif
    __implicit_read(meta_cache);

done:
err:
    return ret;
}


__intrinsic void
nfd_in_fill_meta(void *pkt_info,
                 __xread struct nfd_in_pkt_desc *nfd_in_meta)
{
    unsigned int data_len;
    unsigned int bls;

    ctassert(__is_in_reg_or_lmem(pkt_info));

    data_len = nfd_in_meta->data_len;

    /* XXX What is typically done with these values
     * when ejecting a packet from CTM? */
    ((struct nbi_meta_pkt_info *) pkt_info)->isl = 0;   /* Signal MU only */
    ((struct nbi_meta_pkt_info *) pkt_info)->pnum = 0;  /* Signal MU only */
    ((struct nbi_meta_pkt_info *) pkt_info)->split = 0; /* Signal MU only */

    ((struct nbi_meta_pkt_info *) pkt_info)->resv0 = 0;

    /* Set the BLS, suppressing length tests if the same BLQ is used
     * for all packets.
     * XXX The test applied in this API must match the test used internally
     * in issue_dma.c. */
#if (NFD_IN_BLM_JUMBO_BLS == NFD_IN_BLM_REG_BLS)
    ((struct nbi_meta_pkt_info *) pkt_info)->bls = NFD_IN_BLM_REG_BLS;
#else
    bls = NFD_IN_BLM_JUMBO_BLS;
    if (!nfd_in_meta->jumbo) {
        bls = NFD_IN_BLM_REG_BLS;
    }
    ((struct nbi_meta_pkt_info *) pkt_info)->bls = bls;
#endif

    ((struct nbi_meta_pkt_info *) pkt_info)->muptr = nfd_in_meta->buf_addr;

    ((struct nbi_meta_pkt_info *) pkt_info)->len = (data_len -
                                                    nfd_in_meta->offset);
}


__intrinsic void
nfd_in_map_queue(unsigned int *type, unsigned int *vnic, unsigned int *queue,
                 unsigned int nfd_queue)
{
    NFD_EXTRACT_QID(*type, *vnic, *queue, nfd_queue);
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
    return nfd_in_meta->seq_num;
#else
    cterror("nfd_in_get_seqn called without NFD_IN_ADD_SEQN defined");
    return 0; /* Avoid missing return warning */
#endif
}
