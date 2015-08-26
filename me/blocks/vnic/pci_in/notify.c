/*
 * Copyright (C) 2014-2015 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/notify.c
 * @brief         Code to notify host and app that packet was transmitted
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_me.h>

#include <nfp/mem_ring.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_in.h>
#include <vnic/pci_in/notify_status.c>
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


/* Shared by both issue DMA MEs */
__visible volatile SIGNAL nfd_in_data_compl_refl_sig;

/* Used for issue DMA 0 */
__shared __gpr unsigned int data_dma_seq_served0 = 0;
__shared __gpr unsigned int data_dma_seq_compl0 = 0;
static __gpr unsigned int data_dma_seq_sent0 = 0;
static __xwrite unsigned int nfd_in_data_served_refl_out0 = 0;

/* Shared with issue DMA 0 */
__visible volatile __xread unsigned int nfd_in_data_compl_refl_in0 = 0;
__remote volatile __xread unsigned int nfd_in_data_served_refl_in0;
__remote volatile SIGNAL nfd_in_data_served_refl_sig0;

/* Used for issue DMA 1 */
__shared __gpr unsigned int data_dma_seq_served1 = 0;
__shared __gpr unsigned int data_dma_seq_compl1 = 0;
static __gpr unsigned int data_dma_seq_sent1 = 0;
static __xwrite unsigned int nfd_in_data_served_refl_out1 = 0;

/* Shared with issue DMA 1 */
__visible volatile __xread unsigned int nfd_in_data_compl_refl_in1 = 0;
__remote volatile __xread unsigned int nfd_in_data_served_refl_in1;
__remote volatile SIGNAL nfd_in_data_served_refl_sig1;


static SIGNAL wq_sig0, wq_sig1, wq_sig2, wq_sig3;
static SIGNAL wq_sig4, wq_sig5, wq_sig6, wq_sig7;
static SIGNAL msg_sig0, msg_sig1, qc_sig;
static SIGNAL get_order_sig;    /* Signal for reordering before issuing get */
static SIGNAL msg_order_sig;    /* Signal for reordering on message return */
static SIGNAL_MASK wait_msk;
static unsigned int next_ctx;

__xwrite struct _pkt_desc_batch batch_out;
__xread unsigned int qc_xfer;

/* XXX declare dst_q counters in LM */

#ifdef NFD_IN_WQ_SHARED

#define NFD_IN_RINGS_MEM_IND2(_isl, _emem)                              \
    ASM(.alloc_mem nfd_in_rings_mem0 _emem global                       \
        (NFD_IN_WQ_SZ * NFD_IN_NUM_WQS) (NFD_IN_WQ_SZ * NFD_IN_NUM_WQS))
#define NFD_IN_RINGS_MEM_IND1(_isl, _emem) NFD_IN_RINGS_MEM_IND2(_isl, _emem)
#define NFD_IN_RINGS_MEM_IND0(_isl)                    \
    NFD_IN_RINGS_MEM_IND1(_isl, NFD_IN_WQ_SHARED)
#define NFD_IN_RINGS_MEM(_isl) NFD_IN_RINGS_MEM_IND0(_isl)

#define NFD_IN_RING_INIT_IND0(_isl, _num)                               \
    NFD_IN_RING_NUM_ALLOC(_isl, _num)                                   \
    ASM(.declare_resource nfd_in_ring_mem_res0##_num                    \
        global NFD_IN_WQ_SZ nfd_in_rings_mem0)                          \
    ASM(.alloc_resource nfd_in_ring_mem0##_num                          \
        nfd_in_ring_mem_res0##_num global NFD_IN_WQ_SZ NFD_IN_WQ_SZ)    \
    ASM(.init_mu_ring nfd_in_ring_num0##_num nfd_in_ring_mem0##_num)
#define NFD_IN_RING_INIT(_isl, _num) NFD_IN_RING_INIT_IND0(_isl, _num)

#else /* !NFD_IN_WQ_SHARED */

#define NFD_IN_RINGS_MEM_IND2(_isl, _emem)                              \
    ASM(.alloc_mem nfd_in_rings_mem##_isl _emem global                  \
        (NFD_IN_WQ_SZ * NFD_IN_NUM_WQS) (NFD_IN_WQ_SZ * NFD_IN_NUM_WQS))
#define NFD_IN_RINGS_MEM_IND1(_isl, _emem) NFD_IN_RINGS_MEM_IND2(_isl, _emem)
#define NFD_IN_RINGS_MEM_IND0(_isl)                    \
    NFD_IN_RINGS_MEM_IND1(_isl, NFD_PCIE##_isl##_EMEM)
#define NFD_IN_RINGS_MEM(_isl) NFD_IN_RINGS_MEM_IND0(_isl)

#define NFD_IN_RING_INIT_IND0(_isl, _num)                               \
    NFD_IN_RING_NUM_ALLOC(_isl, _num)                                   \
    ASM(.declare_resource nfd_in_ring_mem_res##_isl##_num               \
        global NFD_IN_WQ_SZ nfd_in_rings_mem##_isl)                     \
    ASM(.alloc_resource nfd_in_ring_mem##_isl##_num                     \
        nfd_in_ring_mem_res##_isl##_num global NFD_IN_WQ_SZ NFD_IN_WQ_SZ) \
    ASM(.init_mu_ring nfd_in_ring_num##_isl##_num nfd_in_ring_mem##_isl##_num)
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


#ifdef NFD_IN_ADD_SEQN
#if (NFD_IN_NUM_WQS == 1)
/* Add sequence numbers, using a shared GPR to store */
static __shared __gpr dst_q_seqn = 0;

#define NFD_IN_ADD_SEQN_PROC                                            \
do {                                                                    \
    pkt_desc_tmp.reserved = dst_q_seqn;                                 \
    dst_q_seqn++;                                                       \
} while (0)

#else
/* Add sequence numbers, using a LM to store */
static __shared __lmem dst_q_seqn[NFD_IN_NUM_WQS];

#define NFD_IN_ADD_SEQN_PROC                                            \
do {                                                                    \
    pkt_desc_tmp.reserved = dst_q_seqn[dst_q];                          \
    dst_q_seqn[dst_q]++;                                                \
} while (0)

#endif

#else
/* Null sequence number add */
#define NFD_IN_ADD_SEQN_PROC                                            \
do {                                                                    \
} while (0)

#endif


/* XXX Move to some sort of CT reflect library */
__intrinsic void
reflect_data(unsigned int dst_me, unsigned int dst_xfer,
             unsigned int sig_no, volatile __xwrite void *src_xfer,
             size_t size)
{
    #define OV_SIG_NUM 13

    unsigned int addr;
    unsigned int count = (size >> 2);
    struct nfp_mecsr_cmd_indirect_ref_0 indirect;

    /* ctassert(__is_write_reg(src_xfer)); */ /* TEMP, avoid volatile warnings */
    ctassert(__is_ct_const(size));

    /* Generic address computation.
     * Could be expensive if dst_me, or dst_xfer
     * not compile time constants */
    addr = ((dst_me & 0xFF0)<<20 | ((dst_me & 0xF)<<10 | (dst_xfer & 0x3F)<<2));

    indirect.__raw = 0;
    indirect.signal_num = sig_no;
    local_csr_write(local_csr_cmd_indirect_ref_0, indirect.__raw);

    /* Currently just support reflect_write_sig_remote */
    __asm {
        alu[--, --, b, 1, <<OV_SIG_NUM];
        ct[reflect_write_sig_remote, *src_xfer, addr, 0, \
           __ct_const_val(count)], indirect_ref;
    };
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
    reorder_start(NFD_IN_NOTIFY_START_CTX, &msg_order_sig);
    reorder_start(NFD_IN_NOTIFY_START_CTX, &get_order_sig);
}


/**
 * Perform per context initialisation (for CTX 1 to 7)
 */
void
notify_setup()
{
    wait_msk = __signals(&msg_sig0, &msg_sig1, &msg_order_sig);
    next_ctx = reorder_get_next_ctx(NFD_IN_NOTIFY_START_CTX,
                                    NFD_IN_NOTIFY_END_CTX);
}

#ifndef NFD_MU_PTR_DBG_MSK
#define NFD_MU_PTR_DBG_MSK 0x1f000000
#endif

#ifdef NFD_IN_NOTIFY_DBG_CHKS
#define _NOTIFY_MU_CHK(_pkt)                                            \
    if ((batch_in.pkt##_pkt##.__raw[1] & NFD_MU_PTR_DBG_MSK) == 0) {    \
        /* Write the error we read to Mailboxes for debug purposes */   \
        local_csr_write(local_csr_mailbox_0,                            \
                        NFD_IN_NOTIFY_MU_PTR_INVALID);                  \
        local_csr_write(local_csr_mailbox_1,                            \
                        batch_in.pkt##_pkt##.__raw[1]);                 \
                                                                        \
        halt();                                                         \
    }
#else
#define _NOTIFY_MU_CHK(_pkt)
#endif

#define _NOTIFY_PROC(_pkt)                                              \
do {                                                                    \
    if (batch_in.pkt##_pkt##.eop) {                                     \
        __critical_path();                                              \
        _NOTIFY_MU_CHK(_pkt)                                            \
        pkt_desc_tmp.sp0 = 0;                                           \
        pkt_desc_tmp.offset = batch_in.pkt##_pkt##.offset;              \
        dst_q = (batch_in.pkt##_pkt##.lso & NFD_IN_DSTQ_MSK);           \
        NFD_IN_ADD_SEQN_PROC;                                           \
        dst_q |= wq_num_base;                                           \
        batch_out.pkt##_pkt##.__raw[0] = pkt_desc_tmp.__raw[0];         \
        batch_out.pkt##_pkt##.__raw[1] = batch_in.pkt##_pkt##.__raw[1]; \
        batch_out.pkt##_pkt##.__raw[2] = batch_in.pkt##_pkt##.__raw[2]; \
        batch_out.pkt##_pkt##.__raw[3] = batch_in.pkt##_pkt##.__raw[3]; \
                                                                        \
        __mem_workq_add_work(dst_q, wq_raddr, &batch_out.pkt##_pkt,     \
                             out_msg_sz, out_msg_sz, sig_done, &wq_sig##_pkt); \
    } else {                                                            \
        /* Remove the wq signal from the wait mask */                   \
        wait_msk &= ~__signals(&wq_sig##_pkt);                          \
    }                                                                   \
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
__forceinline void
_notify(__gpr unsigned int *complete, __gpr unsigned int *served,
        int input_ring)
{

    unsigned int n_batch;
    unsigned int q_batch;
    unsigned int qc_queue;

    unsigned int dst_q;
    unsigned int out_msg_sz = sizeof(struct nfd_in_pkt_desc);

    __xread struct _issued_pkt_batch batch_in;
    struct _pkt_desc_batch batch_tmp;
    struct nfd_in_pkt_desc pkt_desc_tmp;


    /* Reorder before potentially issuing a ring get */
    wait_for_all(&get_order_sig);
    reorder_done_opt(&next_ctx, &get_order_sig);

    /* Is there a batch to process
     * XXX assume that issue_dma only inc's dma seq for final dma in batch */
    if (*complete != *served)
    {
        /* Process whole batch */
        __critical_path();

        /* Increment data_dma_seq_served before swapping */
        *served += 1;

        ctm_ring_get(NOTIFY_RING_ISL, input_ring, &batch_in.pkt0,
                     (sizeof(struct nfd_in_issued_desc) * 4), &msg_sig0);
        ctm_ring_get(NOTIFY_RING_ISL, input_ring, &batch_in.pkt4,
                     (sizeof(struct nfd_in_issued_desc) * 4), &msg_sig1);

        __asm {
            ctx_arb[--], defer[1];
            local_csr_wr[local_csr_active_ctx_wakeup_events, wait_msk];
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

        reorder_done_opt(&next_ctx, &msg_order_sig);

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
        pkt_desc_tmp.reserved = 0;
#endif

        _NOTIFY_PROC(0);
        _NOTIFY_PROC(1);
        _NOTIFY_PROC(2);
        _NOTIFY_PROC(3);
        _NOTIFY_PROC(4);
        _NOTIFY_PROC(5);
        _NOTIFY_PROC(6);
        _NOTIFY_PROC(7);

        /* Map batch.queue to a QC queue and increment the TX_R pointer
         * for that queue by n_batch */
        qc_queue = NFD_NATQ2QC(NFD_BMQ2NATQ(q_batch), NFD_IN_TX_QUEUE);
        __qc_add_to_ptr(PCIE_ISL, qc_queue, QC_RPTR, n_batch, &qc_xfer,
                        sig_done, &qc_sig);
    } else {
        /* Participate in msg ordering */
        wait_for_all(&msg_order_sig);
        reorder_done_opt(&next_ctx, &msg_order_sig);
        return;
    }
}


__forceinline void
notify(int side)
{
    if (side == 0) {
        _notify(&data_dma_seq_compl0, &data_dma_seq_served0,
                NFD_IN_ISSUED_RING0_NUM);
    } else {
        _notify(&data_dma_seq_compl1, &data_dma_seq_served1,
                NFD_IN_ISSUED_RING1_NUM);
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
distr_notify()
{
    if (signal_test(&nfd_in_data_compl_refl_sig)) {
#ifdef NFD_IN_HAS_ISSUE0
        data_dma_seq_compl0 = nfd_in_data_compl_refl_in0;
#endif
#ifdef NFD_IN_HAS_ISSUE1
        data_dma_seq_compl1 = nfd_in_data_compl_refl_in1;
#endif NFD_IN_HAS_ISSUE1
    }

#ifdef NFD_IN_HAS_ISSUE0
    if (data_dma_seq_served0 != data_dma_seq_sent0) {
        __implicit_read(&nfd_in_data_served_refl_out0);

        data_dma_seq_sent0 = data_dma_seq_served0;

        nfd_in_data_served_refl_out0 = data_dma_seq_sent0;
        reflect_data(NFD_IN_DATA_DMA_ME0,
                     __xfer_reg_number(&nfd_in_data_served_refl_in0,
                                       NFD_IN_DATA_DMA_ME0),
                     __signal_number(&nfd_in_data_served_refl_sig0,
                                     NFD_IN_DATA_DMA_ME0),
                     &nfd_in_data_served_refl_out0,
                     sizeof nfd_in_data_served_refl_out0);
    }
#endif

#ifdef NFD_IN_HAS_ISSUE1
    if (data_dma_seq_served1 != data_dma_seq_sent1) {
        __implicit_read(&nfd_in_data_served_refl_out1);

        data_dma_seq_sent1 = data_dma_seq_served1;

        nfd_in_data_served_refl_out1 = data_dma_seq_sent1;
        reflect_data(NFD_IN_DATA_DMA_ME1,
                     __xfer_reg_number(&nfd_in_data_served_refl_in1,
                                       NFD_IN_DATA_DMA_ME1),
                     __signal_number(&nfd_in_data_served_refl_sig1,
                                     NFD_IN_DATA_DMA_ME1),
                     &nfd_in_data_served_refl_out1,
                     sizeof nfd_in_data_served_refl_out1);
    }
#endif
}


int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {

        nfd_cfg_check_pcie_link(); /* Will halt ME on failure */

        notify_setup_shared();
        notify_status_setup();

        /* NFD_INIT_DONE_SET(PCIE_ISL, 2);     /\* XXX Remove? *\/ */

    }

    notify_setup();

    if (ctx() == 0) {

        for (;;) {
            distr_notify();
#ifdef NFD_IN_HAS_ISSUE0
            notify(0);
#endif
#ifdef NFD_IN_HAS_ISSUE1
            notify(1);
#endif
        }

    } else {

        for (;;) {
#ifdef NFD_IN_HAS_ISSUE0
            notify(0);
#endif
#ifdef NFD_IN_HAS_ISSUE1
            notify(1);
#endif
        }

    }
}
