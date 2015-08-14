/*
 * Copyright (C) 2014-2015 Netronome Systems, Inc.  All rights reserved.
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

#ifndef PCI_IN_ISSUE_DMA_IDX
#warning "PCI_IN_ISSUE_DMA_IDX not defined.  Defaulting to 0.  Make sure there is only one instance"
#define PCI_IN_ISSUE_DMA_IDX 0
#endif

/* Configure *l$index3 to be a global pointer, and
 * set up a convenience define */
#define NFD_IN_BUF_STORE_PTR *l$index3
_init_csr("mecsr:CtxEnables.LMAddr3Global 1");


struct precache_bufs_state {
    unsigned int sigs_even_compl:2; /* bits for completed on mem_pop sigs */
    unsigned int spare:30;
};


NFD_BLM_Q_ALLOC(NFD_IN_BLM_POOL);

__shared __lmem unsigned int buf_store[NFD_IN_BUF_STORE_SZ];
static __shared unsigned int buf_store_start; /* Units: bytes */
static struct precache_bufs_state state = {0x3, 0};
static volatile SIGNAL_PAIR precache_sig0;
static volatile SIGNAL_PAIR precache_sig1;

static __xread unsigned int bufs_rd[NFD_IN_BUF_RECACHE_WM];
static unsigned int blm_queue_addr;
static unsigned int blm_queue_num;

extern __shared __gpr unsigned int data_dma_seq_issued;
__shared __gpr unsigned int data_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_served = 0;
__shared __gpr unsigned int data_dma_seq_safe = 0;
extern __shared __gpr unsigned int jumbo_dma_seq_issued;
extern __shared __gpr unsigned int jumbo_dma_seq_compl;

/* Signals and transfer registers for receiving DMA events */
static volatile __xread unsigned int nfd_in_data_event_xfer;
static SIGNAL nfd_in_data_event_sig;
static volatile __xread unsigned int nfd_in_jumbo_event_xfer;
static SIGNAL nfd_in_jumbo_event_sig;

/* Signals and transfer registers for sequence number reflects */
static __xwrite unsigned int nfd_in_data_compl_refl_out = 0;
__remote volatile SIGNAL nfd_in_data_compl_refl_sig;

#if (PCI_IN_ISSUE_DMA_IDX == 0)

__remote volatile __xread unsigned int nfd_in_data_compl_refl_in0;
__visible volatile __xread unsigned int nfd_in_data_served_refl_in0;
__visible volatile SIGNAL nfd_in_data_served_refl_sig0;
#define nfd_in_data_compl_refl_in nfd_in_data_compl_refl_in0
#define nfd_in_data_served_refl_in nfd_in_data_served_refl_in0
#define nfd_in_data_served_refl_sig nfd_in_data_served_refl_sig0
#define NFD_IN_ISSUED_RING_SZ NFD_IN_ISSUED_RING0_SZ
#define NFD_IN_ISSUED_RING_RES NFD_IN_ISSUED_RING0_RES

#elif (PCI_IN_ISSUE_DMA_IDX == 1)

__remote volatile __xread unsigned int nfd_in_data_compl_refl_in1;
__visible volatile __xread unsigned int nfd_in_data_served_refl_in1;
__visible volatile SIGNAL nfd_in_data_served_refl_sig1;
#define nfd_in_data_compl_refl_in nfd_in_data_compl_refl_in1
#define nfd_in_data_served_refl_in nfd_in_data_served_refl_in1
#define nfd_in_data_served_refl_sig nfd_in_data_served_refl_sig1
#define NFD_IN_ISSUED_RING_SZ NFD_IN_ISSUED_RING1_SZ
#define NFD_IN_ISSUED_RING_RES NFD_IN_ISSUED_RING1_RES
#else

#error "Invalid PCI_IN_ISSUE_DMA_IDX.  Must be 0 or 1."

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


__intrinsic unsigned int
_precache_buf_bytes_used()
{
    unsigned int ret;

    ret = local_csr_read(local_csr_active_lm_addr_3);
    ret = (sizeof(unsigned int) * NFD_IN_BUF_STORE_SZ) - (ret - buf_store_start);
    return ret;
}


#define _PRECACHE_BUFS_COPY(_num, _start)                                    \
do {                                                                         \
    ctassert(__is_ct_const(_num));                                           \
    ctassert(__is_ct_const(_start));                                         \
    ctassert((_num == 24) || (_num == 16) || (_num == 12) || (_num == 8));   \
    ctassert((_start == _num) || (_start == 0));                             \
                                                                             \
    /* Move to empty slot */                                                 \
    __asm { alu[--, --, b, NFD_IN_BUF_STORE_PTR++] }                         \
                                                                             \
    /* Perform bulk copy */                                                  \
    switch(_num) {                                                           \
    case 16:                                                                 \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 60] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 56] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 52] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 48] }                           \
    case 12:                                                                 \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 44] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 40] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 36] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 32] }                           \
    case 8:                                                                  \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 28] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 24] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 20] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 16] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 12] }                           \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 8] }                            \
        __asm { alu[NFD_IN_BUF_STORE_PTR++, --, b,                           \
                    bufs_rd + (4 * _start) + 4] }                            \
        __asm { alu[NFD_IN_BUF_STORE_PTR, --, b,                             \
                    bufs_rd + (4 * _start) + 0] }                            \
    }                                                                        \
} while (0)


/**
 * Initialise the precache_bufs block
 */
void
precache_bufs_setup()
{
    struct nfp_mecsr_ctx_enables cfg;

    __assign_relative_register(&nfd_in_data_event_xfer,
                               NFD_IN_DATA_EVENT_XFER_ASSIGN);

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
__intrinsic void
precache_bufs()
{
    /* test completion signal for mem_ring_pops */
    if (signal_test(&precache_sig0.even)) {
        state.sigs_even_compl |= 0x1;
        if (!signal_test(&precache_sig0.odd)) {
            _PRECACHE_BUFS_COPY(NFD_IN_BUF_RING_POP_SZ, 0);
            __implicit_read(bufs_rd, (NFD_IN_BUF_RING_POP_SZ << 2));
        }
    }

    if (signal_test(&precache_sig1.even)) {
        state.sigs_even_compl |= 0x2;
        if (!signal_test(&precache_sig1.odd)) {
            _PRECACHE_BUFS_COPY(NFD_IN_BUF_RING_POP_SZ,
                                NFD_IN_BUF_RING_POP_SZ);
            __implicit_read(&bufs_rd[NFD_IN_BUF_RING_POP_SZ],
                            (NFD_IN_BUF_RING_POP_SZ << 2));
        }
    }


    if (state.sigs_even_compl == 0x3) {
        if (_precache_buf_bytes_used() >
            (sizeof (unsigned int) * NFD_IN_BUF_RECACHE_WM)) {
            /* issue first pop with sig1 hoping it finished before sig0 */
            __mem_ring_pop(blm_queue_num, blm_queue_addr,
                           &bufs_rd[NFD_IN_BUF_RING_POP_SZ],
                           (NFD_IN_BUF_RING_POP_SZ << 2), 64,
                           sig_done, &precache_sig1);
            __mem_ring_pop(blm_queue_num, blm_queue_addr,
                           &bufs_rd[0],
                           (NFD_IN_BUF_RING_POP_SZ << 2), 64,
                           sig_done, &precache_sig0);
            state.sigs_even_compl = 0;
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
                      NFD_IN_DATA_EVENT_TYPE, 1, &nfd_in_data_event_xfer,
                      &nfd_in_data_event_sig);

    dma_seqn_ap_setup(NFD_IN_JUMBO_EVENT_FILTER, NFD_IN_JUMBO_EVENT_FILTER,
                      NFD_IN_JUMBO_EVENT_TYPE, 1, &nfd_in_jumbo_event_xfer,
                      &nfd_in_jumbo_event_sig);
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
 *
 * jumbo_dma_seq_compl tracks the number of reserve DMAs completed.  These
 * reserve DMAs are used if the packets exceeds NFD_IN_DMA_SPLIT_LEN.
 */
__intrinsic void
distr_precache_bufs(SIGNAL_MASK * wait_msk, SIGNAL *data_sig,
                    SIGNAL *jumbo_sig)
{
    if (signal_test(&nfd_in_data_served_refl_sig)) {
        data_dma_seq_served = nfd_in_data_served_refl_in;
    }

    if (signal_test(&nfd_in_data_event_sig)) {
        __implicit_read(&nfd_in_data_compl_refl_out);

        dma_seqn_advance(&nfd_in_data_event_xfer, &data_dma_seq_compl);

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
        __event_cls_autopush_filter_reset(
            NFD_IN_DATA_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            NFD_IN_DATA_EVENT_FILTER,
            sig_done, data_sig);
        *wait_msk |= __signals(data_sig);

    }

    if (signal_test(&nfd_in_jumbo_event_sig)) {
        dma_seqn_advance(&nfd_in_jumbo_event_xfer, &jumbo_dma_seq_compl);

        __implicit_write(&nfd_in_jumbo_event_sig);
        __event_cls_autopush_filter_reset(
            NFD_IN_JUMBO_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            NFD_IN_JUMBO_EVENT_FILTER,
            sig_done, jumbo_sig);
        *wait_msk |= __signals(jumbo_sig);

    }
}
