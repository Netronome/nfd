/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/issue_dma.c
 * @brief         Code to DMA packet data from the NFP
 */

/* XXX probably better to name the PCI.OUT and PCI.IN files differently */

#include <assert.h>
#include <nfp.h>

#include <pkt/pkt.h>

#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/pci_out/issue_dma.h>

#include <vnic/pci_out.h>
#include <vnic/pci_out_cfg.h>
#include <vnic/pci_out/pci_out_internal.h>
#include <vnic/shared/qc.h>
#include <vnic/utils/cls_ring.h>
#include <vnic/utils/nn_ring.h>
#include <vnic/utils/pcie.h>

/* XXX move somewhere shared? */
struct _dma_desc_batch {
    struct nfp_pcie_dma_cmd pkt0;
    struct nfp_pcie_dma_cmd pkt1;
    struct nfp_pcie_dma_cmd pkt2;
    struct nfp_pcie_dma_cmd pkt3;
};

/* XXX Further data will be required when checking for follow on packets
 * volatile set temporarily */
volatile __shared __lmem unsigned int ring_rids[MAX_RX_QUEUES];

__shared __gpr unsigned int data_dma_msg_served = 0; /* XXX necessary? */

__shared __gpr unsigned int data_dma_seq_issued = 0;
__shared __gpr unsigned int data_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_served = 0;

__visible volatile __xread unsigned int rx_data_compl_reflect_xread = 0;
__visible volatile SIGNAL rx_data_compl_reflect_sig;

static __gpr struct nfp_pcie_dma_cmd descr_tmp;
static __xwrite struct _dma_desc_batch dma_out_main;
static __xwrite struct _dma_desc_batch dma_out_res;
static SIGNAL data_sig0, data_sig1, data_sig2, data_sig3;
static SIGNAL_MASK data_wait_msk = 0;
static SIGNAL fl_sig0,  fl_sig1,  fl_sig2,  fl_sig3;

static __xread struct nfd_pci_out_fl_desc fl_entries[4];

NN_RING_ZERO_PTRS;
NN_RING_EMPTY_ASSERT_SET(0);


__intrinsic void
_get_fl_entry(__xread struct nfd_pci_out_fl_desc *entry,
              __xread struct pci_out_data_dma_info *info,
              sync_t sync, SIGNAL *sig)
{
    unsigned int index = info->fl_cache_index;
    unsigned int count = sizeof(struct nfd_pci_out_fl_desc) >> 3;

    ctassert(sync == sig_done);
    __asm mem[read, *entry, 0, index, count], sig_done[*sig];
}

__intrinsic void
_get_ctm_addr(__gpr struct nfp_pcie_dma_cmd *descr,
              __xread struct pci_out_data_dma_info *info)
{
    descr->cpp_addr_hi = 0x80 | info->cpp.isl;
    descr->cpp_addr_lo = ((1 << 31) | (info->cpp.pktnum << 16) |
                          (info->cpp.offset & ((1<<11) - 1)));
}


__intrinsic void
_swap_on_msk(SIGNAL_MASK *wait_msk)
{
    __asm {
        ctx_arb[--], defer[1];
        local_csr_wr[NFP_MECSR_ACTIVE_CTX_WAKEUP_EVENTS>>2, *wait_msk];
    }
}

void
issue_dma_setup_shared()
{
    /* XXX complete non-per-queue data */
    unsigned int queue;
    unsigned int vnic;
    unsigned int vnic_q;
    struct nfp_pcie_dma_cfg cfg_tmp;
    __xwrite struct nfp_pcie_dma_cfg cfg;

    ctassert(__is_log2(MAX_VNICS));
    ctassert(__is_log2(MAX_VNIC_QUEUES));

    /*
     * Set requester IDs
     */
    for (queue = 0, vnic = 0; vnic < MAX_VNICS; vnic++) {
        for (vnic_q = 0; vnic_q < MAX_VNIC_QUEUES; vnic_q++, queue++) {
            unsigned int bmsk_queue;

            bmsk_queue = map_natural_to_bitmask(queue);
            ring_rids[bmsk_queue] = vnic;
        }
    }

    /*
     * Setup the DMA configuration registers
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

    pcie_dma_cfg_set_pair(PCIE_ISL, RX_DATA_CFG_REG, &cfg);
}

void
issue_dma_setup()
{
    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, and PCIe address will be
     * overwritten per transaction.
     * For dma_mode, we technically only want to overwrite the "source"
     * field, i.e. 12 of the 16 bits.
     */
    descr_tmp.length = sizeof(struct nfd_pci_out_rx_desc) - 1;
    descr_tmp.rid_override = 1;
    descr_tmp.trans_class = 0;
    descr_tmp.cpp_token = RX_DATA_DMA_TOKEN;
    descr_tmp.dma_cfg_index = RX_DATA_CFG_REG;

    /* Expect a full batch by default */
    data_wait_msk = __signals(&data_sig0, &data_sig1, &data_sig2, &data_sig3);
}


void
issue_dma_check_compl()
{
    if (signal_test(&rx_data_compl_reflect_sig)) {
        data_dma_seq_compl = rx_data_compl_reflect_xread;
    }
}


/* XXX if CTM reads aren't threaded we can save 3 signals here */
#define _FL_PROC(_pkt)                                    \
do {                                                      \
    _get_fl_entry(&fl_entries[_pkt], &in_batch.pkt##_pkt, \
                  sig_done, &fl_sig##_pkt);               \
} while (0)


#define _FL_CLR(_pkt)                           \
do {                                            \
} while (0)


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
                pcie_dma_set_event(&descr_tmp, RX_DATA_IGN_EVENT_TYPE, 0); \
                                                                        \
                dma_out_res.pkt##_pkt = descr_tmp;                      \
                pcie_dma_enq_no_sig(PCIE_ISL, &dma_out_res.pkt##_pkt##, \
                                    RX_DATA_DMA_QUEUE);                 \
                                                                        \
                /* Issue main DMA */                                    \
                _get_ctm_addr(&descr_tmp, &in_batch.pkt##_pkt);         \
                descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;  \
                                                                        \
                descr_tmp.length = ctm_bytes - 1;                       \
                pcie_dma_set_event(&descr_tmp, _type, _src);            \
                                                                        \
                dma_out_main.pkt##_pkt = descr_tmp;                     \
                __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt##,     \
                               RX_DATA_DMA_QUEUE,                       \
                               sig_done, &data_sig##_pkt##);            \
                                                                        \
            } else {                                                    \
                __critical_path();                                      \
                _get_ctm_addr(&descr_tmp, &in_batch.pkt##_pkt);         \
                descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;  \
                                                                        \
                /* data_len is guaranteed to fit in one DMA */          \
                descr_tmp.length = in_batch.pkt##_pkt##.data_len - 1;   \
                pcie_dma_set_event(&descr_tmp, _type, _src);            \
                                                                        \
                dma_out_main.pkt##_pkt = descr_tmp;                     \
                __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt##,     \
                               RX_DATA_DMA_QUEUE,                       \
                               sig_done, &data_sig##_pkt##);            \
            }                                                           \
        } else {                                                        \
            /* We have an MU only packet */                             \
            /* Issue one DMA for the first 4k of the packet */          \
            descr_tmp.cpp_addr_hi = in_batch.pkt##_pkt##.cpp.mu_addr>>21; \
            descr_tmp.cpp_addr_lo = in_batch.pkt##_pkt##.cpp.mu_addr<<11; \
            descr_tmp.cpp_addr_lo += in_batch.pkt##_pkt##.cpp.offset;   \
            descr_tmp.pcie_addr_lo = fl_entries[_pkt].dma_addr_lo;      \
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
                           RX_DATA_DMA_QUEUE, sig_done, &data_sig##_pkt##); \
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
                descr_tmp.pcie_addr_lo += (2 * PCIE_DMA_MAX_SZ);        \
                                                                        \
                descr_tmp.length = len_tmp - (2 * PCIE_DMA_MAX_SZ + 1); \
                pcie_dma_set_event(&descr_tmp, RX_DATA_IGN_EVENT_TYPE, 0); \
                                                                        \
                dma_out_res.pkt##_pkt = descr_tmp;                      \
                pcie_dma_enq_no_sig(PCIE_ISL, &dma_out_res.pkt##_pkt##, \
                                    RX_DATA_DMA_QUEUE);                 \
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
            descr_tmp.pcie_addr_lo += PCIE_DMA_MAX_SZ;                  \
                                                                        \
            descr_tmp.length = len_tmp - PCIE_DMA_MAX_SZ;               \
            pcie_dma_set_event(&descr_tmp, _type, _src);                \
                                                                        \
            dma_out_main.pkt##_pkt = descr_tmp;                         \
            __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt##,         \
                           RX_DATA_DMA_QUEUE, sig_done, &data_sig##_pkt##); \
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
            descr_tmp.dma_cfg_index = RX_DATA_CFG_REG_SIG_ONLY;         \
            dma_out_main.pkt##_pkt = descr_tmp;                         \
            descr_tmp.dma_cfg_index = RX_DATA_CFG_REG;                  \
            __pcie_dma_enq(PCIE_ISL, &dma_out_main.pkt##_pkt,           \
                           RX_DATA_DMA_QUEUE,                           \
                           sig_done, &data_sig##_pkt);                  \
        }                                                               \
    }                                                                   \
} while (0)


#define _ISSUE_CLR(_pkt)                             \
do {                                                 \
    data_wait_msk &= ~__signals(&data_sig##_pkt);    \
} while (0)



/** Parameters list to be filled out as extended */
void
issue_dma()
{
    static __xread struct pci_out_data_batch in_batch;
    struct pci_out_data_batch_msg msg;
    unsigned int n_bat;

    SIGNAL get_sig;

    if (!nn_ring_empty()) {
        /* We have a message! */
        msg.__raw = nn_ring_get();
        n_bat = msg.num;

        cls_ring_get(RX_DATA_BATCH_RING_NUM, &in_batch, sizeof in_batch,
                     &get_sig);
        wait_for_all(&get_sig); /* Order this wait! */

        switch (n_bat) {
        case 4:
            __critical_path();

            _FL_PROC(0);
            _FL_PROC(1);
            _FL_PROC(2);
            _FL_PROC(3);

            wait_for_all(&fl_sig0, &fl_sig1, &fl_sig2, &fl_sig3);

            data_dma_seq_issued++;
            _ISSUE_PROC(0, RX_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(1, RX_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(2, RX_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(3, RX_DATA_EVENT_TYPE, data_dma_seq_issued);

            break;
        case 3:
            _FL_PROC(0);
            _FL_PROC(1);
            _FL_PROC(2);

            _FL_CLR(3);

            wait_for_all(&fl_sig0, &fl_sig1, &fl_sig2);

            data_dma_seq_issued++;
            _ISSUE_PROC(0, RX_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(1, RX_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(2, RX_DATA_EVENT_TYPE, data_dma_seq_issued);

            _ISSUE_CLR(3);

            break;
        case 2:
            _FL_PROC(0);
            _FL_PROC(1);

            _FL_CLR(2);
            _FL_CLR(3);

            wait_for_all(&fl_sig0, &fl_sig1);

            data_dma_seq_issued++;
            _ISSUE_PROC(0, RX_DATA_IGN_EVENT_TYPE, 0);
            _ISSUE_PROC(1, RX_DATA_EVENT_TYPE, data_dma_seq_issued);

            _ISSUE_CLR(2);
            _ISSUE_CLR(3);

            break;
        case 1:
            _FL_PROC(0);

            _FL_CLR(1);
            _FL_CLR(2);
            _FL_CLR(3);

            wait_for_all(&fl_sig0);

            data_dma_seq_issued++;
            _ISSUE_PROC(0, RX_DATA_EVENT_TYPE, data_dma_seq_issued);

            _ISSUE_CLR(1);
            _ISSUE_CLR(2);
            _ISSUE_CLR(3);

            break;
        default:
            halt();
        }

        _swap_on_msk(&data_wait_msk);
        __implicit_read(&data_sig0);
        __implicit_read(&data_sig1);
        __implicit_read(&data_sig2);
        __implicit_read(&data_sig3);
        __implicit_read(&dma_out_res, sizeof dma_out_res);
    }
}

