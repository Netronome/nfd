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
#include <std/reg_utils.h>

#include <nfp6000/nfp_me.h>

#include <vnic/shared/nfd_internal.h>


/* Configure *l$index3 to be a global pointer, and
 * set up a convenience define */
#define NFD_IN_BUF_STORE_PTR *l$index3
_init_csr("mecsr:CtxEnables.LMAddr3Global 1");

NFD_BLM_Q_ALLOC(NFD_IN_BLM_POOL);

__shared __lmem unsigned int buf_store[NFD_IN_BUF_STORE_SZ];
static __shared unsigned int buf_store_start; /* Units: bytes */
static struct nfd_in_me1_state state = {0, 0, 0};
static SIGNAL_PAIR precache_sig;

static __xread unsigned int bufs_rd[NFD_IN_BUF_RECACHE_WM];
static unsigned int blm_queue_addr;
static unsigned int blm_queue_num;

extern __shared __gpr unsigned int data_dma_seq_compl;
extern __shared __gpr unsigned int data_dma_seq_served;
extern __shared __gpr unsigned int data_dma_seq_issued;
__shared __gpr unsigned int data_dma_seq_safe = 0;


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
            __mem_ring_get(blm_queue_num, blm_queue_addr, bufs_rd,
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
__inline unsigned int
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
__inline void
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
