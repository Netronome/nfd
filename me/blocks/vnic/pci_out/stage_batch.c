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
#include <nfp/pcie.h>
#include <std/event.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_event.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/pci_out.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/dma_seqn.h>
#include <vnic/utils/nn_ring.h>
#include <vnic/utils/ordering.h>
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
extern __shared __lmem struct nfd_out_queue_info *queue_data;
extern __shared __gpr struct qc_bitmask urgent_bmsk;


/*
 * Rings and queues
 */
#define NFD_OUT_RING_INIT_IND2(_isl, _emem)                         \
    ASM(.alloc_mem nfd_out_ring_mem##_isl _emem global              \
        NFD_OUT_RING_SZ NFD_OUT_RING_SZ)                            \
    ASM(.init_mu_ring nfd_out_ring_num##_isl##0 nfd_out_ring_mem##_isl)
#define NFD_OUT_RING_INIT_IND1(_isl, _emem) NFD_OUT_RING_INIT_IND2(_isl, _emem)
#define NFD_OUT_RING_INIT_IND0(_isl)                    \
    NFD_OUT_RING_INIT_IND1(_isl, NFD_PCIE##_isl##_EMEM)
#define NFD_OUT_RING_INIT(_isl) NFD_OUT_RING_INIT_IND0(_isl)

NFD_OUT_RING_INIT(PCIE_ISL);

#define NFD_OUT_SEND_ADDR_IND(_isl) _link_sym(nfd_out_ring_mem##_isl)
#define NFD_OUT_SEND_ADDR(_isl) NFD_OUT_SEND_ADDR_IND(_isl)


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
__shared __gpr unsigned int desc_batch_compl = 0;
__shared __gpr unsigned int desc_dma_inc = 0;
__shared __gpr unsigned int desc_dma_inc_safe = 0;

__shared __gpr unsigned int full_cnt0 = 0;
__shared __gpr unsigned int full_cnt1 = 0;

__shared __gpr unsigned int send_desc_msg_addr;
__shared __gpr unsigned int send_desc_msg_sz_msk = NFD_OUT_RING_SZ - 1;
__shared __gpr unsigned int send_desc_cpp_msk = 0xfc000fff;
__shared __gpr unsigned int inc_sent_msg_addr;

unsigned int next_ctx;


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
    local_csr_write(local_csr_cmd_indirect_ref_0, indirect.__raw);

    /* Currently just support reflect_write_sig_remote */
    __asm {
        alu[--, --, b, 1, <<OV_SIG_NUM];
        ct[reflect_write_sig_remote, *src_xfer, addr, 0, \
           __ct_const_val(count)], indirect_ref;
    };
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
    in_ring_num = NFD_RING_LINK(PCIE_ISL, nfd_out, 0);
    in_ring_addr = (unsigned long long) NFD_EMEM_LINK(PCIE_ISL) >> 8;

    /* Allow polling initially */
    reorder_self(&may_poll);

    /* Wait on get_sig and put_order_sig.  There is no previous
     * put that needs to complete, so it is removed at start. */
    stage_wait_msk = __signals(&get_sig, &put_order_sig);

    /* Initialise the "next_ctx" message used for ordering signals. */
    next_ctx = reorder_get_next_ctx(NFD_OUT_STAGE_START_CTX);
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
__forceinline void
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
    /* XXX: Assess the impact of NN_FULL testing on performance
     * and consider alternative rings for batch data */
    while (nn_ring_full()) {
        ctx_swap();
    }

    nn_ring_put(msg->__raw[0]);
    nn_ring_put(msg->__raw[1]);

    while (nn_ring_full()) {
        ctx_swap();
    }

    nn_ring_put(msg->__raw[2]);
    nn_ring_put(msg->__raw[3]);
}


/**
 * Construct messages for the "data_batch_ring" and the "desc_batch_ring".
 *
 * The "desc_batch_ring" only needs the queue number from the RX descriptor.
 * "send_pktX" is set if the queue is up.  It determines whether the
 * RX descriptor is DMAed to the host and the NFD_OUT_ATOMICS_SENT is
 * incremented or not.
 *
 * The "data_batch_ring" needs the CPP descriptor and the address of a
 * freelist descriptor to use.  If the queue is down, the freelist index
 * should not be advanced.  The "reserved" bit in the CPP descriptor is
 * must be zero when received from the app, and is set to one if the queue
 * is up.  This will allow the data DMA(s) to go to the host.  The buffer
 * is always freed whether the queue is down or not.
 */
/* XXX unclear why we can't accessin_batch.pkt3.rxd.__raw[0] in ASM  */
#define _STAGE_BATCH_PROC(_pkt, _num)                                   \
do {                                                                    \
    rx_desc_info = in_batch.pkt##_pkt##.rxd.__raw[0];                   \
    __asm {                                                             \
        __asm { alu[queue, NFD_OUT_RX_DESC_QUEUE_msk, and, rx_desc_info, \
                    >>NFD_OUT_RX_DESC_QUEUE_shf] }                      \
        __asm { alu[queue_ptr, --, b, queue, <<NFD_OUT_QUEUE_INFO_SZ_lg2] } \
        __asm { local_csr_wr[active_lm_addr_2, queue_ptr] }             \
        __asm { alu[desc_batch_tmp, desc_batch_tmp, or, queue,          \
                    <<NFD_OUT_DESC_QUEUE_PKT##_pkt##_shf] }             \
        __asm { alu[fl_index, fl_cache_mem_addr_lo, or, queue,          \
                    <<NFD_OUT_FL_CACHE_Q_SZ_lg2] }                      \
                                                                        \
        __asm { br_inp_state[nn_full, sb_full1##_pkt##_num] }           \
    sb_cont1##_pkt##_num:                                               \
        __asm { alu[up, 1, AND, *l$index2[NFD_OUT_QUEUE_INFO_BITFIELD], \
                    >>NFD_OUT_QUEUE_INFO_UP_shf] }                      \
        /* Check for urgent queues */                                   \
        __asm { alu[urgent, *l$index2[NFD_OUT_QUEUE_INFO_FLA], -,       \
                    NFD_OUT_FL_SOFT_THRESH] }                           \
        __asm { alu[urgent, urgent, -, *l$index2[NFD_OUT_QUEUE_INFO_FLU]] } \
        __asm { blt[sb_urgent##_pkt##_num] }                            \
    sb_cont_urgent##_pkt##_num:                                         \
                                                                        \
        __asm { alu[*n$index++, in_batch + (0 + 16 * _pkt), or, up,     \
                    <<NFD_OUT_CPP_CTM_RESERVED_shf] }                   \
        __asm { alu[*n$index++, --, b, in_batch + (4 + 16 * _pkt)] }    \
        __asm { alu[desc_batch_tmp, desc_batch_tmp, or, up,             \
                    <<NFD_OUT_DESC_SEND_PKT##_pkt##_shf] }              \
        __asm { ld_field[rx_desc_info, 4,                               \
                         *l$index2[NFD_OUT_QUEUE_INFO_BITFIELD],        \
                         >>(NFD_OUT_QUEUE_INFO_RID_shf -                \
                            NFD_OUT_RX_DESC_QUEUE_shf)] }               \
                                                                        \
        __asm { br_inp_state[nn_full, sb_full2##_pkt##_num] }           \
    sb_cont2##_pkt##_num:                                               \
                                                                        \
        __asm { alu[*n$index++, --, b, rx_desc_info] }                  \
        __asm { alu[used_seq, (NFD_OUT_FL_BUFS_PER_QUEUE - 1), and,     \
                    *l$index2[NFD_OUT_QUEUE_INFO_FLU]] }                \
        __asm { alu[*n$index++, fl_index, or, used_seq, <<3] }          \
        __asm { alu[*l$index2[NFD_OUT_QUEUE_INFO_FLU],                  \
                    *l$index2[NFD_OUT_QUEUE_INFO_FLU], +, up] }         \
        __asm { br[sb_end##_pkt##_num] }                                \
                                                                        \
    sb_urgent##_pkt##_num:                                              \
        __asm { alu[--, queue, and, 32] }                               \
        __asm { alu[urgent, 0, or, up, <<indirect], no_cc }             \
        __asm { bne[sb_urgent_hi##_pkt##_num] }                         \
        __asm { alu[urgent_bmsk.bmsk_lo, urgent_bmsk.bmsk_lo, or, urgent] } \
        __asm { br[sb_cont_urgent##_pkt##_num] }                        \
    sb_urgent_hi##_pkt##_num:                                           \
        __asm { alu[urgent_bmsk.bmsk_hi, urgent_bmsk.bmsk_hi, or, urgent] } \
        __asm { br[sb_cont_urgent##_pkt##_num] }                        \
                                                                        \
    sb_full1##_pkt##_num:                                               \
        __asm { alu[full_cnt1, full_cnt1, +, 1] }                       \
        __asm { local_csr_wr[local_csr_mailbox_1, full_cnt1] }     \
        __asm { ctx_arb[voluntary] }                                    \
        __asm { br_inp_state[nn_full, sb_full1##_pkt##_num] }           \
        __asm { br[sb_cont1##_pkt##_num] }                              \
                                                                        \
    sb_full2##_pkt##_num:                                               \
        __asm { alu[full_cnt1, full_cnt1, +, 1] }                       \
        __asm { local_csr_wr[local_csr_mailbox_1, full_cnt1] }     \
        __asm { ctx_arb[voluntary] }                                    \
        __asm { br_inp_state[nn_full, sb_full2##_pkt##_num] }           \
        __asm { br[sb_cont2##_pkt##_num] }                              \
    sb_end##_pkt##_num:                                                 \
    }                                                                   \
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
__forceinline void
stage_batch()
{
    __gpr unsigned int queue;
    unsigned int up, used_seq;
    unsigned int rx_desc_info;
    unsigned int queue_ptr;
    unsigned int fl_index;
    unsigned int urgent;
    /* __shared __lmem struct nfd_out_queue_info *queue_ptr; */
    struct nfd_out_data_batch_msg data_batch_msg = {0};
    struct nfd_out_desc_batch_msg desc_batch_tmp;
    struct nfd_out_data_dma_info data_batch_tmp;
    unsigned int desc_batch_index;

    if (signal_test(&may_poll)) {
        __critical_path();

       /* Check ordering requirements
        * Assume that we will need to wait */
       /* reorder_test_swap(&get_order_sig); */
        __wait_for_all(&get_order_sig);

        reorder_done_opt(&next_ctx, &get_order_sig);

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
        local_csr_wr[local_csr_active_ctx_wakeup_events, stage_wait_msk];
    }

    __implicit_read(&get_sig);
    __implicit_read(&put_order_sig);

    if (batch_issued == batch_safe || nn_ring_full()) {
        /* Recompute safe sequence number and clear the wait mask
         * to allow the test to execute again.
         * NB: the next thread cannot continue yet as put_order_sig
         * has not been issued. */
        batch_safe = desc_batch_compl + NFD_OUT_DESC_BATCH_RING_BAT;
        stage_wait_msk = 0;

        if (nn_ring_full()) {
            full_cnt0++;
            local_csr_write(local_csr_mailbox_0, full_cnt0);
        }

        return;
    } else {
        /* Mark the do nothing path critical */
        __critical_path();
    }

    /* Reset the wait mask */
    stage_wait_msk = __signals(&get_sig, &put_order_sig);

    /*
     * Increment batch_issued upfront to avoid ambiguity about
     * sequence number zero, but place the batch message at the
     * pre-incremented index so that other blocks can increment their
     * indices in the usage shadow of the desc batch LM pointer.
     */
    desc_batch_index = batch_issued & (NFD_OUT_DESC_BATCH_RING_BAT - 1);
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

        _STAGE_BATCH_PROC(0, 4);
        _STAGE_BATCH_PROC(1, 4);
        _STAGE_BATCH_PROC(2, 4);
        _STAGE_BATCH_PROC(3, 4);

    } else {
        if (in_batch.pkt0.cpp.__raw[0] == 0) {
            /* Handle an empty queue */

            /* Delay this context from polling again */
            reorder_future_sig(&may_poll, NFD_OUT_STAGE_WAIT_CYCLES);

            /* Allow next CTX to process a batch */
            reorder_done_opt(&next_ctx, &put_order_sig);

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

            _STAGE_BATCH_PROC(0, 1);

            _STAGE_BATCH_CLR(1);
            _STAGE_BATCH_CLR(2);
            _STAGE_BATCH_CLR(3);

        } else if (in_batch.pkt2.cpp.__raw[0] == 0) {
            /* Batch of 2 */
            data_batch_msg.num = 2;
            desc_batch_tmp.__raw = 2;
            nn_ring_put(data_batch_msg.__raw);

            _STAGE_BATCH_PROC(0, 2);
            _STAGE_BATCH_PROC(1, 2);

            _STAGE_BATCH_CLR(2);
            _STAGE_BATCH_CLR(3);

        } else {
            /* Batch of 3 */
            data_batch_msg.num = 3;
            desc_batch_tmp.__raw = 3;
            nn_ring_put(data_batch_msg.__raw);

            _STAGE_BATCH_PROC(0, 3);
            _STAGE_BATCH_PROC(1, 3);
            _STAGE_BATCH_PROC(2, 3);

            _STAGE_BATCH_CLR(3);

        }
    }

    /* Enqueue desc batch message */
    desc_batch_msg[desc_batch_index] = desc_batch_tmp;

    /* Allow this thread to poll again on its next turn */
    reorder_self(&may_poll);

    /* Allow next CTX to process a batch */
    reorder_done_opt(&next_ctx, &put_order_sig);
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
__forceinline void
distr_seqn()
{
    /* Service data_dma_compl first in case issue_dma() is waiting */
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

    } else {
        /* Swap to give other threads a chance to run */
        ctx_swap();
    }

    /* desc_dma_compl is only used locally, so less urgent */
    if (signal_test(&desc_dma_event_sig)) {
        dma_seqn_advance(&desc_dma_event_xfer, &desc_dma_compl);

        /* We need to know that a whole batch has completed before
         * we send wptr updates for the descriptors. It is always safe
         * to process up to NFD_OUT_MAX_BATCH_SZ behind the desc_dma_compl
         * count. */
        desc_dma_inc_safe = desc_dma_compl - NFD_OUT_MAX_BATCH_SZ;
        if (desc_dma_compl == desc_dma_issued) {
            /* In the special case where the completed count has caught
             * the issued count, we know desc_dma_compl points to the
             * end of a batch because the issued count tracks full batches.
             * This allows us to handle the last packets when traffic stops. */
            desc_dma_inc_safe = desc_dma_compl;
        }

        desc_dma_safe = (desc_dma_compl + NFD_OUT_DESC_MAX_IN_FLIGHT -
                         NFD_OUT_MAX_BATCH_SZ);

        event_cls_autopush_filter_reset(
            NFD_OUT_DESC_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            NFD_OUT_DESC_EVENT_FILTER);
        __implicit_write(&desc_dma_event_sig);

    } else {
        /* Swap to give other threads a chance to run */
        ctx_swap();
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

    /* XXX tidy up */
    send_desc_msg_addr = &desc_batch_msg[desc_batch_served];
    inc_sent_msg_addr = &desc_batch_msg[desc_batch_compl];
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
    descr_tmp.cpp_addr_hi =
        (((unsigned long long) NFD_EMEM_LINK(PCIE_ISL)) >> 32) & 0xFF;
    pcie_dma_set_event(&descr_tmp, NFD_OUT_DESC_EVENT_TYPE, 0);

    send_desc_addr_lo = ((unsigned long long) NFD_OUT_SEND_ADDR(PCIE_ISL) &
                         0xffffffff);
}

/**
 * Setup the RX descriptor DMA from the PCI.OUT input ring
 * "send_desc_off" must be updated even if we don't actually DMA
 * the descriptor for a given packet.
 */
#define SEND_DESC_PROC(_pkt)                                            \
do {                                                                    \
    if (msg & (1 << NFD_OUT_DESC_SEND_PKT##_pkt##_shf)) {               \
        __asm { alu[queue, NFD_OUT_DESC_QUEUE_msk, and, msg,            \
                    >>NFD_OUT_DESC_QUEUE_PKT##_pkt##_shf] }             \
        __asm { alu[queue, --, b, queue, <<NFD_OUT_QUEUE_INFO_SZ_lg2] } \
        __asm { local_csr_wr[(local_csr_active_lm_addr_2), queue] } \
        __asm { alu[desc_dma_issued, desc_dma_issued, +, 1] }           \
        __asm { alu[send_desc_off, send_desc_off, and,                  \
                    send_desc_msg_sz_msk] }                             \
        __asm { alu[dma_out + (16 * _pkt), send_desc_addr_lo, or,       \
                    send_desc_off] }                                    \
                                                                        \
        /* PCIe addr lo */                                              \
        __asm { alu[pcie_addr_lo, --, b,                                \
                    *l$index2[NFD_OUT_QUEUE_INFO_RING_SZ_MSK],          \
                    <<NFD_OUT_FL_ENTRY_SZ_lg2] }                        \
        __asm { alu[pcie_addr_lo, pcie_addr_lo, and,                    \
                    *l$index2[NFD_OUT_QUEUE_INFO_RXW],                  \
                    <<NFD_OUT_FL_ENTRY_SZ_lg2] }                        \
        __asm { alu[*l$index2[NFD_OUT_QUEUE_INFO_RXW],                  \
                    *l$index2[NFD_OUT_QUEUE_INFO_RXW], +, 1] }          \
        __asm { alu[dma_out + (8 + 16 * _pkt), pcie_addr_lo, +,         \
                    *l$index2[NFD_OUT_QUEUE_INFO_RING_LO]] }            \
                                                                        \
        /* PCIe addr hi */                                              \
        __asm { ld_field[descr_tmp + 12, 0001,                          \
                         *l$index2[NFD_OUT_QUEUE_INFO_BITFIELD]] }      \
        __asm { alu[dma_out + (12 + 16 * _pkt), descr_tmp + 12, or,     \
                    *l$index2[NFD_OUT_QUEUE_INFO_BITFIELD],             \
                    >>(NFD_OUT_QUEUE_INFO_RID_shf -                     \
                       NFP_PCIE_DMA_CMD_RID_shf)] }                     \
                                                                        \
        /* CPP addr hi */                                               \
        __asm { alu[descr_tmp + 4, descr_tmp + 4, and,                  \
                    send_desc_cpp_msk] }                                \
        __asm { alu[dma_event, desc_dma_issued, and, send_desc_cpp_msk] } \
        __asm { alu[dma_out + (4 + 16 * _pkt), descr_tmp + 4, or,       \
                    dma_event, <<NFP_PCIE_DMA_CMD_DMA_MODE_shf] }       \
                                                                        \
        /* Issue DMA */                                                 \
        __asm { pcie[write_pci, dma_out + (16 * _pkt), addr_hi, <<8,    \
                     addr_lo, 4], sig_done[desc_sig##_pkt] }            \
                                                                        \
    } else {                                                            \
        desc_dma_wait_msk &= ~__signals(&desc_sig##_pkt);               \
    }                                                                   \
    __asm { alu[send_desc_off, send_desc_off, +,                        \
                sizeof(struct nfd_out_input)] }                         \
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
__forceinline void
send_desc()
{
    /* struct nfd_out_desc_batch_msg msg; */
    unsigned int msg;
    unsigned int queue;
    unsigned int queue_base;
    unsigned int pcie_addr_lo;
    unsigned int dma_event;
    unsigned int desc_batch_index;
    int test_safe;
    unsigned int addr_hi = PCIE_ISL << 30;
    unsigned int addr_lo = NFD_OUT_DESC_DMA_QUEUE;

    /* ctassert(&queue_data[0] == 0); */

    /* Wait for previous DMAs to be enqueued */
    __asm {
        ctx_arb[--], defer[1];
        local_csr_wr[local_csr_active_ctx_wakeup_events, desc_dma_wait_msk];
    }

    __implicit_read(&desc_sig0);
    __implicit_read(&desc_sig1);
    __implicit_read(&desc_sig2);
    __implicit_read(&desc_sig3);

    /* XXX THSDK-1813 workaround */
    test_safe = desc_dma_safe - desc_dma_issued;
    if ((desc_batch_served != data_dma_compl) && (test_safe > 0)) {

        desc_dma_wait_msk = __signals(&desc_sig0, &desc_sig1, &desc_sig2,
                                      &desc_sig3);

        /* We have a batch to process and resources to process it */

        local_csr_write(local_csr_active_lm_addr_3, send_desc_msg_addr);
        /*
         * Increment desc_batch_served upfront to avoid ambiguity about
         * sequence number zero
         */
        desc_batch_served++;
        desc_batch_index = (desc_batch_served &
                            (NFD_OUT_DESC_BATCH_RING_BAT - 1));
        send_desc_msg_addr = &desc_batch_msg[desc_batch_index];

        __asm alu[msg, --, B, *l$index3];

        if ((msg & 4) == 0) {
            if ((msg & NFD_OUT_DESC_NUM_msk) == 3) {
                SEND_DESC_PROC(0);
                SEND_DESC_PROC(1);
                SEND_DESC_PROC(2);

                SEND_DESC_CLR(3);

            } else if ((msg & NFD_OUT_DESC_NUM_msk) == 2) {
                SEND_DESC_PROC(0);
                SEND_DESC_PROC(1);

                SEND_DESC_CLR(2);
                SEND_DESC_CLR(3);

            } else {
                SEND_DESC_PROC(0);

                SEND_DESC_CLR(1);
                SEND_DESC_CLR(2);
                SEND_DESC_CLR(3);

            }
        } else {
           /* Handle full batch */
            __critical_path();

            SEND_DESC_PROC(0);
            SEND_DESC_PROC(1);
            SEND_DESC_PROC(2);
            SEND_DESC_PROC(3);
        }
    } else {
       /* There are no DMAs to be enqueued */
       desc_dma_wait_msk = 0;
   }
}


/**
 * Increment the count of packets sent successfully.
 */
#define INC_SENT_PROC(_pkt)                                             \
do {                                                                    \
    if (msg & (1 << NFD_OUT_DESC_SEND_PKT##_pkt##_shf)) {               \
    __critical_path();                                                  \
                                                                        \
    /* Increment desc_dma_inc upfront */                                \
    /* to avoid ambiguity about sequence number zero */                 \
    desc_dma_inc++;                                                     \
                                                                        \
    addr = (NFD_OUT_DESC_QUEUE_msk &                                    \
            (msg >> NFD_OUT_DESC_QUEUE_PKT##_pkt##_shf ));              \
    addr = (addr * NFD_OUT_ATOMICS_SZ | NFD_OUT_ATOMICS_SENT |          \
            NFD_OUT_CREDITS_BASE);                                      \
    __asm { mem[incr, --, 0, addr] }                                    \
                                                                        \
    }                                                                   \
} while (0)



/**
 * Placeholder for processing smaller batches
 */
#define INC_SENT_CLR(_pkt)                               \
do {                                                     \
} while (0)


__forceinline void
inc_sent()
{
    unsigned int msg;
    unsigned int addr;
    unsigned int desc_batch_index;
    int test_safe;

    test_safe = desc_dma_inc_safe - desc_dma_inc;
    if (test_safe > 0) {

        local_csr_write(local_csr_active_lm_addr_3, inc_sent_msg_addr);

        /*
         * Increment desc_batch_served upfront to avoid ambiguity about
         * sequence number zero
         */
        desc_batch_compl++;
        desc_batch_index = desc_batch_compl & (NFD_OUT_DESC_BATCH_RING_BAT - 1);
        inc_sent_msg_addr = &desc_batch_msg[desc_batch_index];

        __asm alu[msg, --, B, *l$index3];

        if ((msg & 4) == 0) {
            if ((msg & NFD_OUT_DESC_NUM_msk) == 3) {
                INC_SENT_PROC(0);
                INC_SENT_PROC(1);
                INC_SENT_PROC(2);

                INC_SENT_CLR(3);

            } else if ((msg & NFD_OUT_DESC_NUM_msk) == 2) {
                INC_SENT_PROC(0);
                INC_SENT_PROC(1);

                INC_SENT_CLR(2);
                INC_SENT_CLR(3);

            } else {
                INC_SENT_PROC(0);

                INC_SENT_CLR(1);
                INC_SENT_CLR(2);
                INC_SENT_CLR(3);

            }
        } else {
            __critical_path();
            /* Handle full batch */

            INC_SENT_PROC(0);
            INC_SENT_PROC(1);
            INC_SENT_PROC(2);
            INC_SENT_PROC(3);

        }
    }
}
