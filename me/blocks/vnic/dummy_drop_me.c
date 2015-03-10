/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/dummy_drop_me.c
 * @brief         Dummy code to count and drop PCI.IN packets
 */

#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>

#include <nfp6000/nfp_me.h>

#include <std/reg_utils.h>

#include <vnic/pci_in.h>
#include <vnic/shared/nfcc_chipres.h>


#ifndef DUMMY_DROP_WQ
#define DUMMY_DROP_WQ   0
#endif


/* XXX variables that app and/or BLM should expose */
emem1_queues_DECL;
ASM(.declare_resource BLQ_EMU_RINGS global 8 emem1_queues+4);
ASM(.alloc_resource BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS+0 global 1);

/* XXX variables that app and/or BLM should expose */
_declare_resource("BLQ_EMU_RINGS global 8 emem1_queues+4");
#define APP_BLM_RADDR __LoadTimeConstant("__addr_emem1")



__shared unsigned long long nrecv = 0;
__shared unsigned long long ndrop = 0;


void main(void)
{
    __xread struct nfd_in_pkt_desc nfd_in_meta;
    __gpr struct nbi_meta_pkt_info pkt_info;

    /* Variables to return MU buffers to BLM when dropping packets
     * Note constant bls currently */
    unsigned int blm_raddr = ((unsigned long long) APP_BLM_RADDR >> 8);
    unsigned int blm_rnum = _link_sym(BLM_NBI8_BLQ0_EMU_QID) + NFD_IN_BLM_BLS;
        /* _alloc_resource(BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS global 1) + */
        /* NFD_IN_BLM_BLS; */

    SIGNAL get_sig;

    int ret;

    if (ctx() == 0) {
        /* Clear the manual delay flag */
        local_csr_write(NFP_MECSR_MAILBOX_3, 0); /* Ensure usage shadow */

        /* Clear counters */
        local_csr_write(NFP_MECSR_MAILBOX_0, 0);
        local_csr_write(NFP_MECSR_MAILBOX_1, 0);
        local_csr_write(NFP_MECSR_MAILBOX_2, 0);

        nfd_in_recv_init();
    }

    /* Manual delay to allow work queues
     * to become configured! */
    while (local_csr_read(NFP_MECSR_MAILBOX_3) == 0);

    for (;;) {
        /* Receive a packet */
        __nfd_in_recv(PCIE_ISL, DUMMY_DROP_WQ, &nfd_in_meta,
                       ctx_swap, &get_sig);

        nrecv++;
        local_csr_write(NFP_MECSR_MAILBOX_0, (nrecv>>32) & 0xffffffff);
        local_csr_write(NFP_MECSR_MAILBOX_1, nrecv & 0xffffffff);

        /* Drop the packet */
        /* XXX this assumes that there is no CTM buffer! */
        mem_ring_journal_fast(blm_rnum, blm_raddr, nfd_in_meta.buf_addr);
        ndrop++;
        local_csr_write(NFP_MECSR_MAILBOX_2, (ndrop >> 32) & 0xffffffff);
        local_csr_write(NFP_MECSR_MAILBOX_3, ndrop & 0xffffffff);
    }
}
