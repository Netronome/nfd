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


/*
 * stage_batch variables
 */
__shared __gpr unsigned int batch_issued = 0;

static __xread struct _input_batch in_batch;
SIGNAL get_sig;

SIGNAL get_order_sig, put_order_sig,  may_poll;
static SIGNAL_MASK stage_wait_msk = 0;


__shared __gpr unsigned int full_cnt0 = 0;
__shared __gpr unsigned int full_cnt1 = 0;

unsigned int next_ctx;



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
 * Construct messages for the "data_batch_ring".
 *
 * The "data_batch_ring" needs the CPP descriptor and the index of a
 * freelist descriptor to use.  If the queue is down, the freelist index
 * should not be advanced.  The "reserved" bit in the CPP descriptor is
 * must be zero when received from the app.  Currently a separate up bit
 * is set if the queue is up.  This will allow the data DMA(s) to go to
 * the host.  The buffer is always freed whether the queue is down or not.
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
        __asm { br_inp_state[nn_full, sb_full1##_pkt##_num] }           \
    sb_cont1##_pkt##_num:                                               \
                                                                        \
        /* Put the first 3 LW of input to output  */                    \
        __asm { alu[*n$index++, --, b, in_batch + (0 + 16 * _pkt)] }    \
        __asm { alu[*n$index++, --, b, in_batch + (4 + 16 * _pkt)] }    \
        __asm { alu[*n$index++, --, b, in_batch + (8 + 16 * _pkt)] }    \
                                                                        \
        /* Compute our new partial data word */                         \
        __asm { alu[up, 1, AND, *l$index2[NFD_OUT_QUEUE_INFO_BITFIELD], \
                    >>NFD_OUT_QUEUE_INFO_UP_shf] }                      \
        __asm { alu[stage_info, (NFD_OUT_FL_BUFS_PER_QUEUE - 1), and,   \
                    *l$index2[NFD_OUT_QUEUE_INFO_FLU]] }                \
        __asm { alu[*l$index2[NFD_OUT_QUEUE_INFO_FLU],                  \
                    *l$index2[NFD_OUT_QUEUE_INFO_FLU], +, up] }         \
        __asm { ld_field[stage_info, 8,                                 \
                         *l$index2[NFD_OUT_QUEUE_INFO_BITFIELD],        \
                         >>(NFD_OUT_QUEUE_INFO_RID_shf -                \
                            NFD_OUT_STAGE_INFO_RID_shf)] }              \
                                                                        \
        __asm { br_inp_state[nn_full, sb_full2##_pkt##_num] }           \
    sb_cont2##_pkt##_num:                                               \
        __asm { alu[*n$index++, --, b, in_batch + (12 + 16 * _pkt)] }   \
        __asm { alu[*n$index++, stage_info, OR, up,                     \
                    <<NFD_OUT_STAGE_INFO_UP_shf] }                      \
        __asm { br[sb_end##_pkt##_num] }                                \
                                                                        \
    sb_full1##_pkt##_num:                                               \
        __asm { alu[full_cnt1, full_cnt1, +, 1] }                       \
        __asm { local_csr_wr[local_csr_mailbox_1, full_cnt1] }          \
        __asm { ctx_arb[voluntary] }                                    \
        __asm { br_inp_state[nn_full, sb_full1##_pkt##_num] }           \
        __asm { br[sb_cont1##_pkt##_num] }                              \
                                                                        \
    sb_full2##_pkt##_num:                                               \
        __asm { alu[full_cnt1, full_cnt1, +, 1] }                       \
        __asm { local_csr_wr[local_csr_mailbox_1, full_cnt1] }          \
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
 * "Staging the batch" consists of constructing a message to the
 * "issue_dma" block, and enqueuing the message in the appropriate
 * ring.
 * This method does not block for historical reasons.  The "may_poll"
 * ordering signal serves a dual function of throttling the input ring
 * polling when it tests empty, and providing a bypass pathway.
 * NB: This method has a path that does not swap.
 */
__forceinline void
stage_batch()
{
    __gpr unsigned int queue;
    unsigned int up, used_seq;
    unsigned int rx_desc_info;
    unsigned int queue_ptr;
    unsigned int stage_info;
    /* __shared __lmem struct nfd_out_queue_info *queue_ptr; */
    struct nfd_out_data_batch_msg data_batch_msg = {0};
    struct nfd_out_data_dma_info data_batch_tmp;

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

    if (nn_ring_full()) {
        /* Clear the wait mask to allow the test to execute again.
         * NB: the next thread cannot continue yet as put_order_sig
         * has not been issued. */
        stage_wait_msk = 0;

        full_cnt0++;
        local_csr_write(local_csr_mailbox_0, full_cnt0);

        return;
    } else {
        /* Mark the do nothing path critical */
        __critical_path();
    }

    /* Reset the wait mask */
    stage_wait_msk = __signals(&get_sig, &put_order_sig);

    /*
     * Increment batch_issued upfront to avoid ambiguity about
     * sequence number zero.
     */
    batch_issued++;

    /* Setup batch messages */
    if (in_batch.pkt3.cpp.__raw[0] != 0) {
        /* XXX this is the critical path, but it may not be guaranteed to get
         * cleared unless there are 3 packets in the batch. */
        /* We have a full batch */
        __critical_path();
        data_batch_msg.num = 4;
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
            nn_ring_put(data_batch_msg.__raw);

            _STAGE_BATCH_PROC(0, 1);

            _STAGE_BATCH_CLR(1);
            _STAGE_BATCH_CLR(2);
            _STAGE_BATCH_CLR(3);

        } else if (in_batch.pkt2.cpp.__raw[0] == 0) {
            /* Batch of 2 */
            data_batch_msg.num = 2;
            nn_ring_put(data_batch_msg.__raw);

            _STAGE_BATCH_PROC(0, 2);
            _STAGE_BATCH_PROC(1, 2);

            _STAGE_BATCH_CLR(2);
            _STAGE_BATCH_CLR(3);

        } else {
            /* Batch of 3 */
            data_batch_msg.num = 3;
            nn_ring_put(data_batch_msg.__raw);

            _STAGE_BATCH_PROC(0, 3);
            _STAGE_BATCH_PROC(1, 3);
            _STAGE_BATCH_PROC(2, 3);

            _STAGE_BATCH_CLR(3);

        }
    }

    /* Allow this thread to poll again on its next turn */
    reorder_self(&may_poll);

    /* Allow next CTX to process a batch */
    reorder_done_opt(&next_ctx, &put_order_sig);
}
