/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/stage_batch.c
 * @brief         Look for a batch and stage the transfer
 */

#include <assert.h>
#include <vnic/shared/nfcc_chipres.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>
#include <std/event.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_event.h>
#include <nfp6000/nfp_me.h>

#include <vnic/pci_out.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/dma_seqn.h>
#include <vnic/utils/nn_ring.h>
#include <vnic/utils/ordering.h>
#include <vnic/utils/pcie.h>
#include <vnic/utils/qc.h>


/* XXX move somewhere shared? */
struct _dma_desc_batch {
    struct nfp_pcie_dma_cmd pkt0;
    struct nfp_pcie_dma_cmd pkt1;
    struct nfp_pcie_dma_cmd pkt2;
    struct nfp_pcie_dma_cmd pkt3;
};

struct _input_batch {
    struct nfd_out_input pkt0;
    struct nfd_out_input pkt1;
    struct nfd_out_input pkt2;
    struct nfd_out_input pkt3;
};


/*
 * Variables "owned" by cache_desc
 */
extern __shared __lmem struct nfd_out_queue_info queue_data[NFD_OUT_MAX_QUEUES];
extern __shared __gpr struct qc_bitmask urgent_bmsk;


/*
 * Rings and queues
 */
NFD_RING_DECLARE(PCIE_ISL, nfd_out, NFD_OUT_RING_SZ);
static __gpr mem_ring_addr_t in_ring_addr;
static __gpr unsigned int in_ring_num;

__shared __lmem struct nfd_out_desc_batch_msg
    desc_batch_msg[NFD_OUT_DESC_BATCH_RING_BAT];


/*
 * stage_batch variables
 */
__shared __gpr unsigned int batch_issued = 0;
__shared __gpr unsigned int batch_safe = NFD_OUT_DESC_BATCH_RING_BAT;

static __xread struct _input_batch in_batch;
SIGNAL get_sig;

SIGNAL get_order_sig, put_order_sig,  may_poll;
static SIGNAL_MASK stage_wait_msk = 0;


/*
 * distr_seqn variables
 */
static volatile __xread unsigned int data_dma_event_xfer;
static volatile __xread unsigned int desc_dma_event_xfer;
static SIGNAL data_dma_event_sig;
static SIGNAL desc_dma_event_sig;

static __xwrite unsigned int nfd_out_data_compl_refl_out = 0;
__remote volatile __xread unsigned int nfd_out_data_compl_refl_in;
__remote volatile SIGNAL nfd_out_data_compl_refl_sig;


/*
 * send_desc variables
 */
static __gpr struct nfp_pcie_dma_cmd descr_tmp;
static __xwrite struct _dma_desc_batch dma_out;
static SIGNAL desc_sig0, desc_sig1, desc_sig2, desc_sig3;
static SIGNAL_MASK desc_dma_wait_msk = 0;

__shared __gpr unsigned int send_desc_addr_lo;
__shared __gpr unsigned int send_desc_off = sizeof(struct nfd_out_cpp_desc);

__shared __gpr unsigned int desc_dma_issued = 0;
__shared __gpr unsigned int desc_dma_compl = 0;
/* Descriptor DMAs are counted individually, but we need space for
 * NFD_OUT_MAX_BATCH_SZ DMAs to ensure we can complete a full batch. */
__shared __gpr unsigned int desc_dma_safe = (NFD_OUT_DESC_MAX_IN_FLIGHT -
                                             NFD_OUT_MAX_BATCH_SZ);

__shared __gpr unsigned int data_dma_compl = 0;
__shared __gpr unsigned int desc_batch_served = 0;



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
    addr = ((dst_me & 0xFF0)<<20 | ((dst_me & 15)<<10 | (dst_xfer & 31)<<2));

    indirect.__raw = 0;
    indirect.signal_num = sig_no;
    local_csr_write(NFP_MECSR_CMD_INDIRECT_REF_0, indirect.__raw);

    /* Currently just support reflect_write_sig_remote */
    __asm {
        alu[--, --, b, 1, <<OV_SIG_NUM];
        ct[reflect_write_sig_remote, *src_xfer, addr, 0, \
           __ct_const_val(count)], indirect_ref;
    };
}


/* XXX remove when THSDK-886 resolved */
void
stage_batch_setup_rings()
{
    /* Input ring */
    in_ring_num = NFD_RING_ALLOC(PCIE_ISL, nfd_out, 1);
    MEM_RING_CONFIGURE(NFD_RING_BASE(PCIE_ISL, nfd_out), in_ring_num);
}

/**
 * Perform once off, CTX0-only initialisation
 */
void
stage_batch_setup_shared()
{
    /* Kick off ordering */
    reorder_start(NFD_OUT_STAGE_START_CTX, &get_order_sig);
    reorder_start(NFD_OUT_STAGE_START_CTX, &put_order_sig);
}


/**
 * Perform per CTX configuration of stage_batch
 */
void
stage_batch_setup()
{
    /* Input ring */
    in_ring_num = NFD_RING_ALLOC(PCIE_ISL, nfd_out, 1);
    in_ring_addr = (unsigned long long) NFD_EMEM(PCIE_ISL) >> 8;

    /* Allow polling initially */
    reorder_self(&may_poll);

    /* Wait on get_sig and put_order_sig.  There is no previous
     * put that needs to complete, so it is removed at start. */
    stage_wait_msk = __signals(&get_sig, &put_order_sig);
}

/**
 * Test available FL entries against a "soft" threshold
 *
 * If FL a lot of entries are available, this code performs only one test.
 * If fewer entries are available, the queue is flagged as "urgent" and
 * more detailed tests are performed.  The method will ultimately head of
 * line block if there is a packet to send on an "up" queue but no FL
 * entry for that packet.  "Down" queues do not head of line block.
 */
void
_fl_avail_check(__gpr unsigned int queue)
{
    /* Only test for fl entries on fast path as it serves as a proxy
     * for the queue being up as well. */
    if ((queue_data[queue].fl_a - queue_data[queue].fl_u) <
        NFD_OUT_FL_SOFT_THRESH) {
        set_queue(&queue, &urgent_bmsk);

        /* XXX check that this code actually rereads the LM values... */
        while (queue_data[queue].fl_a - queue_data[queue].fl_u == 0) {
            /* Head of line block if the queue is up, otherwise continue. */
            if (queue_data[queue].up) {
                ctx_swap();
            } else {
                break;
            }
        }

    } else {
        __critical_path();
    }
}


/**
 *  Add a "struct nfd_out_data_dma_info" to the NN ring
 *  @param msg      The structure to send
 */
__intrinsic void
_nn_put_msg(struct nfd_out_data_dma_info *msg)
{
    /* TEMP: NN ring false overflow workaround (THS TBD) */
    while (nn_ring_full()) {
        ctx_swap();
    }

    nn_ring_put(msg->__raw[0]);
    nn_ring_put(msg->__raw[1]);
    nn_ring_put(msg->__raw[2]);
    nn_ring_put(msg->__raw[3]);
}


/**
 * Construct messages for the "data_batch_ring" and the "desc_batch_ring".
 *
 * The "desc_batch_ring" needs the queue number from the RX descriptor and
 * the "EOP" bit from the CPP descriptor.  If the queue is down, we signal
 * this to the descriptor DMA using the "EOP" bit.
 *
 * The "data_batch_ring" needs the CPP descriptor and the address of a
 * freelist descriptor to use.  The freelist index is only advanced on
 * "EOP".  If the queue is down, the freelist index should not be advanced.
 * Marking the "down" bit will prevent the data DMA, but allow the buffer
 * to be freed as normal on "EOP".  For down queues, "SOP" is cleared to
 * push the packet off the fast path in the "issue_dma" block.
 */
#define _STAGE_BATCH_PROC(_pkt)                                         \
do {                                                                    \
    queue = in_batch.pkt##_pkt##.rxd.queue;                             \
    _fl_avail_check(queue);                                             \
                                                                        \
    /* XXX Check up is read after the potential ctx swap in _fl_avail_check */ \
    up = queue_data[queue].up;                                          \
    eop = in_batch.pkt##_pkt##.cpp.eop;                                 \
    eop &= up;                                                          \
                                                                        \
    data_batch_tmp.fl_cache_index =                                     \
        cache_desc_compute_fl_addr(&queue, queue_data[queue].fl_u);     \
    queue_data[queue].fl_u += eop;                                      \
    data_batch_tmp.rid = queue_data[queue].requester_id;                \
    data_batch_tmp.data_len = in_batch.pkt##_pkt##.rxd.data_len;        \
    data_batch_tmp.spare = 0;                                           \
                                                                        \
    data_batch_tmp.cpp = in_batch.pkt##_pkt##.cpp;                      \
    data_batch_tmp.cpp.sop &= up;                                       \
    data_batch_tmp.cpp.down = ~up;                                      \
    _nn_put_msg(&data_batch_tmp);                                       \
                                                                        \
    desc_batch_tmp.send_pkt##_pkt = eop;                                \
    desc_batch_tmp.queue_pkt##_pkt = queue;                             \
} while (0)


/* XXX is this even necessary? */
#define _STAGE_BATCH_CLR(_pkt)                  \
do {                                            \
} while (0)


/**
 * Check the input ring for a batch of packets and stage the batch.
 * "Staging the batch" consists of constructing messages to the "send_desc"
 * and the "issue_dma" blocks, and enqueuing these messages in the appropriate
 * rings.
 * This method may not block the thread as send_desc runs on the same
 * threads.  The "may_poll" ordering signal serves a dual function of
 * throttling the input ring polling when it tests empty, and providing
 * a bypass pathway.
 * NB: This method has a path that does not swap.
 */
void
stage_batch()
{
    __gpr unsigned int queue;
    unsigned int eop, up;
    struct nfd_out_data_batch_msg data_batch_msg = {0};
    struct nfd_out_desc_batch_msg desc_batch_tmp;
    struct nfd_out_data_dma_info data_batch_tmp;
    unsigned int desc_batch_index;

    if (signal_test(&may_poll)) {
        __critical_path();

       /* Check ordering requirements */
        reorder_test_swap(&get_order_sig);

        reorder_done(NFD_OUT_STAGE_START_CTX, &get_order_sig);

        __mem_ring_get_freely(in_ring_num, in_ring_addr, &in_batch,
                              sizeof in_batch, sizeof in_batch,
                              sig_done, &get_sig);
    } else {
        if (stage_wait_msk & __signals(&get_sig)) {
            /* The next stage is waiting for a batch, but we can't poll,
             * so abort without trying to process the batch either.
             * If a batch was not processed previously due to the sequence
             * number test, we will fall through and repeat the test. */
            return;
        }
    }


    /* Start of put_order_sig reorder stage
     * If we reach this point, there must be a get pending,
     * so the method will complete shortly */
    __asm {
        ctx_arb[--], defer[1];
        local_csr_wr[NFP_MECSR_ACTIVE_CTX_WAKEUP_EVENTS>>2, stage_wait_msk];
    }

    __implicit_read(&get_sig);
    __implicit_read(&put_order_sig);

    if (batch_issued == batch_safe || nn_ring_full()) {
        /* Recompute safe sequence number and clear the wait mask
         * to allow the test to execute again.
         * NB: the next thread cannot continue yet as put_order_sig
         * has not been issued. */
        batch_safe = desc_batch_served + NFD_OUT_DESC_BATCH_RING_BAT;
        stage_wait_msk = 0;

        return;
    } else {
        /* Mark the do nothing path critical */
        __critical_path();
    }

    /* Reset the wait mask */
    stage_wait_msk = __signals(&get_sig, &put_order_sig);

    /*
     * Increment batch_issued upfront to avoid ambiguity about
     * sequence number zero
     */
    batch_issued++;

    /* Setup batch messages */
    if (in_batch.pkt3.cpp.__raw[0] != 0) {
        /* XXX this is the critical path, but it may not be guaranteed to get
         * cleared unless there are 3 packets in the batch. */
        /* We have a full batch */
        __critical_path();
        data_batch_msg.num = 4;
        desc_batch_tmp.__raw = 4;
        nn_ring_put(data_batch_msg.__raw);

        _STAGE_BATCH_PROC(0);
        _STAGE_BATCH_PROC(1);
        _STAGE_BATCH_PROC(2);
        _STAGE_BATCH_PROC(3);

    } else if (in_batch.pkt0.cpp.__raw[0] == 0) {
        /* Handle an empty queue */

        /* Delay this context from polling again */
        reorder_future_sig(&may_poll, NFD_OUT_STAGE_WAIT_CYCLES);

        /* Allow next CTX to process a batch */
        reorder_done(NFD_OUT_STAGE_START_CTX, &put_order_sig);

        /* We don't issue this batch after all. We haven't swapped, so
         * can just decrement batch_issued again to correct it. */
        batch_issued--;

        /* Skip adding work to CLS and NN rings */
        return;

    } else if (in_batch.pkt1.cpp.__raw[0] == 0) {
        /* Batch of 1 */
        data_batch_msg.num = 1;
        desc_batch_tmp.__raw = 1;
        nn_ring_put(data_batch_msg.__raw);

        _STAGE_BATCH_PROC(0);

        _STAGE_BATCH_CLR(1);
        _STAGE_BATCH_CLR(2);
        _STAGE_BATCH_CLR(3);

    } else if (in_batch.pkt2.cpp.__raw[0] == 0) {
        /* Batch of 2 */
        data_batch_msg.num = 2;
        desc_batch_tmp.__raw = 2;
        nn_ring_put(data_batch_msg.__raw);

        _STAGE_BATCH_PROC(0);
        _STAGE_BATCH_PROC(1);

        _STAGE_BATCH_CLR(2);
        _STAGE_BATCH_CLR(3);

    } else {
        /* Batch of 3 */
        data_batch_msg.num = 3;
        desc_batch_tmp.__raw = 3;
        nn_ring_put(data_batch_msg.__raw);

        _STAGE_BATCH_PROC(0);
        _STAGE_BATCH_PROC(1);
        _STAGE_BATCH_PROC(2);

        _STAGE_BATCH_CLR(3);

    }

    /* Enqueue desc batch message */
    desc_batch_index = batch_issued & (NFD_OUT_DESC_BATCH_RING_BAT - 1);
    desc_batch_msg[desc_batch_index] = desc_batch_tmp;

    /* Allow this thread to poll again on its next turn */
    reorder_self(&may_poll);

    /* Allow next CTX to process a batch */
    reorder_done(NFD_OUT_STAGE_START_CTX, &put_order_sig);
}


/**
 * Perform once off, CTX0-only initialisation of sequence number autopushes
 */
void
distr_seqn_setup_shared()
{
    dma_seqn_ap_setup(NFD_OUT_DESC_EVENT_FILTER, NFD_OUT_DESC_EVENT_FILTER,
                      NFD_OUT_DESC_EVENT_TYPE, &desc_dma_event_xfer,
                      &desc_dma_event_sig);

    dma_seqn_ap_setup(NFD_OUT_DATA_EVENT_FILTER, NFD_OUT_DATA_EVENT_FILTER,
                      NFD_OUT_DATA_EVENT_TYPE, &data_dma_event_xfer,
                      &data_dma_event_sig);
}


/**
 * Check autopushes, compute sequence numbers, and reflect to issue_dma ME
 */
void
distr_seqn()
{
    if (signal_test(&desc_dma_event_sig)) {
        dma_seqn_advance(&desc_dma_event_xfer, &desc_dma_compl);

        desc_dma_safe = (desc_dma_compl + NFD_OUT_DESC_MAX_IN_FLIGHT -
                         NFD_OUT_MAX_BATCH_SZ);

        event_cls_autopush_filter_reset(
            NFD_OUT_DESC_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            NFD_OUT_DESC_EVENT_FILTER);
        __implicit_write(&desc_dma_event_sig);
    }

    if (signal_test(&data_dma_event_sig)) {
        __implicit_read(&nfd_out_data_compl_refl_out);

        dma_seqn_advance(&data_dma_event_xfer, &data_dma_compl);

        /* Mirror to remote ME */
        nfd_out_data_compl_refl_out = data_dma_compl;
        reflect_data(NFD_OUT_DATA_DMA_ME,
                     __xfer_reg_number(&nfd_out_data_compl_refl_in,
                                       NFD_OUT_DATA_DMA_ME),
                     __signal_number(&nfd_out_data_compl_refl_sig,
                                     NFD_OUT_DATA_DMA_ME),
                     &nfd_out_data_compl_refl_out,
                     sizeof nfd_out_data_compl_refl_out);

        event_cls_autopush_filter_reset(
            NFD_OUT_DATA_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            NFD_OUT_DATA_EVENT_FILTER);
        __implicit_write(&data_dma_event_sig);
    }
}

/**
 * Perform once off, CTX0-only initialisation of the send_desc DMA config
 */
void
send_desc_setup_shared()
{
    struct pcie_dma_cfg_one cfg;

    /*
     * Set up RX_FL_CFG_REG DMA Config Register
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
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_OUT_DESC_CFG_REG, cfg);
}


/**
 * Perform per CTX initialisation of the "send_desc" DMA config
 */
void
send_desc_setup()
{
    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, and PCIe address will be
     * overwritten per transaction.
     * For dma_mode, we technically only want to overwrite the "source"
     * field, i.e. 12 of the 16 bits.
     */
    descr_tmp.length = sizeof(struct nfd_out_rx_desc) - 1;
    descr_tmp.rid_override = 1;
    descr_tmp.trans_class = 0;
    descr_tmp.cpp_token = 0;
    descr_tmp.dma_cfg_index = NFD_OUT_DESC_CFG_REG;
    descr_tmp.cpp_addr_hi = (((unsigned long long) NFD_EMEM(PCIE_ISL) >> 32) &
                             0xFF);

    send_desc_addr_lo = ((unsigned long long) NFD_RING_BASE(PCIE_ISL, nfd_out) &
                         0xffffffff);
}


/**
 * Setup the RX descriptor DMA from the PCI.OUT input ring
 * "send_desc_off" must be updated even if we don't actually DMA
 * the descriptor for a given packet.
 */
#define SEND_DESC_PROC(_pkt)                                            \
do {                                                                    \
    if (msg.send_pkt##_pkt) {                                           \
    __critical_path();                                                  \
                                                                        \
    /* Increment fl_cache_dma_seq_issued upfront */                     \
    /* to avoid ambiguity about sequence number zero */                 \
    desc_dma_issued++;                                                  \
                                                                        \
    queue = msg.queue_pkt##_pkt;                                        \
    pcie_addr_off = (queue_data[queue].rx_w &                           \
                     queue_data[queue].ring_sz_msk);                    \
    pcie_addr_off = pcie_addr_off * sizeof(struct nfd_out_rx_desc);     \
    queue_data[queue].rx_w++;                                           \
    descr_tmp.pcie_addr_hi = queue_data[queue].ring_base_hi;            \
    descr_tmp.pcie_addr_lo = (queue_data[queue].ring_base_lo +          \
                              pcie_addr_off);                           \
    descr_tmp.rid = queue_data[queue].requester_id;                     \
                                                                        \
    descr_tmp.cpp_addr_lo = send_desc_addr_lo | send_desc_off;          \
    pcie_dma_set_event(&descr_tmp, NFD_OUT_DESC_EVENT_TYPE, desc_dma_issued); \
                                                                        \
    dma_out.pkt##_pkt = descr_tmp;                                      \
    __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt, NFD_OUT_DESC_DMA_QUEUE, \
                   sig_done, &desc_sig##_pkt);                          \
    } else {                                                            \
        /* Don't wait on the DMA signal */                              \
        desc_dma_wait_msk &= ~__signals(&desc_sig##_pkt);               \
    }                                                                   \
    /* Increment send_desc_off whether we send the descriptor or not. */ \
    /* The descriptor still occupied a space in the input ring. */      \
    send_desc_off += sizeof(struct nfd_out_input);                      \
    send_desc_off &= (NFD_OUT_RING_SZ - 1);                             \
} while (0)


/**
 * Remove "desc_sigX" for the packet from the wait mask
 */
#define SEND_DESC_CLR(_pkt)                              \
do {                                                     \
    desc_dma_wait_msk &= ~__signals(&desc_sig##_pkt);    \
} while (0)


/**
 * Check for completed DMA batches and send the RX descriptors
 * "send_desc" has no ordering requirements and will always swap.
 */
void
send_desc()
{
    struct nfd_out_desc_batch_msg msg;
    unsigned int queue;
    unsigned int pcie_addr_off;
    unsigned int desc_batch_index;
    int test_safe;

    /* Wait for previous DMAs to be enqueued */
    __asm {
        ctx_arb[--], defer[1];
        local_csr_wr[NFP_MECSR_ACTIVE_CTX_WAKEUP_EVENTS>>2, desc_dma_wait_msk];
    }

    __implicit_read(&desc_sig0);
    __implicit_read(&desc_sig1);
    __implicit_read(&desc_sig2);
    __implicit_read(&desc_sig3);

    /* XXX THSDK-1813 workaround */
    test_safe = desc_dma_safe - desc_dma_issued;
    if ((desc_batch_served != data_dma_compl) && (test_safe > 0)) {
       __critical_path();

        desc_dma_wait_msk = __signals(&desc_sig0, &desc_sig1, &desc_sig2,
                                      &desc_sig3);

        /* We have a batch to process and resources to process it */

        /*
         * Increment desc_batch_served upfront to avoid ambiguity about
         * sequence number zero
         */
        desc_batch_served++;

        desc_batch_index = desc_batch_served & (NFD_OUT_DESC_BATCH_RING_BAT - 1);
        msg = desc_batch_msg[desc_batch_index];

        switch (msg.num) {
        case 4:
            __critical_path();
            /* Handle full batch */

            SEND_DESC_PROC(0);
            SEND_DESC_PROC(1);
            SEND_DESC_PROC(2);
            SEND_DESC_PROC(3);

            break;

        case 3:
            SEND_DESC_PROC(0);
            SEND_DESC_PROC(1);
            SEND_DESC_PROC(2);

            SEND_DESC_CLR(3);
            break;

        case 2:
            SEND_DESC_PROC(0);
            SEND_DESC_PROC(1);

            SEND_DESC_CLR(2);
            SEND_DESC_CLR(3);
            break;

        default:
            SEND_DESC_PROC(0);

            SEND_DESC_CLR(1);
            SEND_DESC_CLR(2);
            SEND_DESC_CLR(3);
            break;
        }
   } else {
       /* There are no DMAs to be enqueued */
       desc_dma_wait_msk = 0;
   }
}
