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
#include <pkt/pkt.h>

#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/pci_out.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/nn_ring.h>
#include <vnic/utils/ordering.h>
#include <vnic/utils/pcie.h>
#include <vnic/utils/qc.h>

#include <ns_vnic_ctrl.h>


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
    struct nfd_out_cpp_desc pkt0;
    struct nfd_out_cpp_desc pkt1;
    struct nfd_out_cpp_desc pkt2;
    struct nfd_out_cpp_desc pkt3;
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
__shared __gpr unsigned int data_dma_seq_safe;

/*
 * issue_dma variables
 */
static __gpr struct nfp_pcie_dma_cmd descr_tmp;
static __xwrite struct _dma_desc_batch dma_out_main;
static __xwrite struct _dma_desc_batch dma_out_res;

static __xread struct nfd_out_fl_desc fl_entries[4];

static SIGNAL data_sig0, data_sig1, data_sig2, data_sig3;
static SIGNAL_MASK data_wait_msk = 0;
static SIGNAL fl_sig0,  fl_sig1,  fl_sig2,  fl_sig3;
static SIGNAL_MASK fl_wait_msk = 0;

SIGNAL get_order_sig, issue_order_sig;

__visible volatile __xread unsigned int nfd_out_data_compl_refl_in = 0;
__visible volatile SIGNAL nfd_out_data_compl_refl_sig;


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
        local_csr_wr[NFP_MECSR_ACTIVE_CTX_WAKEUP_EVENTS>>2, *wait_msk];
    }
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
}


/**
 * Recompute the data_dma_seq_safe sequence number.  This sequence number
 * combines space in the "cpp_desc_ring" and inflight DMAs into a single
 * value to test against.
 */
void
_recompute_safe()
{
    unsigned int cpp_batch_safe = (data_dma_seq_served +
                                   NFD_OUT_CPP_BATCH_RING_BAT);
    /* A single descriptor may require 2 DMAs */
    unsigned int dma_batch_safe = (data_dma_seq_compl +
                                   (NFD_OUT_DATA_MAX_IN_FLIGHT /
                                    (NFD_OUT_MAX_BATCH_SZ * 2)));

    data_dma_seq_safe = dma_batch_safe;
    if (cpp_batch_safe < dma_batch_safe) {
        data_dma_seq_safe = cpp_batch_safe;
    }
}


/**
 * Copy reflected data_dma_seq_compl to shared variable and recompute the
 * safe sequence number based on the updated value.
 */
void
issue_dma_check_compl()
{
    if (signal_test(&nfd_out_data_compl_refl_sig)) {
        data_dma_seq_compl = nfd_out_data_compl_refl_in;
        _recompute_safe();
    }
}


/**
 * Access FL entry from local CTM
 * @param entry     Output FL descriptor
 * @param info      Message from "stage_batch" containing fl_cache_index
 * @param sync      Synchronisation type, must be "sig_done"
 * @param sig       Signal to use for the read
 */
__intrinsic void
_get_fl_entry(__xread struct nfd_out_fl_desc *entry,
              struct nfd_out_data_dma_info *info,
              sync_t sync, SIGNAL *sig)
{
    unsigned int index = info->fl_cache_index;
    unsigned int count = sizeof(struct nfd_out_fl_desc) >> 3;

    ctassert(sync == sig_done);
    __asm mem[read, *entry, 0, index, count], sig_done[*sig];
}

/**
 * Wrapper macro for the "_get_fl_entry" method
 */
/* XXX if CTM reads aren't threaded we can save 3 signals here */
#define _FL_PROC(_pkt)                                     \
do {                                                       \
    _get_fl_entry(&fl_entries[_pkt], &in_batch.pkt##_pkt,  \
                  sig_done, &fl_sig##_pkt);                \
} while (0)


/**
 * Remove "fl_sigX" for the packet from the wait mask
 */
#define _FL_CLR(_pkt)                            \
do {                                             \
    fl_wait_msk &= ~__signals(&fl_sig##_pkt);    \
} while (0)


/**
 * Populate a DMA descriptor with CTM address info from PCI.OUT input
 * @param descr     Partially completed DMA descriptor
 * @param info      PCI.OUT input message
 */
__intrinsic void
_get_ctm_addr(__gpr struct nfp_pcie_dma_cmd *descr,
              struct nfd_out_data_dma_info *info)
{
    descr->cpp_addr_hi = 0x80 | info->cpp.isl;
    descr->cpp_addr_lo = ((1 << 31) | (info->cpp.pktnum << 16) |
                          (info->cpp.offset & ((1<<11) - 1)));
}


/**
 * Handle one descriptor in the batch, issuing the DMAs that it requires,
 * and passing the CPP descriptor on to the "free_buf" block.
 * There following affect processing:
 *      "SOP": if a packet is not "SOP" DMA addresses are offset
 *      to complete the packet.
 *      "EOP": if a packet is "EOP" we can neglect some checks on DMA size.
 *      MU only packets (cpp.isl == 0): do not DMA from CTM
 *      Queue "down": pass on CPP descriptor only
 */
#define _ISSUE_PROC(_pkt, _type, _src)                                  \
do {                                                                    \
    descr_tmp.rid = in_batch.pkt##_pkt##.rid;                           \
    descr_tmp.pcie_addr_hi = fl_entries[_pkt].dma_addr_hi;              \
                                                                        \
    if (in_batch.pkt##_pkt##.cpp.sop) {                                 \
        __critical_path();                                              \
        if (in_batch.pkt##_pkt##.cpp.isl != 0) {                        \
            unsigned int split_len, ctm_bytes;                          \
            __critical_path();                                          \
                                                                        \
            split_len = 256 << in_batch.pkt##_pkt##.cpp.split;          \
            ctm_bytes = split_len - in_batch.pkt##_pkt##.cpp.offset;    \
                                                                        \
            if (in_batch.pkt##_pkt##.data_len > ctm_bytes) {            \
                /* Issue reserve DMA */                                 \
                descr_tmp.cpp_addr_hi = in_batch.pkt##_pkt##.cpp.mu_addr>>21; \
                descr_tmp.cpp_addr_lo = in_batch.pkt##_pkt##.cpp.mu_addr<<11; \
                descr_tmp.cpp_addr_lo += split_len;                     \
                descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;  \
                descr_tmp.pcie_addr_lo += NS_VNIC_RX_OFFSET;            \
                descr_tmp.pcie_addr_lo -= in_batch.pkt##_pkt##.meta_len; \
                descr_tmp.pcie_addr_lo += ctm_bytes;                    \
                                                                        \
                if (in_batch.pkt##_pkt##.cpp.eop) {                     \
                    /* This packet is both sop and eop, which means it */ \
                    /* can be completed with one DMA. */                \
                    /* XXX make app subtract the 1? */                  \
                    descr_tmp.length = in_batch.pkt##_pkt##.data_len - 1; \
                } else {                                                \
                    /* DMA as much as possible in the first DMA, */     \
                    /* it won't finish the packet. */                   \
                    descr_tmp.length = PCIE_DMA_MAX_SZ - 1;             \
                }                                                       \
                descr_tmp.length -= ctm_bytes;                          \
                pcie_dma_set_event(&descr_tmp,                          \
                                   NFD_OUT_DATA_IGN_EVENT_TYPE, 0);     \
                                                                        \
                dma_out_res.pkt##_pkt = descr_tmp;                      \
                pcie_dma_enq_no_sig(PCIE_ISL, &dma_out_res.pkt##_pkt##, \
                                    NFD_OUT_DATA_DMA_QUEUE);            \
                                                                        \
                /* Issue main DMA */                                    \
                _get_ctm_addr(&descr_tmp, &in_batch.pkt##_pkt);         \
                descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;  \
                descr_tmp.pcie_addr_lo += NS_VNIC_RX_OFFSET;            \
                descr_tmp.pcie_addr_lo -= in_batch.pkt##_pkt##.meta_len; \
                                                                        \
                descr_tmp.length = ctm_bytes - 1;                       \
                pcie_dma_set_event(&descr_tmp, _type, _src);            \
                                                                        \
                dma_out_main.pkt##_pkt = descr_tmp;                     \
                __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt##,     \
                               NFD_OUT_DATA_DMA_QUEUE,                  \
                               sig_done, &data_sig##_pkt##);            \
                                                                        \
            } else {                                                    \
                __critical_path();                                      \
                _get_ctm_addr(&descr_tmp, &in_batch.pkt##_pkt);         \
                descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;  \
                descr_tmp.pcie_addr_lo += NS_VNIC_RX_OFFSET;            \
                descr_tmp.pcie_addr_lo -= in_batch.pkt##_pkt##.meta_len; \
                                                                        \
                /* data_len is guaranteed to fit in one DMA */          \
                descr_tmp.length = in_batch.pkt##_pkt##.data_len - 1;   \
                pcie_dma_set_event(&descr_tmp, _type, _src);            \
                                                                        \
                dma_out_main.pkt##_pkt = descr_tmp;                     \
                __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt##,     \
                               NFD_OUT_DATA_DMA_QUEUE,                  \
                               sig_done, &data_sig##_pkt##);            \
            }                                                           \
        } else {                                                        \
            /* We have an MU only packet */                             \
            /* Issue one DMA for the first 4k of the packet */          \
            descr_tmp.cpp_addr_hi = in_batch.pkt##_pkt##.cpp.mu_addr>>21; \
            descr_tmp.cpp_addr_lo = in_batch.pkt##_pkt##.cpp.mu_addr<<11; \
            descr_tmp.cpp_addr_lo += in_batch.pkt##_pkt##.cpp.offset;   \
            descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;      \
            descr_tmp.pcie_addr_lo += NS_VNIC_RX_OFFSET;                \
            descr_tmp.pcie_addr_lo -= in_batch.pkt##_pkt##.meta_len;    \
                                                                        \
            if (in_batch.pkt##_pkt##.cpp.eop) {                         \
                /* This packet is both sop and eop, which means it */   \
                /* can be completed with one DMA. */                    \
                /* XXX make app subtract the 1? */                      \
                descr_tmp.length = in_batch.pkt##_pkt##.data_len - 1;   \
            } else {                                                    \
                /* DMA as much as possible in the first DMA, */         \
                /* it won't finish the packet. */                       \
                descr_tmp.length = PCIE_DMA_MAX_SZ - 1;                 \
            }                                                           \
            pcie_dma_set_event(&descr_tmp, _type, _src);                \
                                                                        \
            dma_out_main.pkt##_pkt = descr_tmp;                         \
            __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt##,         \
                           NFD_OUT_DATA_DMA_QUEUE, sig_done,            \
                           &data_sig##_pkt##);                          \
        }                                                               \
    } else { /* !SOP */                                                 \
        if (!in_batch.pkt##_pkt##.cpp.down) {                           \
            unsigned int len_tmp = in_batch.pkt##_pkt##.data_len;       \
                                                                        \
            if (len_tmp > (2 * PCIE_DMA_MAX_SZ)) {                      \
                /* Issue reserve DMA for the final bytes of the packet  */ \
                descr_tmp.cpp_addr_hi = in_batch.pkt##_pkt##.cpp.mu_addr>>21; \
                descr_tmp.cpp_addr_lo = in_batch.pkt##_pkt##.cpp.mu_addr<<11; \
                descr_tmp.cpp_addr_lo += in_batch.pkt##_pkt##.cpp.offset; \
                descr_tmp.cpp_addr_lo += (2 * PCIE_DMA_MAX_SZ);         \
                descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;  \
                descr_tmp.pcie_addr_lo += NS_VNIC_RX_OFFSET;            \
                descr_tmp.pcie_addr_lo -= in_batch.pkt##_pkt##.meta_len; \
                descr_tmp.pcie_addr_lo += (2 * PCIE_DMA_MAX_SZ);        \
                                                                        \
                descr_tmp.length = len_tmp - (2 * PCIE_DMA_MAX_SZ + 1); \
                pcie_dma_set_event(&descr_tmp, NFD_OUT_DATA_IGN_EVENT_TYPE, \
                                   0);                                  \
                                                                        \
                dma_out_res.pkt##_pkt = descr_tmp;                      \
                pcie_dma_enq_no_sig(PCIE_ISL, &dma_out_res.pkt##_pkt##, \
                                    NFD_OUT_DATA_DMA_QUEUE);            \
                                                                        \
                len_tmp = (2 * PCIE_DMA_MAX_SZ);                        \
                                                                        \
            }                                                           \
                                                                        \
            /* Issue main DMA for bytes 4097 to 8192 */                 \
            descr_tmp.cpp_addr_hi = in_batch.pkt##_pkt##.cpp.mu_addr>>21; \
            descr_tmp.cpp_addr_lo = in_batch.pkt##_pkt##.cpp.mu_addr<<11; \
            descr_tmp.cpp_addr_lo += in_batch.pkt##_pkt##.cpp.offset;   \
            descr_tmp.cpp_addr_lo += PCIE_DMA_MAX_SZ;                   \
            descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;      \
            descr_tmp.pcie_addr_lo += NS_VNIC_RX_OFFSET;                \
            descr_tmp.pcie_addr_lo -= in_batch.pkt##_pkt##.meta_len;    \
            descr_tmp.pcie_addr_lo += PCIE_DMA_MAX_SZ;                  \
                                                                        \
            descr_tmp.length = len_tmp - PCIE_DMA_MAX_SZ - 1;           \
            pcie_dma_set_event(&descr_tmp, _type, _src);                \
                                                                        \
            dma_out_main.pkt##_pkt = descr_tmp;                         \
            __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt##,         \
                           NFD_OUT_DATA_DMA_QUEUE, sig_done,            \
                           &data_sig##_pkt##);                          \
                                                                        \
        } else { /* Queue down */                                       \
            /* Pass message on to free buffer only */                   \
            descr_tmp.cpp_addr_hi = 0;                                  \
            descr_tmp.cpp_addr_lo = 0;                                  \
            descr_tmp.pcie_addr_hi = 0;                                 \
            descr_tmp.pcie_addr_lo = 0;                                 \
            pcie_dma_set_event(&descr_tmp, _type, _src);                \
            descr_tmp.length = 0;                                       \
                                                                        \
            descr_tmp.dma_cfg_index = NFD_OUT_DATA_CFG_REG_SIG_ONLY;    \
            dma_out_main.pkt##_pkt = descr_tmp;                         \
            descr_tmp.dma_cfg_index = NFD_OUT_DATA_CFG_REG;             \
            __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt,           \
                           NFD_OUT_DATA_DMA_QUEUE,                      \
                           sig_done, &data_sig##_pkt);                  \
        }                                                               \
    }                                                                   \
                                                                        \
    /* Pass CPP descriptor on to next block */                          \
    cpp_desc_ring[cpp_desc_index].pkt##_pkt = in_batch.pkt##_pkt##.cpp; \
} while (0)


/**
 * Remove "data_sigX" for the packet from the wait mask.
 * Ensure that "EOP" is unset in the CPP descriptor message so that
 * "free_buf" ignores the descriptor.
 */
#define _ISSUE_CLR(_pkt)                                             \
do {                                                                 \
    struct nfd_out_cpp_desc cpp_zero = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  \
                                                                     \
    data_wait_msk &= ~__signals(&data_sig##_pkt);                    \
    cpp_desc_ring[cpp_desc_index].pkt##_pkt = cpp_zero;              \
} while (0)


/**
 * Dequeue a message from "stage_batch".
 */
__intrinsic void
_nn_get_msg(struct nfd_out_data_dma_info *msg)
{
    /* Stage batch may head of line block, while enqueuing a message batch,
     * so while dequeuing we must also head of line block. */
    while (nn_ring_empty()) {
        ctx_swap();
    }

    msg->__raw[0] = nn_ring_get();
    msg->__raw[1] = nn_ring_get();
    msg->__raw[2] = nn_ring_get();
    msg->__raw[3] = nn_ring_get();
}



/**
 * Check for messages from "stage_batch" and then execute those messages.
 * For each message we fetch a FL entry from CTM, performing an ordered swap
 * until it is available, and then issue the DMAs required by the descriptor.
 * This method *may* head of line block (in particular if "stage_block" head
 * of line blocks).  This is not a problem because "free_buf" also runs on
 * CTX0, so the other functions running on this ME can continue.
 */
void
issue_dma()
{
    struct nfd_out_data_batch in_batch;
    struct nfd_out_data_batch_msg msg;
    unsigned int n_bat;
    unsigned int cpp_desc_index;

    /* Start of "get" order stage */
    _swap_on_msk(&data_wait_msk);
    __implicit_read(&data_sig0);
    __implicit_read(&data_sig1);
    __implicit_read(&data_sig2);
    __implicit_read(&data_sig3);
    __implicit_read(&get_order_sig);
    __implicit_read(&dma_out_res, sizeof dma_out_res);

    if (!nn_ring_empty() && data_dma_seq_started != data_dma_seq_safe) {
        data_dma_seq_started++;

        /* We have a message! */
        data_wait_msk = __signals(&data_sig0, &data_sig1, &data_sig2,
                                  &data_sig3, &get_order_sig);

        msg.__raw = nn_ring_get();
        n_bat = msg.num;

        switch (n_bat) {
        case 4:
            __critical_path();
            _nn_get_msg(&in_batch.pkt0);
            _nn_get_msg(&in_batch.pkt1);
            _nn_get_msg(&in_batch.pkt2);
            _nn_get_msg(&in_batch.pkt3);

            reorder_done(NFD_OUT_ISSUE_START_CTX, &get_order_sig);

            _FL_PROC(0);
            _FL_PROC(1);
            _FL_PROC(2);
            _FL_PROC(3);

            _swap_on_msk(&fl_wait_msk);
            __implicit_read(&fl_sig0);
            __implicit_read(&fl_sig1);
            __implicit_read(&fl_sig2);
            __implicit_read(&fl_sig3);
            __implicit_read(&issue_order_sig);

            fl_wait_msk = __signals(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3,
                                    &issue_order_sig);

            reorder_done(NFD_OUT_ISSUE_START_CTX, &issue_order_sig);

            data_dma_seq_issued++;
            cpp_desc_index = (data_dma_seq_issued &
                              (NFD_OUT_CPP_BATCH_RING_BAT - 1));
            _ISSUE_PROC(0, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(1, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(2, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(3, NFD_OUT_DATA_EVENT_TYPE, data_dma_seq_issued);

            break;
        case 3:
            _nn_get_msg(&in_batch.pkt0);
            _nn_get_msg(&in_batch.pkt1);
            _nn_get_msg(&in_batch.pkt2);

            reorder_done(NFD_OUT_ISSUE_START_CTX, &get_order_sig);

            _FL_PROC(0);
            _FL_PROC(1);
            _FL_PROC(2);

            _FL_CLR(3);

            _swap_on_msk(&fl_wait_msk);
            __implicit_read(&fl_sig0);
            __implicit_read(&fl_sig1);
            __implicit_read(&fl_sig2);
            __implicit_read(&fl_sig3);
            __implicit_read(&issue_order_sig);

            fl_wait_msk = __signals(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3,
                                    &issue_order_sig);

            reorder_done(NFD_OUT_ISSUE_START_CTX, &issue_order_sig);

            data_dma_seq_issued++;
            cpp_desc_index = (data_dma_seq_issued &
                              (NFD_OUT_CPP_BATCH_RING_BAT - 1));
            _ISSUE_PROC(0, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(1, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(2, NFD_OUT_DATA_EVENT_TYPE, data_dma_seq_issued);

            _ISSUE_CLR(3);

            break;
        case 2:
            _nn_get_msg(&in_batch.pkt0);
            _nn_get_msg(&in_batch.pkt1);

            reorder_done(NFD_OUT_ISSUE_START_CTX, &get_order_sig);

            _FL_PROC(0);
            _FL_PROC(1);

            _FL_CLR(2);
            _FL_CLR(3);

            _swap_on_msk(&fl_wait_msk);
            __implicit_read(&fl_sig0);
            __implicit_read(&fl_sig1);
            __implicit_read(&fl_sig2);
            __implicit_read(&fl_sig3);
            __implicit_read(&issue_order_sig);

            fl_wait_msk = __signals(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3,
                                    &issue_order_sig);

            reorder_done(NFD_OUT_ISSUE_START_CTX, &issue_order_sig);

            data_dma_seq_issued++;
            cpp_desc_index = (data_dma_seq_issued &
                              (NFD_OUT_CPP_BATCH_RING_BAT - 1));
            _ISSUE_PROC(0, NFD_OUT_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(1, NFD_OUT_DATA_EVENT_TYPE, data_dma_seq_issued);

            _ISSUE_CLR(2);
            _ISSUE_CLR(3);

            break;
        case 1:
            _nn_get_msg(&in_batch.pkt0);

            reorder_done(NFD_OUT_ISSUE_START_CTX, &get_order_sig);

            _FL_PROC(0);

            _FL_CLR(1);
            _FL_CLR(2);
            _FL_CLR(3);

            _swap_on_msk(&fl_wait_msk);
            __implicit_read(&fl_sig0);
            __implicit_read(&fl_sig1);
            __implicit_read(&fl_sig2);
            __implicit_read(&fl_sig3);
            __implicit_read(&issue_order_sig);

            fl_wait_msk = __signals(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3,
                                    &issue_order_sig);

            reorder_done(NFD_OUT_ISSUE_START_CTX, &issue_order_sig);

            data_dma_seq_issued++;
            cpp_desc_index = (data_dma_seq_issued &
                              (NFD_OUT_CPP_BATCH_RING_BAT - 1));
            _ISSUE_PROC(0, NFD_OUT_DATA_EVENT_TYPE, data_dma_seq_issued);

            _ISSUE_CLR(1);
            _ISSUE_CLR(2);
            _ISSUE_CLR(3);

            break;
        default:
            halt();
        }
    } else { /* nn_ring_empty() or insufficient resources */
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


/**
 * Issue a "packet_free" command for the packet listed in the CPP descriptor
 * @param cpp       CPP descriptor containing CTM packet information
 */
__intrinsic void
_free_ctm_addr(struct nfd_out_cpp_desc *cpp)
{
    unsigned int addr_hi, addr_lo;

    /* Construct address >>8 in single register */
    addr_hi = (1<<31) | (cpp->isl << 24);
    addr_lo = cpp->pktnum;
    __asm mem[packet_free, --, addr_hi, <<8, addr_lo];
}


/**
 * Free the CTM and MU buffers associated with a CPP descriptor.
 */
#define _FREE_BUF(_pkt)                                                 \
do {                                                                    \
    struct nfd_out_cpp_desc cpp_desc;                                   \
                                                                        \
    cpp_desc = cpp_desc_ring[cpp_desc_index].pkt##_pkt;                 \
                                                                        \
    /* Only free for EOP */                                             \
    if (cpp_desc.eop) {                                                 \
        /* Don't free CTM buffer for MU only packets */                 \
        if (cpp_desc.isl != 0) {                                        \
            _free_ctm_addr(&cpp_desc);                                  \
        }                                                               \
                                                                        \
        rnum = blm_rnum_start + cpp_desc.bls;                           \
        mem_ring_journal_fast(rnum, blm_raddr, cpp_desc.mu_addr);       \
    }                                                                   \
} while (0)


/**
 * Check whether any batches can be freed, and handle the batch
 * NB this method does not swap
 */
__forceinline void
free_buf()
{
    unsigned int rnum;
    unsigned int cpp_desc_index;

    if (data_dma_seq_served != data_dma_seq_compl) {
        /*
         * Increment data_dma_seq_served upfront to avoid ambiguity about
         * sequence number zero
         */
        data_dma_seq_served++;
        cpp_desc_index = data_dma_seq_served & (NFD_OUT_CPP_BATCH_RING_BAT - 1);

        _FREE_BUF(0);
        _FREE_BUF(1);
        _FREE_BUF(2);
        _FREE_BUF(3);
    }
}

