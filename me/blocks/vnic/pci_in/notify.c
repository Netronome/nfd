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
 * @file          blocks/vnic/pci_in/notify.c
 * @brief         Code to notify host and app that packet was transmitted
 */

#include <assert.h>
#include <nfp.h>
#include <nfp_chipres.h>

#include <nfp/me.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_me.h>

#include <nfp/mem_ring.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_in.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_cfg_internal.c>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/ctm_ring.h>
#include <vnic/utils/ordering.h>
#include <vnic/utils/qc.h>
#include <vnic/utils/qcntl.h>

/* TODO: get NFD_PCIE_ISL_BASE from a common header file */
#define NOTIFY_RING_ISL (PCIE_ISL + 4)

#if !defined(NFD_IN_HAS_ISSUE0) && !defined(NFD_IN_HAS_ISSUE1)
#error "At least one of NFD_IN_HAS_ISSUE0 and NFD_IN_HAS_ISSUE1 must be defined"
#endif


struct _issued_pkt_batch {
    struct nfd_in_issued_desc pkt0;
    struct nfd_in_issued_desc pkt1;
    struct nfd_in_issued_desc pkt2;
    struct nfd_in_issued_desc pkt3;
    struct nfd_in_issued_desc pkt4;
    struct nfd_in_issued_desc pkt5;
    struct nfd_in_issued_desc pkt6;
    struct nfd_in_issued_desc pkt7;
};

struct _pkt_desc_batch {
    struct nfd_in_pkt_desc pkt0;
    struct nfd_in_pkt_desc pkt1;
    struct nfd_in_pkt_desc pkt2;
    struct nfd_in_pkt_desc pkt3;
    struct nfd_in_pkt_desc pkt4;
    struct nfd_in_pkt_desc pkt5;
    struct nfd_in_pkt_desc pkt6;
    struct nfd_in_pkt_desc pkt7;
};


NFD_INIT_DONE_DECLARE;


/* Shared with issue DMA */
__visible volatile __xread unsigned int nfd_in_data_compl_refl_in = 0;
__visible volatile __xread unsigned int nfd_in_jumbo_compl_refl_in = 0;
__remote volatile __xread unsigned int nfd_in_data_served_refl_in;
__remote volatile SIGNAL nfd_in_data_served_refl_sig;


/* Used for issue DMA 0 */
__shared __gpr unsigned int data_dma_seq_served0 = 0;
__shared __gpr unsigned int data_dma_seq_compl0 = 0;

/* Used for issue DMA 1 */
__shared __gpr unsigned int data_dma_seq_served1 = 0;
__shared __gpr unsigned int data_dma_seq_compl1 = 0;


/* Notify private variables */
static __xwrite unsigned int nfd_in_data_served_refl_out = 0;
static __gpr unsigned int data_dma_seq_sent = 0;
static __gpr mem_ring_addr_t lso_ring_addr;
static __gpr unsigned int lso_ring_num;


static SIGNAL wq_sig0, wq_sig1, wq_sig2, wq_sig3;
static SIGNAL wq_sig4, wq_sig5, wq_sig6, wq_sig7;
static SIGNAL msg_sig0, msg_sig1, qc_sig;
static SIGNAL get_order_sig;    /* Signal for reordering before issuing get */
static SIGNAL msg_order_sig;    /* Signal for reordering on message return */
static SIGNAL_MASK wait_msk;
static unsigned int next_ctx;

__xwrite struct _pkt_desc_batch batch_out;
__xread unsigned int qc_xfer;

#ifdef NFD_IN_LSO_CNTR_ENABLE
static unsigned int nfd_in_lso_cntr_addr = 0;
#endif


#ifdef NFD_IN_WQ_SHARED

#define NFD_IN_RINGS_MEM_IND2(_isl, _emem)                              \
    _NFP_CHIPRES_ASM(.alloc_mem nfd_in_rings_mem0 _emem global          \
                     (NFD_IN_WQ_SZ * NFD_IN_NUM_WQS)                    \
                     (NFD_IN_WQ_SZ * NFD_IN_NUM_WQS))
#define NFD_IN_RINGS_MEM_IND1(_isl, _emem) NFD_IN_RINGS_MEM_IND2(_isl, _emem)
#define NFD_IN_RINGS_MEM_IND0(_isl)                     \
    NFD_IN_RINGS_MEM_IND1(_isl, NFD_IN_WQ_SHARED)
#define NFD_IN_RINGS_MEM(_isl) NFD_IN_RINGS_MEM_IND0(_isl)

#define NFD_IN_RING_INIT_IND0(_isl, _num)                               \
    NFD_IN_RING_NUM_ALLOC(_isl, _num)                                   \
    _NFP_CHIPRES_ASM(.declare_resource nfd_in_ring_mem_res0##_num       \
                     global NFD_IN_WQ_SZ nfd_in_rings_mem0)             \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_in_ring_mem0##_num             \
                     nfd_in_ring_mem_res0##_num global                  \
                     NFD_IN_WQ_SZ NFD_IN_WQ_SZ)                         \
    _NFP_CHIPRES_ASM(.init_mu_ring nfd_in_ring_num0##_num               \
                     nfd_in_ring_mem0##_num)
#define NFD_IN_RING_INIT(_isl, _num) NFD_IN_RING_INIT_IND0(_isl, _num)

#else /* !NFD_IN_WQ_SHARED */

#define NFD_IN_RINGS_MEM_IND2(_isl, _emem)                              \
    _NFP_CHIPRES_ASM(.alloc_mem nfd_in_rings_mem##_isl _emem global     \
                     (NFD_IN_WQ_SZ * NFD_IN_NUM_WQS)                    \
                     (NFD_IN_WQ_SZ * NFD_IN_NUM_WQS))
#define NFD_IN_RINGS_MEM_IND1(_isl, _emem) NFD_IN_RINGS_MEM_IND2(_isl, _emem)
#define NFD_IN_RINGS_MEM_IND0(_isl)                     \
    NFD_IN_RINGS_MEM_IND1(_isl, NFD_PCIE##_isl##_EMEM)
#define NFD_IN_RINGS_MEM(_isl) NFD_IN_RINGS_MEM_IND0(_isl)

#define NFD_IN_RING_INIT_IND0(_isl, _num)                               \
    NFD_IN_RING_NUM_ALLOC(_isl, _num)                                   \
    _NFP_CHIPRES_ASM(.declare_resource nfd_in_ring_mem_res##_isl##_num  \
                     global NFD_IN_WQ_SZ nfd_in_rings_mem##_isl)        \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_in_ring_mem##_isl##_num        \
                     nfd_in_ring_mem_res##_isl##_num                    \
                     global NFD_IN_WQ_SZ NFD_IN_WQ_SZ)                  \
    _NFP_CHIPRES_ASM(.init_mu_ring nfd_in_ring_num##_isl##_num          \
                     nfd_in_ring_mem##_isl##_num)
#define NFD_IN_RING_INIT(_isl, _num) NFD_IN_RING_INIT_IND0(_isl, _num)

#endif /* NFD_IN_WQ_SHARED */


NFD_IN_RINGS_MEM(PCIE_ISL);

#if NFD_IN_NUM_WQS > 0
    NFD_IN_RING_INIT(PCIE_ISL, 0);
#else
    #error "NFD_IN_NUM_WQS must be a power of 2 between 1 and 8"
#endif

#if NFD_IN_NUM_WQS > 1
    NFD_IN_RING_INIT(PCIE_ISL, 1);
#endif

#if NFD_IN_NUM_WQS > 2
    NFD_IN_RING_INIT(PCIE_ISL, 2);
    NFD_IN_RING_INIT(PCIE_ISL, 3);
#endif

#if NFD_IN_NUM_WQS > 4
    NFD_IN_RING_INIT(PCIE_ISL, 4);
    NFD_IN_RING_INIT(PCIE_ISL, 5);
    NFD_IN_RING_INIT(PCIE_ISL, 6);
    NFD_IN_RING_INIT(PCIE_ISL, 7);
#endif

#if NFD_IN_NUM_WQS > 8
    #error "NFD_IN_NUM_WQS > 8 is not supported"
#endif


static __shared mem_ring_addr_t wq_raddr;
static __shared unsigned int wq_num_base;
static __gpr unsigned int dst_q;



#ifdef NFD_IN_ADD_SEQN

#if (NFD_IN_NUM_SEQRS == 1)
/* Add sequence numbers, using a shared GPR to store */
static __shared __gpr unsigned int dst_q_seqn = 0;

#define NFD_IN_ADD_SEQN_PROC                                            \
do {                                                                    \
    pkt_desc_tmp.seq_num = dst_q_seqn;                                  \
    dst_q_seqn++;                                                       \
} while (0)

#else /* (NFD_IN_NUM_SEQRS == 1) */

/* Add sequence numbers, using a LM to store */
static __shared __lmem unsigned int seq_nums[NFD_IN_NUM_SEQRS];

/*
 * XXX this ugly bit of code was the best way I could find to make the
 * compiler generate intelligent assembly here.  This should just be
 * a shift + AND operation to get the LM address.  But if I use the
 * NFD_IN_SEQR_NUM(q), I get 2-4 extra instructions and this is on the
 * fast path in a potentially down-clocked ME.
 */
#define NFD_IN_ADD_SEQN_PROC                                            \
do {                                                                    \
    pkt_desc_tmp.seq_num =                                              \
        seq_nums[(pkt_desc_tmp.__raw[0] >> NFD_IN_SEQR_QSHIFT) &        \
                 (NFD_IN_NUM_SEQRS - 1)]++;                             \
} while (0)

#endif /* (NFD_IN_NUM_SEQRS == 1) */

#else /* NFD_IN_ADD_SEQN */

/* Null sequence number add */
#define NFD_IN_ADD_SEQN_PROC                                            \
do {                                                                    \
} while (0)

#endif /* NFD_IN_ADD_SEQN */

#if (NFD_IN_NUM_WQS == 1)
#define _SET_DST_Q(_pkt)                                                \
do {                                                                    \
} while (0)
#else /* (NFD_IN_NUM_WQS == 1) */
#define _SET_DST_Q(_pkt)                                                \
do {                                                                    \
    /* Removing dst_q support for driving pkts to specified wq */       \
} while (0)
#endif /* (NFD_IN_NUM_WQS == 1) */


/* Registers to receive and store reset state */
__visible volatile __xread unsigned int gather_reset_state_xfer = 0;
__shared __gpr unsigned int gather_reset_state_gpr = 0;


/* XXX Move to some sort of CT reflect library */
__intrinsic void
reflect_data(unsigned int dst_me, unsigned int dst_ctx,
             unsigned int dst_xfer, unsigned int sig_no,
             volatile __xwrite void *src_xfer, size_t size)
{
    unsigned int addr;
    unsigned int count = (size >> 2);
    struct nfp_mecsr_cmd_indirect_ref_0 indirect;

    /* ctassert(__is_write_reg(src_xfer)); */ /* TEMP, avoid volatile warnings */
    ctassert(__is_ct_const(size));

    /* Generic address computation.
     * Could be expensive if dst_me, or dst_xfer
     * not compile time constants */
    addr = ((dst_me & 0xFF0)<<20 | (dst_me & 0xF)<<10 |
            (dst_ctx & 7)<<7 | (dst_xfer & 0x3F)<<2);

    indirect.__raw = 0;
    if (sig_no != 0) {
        indirect.signal_num = sig_no;
        indirect.signal_ctx = dst_ctx;
    }
    local_csr_write(local_csr_cmd_indirect_ref_0, indirect.__raw);

    /* Currently just support reflect_write_sig_remote */
    /* XXX NFP_MECSR_PREV_ALU_OV_SIG_CTX_bit is next to SIG_NUM */
    __asm {
        alu[--, --, b, 3, <<NFP_MECSR_PREV_ALU_OV_SIG_NUM_bit];
        ct[reflect_write_sig_remote, *src_xfer, addr, 0, \
           __ct_const_val(count)], indirect_ref;
    };
}


__intrinsic void
copy_absolute_xfer(__shared __gpr unsigned int *dst, unsigned int src_xnum)
{
    /* XXX assumes src_xnum already accounts for CTX */
    local_csr_write(local_csr_t_index, MECSR_XFER_INDEX(src_xnum));
    __asm alu[*dst, --, B, *$index];
}


/**
 * Perform shared configuration for notify
 */
void
notify_setup_shared()
{
#ifdef NFD_IN_WQ_SHARED
    wq_num_base = NFD_RING_LINK(0, nfd_in, 0);
    wq_raddr = (unsigned long long) NFD_EMEM_SHARED(NFD_IN_WQ_SHARED) >> 8;
#else
    wq_num_base = NFD_RING_LINK(PCIE_ISL, nfd_in, 0);
    wq_raddr = (unsigned long long) NFD_EMEM_LINK(PCIE_ISL) >> 8;
#endif

    /* Kick off ordering */
    reorder_start(NFD_IN_NOTIFY_MANAGER0, &msg_order_sig);
    reorder_start(NFD_IN_NOTIFY_MANAGER0, &get_order_sig);
    reorder_start(NFD_IN_NOTIFY_MANAGER1, &msg_order_sig);
    reorder_start(NFD_IN_NOTIFY_MANAGER1, &get_order_sig);
}


/**
 * Perform per context initialization (for CTX 0 to 7)
 */
void
notify_setup(int side)
{
    dst_q = wq_num_base;
    wait_msk = __signals(&msg_sig0, &msg_sig1, &msg_order_sig);

    next_ctx = reorder_get_next_ctx_off(ctx(), NFD_IN_NOTIFY_STRIDE);

#ifdef NFD_IN_LSO_CNTR_ENABLE
    /* get the location of LSO statistics */
    nfd_in_lso_cntr_addr =
        cntr64_get_addr((__mem40 void *) nfd_in_lso_cntrs);
#endif

    if (side == 0) {
        lso_ring_num = NFD_RING_LINK(PCIE_ISL, nfd_in_issued_lso,
                                     NFD_IN_ISSUED_LSO_RING0_NUM);
        lso_ring_addr = ((((unsigned long long)
                           NFD_EMEM_LINK(PCIE_ISL)) >> 32) << 24);
    } else {
        lso_ring_num =  NFD_RING_LINK(PCIE_ISL, nfd_in_issued_lso,
                                      NFD_IN_ISSUED_LSO_RING1_NUM);
        lso_ring_addr = ((((unsigned long long)
                           NFD_EMEM_LINK(PCIE_ISL)) >> 32) << 24);
    }
}

#ifndef NFD_MU_PTR_DBG_MSK
#define NFD_MU_PTR_DBG_MSK 0x0f000000
#endif

#ifdef NFD_IN_NOTIFY_DBG_CHKS
#define _NOTIFY_MU_CHK(_pkt)                                            \
do {                                                                    \
    if ((batch_in.pkt##_pkt##.__raw[1] & NFD_MU_PTR_DBG_MSK) == 0) {    \
        /* Write the error we read to Mailboxes for debug purposes */   \
        local_csr_write(local_csr_mailbox_0,                            \
                        NFD_IN_NOTIFY_MU_PTR_INVALID);                  \
        local_csr_write(local_csr_mailbox_1,                            \
                        batch_in.pkt##_pkt##.__raw[1]);                 \
                                                                        \
        halt();                                                         \
    }                                                                   \
} while (0)
#else
#define _NOTIFY_MU_CHK(_pkt)                    \
do {} while (0)
#endif


#define _NOTIFY_PROC(_pkt)                                                   \
do {                                                                         \
    NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                               \
                         NFD_IN_LSO_CNTR_T_NOTIFY_ALL_PKT_DESC);             \
    /* finished packet and no LSO */                                         \
    if (batch_in.pkt##_pkt##.eop) {                                          \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                           \
                             NFD_IN_LSO_CNTR_T_NOTIFY_NON_LSO_PKT_DESC);     \
        __critical_path();                                                   \
        _NOTIFY_MU_CHK(_pkt);                                                \
        pkt_desc_tmp.sp0 = 0;                                                \
        pkt_desc_tmp.offset = batch_in.pkt##_pkt##.offset;                   \
        NFD_IN_ADD_SEQN_PROC;                                                \
        batch_out.pkt##_pkt##.__raw[0] = pkt_desc_tmp.__raw[0];              \
        batch_out.pkt##_pkt##.__raw[1] = (batch_in.pkt##_pkt##.__raw[1] |    \
                                          gather_reset_state_gpr);           \
        batch_out.pkt##_pkt##.__raw[2] = batch_in.pkt##_pkt##.__raw[2];      \
        batch_out.pkt##_pkt##.__raw[3] = batch_in.pkt##_pkt##.__raw[3];      \
                                                                             \
        _SET_DST_Q(_pkt);                                                    \
        __mem_workq_add_work(dst_q, wq_raddr, &batch_out.pkt##_pkt,          \
                             out_msg_sz, out_msg_sz, sig_done,               \
                             &wq_sig##_pkt);                                 \
    } else if (batch_in.pkt##_pkt##.lso != NFD_IN_ISSUED_DESC_LSO_NULL) {    \
        /* else LSO packets */                                               \
        __xread struct nfd_in_lso_desc lso_pkt;                              \
        SIGNAL_PAIR lso_sig_pair;                                            \
        SIGNAL_MASK lso_wait_msk;                                            \
        __shared __gpr unsigned int jumbo_compl_seq;                         \
        int seqn_chk;                                                        \
                                                                             \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                           \
                             NFD_IN_LSO_CNTR_T_NOTIFY_LSO_PKT_DESC);         \
        /* XXX __signals(&lso_sig_pair.even) lists both even and odd */      \
        lso_wait_msk = 1 << __signal_number(&lso_sig_pair.even);             \
                                                                             \
                                                                             \
         /* finished packet with LSO to handle */                            \
        for (;;) {                                                           \
            /* read packet from nfd_in_issued_lso_ring */                    \
            __mem_ring_get(lso_ring_num, lso_ring_addr, &lso_pkt,            \
                           sizeof(lso_pkt), sizeof(lso_pkt), sig_done,       \
                           &lso_sig_pair);                                   \
            wait_sig_mask(lso_wait_msk);                                     \
            __implicit_read(&lso_sig_pair.even);                             \
            __implicit_read(&wq_sig##_pkt);                                  \
            while (signal_test(&lso_sig_pair.odd)) {                         \
                /* Ring get failed, retry */                                 \
                __mem_ring_get(lso_ring_num, lso_ring_addr, &lso_pkt,        \
                               sizeof(lso_pkt), sizeof(lso_pkt), sig_done,   \
                               &lso_sig_pair);                               \
                wait_for_all_single(&lso_sig_pair.even);                     \
            }                                                                \
                                                                             \
            NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                       \
                    NFD_IN_LSO_CNTR_T_NOTIFY_ALL_PKT_FM_LSO_RING);           \
                                                                             \
            /* Wait for the jumbo compl seq to catch up to the encoded seq */ \
            copy_absolute_xfer(&jumbo_compl_seq, jumbo_compl_xnum);          \
            seqn_chk = lso_pkt.jumbo_seq - jumbo_compl_seq;                  \
            while (seqn_chk > 0) {                                           \
                ctx_swap();                                                  \
                                                                             \
                copy_absolute_xfer(&jumbo_compl_seq, jumbo_compl_xnum);      \
                seqn_chk = lso_pkt.jumbo_seq - jumbo_compl_seq;              \
            }                                                                \
                                                                             \
            /* We can carry on processing the descriptor */                  \
            /* Check whether it should go to the app */                      \
            if (lso_pkt.desc.eop) {                                          \
                /* XXX always check the MU pointer in LSO handling. */       \
                if ((lso_pkt.desc.__raw[1] & NFD_MU_PTR_DBG_MSK) == 0) {     \
                    /* Write the error we read to Mailboxes */               \
                    /* for debug purposes */                                 \
                    local_csr_write(local_csr_mailbox_0,                     \
                                    NFD_IN_NOTIFY_MU_PTR_INVALID);           \
                    local_csr_write(local_csr_mailbox_1,                     \
                                    lso_pkt.desc.__raw[1]);                  \
                                                                             \
                    halt();                                                  \
                }                                                            \
                                                                             \
                pkt_desc_tmp.sp0 = 0;                                        \
                pkt_desc_tmp.offset = lso_pkt.desc.offset;                   \
                NFD_IN_ADD_SEQN_PROC;                                        \
                batch_out.pkt##_pkt##.__raw[0] = pkt_desc_tmp.__raw[0];      \
                batch_out.pkt##_pkt##.__raw[1] = (lso_pkt.desc.__raw[1] |    \
                                                  gather_reset_state_gpr);   \
                batch_out.pkt##_pkt##.__raw[2] = lso_pkt.desc.__raw[2];      \
                batch_out.pkt##_pkt##.__raw[3] = lso_pkt.desc.__raw[3];      \
                _SET_DST_Q(_pkt);                                            \
                                                                             \
                __mem_workq_add_work(dst_q, wq_raddr, &batch_out.pkt##_pkt,  \
                                     out_msg_sz, out_msg_sz,                 \
                                     sig_done, &wq_sig##_pkt);               \
                lso_wait_msk |= __signals(&wq_sig##_pkt);                    \
                                                                             \
                NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                   \
                        NFD_IN_LSO_CNTR_T_NOTIFY_ALL_LSO_PKTS_TO_ME_WQ);     \
                if (lso_pkt.desc.lso_end) {                                  \
                    NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,               \
                            NFD_IN_LSO_CNTR_T_NOTIFY_LSO_END_PKTS_TO_ME_WQ); \
                }                                                            \
            } else {                                                         \
                NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                   \
                        NFD_IN_LSO_CNTR_T_NOTIFY_LSO_CONT_SKIP_ME_WQ);       \
                                                                             \
                /* XXX lso_pkt.desc.lso must be NFD_IN_ISSUED_DESC_LSO_RET */ \
                /* else we have a logic bug or ring corruption */            \
                if (lso_pkt.desc.lso != NFD_IN_ISSUED_DESC_LSO_RET) {        \
                    local_csr_write(local_csr_mailbox_0,                     \
                                    NFD_IN_NOTIFY_LSO_DESC_INVALID);         \
                    local_csr_write(local_csr_mailbox_1,                     \
                                    lso_pkt.desc.__raw[0]);                  \
                    halt();                                                  \
                }                                                            \
                                                                             \
                /* Remove the wq signal from the wait mask */                \
                /* XXX flag the wq_sig as written for live range tracking */ \
                wait_msk &= ~__signals(&wq_sig##_pkt);                       \
                __implicit_write(&wq_sig##_pkt);                             \
            }                                                                \
                                                                             \
            /* if it is last LSO being read from ring */                     \
            if (lso_pkt.desc.lso == NFD_IN_ISSUED_DESC_LSO_RET) {            \
                /* XXX this may be a msg rather than a pkt, if cont */       \
                NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                   \
                        NFD_IN_LSO_CNTR_T_NOTIFY_LAST_PKT_FM_LSO_RING);      \
                                                                             \
                /* Break out of loop processing LSO ring */                  \
                /* TODO how can we catch obvious MU ring corruption? */      \
                break;                                                       \
            }                                                                \
        }                                                                    \
    } else {                                                                 \
        /* Remove the wq signal from the wait mask */                        \
        /* XXX flag the wq_sig as written for live range tracking */         \
        wait_msk &= ~__signals(&wq_sig##_pkt);                               \
        __implicit_write(&wq_sig##_pkt);                                     \
    }                                                                        \
} while (0)


/**
 * Dequeue a batch of "issue_dma" messages and process that batch, incrementing
 * TX.R for the queue and adding an output message to one of the PCI.IN work
 * queueus.  An output message is only sent for the final message for a packet
 * (EOP bit set).  A count of the total number of descriptors in the batch is
 * added by the "issue_dma" block.
 *
 * We reorder before getting a batch of "issue_dma" messages and then ensure
 * batches are processed in order.  If there is no batch of messages to fetch,
 * we must still participate in the "msg_order_sig" ordering.
 */
__intrinsic void
_notify(__gpr unsigned int *complete, __gpr unsigned int *served,
        int input_ring, unsigned int data_compl_xnum,
        unsigned int jumbo_compl_xnum)
{

    unsigned int n_batch;
    unsigned int q_batch;
    unsigned int qc_queue;
    unsigned int num_avail;

    unsigned int out_msg_sz = sizeof(struct nfd_in_pkt_desc);

    __xread struct _issued_pkt_batch batch_in;
    struct _pkt_desc_batch batch_tmp;
    struct nfd_in_pkt_desc pkt_desc_tmp;

    /* Reorder before potentially issuing a ring get */
    wait_for_all(&get_order_sig);

    /* There is a FULL batch to process
     * XXX assume that issue_dma inc's dma seq for each nfd_in_issued_desc in
     * batch. */
    num_avail = *complete - *served;
    if (num_avail >= NFD_IN_MAX_BATCH_SZ)
    {
        /* Process whole batch */
        __critical_path();

        /* Participate in ctm_ring_get ordering */
        reorder_done_opt(&next_ctx, &get_order_sig);

        ctm_ring_get(NOTIFY_RING_ISL, input_ring, &batch_in.pkt0,
                     (sizeof(struct nfd_in_issued_desc) * 4), &msg_sig0);
        ctm_ring_get(NOTIFY_RING_ISL, input_ring, &batch_in.pkt4,
                     (sizeof(struct nfd_in_issued_desc) * 4), &msg_sig1);

        __asm {
            ctx_arb[--], defer[2];
            local_csr_wr[local_csr_active_ctx_wakeup_events, wait_msk];
            alu[*served, *served, +, NFD_IN_MAX_BATCH_SZ];
        }

        wait_msk = __signals(&wq_sig0, &wq_sig1, &wq_sig2, &wq_sig3,
                             &wq_sig4, &wq_sig5, &wq_sig6, &wq_sig7,
                             &qc_sig, &msg_sig0, &msg_sig1, &msg_order_sig);
        __implicit_read(&wq_sig0);
        __implicit_read(&wq_sig1);
        __implicit_read(&wq_sig2);
        __implicit_read(&wq_sig3);
        __implicit_read(&wq_sig4);
        __implicit_read(&wq_sig5);
        __implicit_read(&wq_sig6);
        __implicit_read(&wq_sig7);
        __implicit_read(&qc_sig);
        __implicit_read(&msg_sig0);
        __implicit_read(&msg_sig1);
        __implicit_read(&msg_order_sig);
        __implicit_read(&qc_xfer);

        /* Batches have a least one packet, but n_batch may still be
         * zero, meaning that the queue is down.  In this case, EOP for
         * all the packets should also be zero, so that notify will
         * essentially skip the batch.
         */
        q_batch = batch_in.pkt0.q_num;
        n_batch = batch_in.pkt0.num_batch;

#ifdef NFD_VNIC_DBG_CHKS
        if (n_batch > NFD_IN_MAX_BATCH_SZ) {
            halt();
        }
#endif

        /* Interface and queue info are the same for all packets in batch */
        pkt_desc_tmp.intf = PCIE_ISL;
        pkt_desc_tmp.q_num = q_batch;
#ifndef NFD_IN_ADD_SEQN
        pkt_desc_tmp.seq_num = 0;
#endif

        _NOTIFY_PROC(0);
        _NOTIFY_PROC(1);
        _NOTIFY_PROC(2);
        _NOTIFY_PROC(3);
        _NOTIFY_PROC(4);
        _NOTIFY_PROC(5);
        _NOTIFY_PROC(6);
        _NOTIFY_PROC(7);

        /* Allow the next context taking a message to go.
         * We have finished _NOTIFY_PROC() where we need to
         * lock out other threads. */
        reorder_done_opt(&next_ctx, &msg_order_sig);

        /* Map batch.queue to a QC queue and increment the TX_R pointer
         * for that queue by n_batch */
        qc_queue = NFD_NATQ2QC(NFD_BMQ2NATQ(q_batch), NFD_IN_TX_QUEUE);
        __qc_add_to_ptr(PCIE_ISL, qc_queue, QC_RPTR, n_batch, &qc_xfer,
                        sig_done, &qc_sig);

    } else if (num_avail > 0) {
        /* There is a partial batch - process messages one at a time. */
        unsigned int partial_served = 0;

        wait_msk &= ~__signals(&msg_sig1);

        /* ctm_ring_get() uses sig_done */
        ctm_ring_get(NOTIFY_RING_ISL, input_ring, &batch_in.pkt0,
                     sizeof(struct nfd_in_issued_desc), &msg_sig0);

        wait_sig_mask(wait_msk);
        __implicit_read(&wq_sig0);
        __implicit_read(&wq_sig1);
        __implicit_read(&wq_sig2);
        __implicit_read(&wq_sig3);
        __implicit_read(&wq_sig4);
        __implicit_read(&wq_sig5);
        __implicit_read(&wq_sig6);
        __implicit_read(&wq_sig7);
        __implicit_read(&qc_sig);
        __implicit_read(&msg_sig0);
        __implicit_read(&msg_order_sig);
        __implicit_read(&qc_xfer);


        /* This is the first message in the batch. Do not wait for
         * signals that will not be set while processing a partial
         * batch and store batch info. */
        q_batch = batch_in.pkt0.q_num;
        n_batch = batch_in.pkt0.num_batch;
        wait_msk = __signals(&msg_sig0, &wq_sig0);

        /* Interface and queue info is the same for all packets in batch */
        pkt_desc_tmp.intf = PCIE_ISL;
        pkt_desc_tmp.q_num = q_batch;
#ifndef NFD_IN_ADD_SEQN
        pkt_desc_tmp.seq_num = 0;
#endif

        for (;;) {
            /* Count the message and service it */
            partial_served++;
            _NOTIFY_PROC(0);

            /* Wait for new messages in ctm ring.
             * Note: other contexts should not fetch new messages or update
             *       'served' until this one has fetched BATCH_SZ messages. */
            while (num_avail <= partial_served) {
                ctx_wait(voluntary);
                /* Copy in reflected data without checking signals */
                copy_absolute_xfer(&gather_reset_state_gpr,
                                   __xfer_reg_number(&gather_reset_state_xfer));
                copy_absolute_xfer(complete, data_compl_xnum);

                num_avail = *complete - *served;
            }

            /* ctm_ring_get() uses sig_done */
            ctm_ring_get(NOTIFY_RING_ISL, input_ring, &batch_in.pkt0,
                         sizeof(struct nfd_in_issued_desc), &msg_sig0);

            /* We always service NFD_IN_MAX_BATCH_SZ messages */
            if (partial_served == (NFD_IN_MAX_BATCH_SZ - 1)) {
                break;
            }

            wait_sig_mask(wait_msk);
            __implicit_read(&wq_sig0);
            __implicit_read(&msg_sig0);
            wait_msk |= __signals(&wq_sig0);
        }

        /* We have finished fetching the messages from the ring.
         * Update served and allow other contexts to get messages
         * from ctm ring */
        *served += NFD_IN_MAX_BATCH_SZ;
        reorder_done_opt(&next_ctx, &get_order_sig);

        /* Wait for the last get to complete */
        wait_sig_mask(wait_msk);
        __implicit_read(&wq_sig0);
        __implicit_read(&msg_sig0);

        /* Set up wait_msk to process a full batch next */
        /* XXX Assume we will do a WQ put, _NOTIFY_PROC will clear
           wq_sig0 if necessary */
        wait_msk = __signals(&wq_sig0, &msg_sig0, &msg_sig1, &qc_sig,
                             &msg_order_sig);

        /* Process the final descriptor from the batch */
        _NOTIFY_PROC(0);

        /* Allow the next context taking a message to go.
         * We have finished _NOTIFY_PROC() where we need to
         * lock out other threads. */
        reorder_done_opt(&next_ctx, &msg_order_sig);

        /* Map batch.queue to a QC queue and increment the TX_R pointer
         * for that queue by n_batch */
        qc_queue = NFD_NATQ2QC(NFD_BMQ2NATQ(q_batch), NFD_IN_TX_QUEUE);
        __qc_add_to_ptr(PCIE_ISL, qc_queue, QC_RPTR, n_batch, &qc_xfer,
                        sig_done, &qc_sig);

    } else {
        /* Participate in ctm_ring_get ordering */
        reorder_done_opt(&next_ctx, &get_order_sig);

        /* Participate in msg ordering */
        wait_for_all(&msg_order_sig);
        reorder_done_opt(&next_ctx, &msg_order_sig);
    }
}


__forceinline void
notify(int side)
{
    if (side == 0) {
        _notify(&data_dma_seq_compl0, &data_dma_seq_served0,
                NFD_IN_ISSUED_RING0_NUM,
                (NFD_IN_NOTIFY_MANAGER0 << 5 |
                 __xfer_reg_number(&nfd_in_data_compl_refl_in)),
                (NFD_IN_NOTIFY_MANAGER0 << 5 |
                 __xfer_reg_number(&nfd_in_jumbo_compl_refl_in)));
    } else {
        _notify(&data_dma_seq_compl1, &data_dma_seq_served1,
                NFD_IN_ISSUED_RING1_NUM,
                (NFD_IN_NOTIFY_MANAGER1 << 5 |
                 __xfer_reg_number(&nfd_in_data_compl_refl_in)),
                (NFD_IN_NOTIFY_MANAGER1 << 5 |
                 __xfer_reg_number(&nfd_in_jumbo_compl_refl_in)));
    }
}


/**
 * Check autopush for seq_compl and reflect seq_served to issue_dma ME
 *
 * "data_dma_seq_compl" tracks the completed gather DMAs.  It is needed by
 * notify to determine when to service the "nfd_in_issued_ring".  The
 * issue_dma ME needs the sequence number more urgently (for in flight
 * DMA tracking) so it constructs the sequence number and reflects the
 * value to this ME.  It must be copied to shared GPRs for worker threads.
 *
 * "data_dma_seq_served" is state owned by this ME.  The issue_dma ME
 * needs the value to determine how many batches can be added to the
 * "nfd_in_issued_ring", so the current value is reflected to that
 * ME.  "data_dma_seq_sent" is used to track which sequence number
 * has been reflected, so that it is not resent.
 */
__intrinsic void
distr_notify(int side)
{
    /* Store reset state in absolute GPR */
    copy_absolute_xfer(&gather_reset_state_gpr,
                       __xfer_reg_number(&gather_reset_state_xfer));

    if (side == 0) {
#ifdef NFD_IN_HAS_ISSUE0
        data_dma_seq_compl0 = nfd_in_data_compl_refl_in;

        if (data_dma_seq_served0 != data_dma_seq_sent) {
            __implicit_read(&nfd_in_data_served_refl_out);

            data_dma_seq_sent = data_dma_seq_served0;

            nfd_in_data_served_refl_out = data_dma_seq_sent;
            reflect_data(NFD_IN_DATA_DMA_ME0, NFD_IN_ISSUE_MANAGER,
                         __xfer_reg_number(&nfd_in_data_served_refl_in,
                                           NFD_IN_DATA_DMA_ME0),
                         __signal_number(&nfd_in_data_served_refl_sig,
                                         NFD_IN_DATA_DMA_ME0),
                         &nfd_in_data_served_refl_out,
                         sizeof nfd_in_data_served_refl_out);
        }
#endif
    } else {

#ifdef NFD_IN_HAS_ISSUE1
        data_dma_seq_compl1 = nfd_in_data_compl_refl_in;

        if (data_dma_seq_served1 != data_dma_seq_sent) {
            __implicit_read(&nfd_in_data_served_refl_out);

            data_dma_seq_sent = data_dma_seq_served1;

            nfd_in_data_served_refl_out = data_dma_seq_sent;
            reflect_data(NFD_IN_DATA_DMA_ME1, NFD_IN_ISSUE_MANAGER,
                         __xfer_reg_number(&nfd_in_data_served_refl_in,
                                           NFD_IN_DATA_DMA_ME1),
                         __signal_number(&nfd_in_data_served_refl_sig,
                                         NFD_IN_DATA_DMA_ME1),
                         &nfd_in_data_served_refl_out,
                         sizeof nfd_in_data_served_refl_out);
        }
#endif
    }
}


int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {

        nfd_cfg_check_pcie_link(); /* Will halt ME on failure */

        /*
         * This function will start ordering for CTX0,
         * the manager for loop 0
         */
        notify_setup_shared();

        /* NFD_INIT_DONE_SET(PCIE_ISL, 2);     /\* XXX Remove? *\/ */

    }

    /* Test which side the context is servicing */
    if ((ctx() & (NFD_IN_NOTIFY_STRIDE - 1)) == 0) {

#ifdef NFD_IN_HAS_ISSUE0
        notify_setup(0);

        for (;;) {
            if (ctx() == NFD_IN_NOTIFY_MANAGER0) {
                distr_notify(0);
            }

            notify(0);
            notify(0);
        }
#else
        for (;;) {
            ctx_swap();
        }
#endif


    } else {

#ifdef NFD_IN_HAS_ISSUE1
        notify_setup(1);

        for (;;) {
            if (ctx() == NFD_IN_NOTIFY_MANAGER1) {
                distr_notify(1);
            }

            notify(1);
            notify(1);
        }
#else
        for (;;) {
            ctx_swap();
        }
#endif

    }
}
