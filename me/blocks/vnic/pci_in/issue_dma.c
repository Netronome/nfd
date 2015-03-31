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
#include <std/reg_utils.h>

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

/* Sequence number declarations */
extern __shared __gpr unsigned int gather_dma_seq_compl;
__shared __gpr unsigned int gather_dma_seq_serv = 0;

__shared __gpr unsigned int data_dma_seq_issued = 0;
extern __shared __gpr unsigned int data_dma_seq_safe;

/* DMA descriptor template */
static __gpr struct nfp_pcie_dma_cmd descr_tmp;

/* Output transfer registers */
static __xwrite struct _dma_desc_batch dma_out;
static __xwrite struct _issued_pkt_batch batch_out;

/* Signalling */
static SIGNAL tx_desc_sig, msg_sig, desc_order_sig, dma_order_sig;
static SIGNAL dma_sig0, dma_sig1, dma_sig2, dma_sig3;
static SIGNAL_MASK wait_msk;

static SIGNAL jumbo0, jumbo1;
static SIGNAL_MASK jumbo_msk = 0;
unsigned int next_ctx;

/* Configure the NN ring */
NN_RING_ZERO_PTRS;
NN_RING_EMPTY_ASSERT_SET(0);


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
        queue_data[bmsk_queue].rid = 0;
        if (cfg_msg->vnic != NFD_MAX_VFS) {
            queue_data[bmsk_queue].rid = cfg_msg->vnic + NFD_CFG_VF_OFFSET;
        }
        queue_data[bmsk_queue].cont = 0;
        queue_data[bmsk_queue].up = 1;
        queue_data[bmsk_queue].curr_buf = 0;
        queue_data[bmsk_queue].offset = 0;
        queue_data[bmsk_queue].sp2 = 0;

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
        /* Leave RID configured after first set */
        /* "cont" is used as part of the "up" signalling,
         * to move the "up" test off the fast path. */
        queue_data[bmsk_queue].cont = 1;
        queue_data[bmsk_queue].up = 0;
        queue_data[bmsk_queue].curr_buf = 0;
        queue_data[bmsk_queue].offset = 0;
        queue_data[bmsk_queue].sp2 = 0;

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
}

/* XXX temporarily enable _ISSUE_PROC_MU_CHK even without debug checks.
 * This gives us extra protection of the CTM counters while PCI.OUT is not
 * double checking credits. */
#ifndef NFD_MU_PTR_DBG_MSK
#define NFD_MU_PTR_DBG_MSK 0x1f000000
#endif

/* #ifdef NFD_VNIC_DBG_CHKS */
#define _ISSUE_PROC_MU_CHK(_val)                                        \
    if ((_val & NFD_MU_PTR_DBG_MSK) == 0) {                             \
        halt();                                                         \
    }
/* #else */
/* #define _ISSUE_PROC_MU_CHK(_val) */
/* #endif */


#define _ISSUE_PROC_JUMBO(_pkt, _sig)                                   \
do {                                                                    \
    /* Issue DMA for 4k of segment, updating processing state */        \
    pcie_dma_set_sig(&descr_tmp, __MEID, ctx(),                         \
                     __signal_number(&_sig));                           \
    dma_out.pkt##_pkt## = descr_tmp;                                    \
                                                                        \
    __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,                      \
                   NFD_IN_DATA_DMA_QUEUE,                               \
                   ctx_swap, &dma_sig##_pkt##);                         \
    __implicit_write(&_sig);                                            \
                                                                        \
    descr_tmp.pcie_addr_lo += PCIE_DMA_MAX_SZ;                          \
    descr_tmp.cpp_addr_lo += PCIE_DMA_MAX_SZ;                           \
    dma_len -= PCIE_DMA_MAX_SZ;                                         \
    jumbo_msk |= __signals(&_sig);                                      \
} while (0)


#define _ISSUE_PROC(_pkt, _type, _src)                                  \
do {                                                                    \
    unsigned int dma_len;                                               \
    __gpr unsigned int buf_addr;                                        \
    __gpr unsigned int curr_buf;                                        \
                                                                        \
    /* THS-54 workaround, round DMA up to next 4B multiple size */      \
    dma_len = ((tx_desc.pkt##_pkt##.dma_len + NFD_IN_DATA_ROUND - 1) &  \
               ~(NFD_IN_DATA_ROUND -1));                                \
                                                                        \
    if (tx_desc.pkt##_pkt##.eop && !queue_data[queue].cont) {           \
        /* Fast path, use buf_store data */                             \
        __critical_path();                                              \
                                                                        \
        /* Set NFP buffer address and offset */                         \
        buf_addr = precache_bufs_use();                                 \
        issued_tmp.buf_addr = buf_addr;                                 \
        _ISSUE_PROC_MU_CHK(buf_addr);                                   \
        descr_tmp.cpp_addr_hi = buf_addr>>21;                           \
        descr_tmp.cpp_addr_lo = buf_addr<<11;                           \
        descr_tmp.cpp_addr_lo += NFD_IN_DATA_OFFSET;                    \
        descr_tmp.cpp_addr_lo -= tx_desc.pkt##_pkt##.offset;            \
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
            /* precache_bufs_compute_seq_safe will give a pessimistic */ \
            /* safe count. */                                           \
            data_dma_seq_issued--;                                      \
                                                                        \
            /* Wait for previous jumbo frame on ctx to complete */      \
            if (jumbo_msk != 0) {                                       \
                wait_sig_mask(jumbo_msk);                               \
                jumbo_msk = 0;                                          \
                __implicit_read(&jumbo0);                               \
                __implicit_read(&jumbo1);                               \
            }                                                           \
                                                                        \
            /* Always DMA PCIE_DMA_MAX_SZ segments for jumbos */        \
            descr_tmp.length = PCIE_DMA_MAX_SZ - 1;                     \
                                                                        \
            /* Handle first PCIE_DMA_MAX_SZ */                          \
            _ISSUE_PROC_JUMBO(_pkt, jumbo0);                            \
                                                                        \
            if (dma_len > PCIE_DMA_MAX_SZ) {                            \
                /* Handle second PCIE_DMA_MAX_SZ */                     \
                _ISSUE_PROC_JUMBO(_pkt, jumbo1);                        \
            }                                                           \
                                                                        \
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
        if (_pkt == 0) {                                                \
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
                                                                        \
    } else {                                                            \
        if (!queue_data[queue].cont) {                                  \
            /* Initialise continuation data */                          \
                                                                        \
            /* XXX check efficiency */                                  \
            curr_buf = precache_bufs_use();                             \
            _ISSUE_PROC_MU_CHK(curr_buf);                               \
            queue_data[queue].cont = 1;                                 \
            queue_data[queue].offset = NFD_IN_DATA_OFFSET;              \
            queue_data[queue].offset -= tx_desc.pkt##_pkt##.offset;     \
            queue_data[queue].curr_buf = curr_buf;                      \
        }                                                               \
        curr_buf = queue_data[queue].curr_buf;                          \
                                                                        \
        /* Use continuation data */                                     \
        descr_tmp.cpp_addr_hi = curr_buf>>21;                           \
        descr_tmp.cpp_addr_lo = curr_buf<<11;                           \
        descr_tmp.cpp_addr_lo += queue_data[queue].offset;              \
        queue_data[queue].offset += dma_len;                            \
                                                                        \
        issued_tmp.buf_addr = curr_buf;                                 \
                                                                        \
        if (tx_desc.pkt##_pkt##.eop) {                                  \
            /* Clear continuation data on EOP */                        \
                                                                        \
            /* XXX check this is done in two cycles */                  \
            queue_data[queue].cont = 0;                                 \
            queue_data[queue].sp1 = 0;                                  \
            queue_data[queue].curr_buf = 0;                             \
            queue_data[queue].offset = 0;                               \
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
            /* precache_bufs_compute_seq_safe will give a pessimistic */ \
            /* safe count. */                                           \
            data_dma_seq_issued--;                                      \
                                                                        \
            /* Wait for previous jumbo frame on ctx to complete */      \
            if (jumbo_msk != 0) {                                       \
                wait_sig_mask(jumbo_msk);                               \
                jumbo_msk = 0;                                          \
                __implicit_read(&jumbo0);                               \
                __implicit_read(&jumbo1);                               \
            }                                                           \
                                                                        \
            /* Always DMA PCIE_DMA_MAX_SZ segments for jumbos */        \
            descr_tmp.length = PCIE_DMA_MAX_SZ - 1;                     \
                                                                        \
            /* Handle first PCIE_DMA_MAX_SZ */                          \
            _ISSUE_PROC_JUMBO(_pkt, jumbo0);                            \
                                                                        \
            if (dma_len > PCIE_DMA_MAX_SZ) {                            \
                /* Handle second PCIE_DMA_MAX_SZ */                     \
                _ISSUE_PROC_JUMBO(_pkt, jumbo1);                        \
            }                                                           \
                                                                        \
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
    }                                                                   \
                                                                        \
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

    __implicit_write(&jumbo0);
    __implicit_write(&jumbo1);

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
        local_csr_wr[NFP_MECSR_ACTIVE_CTX_WAKEUP_EVENTS>>2, wait_msk];
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
        halt();
    }

    /* We have finished processing the batch, let the next continue */
    reorder_done_opt(&next_ctx, &dma_order_sig);

    /* XXX THS-50 workaround */
    /* cls_ring_put(NFD_IN_ISSUED_RING_NUM, &batch_out, sizeof batch_out, */
    /*              &msg_sig); */
    ctm_ring_put(NFD_IN_ISSUED_RING_NUM, &batch_out, sizeof batch_out, &msg_sig);
}
