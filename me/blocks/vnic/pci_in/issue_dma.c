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
#include <std/reg_utils.h>

#include <nfp6000/nfp_me.h>

#include <vnic/pci_in.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_internal.h>

/*#include <vnic/utils/cls_ring.h> */ /* XXX THS-50 workaround */
#include <vnic/utils/ctm_ring.h> /* XXX THS-50 workaround */
#include <vnic/utils/nn_ring.h>
#include <vnic/utils/ordering.h>
#include <vnic/utils/qc.h>
#include <vnic/utils/pcie.h>

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

/* Configure the NN ring */
NN_RING_ZERO_PTRS;
NN_RING_EMPTY_ASSERT_SET(0);


/**
 * Perform shared configuration for issue_dma
 */
void
issue_dma_setup_shared()
{
    struct pcie_dma_cfg_one cfg;

    /* XXX complete non-per-queue data */
    unsigned int queue;
    unsigned int vnic;
    unsigned int vnic_q;

    ctassert(__is_log2(NFD_MAX_VNICS));
    ctassert(__is_log2(NFD_MAX_VNIC_QUEUES));

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
     * Set requester IDs
     */
    for (queue = 0, vnic = 0; vnic < NFD_MAX_VNICS; vnic++) {
        for (vnic_q = 0; vnic_q < NFD_MAX_VNIC_QUEUES; vnic_q++, queue++) {
            unsigned int bmsk_queue;

            bmsk_queue = map_natural_to_bitmask(queue);
            queue_data[bmsk_queue].rid = vnic;
#ifdef NFD_VNIC_VF
            queue_data[bmsk_queue].rid += NFD_CFG_VF_OFFSET;
#endif
        }
    }

    /*
     * Set up NFD_IN_DATA_CFG_REG DMA Config Register
     */
    cfg.__raw = 0;
#ifdef NFD_VNIC_NO_HOST
    /* Use signal_only for seqn num generation
     * Don't actually DMA data */
    cfg.signal_only = 1;
#else
    cfg.signal_only = 0;
#endif
    cfg.end_pad     = 0;
    cfg.start_pad   = 0;
    /* Ordering settings? */
    cfg.target_64   = 1;
    cfg.cpp_target  = 7;
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_IN_DATA_CFG_REG, cfg);

    /* Kick off ordering */
    reorder_start(NFD_IN_ISSUE_START_CTX, &desc_order_sig);
    reorder_start(NFD_IN_ISSUE_START_CTX, &dma_order_sig);
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
}


#define _ISSUE_PROC(_pkt, _type, _src)                                  \
do {                                                                    \
    unsigned int dma_len;                                               \
                                                                        \
    /* THS-54 workaround, round DMA up to next 4B multiple size */      \
    dma_len = ((tx_desc.pkt##_pkt##.dma_len + NFD_IN_DATA_ROUND) &      \
               ~(NFD_IN_DATA_ROUND -1));                                \
                                                                        \
    if (tx_desc.pkt##_pkt##.eop && !queue_data[queue].cont) {           \
        /* Fast path, use buf_store data */                             \
        __critical_path();                                              \
                                                                        \
        issued_tmp.buf_addr = precache_bufs_use();                      \
        descr_tmp.cpp_addr_hi = issued_tmp.buf_addr>>21;                \
        descr_tmp.cpp_addr_lo = issued_tmp.buf_addr<<11;                \
        descr_tmp.cpp_addr_lo += NFD_IN_DATA_OFFSET;                    \
        descr_tmp.cpp_addr_lo -= tx_desc.pkt##_pkt##.offset;            \
                                                                        \
    } else {                                                            \
        if (!queue_data[queue].cont) {                                  \
            /* Initialise continuation data */                          \
                                                                        \
            /* XXX check efficiency */                                  \
            queue_data[queue].curr_buf = precache_bufs_use();           \
            queue_data[queue].cont = 1;                                 \
            queue_data[queue].offset = NFD_IN_DATA_OFFSET;              \
            queue_data[queue].offset -= tx_desc.pkt##_pkt##.offset;     \
        }                                                               \
                                                                        \
        /* Use continuation data */                                     \
        descr_tmp.cpp_addr_hi = queue_data[queue].curr_buf>>21;         \
        descr_tmp.cpp_addr_lo = queue_data[queue].curr_buf<<11;         \
        descr_tmp.cpp_addr_lo += queue_data[queue].offset;              \
        queue_data[queue].offset += dma_len;                            \
                                                                        \
        issued_tmp.buf_addr = queue_data[queue].curr_buf;               \
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
    }                                                                   \
                                                                        \
    /* NB: EOP is required for all packets */                           \
    /*     q_num is must be set on pkt0 */                              \
    /*     notify technically doesn't use the rest unless */            \
    /*     EOP is set */                                                \
    issued_tmp.eop = tx_desc.pkt##_pkt##.eop;                           \
    issued_tmp.sp0 = 0;                                                 \
    issued_tmp.sp1 = 0; /* XXX most efficient value to set? */          \
    issued_tmp.dst_q = tx_desc.pkt##_pkt##.dst_q;                       \
                                                                        \
    batch_out.pkt##_pkt## = issued_tmp;                                 \
    batch_out.pkt##_pkt##.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];      \
    batch_out.pkt##_pkt##.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];      \
                                                                        \
    descr_tmp.pcie_addr_hi = tx_desc.pkt##_pkt##.dma_addr_hi;           \
    descr_tmp.pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;           \
                                                                        \
    descr_tmp.rid = queue_data[queue].rid;                              \
    pcie_dma_set_event(&descr_tmp, _type, _src);                        \
    descr_tmp.length = dma_len - 1;                                     \
    dma_out.pkt##_pkt## = descr_tmp;                                    \
                                                                        \
    __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##, NFD_IN_DATA_DMA_QUEUE, \
                   sig_done, &dma_sig##_pkt##);                         \
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
void
issue_dma()
{
    static __xread struct _tx_desc_batch tx_desc;
    __cls void *desc_ring_addr;
    unsigned int desc_ring_off;

    __gpr struct nfd_in_issued_desc issued_tmp;

    struct nfd_in_batch_desc batch;
    unsigned int queue;


    reorder_test_swap(&desc_order_sig);

    /* Check "DMA" completed and we can read the batch
     * If so, the NN ring MUST have a batch descriptor for us
     * NB: only one ctx can execute this at any given time */
    while (gather_dma_seq_compl == gather_dma_seq_serv) {
        ctx_swap(); /* Yield while waiting for work */
    }

    reorder_done(NFD_IN_ISSUE_START_CTX, &desc_order_sig);

    if (nn_ring_empty()) {
        halt();          /* A serious error has occurred */
    }

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

    while (data_dma_seq_issued == data_dma_seq_safe) {
        /* We can't process this batch yet.
         * Swap then recompute seq_safe.
         * NB: only one ctx can execute this at any given time */
        ctx_swap();
        precache_bufs_compute_seq_safe();
    }

    /* We can process this batch, allow next CTX go too */
    reorder_done(NFD_IN_ISSUE_START_CTX, &dma_order_sig);

    queue = batch.queue;
    data_dma_seq_issued++;

    issued_tmp.q_num = queue;
    issued_tmp.num_batch = batch.num;   /* Only needed in pkt0 */

    /* Maybe add "full" bit */
    switch (batch.num) {
    case 4:
        /* Full batches are the critical path */
        /* XXX maybe tricks with an extra nfd_in_dma_state
         * struct would convince nfcc to use one set LM index? */
        __critical_path();
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(2, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(3, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);
        break;

    case 3:
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(2, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(3);
        break;

    case 2:
        _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0);
        _ISSUE_PROC(1, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(2);
        _ISSUE_CLR(3);
        break;

    case 1:
        _ISSUE_PROC(0, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued);

        _ISSUE_CLR(1);
        _ISSUE_CLR(2);
        _ISSUE_CLR(3);
        break;

    default:
        halt();
    }

    /* XXX THS-50 workaround */
    /* cls_ring_put(NFD_IN_ISSUED_RING_NUM, &batch_out, sizeof batch_out, */
    /*              &msg_sig); */
    ctm_ring_put(NFD_IN_ISSUED_RING_NUM, &batch_out, sizeof batch_out, &msg_sig);
}
