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

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_bulk.h>
#include <nfp/mem_ring.h>
#include <nfp/mem_atomic.h>

#include <nfp6000/nfp_me.h>

#include <std/reg_utils.h>

#include <vnic/shared/nfd_cfg.h>
#include <vnic/pci_in.h>
#include <vnic/pci_out.h>
#include <vnic/utils/ordering.h>


#ifndef DUMMY_LOOPBACK_WQ
#define DUMMY_LOOPBACK_WQ   0
#endif


/* XXX variables that app and/or BLM should expose */
emem1_queues_DECL;
ASM(.declare_resource BLQ_EMU_RINGS global 8 emem1_queues+4);
ASM(.alloc_resource BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS+0 global 1);


_declare_resource("BLQ_EMU_RINGS global 8 emem1_queues+4");
#define APP_BLM_RADDR __LoadTimeConstant("__addr_emem1")


#define NO_CREDIT_SLEEP         256
#define NO_BUF_SLEEP            256
#define COPY_CHUNK              64


__shared unsigned long long nrecv = 0;
__shared unsigned long long nsent = 0;
volatile __shared unsigned long long nfail = 0;

/* Ordering  */
static SIGNAL get_order_sig;
static SIGNAL credit_order_sig;
static SIGNAL send_order_sig;
#ifndef LOOPBACK_MU_ONLY
static SIGNAL ctm_order_sig;
#endif

/* Variables to return MU buffers to BLM when dropping packets */
unsigned int blm_raddr;
unsigned int blm_rnum_base;


#define NDROP_IND(_isl) ndrop##_isl
#define NDROP(_isl) NDROP_IND(_isl)

__export __emem __align(NFD_OUT_MAX_QUEUES * 4)
    unsigned int NDROP(PCIE_ISL)[NFD_OUT_MAX_QUEUES];


NFD_CFG_BASE_DECLARE(PCIE_ISL);


/* Pack a CTM address for use in addr_hi
 * XXX replace with API call */
unsigned int
ctm_addr_hi(unsigned int isl, unsigned int pnum)
{
    unsigned int addr_hi;

    addr_hi = 0x80800000 | (isl << 24);
    addr_hi |= (pnum & 0x1ff) << 8;
    return addr_hi;
}

/* Pack an MU address for use in addr_hi
 * XXX replace with API call */
unsigned int
mu_addr_hi(unsigned int mubuf)
{
    return mubuf << (11 - 8);
}

/* Copy 64B chunks from MU to CTM until the full length is copied
 * XXX replace with API call*/
__intrinsic void
cp_ctm_data(unsigned int ctm_base, unsigned int mu_base, unsigned int start,
            unsigned int max_bytes, unsigned int pkt_len)
{
    __xread unsigned int data_rd[COPY_CHUNK / sizeof(unsigned int)];
    __xwrite unsigned int data_wr[COPY_CHUNK / sizeof(unsigned int)];
    SIGNAL cp_sig;
    unsigned int offset;
    unsigned int end_offset;
    unsigned int count = COPY_CHUNK >> 3;

    ctassert(__is_ct_const(start));
    ctassert((start % COPY_CHUNK) == 0);
    ctassert(__is_ct_const(max_bytes));
    ctassert((max_bytes % COPY_CHUNK) == 0);
    ctassert((COPY_CHUNK / 8) <= 8);

    end_offset = max_bytes;
    if (pkt_len < max_bytes) {
        end_offset = pkt_len;
    }
    end_offset += start;

    for (offset = 0; offset < end_offset; offset += COPY_CHUNK) {
        __asm mem[read, data_rd, mu_base, <<8, offset, count], ctx_swap[cp_sig];
        reg_cp(data_wr, data_rd, COPY_CHUNK);
        __asm mem[write, data_wr, ctm_base, <<8, offset, count], \
            ctx_swap[cp_sig];
    }
}


/* Get a 256B CTM buffer from the local island
 * Use of packet_alloc_poll assumes that only MEs use this CTM
 * XXX replace with API call */
__intrinsic void
__get_ctm_buf_256(__xread unsigned int *ctm_buf, sync_t sync, SIGNAL *sig)
{
    unsigned int addr_hi = 0;
    unsigned int ind = NFP_MECSR_PREV_ALU_OV_LEN;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    if (sync == sig_done) {
        __asm alu[--, --, b, ind];
        __asm mem[packet_alloc_poll, *ctm_buf, addr_hi, 0, 1], indirect_ref, \
            sig_done[*sig];
    } else {
        __asm alu[--, --, b, ind];
        __asm mem[packet_alloc_poll, *ctm_buf, addr_hi, 0, 1], indirect_ref, \
            ctx_swap[*sig];
    }
}

/* Free the CTM and MU buffer */
__intrinsic void
drop_pkt(__gpr struct nbi_meta_pkt_info *pkt_info, unsigned int bmsk_queue)
{
    if (pkt_info->isl != 0)
    {
        /* Free CTM buffer */
        pkt_ctm_free(pkt_info->isl, pkt_info->pnum);
    }

    /* Always free MU buffer */
    mem_ring_journal_fast(blm_rnum_base + pkt_info->bls, blm_raddr,
                          pkt_info->muptr);

    /* Increment a per queue EMEM counter */
    mem_incr32(&NDROP(PCIE_ISL)[bmsk_queue]);
}


/* Check CFG BAR for ring enables */
__intrinsic int
test_enable(unsigned int vnic, unsigned int queue) {
    __emem char *ptr;
    __xread unsigned int enables;

    /* Check global enable */
    ptr = NFD_CFG_BASE(PCIE_ISL)[vnic] + NS_VNIC_CFG_CTRL;
    mem_read32(&enables, ptr, sizeof(enables));
    if (!(enables & NS_VNIC_CFG_CTRL_ENABLE)) {
        return 0;
    }

    /* Check per ring enable */
    ptr = NFD_CFG_BASE(PCIE_ISL)[vnic] + NS_VNIC_CFG_RXRS_ENABLE;
    if (queue & 32) {
        ptr += sizeof(unsigned int);
    }
    queue &= 31;
    mem_read32(&enables, ptr, sizeof(enables));
    return ((enables >> queue) & 1);
}


void main(void)
{
    __xread struct nfd_in_pkt_desc nfd_in_meta;
    __gpr struct nfd_out_input nfd_out_desc;
    __xrw struct nfd_out_input nfd_out_desc_xfer[2];
    __gpr struct nbi_meta_pkt_info pkt_info;

    unsigned int bmsk_queue;
    unsigned int queue;
    unsigned int vnic;
    __xrw unsigned int credit;

    SIGNAL get_sig;
    SIGNAL_PAIR credit_sig;
    SIGNAL_PAIR send_sig;

#ifndef LOOPBACK_MU_ONLY
    unsigned int island = __ISLAND;
    __xread unsigned int ctm_buf;
    unsigned int ctm_addr;
    unsigned int mu_addr;
    SIGNAL ctm_buf_sig;
#endif

    int ret;

    if (ctx() == 0) {
        /* Kick off ordering */
        signal_ctx(0, __signal_number(&get_order_sig));
        __implicit_write(&get_order_sig);
        signal_ctx(0, __signal_number(&credit_order_sig));
        __implicit_write(&credit_order_sig);
        signal_ctx(0, __signal_number(&send_order_sig));
        __implicit_write(&send_order_sig);
#ifndef LOOPBACK_MU_ONLY
        signal_ctx(0, __signal_number(&ctm_order_sig));
        __implicit_write(&ctm_order_sig);
#endif

        /* Clear the manual delay flag */
        local_csr_write(NFP_MECSR_MAILBOX_3, 0); /* Ensure usage shadow */

        /* Clear counters */
        local_csr_write(NFP_MECSR_MAILBOX_0, 0);
        local_csr_write(NFP_MECSR_MAILBOX_1, 0);
        local_csr_write(NFP_MECSR_MAILBOX_2, 0);

        nfd_in_recv_init();
        nfd_out_send_init();
    }

    blm_raddr = ((unsigned long long) APP_BLM_RADDR >> 8);
    blm_rnum_base = _link_sym(BLM_NBI8_BLQ0_EMU_QID);
        /* _alloc_resource(BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS global 1); */

    /* Manual delay to allow work queues
     * to become configured! */
    while (local_csr_read(NFP_MECSR_MAILBOX_3) == 0);

    /* Reorder before starting the work loop */
    wait_for_all(&send_order_sig);
    signal_next_ctx(__signal_number(&send_order_sig));
    __implicit_write(&send_order_sig);


    for (;;) {
        /* Receive a packet */
        __nfd_in_recv(PCIE_ISL, DUMMY_LOOPBACK_WQ, &nfd_in_meta,
                      sig_done, &get_sig);
        wait_for_all(&get_sig, &get_order_sig);
        signal_next_ctx(__signal_number(&get_order_sig));
        __implicit_write(&get_order_sig);

        nrecv++;
        local_csr_write(NFP_MECSR_MAILBOX_0, (nrecv>>32) & 0xffffffff);
        local_csr_write(NFP_MECSR_MAILBOX_1, nrecv & 0xffffffff);

        /* Extract pkt_info */
        nfd_in_fill_meta(&pkt_info, &nfd_in_meta);


        /* Map the packet vnic and queue */
        nfd_in_map_queue(&vnic, &queue, nfd_in_meta.q_num);

        /* Pick loopback queue and vNIC */
#ifdef LOOPBACK_FLIP_QUEUE
        queue = queue ^ 1;
#endif
#ifdef LOOPBACK_FLIP_VNIC
        vnic = vnic ^ 1;
#endif

        /* PCI.OUT transmit */
        bmsk_queue = nfd_out_map_queue(vnic, queue);


        /* Get a credit, this does not need to be ordered provided that
         * the number of credits that PCI.OUT issues per queue is
         * much greater than the number of worker contexts */
        __nfd_out_get_credit(PCIE_ISL, bmsk_queue, 1, &credit,
                             ctx_swap, &credit_sig);
        while (credit == 0) {
            /* Check whether the queue is up */
            if (!test_enable(vnic, queue)) {
                /* Exit the loop leaving the CTX without a credit */
                break;
            }

            /* Throttle credit polling */
            sleep(NO_CREDIT_SLEEP);

            /* Try to claim a credit again */
            __nfd_out_get_credit(PCIE_ISL, bmsk_queue, 1, &credit,
                                 ctx_swap, &credit_sig);
        }

        /* Reorder on exit */
        if (!signal_test(&credit_order_sig)) {
            wait_for_all(&credit_order_sig);
        }
        signal_next_ctx(__signal_number(&credit_order_sig));
        __implicit_write(&credit_order_sig);


#ifndef LOOPBACK_MU_ONLY
        /* Delay allocating the CTM buffer until we know we have
         * a PCI.OUT credit.  This means we are unlikely to leave
         * CTM buffers used if the host app is killed or crashes. */

        /* XXX use API to allocate CTM buffer */
        __get_ctm_buf_256(&ctm_buf, ctx_swap, &ctm_buf_sig);
        while (((int) ctm_buf) < 0) {
            /* Throttle PE polling */
            sleep(NO_BUF_SLEEP);
            __get_ctm_buf_256(&ctm_buf, ctx_swap, &ctm_buf_sig);
        }

        pkt_info.isl = island;
        pkt_info.pnum = (ctm_buf >> 20);
        if (pkt_info.len > (256 - NFD_IN_DATA_OFFSET)) {
            pkt_info.split = 1;
        }

        /* XXX use API to copy data from MU to CTM */
        ctm_addr = ctm_addr_hi(island, ctm_buf >> 20);
        mu_addr = mu_addr_hi(nfd_in_meta.buf_addr);
        cp_ctm_data(ctm_addr, mu_addr, NFD_IN_DATA_OFFSET,
                    (256 - NFD_IN_DATA_OFFSET), pkt_info.len);

        /* Reorder on exit */
        if (!signal_test(&ctm_order_sig)) {
            wait_for_all(&ctm_order_sig);
        }
        signal_next_ctx(__signal_number(&ctm_order_sig));
        __implicit_write(&ctm_order_sig);
#endif


        /* Return the packet */
        if (credit != 0) {
            nfd_out_fill_desc(&nfd_out_desc, &pkt_info, 0, 0,
                              NFD_IN_DATA_OFFSET, nfd_in_meta.offset);

            nfd_out_dummy_vlan(&nfd_out_desc, nfd_in_meta.vlan,
                               nfd_in_meta.flags);

            __nfd_out_send(PCIE_ISL, bmsk_queue, nfd_out_desc_xfer,
                           &nfd_out_desc, sig_done, &send_sig);
            wait_for_all(&send_sig, &send_order_sig);
            ret = nfd_out_send_test(nfd_out_desc_xfer);
            if (ret >= 0) {
                nsent++;
            } else {
                nfail++;
            }
        } else {
            /* Drop the packet and participate in ordering */
            drop_pkt(&pkt_info, bmsk_queue);
            wait_for_all(&send_order_sig);
        }
        signal_next_ctx(__signal_number(&send_order_sig));
        __implicit_write(&send_order_sig);

        local_csr_write(NFP_MECSR_MAILBOX_2, (nsent >> 32) & 0xffffffff);
        local_csr_write(NFP_MECSR_MAILBOX_3, nsent & 0xffffffff);
        /* local_csr_write(NFP_MECSR_MAILBOX_2, nfail); */
    }
}
