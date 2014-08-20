/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/dummy_loopback_me.c
 * @brief         Dummy code to loopback PCI.IN to PCI.OUT
 */

/* NB: This code enqueues one thread per PCI.IN work queue and returns
 * packets received on each work queue to PCI.OUT, via the same queue
 * that they were received on.
 *
 * As there is no ordering between PCI.IN work queues, and by allocating
 * just one thread per work queue each work queue is implicitly ordered,
 * no explicit ordering is performed in this code. */

#include <nfp.h>

#include <nfp/me.h>

#include <nfp6000/nfp_me.h>

#include <std/reg_utils.h>

#include <vnic/pci_in.h>
#include <vnic/pci_in_cfg.h>
#include <vnic/pci_out.h>
#include <vnic/shared/nfd_shared.h>
#include <vnic/utils/ordering.h>


__shared int nrecv = 0;
__shared int nsent = 0;
__shared int nfail = 0;

void main(void)
{
    __xread struct nfd_pci_in_pkt_desc pci_in_meta;
    __gpr struct nfd_pci_out_input pci_out_desc;
    unsigned int bls = TX_BLM_BLS;
    unsigned int queue = 0;
    unsigned int pktlen;
    __xrw unsigned int credit;
    SIGNAL get_sig;
    SIGNAL_PAIR credit_sig;

    int ret;

    if (ctx() == 0) {
        local_csr_write(NFP_MECSR_MAILBOX_0, 0);
        local_csr_write(NFP_MECSR_MAILBOX_1, 0);
        local_csr_write(NFP_MECSR_MAILBOX_2, 0);
    }

    /* Cheep and nasty delay to allow work queues
     * to become configured! */
    sleep(7000);

    for (;;) {
        __nfd_pkt_recv(PCIE_ISL, ctx(), &pci_in_meta, sig_done, &get_sig);
        wait_for_all(&get_sig);

        nrecv++;
        local_csr_write(NFP_MECSR_MAILBOX_0, nrecv);

        queue = pci_in_meta.q_num;

        /* Get a credit */
        __pci_out_get_credit(PCIE_ISL, queue, 1, &credit,
                             ctx_swap, &credit_sig);
        if (credit != 0) {
            /* Return the packet */
            pci_out_fill_addr_mu_only(&pci_out_desc, pci_in_meta.buf_addr, 0,
                                      bls);

            /* XXX alternative:
             * pktlen = pci_in_meta.data_len - pci_in_meta.offset; */
            pktlen = pci_in_pkt_len(&pci_in_meta);

            pci_out_fill_size(&pci_out_desc, TX_DATA_OFFSET, pktlen,
                              pci_in_meta.offset);
            pci_out_dummy_vlan(&pci_out_desc, pci_in_meta.vlan,
                               pci_in_meta.flags);

            ret = pci_out_send(PCIE_ISL, queue, &pci_out_desc);

            if (ret >= 0) {
                nsent++;
            } else {
                nfail++;
            }

        } else {
            /* Drop the packet */
            nfail++;

        }

        local_csr_write(NFP_MECSR_MAILBOX_1, nsent);
        local_csr_write(NFP_MECSR_MAILBOX_2, nfail);
    }
}

