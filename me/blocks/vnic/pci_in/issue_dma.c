/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/issue_dma.c
 * @brief         Code to DMA packet data to the NFP
 */

#include <assert.h>
#include <nfp.h>
#include <types.h>

#include <nfp/cls.h>
#include <nfp/me.h>
#include <nfp/pcie.h>
#include <nfp/mem_ring.h>
#include <std/reg_utils.h>
#include <std/cntrs.h>
#include <blm/libblm.h>
#include <blm/libblm_pkt_fl.h>

#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_in.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_internal.h>

/*#include <vnic/utils/cls_ring.h> */ /* XXX THS-50 workaround */
#include <vnic/utils/ctm_ring.h> /* XXX THS-50 workaround */
#include <vnic/utils/nn_ring.h>
#include <vnic/utils/ordering.h>
#include <vnic/utils/qc.h>


struct _tx_desc_batch {
    struct nfd_in_tx_desc pkt0;
    struct nfd_in_tx_desc pkt1;
    struct nfd_in_tx_desc pkt2;
    struct nfd_in_tx_desc pkt3;
};

struct _issued_pkt_batch {
    struct nfd_in_issued_desc pkt0;
    struct nfd_in_issued_desc pkt1;
    struct nfd_in_issued_desc pkt2;
    struct nfd_in_issued_desc pkt3;
};

struct _dma_desc_batch {
    struct nfp_pcie_dma_cmd pkt0;
    struct nfp_pcie_dma_cmd pkt1;
    struct nfp_pcie_dma_cmd pkt2;
    struct nfp_pcie_dma_cmd pkt3;
};


NFD_BLM_Q_ALLOC(NFD_IN_BLM_POOL);


/* Ring declarations */
/* XXX use CLS ring API when available */
/* XXX THS-50 workaround, use CTM instead of CLS rings */
__export __ctm __align(sizeof(struct nfd_in_issued_desc) * NFD_IN_ISSUED_RING_SZ)
    struct nfd_in_issued_desc nfd_in_issued_ring[NFD_IN_ISSUED_RING_SZ];

#define NFD_IN_DESC_RING_SZ (NFD_IN_MAX_BATCH_SZ * NFD_IN_DESC_BATCH_Q_SZ * \
                      sizeof(struct nfd_in_tx_desc))
__export __shared __cls __align(NFD_IN_DESC_RING_SZ) struct nfd_in_tx_desc
    desc_ring[NFD_IN_MAX_BATCH_SZ * NFD_IN_DESC_BATCH_Q_SZ];

static __shared __gpr unsigned int desc_ring_base;


/* Storage declarations */
__shared __lmem struct nfd_in_dma_state queue_data[NFD_IN_MAX_QUEUES];

static unsigned int nfd_in_lso_cntr_addr = 0;

/* storage for LSO header on a per queue basis */
#define NFD_IN_MAX_LSO_HDR_SZ 256
__export __shared __ctm __align(NFD_IN_MAX_LSO_HDR_SZ) unsigned char
    lso_hdr_data[NFD_IN_MAX_LSO_HDR_SZ * NFD_IN_MAX_QUEUES];

static __shared __gpr unsigned int lso_hdr_data_base;

#define NFD_IN_ISSUED_LSO_RING_INIT_IND2(_isl, _emem)                        \
    ASM(.alloc_mem nfd_in_issued_lso_ring_mem##_isl _emem global             \
        NFD_IN_ISSUED_LSO_RING_SZ NFD_IN_ISSUED_LSO_RING_SZ)                 \
    ASM(.init_mu_ring nfd_in_issued_lso_ring_num##_isl##0                    \
        nfd_in_issued_lso_ring_mem##_isl)
#define NFD_IN_ISSUED_LSO_RING_INIT_IND1(_isl, _emem)                        \
    NFD_IN_ISSUED_LSO_RING_INIT_IND2(_isl, _emem)
#define NFD_IN_ISSUED_LSO_RING_INIT_IND0(_isl)                               \
    NFD_IN_ISSUED_LSO_RING_INIT_IND1(_isl, NFD_PCIE##_isl##_EMEM)
#define NFD_IN_ISSUED_LSO_RING_INIT(_isl)                                    \
    NFD_IN_ISSUED_LSO_RING_INIT_IND0(_isl)

NFD_IN_ISSUED_LSO_RING_INIT(PCIE_ISL);

#define NFD_IN_ISSUED_LSO_RING_ADDR_IND(_isl)                                \
    _link_sym(nfd_in_issued_lso_ring_mem##_isl)
#define NFD_IN_ISSUED_LSO_RING_ADDR(_isl)                                    \
    NFD_IN_ISSUED_LSO_RING_ADDR_IND(_isl)


static __gpr mem_ring_addr_t nfd_in_issued_lso_ring_addr = 0;
static __gpr unsigned int nfd_in_issued_lso_ring_num = 0;

/* Sequence number declarations */
__shared __gpr unsigned int gather_dma_seq_compl = 0;
__shared __gpr unsigned int gather_dma_seq_serv = 0;

__shared __gpr unsigned int data_dma_seq_issued = 0;
extern __shared __gpr unsigned int data_dma_seq_safe;

/* Signals and transfer registers for managing
 * gather_dma_seq_compl updates*/
__visible volatile __xread unsigned int nfd_in_gather_compl_refl_in;
__visible volatile SIGNAL nfd_in_gather_compl_refl_sig;

/* DMA descriptor template */
static __gpr struct nfp_pcie_dma_cmd descr_tmp;

/* Output transfer registers */
static __xwrite struct _dma_desc_batch dma_out;
static __xwrite struct _issued_pkt_batch batch_out;

/* Signalling */
static SIGNAL tx_desc_sig, msg_sig, desc_order_sig, dma_order_sig;
static SIGNAL dma_sig0, dma_sig1, dma_sig2, dma_sig3;
static SIGNAL_MASK wait_msk;

unsigned int next_ctx;

/* Configure the NN ring */
NN_RING_ZERO_PTRS;
NN_RING_EMPTY_ASSERT_SET(0);


/**
 * Add a length to a PCIe address, with carry to PCIe HI
 * @param descr_tmp_raw     "struct nfp_pcie_dma_cmd" to update
 * @param dma_len           length to add to the address
 *
 * "descr_tmp_raw" must point to the "unsigned int __raw[4]" part
 * of the union.
 */
__intrinsic void
_add_to_pcie_addr(__gpr unsigned int *descr_tmp_raw, unsigned int dma_len)
{
    /* We need to use the +carry op to update the 8bit PCIe HI value.
     * This field is in the low 8bits of __raw[3].  Therefore we
     * use inline asm. */
    __asm { alu[descr_tmp_raw[2], descr_tmp_raw[2], +, dma_len] }
    __asm { alu[descr_tmp_raw[3], descr_tmp_raw[3], +carry, 0] }
}


/* Enable B0 DMA ByteMask swapping to ensure that DMAs with the byte
 * swap token complete correctly for DMAs that aren't 4B multiples in size. */
void
_issue_dma_enable_DmaByteMaskSwap(unsigned char pcie_isl)
{
/* XXX nfp6000/nfp_pcie.h should provide this */
#define NFP_PCIE_DMA_DBG_REG_0          0x400f0

    __xwrite unsigned int data = 0x80000000;
    __gpr unsigned int addr_hi = pcie_isl << 30;
    unsigned int dma_dbg_reg_0_addr = NFP_PCIE_DMA_DBG_REG_0;
    SIGNAL sig;

    __asm pcie[write_pci, data, addr_hi, <<8, dma_dbg_reg_0_addr, 1], \
        ctx_swap[sig]
}


/**
 * Perform shared configuration for issue_dma
 */
void
issue_dma_setup_shared()
{
    struct nfp_pcie_dma_cfg cfg_tmp;
    __xwrite struct nfp_pcie_dma_cfg cfg;
    __xwrite uint32_t lso_hdr_data_init_xw = 0xDEAD0000;
    __gpr uint32_t i;

    /* XXX THS-50 workaround */
    /* cls_ring_setup(NFD_IN_ISSUED_RING_NUM, nfd_in_issued_ring,
     * sizeof nfd_in_issued_ring); */
    ctm_ring_setup(NFD_IN_ISSUED_RING_NUM, nfd_in_issued_ring,
                   sizeof nfd_in_issued_ring);

/* Enable B0 DMA ByteMask swapping to ensure that DMAs with the byte
 * swap token complete correctly for DMAs that aren't 4B multiples in size. */
#if __REVISION_MIN >= __REVISION_B0
    _issue_dma_enable_DmaByteMaskSwap(PCIE_ISL);
#endif


    /*
     * Initialise the CLS TX descriptor ring
     */
    desc_ring_base = ((unsigned int) &desc_ring) & 0xFFFFFFFF;

    /* Initialize the CLS LSO header data */
    lso_hdr_data_base = ((unsigned int) &lso_hdr_data) & 0xFFFFFFFF;
    for (i = 0; i < ((NFD_IN_MAX_LSO_HDR_SZ >> 2) * NFD_IN_MAX_QUEUES); i++) {
        lso_hdr_data_init_xw = 0xDEADBEEF;
        mem_write32(&lso_hdr_data_init_xw, (__mem void *)&lso_hdr_data[i * 4],
                    sizeof(lso_hdr_data_init_xw));
    }

    /*
     * Setup the DMA configuration registers
     * XXX PCI.IN and PCI.OUT use the same settings,
     * could share configuration registers.
     */
    cfg_tmp.__raw = 0;
    /* Signal only configuration for null messages */
    cfg_tmp.signal_only_odd = 1;
    cfg_tmp.target_64_odd = 1;
    cfg_tmp.cpp_target_odd = 7;
    /* Regular configuration */
#ifdef NFD_VNIC_NO_HOST
    /* Use signal_only for seqn num generation
     * Don't actually DMA data */
    cfg_tmp.signal_only_even = 1;
#else
    cfg_tmp.signal_only_even = 0;
#endif
    cfg_tmp.end_pad_even = 0;
    cfg_tmp.start_pad_even = 0;
    cfg_tmp.target_64_even = 1;
    cfg_tmp.cpp_target_even = 7;
    cfg = cfg_tmp;

    pcie_dma_cfg_set_pair(PCIE_ISL, NFD_IN_DATA_CFG_REG, &cfg);

    /* Kick off ordering */
    reorder_start(NFD_IN_ISSUE_START_CTX, &desc_order_sig);
    reorder_start(NFD_IN_ISSUE_START_CTX, &dma_order_sig);
}


/**
 * Setup PCI.IN configuration for the vNIC specified in cfg_msg
 * @param cfg_msg   Standard configuration message
 *
 * This method handles all PCI.IN configuration related to bringing a vNIC up
 * or down on the "issue_dma" ME.
 */
__intrinsic void
issue_dma_vnic_setup(struct nfd_cfg_msg *cfg_msg)
{
    unsigned int queue;
    unsigned int bmsk_queue;

    ctassert(__is_log2(NFD_MAX_VF_QUEUES));
    ctassert(__is_log2(NFD_MAX_PF_QUEUES));

    nfd_cfg_next_queue(cfg_msg, &queue);

    if (cfg_msg->error || !cfg_msg->interested) {
        return;
    }

    queue += cfg_msg->vnic * NFD_MAX_VF_QUEUES;
    bmsk_queue = NFD_NATQ2BMQ(queue);

    if (cfg_msg->up_bit && !queue_data[bmsk_queue].up) {
        /* Initialise queue state */
        queue_data[bmsk_queue].sp0 = 0;
        queue_data[bmsk_queue].lso_hdr_len = 0;
        queue_data[bmsk_queue].lso_payload_len = 0;
        queue_data[bmsk_queue].rid = 0;
        if (cfg_msg->vnic != NFD_MAX_VFS) {
            queue_data[bmsk_queue].rid = cfg_msg->vnic + NFD_CFG_VF_OFFSET;
        }
        queue_data[bmsk_queue].cont = 0;
        queue_data[bmsk_queue].up = 1;
        queue_data[bmsk_queue].curr_buf = 0;
        queue_data[bmsk_queue].offset = 0;

    } else if (!cfg_msg->up_bit && queue_data[bmsk_queue].up) {
        /* Free the MU buffer */
        if (queue_data[bmsk_queue].curr_buf != 0) {
            unsigned int blm_raddr;
            unsigned int blm_rnum;

            /* XXX possibly move BLM constants to GPRs
             * if some are available */
            blm_raddr = (((unsigned long long) NFD_IN_BLM_RADDR >> 8) &
                         0xff000000);
            blm_rnum = NFD_BLM_Q_LINK(NFD_IN_BLM_POOL);
            mem_ring_journal_fast(blm_rnum, blm_raddr,
                                  queue_data[bmsk_queue].curr_buf);
        }

        /* Clear queue state */
        queue_data[bmsk_queue].sp0 = 0;
        queue_data[bmsk_queue].lso_hdr_len = 0;
        queue_data[bmsk_queue].lso_payload_len = 0;
        /* Leave RID configured after first set */
        /* "cont" is used as part of the "up" signalling,
         * to move the "up" test off the fast path. */
        queue_data[bmsk_queue].cont = 1;
        queue_data[bmsk_queue].up = 0;
        queue_data[bmsk_queue].curr_buf = 0;
        queue_data[bmsk_queue].offset = 0;
    }
}


/**
 * Perform per context initialisation (for CTX 1 to 7)
 */
void
issue_dma_setup()
{
    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, PCIe address,
     * and dma_mode will be overwritten per transaction.
     */
    descr_tmp.rid_override = 1;
    descr_tmp.trans_class = 0;
    descr_tmp.cpp_token = NFD_IN_DATA_DMA_TOKEN;
    descr_tmp.dma_cfg_index = NFD_IN_DATA_CFG_REG;

    /* wait_msk initially only needs tx_desc_sig and dma_order_sig
     * No DMAs or messages have been issued at this stage */
    wait_msk = __signals(&tx_desc_sig, &dma_order_sig);
    next_ctx = reorder_get_next_ctx(NFD_IN_ISSUE_START_CTX);

#ifdef NFD_IN_LSO_CNTR_ENABLE
    nfd_in_lso_cntr_addr = cntr64_get_addr((__mem void *) nfd_in_lso_cntrs);
#endif
    nfd_in_issued_lso_ring_num = NFD_RING_LINK(PCIE_ISL, nfd_in_issued_lso,
                                               0);
    nfd_in_issued_lso_ring_addr = ((((unsigned long long)
                                      NFD_EMEM_LINK(PCIE_ISL)) >> 32) << 24);
}


/**
 * Check for gather_dma_seq_compl updates
 *
 * The gather ME tracks gather DMA completions and reflects the
 * full sequence number to this ME.  The value must be copied from
 * transfer registers to shared GPRs for the worker threads.  This
 * function runs on CTX0 only.
 */
__intrinsic void
issue_dma_gather_seq_recv()
{
    if (signal_test(&nfd_in_gather_compl_refl_sig)) {
        gather_dma_seq_compl = nfd_in_gather_compl_refl_in;
    }
}


/* XXX temporarily enable _ISSUE_PROC_MU_CHK even without debug checks.
 * This gives us extra protection of the CTM counters while PCI.OUT is not
 * double checking credits. */
#ifndef NFD_MU_PTR_DBG_MSK
#define NFD_MU_PTR_DBG_MSK 0x1f000000
#endif

#ifdef NFD_VNIC_DBG_CHKS
#define _ISSUE_PROC_MU_CHK(_val)                                        \
    if ((_val & NFD_MU_PTR_DBG_MSK) == 0) {                             \
        halt();                                                         \
    }
#else
#define _ISSUE_PROC_MU_CHK(_val)
#endif

#ifdef NFD_IN_LSO_CNTR_ENABLE
#define _LSO_TX_DESC_TYPE_CNTR(_pkt)                                \
    if (tx_desc.pkt##_pkt##.eop) {                                  \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                  \
                          NFD_IN_LSO_CNTR_T_ISSUED_LSO_EOP_TX_DESC);\
    } else {                                                        \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                  \
                         NFD_IN_LSO_CNTR_T_ISSUED_LSO_CONT_TX_DESC);\
    }
#else
#define _LSO_TX_DESC_TYPE_CNTR(_pkt)
#endif

#define _ISSUE_PROC_JUMBO(_pkt, _enq_sig, _dma_sig)                     \
do {                                                                    \
    /* Issue DMA for 4k of segment, updating processing state */        \
    pcie_dma_set_sig(&descr_tmp, __MEID, ctx(),                         \
                     __signal_number(&_dma_sig));                       \
    dma_out.pkt##_pkt## = descr_tmp;                                    \
                                                                        \
    __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,                      \
                   NFD_IN_DATA_DMA_QUEUE,                               \
                   sig_done, &_enq_sig);                                \
    __implicit_write(&_dma_sig);                                        \
                                                                        \
    _add_to_pcie_addr(descr_tmp.__raw, PCIE_DMA_MAX_SZ);                \
    descr_tmp.cpp_addr_lo += PCIE_DMA_MAX_SZ;                           \
    dma_len -= PCIE_DMA_MAX_SZ;                                         \
} while (0)

#define _ISSUE_PROC_LSO_JUMBO(_pkt, _enq_sig, _dma_sig)                 \
do {                                                                    \
    pcie_dma_set_sig(&descr_tmp, __MEID, ctx(),                         \
                     __signal_number(&_dma_sig));                       \
    dma_out.pkt##_pkt## = descr_tmp;                                    \
                                                                        \
    __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,                      \
                   NFD_IN_DATA_DMA_QUEUE,                               \
                   sig_done, &_enq_sig);                                \
    __implicit_write(&_dma_sig);                                        \
                                                                        \
    _add_to_pcie_addr(descr_tmp.__raw, PCIE_DMA_MAX_SZ);                \
    descr_tmp.cpp_addr_lo += PCIE_DMA_MAX_SZ;                           \
    dma_length -= PCIE_DMA_MAX_SZ;                                      \
    lso_dma_index += PCIE_DMA_MAX_SZ;                                   \
    amount_extra_dmaed += PCIE_DMA_MAX_SZ;                              \
} while (0)

#define _ISSUE_PROC_LSO(_pkt)                                                \
do {                                                                         \
    unsigned int lso_dma_index = 0;                                          \
    unsigned int lso_issued_index = 0;                                       \
    unsigned int mu_buf_left;                                                \
    unsigned int dma_left;                                                   \
    SIGNAL lso_dma_sig;                                                      \
    SIGNAL lso_enq_sig;                                                      \
    SIGNAL lso_hdr_dma_sig;                                                  \
    SIGNAL lso_hdr_enq_sig;                                                  \
    unsigned int lso_hdr_mask = 0;                                           \
    SIGNAL lso_journal_sig;                                                  \
    SIGNAL lso_hdr_sig;                                                      \
    __xread unsigned int mu_buf_xread;                                       \
    unsigned int i = 0;                                                      \
    __xread unsigned int lso_hdr_xread[4];                                   \
    /* this maxs out our xwrite reg or we would have used more */            \
    __xwrite unsigned int lso_hdr_xwrite[4];                                 \
    __xread unsigned int lso_hdr_chk_xread;                                  \
    unsigned int hdr_remainder;                                              \
    __addr40 void *pkt_ptr;                                                  \
    __addr40 void *hdr_pkt_ptr;                                              \
    unsigned int hdr_offset;                                                 \
    unsigned int header_to_read;                                             \
    unsigned int dma_length = 0;                                             \
    unsigned int amount_extra_dmaed = 0;                                     \
    queue_data[queue].cont = 0;                                              \
    /* if we do not have a header or all the header */                       \
    if (queue_data[queue].lso_hdr_len != tx_desc.pkt##_pkt##.l4_offset) {    \
        /* DMA in the header and save it */                                  \
        buf_addr = &lso_hdr_data[(queue << __log2(NFD_IN_MAX_LSO_HDR_SZ)) +  \
                                 queue_data[queue].lso_hdr_len];             \
        descr_tmp.cpp_addr_hi = (__ISLAND | (2 << 6));                       \
        descr_tmp.cpp_addr_lo = buf_addr & 0xFFFFFFFF;                       \
        descr_tmp.pcie_addr_hi = tx_desc.pkt##_pkt##.dma_addr_hi;            \
        descr_tmp.pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;            \
        descr_tmp.rid = queue_data[queue].rid;                               \
        descr_tmp.dma_cfg_index = NFD_IN_DATA_CFG_REG;                       \
        if (tx_desc.pkt##_pkt##.dma_len >= (tx_desc.pkt##_pkt##.l4_offset -  \
            queue_data[queue].lso_hdr_len)) {                                \
            /* we have full header in this TX descriptor */                  \
            descr_tmp.length = tx_desc.pkt##_pkt##.l4_offset -               \
            queue_data[queue].lso_hdr_len - 1;                               \
        } else {                                                             \
            /* not enough header available */                                \
            descr_tmp.length = tx_desc.pkt##_pkt##.dma_len - 1;              \
        }                                                                    \
        pcie_dma_set_sig(&descr_tmp, __MEID, ctx(),                          \
                         __signal_number(&lso_hdr_dma_sig));                 \
        dma_out.pkt##_pkt## = descr_tmp;                                     \
        __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##, NFD_IN_DATA_DMA_QUEUE,\
                       sig_done, &lso_hdr_enq_sig);                          \
        queue_data[queue].lso_hdr_len += descr_tmp.length + 1;               \
        lso_dma_index += descr_tmp.length + 1;                               \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                           \
                             NFD_IN_LSO_CNTR_T_ISSUED_LSO_HDR_READ);         \
        queue_data[queue].lso_seq_cnt = 0;                                   \
        lso_hdr_mask |= __signals(&lso_hdr_enq_sig);                         \
        lso_hdr_mask |= __signals(&lso_hdr_dma_sig);                         \
    }                                                                        \
    /* now get payload for packets */                                        \
    while (lso_dma_index < tx_desc.pkt##_pkt##.dma_len) {                    \
        /* do we do not have a pkt buffer to fill into */                    \
        if (queue_data[queue].curr_buf == 0) {                               \
            /* get a buffer. */                                              \
            while (blm_buf_alloc(&mu_buf_xread, 0) != NFD_IN_BLM_BLS) {      \
                NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                   \
                          NFD_IN_LSO_CNTR_T_ISSUED_LSO_BLM_BUF_ALLOC_FAILED);\
            }                                                                \
            queue_data[queue].curr_buf = mu_buf_xread;                       \
            queue_data[queue].lso_payload_len = 0;                           \
            queue_data[queue].offset = NFD_IN_DATA_OFFSET -                  \
                tx_desc.pkt##_pkt.offset;                                    \
                                                                             \
            /* got buffer setup where hdr and payload is to start */         \
            hdr_offset = queue_data[queue].offset;                           \
            hdr_pkt_ptr = (__addr40 void *)(((uint64_t)                      \
                    queue_data[queue].curr_buf << 11) | hdr_offset);         \
            queue_data[queue].offset = hdr_offset +                          \
                                            queue_data[queue].lso_hdr_len;   \
        }                                                                    \
        /* setup to start DMA of the payload */                              \
        /* Fill in pay load */                                               \
        /* get the length left in mu buffer */                               \
        mu_buf_left = tx_desc.pkt##_pkt##.lso -                              \
            queue_data[queue].lso_payload_len;                               \
        /* get the length left to DMA. */                                    \
        dma_left = tx_desc.pkt##_pkt##.dma_len - lso_dma_index;              \
        /* more to dma than we have buffer for */                            \
        if (mu_buf_left <= dma_left) {                                       \
            /* fill in descriptor length to dma */                           \
            dma_length = mu_buf_left;                                        \
        }                                                                    \
        /* more mu buffer than we have dma for */                            \
        else {                                                               \
            /* fill in descriptor length to dma */                           \
            dma_length = dma_left;                                           \
        }                                                                    \
        amount_extra_dmaed = 0;                                              \
        /* if the amount needed to dma is larger than max dma limit */       \
        if (dma_length > PCIE_DMA_MAX_SZ) {                                  \
            buf_addr = queue_data[queue].curr_buf;                           \
            /* set up the descr_tmp dma values */                            \
            descr_tmp.cpp_addr_hi = buf_addr >> 21;                          \
            descr_tmp.cpp_addr_lo = buf_addr << 11;                          \
            descr_tmp.cpp_addr_lo += queue_data[queue].offset;               \
            descr_tmp.pcie_addr_hi = tx_desc.pkt##_pkt##.dma_addr_hi;        \
            descr_tmp.pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;        \
            _add_to_pcie_addr(descr_tmp.__raw, lso_dma_index);               \
            descr_tmp.rid = queue_data[queue].rid;                           \
            descr_tmp.length = PCIE_DMA_MAX_SZ - 1;                          \
            /* issue a dma for max size */                                   \
            _ISSUE_PROC_LSO_JUMBO(_pkt, lso_jumbo_enq_sig,                   \
                                  lso_jumbo_dma_sig);                        \
            NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                       \
                                 NFD_IN_LSO_CNTR_T_ISSUED_LSO_JUMBO_TX_DESC);\
            __wait_for_all(&lso_jumbo_enq_sig, &lso_jumbo_dma_sig);          \
            if (dma_length > PCIE_DMA_MAX_SZ) {                              \
                NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                   \
                                 NFD_IN_LSO_CNTR_T_ISSUED_LSO_JUMBO_DBL_DMA);\
                /* issue a dma for max size */                               \
                _ISSUE_PROC_LSO_JUMBO(_pkt, lso_jumbo_enq_sig,               \
                                      lso_jumbo_dma_sig);                    \
                __wait_for_all(&lso_jumbo_enq_sig, &lso_jumbo_dma_sig);      \
            }                                                                \
        }                                                                    \
        buf_addr = queue_data[queue].curr_buf;                               \
        /* set up the descr_tmp dma values */                                \
        descr_tmp.cpp_addr_hi = buf_addr >> 21;                              \
        descr_tmp.cpp_addr_lo = buf_addr << 11;                              \
        descr_tmp.cpp_addr_lo += queue_data[queue].offset +                  \
                                 amount_extra_dmaed;                         \
        descr_tmp.pcie_addr_hi = tx_desc.pkt##_pkt##.dma_addr_hi;            \
        descr_tmp.pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;            \
        _add_to_pcie_addr(descr_tmp.__raw, lso_dma_index);                   \
        descr_tmp.rid = queue_data[queue].rid;                               \
        dma_length--;                                                        \
        descr_tmp.length = dma_length;                                       \
                                                                             \
        pcie_dma_set_sig(&descr_tmp, __MEID, ctx(),                          \
                         __signal_number(&lso_dma_sig));                     \
        dma_out.pkt##_pkt## = descr_tmp;                                     \
        __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,                       \
                       NFD_IN_DATA_DMA_QUEUE, sig_done, &lso_enq_sig);       \
        /* copy header into packet while we have fired the DMA */            \
        /* XXX Optimize this with ctm dma to mu from flowenv */              \
        if (lso_hdr_mask != 0) {                                             \
            wait_sig_mask(lso_hdr_mask);                                     \
            lso_hdr_mask = 0;                                                \
            __implicit_read(&lso_hdr_enq_sig);                               \
            __implicit_read(&lso_hdr_dma_sig);                               \
        }                                                                    \
        i = 0;                                                               \
        if (queue_data[queue].lso_payload_len == 0) {                        \
            hdr_remainder = queue_data[queue].lso_hdr_len & 0xF;             \
            header_to_read = (queue_data[queue].lso_hdr_len - hdr_remainder);\
            /* copy main part of header */                                   \
            while (i < header_to_read) {                                     \
                __mem_read64((void *)&lso_hdr_xread[0],                      \
                            (__mem void *)&lso_hdr_data[                     \
                            ((queue << __log2(NFD_IN_MAX_LSO_HDR_SZ)) +      \
                            i)],                                             \
                            sizeof(lso_hdr_xread), sizeof(lso_hdr_xread),    \
                            sig_done, &lso_hdr_sig);                         \
                __wait_for_all(&lso_hdr_sig);                                \
                lso_hdr_xwrite[0] = lso_hdr_xread[0];                        \
                lso_hdr_xwrite[1] = lso_hdr_xread[1];                        \
                lso_hdr_xwrite[2] = lso_hdr_xread[2];                        \
                lso_hdr_xwrite[3] = lso_hdr_xread[3];                        \
                __mem_write64((void *)&lso_hdr_xwrite[0],                    \
                              (__mem void *)hdr_pkt_ptr,                     \
                              sizeof(lso_hdr_xwrite), sizeof(lso_hdr_xwrite),\
                              sig_done, &lso_hdr_sig);                       \
                i += sizeof(lso_hdr_xwrite);                                 \
                hdr_pkt_ptr = (__addr40 void *)(((uint64_t)                  \
                       queue_data[queue].curr_buf << 11) | hdr_offset +  i); \
                __wait_for_all(&lso_hdr_sig);                                \
            }                                                                \
            /* copy remainder portion of header 1-15 bytes*/                 \
            if (hdr_remainder > 0) {                                         \
                __mem_read8((void *)&lso_hdr_xread[0],                       \
                           (__mem void *)&lso_hdr_data[((queue <<            \
                           __log2(NFD_IN_MAX_LSO_HDR_SZ)) + i)],             \
                           hdr_remainder, sizeof(lso_hdr_xread), sig_done,   \
                           &lso_hdr_sig);                                    \
                __wait_for_all(&lso_hdr_sig);                                \
                                                                             \
                lso_hdr_xwrite[0] = lso_hdr_xread[0];                        \
                lso_hdr_xwrite[1] = lso_hdr_xread[1];                        \
                lso_hdr_xwrite[2] = lso_hdr_xread[2];                        \
                lso_hdr_xwrite[3] = lso_hdr_xread[3];                        \
                                                                             \
                __mem_write8((void *)&lso_hdr_xwrite[0],                     \
                            (__mem void *)hdr_pkt_ptr, hdr_remainder,        \
                            sizeof(lso_hdr_xwrite), sig_done, &lso_hdr_sig); \
                __wait_for_all(&lso_hdr_sig);                                \
            }                                                                \
        }                                                                    \
            /* account for how much was read */                              \
        lso_dma_index += (descr_tmp.length + 1);                             \
        queue_data[queue].lso_payload_len += (descr_tmp.length + 1 +         \
                                             amount_extra_dmaed);            \
        queue_data[queue].offset += (descr_tmp.length + 1 +                  \
                                     amount_extra_dmaed);                    \
        __wait_for_all(&lso_enq_sig, &lso_dma_sig);                          \
        /* if we are at end of mu_buf */                                     \
        if (queue_data[queue].lso_payload_len >=  tx_desc.pkt##_pkt##.lso) { \
            pkt_ptr = (__addr40 void *)(((uint64_t)                          \
                        queue_data[queue].curr_buf << 11) |                  \
                        queue_data[queue].offset);                           \
            /* put finished mu buffer on lso_ring to notify */               \
            issued_tmp.eop = 1;                                              \
            issued_tmp.sp1 = 0;                                              \
            issued_tmp.offset = tx_desc.pkt##_pkt##.offset;                  \
            issued_tmp.buf_addr = queue_data[queue].curr_buf;                \
            issued_tmp.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];              \
            issued_tmp.l4_offset = ++queue_data[queue].lso_seq_cnt;          \
            /* if last of LSO segment set lso end flag and we have no */     \
            /* more dma data */                                              \
            if ((lso_dma_index == tx_desc.pkt##_pkt##.dma_len) &&            \
                tx_desc.pkt##_pkt##.eop) {                                   \
                issued_tmp.flags |= PCIE_DESC_TX_LSO;                        \
                NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                   \
                        NFD_IN_LSO_CNTR_T_ISSUED_LSO_END_PKT_TO_NOTIFY_RING);\
                queue_data[queue].lso_hdr_len = 0;                           \
            }                                                                \
            issued_tmp.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];              \
            issued_tmp.data_len = queue_data[queue].offset -                 \
                             (NFD_IN_DATA_OFFSET - tx_desc.pkt##_pkt.offset);\
            /* set that we do not have a curr_buf */                         \
            queue_data[queue].curr_buf = 0;                                  \
            queue_data[queue].offset = 0;                                    \
            /* send the lso pkt desc to the lso ring */                      \
            batch_out.pkt##_pkt## = issued_tmp;                              \
            __mem_ring_journal(nfd_in_issued_lso_ring_num,                   \
                            nfd_in_issued_lso_ring_addr,                     \
                            &batch_out.pkt##_pkt##,                          \
                            sizeof(struct nfd_in_issued_desc),               \
                            sizeof(struct nfd_in_issued_desc), sig_done,     \
                            &lso_journal_sig);                               \
            NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                       \
                        NFD_IN_LSO_CNTR_T_ISSUED_LSO_ALL_PKT_TO_NOTIFY_RING);\
            /* used to track how many desc we have put on ring will */       \
            /* set value in the batch out sp0 for the packet so that */      \
            /* notify how how many to read in  */                            \
            lso_issued_index++;                                              \
            __wait_for_all(&lso_journal_sig);                                \
        }                                                                    \
    } /* while */                                                            \
    if (lso_hdr_mask != 0) {                                                 \
        wait_sig_mask(lso_hdr_mask);                                         \
        lso_hdr_mask = 0;                                                    \
        __implicit_read(&lso_hdr_enq_sig);                                   \
        __implicit_read(&lso_hdr_dma_sig);                                   \
    }                                                                        \
    /* if we still have a partial buffer and it is marked as last LSO */     \
    /* segment send it */                                                    \
    if ((queue_data[queue].curr_buf) && (tx_desc.pkt##_pkt##.eop)) {         \
        /* fill the last info in. */                                         \
        issued_tmp.eop = 1;                                                  \
        issued_tmp.sp1 = 0;                                                  \
        issued_tmp.offset = tx_desc.pkt##_pkt##.offset;                      \
        issued_tmp.buf_addr = queue_data[queue].curr_buf;                    \
        issued_tmp.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];                  \
        issued_tmp.l4_offset = ++queue_data[queue].lso_seq_cnt;              \
        issued_tmp.flags |= PCIE_DESC_TX_LSO;                                \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                           \
                    NFD_IN_LSO_CNTR_T_ISSUED_LSO_END_PKT_TO_NOTIFY_RING_END);\
        issued_tmp.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];                  \
        issued_tmp.data_len = queue_data[queue].offset -                     \
            (NFD_IN_DATA_OFFSET - tx_desc.pkt##_pkt.offset);                 \
        batch_out.pkt##_pkt## = issued_tmp;                                  \
        __mem_ring_journal(nfd_in_issued_lso_ring_num,                       \
                         nfd_in_issued_lso_ring_addr,                        \
                         &batch_out.pkt##_pkt##,                             \
                         sizeof(struct nfd_in_issued_desc),                  \
                         sizeof(struct nfd_in_issued_desc), sig_done,        \
                         &lso_journal_sig);                                  \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                           \
                    NFD_IN_LSO_CNTR_T_ISSUED_LSO_ALL_PKT_TO_NOTIFY_RING_END);\
        lso_issued_index++;                                                  \
        queue_data[queue].lso_hdr_len = 0;                                   \
        queue_data[queue].curr_buf = 0;                                      \
        queue_data[queue].offset = 0;                                        \
        __wait_for_all(&lso_journal_sig);                                    \
    }                                                                        \
    NFD_IN_LSO_CNTR_CLR(nfd_in_lso_cntr_addr,                                \
                        NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_MSS);              \
    NFD_IN_LSO_CNTR_ADD(nfd_in_lso_cntr_addr,                                \
                       NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_MSS,                \
                       tx_desc.pkt##_pkt##.lso);                             \
    NFD_IN_LSO_CNTR_CLR(nfd_in_lso_cntr_addr,                                \
                        NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_L4_OFFSET);        \
    NFD_IN_LSO_CNTR_ADD(nfd_in_lso_cntr_addr,                                \
                        NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_L4_OFFSET,         \
                        tx_desc.pkt##_pkt##.l4_offset);                      \
    /* prep batch_out for LSO segment info */                                \
    issued_tmp.eop = 0;                                                      \
    issued_tmp.sp0 = lso_issued_index & 0xFF;                                \
    batch_out.pkt##_pkt## = issued_tmp;                                      \
    issued_tmp.__raw[0] = 0;                                                 \
} while(0)

#if __REVISION_MIN < __REVISION_B0
    /* THS-54 workaround, round DMA up to next 4B multiple size.
     * This workaround is incompatible with gather support. */
#if ((NFD_CFG_VF_CAP & NFP_NET_CFG_CTRL_GATHER) || \
     (NFD_CFG_PF_CAP & NFP_NET_CFG_CTRL_GATHER))
#error "NFP_NET_CFG_CTRL_GATHER not supported for A0 chips"
#endif

#define _ISSUE_PROC_A0_SUPPORT(_len)                                 \
    _len = (_len + NFD_IN_DATA_ROUND - 1) & ~(NFD_IN_DATA_ROUND -1); \

#else
#define _ISSUE_PROC_A0_SUPPORT(_len)
#endif

#define _ISSUE_PROC(_pkt, _type, _src)                                  \
do {                                                                    \
    SIGNAL lso_jumbo_dma_sig;                                           \
    SIGNAL lso_jumbo_enq_sig;                                           \
    unsigned int dma_len;                                               \
    unsigned int buf_addr;                                              \
    unsigned int curr_buf;                                              \
                                                                        \
    dma_len = tx_desc.pkt##_pkt##.dma_len;                              \
    _ISSUE_PROC_A0_SUPPORT(dma_len);                                    \
    NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                          \
                         NFD_IN_LSO_CNTR_T_ISSUED_ALL_TX_DESC);         \
                                                                        \
    if (tx_desc.pkt##_pkt##.lso) {                                      \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                      \
                             NFD_IN_LSO_CNTR_T_ISSUED_LSO_ALL_TX_DESC); \
        _ISSUE_PROC_LSO(_pkt);                                          \
        if (_type == NFD_IN_DATA_EVENT_TYPE) {                          \
            descr_tmp.cpp_addr_hi = 0;                                  \
            descr_tmp.cpp_addr_lo = 0;                                  \
            descr_tmp.pcie_addr_hi = 0;                                 \
            descr_tmp.pcie_addr_lo = 0;                                 \
            /* mode_sel and dma_mode set replaced pcie_dma_set_event */ \
            descr_tmp.mode_sel = NFP_PCIE_DMA_CMD_DMA_MODE_2;           \
            descr_tmp.dma_mode = (((_type & 0xF) << 12) |               \
                                   (_src & 0xFFF));                     \
            descr_tmp.length = 0;                                       \
                                                                        \
            descr_tmp.dma_cfg_index = NFD_IN_DATA_CFG_REG_SIG_ONLY;     \
            dma_out.pkt##_pkt = descr_tmp;                              \
            descr_tmp.dma_cfg_index = NFD_IN_DATA_CFG_REG;              \
            __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt,                \
                           NFD_IN_DATA_DMA_QUEUE,                       \
                           sig_done, &dma_sig##_pkt);                   \
        } else {                                                        \
            wait_msk &= ~__signals(&dma_sig##_pkt##);                   \
        }                                                               \
        _LSO_TX_DESC_TYPE_CNTR(_pkt);                                   \
    } else if (!queue_data[queue].up) {                                 \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                      \
                             NFD_IN_LSO_CNTR_T_ISSUED_NOT_Q_UP_TX_DESC);\
        /* Handle down queues off the fast path. */                     \
        /* As all packets in a batch come from one queue and are */     \
        /* processed without swapping, all the packets in the batch */  \
        /* will receive the same treatment.  The batch will still */    \
        /* use its slot in the DMA sequence numbers and the */          \
        /* nfd_in_issued_ring. */                                       \
                                                                        \
        /* Setting "cont" when the queue is down ensures */             \
        /* that this processing happens off the fast path. */           \
                                                                        \
        /* Flag the packet for notify. */                               \
        /* Zero EOP and num_batch so that the notify block will not */  \
        /* produce output to the work queues, and will have no */       \
        /* effect on the queue controller queue. */                     \
        /* NB: the rest of the message will be stale. */                \
        issued_tmp.eop = 0;                                             \
        issued_tmp.offset = 0;                                          \
        issued_tmp.sp0 = 0;                                             \
        issued_tmp.num_batch = 0;                                       \
        issued_tmp.sp1 = 0;                                             \
        batch_out.pkt##_pkt##.__raw[0] = issued_tmp.__raw[0];           \
                                                                        \
        /* Handle the DMA sequence numbers for the batch */             \
        if (_type == NFD_IN_DATA_EVENT_TYPE) {                          \
            descr_tmp.cpp_addr_hi = 0;                                  \
            descr_tmp.cpp_addr_lo = 0;                                  \
            descr_tmp.pcie_addr_hi = 0;                                 \
            descr_tmp.pcie_addr_lo = 0;                                 \
            /* mode_sel and dma_mode set replaced pcie_dma_set_event */ \
            descr_tmp.mode_sel = NFP_PCIE_DMA_CMD_DMA_MODE_2;           \
            descr_tmp.dma_mode = (((NFD_IN_DATA_EVENT_TYPE & 0xF) << 12)\
                                    | (data_dma_seq_issued & 0xFFF));   \
            descr_tmp.length = 0;                                       \
                                                                        \
            descr_tmp.dma_cfg_index = NFD_IN_DATA_CFG_REG_SIG_ONLY;     \
            dma_out.pkt##_pkt = descr_tmp;                              \
            descr_tmp.dma_cfg_index = NFD_IN_DATA_CFG_REG;              \
            __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt,                \
                           NFD_IN_DATA_DMA_QUEUE,                       \
                           sig_done, &dma_sig##_pkt);                   \
                                                                        \
        } else {                                                        \
            wait_msk &= ~__signals(&dma_sig##_pkt##);                   \
        }                                                               \
    } else {                                                            \
        if (tx_desc.pkt##_pkt##.eop && !queue_data[queue].cont) {       \
            NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                  \
                         NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_CONT_TX_DESC);\
            /* Fast path, use buf_store data */                         \
            __critical_path();                                          \
            /* Set NFP buffer address and offset */                     \
            buf_addr = precache_bufs_use();                             \
            issued_tmp.buf_addr = buf_addr;                             \
            _ISSUE_PROC_MU_CHK(buf_addr);                               \
            descr_tmp.cpp_addr_hi = buf_addr>>21;                       \
            descr_tmp.cpp_addr_lo = buf_addr<<11;                       \
            descr_tmp.cpp_addr_lo += NFD_IN_DATA_OFFSET;                \
            descr_tmp.cpp_addr_lo -= tx_desc.pkt##_pkt##.offset;        \
        } else {                                                        \
            if (!queue_data[queue].cont) {                              \
                /* Initialize continuation data */                      \
                                                                        \
                /* XXX check efficiency */                              \
                curr_buf = precache_bufs_use();                         \
                _ISSUE_PROC_MU_CHK(curr_buf);                           \
                queue_data[queue].cont = 1;                             \
                queue_data[queue].offset = NFD_IN_DATA_OFFSET;          \
                queue_data[queue].offset -= tx_desc.pkt##_pkt##.offset; \
                queue_data[queue].curr_buf = curr_buf;                  \
            }                                                           \
            curr_buf = queue_data[queue].curr_buf;                      \
                                                                        \
            /* Use continuation data */                                 \
            descr_tmp.cpp_addr_hi = curr_buf>>21;                       \
            descr_tmp.cpp_addr_lo = curr_buf<<11;                       \
            descr_tmp.cpp_addr_lo += queue_data[queue].offset;          \
            queue_data[queue].offset += dma_len;                        \
                                                                        \
            issued_tmp.buf_addr = curr_buf;                             \
                                                                        \
            if (tx_desc.pkt##_pkt##.eop) {                              \
                NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,              \
                          NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_EOP_TX_DESC);\
                /* Clear continuation data on EOP */                    \
                                                                        \
                /* XXX check this is done in two cycles */              \
                queue_data[queue].cont = 0;                             \
                queue_data[queue].sp1 = 0;                              \
                queue_data[queue].curr_buf = 0;                         \
                queue_data[queue].offset = 0;                           \
            }                                                           \
        }                                                               \
                                                                        \
        /* Set up notify message */                                     \
        /* NB: EOP is required for all packets */                       \
        /*     q_num is must be set on pkt0 */                          \
        /*     notify technically doesn't use the rest unless */        \
        /*     EOP is set */                                            \
        issued_tmp.eop = tx_desc.pkt##_pkt##.eop;                       \
        issued_tmp.offset = tx_desc.pkt##_pkt##.offset;                 \
                                                                        \
        /* Apply a standard "recipe" to complete the DMA issue */       \
        batch_out.pkt##_pkt## = issued_tmp;                             \
        batch_out.pkt##_pkt##.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];  \
        batch_out.pkt##_pkt##.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];  \
                                                                        \
        descr_tmp.pcie_addr_hi = tx_desc.pkt##_pkt##.dma_addr_hi;       \
        descr_tmp.pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;       \
                                                                        \
        descr_tmp.rid = queue_data[queue].rid;                          \
                                                                        \
        if (dma_len > PCIE_DMA_MAX_SZ) {                                \
            /* data_dma_seq_issued was pre-incremented once we could */ \
            /* process batch.  Since we are going to swap, we */        \
            /* decrement it temporarily to ensure */                    \
            /* precache_bufs_compute_seq_safe will give a pessimistic */\
            /* safe count. */                                           \
            data_dma_seq_issued--;                                      \
                                                                        \
            /* Always DMA PCIE_DMA_MAX_SZ segments for jumbos */        \
            descr_tmp.length = PCIE_DMA_MAX_SZ - 1;                     \
                                                                        \
            /* Handle first PCIE_DMA_MAX_SZ */                          \
            _ISSUE_PROC_JUMBO(_pkt, lso_jumbo_enq_sig,                  \
                              lso_jumbo_dma_sig);                       \
            __wait_for_all(&lso_jumbo_enq_sig, &lso_jumbo_dma_sig);     \
                                                                        \
            if (dma_len > PCIE_DMA_MAX_SZ) {                            \
                /* Handle second PCIE_DMA_MAX_SZ */                     \
                _ISSUE_PROC_JUMBO(_pkt, lso_jumbo_enq_sig,              \
                                  lso_jumbo_dma_sig);                   \
                __wait_for_all(&lso_jumbo_enq_sig, &lso_jumbo_dma_sig); \
            }                                                           \
            /* Re-increment data_dma_seq_issued */                      \
            data_dma_seq_issued++;                                      \
        }                                                               \
                                                                        \
        /* Issue final DMA for the packet */                            \
        /* mode_sel and dma_mode set replaced pcie_dma_set_event */     \
        descr_tmp.mode_sel = NFP_PCIE_DMA_CMD_DMA_MODE_2;               \
        descr_tmp.dma_mode = (((_type & 0xF) << 12) | (_src & 0xFFF));  \
        descr_tmp.length = dma_len - 1;                                 \
        dma_out.pkt##_pkt## = descr_tmp;                                \
                                                                        \
        __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,                  \
                       NFD_IN_DATA_DMA_QUEUE,                           \
                       sig_done, &dma_sig##_pkt##);                     \
                                                                        \
    }                                                                   \
} while (0)


#define _ISSUE_CLR(_pkt)                                                \
do {                                                                    \
    /* Do minimal clean up so local signalling works and */             \
    /* notify block ignores the message */                              \
    batch_out.pkt##_pkt##.__raw[0] = 0;                                 \
    wait_msk &= ~__signals(&dma_sig##_pkt##);                           \
} while (0)


/**
 * Fetch batch messages from the NN ring and process them, issuing up to
 * PCI_IN_MAX_BATCH_SZ DMAs, and placing a batch of messages onto the
 * "nfd_in_issued_ring".  Messages are only dequeued from the NN ring when the
 * "gather_dma_seq_compl" sequence number indicates that it is safe to do so.
 * The message processing stalls until "data_dma_seq_safe" and
 * "data_dma_seq_issued" indicate that it is safe to continue.  Two ordering
 * stages ensure that packet DMAs are issued in sequence.
 */
__forceinline void
issue_dma()
{
    static __xread struct _tx_desc_batch tx_desc;
    __cls void *desc_ring_addr;
    unsigned int desc_ring_off;

    __gpr struct nfd_in_issued_desc issued_tmp;

    struct nfd_in_batch_desc batch;
    unsigned int queue;
    unsigned int num;

    reorder_test_swap(&desc_order_sig);

    /* Check "DMA" completed and we can read the batch
     * If so, the NN ring MUST have a batch descriptor for us
     * NB: only one ctx can execute this at any given time */
    while (gather_dma_seq_compl == gather_dma_seq_serv) {
        ctx_swap(); /* Yield while waiting for work */
    }

    reorder_done_opt(&next_ctx, &desc_order_sig);

#ifdef NFD_ERROR_CHCECKING
    if (nn_ring_empty()) {
        halt();          /* A serious error has occurred */
    }
#endif
    /*
     * Increment gather_dma_seq_serv upfront to avoid ambiguity
     * about sequence number zero
     */
    gather_dma_seq_serv++;

    /* Read the batch */
    batch.__raw = nn_ring_get();
    desc_ring_off = ((gather_dma_seq_serv * sizeof(tx_desc)) &
                     (NFD_IN_DESC_RING_SZ - 1));
    desc_ring_addr = (__cls void *) (desc_ring_base | desc_ring_off);
    __cls_read(&tx_desc, desc_ring_addr, sizeof tx_desc, sizeof tx_desc,
               sig_done, &tx_desc_sig);


    /* Start of dma_order_sig reorder stage */
    __asm {
        ctx_arb[--], defer[1];
        local_csr_wr[local_csr_active_ctx_wakeup_events, wait_msk];
    }

    wait_msk = __signals(&dma_sig0, &dma_sig1, &dma_sig2, &dma_sig3,
                         &tx_desc_sig, &msg_sig, &dma_order_sig);
    __implicit_read(&dma_sig0);
    __implicit_read(&dma_sig1);
    __implicit_read(&dma_sig2);
    __implicit_read(&dma_sig3);
    __implicit_read(&msg_sig);
    __implicit_read(&tx_desc_sig);
    __implicit_read(&dma_order_sig);

    while ((int)(data_dma_seq_issued - data_dma_seq_safe) >= 0) {
        /* We can't process this batch yet.
         * Swap then recompute seq_safe.
         * NB: only one ctx can execute this at any given time */
        ctx_swap();
        precache_bufs_compute_seq_safe();
    }

    /* We can start to process this batch but may need to issue multiple
     * DMAs and swap for large packets, so don't let other batches start
     * just yet. */

    queue = batch.queue;
    num = batch.num;
    data_dma_seq_issued++;

    issued_tmp.sp0 = 0;
    issued_tmp.num_batch = num;   /* Only needed in pkt0 */
    issued_tmp.sp1 = 0;
    issued_tmp.q_num = queue;

    /* Maybe add "full" bit */
    if (num == 4) {
        /* Full batches are the critical path */
        /* XXX maybe tricks with an extra nfd_in_dma_state
         * struct would convince nfcc to use one set LM index? */
        __critical_path();
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(2, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(3, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);
    } else if (num == 3) {
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(2, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(3);
    } else if (num == 2) {
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(2);
        _ISSUE_CLR(3);
    } else if (num == 1) {
        _ISSUE_PROC(0, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(1);
        _ISSUE_CLR(2);
        _ISSUE_CLR(3);
    } else {
        local_csr_write(local_csr_mailbox_0, 0xDEEDBEEF);
        halt();
    }

    /* We have finished processing the batch, let the next continue */
    reorder_done_opt(&next_ctx, &dma_order_sig);

    /* XXX THS-50 workaround */
    /* cls_ring_put(NFD_IN_ISSUED_RING_NUM, &batch_out, sizeof batch_out, */
    /*              &msg_sig); */
    ctm_ring_put(NFD_IN_ISSUED_RING_NUM, &batch_out, sizeof batch_out, &msg_sig);
}
