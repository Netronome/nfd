/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/issue_dma.c
 * @brief         Code to DMA packet data to the NFP
 */

#include <assert.h>
#include <nfp.h>
#include <types.h>

#include <nfp/cls.h>                /* TEMP */
#include <nfp/mem_ring.h>           /* TEMP */
#include <std/reg_utils.h>

#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_cls.h>        /* TEMP */

#include <vnic/pci_in/issue_dma.h>

#include <vnic/pci_in.h>
#include <vnic/pci_in_cfg.h>
#include <vnic/pci_in/pci_in_internal.h>
#include <vnic/pci_in/precache_bufs.h>
#include <vnic/shared/qc.h>
#include <vnic/utils/cls_ring.h>
#include <vnic/utils/pcie.h>
#include <vnic/utils/nn_ring.h>


struct _tx_desc_batch {
    struct nfd_pci_in_tx_desc pkt0;
    struct nfd_pci_in_tx_desc pkt1;
    struct nfd_pci_in_tx_desc pkt2;
    struct nfd_pci_in_tx_desc pkt3;
};

struct _issued_pkt_batch {
    struct nfd_pci_in_issued_desc pkt0;
    struct nfd_pci_in_issued_desc pkt1;
    struct nfd_pci_in_issued_desc pkt2;
    struct nfd_pci_in_issued_desc pkt3;
};

struct _dma_desc_batch {
    struct nfp_pcie_dma_cmd pkt0;
    struct nfp_pcie_dma_cmd pkt1;
    struct nfp_pcie_dma_cmd pkt2;
    struct nfp_pcie_dma_cmd pkt3;
};

/* Ring declarations */
/* XXX use CLS ring API when available */
__export __align(sizeof(struct nfd_pci_in_issued_desc) * TX_ISSUED_RING_SZ)
    __cls struct nfd_pci_in_issued_desc tx_issued_ring[TX_ISSUED_RING_SZ];

#define DESC_RING_SZ (MAX_TX_BATCH_SZ * DESC_BATCH_Q_SZ *       \
                      sizeof(struct nfd_pci_in_tx_desc))
__export __shared __cls __align(DESC_RING_SZ) struct nfd_pci_in_tx_desc
    desc_ring[MAX_TX_BATCH_SZ * DESC_BATCH_Q_SZ];

static __shared __gpr unsigned int desc_ring_base;


/* Storage declarations */
static __shared __lmem struct tx_dma_state queue_info[MAX_TX_QUEUES];

/* Sequence number declarations */
extern __shared __gpr unsigned int gather_dma_seq_compl;
__shared __gpr unsigned int gather_dma_seq_serv = 0;

extern __shared __gpr unsigned int data_dma_seq_compl;
__shared __gpr unsigned int data_dma_seq_issued = 0;

/* DMA descriptor template */
static __gpr struct nfp_pcie_dma_cmd descr_tmp;

/* Signalling */
static SIGNAL tx_desc_sig;
static SIGNAL_MASK wait_msk;

/* TEMP */
__shared __gpr mem_ring_addr_t data_dmas_addr; /* TEMP */
MEM_JOURNAL_DECLARE(data_dmas, 1024); /* TEMP */

void
issue_dma_setup_shared()
{
    struct pcie_dma_cfg_one cfg;

    /* XXX complete non-per-queue data */
    unsigned int queue;
    unsigned int vnic;
    unsigned int vnic_q;

    ctassert(__is_log2(MAX_VNICS));
    ctassert(__is_log2(MAX_VNIC_QUEUES));

    nn_ring_init_receive(0, 0);             /* TEMP */

    cls_ring_setup(TX_ISSUED_RING_NUM, tx_issued_ring, sizeof tx_issued_ring);

    /*
     * Initialise the CLS TX descriptor ring
     */
    desc_ring_base = ((unsigned int) &desc_ring) & 0xFFFFFFFF;

    data_dmas_addr = MEM_JOURNAL_CONFIGURE(data_dmas, 0); /* TEMP */

    /*
     * Set requester IDs
     */
    for (queue = 0, vnic = 0; vnic < MAX_VNICS; vnic++) {
        for (vnic_q = 0; vnic_q < MAX_VNIC_QUEUES; vnic_q++, queue++) {
            unsigned int bmsk_queue;

            bmsk_queue = map_natural_to_bitmask(queue);
            queue_info[bmsk_queue].rid = vnic;
        }
    }

    /*
     * Set up TX_DATA_CFG_REG DMA Config Register
     */
    cfg.__raw = 0;
    cfg.signal_only = 1; /* TEMP use DMAs for event seqn only */
    cfg.end_pad     = 0;
    cfg.start_pad   = 0;
    /* Ordering settings? */
    cfg.target_64   = 1;
    cfg.cpp_target  = 7;
    pcie_dma_cfg_set_one(PCIE_ISL, TX_DATA_CFG_REG, cfg);
}

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
    descr_tmp.cpp_token = 0;    /* XXX Token required TBD */
    descr_tmp.dma_cfg_index = TX_DATA_CFG_REG;

    /* Initialise wait_msk to wait on msg_sig only */
    wait_msk = __signals(&tx_desc_sig);
}


static __intrinsic void dummy_dma(__xwrite void *descr,
                                  SIGNAL_PAIR *jsig, SIGNAL *dsig)
{
    __mem_ring_journal(0, data_dmas_addr, descr,
                       sizeof(struct nfp_pcie_dma_cmd),
                       sizeof(struct nfp_pcie_dma_cmd),
                       sig_done, jsig);

    /* Fake a DMA ... using a DMA (signal only) */
    __pcie_dma_enq(PCIE_ISL, descr, TX_DATA_DMA_QUEUE, sig_done, dsig);
}


#define _ISSUE_PROC(_pkt, _type, _src)                                  \
do {                                                                    \
    if (tx_desc.pkt##_pkt##.eop && !queue_info[queue].cont) {           \
        /* Fast path, use buf_store data */                             \
        __critical_path();                                              \
                                                                        \
        issued_tmp.buf_addr = precache_bufs_use();                      \
        descr_tmp.cpp_addr_hi = issued_tmp.buf_addr>>21;                \
        descr_tmp.cpp_addr_lo = issued_tmp.buf_addr<<11;                \
                                                                        \
    } else {                                                            \
        if (!queue_info[queue].cont) {                                  \
            /* Initialise continuation data */                          \
                                                                        \
            /* XXX check efficiency */                                  \
            queue_info[queue].curr_buf = precache_bufs_use();           \
            queue_info[queue].cont = 1;                                 \
            queue_info[queue].offset = 0;                               \
        }                                                               \
                                                                        \
        /* Use continuation data */                                     \
        descr_tmp.cpp_addr_hi = queue_info[queue].curr_buf>>21;         \
        descr_tmp.cpp_addr_lo = queue_info[queue].curr_buf<<11;         \
        descr_tmp.cpp_addr_lo += queue_info[queue].offset;              \
        queue_info[queue].offset += tx_desc.pkt##_pkt##.dma_len;        \
                                                                        \
        issued_tmp.buf_addr = queue_info[queue].curr_buf;               \
                                                                        \
        if (tx_desc.pkt##_pkt##.eop) {                                  \
            /* Clear continuation data on EOP */                        \
                                                                        \
            /* XXX check this is done in to cycles */                   \
            queue_info[queue].cont = 0;                                 \
            queue_info[queue].sp1 = 0;                                  \
            queue_info[queue].curr_buf = 0;                             \
            queue_info[queue].offset = 0;                               \
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
    descr_tmp.rid = queue_info[queue].rid;                              \
    pcie_dma_set_event(&descr_tmp, _type, _src);                        \
    descr_tmp.length = tx_desc.pkt##_pkt##.dma_len;                     \
    dma_out.pkt##_pkt## = descr_tmp;                                    \
                                                                        \
    dummy_dma(&dma_out.pkt##_pkt##, &jsig##_pkt##, &dma_sig##_pkt##);   \
} while (0)


#define _ISSUE_CLR(_pkt)                                                \
do {                                                                    \
    /* Do minimal clean up so local signalling works and */             \
    /* notify block ignores the message */                              \
    batch_out.pkt##_pkt##.__raw[0] = 0;                                 \
    wait_msk &= ~__signals(&dma_sig##_pkt##);                           \
} while (0)


/** Parameters list to be filled out as extended */
void
issue_dma()
{
    static __xread struct _tx_desc_batch tx_desc;
    static __xwrite struct _dma_desc_batch dma_out;
    static __xwrite struct _issued_pkt_batch batch_out;
    static SIGNAL msg_sig, dma_sig0, dma_sig1, dma_sig2, dma_sig3;

    static SIGNAL_PAIR jsig0, jsig1, jsig2, jsig3; /* TEMP */

    unsigned int queue;
    unsigned int desc_ring_off;

    __cls void *desc_ring_addr;
    __gpr struct nfd_pci_in_issued_desc issued_tmp;


    /* XXX Reorder! */

    /* Check "DMA" completed and we can read the batch
     * If so, the NN ring MUST have a batch descriptor for us */
    if (gather_dma_seq_compl != gather_dma_seq_serv) {
        struct batch_desc batch;

        if (nn_ring_empty()) {
            halt();          /* A serious error has occurred */
        }

        /*
         * Increment gather_dma_seq_serv upfront to avoid
         * ambiguity about sequence number zero
         */
        gather_dma_seq_serv++;

        /* Read the batch */
        batch.__raw = nn_ring_get();
        desc_ring_off = ((gather_dma_seq_serv * sizeof(tx_desc)) &
                         (DESC_RING_SZ - 1));
        desc_ring_addr = (__cls void *) (desc_ring_base | desc_ring_off);
        __cls_read(&tx_desc, desc_ring_addr, sizeof tx_desc, sizeof tx_desc,
                   sig_done, &tx_desc_sig);

        __asm {
            ctx_arb[--], defer[1];
            local_csr_wr[NFP_MECSR_ACTIVE_CTX_WAKEUP_EVENTS>>2, wait_msk];
        }

        /* XXX live dangerously: don't wait on journal sigs
         * (__signals takes odd and even...) */
        wait_msk = __signals(&dma_sig0, &dma_sig1, &dma_sig2, &dma_sig3,
                             &tx_desc_sig, &msg_sig);
        __implicit_read(&batch_out, sizeof batch_out);
        __implicit_read(&dma_sig0);
        __implicit_read(&dma_sig1);
        __implicit_read(&dma_sig2);
        __implicit_read(&dma_sig3);
        __implicit_read(&msg_sig);

        /* XXX replace with blended test */
        if (data_dma_seq_issued < (TX_DATA_MAX_IN_FLIGHT + data_dma_seq_compl)) {
            queue = batch.queue;
            data_dma_seq_issued++;

            issued_tmp.q_num = queue;
            issued_tmp.num_batch = batch.num;   /* Only needed in pkt0 */

            /* Maybe add "full" bit */
            if (batch.num == 4)
            {
                /* Full batches are the critical path */
                /* XXX maybe tricks with an extra tx_dma_state
                 * struct would convince nfcc to use one set LM index? */
                __critical_path();
                _ISSUE_PROC(0, TX_DATA_IGN_EVENT_TYPE, 0);
                _ISSUE_PROC(1, TX_DATA_IGN_EVENT_TYPE, 0);
                _ISSUE_PROC(2, TX_DATA_IGN_EVENT_TYPE, 0);
                _ISSUE_PROC(3, TX_DATA_EVENT_TYPE, data_dma_seq_issued);
            } else {
                /* Off the critical path, handle non-full batches */
                if (batch.num == 3) {
                    _ISSUE_PROC(0, TX_DATA_IGN_EVENT_TYPE, 0);
                    _ISSUE_PROC(1, TX_DATA_IGN_EVENT_TYPE, 0);
                    _ISSUE_PROC(2, TX_DATA_EVENT_TYPE, data_dma_seq_issued);

                    _ISSUE_CLR(3);
                } else if (batch.num == 2) {
                    _ISSUE_PROC(0, TX_DATA_IGN_EVENT_TYPE, 0);
                    _ISSUE_PROC(1, TX_DATA_EVENT_TYPE, data_dma_seq_issued);

                    _ISSUE_CLR(2);
                    _ISSUE_CLR(3);
                } else {
                    _ISSUE_PROC(0, TX_DATA_EVENT_TYPE, data_dma_seq_issued);

                    _ISSUE_CLR(1);
                    _ISSUE_CLR(2);
                    _ISSUE_CLR(3);
                }
            }

            cls_ring_put(TX_ISSUED_RING_NUM, &batch_out, sizeof batch_out,
                         &msg_sig);
        }
    } else {
        ctx_swap();
    }
}

