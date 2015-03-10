/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/dummy_source_me.c
 * @brief         Dummy code to source packets for PCI.OUT
 */

#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>

#include <nfp6000/nfp_me.h>

#include <std/reg_utils.h>

#include <vnic/pci_out.h>
#include <vnic/shared/nfcc_chipres.h>
#include <vnic/utils/ordering.h>


#define SEND_BATCH          4
#define VF                  0


/* XXX variables that app and/or BLM should expose */
emem1_queues_DECL;
ASM(.declare_resource BLQ_EMU_RINGS global 8 emem1_queues+4);
ASM(.alloc_resource BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS+0 global 1);

/* XXX variables that app and/or BLM should expose */
_declare_resource("BLQ_EMU_RINGS global 8 emem1_queues+4");
#define APP_BLM_RADDR __LoadTimeConstant("__addr_emem1")


__shared unsigned long long nsent = 0;
__shared unsigned int max_queues = 0;
__shared unsigned int max_packets = 0;
__shared unsigned int pkt_sz = 0;
__shared unsigned int vf_queue = 0;


/* Ordering  */
static SIGNAL get_order_sig;        /* Get buffer handle */
static SIGNAL credit_order_sig;
static SIGNAL send_order_sig;


#define SEND_PKT(_pkt)                                                  \
do {                                                                    \
    pkt_info.muptr = bufs_rd[_pkt];                                     \
    nfd_out_fill_desc(&nfd_out_desc, &pkt_info, 0, 0, 64, 0);           \
    __nfd_out_send(PCIE_ISL, queue, nfd_out_desc_xfer##_pkt, &nfd_out_desc, \
                   sig_done, &send_sig##_pkt);                          \
} while (0)


void main(void)
{
    __gpr struct nbi_meta_pkt_info pkt_info;
    __gpr struct nfd_out_input nfd_out_desc;
    __xrw struct nfd_out_input nfd_out_desc_xfer0[2];
    __xrw struct nfd_out_input nfd_out_desc_xfer1[2];
    __xrw struct nfd_out_input nfd_out_desc_xfer2[2];
    __xrw struct nfd_out_input nfd_out_desc_xfer3[2];

    /* Variables to allocate  MU buffers from the BLM
     * Note constant bls currently */
    unsigned int blm_raddr = ((unsigned long long) APP_BLM_RADDR >> 8);
    unsigned int blm_rnum = _link_sym(BLM_NBI8_BLQ0_EMU_QID) + NFD_IN_BLM_BLS;
        /* _alloc_resource(BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS global 1) + */
        /* NFD_IN_BLM_BLS; */

    static __xread unsigned int bufs_rd[SEND_BATCH];

    unsigned int queue;
    __xrw unsigned int credit;
    SIGNAL_PAIR credit_sig;
    SIGNAL_PAIR send_sig0, send_sig1, send_sig2, send_sig3;

    int ret;

    if (ctx() == 0) {
        /* Kick off ordering */
        signal_ctx(0, __signal_number(&get_order_sig));
        signal_ctx(0, __signal_number(&credit_order_sig));
        signal_ctx(0, __signal_number(&send_order_sig));

        /* Reset the mailboxes */
        local_csr_write(NFP_MECSR_MAILBOX_0, max_queues);
        local_csr_write(NFP_MECSR_MAILBOX_1, max_packets);
        local_csr_write(NFP_MECSR_MAILBOX_2, pkt_sz);
        local_csr_write(NFP_MECSR_MAILBOX_3, nsent);

        nfd_out_send_init();

        while (local_csr_read(NFP_MECSR_MAILBOX_0) == 0);

        max_queues = local_csr_read(NFP_MECSR_MAILBOX_0);
        max_packets = local_csr_read(NFP_MECSR_MAILBOX_1);
        pkt_sz = local_csr_read(NFP_MECSR_MAILBOX_2);
    }

    /* Set dummy vlan values once to minimise per packet work */
    nfd_out_dummy_vlan(&nfd_out_desc, 0, 0);

    /* Set basic packet info */
    pkt_info.isl = 0;   /* Indicate MU only */
    pkt_info.pnum = 0;  /* Indicate MU only */
    pkt_info.bls = NFD_IN_BLM_BLS;
    pkt_info.len = pkt_sz;

    /* Reorder before starting the work loop */
    wait_for_all(&send_order_sig);
    signal_next_ctx(__signal_number(&send_order_sig));

    while(nsent < max_packets) {
        queue = nfd_out_map_queue(VF, vf_queue);
        vf_queue = vf_queue + 1;
        if (vf_queue == max_queues) {
            vf_queue = 0;
        }

        /* Get a batch of buffers */
        wait_for_all(&get_order_sig);
        ret = mem_ring_get(blm_rnum, blm_raddr, bufs_rd, sizeof bufs_rd);
        while (ret != 0) {
            ret = mem_ring_get(blm_rnum, blm_raddr, bufs_rd, sizeof bufs_rd);
        }
        signal_next_ctx(__signal_number(&get_order_sig));


        /* Get a batch of credits, only one ME may poll at a time */
        wait_for_all(&credit_order_sig);
        __nfd_out_get_credit(PCIE_ISL, queue, SEND_BATCH, &credit,
                             ctx_swap, &credit_sig);
        while (credit < SEND_BATCH) {
            if (credit != 0) {
                /* We have encountered an issue.
                 * NFD fetches credits in batches of 8, and this code uses
                 * credits in batches of 4, so we should never get a
                 * non-4B-multiple.
                 */
                halt();
            }

            __nfd_out_get_credit(PCIE_ISL, queue, SEND_BATCH, &credit,
                                 ctx_swap, &credit_sig);
        }
        signal_next_ctx(__signal_number(&credit_order_sig));


        /* Send the packets */
        SEND_PKT(0);
        SEND_PKT(1);
        SEND_PKT(2);
        SEND_PKT(3);
        wait_for_all(&send_sig0, &send_sig1, &send_sig2, &send_sig3,
                     &send_order_sig);
        nsent += SEND_BATCH; /* Don't check for send failures */
        signal_next_ctx(__signal_number(&send_order_sig));

        local_csr_write(NFP_MECSR_MAILBOX_3, nsent);
    }

    /* Catch work completed */
    for (;;) {
        ctx_swap();
    }
}
