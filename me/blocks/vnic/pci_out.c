/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/pci_out.c
 * @brief         Interface to PCI.OUT
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>
#include <pkt/pkt.h>
#include <std/reg_utils.h>
#include <std/cntrs.h>
#include <nfp/mem_bulk.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_out.h>
#include <vnic/shared/nfd.h>
#include <vnic/utils/qc.h>
#include <vnic/shared/nfd_cfg.h>

extern __intrinsic uint64_t swapw64(uint64_t val);

#ifdef NFD_PCIE0_EMEM
    PKTS_CNTRS_DECLARE(nfd_out_cntrs0, NFD_OUT_MAX_QUEUES, __imem_n(0));
#endif

#ifdef NFD_PCIE1_EMEM
    PKTS_CNTRS_DECLARE(nfd_out_cntrs1, NFD_OUT_MAX_QUEUES, __imem_n(0));
#endif

#ifdef NFD_PCIE2_EMEM
    PKTS_CNTRS_DECLARE(nfd_out_cntrs2, NFD_OUT_MAX_QUEUES, __imem_n(1));
#endif

#ifdef NFD_PCIE3_EMEM
    PKTS_CNTRS_DECLARE(nfd_out_cntrs3, NFD_OUT_MAX_QUEUES, __imem_n(1));
#endif


#define NFD_OUT_RING_LINK(_isl)                                         \
do {                                                                    \
    nfd_out_ring_info[_isl].addr_hi =                                   \
        ((unsigned long long) NFD_EMEM_LINK(_isl) >> 32);               \
    nfd_out_ring_info[_isl].sp0 = 0;                                    \
    nfd_out_ring_info[_isl].rnum = NFD_RING_LINK(_isl, nfd_out, 0);     \
} while(0)

__shared __lmem struct nfd_ring_info nfd_out_ring_info[NFD_MAX_ISL];
__shared __lmem struct pkt_cntr_addr nfd_out_cntrs_base[NFD_MAX_ISL];


/* XXX point unused islands at a small "stray" ring? */
__intrinsic void
nfd_out_send_init()
{
#ifdef NFD_PCIE0_EMEM
    NFD_OUT_RING_LINK(0);
    nfd_out_cntrs_base[0] = pkt_cntr_get_addr(nfd_out_cntrs0);
#endif

#ifdef NFD_PCIE1_EMEM
    NFD_OUT_RING_LINK(1);
    nfd_out_cntrs_base[1] = pkt_cntr_get_addr(nfd_out_cntrs1);
#endif

#ifdef NFD_PCIE2_EMEM
    NFD_OUT_RING_LINK(2);
    nfd_out_cntrs_base[2] = pkt_cntr_get_addr(nfd_out_cntrs2);
#endif

#ifdef NFD_PCIE3_EMEM
    NFD_OUT_RING_LINK(3);
    nfd_out_cntrs_base[3] = pkt_cntr_get_addr(nfd_out_cntrs3);
#endif
}


__intrinsic unsigned int
nfd_out_map_queue(unsigned int type, unsigned int vnic, unsigned int queue)
{
    return NFD_BUILD_QID(type, vnic, queue);
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
    addr_lo = bmsk_queue * NFD_OUT_ATOMICS_SZ;

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
__nfd_out_cnt_pkt(unsigned int pcie_isl, unsigned int bmsk_queue,
                  unsigned int byte_count, sync_t sync, SIGNAL *sig)
{
    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    pkt_cntr_add(nfd_out_cntrs_base[pcie_isl], bmsk_queue, 0, byte_count,
                 sync, sig);
}

__intrinsic void
__nfd_out_push_pkt_cnt(unsigned int pcie_isl, unsigned int bmsk_queue,
                       sync_t sync, SIGNAL *sig)
{
    unsigned int pkt_count;
    unsigned long long byte_count;
    __xwrite unsigned long long xfer_update[2];
    int vid;
    int vqn;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    pkt_cntr_read_and_clr(nfd_out_cntrs_base[pcie_isl], bmsk_queue, 0,
                          &pkt_count, &byte_count);

    if (pkt_count != 0) {
        xfer_update[0] = swapw64(pkt_count);
        xfer_update[1] = swapw64(byte_count);
        NFD_QID2VID(vid, vqn, bmsk_queue);
        __mem_add64(xfer_update, (nfd_cfg_bar_base(pcie_isl, vid) +
                                  NFP_NET_CFG_RXR_STATS(vqn)),
                    sizeof xfer_update, sizeof xfer_update, sync, sig);
    }
}

__intrinsic int
__nfd_out_metadata_push(void *meta_len,
                        unsigned int meta_types,
                        void *meta_val,
                        unsigned int meta_type_num,
                        unsigned int meta_val_len,
                        const unsigned int meta_val_len_max,
                        __mem40 void *pkt_start_ptr,
                        sync_t sync, SIGNAL *sig)
{
    unsigned int meta_len_cp;
    __mem40 char *meta_ptr;
    __xwrite unsigned int meta_data[(NFD_OUT_MAX_META_ITEM_LEN + 4) / 4];
    SIGNAL sig_meta;
    int ret = 0;

    ctassert(__is_in_reg_or_lmem((void *)meta_len));
    ctassert(__is_in_reg_or_lmem(meta_val));
    ctassert(__is_ct_const(meta_val_len_max));
    ctassert(__is_in_reg_or_lmem((__mem40 void *)pkt_start_ptr));
    ctassert(meta_val_len_max <= NFD_MAX_META_VAL_LEN);
    ctassert(meta_val_len_max <= NFD_OUT_MAX_META_ITEM_LEN);
    ctassert(meta_val_len_max % 4 == 0);
    ctassert(sync == ctx_swap);

    reg_cp((void *) &meta_len_cp, meta_len, sizeof(meta_len_cp));
    if (meta_len_cp == 0) {
        /* This is the first word of metadata being prepended. */
        meta_data[0] = meta_types;
        meta_len_cp = 4;
    } else if (meta_len_cp <= (NFD_OUT_MAX_META_LEN - meta_val_len)) {
        __xread unsigned int meta_info;

        meta_ptr = (__mem40 char *)((unsigned long long)pkt_start_ptr -
                                     (unsigned long long)meta_len_cp);
        mem_read32(&meta_info, meta_ptr, 4);
        meta_data[0] = ((meta_info <<
                         (NFP_NET_META_FIELD_SIZE * meta_type_num)) |
                        meta_types);
    } else {
        ret = -1;
        goto err;
    }

    reg_cp(&meta_data[1], meta_val, meta_val_len_max);
    meta_len_cp += meta_val_len;
    meta_ptr = (__mem40 char *)((unsigned long long)pkt_start_ptr -
                                (unsigned long long)meta_len_cp);
    __mem_write32(meta_data, meta_ptr, 4 + meta_val_len, 4 + meta_val_len_max,
                  sync, sig);

err:
    reg_cp(meta_len, (void *) &meta_len_cp, sizeof(meta_len_cp));
    return ret;
}

__intrinsic int
nfd_out_metadata_push(void *meta_len,
                      unsigned int meta_types,
                      void *meta_val,
                      unsigned int meta_type_num,
                      const unsigned int meta_val_len,
                      __mem40 void *pkt_start_ptr)
{
    SIGNAL sig_meta;
    return __nfd_out_metadata_push(meta_len, meta_types, meta_val,
                                   meta_type_num, meta_val_len, meta_val_len,
                                   pkt_start_ptr, ctx_swap, &sig_meta);
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
nfd_out_check_ctm_only(__gpr struct nfd_out_input *desc)
{
    unsigned int ctm_bytes;
    unsigned int dma_end;

    /* Check whether this is an MU only packet.
     * MU only packets can't be "ctm_only" */
    if (desc->cpp.isl == 0) {
        desc->cpp.ctm_only = 0;
        return;
    }

    ctm_bytes = 256 << desc->cpp.split;
    dma_end = desc->cpp.offset + desc->rxd.data_len;

    /* Check whether the packet will spill over the end of the CTM buffer  */
    if (ctm_bytes >= dma_end) {
        desc->cpp.ctm_only = 1;
    } else {
        desc->cpp.ctm_only = 0;
    }
}


__intrinsic void
nfd_out_dummy_vlan(__gpr struct nfd_out_input *desc, unsigned int vlan,
                   unsigned int flags)
{
    desc->rxd.vlan  = vlan;
    desc->rxd.flags = flags;
}


__intrinsic void
__nfd_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
               __xwrite struct nfd_out_input *desc_out,
               __gpr struct nfd_out_input *desc,
               sync_t sync, SIGNAL *sig)
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
    desc->cpp.reserved = 0;
    *desc_out = *desc;

    __mem_workq_add_work(rnum, raddr, desc_out, desc_sz, desc_sz,
                         sig_done, sig);
}


__intrinsic void
nfd_out_send(unsigned int pcie_isl, unsigned int bmsk_queue,
             __gpr struct nfd_out_input *desc)
{
    __xwrite struct nfd_out_input data;
    SIGNAL sig;

    __nfd_out_send(pcie_isl, bmsk_queue, &data, desc, sig_done, &sig);
    wait_for_all(&sig);
}
