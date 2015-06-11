/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/precache_bufs.c
 * @brief         Fill the local (LM) cache of MU buffers
 */

#include <vnic/shared/nfcc_chipres.h>
#include <nfp.h>
#include <assert.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>
#include <std/event.h>
#include <std/reg_utils.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_me.h>

#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/dma_seqn.h>


/* Configure *l$index3 to be a global pointer, and
 * set up a convenience define */
#define NFD_IN_BUF_STORE_PTR *l$index3
_init_csr("mecsr:CtxEnables.LMAddr3Global 1");


struct precache_bufs_state {
    unsigned int pending_fetch:1;       /* A  buffers "get" has been issued */
    unsigned int recompute_seq_safe:1;  /* The safe seqn must be recomputed */
    unsigned int spare:30;
};


NFD_BLM_Q_ALLOC(NFD_IN_BLM_POOL);

__shared __lmem unsigned int buf_store[NFD_IN_BUF_STORE_SZ];
static __shared unsigned int buf_store_start; /* Units: bytes */
static struct precache_bufs_state state = {0, 0, 0};
static SIGNAL_PAIR precache_sig;

static __xread unsigned int bufs_rd[NFD_IN_BUF_RECACHE_WM];
static unsigned int blm_queue_addr;
static unsigned int blm_queue_num;

extern __shared __gpr unsigned int data_dma_seq_issued;
__shared __gpr unsigned int data_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_served = 0;
__shared __gpr unsigned int data_dma_seq_safe = 0;

/* Signals and transfer registers for receiving DMA events */
static volatile __xread unsigned int nfd_in_data_event_xfer;
static volatile SIGNAL nfd_in_data_event_sig;

/* Signals and transfer registers for sequence number reflects */
static __xwrite unsigned int nfd_in_data_compl_refl_out = 0;
__remote volatile __xread unsigned int nfd_in_data_compl_refl_in;
__remote volatile SIGNAL nfd_in_data_compl_refl_sig;
__visible volatile __xread unsigned int nfd_in_data_served_refl_in;
__visible volatile SIGNAL nfd_in_data_served_refl_sig;


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


/*
 * buf_store is managed as a LIFO with buf_store[0] as the first-in/last-out
 * buffer.  On the NFP, *l$index and *$index support a post-increment op.
 * In order that the fast-path method precache_bufs_use() can issue just
 * this one operation, *l$index must always point to a valid buffer (provided
 * the queue tests non-empty).  Therefore, when refilling the LIFO the
 * first step is to advance *l$index to a vacant slot, then copy using
 * the post-increment op, and finally move it back to the last filled slot.
 *
 * In this scheme, buf_store[0] never holds a valid buffer.  The number
 * of buffers available is (buf_store_curr - &buf_store[1])/4.  The
 * number of buffers that can be added is (NFD_IN_BUF_STORE_SZ -
 * buf_store_curr + &buf_store[0])/4.  Refills are done if this exceeds
 * NFD_IN_BUF_RECACHE_WM/4, where the "/4" can be dropped through out.
 *
 * No count of the buffers available is maintained.  Instead, the
 * number of buffers on hand is periodically used to compute a "safe
 * sequence number" that covers availability of multiple resources.
 */


__inline unsigned int
_precache_buf_bytes_used()
{
    unsigned int ret;

    ret = local_csr_read(local_csr_active_lm_addr_3);
    ret = (sizeof(unsigned int) * NFD_IN_BUF_STORE_SZ) - (ret - buf_store_start);
    return ret;
}


__intrinsic void
_precache_bufs_copy(unsigned int num)
{
    ctassert(__is_ct_const(num));
    ctassert((num == 16) || (num == 8) || (num == 4));

    /* Move to empty slot */
    __asm alu[--, --, b, NFD_IN_BUF_STORE_PTR++];

    /* Perform bulk copy */
    switch(num) {
    case 16:
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
    case 8:
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
    case 4:
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[NFD_IN_BUF_STORE_PTR, --, b, *$index];
    }
}


/**
 * Initialise the precache_bufs block
 */
void
precache_bufs_setup()
{
    struct nfp_mecsr_ctx_enables cfg;

    /* XXX Replace with _init_csr command, once query resolved */
    cfg.__raw = local_csr_read(local_csr_ctx_enables);
    cfg.lm_addr_3_glob = 1;
    local_csr_write(local_csr_ctx_enables, cfg.__raw);

    blm_queue_addr = ((unsigned long long) NFD_IN_BLM_RADDR >> 8) & 0xff000000;
    blm_queue_num = NFD_BLM_Q_LINK(NFD_IN_BLM_POOL);

    buf_store_start = (unsigned int) &buf_store;
    local_csr_write(local_csr_active_lm_addr_3, buf_store_start);

    /* Write a magic value to buf_store[0]. This should never be
     * used and will help to identify buf_store underflows */
    buf_store[0] = 0x195fde01; /* Magically becomes 0xcafef00800 */
}


/**
 * If there is space in the local cache and no request outstanding, request
 * a batch of TX_BUF_RECACHE_WM buffers from the specified BLM queue.  If
 * there is a request outstanding check whether it returned and was filled.
 * Copy the buffers into the cache if the request succeeded. */
void
precache_bufs()
{
    /* NB: Ordering of if clauses allows back to back fetches,
     *     if buf_store is sufficiently empty */

    if (signal_test(&precache_sig.even)) {
        /* Process the fetch */
        unsigned int bufs_rd_off;

        /* Prepare T-INDEX early so usage shadow is filled easily */
        bufs_rd_off = MECSR_XFER_INDEX(__xfer_reg_number(bufs_rd));
        local_csr_write(local_csr_t_index, bufs_rd_off);
        state.pending_fetch = 0;

        if (!signal_test(&precache_sig.odd)) {
            /* Fetch succeeded */
            _precache_bufs_copy(NFD_IN_BUF_RECACHE_WM);
            __implicit_read(bufs_rd, sizeof bufs_rd);

            precache_bufs_compute_seq_safe();
        }
    }

    if (!state.pending_fetch) {
        if (_precache_buf_bytes_used() >
            (sizeof (unsigned int) * NFD_IN_BUF_RECACHE_WM)) {
            /* Issue the fetch */
            __mem_ring_pop(blm_queue_num, blm_queue_addr, bufs_rd,
                           sizeof bufs_rd, sizeof bufs_rd,
                           sig_done, &precache_sig);
            state.pending_fetch = 1;
        }
    }
}


/**
 * Get a buffer from the cache, updating the cache pointer.
 */
__intrinsic unsigned int
precache_bufs_use()
{
    unsigned int ret;

    __asm alu[ret, --, b, NFD_IN_BUF_STORE_PTR--];
    return ret;
}


/**
 * Check the number of buffers available in the cache.
 */
__intrinsic unsigned int
precache_bufs_avail()
{
    unsigned int ret;

    ret = local_csr_read(local_csr_active_lm_addr_3);
    ret = ret - buf_store_start;
    ret = ret / sizeof(unsigned int);
    return ret;
}


/**
 * Compute a safe sequence for the issue_dma block based on the number of
 * buffers in the cache, the number of batches in "tx_issued_ring" and the
 * number of DMA batches in flight.
 *
 * Note that DMAs are only tracked per batch, with each batch assumed to be
 * filled.  This makes the DMA batches count slightly pessimistic if a few
 * batches are not full.
 */
__intrinsic void
precache_bufs_compute_seq_safe()
{
    unsigned int min_bat, buf_bat, dma_bat, ring_bat;

    buf_bat = precache_bufs_avail() / NFD_IN_MAX_BATCH_SZ;
    dma_bat = (NFD_IN_DATA_MAX_IN_FLIGHT / NFD_IN_MAX_BATCH_SZ -
               data_dma_seq_issued + data_dma_seq_compl);
    ring_bat = ((NFD_IN_ISSUED_RING_SZ - NFD_IN_ISSUED_RING_RES) /
                NFD_IN_MAX_BATCH_SZ);
    ring_bat = ring_bat - data_dma_seq_issued + data_dma_seq_served;

    /* Perform min(buf_bat, dma_bat, ring_bat) */
    min_bat = buf_bat;

    if (dma_bat < min_bat) {
        min_bat = dma_bat;
    }

    if (ring_bat < min_bat) {
        min_bat = ring_bat;
    }

    /* min_bat batches after data_dma_seq_issued are safe */
    data_dma_seq_safe = data_dma_seq_issued + min_bat;
}


/**
 * Perform once off, CTX0-only initialisation of sequence number autopushes
 */
void
distr_precache_bufs_setup_shared()
{
    dma_seqn_ap_setup(NFD_IN_DATA_EVENT_FILTER, NFD_IN_DATA_EVENT_FILTER,
                      NFD_IN_DATA_EVENT_TYPE, &nfd_in_data_event_xfer,
                      &nfd_in_data_event_sig);
}


/**
 * Compute, reflect, and receive sequence nos shared by issue and notify
 *
 * data_dma_seq_served tracks which batch notify has finished servicing.
 * It is needed by issue_dma to know how many batches can be placed on
 * the ring between the two MEs.
 *
 * data_dma_seq_compl tracks the completed data DMAs.  Events containing
 * the low 12 bits of a sequence number are received when DMAs complete,
 * and these are used to advance the full 32 bit sequence number.  The
 * notify code also needs this sequence number to determine when to process
 * issue_dma batches, so it is reflected to that ME.
 *
 * precache_bufs owns and updates data_dma_seq_safe, which requires these
 * sequence numbers, so the distribution code lives in precache_bufs.c.
 * If either sequence number has been advanced,
 * "precache_bufs_compute_seq_safe()" must be called.  "data_dma_seq_compl"
 * tracks in flight DMAs and is the priority, so it causes the safe sequence
 * to be updated immediately (before swapping).  If just "data_dma_seq_served"
 * has advanced, then the safe sequence will only be updated after the swap,
 * or on demand from the worker threads.
 */
__intrinsic void
distr_precache_bufs()
{
    if (signal_test(&nfd_in_data_served_refl_sig)) {
        data_dma_seq_served = nfd_in_data_served_refl_in;

        state.recompute_seq_safe = 1;
    }

    if (signal_test(&nfd_in_data_event_sig)) {
        __implicit_read(&nfd_in_data_compl_refl_out);

        dma_seqn_advance(&nfd_in_data_event_xfer, &data_dma_seq_compl);
        precache_bufs_compute_seq_safe();
        state.recompute_seq_safe = 0;

        /* Mirror to remote ME */
        nfd_in_data_compl_refl_out = data_dma_seq_compl;
        reflect_data(NFD_IN_NOTIFY_ME,
                     __xfer_reg_number(&nfd_in_data_compl_refl_in,
                                       NFD_IN_NOTIFY_ME),
                     __signal_number(&nfd_in_data_compl_refl_sig,
                                     NFD_IN_NOTIFY_ME),
                     &nfd_in_data_compl_refl_out,
                     sizeof nfd_in_data_compl_refl_out);

        __implicit_write(&nfd_in_data_event_sig);
        event_cls_autopush_filter_reset(
            NFD_IN_DATA_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            NFD_IN_DATA_EVENT_FILTER);

    } else {
        /* Swap to give other threads a chance to run */
        ctx_swap();
    }

    if (state.recompute_seq_safe == 1) {
        precache_bufs_compute_seq_safe();
        state.recompute_seq_safe = 0;
    }
}
