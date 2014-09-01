/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/dummy_rx_me0.c
 * @brief         Dummy code for ME serving as RX ME
 */

#include <nfp.h>

#include <nfp/me.h>

#include <nfp6000/nfp_me.h>

#include <pkt/pkt.h>
#include <std/reg_utils.h>

#include <vnic/pci_out.h>
#include <vnic/utils/ordering.h>


#ifndef PKT_NBI_OFFSET
#warning "PKT_NBI_OFFSET not #defined: assuming 64"
#define PKT_NBI_OFFSET 64
#endif

#ifndef MY_NBI
#warning "No MY_NBI #defined: assuming 0"
#define MY_NBI 0
#endif

#define OUT_TXQ 0

__shared int nrecv = 0;
__shared int nsent = 0;
__shared int nfail = 0;

/* Ordering  */
#define _START_CTX      0
static SIGNAL get_order_sig;
static SIGNAL get_sig;
static SIGNAL wait_order_sig;



void main(void)
{
    __gpr struct pkt_ms_info msi;
    __addr40 char *pbuf;
    __xread struct nbi_meta_null nbi_meta;
    __xread struct nbi_meta_pkt_info *pi = &nbi_meta.pkt_info;

    __gpr struct nfd_pci_out_input pci_out_desc;
    __xwrite struct nfd_pci_out_input tmp;
    unsigned int queue;
    int ret;

    unsigned int zero = 0;
    SIGNAL sig;

    if (ctx() == 0) {
        reorder_start(_START_CTX, &get_order_sig);
        reorder_start(_START_CTX, &wait_order_sig);

        local_csr_write(NFP_MECSR_MAILBOX_0, 0);
        local_csr_write(NFP_MECSR_MAILBOX_1, 0);
        local_csr_write(NFP_MECSR_MAILBOX_2, 0);
    }


    for (;;) {
        reorder_test_swap(&get_order_sig);
        __pkt_nbi_recv(&nbi_meta, sizeof(nbi_meta), sig_done, &get_sig);
        reorder_done(_START_CTX, &get_order_sig);

        wait_for_all(&get_sig, &wait_order_sig);
        reorder_done(_START_CTX, &wait_order_sig);

        nrecv++;
        local_csr_write(NFP_MECSR_MAILBOX_0, nrecv);

        pci_out_fill_addr(&pci_out_desc, pi->isl, pi->pnum, pi->muptr, 0,
                          pi->bls);
        pci_out_fill_size(&pci_out_desc, PKT_NBI_OFFSET, pi->len, 0);
        pci_out_dummy_vlan(&pci_out_desc, 0x1234, 0xd);

        queue = pci_out_map_queue(0, 0);
        ret = pci_out_send(PCIE_ISL, queue, &pci_out_desc);


/*         /\* pbuf = pkt_ctm_ptr40(pi->isl, pi->pnum, 0); *\/ */
/*         /\* msi = pkt_nop_ms_write(pbuf, PKT_NBI_OFFSET); *\/ */
/*         /\* pkt_nbi_send(0, pi->pnum, &msi, pi->len, MY_NBI ^ 1, OUT_TXQ, *\/ */
/*         /\*              nbi_meta.seqr, nbi_meta.seq); *\/ */

        if (ret >= 0) {
            nsent++;
        } else {
            nfail++;
        }

        local_csr_write(NFP_MECSR_MAILBOX_1, nsent);
        local_csr_write(NFP_MECSR_MAILBOX_2, nfail);
    }
}
