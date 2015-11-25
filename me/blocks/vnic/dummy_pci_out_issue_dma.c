/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/dummy_pci_out_issue_dma.c
 * @brief         Code to DMA packet data from the NFP
 */


#include <assert.h>
#include <nfp.h>
#include <nfp_chipres.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>
#include <nfp/pcie.h>

#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/pci_out.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_internal.h>


#include <nfp_net_ctrl.h>


NFD_BLM_Q_ALLOC(NFD_OUT_BLM_POOL_START);

#define DECLARE_SB_IND2(_isl, _emem)                                    \
    _emem##_queues_DECL                                                 \
    ASM(.alloc_resource nfd_out_sb_ring_num##_isl _emem##_queues global 1 1) \
    ASM(.alloc_mem nfd_out_sb_wq_credits##_isl ctm island 4 4)
#define DECLARE_SB_IND1(_isl, _emem) DECLARE_SB_IND2(_isl, _emem)
#define DECLARE_SB_IND0(_isl) DECLARE_SB_IND1(_isl, NFD_PCIE##_isl##_EMEM)
#define DECLARE_SB(_isl) DECLARE_SB_IND0(_isl)

DECLARE_SB(PCIE_ISL);

#define LINK_SB_WQ_IND(_isl)                    \
    _link_sym(nfd_out_sb_ring_num##_isl)
#define LINK_SB_WQ(_isl) LINK_SB_WQ_IND(_isl)

#define LINK_SB_CREDITS_IND(_isl)               \
    ((__mem void *) _link_sym(nfd_out_sb_wq_credits##_isl))
#define LINK_SB_CREDITS(_isl) LINK_SB_CREDITS_IND(_isl)

#define LINK_DMA_DONE_IND(_isl)                         \
    (__LoadTimeConstant("__addr_pcie" #_isl "_ctm"))
#define LINK_DMA_DONE(_isl) LINK_DMA_DONE_IND(_isl)


EMEM0_QUEUE_ALLOC(nfd_out_issue_dbg_num, global);
__asm .alloc_mem nfd_out_issue_dbg_journal emem0 global SZ_16M SZ_16M;
__asm .init_mu_ring nfd_out_issue_dbg_num nfd_out_issue_dbg_journal;


#define NFD_PCI_OUT_SB_CREDIT_WM    64


struct nfd_out_wq_msg {
    union {
        struct {
            unsigned int up:1;           /* The queue is up */
            unsigned int rid:8;          /* Requester ID for the queue */
            unsigned int spare:5;        /* Unused */
            unsigned int seqn:10;        /* Packet sequence number, per queue */
            unsigned int pcie_addr_hi:8; /* High bits of the PCIe address */

            unsigned int pcie_addr_lo;  /* Low bits of the PCIe address */

            unsigned int isl:6;         /* CTM island, zero for MU only pkts */
            unsigned int ctm_only:1;    /* The packet is entirely in CTM */
            unsigned int pktnum:9;      /* CTM packet number */
            unsigned int split:2;       /* Split length allocated to the pkt */
            unsigned int reserved:1;    /* Must be zero from application */
            unsigned int offset:13;     /* Offset where data starts in NFP */

            unsigned int blq:3;         /* BLM buffer queue (nbi:bls) */
            unsigned int mu_addr:29;    /* Pkt MU address */

            unsigned int dd:1;          /* Descriptor done, must be set */
            unsigned int meta_len:7;    /* Length of meta data prepended */
            unsigned int queue:8;       /* Queue, bitmask numbered */
            unsigned int data_len:16;   /* Length of frame + meta data */
        };
        unsigned int __raw[5];
    };
};


/* XXX move to some sort of CT library */
__intrinsic void
send_interthread_sig(unsigned int dst_me, unsigned int ctx, unsigned int sig_no)
{
    unsigned int addr;

    /* Generic address computation.
     * Could be expensive if dst_me, or dst_xfer
     * not compile time constants */
    addr = ((dst_me & 0x3F0)<<20 | (dst_me & 15)<<9 | (ctx & 7) << 6 |
            (sig_no & 15)<<2);

    // REMOVE ME
    local_csr_write(local_csr_mailbox_0, addr);
    __asm ct[interthread_signal, --, addr, 0, --];
}


mem_ring_addr_t in_ring_addr;
unsigned int in_ring_num;


unsigned int dbg_rnum;
mem_ring_addr_t dbg_raddr;


mem_ring_addr_t blm_raddr;
unsigned int blm_rnum_start;


unsigned int pkt_cnt = 0;


__xread struct nfd_out_wq_msg in_msg;
__xwrite unsigned int journal_msg[8];
__gpr struct nfp_pcie_dma_cmd descr_tmp;
__xwrite struct nfp_pcie_dma_cmd descr;

SIGNAL dma_compl;

__shared __gpr unsigned int credits_to_return = 0;
__remote SIGNAL nfd_credit_sig_sb;

unsigned int dma_done_hi;


void
issue_dma_setup_shared()
{
    struct pcie_dma_cfg_one cfg;

    /*
     * Set up NFD_OUT_DATA_CFG_REG Config Register
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
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_OUT_DATA_CFG_REG, cfg);

    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, and PCIe address will be
     * overwritten per transaction.
     * For dma_mode, we technically only want to overwrite the "source"
     * field, i.e. 12 of the 16 bits.
     */
    descr_tmp.rid_override = 1;
    descr_tmp.trans_class = 0;
    descr_tmp.cpp_token = NFD_OUT_DATA_DMA_TOKEN;
    descr_tmp.dma_cfg_index = NFD_OUT_DATA_CFG_REG;
}


int
main(void)
{
    unsigned int ctx;
    unsigned int meid;

    //if (ctx() != 0) {
        /* We just use CTX0 to avoid any ordering issues */
        ctx_wait(kill);
    //}

    /* Extract thread data */
    ctx = ctx();
    meid = __MEID;

    /* Input ring */
    in_ring_num = LINK_SB_WQ(PCIE_ISL);
    in_ring_addr = (unsigned long long) NFD_EMEM_LINK(PCIE_ISL) >> 8;

    /* Credit access */
    dma_done_hi = (unsigned long long) (LINK_DMA_DONE(PCIE_ISL) >> 8);

    /* Debug journal */
    dbg_rnum = _link_sym(nfd_out_issue_dbg_num);
    dbg_raddr = (_link_sym(nfd_out_issue_dbg_journal) >> 8) & 0xff000000;

    /* Freeing packets */
    blm_raddr = ((unsigned long long) NFD_OUT_BLM_RADDR >> 8) & 0xff000000;
    blm_rnum_start = NFD_BLM_Q_LINK(NFD_OUT_BLM_POOL_START);


    /* Setup DMA CFG and template */
    issue_dma_setup_shared();

#if 1 /* XXX kill all contexts */
    ctx_wait(kill);
#endif

    for (;;) {
        unsigned int queue;
        unsigned int ctm_hi;
        unsigned int ctm_lo;
        unsigned int dma_done_lo;
        unsigned int blm_rnum;
        unsigned int pktnum;

        /* Get a packet and return the credit */
        mem_workq_add_thread(in_ring_num, in_ring_addr, &in_msg,
                             sizeof in_msg);
        credits_to_return++;

        pkt_cnt++;

        /* Journal whatever came off the work queue */
        journal_msg[0] = in_msg.__raw[0];
        journal_msg[1] = in_msg.__raw[1];
        journal_msg[2] = in_msg.__raw[2];
        journal_msg[3] = in_msg.__raw[3];
        journal_msg[4] = in_msg.__raw[4];

        journal_msg[5] = pkt_cnt;
        journal_msg[6] = 0;
        journal_msg[7] = 0;

        mem_ring_journal(dbg_rnum, dbg_raddr, journal_msg, sizeof journal_msg);


        /* Issue the DMA */
        queue = in_msg.queue;

        if (in_msg.ctm_only == 0) {
            /* Only handle the fast path */
            halt();
        }

        if (in_msg.isl == 0) {
            /* Only handle the fast path */
            halt();
        }


        if (in_msg.up) {
            /* ctm_hi address will be reused when freeing the buffer */
            ctm_hi = 0x80 | in_msg.isl;
            ctm_lo = 0x80000000 | (in_msg.pktnum << 16);

            descr_tmp.cpp_addr_hi = ctm_hi;
            descr_tmp.cpp_addr_lo = ctm_lo | in_msg.offset;

            /* PCIe addresses */
            descr_tmp.pcie_addr_hi = in_msg.pcie_addr_hi;
            descr_tmp.pcie_addr_lo = in_msg.pcie_addr_lo;
            descr_tmp.pcie_addr_lo += NFP_NET_RX_OFFSET;
            descr_tmp.pcie_addr_lo -= in_msg.meta_len;

            /* Finish off and issue the DMA */
            descr_tmp.length = in_msg.data_len - 1;
            descr_tmp.rid = in_msg.rid;
            pcie_dma_set_sig(&descr_tmp, meid, ctx,
                             __signal_number(&dma_compl));
            descr = descr_tmp;
            pcie_dma_enq(PCIE_ISL, &descr, NFD_OUT_DATA_DMA_QUEUE);
            __implicit_write(&dma_compl);


            /* Wait for the DMA to complete */
            wait_for_all(&dma_compl);

            /* Increment the dma_done atomic */
            dma_done_lo = ((queue * NFD_OUT_ATOMICS_SZ) |
            NFD_OUT_ATOMICS_DMA_DONE);
            __asm { mem[incr, --, dma_done_hi, <<8, dma_done_lo] };
        }

        /* Free the buffer */
        blm_rnum = blm_rnum_start + in_msg.blq;
        mem_ring_journal_fast(blm_rnum, blm_raddr, in_msg.mu_addr);

        if (in_msg.ctm_only == 1) {
            ctm_hi = ctm_hi << 24;
            pktnum = in_msg.pktnum;

            __asm mem[packet_free, --, ctm_hi, <<8, pktnum];
        }


        /* Return credits to stage batch if GE watermark */
        if (credits_to_return >= NFD_PCI_OUT_SB_CREDIT_WM) {
            /* Add back the credits atomically */
            mem_add32_imm(credits_to_return, LINK_SB_CREDITS(PCIE_ISL));
            credits_to_return = 0;

            /* Notify stage batch to check the credits */
            send_interthread_sig(NFD_OUT_STAGE_ME, 0,
                                 __signal_number(&nfd_credit_sig_sb,
                                                 NFD_OUT_STAGE_ME));
        }
    }
}
