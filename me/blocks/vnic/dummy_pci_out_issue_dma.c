/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/dummy_pci_out_issue_dma.c
 * @brief         Code to DMA packet data from the NFP
 */


#include <assert.h>
#include <vnic/shared/nfcc_chipres.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>
#include <nfp/pcie.h>

#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/shared/nfd.h>


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
    _link_sym(nfd_out_sb_wq_credits##_isl)
#define LINK_SB_CREDITS(_isl) LINK_SB_CREDITS_IND(_isl)


EMEM0_QUEUE_ALLOC(nfd_out_issue_dbg_num, global);
__asm .alloc_mem nfd_out_issue_dbg_journal emem0 global SZ_16M SZ_16M;
__asm .init_mu_ring nfd_out_issue_dbg_num nfd_out_issue_dbg_journal;


struct nfd_out_wq_msg {
    union {
        struct {
            unsigned int up:1;          /* The queue is up */
            unsigned int rid:8;         /* Requester ID for the queue */
            unsigned int spare:7;       /* Unused */
            unsigned int seqn:8;        /* Packet sequence number, per queue */
            unsigned int pcie_addr_hi:8; /* High bits of the PCIe address */

            unsigned int pcie_addr_lo;  /* Low bits of the PCIe address */

            unsigned int isl:6;         /* CTM island, zero for MU only pkts */
            unsigned int ctm_only:1;    /* The packet is entirely in CTM */
            unsigned int pktnum:9;      /* CTM packet number */
            unsigned int split:2;       /* Split length allocated to the pkt */
            unsigned int reserved:1;    /* Must be zero from application */
            unsigned int offset:13;     /* Offset where data starts in NFP */

            unsigned int nbi:1;         /* NBI that received the pkt */
            unsigned int bls:2;         /* NBI buffer list */
            unsigned int mu_addr:29;    /* Pkt MU address */

            unsigned int dd:1;          /* Descriptor done, must be set */
            unsigned int meta_len:7;    /* Length of meta data prepended */
            unsigned int queue:8;       /* Queue, bitmask numbered */
            unsigned int data_len:16;   /* Length of frame + meta data */
        };
        unsigned int __raw[5];
    };
};


mem_ring_addr_t in_ring_addr;
unsigned int in_ring_num;
unsigned int credit_addr;


unsigned int dbg_rnum;
mem_ring_addr_t dbg_raddr;


unsigned int pkt_cnt = 0;


__xread struct nfd_out_wq_msg in_msg;
__xwrite unsigned int journal_msg[8];

int
main(void)
{
    if (ctx() != 0) {
        /* We just use CTX0 to avoid any ordering issues */
        ctx_wait(kill);
    }

    /* Input ring */
    in_ring_num = LINK_SB_WQ(PCIE_ISL);
    in_ring_addr = (unsigned long long) NFD_EMEM_LINK(PCIE_ISL) >> 8;

    /* Credit access */
    credit_addr = (unsigned int) (LINK_SB_CREDITS(PCIE_ISL) >> 8);

    /* Debug journal */
    dbg_rnum = _link_sym(nfd_out_issue_dbg_num);
    dbg_raddr = (_link_sym(nfd_out_issue_dbg_journal) >> 8) & 0xff000000;



    for (;;) {
        mem_workq_add_thread(in_ring_num, in_ring_addr, &in_msg, sizeof in_msg);

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
    }
}
