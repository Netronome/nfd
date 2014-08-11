/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/precache_bufs.c
 * @brief         Fill the local (LM) cache of MU buffers
 */

#include <nfcc_chipres.h>
#include <nfp.h>
#include <assert.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>
#include <std/reg_utils.h>

#include <nfp6000/nfp_me.h>

#include <vnic/pci_in_cfg.h>
#include <vnic/pci_in/precache_bufs.h>
#include <vnic/shared/nfd_shared.h>


/* Configure *l$index3 to be a global pointer, and
 * set up a convenience define */
#define TX_BUF_STORE_PTR *l$index3
_init_csr("mecsr:CtxEnables.LMAddr3Global 1");

struct precache_state {
    unsigned int pending_fetch:1;
    unsigned int spare:31;
};


__shared __lmem unsigned int buf_store[TX_BUF_STORE_SZ];
static __shared unsigned int buf_store_start; /* Units: bytes */
static struct precache_state state = {0, 0};
static SIGNAL_PAIR precache_sig;

static __xread unsigned int bufs_rd[TX_BUF_RECACHE_WM];
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
 * number of buffers that can be added is (TX_BUF_STORE_SZ -
 * buf_store_curr + &buf_store[0])/4.  Refills are done if this exceeds
 * TX_BUF_RECACHE_WM/4, where the "/4" can be dropped through out.
 *
 * No count of the buffers available is maintained.  Instead, the
 * number of buffers on hand is periodically used to compute a "safe
 * sequence number" that covers availability of multiple resources.
 */


__inline unsigned int
_precache_buf_bytes_used()
{
    unsigned int ret;

    ret = local_csr_read(NFP_MECSR_ACTIVE_LM_ADDR_3);
    ret = (sizeof(unsigned int) * TX_BUF_STORE_SZ) - (ret - buf_store_start);
    return ret;
}


__intrinsic void
_precache_bufs_copy(unsigned int num)
{
    ctassert(__is_ct_const(num));
    ctassert((num == 16) || (num == 8) || (num == 4));

    /* Move to empty slot */
    __asm alu[--, --, b, TX_BUF_STORE_PTR++];

    /* Perform bulk copy */
    switch(num) {
    case 16:
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
    case 8:
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
    case 4:
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR++, --, b, *$index++];
        __asm alu[TX_BUF_STORE_PTR, --, b, *$index];
    }
}


void
precache_bufs_setup()
{
    struct nfp_mecsr_ctx_enables cfg;

    /* XXX Replace with _init_csr command, once query resolved */
    cfg.__raw = local_csr_read(NFP_MECSR_CTX_ENABLES);
    cfg.lm_addr_3_glob = 1;
    local_csr_write(NFP_MECSR_CTX_ENABLES, cfg.__raw);

    blm_queue_addr = ((unsigned long long) TX_BLM_RADDR >> 8);
    blm_queue_num = NFD_BLM_Q_ALLOC(TX_BLM_POOL);

    buf_store_start = (unsigned int) &buf_store;
    local_csr_write(NFP_MECSR_ACTIVE_LM_ADDR_3, buf_store_start);

    /* Write a magic value to buf_store[0]. This should never be
     * used and will help to identify buf_store underflows */
    buf_store[0] = 0x195fde01; /* Magically becomes 0xcafef00800 */
}


void
precache_bufs()
{
    /* NB: Ordering of if clauses allows back to back fetches,
     *     if buf_store is sufficiently empty */

    if (signal_test(&precache_sig.even)) {
        /* Process the fetch */

        /* Prepare T-INDEX early so usage shadow is filled easily */
        local_csr_write(NFP_MECSR_T_INDEX, __xfer_reg_number(bufs_rd));
        state.pending_fetch = 0;

        if (!signal_test(&precache_sig.odd)) {
            /* Fetch succeeded */
            _precache_bufs_copy(TX_BUF_RECACHE_WM);
            __implicit_read(bufs_rd, sizeof bufs_rd);

            precache_bufs_compute_seq_safe();
        }
    }

    if (!state.pending_fetch) {
        if (_precache_buf_bytes_used() >
            (sizeof (unsigned int) * TX_BUF_RECACHE_WM)) {
            /* Issue the fetch */
            __mem_ring_get(blm_queue_num, blm_queue_addr, bufs_rd,
                           sizeof bufs_rd, sizeof bufs_rd,
                           sig_done, &precache_sig);
            state.pending_fetch = 1;
        }
    }
}


__intrinsic unsigned int
precache_bufs_use()
{
    unsigned int ret;

    __asm alu[ret, --, b, TX_BUF_STORE_PTR--];
    return ret;
}


__inline unsigned int
precache_bufs_avail()
{
    unsigned int ret;

    ret = local_csr_read(NFP_MECSR_ACTIVE_LM_ADDR_3);
    ret = ret - buf_store_start;
    ret = ret / sizeof(unsigned int);
    return ret;
}


__inline void
precache_bufs_compute_seq_safe()
{
    unsigned int min_bat, buf_bat, dma_bat, ring_bat;

    buf_bat = precache_bufs_avail() / MAX_TX_BATCH_SZ;
    dma_bat = (TX_DATA_MAX_IN_FLIGHT / MAX_TX_BATCH_SZ -
               data_dma_seq_issued + data_dma_seq_compl);
    ring_bat = ((TX_ISSUED_RING_SZ - TX_ISSUED_RING_RES) / MAX_TX_BATCH_SZ -
                data_dma_seq_issued + data_dma_seq_served);

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
