/*
 * Copyright (C) 2014-2015 Netronome Systems, Inc.  All rights reserved.
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
#include <std/reg_utils.h>

#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_in.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_internal.h>

#include <vnic/utils/cls_ring.h>
#include <vnic/utils/ctm_ring.h> /* XXX THS-50 workaround */
#include <vnic/utils/ordering.h>
#include <vnic/utils/qc.h>

#ifndef PCI_IN_ISSUE_DMA_IDX
#warning "PCI_IN_ISSUE_DMA_IDX not defined.  Defaulting to 0.  Make sure there is only one instance"
#define PCI_IN_ISSUE_DMA_IDX 0
#endif

struct _tx_desc_batch {
    struct nfd_in_tx_desc pkt0;
    struct nfd_in_tx_desc pkt1;
    struct nfd_in_tx_desc pkt2;
    struct nfd_in_tx_desc pkt3;
    struct nfd_in_tx_desc pkt4;
    struct nfd_in_tx_desc pkt5;
    struct nfd_in_tx_desc pkt6;
    struct nfd_in_tx_desc pkt7;
};

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

struct _dma_desc_batch {
    struct nfp_pcie_dma_cmd pkt0;
    struct nfp_pcie_dma_cmd pkt1;
    struct nfp_pcie_dma_cmd pkt2;
    struct nfp_pcie_dma_cmd pkt3;
    struct nfp_pcie_dma_cmd pkt4;
    struct nfp_pcie_dma_cmd pkt5;
    struct nfp_pcie_dma_cmd pkt6;
    struct nfp_pcie_dma_cmd pkt7;
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

/* Sequence number declarations */
__shared __gpr unsigned int gather_dma_seq_compl = 0;
__shared __gpr unsigned int gather_dma_seq_serv = 0;

__shared __gpr unsigned int data_dma_seq_issued = 0;
extern __shared __gpr unsigned int data_dma_seq_safe;

__shared __gpr unsigned int jumbo_dma_seq_issued = 0;
__shared __gpr unsigned int jumbo_dma_seq_compl = 0;


/* Signals and transfer registers for managing
 * gather_dma_seq_compl updates*/
__visible volatile __xread unsigned int nfd_in_gather_compl_refl_in;
__visible volatile SIGNAL nfd_in_gather_compl_refl_sig;

/* DMA descriptor template */
static __gpr unsigned int cpp_hi_no_sig_part;
static __gpr unsigned int cpp_hi_event_part;
static __gpr unsigned int pcie_hi_word;


/* Output transfer registers */
static __xwrite struct _dma_desc_batch dma_out;
static __xwrite struct _issued_pkt_batch batch_out;

/* Signalling */
static SIGNAL tx_desc_sig, msg_sig0, desc_order_sig, dma_order_sig;
static SIGNAL last_of_batch_dma_sig;
static SIGNAL msg_sig1;
static SIGNAL batch_sig;
static SIGNAL_MASK wait_msk;

unsigned int next_ctx;

/* CLS ring of batch information from the gather() block */
CLS_RING_DECL;
ASM(.alloc_resource nfd_in_batch_ring0_num cls_rings+NFD_IN_BATCH_RING0_NUM \
    island 1 1);
ASM(.alloc_resource nfd_in_batch_ring1_num cls_rings+NFD_IN_BATCH_RING1_NUM \
    island 1 1);

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
        queue_data[bmsk_queue].zero = 0;
        queue_data[bmsk_queue].rid = 0;
        if (cfg_msg->vnic != NFD_MAX_VFS) {
            queue_data[bmsk_queue].rid = cfg_msg->vnic + NFD_CFG_VF_OFFSET;
        }
        queue_data[bmsk_queue].cont = 0;
        queue_data[bmsk_queue].up = 1;
        queue_data[bmsk_queue].curr_buf = 0;
        queue_data[bmsk_queue].offset = 0;
        queue_data[bmsk_queue].sp1 = 0;

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
        queue_data[bmsk_queue].zero = 0;
        /* Leave RID configured after first set */
        /* "cont" is used as part of the "up" signalling,
         * to move the "up" test off the fast path. */
        queue_data[bmsk_queue].cont = 1;
        queue_data[bmsk_queue].up = 0;
        queue_data[bmsk_queue].curr_buf = 0;
        queue_data[bmsk_queue].offset = 0;
        queue_data[bmsk_queue].sp1 = 0;

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

    /* Prepare partial descriptors for the DMA CPP hi word */
    cpp_hi_no_sig_part = (NFP_PCIE_DMA_CMD_MODE_SEL(0) |
                          NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN) |
                          NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG));
    cpp_hi_event_part = dma_seqn_init_event(NFD_IN_DATA_EVENT_TYPE, 1);
    cpp_hi_event_part |= (NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN) |
                          NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG));

    /* Prepare static portion for the DMA pcie hi word */
    pcie_hi_word = (NFP_PCIE_DMA_CMD_RID_OVERRIDE |
                    NFP_PCIE_DMA_CMD_TRANS_CLASS(NFD_IN_DATA_DMA_TRANS_CLASS));

    /* wait_msk initially only needs batch_sig, tx_desc_sig and dma_order_sig
     * No DMAs or messages have been issued at this stage */
    wait_msk = __signals(&batch_sig, &tx_desc_sig, &dma_order_sig);
    next_ctx = reorder_get_next_ctx(NFD_IN_ISSUE_START_CTX,
                                    NFD_IN_ISSUE_END_CTX);
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

/* performance hit if NFD_IN_ISSUE_DMA_DBG_CHKS is enabled in makefile.
 * The MU_PTR check done in notify by default which hides the cost of the
 * check. */
#ifdef NFD_IN_ISSUE_DMA_DBG_CHKS
#define _ISSUE_PROC_MU_CHK(_val)                                        \
    if ((_val & NFD_MU_PTR_DBG_MSK) == 0) {                             \
        /* Write the error we read to Mailboxes for debug purposes */   \
        local_csr_write(local_csr_mailbox_0,                            \
                        NFD_IN_ISSUE_DMA_MU_PTR_INVALID);               \
        local_csr_write(local_csr_mailbox_1, _val);                     \
                                                                        \
        halt();                                                         \
    }
#else
#define _ISSUE_PROC_MU_CHK(_val)
#endif


/* This macro issues a DMA for a NFD_IN_DMA_SPLIT_LEN chunk of the
 * current packet.  It assumes that the descr_tmp has been mostly
 * configured (RID, addresses, etc), but sets the DMA event and length
 * info.  It updates the addresses and remaining dma_len before exiting.
 *
 * DMAs are tracked from a separate sequence space (jumbo_dma_seq_issued
 * and jumbo_dma_seq_compl).
 */
#define _ISSUE_PROC_JUMBO(_pkt, _buf)                                   \
do {                                                                    \
    int jumbo_seq_test;                                                 \
                                                                        \
    /* data_dma_seq_issued was pre-incremented once we could */         \
    /* process batch.  Since we are going to swap, we */                \
    /* decrement it temporarily to ensure */                            \
    /* precache_bufs_compute_seq_safe will give a pessimistic */        \
    /* safe count. */                                                   \
    data_dma_seq_issued--;                                              \
                                                                        \
    /* Take a jumbo frame sequence number and */                        \
    /* check it is safe to use */                                       \
    jumbo_dma_seq_issued++;                                             \
                                                                        \
    jumbo_seq_test = (NFD_IN_JUMBO_MAX_IN_FLIGHT - jumbo_dma_seq_issued); \
    while ((int) (jumbo_seq_test + jumbo_dma_seq_compl) <= 0) {         \
        /* It is safe to simply swap for CTX0 */                        \
        /* to advance jumbo_dma_seq_compl. */                           \
        ctx_swap();                                                     \
    }                                                                   \
                                                                        \
    dma_out.pkt##_pkt##.__raw[0] = cpp_addr_lo + NFD_IN_DATA_OFFSET;    \
                                                                        \
    cpp_hi_word = dma_seqn_init_event(NFD_IN_JUMBO_EVENT_TYPE, 1);      \
    cpp_hi_word = dma_seqn_set_seqn(cpp_hi_word, jumbo_dma_seq_issued); \
    cpp_hi_word |= NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN);   \
    cpp_hi_word |=                                                      \
        NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG);            \
    dma_out.pkt##_pkt##.__raw[1] = cpp_hi_word | (_buf >> 21);          \
                                                                        \
    dma_out.pkt##_pkt##.__raw[2] = pcie_addr_lo;                        \
    dma_out.pkt##_pkt##.__raw[3] = (pcie_hi_word |                      \
                                    NFP_PCIE_DMA_CMD_LENGTH(dma_len - 1)); \
                                                                        \
    pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt, NFD_IN_DATA_DMA_QUEUE);  \
                                                                        \
    pcie_addr_lo += NFD_IN_DMA_SPLIT_LEN;                               \
    cpp_addr_lo += NFD_IN_DMA_SPLIT_LEN;                                \
    dma_len -= NFD_IN_DMA_SPLIT_LEN;                                    \
                                                                        \
    /* Re-increment data_dma_seq_issued */                              \
    data_dma_seq_issued++;                                              \
} while (0)


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
    unsigned int dma_len;                                               \
    __gpr unsigned int buf_addr;                                        \
    __gpr unsigned int curr_buf;                                        \
    unsigned int cpp_addr_lo;                                           \
    unsigned int pcie_addr_lo;                                          \
    unsigned int cpp_hi_word;                                           \
                                                                        \
    dma_len = tx_desc.pkt##_pkt##.dma_len;                              \
    _ISSUE_PROC_A0_SUPPORT(dma_len);                                    \
                                                                        \
    if (tx_desc.pkt##_pkt##.eop && !queue_data[queue].cont) {           \
        /* Fast path, use buf_store data */                             \
        __critical_path();                                              \
                                                                        \
        /* Set NFP buffer address and offset */                         \
        buf_addr = precache_bufs_use();                                 \
        _ISSUE_PROC_MU_CHK(buf_addr);                                   \
        cpp_addr_lo = buf_addr << 11;                                   \
        cpp_addr_lo -= tx_desc.pkt##_pkt##.offset;                      \
                                                                        \
        pcie_hi_word |= NFP_PCIE_DMA_CMD_PCIE_ADDR_HI(                  \
                                      tx_desc.pkt##_pkt##.dma_addr_hi); \
        pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;                 \
                                                                        \
        /* Check for and handle large (jumbo) packets  */               \
        while (dma_len > NFD_IN_DMA_SPLIT_THRESH) {                     \
            _ISSUE_PROC_JUMBO(_pkt, buf_addr);                          \
        }                                                               \
                                                                        \
        /* Issue final DMA for the packet */                            \
        dma_out.pkt##_pkt##.__raw[0] = cpp_addr_lo + NFD_IN_DATA_OFFSET; \
        dma_out.pkt##_pkt##.__raw[2] = pcie_addr_lo;                    \
        dma_out.pkt##_pkt##.__raw[3] = (pcie_hi_word |                  \
                                 NFP_PCIE_DMA_CMD_LENGTH(dma_len - 1)); \
                                                                        \
        if (_type == NFD_IN_DATA_IGN_EVENT_TYPE) {                      \
            dma_out.pkt##_pkt##.__raw[1] = (cpp_hi_no_sig_part |        \
                                            (buf_addr >> 21));          \
            pcie_dma_enq_no_sig(PCIE_ISL, &dma_out.pkt##_pkt##,         \
                           NFD_IN_DATA_DMA_QUEUE);                      \
        } else {                                                        \
            cpp_hi_word = dma_seqn_set_seqn(cpp_hi_event_part, _src);   \
            dma_out.pkt##_pkt##.__raw[1] = cpp_hi_word | (buf_addr >> 21); \
            __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,              \
                           NFD_IN_DATA_DMA_QUEUE,                       \
                           sig_done, &last_of_batch_dma_sig);           \
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
        batch_out.pkt##_pkt##.__raw[1] = buf_addr;                      \
        batch_out.pkt##_pkt##.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];  \
        batch_out.pkt##_pkt##.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];  \
                                                                        \
                                                                        \
    } else if (!queue_data[queue].up) {                                 \
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
        /* XXX add _last_pkt parameter to avoid race? */                \
        if (_type == NFD_IN_DATA_EVENT_TYPE) {                          \
            pcie_hi_word = NFP_PCIE_DMA_CMD_PCIE_ADDR_HI(0);            \
                                                                        \
            cpp_hi_word = dma_seqn_init_event(NFD_IN_DATA_EVENT_TYPE, 1); \
            cpp_hi_word = dma_seqn_set_seqn(cpp_hi_word, _src);         \
            cpp_hi_word |= NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN); \
            cpp_hi_word |=                                              \
                NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG_SIG_ONLY); \
                                                                        \
            dma_out.pkt##_pkt##.__raw[0] = 0;                           \
            dma_out.pkt##_pkt##.__raw[1] = cpp_hi_word;                 \
            dma_out.pkt##_pkt##.__raw[2] = 0;                           \
            dma_out.pkt##_pkt##.__raw[3] = pcie_hi_word |               \
                                           NFP_PCIE_DMA_CMD_LENGTH(0);  \
            cpp_hi_word |= NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG); \
            __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt,                \
                           NFD_IN_DATA_DMA_QUEUE,                       \
                           sig_done, &last_of_batch_dma_sig);           \
                                                                        \
        }                                                               \
                                                                        \
    } else {                                                            \
        if (!queue_data[queue].cont) {                                  \
            /* Initialise continuation data */                          \
                                                                        \
            /* XXX check efficiency */                                  \
            curr_buf = precache_bufs_use();                             \
            _ISSUE_PROC_MU_CHK(curr_buf);                               \
            queue_data[queue].cont = 1;                                 \
            queue_data[queue].offset = -tx_desc.pkt##_pkt##.offset;     \
            queue_data[queue].curr_buf = curr_buf;                      \
        }                                                               \
        curr_buf = queue_data[queue].curr_buf;                          \
                                                                        \
        /* Use continuation data */                                     \
        cpp_addr_lo = curr_buf << 11;                                   \
        cpp_addr_lo += queue_data[queue].offset;                        \
        queue_data[queue].offset += dma_len;                            \
                                                                        \
        pcie_hi_word |= NFP_PCIE_DMA_CMD_PCIE_ADDR_HI(                  \
                                      tx_desc.pkt##_pkt##.dma_addr_hi); \
        pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;                 \
                                                                        \
        /* Check for and handle large (jumbo) packets  */               \
        while (dma_len > NFD_IN_DMA_SPLIT_THRESH) {                     \
            _ISSUE_PROC_JUMBO(_pkt, curr_buf);                          \
        }                                                               \
                                                                        \
        /* Issue final DMA for the packet */                            \
        dma_out.pkt##_pkt##.__raw[0] = cpp_addr_lo + NFD_IN_DATA_OFFSET; \
        dma_out.pkt##_pkt##.__raw[2] = pcie_addr_lo;                    \
        dma_out.pkt##_pkt##.__raw[3] = (pcie_hi_word |                  \
                                 NFP_PCIE_DMA_CMD_LENGTH(dma_len - 1)); \
                                                                        \
        if (_type == NFD_IN_DATA_IGN_EVENT_TYPE) {                      \
            dma_out.pkt##_pkt##.__raw[1] = (cpp_hi_no_sig_part |        \
                                            (curr_buf >> 21));          \
            pcie_dma_enq_no_sig(PCIE_ISL, &dma_out.pkt##_pkt##,         \
                           NFD_IN_DATA_DMA_QUEUE);                      \
        } else {                                                        \
            cpp_hi_word = dma_seqn_set_seqn(cpp_hi_event_part, _src);   \
            dma_out.pkt##_pkt##.__raw[1] = cpp_hi_word | (curr_buf >> 21); \
             __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,             \
                       NFD_IN_DATA_DMA_QUEUE,                           \
                       sig_done, &last_of_batch_dma_sig);               \
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
        batch_out.pkt##_pkt##.__raw[1] = curr_buf;                      \
        batch_out.pkt##_pkt##.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];  \
        batch_out.pkt##_pkt##.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];  \
                                                                        \
        /* Clear continuation data on EOP */                            \
        if (tx_desc.pkt##_pkt##.eop) {                                  \
            /* XXX check this is done in two cycles */                  \
            queue_data[queue].cont = 0;                                 \
            queue_data[queue].curr_buf = 0;                             \
            queue_data[queue].offset = 0;                               \
        }                                                               \
                                                                        \
    }                                                                   \
                                                                        \
} while (0)


#define _ISSUE_CLR(_pkt)                                                \
do {                                                                    \
    /* Do minimal clean up so local signalling works and */             \
    /* notify block ignores the message */                              \
    batch_out.pkt##_pkt##.__raw[0] = 0;                                 \
} while (0)


/**
 * Fetch batch messages from the CLS ring and process them, issuing up to
 * PCI_IN_MAX_BATCH_SZ DMAs, and placing a batch of messages onto the
 * "nfd_in_issued_ring".  Messages are only dequeued from the CLS ring when the
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

    static __xread struct nfd_in_batch_desc batch;
    unsigned int queue;
    unsigned int num;

    reorder_test_swap(&desc_order_sig);

    /* Check "DMA" completed and we can read the batch
     * If so, the CLS ring MUST have a batch descriptor for us
     * NB: only one ctx can execute this at any given time */
    while (gather_dma_seq_compl == gather_dma_seq_serv) {
        ctx_swap(); /* Yield while waiting for work */
    }

    reorder_done_opt(&next_ctx, &desc_order_sig);

    /*
     * Increment gather_dma_seq_serv upfront to avoid ambiguity
     * about sequence number zero
     */
    gather_dma_seq_serv++;

    /* Read the batch descriptor */
    cls_ring_get(NFD_IN_BATCH_RING0_NUM + PCI_IN_ISSUE_DMA_IDX,
                 &batch, sizeof(batch), &batch_sig);

    /* Read the batch */
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

    wait_msk = __signals(&last_of_batch_dma_sig, &batch_sig, &tx_desc_sig,
                         &msg_sig0, &msg_sig1, &dma_order_sig);
    __implicit_read(&last_of_batch_dma_sig);
    __implicit_read(&msg_sig1);
    __implicit_read(&msg_sig0);
    __implicit_read(&batch_sig);
    __implicit_read(&tx_desc_sig);
    __implicit_read(&dma_order_sig);
    __implicit_read(&dma_out, sizeof dma_out);

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
    pcie_hi_word |= NFP_PCIE_DMA_CMD_RID(queue_data[queue].rid);

    /* Maybe add "full" bit */
    if (num == 8) {
        /* Full batches are the critical path */
        /* XXX maybe tricks with an extra nfd_in_dma_state
         * struct would convince nfcc to use one set LM index? */
        __critical_path();
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(2, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(3, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(4, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(5, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(6, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(7, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);
    } else if (num == 4) {
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(2, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(3, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(4);
        _ISSUE_CLR(5);
        _ISSUE_CLR(6);
        _ISSUE_CLR(7);
    } else if (num == 3) {
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(2, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(3);
        _ISSUE_CLR(4);
        _ISSUE_CLR(5);
        _ISSUE_CLR(6);
        _ISSUE_CLR(7);
    } else if (num == 2) {
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(2);
        _ISSUE_CLR(3);
        _ISSUE_CLR(4);
        _ISSUE_CLR(5);
        _ISSUE_CLR(6);
        _ISSUE_CLR(7);
    } else if (num == 1) {
        _ISSUE_PROC(0, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(1);
        _ISSUE_CLR(2);
        _ISSUE_CLR(3);
        _ISSUE_CLR(4);
        _ISSUE_CLR(5);
        _ISSUE_CLR(6);
        _ISSUE_CLR(7);
    } else {
        local_csr_write(local_csr_mailbox_0, 0xdeadbeef);
        local_csr_write(local_csr_mailbox_1, batch.__raw);
        halt();
    }

    /* We have finished processing the batch, let the next continue */
    reorder_done_opt(&next_ctx, &dma_order_sig);

    /* XXX THS-50 workaround */
    /* cls_ring_put(NFD_IN_ISSUED_RING_NUM, &batch_out, sizeof batch_out, */
    /*              &msg_sig); */
    ctm_ring_put(NFD_IN_ISSUED_RING_NUM, &batch_out.pkt0,
                 (sizeof(struct nfd_in_issued_desc) * 4), &msg_sig0);
    ctm_ring_put(NFD_IN_ISSUED_RING_NUM, &batch_out.pkt4,
                 (sizeof(struct nfd_in_issued_desc) * 4), &msg_sig1);
}
