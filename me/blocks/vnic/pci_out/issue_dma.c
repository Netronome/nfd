/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/issue_dma.c
 * @brief         Code to DMA packet data from the NFP
 */

/* XXX probably better to name the PCI.OUT and PCI.IN files differently */

#include <assert.h>
#include <nfp.h>

#include <nfp/mem_ring.h>
#include <nfp/pcie.h>
#include <pkt/pkt.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/pci_out.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/nn_ring.h>
#include <vnic/utils/ordering.h>
#include <vnic/utils/qc.h>

#include <nfp_net_ctrl.h>


/* Required user configuration */
#ifndef NFD_OUT_BLM_POOL_START
#error "NFD_OUT_BLM_POOL_START must be defined by the user"
#endif

#ifndef NFD_OUT_BLM_RADDR
#error "NFD_OUT_BLM_RADDR must be defined by the user"
#endif

NFD_BLM_Q_ALLOC(NFD_OUT_BLM_POOL_START);


/* XXX move somewhere shared? */
struct _dma_desc_batch {
    struct nfp_pcie_dma_cmd pkt0;
    struct nfp_pcie_dma_cmd pkt1;
    struct nfp_pcie_dma_cmd pkt2;
    struct nfp_pcie_dma_cmd pkt3;
};

struct _cpp_desc_batch {
    struct nfd_out_cpp_desc_ring pkt0;
    struct nfd_out_cpp_desc_ring pkt1;
    struct nfd_out_cpp_desc_ring pkt2;
    struct nfd_out_cpp_desc_ring pkt3;
};


/*
 * Rings and queues
 */
static __shared __lmem struct _cpp_desc_batch
    cpp_desc_ring[NFD_OUT_CPP_BATCH_RING_BAT];

/*
 * Sequence numbers
 */
__shared __gpr unsigned int data_dma_seq_started = 0;
__shared __gpr unsigned int data_dma_seq_issued = 0;
__shared __gpr unsigned int data_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_served = 0;
__shared __gpr unsigned int data_dma_seq_safe = 0; /* Will recompute at start */

/*
 * "data_dma_resv_avail" works as a count of how many more reserve DMAs
 * can be sent.  Credits are claimed as "issue_dma()" sends reserve DMAs
 * and returned as "free_buf()" frees the packet.  The count of how many
 * reserve DMAs a packet required is passed in the cpp_desc_ring.
 * Tests to determine whether there are enough credits to send a packet are
 * performed once off, when it is detected that a reserve DMA will be required.
 * NFD_OUT_MAX_RESV_PER_PKT is used to ensure there are enough reserve
 * DMAs available to send a jumbo frame before the packet is processed.
 */
__shared __gpr int data_dma_resv_avail = NFD_OUT_RESV_MAX_IN_FLIGHT;

__shared __gpr unsigned int empty_cnt0 = 0;
__shared __gpr unsigned int empty_cnt1 = 0;

/*
 * issue_dma variables
 */
static __gpr struct nfp_pcie_dma_cmd descr_tmp;
static __xwrite struct _dma_desc_batch dma_out_main;
static __xwrite struct _dma_desc_batch dma_out_resv;

static __xread struct nfd_out_fl_desc fl_entries[4];

static __shared __gpr unsigned int ctm_cpp_lo_msk =
    ((NFD_OUT_CPP_CTM_PKTNUM_msk << NFD_OUT_CPP_CTM_PKTNUM_shf) |
     (NFD_OUT_CPP_CTM_OFFSET_msk << NFD_OUT_CPP_CTM_OFFSET_shf));

static SIGNAL data_sig0, data_sig1, data_sig2, data_sig3;
static SIGNAL_MASK data_wait_msk = 0;
static SIGNAL fl_sig0,  fl_sig1,  fl_sig2,  fl_sig3;
static SIGNAL_MASK fl_wait_msk = 0;

SIGNAL get_order_sig, issue_order_sig;
unsigned int next_ctx;

static volatile __xread unsigned int data_dma_event_xfer;
static SIGNAL data_dma_event_sig;

__remote volatile __xread unsigned int nfd_out_data_compl_refl_in;
__remote volatile SIGNAL nfd_out_data_compl_refl_sig;
static __xwrite unsigned int nfd_out_data_compl_refl_out = 0;


/*
 * free_buf variables
 */
static __gpr unsigned int blm_raddr;
static __gpr unsigned int blm_rnum_start;


/*
 * Initialise the NN ring from stage_batch to issue_dma
 */
NN_RING_ZERO_PTRS;
NN_RING_EMPTY_ASSERT_SET(3);


/* XXX replace with flowenv function */
__intrinsic void
_swap_on_msk(SIGNAL_MASK *wait_msk)
{
    __asm {
        ctx_arb[--], defer[1];
        local_csr_wr[local_csr_active_ctx_wakeup_events, *wait_msk];
    }
}


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
issue_dma_setup_shared()
{
    struct nfp_pcie_dma_cfg cfg_tmp;
    __xwrite struct nfp_pcie_dma_cfg cfg;

    /*
     * Setup the DMA configuration registers
     * XXX PCI.IN and PCI.OUT use the same settings,
     * could share configuration registers.
     */
    cfg_tmp.__raw = 0;
    /* Signal only configuration for null messages */
    cfg_tmp.signal_only_odd = 1;
    cfg_tmp.target_64_odd = 1;
    cfg_tmp.cpp_target_odd = 7;
    /* Regular configuration */
#ifdef NFD_VNIC_NO_HOST
    /* Use signal_only for seqn num generation
     * Don't actually DMA data */
    cfg_tmp.signal_only_even = 1;
#else
    cfg_tmp.signal_only_even = 0;
#endif
    cfg_tmp.end_pad_even = 0;
    cfg_tmp.start_pad_even = 0;
    cfg_tmp.target_64_even = 1;
    cfg_tmp.cpp_target_even = 7;
    cfg = cfg_tmp;

    pcie_dma_cfg_set_pair(PCIE_ISL, NFD_OUT_DATA_CFG_REG, &cfg);

    /* Kick off ordering */
    reorder_start(NFD_OUT_ISSUE_START_CTX, &get_order_sig);
    reorder_start(NFD_OUT_ISSUE_START_CTX, &issue_order_sig);
}


/**
 * Perform per CTX configuration of issue_dma
 */
void
issue_dma_setup()
{
    /*
     * Initialise a DMA descriptor template RequesterID (rid), CPP address, and
     * PCIe address will be overwritten per transaction. For dma_mode, we
     * technically only want to overwrite the "source" field, i.e. 12 of the 16
     * bits.
     */
    descr_tmp.length = sizeof(struct nfd_out_rx_desc) - 1;
    descr_tmp.rid_override = 1;
    descr_tmp.trans_class = 0;
    descr_tmp.cpp_token = NFD_OUT_DATA_DMA_TOKEN;
    descr_tmp.dma_cfg_index = NFD_OUT_DATA_CFG_REG;

    /* Expect a full batch by default */
    data_wait_msk = __signals(&get_order_sig);
    fl_wait_msk = __signals(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3,
                            &issue_order_sig);

    next_ctx = reorder_get_next_ctx(NFD_OUT_ISSUE_START_CTX);
}


/**
 * Recompute the data_dma_seq_safe sequence number.  This sequence number
 * combines space in the "cpp_desc_ring" and inflight DMAs into a single
 * value to test against.
 */
__intrinsic void
_recompute_safe()
{
    unsigned int cpp_batch_safe = (data_dma_seq_served +
                                   NFD_OUT_CPP_BATCH_RING_BAT);
    /* Reserve DMAs are tracked separately and on demand */
    unsigned int dma_batch_safe = (data_dma_seq_compl +
                                   (NFD_OUT_DATA_MAX_IN_FLIGHT /
                                    NFD_OUT_MAX_BATCH_SZ));

    data_dma_seq_safe = dma_batch_safe;
    if (cpp_batch_safe < dma_batch_safe) {
        data_dma_seq_safe = cpp_batch_safe;
    }
}


/**
 * Perform once off, CTX0-only initialisation of sequence number autopushes
 */
void
distr_seqn_setup_shared()
{
    dma_seqn_ap_setup(NFD_OUT_DATA_EVENT_FILTER, NFD_OUT_DATA_EVENT_FILTER,
                      NFD_OUT_DATA_EVENT_TYPE, &data_dma_event_xfer,
                      &data_dma_event_sig);
}


/**
 * Check autopush, compute data_dma_compl, and reflect to stage batch ME
 */
__intrinsic void
distr_seqn()
{
    if (signal_test(&data_dma_event_sig)) {
        __implicit_read(&nfd_out_data_compl_refl_out);

        dma_seqn_advance(&data_dma_event_xfer, &data_dma_seq_compl);
        _recompute_safe();

        /* Mirror to remote ME */
        nfd_out_data_compl_refl_out = data_dma_seq_compl;
        reflect_data(NFD_OUT_STAGE_ME,
                     __xfer_reg_number(&nfd_out_data_compl_refl_in,
                                       NFD_OUT_STAGE_ME),
                     __signal_number(&nfd_out_data_compl_refl_sig,
                                     NFD_OUT_STAGE_ME),
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
}



/**
 * Handle one descriptor in the batch, issuing the DMAs that it requires,
 * and passing the CPP descriptor on to the "free_buf" block.
 * There following affect processing:
 *      MU only packets (cpp.isl == 0): do not DMA from CTM
 *      "reserved" holds the queue "up" bit.
 *      Queue not "up": pass on CPP descriptor only
 *
 * If a packet fits entirely in CTM, it must be flagged as "ctm_only"
 * and it will be processed on the fast path.  We can minimise cycles
 * because we know that we only have to send one DMA of the data_len
 * given in the descriptor.
 *
 * MU only packets that are smaller than NFD_OUT_DMA_SPLIT_LEN are DMAed
 * in one DMA (the "main" DMA, and use the batch sequence number in flight
 * DMA tracking system).  Large packets are split at the first 4k aligned
 * host address.  The first section uses the batch sequence number in flight
 * tracking, and subsequent sections use "data_dma_resv_avail" credits.
 * "free_buf()" returns these credits as it processes the packet completions.
 *
 * Packets that spill out of CTM use the main DMA to send the CTM portion.
 * The DMA goes strictly up to the end of CTM buffer.  If a "ctm_only" packet
 * is not flagged it is a serious application bug.  NFD will halt() if this
 * is detected.  Reserve DMAs are sent to transfer the MU portion.  The
 * first reserve DMA will be up to a 4k aligned host address, provided
 * the packet doesn't end sooner, and that the transfer is at least
 * NFD_OUT_DMA_SPLIT_MIN bytes (to ensure we can finish a jumbo frame in
 * NFD_OUT_MAX_RESV_PER_PKT reserve DMAs).
 *
 * If "data_dma_resv_avail" credits deplete and a packet requiring reserve
 * DMAs is received, _ISSUE_PROC will head of line block (ctx_swap()).
 * This allows "free_buf()" on CTX0 to execute and free up credits.
 */
#define _ISSUE_PROC(_pkt, _type, _src)                                  \
do {                                                                    \
    unsigned int ctm_msg;                                               \
                                                                        \
    __asm { alu[ctm_msg, --, b, *l$index2[NFD_OUT_CPP_CTM_INDEX##_pkt]] } \
    if (ctm_msg & (1<< NFD_OUT_CPP_CTM_RESERVED_shf)) {                 \
        unsigned int ctm_hi;                                            \
                                                                        \
        if (ctm_msg & (1<< NFD_OUT_CPP_CTM_CTM_ONLY_shf)) {             \
            __critical_path();                                          \
            descr_tmp.rid = rx_desc##_pkt.rid;                          \
            descr_tmp.pcie_addr_hi = fl_entries[_pkt].dma_addr_hi;      \
                                                                        \
            ctm_hi = 0x80 | (ctm_msg >> NFD_OUT_CPP_CTM_ISL_shf);       \
            descr_tmp.cpp_addr_hi = ctm_hi;                             \
            ctm_msg &= ctm_cpp_lo_msk;                                  \
            descr_tmp.cpp_addr_lo = 0x80000000 | ctm_msg;               \
                                                                        \
            /* Pack CTM address processing into CPP descriptor ring */  \
            ctm_msg = ((ctm_msg >> NFD_OUT_CPP_CTM_PKTNUM_shf) |        \
                       (ctm_hi << NFD_OUT_FREE_CTM_ISL_HI_shf));        \
            __asm { alu[*l$index2[NFD_OUT_CPP_CTM_INDEX##_pkt],         \
                        --, b, ctm_msg] }                               \
                                                                        \
            descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;      \
            descr_tmp.pcie_addr_lo += NFP_NET_RX_OFFSET;                \
            descr_tmp.pcie_addr_lo -= rx_desc##_pkt.meta_len;           \
                                                                        \
            descr_tmp.length = rx_desc##_pkt##.data_len - 1;            \
            pcie_dma_set_event(&descr_tmp, _type, _src);                \
                                                                        \
            dma_out_main.pkt##_pkt = descr_tmp;                         \
            __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt,           \
                           NFD_OUT_DATA_DMA_QUEUE,                      \
                           sig_done, &data_sig##_pkt);                  \
                                                                        \
        } else {                                                        \
            unsigned int resv_dma_cnt = 0;                              \
            unsigned int data_len;                                      \
            unsigned int pcie_lo_start;                                 \
            struct nfd_out_cpp_desc_ring msg;                           \
                                                                        \
            msg.cpp.__raw[0] = ctm_msg;                                 \
            __asm { alu[msg.cpp.__raw[1], --, b,                        \
                        *l$index2[NFD_OUT_CPP_MU_INDEX##_pkt]] }        \
            descr_tmp.rid = rx_desc##_pkt.rid;                          \
            descr_tmp.pcie_addr_hi = fl_entries[_pkt].dma_addr_hi;      \
                                                                        \
            data_len = rx_desc##_pkt##.data_len;                        \
            pcie_lo_start = fl_entries[_pkt].dma_addr_lo;               \
            pcie_lo_start += NFP_NET_RX_OFFSET;                         \
            pcie_lo_start -= rx_desc##_pkt.meta_len;                    \
                                                                        \
            ctm_hi = msg.cpp.isl;                                       \
            if (ctm_hi == 0) {                                          \
                /* We have an MU only packet */                         \
                unsigned int cpp_lo_start;                              \
                                                                        \
                descr_tmp.cpp_addr_hi = msg.cpp.mu_addr >> 21;          \
                                                                        \
                cpp_lo_start = msg.cpp.mu_addr << 11;                   \
                cpp_lo_start += msg.cpp.offset;                         \
                                                                        \
                if (data_len > NFD_OUT_DMA_SPLIT_LEN) {                 \
                    unsigned int first_dma_len;                         \
                                                                        \
                    first_dma_len = pcie_lo_start + data_len;           \
                    first_dma_len &= PCIE_DMA_MAX_SZ - 1;               \
                    first_dma_len = PCIE_DMA_MAX_SZ - first_dma_len;    \
                                                                        \
                    if (first_dma_len < data_len) {                     \
                        /* We will actually use reserve DMAs */         \
                        /* Check reserve DMA credits and HOL block */   \
                        /* insufficient are available. */               \
                        while ((data_dma_resv_avail -                   \
                                NFD_OUT_MAX_RESV_PER_PKT) < 0) {        \
                            ctx_swap();                                 \
                        }                                               \
                                                                        \
                        data_len -= first_dma_len;                      \
                        descr_tmp.pcie_addr_lo = (pcie_lo_start +       \
                                                  first_dma_len);       \
                        descr_tmp.cpp_addr_lo = (cpp_lo_start +         \
                                                 first_dma_len);        \
                        pcie_dma_set_event(&descr_tmp,                  \
                                           NFD_OUT_DATA_IGN_EVENT_TYPE, \
                                           0);                          \
                                                                        \
                        /* Process as many 4k chunks as we can */       \
                        while (data_len > PCIE_DMA_MAX_SZ) {            \
                            SIGNAL resv_dma_sig;                        \
                                                                        \
                            descr_tmp.length = PCIE_DMA_MAX_SZ - 1;     \
                            dma_out_resv.pkt##_pkt = descr_tmp;         \
                            __pcie_dma_enq(PCIE_ISL,                    \
                                           &dma_out_resv.pkt##_pkt,     \
                                           NFD_OUT_DATA_DMA_QUEUE,      \
                                           ctx_swap, &resv_dma_sig);    \
                                                                        \
                            data_len -= PCIE_DMA_MAX_SZ;                \
                            descr_tmp.pcie_addr_lo += PCIE_DMA_MAX_SZ;  \
                            descr_tmp.cpp_addr_lo += PCIE_DMA_MAX_SZ;   \
                                                                        \
                            resv_dma_cnt++;                             \
                            if (resv_dma_cnt >= NFD_OUT_MAX_RESV_PER_PKT) { \
                                /* We have a runaway packet */          \
                                halt();                                 \
                            }                                           \
                        }                                               \
                                                                        \
                        /* DMA bytes at the end of the packet as the */ \
                        /* final "reserve" DMA.  We can leave this */   \
                        /* DMA to complete without waiting on a signal */ \
                        /* because it is issued with the main DMA.  */  \
                        descr_tmp.length = data_len - 1;                \
                        dma_out_resv.pkt##_pkt = descr_tmp;             \
                        pcie_dma_enq_no_sig(PCIE_ISL,                   \
                                            &dma_out_resv.pkt##_pkt,    \
                                            NFD_OUT_DATA_DMA_QUEUE);    \
                                                                        \
                        resv_dma_cnt++;                                 \
                        data_len = first_dma_len;                       \
                    }                                                   \
                }                                                       \
                                                                        \
                /* Zero CTM address and manage reserve DMA credits */   \
                __asm { ld_field_w_clr[*l$index2[NFD_OUT_CPP_CTM_INDEX##_pkt], \
                                       4, resv_dma_cnt,                 \
                                       <<NFD_OUT_FREE_CTM_RESV_shf] }   \
                data_dma_resv_avail -= resv_dma_cnt;                    \
                                                                        \
                /* Issue the main DMA */                                \
                descr_tmp.pcie_addr_lo = pcie_lo_start;                 \
                descr_tmp.cpp_addr_lo = cpp_lo_start;                   \
                                                                        \
                descr_tmp.length = data_len - 1;                        \
                pcie_dma_set_event(&descr_tmp, _type, _src);            \
                                                                        \
                dma_out_main.pkt##_pkt = descr_tmp;                     \
                __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt,       \
                               NFD_OUT_DATA_DMA_QUEUE,                  \
                               sig_done, &data_sig##_pkt);              \
                                                                        \
            } else {                                                    \
                /* We have a packet split between CTM and MU */         \
                unsigned int split_len;                                 \
                unsigned int ctm_bytes;                                 \
                unsigned int this_dma_len;                              \
                                                                        \
                /* We will actually use reserve DMAs */                 \
                /* Check reserve DMA credits and HOL block */           \
                /* insufficient are available. */                       \
                while ((data_dma_resv_avail - NFD_OUT_MAX_RESV_PER_PKT) < 0) { \
                    ctx_swap();                                         \
                }                                                       \
                                                                        \
                split_len = (256 << msg.cpp.split);                     \
                ctm_bytes = (split_len - msg.cpp.offset);               \
                data_len -= ctm_bytes;                                  \
                                                                        \
                /* Issue reserved DMAs */                               \
                descr_tmp.cpp_addr_hi = msg.cpp.mu_addr >> 21;          \
                descr_tmp.cpp_addr_lo = msg.cpp.mu_addr << 11;          \
                descr_tmp.cpp_addr_lo += split_len;                     \
                                                                        \
                descr_tmp.pcie_addr_lo = pcie_lo_start + ctm_bytes;     \
                                                                        \
                pcie_dma_set_event(&descr_tmp,                          \
                                   NFD_OUT_DATA_IGN_EVENT_TYPE, 0);     \
                                                                        \
                /* Find a good amount to DMA with first reserve DMA */  \
                if (data_len > NFD_OUT_DMA_SPLIT_LEN) {                 \
                    this_dma_len = pcie_lo_start + data_len;            \
                    this_dma_len &= PCIE_DMA_MAX_SZ - 1;                \
                    this_dma_len = PCIE_DMA_MAX_SZ - this_dma_len;      \
                                                                        \
                    if (this_dma_len > data_len) {                      \
                        /* We can finish the packet in one DMA */       \
                        this_dma_len = data_len;                        \
                    } else if (this_dma_len < NFD_OUT_DMA_SPLIT_MIN) {  \
                        /* Give up trying to find a good alignment */   \
                        /* and just DMA what we can.  */                \
                        if (data_len > PCIE_DMA_MAX_SZ) {               \
                            this_dma_len = PCIE_DMA_MAX_SZ;             \
                        } else {                                        \
                            this_dma_len = data_len;                    \
                        }                                               \
                    } else {                                            \
                        /* We have found a good first DMA len. */       \
                        /* Nothing more to do.  */                      \
                    }                                                   \
                } else {                                                \
                    this_dma_len = data_len;                            \
                }                                                       \
                                                                        \
                do {                                                    \
                    resv_dma_cnt++;                                     \
                    if (resv_dma_cnt > NFD_OUT_MAX_RESV_PER_PKT) {      \
                        /* We have a runaway packet */                  \
                        /* e.g. a ctm_only packet wasn't flagged correctly */ \
                        halt();                                         \
                    }                                                   \
                    descr_tmp.length = this_dma_len - 1;                \
                    dma_out_resv.pkt##_pkt = descr_tmp;                 \
                                                                        \
                    data_len -= this_dma_len;                           \
                    if (data_len > 0) {                                 \
                        SIGNAL resv_dma_sig;                            \
                                                                        \
                        descr_tmp.cpp_addr_lo += this_dma_len;          \
                        descr_tmp.pcie_addr_lo += this_dma_len;         \
                                                                        \
                        /* Pick dma_len for the next DMA */             \
                        if (data_len > 4096) {                          \
                            this_dma_len = 4096;                        \
                        } else {                                        \
                            this_dma_len = data_len;                    \
                        }                                               \
                                                                        \
                        __pcie_dma_enq(PCIE_ISL, &dma_out_resv.pkt##_pkt, \
                                       NFD_OUT_DATA_DMA_QUEUE,          \
                                       ctx_swap, &resv_dma_sig);        \
                    } else {                                            \
                        /* Issue final reserve DMA */                   \
                        pcie_dma_enq_no_sig(PCIE_ISL,                   \
                                            &dma_out_resv.pkt##_pkt,    \
                                            NFD_OUT_DATA_DMA_QUEUE);    \
                    }                                                   \
                } while (data_len > 0);                                 \
                                                                        \
                /* Finally, issue the CTM DMA */                        \
                ctm_hi |= 0x80;                                         \
                descr_tmp.cpp_addr_hi = ctm_hi;                         \
                descr_tmp.cpp_addr_lo = (0x80000000 | (msg.cpp.pktnum << 16) | \
                                         (msg.cpp.offset));             \
                                                                        \
                /* Manage reserve DMA credits and pack CTM address */   \
                /* processing into CPP descriptor ring */               \
                ctm_hi = ((ctm_hi << NFD_OUT_FREE_CTM_ISL_HI_shf) |     \
                          (resv_dma_cnt << NFD_OUT_FREE_CTM_RESV_shf) | \
                          msg.cpp.pktnum);                              \
                __asm { alu[*l$index2[NFD_OUT_CPP_CTM_INDEX##_pkt],     \
                            --, b, ctm_hi] }                            \
                data_dma_resv_avail -= resv_dma_cnt;                    \
                                                                        \
                /* Issue the main DMA */                                \
                descr_tmp.pcie_addr_lo = pcie_lo_start;                 \
                                                                        \
                descr_tmp.length = ctm_bytes - 1;                       \
                pcie_dma_set_event(&descr_tmp, _type, _src);            \
                                                                        \
                dma_out_main.pkt##_pkt = descr_tmp;                     \
                __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt,       \
                               NFD_OUT_DATA_DMA_QUEUE,                  \
                               sig_done, &data_sig##_pkt);              \
                                                                        \
            } /* end of CTM/MU split packets */                         \
        } /* end of !ctm_only */                                        \
    } else { /* Down */                                                 \
        unsigned int pktnum;                                            \
        unsigned int isl;                                               \
                                                                        \
        /* Pass message on to free buffer only */                       \
        descr_tmp.rid = 0;                                              \
        descr_tmp.cpp_addr_hi = 0;                                      \
        descr_tmp.cpp_addr_lo = 0;                                      \
        descr_tmp.pcie_addr_hi = 0;                                     \
        descr_tmp.pcie_addr_lo = 0;                                     \
        pcie_dma_set_event(&descr_tmp, _type, _src);                    \
        descr_tmp.length = 0;                                           \
                                                                        \
        descr_tmp.dma_cfg_index = NFD_OUT_DATA_CFG_REG_SIG_ONLY;        \
        dma_out_main.pkt##_pkt = descr_tmp;                             \
        descr_tmp.dma_cfg_index = NFD_OUT_DATA_CFG_REG;                 \
        __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt,               \
                       NFD_OUT_DATA_DMA_QUEUE,                          \
                       sig_done, &data_sig##_pkt);                      \
                                                                        \
        /* Repack CTM address for FREE_BUF */                           \
        pktnum = ((ctm_msg >> NFD_OUT_CPP_CTM_PKTNUM_shf) &             \
                  NFD_OUT_CPP_CTM_PKTNUM_msk);                          \
        isl = ((ctm_msg >> NFD_OUT_CPP_CTM_ISL_shf) &                   \
               NFD_OUT_CPP_CTM_ISL_msk);                                \
        ctm_msg = 0x80000000 | (isl << NFD_OUT_FREE_CTM_ISL_HI_shf) | pktnum; \
        __asm { alu[*l$index2[NFD_OUT_CPP_CTM_INDEX##_pkt], --, b, ctm_msg] } \
                                                                        \
    } /* end of "down" */                                               \
                                                                        \
} while (0)


/**
 * Remove "data_sigX" for the packet from the wait mask.
 * Ensure that "EOP" is unset in the CPP descriptor message so that
 * "free_buf" ignores the descriptor.
 */
#define _ISSUE_CLR(_pkt)                                             \
do {                                                                 \
    data_wait_msk &= ~__signals(&data_sig##_pkt);                    \
} while (0)



/**
 * Read a message off the NN ring into storage
 * The cpp_desc_ring is used to store the CPP descriptor so that it
 * can be left and edited in place for later use by "free_buf()".
 * The FL descriptor index is used immediately, and the RX descriptor
 * info is stored in GPRs.
 */
#define _GET_MSG(_pkt)                                                  \
do {                                                                    \
    unsigned int count = sizeof(struct nfd_out_fl_desc) >> 3;           \
                                                                        \
    /* Stage batch may head of line block, while enqueuing */           \
    /* a message batch, so while dequeuing we must also */              \
    /* head of line block. */                                           \
    while (nn_ring_empty()) {                                           \
        empty_cnt1++;                                                   \
        local_csr_write(local_csr_mailbox_1, empty_cnt1);               \
        ctx_swap();                                                     \
    }                                                                   \
                                                                        \
    __asm { alu[*l$index2[NFD_OUT_CPP_CTM_INDEX##_pkt], --, b, *n$index++] } \
    __asm { alu[*l$index2[NFD_OUT_CPP_MU_INDEX##_pkt], --, b, *n$index++] } \
    rx_desc##_pkt.__raw = nn_ring_get();                                \
    fl_cache_index = nn_ring_get();                                     \
    __asm { mem[read, fl_entries[_pkt], 0, fl_cache_index,              \
                __ct_const_val(count)], sig_done[fl_sig##_pkt] }        \
} while (0)


/**
 * Remove "fl_sigX" for the packet from the wait mask
 * and zero the unused slots in cpp_desc_ring
 */
#define _GET_MSG_CLR(_pkt)                                          \
do {                                                                \
    __asm { alu[*l$index2[NFD_OUT_CPP_CTM_INDEX##_pkt], --, b, 0] } \
    __asm { alu[*l$index2[NFD_OUT_CPP_MU_INDEX##_pkt], --, b, 0] }  \
    fl_wait_msk &= ~__signals(&fl_sig##_pkt);                       \
} while (0)


/**
 * Check for messages from "stage_batch" and then execute those messages.
 * For each message we fetch a FL entry from CTM, performing an ordered swap
 * until it is available, and then issue the DMAs required by the descriptor.
 * This method *may* head of line block (in particular if "stage_block" head
 * of line blocks).  This is not a problem because "free_buf" also runs on
 * CTX0, so the other functions running on this ME can continue.
 */
__intrinsic void
issue_dma()
{
    struct nfd_out_data_batch_msg msg;
    unsigned int n_bat;
    unsigned int cpp_index;
    struct nfd_out_issue_rx_desc rx_desc0, rx_desc1, rx_desc2, rx_desc3;
    unsigned int fl_cache_index;

    /* Start of "get" order stage */
    _swap_on_msk(&data_wait_msk);
    __implicit_read(&data_sig0);
    __implicit_read(&data_sig1);
    __implicit_read(&data_sig2);
    __implicit_read(&data_sig3);
    __implicit_read(&get_order_sig);
    __implicit_read(&dma_out_resv, sizeof dma_out_resv);

    if (!nn_ring_empty() && data_dma_seq_started != data_dma_seq_safe) {
        /* We have a message! */
        cpp_index = (data_dma_seq_started &
                     (NFD_OUT_CPP_BATCH_RING_BAT - 1));
        data_dma_seq_started++;
        cpp_index = (unsigned int) &cpp_desc_ring[cpp_index];
        local_csr_write(local_csr_active_lm_addr_2, cpp_index);

        data_wait_msk = __signals(&data_sig0, &data_sig1, &data_sig2,
                                  &data_sig3, &get_order_sig);

        msg.__raw = nn_ring_get();

        if ((msg.__raw & 4) == 0) {
            n_bat = msg.num;
            if (n_bat == 3) {
                _GET_MSG(0);
                _GET_MSG(1);
                _GET_MSG(2);

                _GET_MSG_CLR(3);

                reorder_done_opt(&next_ctx, &get_order_sig);

                _swap_on_msk(&fl_wait_msk);
                __implicit_read(&fl_sig0);
                __implicit_read(&fl_sig1);
                __implicit_read(&fl_sig2);
                __implicit_read(&fl_sig3);
                __implicit_read(&issue_order_sig);

                fl_wait_msk = __signals(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3,
                                        &issue_order_sig);

                data_dma_seq_issued++;
                _ISSUE_PROC(0, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
                _ISSUE_PROC(1, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
                _ISSUE_PROC(2, NFD_OUT_DATA_EVENT_TYPE, data_dma_seq_issued);

                _ISSUE_CLR(3);

                reorder_done_opt(&next_ctx, &issue_order_sig);

            } else if (n_bat == 2) {
                _GET_MSG(0);
                _GET_MSG(1);

                _GET_MSG_CLR(2);
                _GET_MSG_CLR(3);

                reorder_done_opt(&next_ctx, &get_order_sig);

                _swap_on_msk(&fl_wait_msk);
                __implicit_read(&fl_sig0);
                __implicit_read(&fl_sig1);
                __implicit_read(&fl_sig2);
                __implicit_read(&fl_sig3);
                __implicit_read(&issue_order_sig);

                fl_wait_msk = __signals(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3,
                                        &issue_order_sig);

                data_dma_seq_issued++;
                _ISSUE_PROC(0, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
                _ISSUE_PROC(1, NFD_OUT_DATA_EVENT_TYPE, data_dma_seq_issued);

                _ISSUE_CLR(2);
                _ISSUE_CLR(3);

                reorder_done_opt(&next_ctx, &issue_order_sig);

            } else if (n_bat == 1) {
                _GET_MSG(0);

                _GET_MSG_CLR(1);
                _GET_MSG_CLR(2);
                _GET_MSG_CLR(3);

                reorder_done_opt(&next_ctx, &get_order_sig);

                _swap_on_msk(&fl_wait_msk);
                __implicit_read(&fl_sig0);
                __implicit_read(&fl_sig1);
                __implicit_read(&fl_sig2);
                __implicit_read(&fl_sig3);
                __implicit_read(&issue_order_sig);

                fl_wait_msk = __signals(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3,
                                        &issue_order_sig);

                data_dma_seq_issued++;
                _ISSUE_PROC(0, NFD_OUT_DATA_EVENT_TYPE, data_dma_seq_issued);

                _ISSUE_CLR(1);
                _ISSUE_CLR(2);
                _ISSUE_CLR(3);

                reorder_done_opt(&next_ctx, &issue_order_sig);

            } else {
                /* n_bat must hold an invalid value */
                halt();
            }
        } else {
            /* We have a full batch! */
            __critical_path();
            _GET_MSG(0);
            _GET_MSG(1);
            _GET_MSG(2);
            _GET_MSG(3);

            reorder_done_opt(&next_ctx, &get_order_sig);

            _swap_on_msk(&fl_wait_msk);
            __implicit_read(&fl_sig0);
            __implicit_read(&fl_sig1);
            __implicit_read(&fl_sig2);
            __implicit_read(&fl_sig3);
            __implicit_read(&issue_order_sig);

            fl_wait_msk = __signals(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3,
                                    &issue_order_sig);

            data_dma_seq_issued++;
            _ISSUE_PROC(0, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(1, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(2, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(3, NFD_OUT_DATA_EVENT_TYPE, data_dma_seq_issued);

            reorder_done_opt(&next_ctx, &issue_order_sig);
        }

    } else { /* nn_ring_empty() or insufficient resources */
        if (nn_ring_empty()) {
            empty_cnt0++;
            local_csr_write(local_csr_mailbox_0, empty_cnt0);
        }

        /* Check resources */
        _recompute_safe();

        /* Allow this thread to poll the NN ring again */
        data_wait_msk = __signals(&get_order_sig);
        reorder_self(&get_order_sig);
    }
}


/**
 * Perform per CTX initialisation of "free_buf"
 */
__intrinsic void
free_buf_setup()
{
    blm_raddr = ((unsigned long long) NFD_OUT_BLM_RADDR >> 8) & 0xff000000;
    blm_rnum_start = NFD_BLM_Q_LINK(NFD_OUT_BLM_POOL_START);
}


/* XXX add back optional checking of MU buffer handles before freeing */
#ifdef NFD_VNIC_DBG_CHKS
#define _FREE_BUF_MU_CHK                                                \
    if ((mu_addr & NFD_MU_PTR_DBG_MSK) == 0) {                          \
        halt();                                                         \
    }
#else
#define _FREE_BUF_MU_CHK
#endif


/**
 * Free the CTM and MU buffers associated with a CPP descriptor.
 * "issue_dma()" restructured the CTM descriptor for more efficient
 * freeing.
 * "mu_addr == 0" signals skip this packet (0 is an illegal buffer handle)
 * "isl_hi == 0" signals skip the CTM buffer (for MU only packets)
 */
#define _FREE_BUF(_pkt)                                                 \
do {                                                                    \
    unsigned int rnum;                                                  \
    unsigned int mu_addr;                                               \
    unsigned int isl_hi;                                                \
    unsigned int pktnum;                                                \
    unsigned int resv_dma_cnt;                                          \
                                                                        \
    __asm {                                                             \
        /* Test whether there's a packet to free */                     \
        __asm { alu [mu_addr,                                           \
                     *l$index3[NFD_OUT_FREE_MU_INDEX##_pkt],            \
                     and~, NFD_OUT_FREE_MU_BLS_msk,                     \
                     << NFD_OUT_FREE_MU_BLS_shf] }                      \
                                                                        \
        __asm { beq[free_end##_pkt] }                                   \
        __asm { alu [rnum, blm_rnum_start, or,                          \
                     *l$index3[NFD_OUT_FREE_MU_INDEX##_pkt],            \
                     >>NFD_OUT_FREE_MU_BLS_shf] }                       \
        __asm { alu[--, 8, or, rnum, <<16] }                            \
        __asm { mem[fast_journal, --, blm_raddr, <<8, mu_addr,          \
                    0], indirect_ref }                                  \
                                                                        \
        /* Do reserve DMA credit house keeping */                       \
        __asm { alu[resv_dma_cnt, NFD_OUT_FREE_CTM_RESV_msk, and,       \
                    *l$index3[NFD_OUT_FREE_CTM_INDEX##_pkt],            \
                    >>NFD_OUT_FREE_CTM_RESV_shf] }                      \
        __asm { alu[data_dma_resv_avail, data_dma_resv_avail, +,        \
                    resv_dma_cnt] }                                     \
                                                                        \
            /* Test whether there's a CTM buffer to free */             \
        __asm { alu[isl_hi, *l$index3[NFD_OUT_FREE_CTM_INDEX##_pkt],    \
                    and, NFD_OUT_FREE_CTM_ISL_HI_msk,                   \
                    <<NFD_OUT_FREE_CTM_ISL_HI_shf] }                    \
                                                                        \
        __asm { beq[free_end##_pkt] }                                   \
        __asm { ld_field_w_clr[pktnum, 3,                               \
                               *l$index3[NFD_OUT_FREE_CTM_INDEX##_pkt]] } \
        __asm { mem[packet_free, --, isl_hi, <<8, pktnum] }             \
                                                                        \
    free_end##_pkt:                                                     \
    }                                                                   \
} while (0)


/**
 * Check whether any batches can be freed, and handle the batch
 * NB this method does not swap
 */
__forceinline void
free_buf()
{
    unsigned int cpp_index;

    if (data_dma_seq_served != data_dma_seq_compl) {
        /*
         * Access cpp_index and Increment data_dma_seq_served upfront
         * to avoid ambiguity about sequence number zero
         */
        cpp_index = data_dma_seq_served & (NFD_OUT_CPP_BATCH_RING_BAT - 1);
        data_dma_seq_served++;
        cpp_index = (unsigned int) &cpp_desc_ring[cpp_index];
        local_csr_write(local_csr_active_lm_addr_3, cpp_index);

        _FREE_BUF(0);
        _FREE_BUF(1);
        _FREE_BUF(2);
        _FREE_BUF(3);
    }
}

