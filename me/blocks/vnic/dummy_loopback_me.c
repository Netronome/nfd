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


__shared unsigned long long nrecv = 0;
__shared unsigned long long nsent = 0;
volatile __shared unsigned long long nfail = 0;
__shared __lmem unsigned int cached_credits[NFD_IN_MAX_QUEUES];
__shared __lmem unsigned int fetched_credits[NFD_IN_MAX_QUEUES];


/* Ordering  */
static SIGNAL get_order_sig;
static SIGNAL credit_order_sig;
static SIGNAL send_order_sig;


void main(void)
{
    __xread struct nfd_in_pkt_desc nfd_in_meta;
    __gpr struct nfd_out_input nfd_out_desc;
    __xrw struct nfd_out_input nfd_out_desc_xfer[2];
    __gpr struct nbi_meta_pkt_info pkt_info;
    unsigned int queue;
    unsigned int queue_credits;
    unsigned int vnic;
    __xrw unsigned int credit;
    SIGNAL get_sig;
    SIGNAL_PAIR credit_sig;
    SIGNAL_PAIR send_sig;

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

        nfd_in_recv_init();
        nfd_out_send_init();
    }

    /* Manual delay to allow work queues
     * to become configured! */
    while (local_csr_read(NFP_MECSR_MAILBOX_3) == 0);

    /* Reorder before starting the work loop */
    wait_for_all(&send_order_sig);
    signal_next_ctx(__signal_number(&send_order_sig));


    for (;;) {
        /* Receive a packet */
        __nfd_in_recv(PCIE_ISL, DUMMY_LOOPBACK_WQ, &nfd_in_meta,
                       sig_done, &get_sig);
        wait_for_all(&get_sig, &get_order_sig);
        signal_next_ctx(__signal_number(&get_order_sig));

        nrecv++;
        local_csr_write(NFP_MECSR_MAILBOX_0, (nrecv>>32) & 0xffffffff);
        local_csr_write(NFP_MECSR_MAILBOX_1, nrecv & 0xffffffff);

        /* Extract pkt_info */
        nfd_in_fill_meta(&pkt_info, &nfd_in_meta);

        /* Increment the queue number within the vnic */
        nfd_in_map_queue(&vnic, &queue, nfd_in_meta.q_num);
        queue = queue + 1;
        queue = nfd_out_map_queue(vnic, queue);


        /* Check cached credits and update if necessary */
        wait_for_all(&credit_order_sig);
        queue_credits = cached_credits[queue];
        if (queue_credits != 0) {
            __critical_path();

        } else {
            /* Poll for credits */
            while (queue_credits == 0) {
                __nfd_out_get_credit(PCIE_ISL, queue, CREDIT_BATCH, &credit,
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
        nfd_out_fill_desc(&nfd_out_desc, &pkt_info, 0, 0, NFD_IN_DATA_OFFSET,
                          nfd_in_meta.offset);

        nfd_out_dummy_vlan(&nfd_out_desc, nfd_in_meta.vlan,
                           nfd_in_meta.flags);

        __nfd_out_send(PCIE_ISL, queue, nfd_out_desc_xfer, &nfd_out_desc,
                       sig_done, &send_sig);
        wait_for_all(&send_sig, &send_order_sig);
        signal_next_ctx(__signal_number(&send_order_sig));
        ret = nfd_out_send_test(nfd_out_desc_xfer);
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
