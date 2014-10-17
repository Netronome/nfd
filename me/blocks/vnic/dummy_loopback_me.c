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
#include <nfp/mem_ring.h>

#include <nfp6000/nfp_me.h>

#include <std/reg_utils.h>

#include <vnic/pci_in.h>
#include <vnic/pci_in_cfg.h>
#include <vnic/pci_out.h>
#include <vnic/shared/nfd_shared.h>
#include <vnic/utils/ordering.h>


#ifndef DUMMY_LOOPBACK_WQ
#define DUMMY_LOOPBACK_WQ   0
#endif

#define CREDIT_BATCH        16

/* XXX variables that app and/or BLM should expose */
_declare_resource("BLQ_EMU_RINGS global 8 emem1_queues+4");
#define APP_BLM_RADDR __LoadTimeConstant("__addr_emem1")

__shared unsigned long long nrecv = 0;
__shared unsigned long long nsent = 0;
volatile __shared unsigned long long nfail = 0;
__shared __lmem unsigned int cached_credits[MAX_TX_QUEUES];
__shared __lmem unsigned int fetched_credits[MAX_TX_QUEUES];


/* Ordering  */
static SIGNAL get_order_sig;
static SIGNAL credit_order_sig;
static SIGNAL send_order_sig;


void main(void)
{
    __xread struct nfd_pci_in_pkt_desc pci_in_meta;
    __gpr struct nfd_pci_out_input pci_out_desc;
    __xrw struct nfd_pci_out_input pci_out_desc_xfer[2];
    unsigned int bls = TX_BLM_BLS;
    unsigned int queue;
    unsigned int queue_credits;
    unsigned int vnic;
    unsigned int pktlen;
    __xrw unsigned int credit;
    SIGNAL get_sig;
    SIGNAL_PAIR credit_sig;
    SIGNAL_PAIR send_sig;

    /* Variables to return MU buffers to BLM when dropping packets
     * Note constant bls currently */
    unsigned int blm_raddr = ((unsigned long long) APP_BLM_RADDR >> 8);
    unsigned int blm_rnum =
        _alloc_resource(BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS global 1) + bls;


    int ret;

    if (ctx() == 0) {
        /* Kick off ordering */
        signal_ctx(0, __signal_number(&get_order_sig));
        signal_ctx(0, __signal_number(&credit_order_sig));
        signal_ctx(0, __signal_number(&send_order_sig));

        /* Clear the manual delay flag */
        local_csr_write(NFP_MECSR_MAILBOX_3, 0); /* Ensure usage shadow */

        /* Clear counters */
        local_csr_write(NFP_MECSR_MAILBOX_0, 0);
        local_csr_write(NFP_MECSR_MAILBOX_1, 0);
        local_csr_write(NFP_MECSR_MAILBOX_2, 0);
    }

    /* Manual delay to allow work queues
     * to become configured! */
    while (local_csr_read(NFP_MECSR_MAILBOX_3) == 0);

    /* Reorder before starting the work loop */
    wait_for_all(&send_order_sig);
    signal_next_ctx(__signal_number(&send_order_sig));


    for (;;) {
        /* Receive a packet */
        __nfd_pkt_recv(PCIE_ISL, DUMMY_LOOPBACK_WQ, &pci_in_meta,
                       sig_done, &get_sig);
        wait_for_all(&get_sig, &get_order_sig);
        signal_next_ctx(__signal_number(&get_order_sig));

        nrecv++;
        local_csr_write(NFP_MECSR_MAILBOX_0, (nrecv>>32) & 0xffffffff);
        local_csr_write(NFP_MECSR_MAILBOX_1, nrecv & 0xffffffff);

        /* Increment the queue number within the vnic */
        pci_in_map_queue(&vnic, &queue, pci_in_meta.q_num);
        queue = queue + 1;
        queue = pci_out_map_queue(vnic, queue);


        /* Check cached credits and update if necessary */
        wait_for_all(&credit_order_sig);
        queue_credits = cached_credits[queue];
        if (queue_credits != 0) {
            __critical_path();

        } else {
            /* Poll for credits */
            while (queue_credits == 0) {
                __pci_out_get_credit(PCIE_ISL, queue, CREDIT_BATCH, &credit,
                                     ctx_swap, &credit_sig);
                if (credit >= CREDIT_BATCH) {
                    queue_credits += CREDIT_BATCH;
                    fetched_credits[queue] += CREDIT_BATCH;
                } else {
                    queue_credits = credit;
                    fetched_credits[queue] += credit;
                }
            }
        }
        signal_next_ctx(__signal_number(&credit_order_sig));

        /* Use a credit and return to cache */
        queue_credits--;
        cached_credits[queue] = queue_credits;

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

        __pci_out_send(PCIE_ISL, queue, pci_out_desc_xfer, &pci_out_desc,
                       sig_done, &send_sig);
        wait_for_all(&send_sig, &send_order_sig);
        signal_next_ctx(__signal_number(&send_order_sig));
        ret = pci_out_send_test(pci_out_desc_xfer);
        if (ret >= 0) {
            nsent++;
        } else {
            nfail++;
        }

        local_csr_write(NFP_MECSR_MAILBOX_2, (nsent >> 32) & 0xffffffff);
        local_csr_write(NFP_MECSR_MAILBOX_3, nsent & 0xffffffff);
        /* local_csr_write(NFP_MECSR_MAILBOX_2, nfail); */
    }
}

