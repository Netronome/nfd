/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/issue_dma.c
 * @brief         Code to DMA packet data to the NFP
 */

#include <assert.h>
#include <nfp.h>
#include <types.h>

#include <nfp/cls.h>                /* TEMP */
#include <nfp/mem_ring.h>           /* TEMP */
#include <std/reg_utils.h>

#include <nfp6000/nfp_cls.h>        /* TEMP */

#include <vnic/pci_in/issue_dma.h>

#include <vnic/pci_in.h>
#include <vnic/pci_in_cfg.h>
#include <vnic/pci_in/pci_in_internal.h>
#include <vnic/shared/qc.h>
#include <vnic/utils/cls_ring.h>
#include <vnic/utils/nn_ring.h>


/* XXX Further data will be required when checking for follow on packets */
static __shared __lmem unsigned int ring_rids[MAX_TX_QUEUES];

/* XXX use CLS ring API when available */
__export __align(sizeof(struct nfd_pci_in_issued_desc) * TX_ISSUED_RING_SZ)
    __cls struct nfd_pci_in_issued_desc tx_issued_ring[TX_ISSUED_RING_SZ];

extern __shared __gpr unsigned int gather_dma_seq_compl;
__shared __gpr unsigned int gather_dma_seq_serv = 0;

extern __shared __gpr unsigned int data_dma_seq_compl;
__shared __gpr unsigned int data_dma_seq_issued = 0;

struct _tx_desc_batch {
    struct nfd_pci_in_tx_desc desc0;
    struct nfd_pci_in_tx_desc desc1;
    struct nfd_pci_in_tx_desc desc2;
    struct nfd_pci_in_tx_desc desc3;
};

struct _issued_pkt_batch {
    struct nfd_pci_in_issued_desc pkt0;
    struct nfd_pci_in_issued_desc pkt1;
    struct nfd_pci_in_issued_desc pkt2;
    struct nfd_pci_in_issued_desc pkt3;
};

__xread struct _tx_desc_batch tx_desc;
__xwrite struct _issued_pkt_batch batch_out;

#define DESC_RING_SZ (MAX_TX_BATCH_SZ * DESC_BATCH_Q_SZ *       \
                      sizeof(struct nfd_pci_in_tx_desc))
__export __shared __cls __align(DESC_RING_SZ) struct nfd_pci_in_tx_desc
    desc_ring[MAX_TX_BATCH_SZ * DESC_BATCH_Q_SZ];

static __shared __gpr unsigned int desc_ring_base;

SIGNAL tx_desc_sig;

__shared __gpr mem_ring_addr_t work_batches_addr; /* TEMP */
MEM_JOURNAL_DECLARE(work_batches, 1024); /* TEMP */

void
issue_dma_setup_shared()
{
    /* XXX complete non-per-queue data */
    unsigned int queue;
    unsigned int vnic;
    unsigned int vnic_q;

    ctassert(__is_log2(MAX_VNICS));
    ctassert(__is_log2(MAX_VNIC_QUEUES));

    nn_ring_init_receive(0, 0);             /* TEMP */

    cls_ring_setup(TX_ISSUED_RING_NUM, tx_issued_ring, sizeof tx_issued_ring);

    /*
     * Initialise the CLS TX descriptor ring
     */
    desc_ring_base = ((unsigned int) &desc_ring) & 0xFFFFFFFF;

    work_batches_addr = MEM_JOURNAL_CONFIGURE(work_batches, 0); /* TEMP */

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
}

void
issue_dma_setup()
{

}


static __intrinsic void dummy_dma(unsigned int dma_mode)
{
    SIGNAL event_sig;
    unsigned int event_swap;
    __xwrite unsigned int evdata;
    unsigned int evaddr = NFP_CLS_AUTOPUSH_USER_EVENT;

    event_swap = ((dma_mode >> 12) & 0xf) | ((dma_mode << 4) & 0xfff0);
    evdata = event_swap;

    __asm cls[write, evdata, evaddr, 0, 1], sig_done[event_sig];

    wait_for_all(&event_sig);
}


/** Parameters list to be filled out as extended */
void
issue_dma()
{
    unsigned int desc_ring_off;
    __cls void *desc_ring_addr;
    __xwrite struct _tx_desc_batch tx_desc_tmp;
    __gpr struct _issued_pkt_batch batch_out_tmp;
    SIGNAL_PAIR jsig_tmp;
    SIGNAL msg_sig;
    unsigned int dma_mode; /* TEMP */

    /* XXX Reorder! */

    /* Check "DMA" completed and we can read the batch
     * If so, the NN ring MUST have a batch descriptor for us */
    if (gather_dma_seq_compl != gather_dma_seq_serv) {
        struct batch_desc batch;
        unsigned char qc_queue;

        if (nn_ring_empty()) {
            halt();          /* A serious error has occurred */
        }

        /* Read the batch */
        batch.__raw = nn_ring_get();
        desc_ring_off = ((gather_dma_seq_serv * sizeof(tx_desc)) &
                         (DESC_RING_SZ - 1));
        gather_dma_seq_serv++; /* Increment before swapping */
        desc_ring_addr = (__cls void *) (desc_ring_base | desc_ring_off);
        __cls_read(&tx_desc, desc_ring_addr, sizeof tx_desc, sizeof tx_desc,
                   sig_done, &tx_desc_sig);

        wait_for_all(&tx_desc_sig); /* TEMP */

        tx_desc_tmp = tx_desc;

        /* Journal the batch */
        mem_ring_journal_fast(0, work_batches_addr, batch.__raw); /* no swap */
        __mem_ring_journal(0, work_batches_addr, &tx_desc_tmp,
                           sizeof tx_desc_tmp, sizeof tx_desc_tmp,
                           sig_done, &jsig_tmp);

        /* XXX replace with blended test */
        if (data_dma_seq_issued < (TX_DATA_MAX_IN_FLIGHT + data_dma_seq_compl)) {
            data_dma_seq_issued++;

            /* Make up a dummy batch_out */
            reg_zero(&batch_out_tmp, sizeof batch_out_tmp);
            batch_out_tmp.pkt0.eop = 1;
            batch_out_tmp.pkt0.q_num = batch.queue;
            batch_out_tmp.pkt0.dst_q = 0;
            batch_out_tmp.pkt0.buf_addr_lo = desc_ring_off;

            batch_out_tmp.pkt1.eop = 1;
            batch_out_tmp.pkt1.q_num = batch.queue;
            batch_out_tmp.pkt1.dst_q = 1;
            batch_out_tmp.pkt1.buf_addr_lo = desc_ring_off;

            batch_out_tmp.pkt2.eop = 1;
            batch_out_tmp.pkt2.q_num = batch.queue;
            batch_out_tmp.pkt2.dst_q = 2;
            batch_out_tmp.pkt2.buf_addr_lo = desc_ring_off;

            batch_out_tmp.pkt3.eop = 1;
            batch_out_tmp.pkt3.q_num = batch.queue;
            batch_out_tmp.pkt3.dst_q = 3;
            batch_out_tmp.pkt3.buf_addr_lo = desc_ring_off;

            batch_out = batch_out_tmp;
            cls_ring_put(TX_ISSUED_RING_NUM, &batch_out, sizeof batch_out,
                         &msg_sig);

            wait_for_all_single(&msg_sig, &jsig_tmp.even);

            dma_mode = ((TX_DATA_EVENT_TYPE << 12) |
                        (data_dma_seq_issued & ((1<<12) - 1)));
            dummy_dma(dma_mode);
        }
    }
}
