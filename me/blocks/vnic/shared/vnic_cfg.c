/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/vnic_cfg.c
 * @brief         An API to manage access to NFD configuration data
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_bulk.h>
#include <nfp/mem_ring.h>
#include <std/event.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_qc.h>

#include <vnic/shared/vnic_cfg.h>

#include <vnic/shared/qc.h>
#include <vnic/utils/qcntl.h>

#include <ns_vnic_ctrl.h>

#define VNIC_CFG_BASE_IND(_x) vnic_cfg_pcie##_x##_base
#define VNIC_CFG_BASE(_x) VNIC_CFG_BASE_IND(_x)

/*
 * Compute constants to help map from the configuration bitmask
 * to configuration queues. We need to compute the spacing between
 * bitmasks, and a mask for how many times a bit has been tested.
 *
 * There are a few corner cases to consider (such as the case of
 * two vNICs with 32 queues each), so #if-#elif-#else statements are used.
 */
#if ((MAX_VNICS == 1) || (MAX_VNICS * MAX_VNIC_QUEUES <= 16))
#define VNIC_CFG_BMSK_TEST_MSK  0
#define VNIC_CFG_BMSK_SPACING   0

#elif ((MAX_VNICS == 2) || (MAX_VNICS * MAX_VNIC_QUEUES <= 32))
#define VNIC_CFG_BMSK_TEST_MSK  1

#if MAX_VNIC_QUEUES == 32
#define VNIC_CFG_BMSK_SPACING   64
#else
#define VNIC_CFG_BMSK_SPACING   32
#endif

#else
#define VNIC_CFG_BMSK_TEST_MSK  3
#define VNIC_CFG_BMSK_SPACING   32
#endif

static SIGNAL cfg_ap_sig;
static __xread unsigned int cfg_ap_xfer;
static volatile __gpr unsigned int cfg_vnic_bmsk = 0;
static __gpr unsigned int cfg_vnic_queue_test_cnt = 0;

__visible SIGNAL VNIC_CFG_SIG;

#ifdef VNIC_CFG_SIG_NEXT_ME
__remote SIGNAL VNIC_CFG_SIG_NEXT_ME;
#endif

__export __emem __align(NS_VNIC_CFG_BAR_SZ * MAX_VNICS) char
    VNIC_CFG_BASE(PCIE_ISL)[MAX_VNICS][NS_VNIC_CFG_BAR_SZ];

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

    __asm ct[interthread_signal, --, addr, 0, --];
}

/* XXX move to libnfp.c */
/**
 * Find first set and return -1 if none
 * @param data          Value to test for first set
 *
 * Wraps the find first set instruction that returns the bit position of the
 * least significant bit set in the input 'data'.  The instruction sets
 * condition codes if 'data' is zero, and this method preserves this information
 * by returning '-1' in this case.
 */
__intrinsic int
ffs(unsigned int data)
{
    int ret;

    __asm {
        ffs[ret, data];
        bne[match];
        immed[ret, -1];
    match:
    }

    return ret;
}


void
_vnic_cfg_queue_setup()
{
    struct qc_queue_config vnic_cfg_queue;

    __cls struct event_cls_filter *event_filter;
    struct nfp_em_filter_status status;
    unsigned int pcie_provider = NFP_EVENT_PROVIDER_NUM(
        ((unsigned int) __MEID>>4), NFP_EVENT_PROVIDER_INDEX_PCIE);
    /*
     * Mask set to be fairly permissive now
     * Can make it stricter (based on number of active vNICs) if justified
     */
    unsigned int event_mask = NFP_EVENT_MATCH(0xFF, 0xF81, 0xF);
    unsigned int event_match = NFP_EVENT_MATCH(pcie_provider,
                                               ((VNIC_CFG_EVENT_DATA<<6) |
                                                VNIC_CFG_QUEUE),
                                               0);

    /*
     * Config queues are small and issue events on not empty.
     * The confq for VNIC N is CONFQ_START + N * vnic_block_size.
     * All confqs are monitored by a single bitmask32 filter.
     */
    vnic_cfg_queue.watermark  = PCIE_QC_WM_4;
    vnic_cfg_queue.size       = PCIE_QC_SZ_256;
    vnic_cfg_queue.event_data = VNIC_CFG_EVENT_DATA;
    vnic_cfg_queue.event_type = PCIE_QC_EVENT_NOT_EMPTY;
    vnic_cfg_queue.ptr        = 0;

    init_qc_queues(PCIE_ISL, &vnic_cfg_queue, VNIC_CFG_QUEUE,
                   2 * MAX_VNIC_QUEUES, MAX_VNICS, ctx_swap);

    /* Setup the Event filter and autopush */
    __implicit_write(&cfg_ap_sig);
    __implicit_write(&cfg_ap_xfer);

    status.__raw = 0; /* bitmask32 requires no further settings */
    event_filter = event_cls_filter_handle(VNIC_CFG_EVENT_FILTER);

    event_cls_filter_setup(event_filter,
                           NFP_EM_FILTER_MASK_TYPE_MASK32,
                           event_match, event_mask, status);

    event_cls_autopush_signal_setup(VNIC_CFG_EVENT_FILTER,
                                    (unsigned int) __MEID,
                                    ctx(),
                                    __signal_number(&cfg_ap_sig),
                                    __xfer_reg_number(&cfg_ap_xfer));
    event_cls_autopush_filter_reset(
        VNIC_CFG_EVENT_FILTER,
        NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
        VNIC_CFG_EVENT_FILTER);
}

void
vnic_cfg_write_cap(unsigned int vnic)
{
    __xwrite unsigned int cfg[] = {VNIC_CFG_VERSION, 0, VNIC_CFG_CAP,
                                   MAX_VNIC_QUEUES, MAX_VNIC_QUEUES,
                                   VNIC_CFG_MAX_MTU};

    mem_write64(&cfg, VNIC_CFG_BASE(PCIE_ISL)[vnic] + NS_VNIC_CFG_VERSION,
                sizeof cfg);
}

void
vnic_cfg_setup()
{
    unsigned int vnic;

    /* Setup the configuration message rings */
    MEM_RING_CONFIGURE(VNIC_CFG_RING_ADDR(PCIE_ISL, 0),
                       VNIC_CFG_RING_NUM(PCIE_ISL, 0));
    MEM_RING_CONFIGURE(VNIC_CFG_RING_ADDR(PCIE_ISL, 1),
                       VNIC_CFG_RING_NUM(PCIE_ISL, 1));
    MEM_RING_CONFIGURE(VNIC_CFG_RING_ADDR(PCIE_ISL, 2),
                       VNIC_CFG_RING_NUM(PCIE_ISL, 2));

    /* Setup the configuration queues */
    _vnic_cfg_queue_setup();

    /*
     * Write compile time configured MAX_VNIC_QUEUES to mem.
     * XXX Could be .init'ed?
     */

    for (vnic = 0; vnic < MAX_VNICS; vnic++) {
        vnic_cfg_write_cap(vnic);
    }
}

__intrinsic void
vnic_cfg_init_cfg_msg(SIGNAL *cfg_sig, struct vnic_cfg_msg *cfg_msg)
{
    __implicit_write(cfg_sig);

    cfg_msg->__raw = 0;
}

void
vnic_cfg_check_cfg_ap()
{
    if (signal_test(&cfg_ap_sig)) {
        /* Set the active bitmask */
        cfg_vnic_bmsk = cfg_ap_xfer;

        /* Mark the autopush signal and xfer as used
         * so that the compiler keeps them reserved. */
        __implicit_write(&cfg_ap_sig);
        __implicit_write(&cfg_ap_xfer);
    }
}

int
vnic_cfg_next_vnic()
{
    int curr_bit;
    int queue;
    int vnic;
    int masked_test_cnt;

    __xread struct nfp_qc_sts_lo cfg_queue_sts;
    SIGNAL sig;

    curr_bit = ffs(cfg_vnic_bmsk);

    /* cfg_vnic_bmsk empty, fast path short circuits any context swaps */
    if (curr_bit < 0) {
        return -1;
    }

    /* Compute the queue to test.
     * A free running count of how many queues have been tested
     * This serves a double function of debugging info and
     * serving as an offset to compute the queue.
     * When masked_test_cnt == 0, we have checked every queue associated
     * with the bitmask. */
    cfg_vnic_queue_test_cnt++;
    masked_test_cnt = cfg_vnic_queue_test_cnt & VNIC_CFG_BMSK_TEST_MSK;
    queue = curr_bit | (masked_test_cnt * VNIC_CFG_BMSK_SPACING);

    /* Compute vNIC and increment QC read pointer.
     * If there is not a one-to-one mapping between queues and bits,
     * first test whether the queue is empty. */
#if VNIC_CFG_BMSK_TEST_MSK == 0
    vnic = queue / (2 * MAX_VNIC_QUEUES);
    qc_add_to_ptr(PCIE_ISL, queue, QC_RPTR, 1);
#else
    cfg_queue_sts.__raw = __qc_read(PCIE_ISL, queue, QC_RPTR,
                                    ctx_swap, &sig);
    if (cfg_queue_sts.empty) {
        /* We haven't found a vNIC to service this time */
        vnic = -1;
    } else {
        vnic = queue / (2 * MAX_VNIC_QUEUES);
        qc_add_to_ptr(PCIE_ISL, queue, QC_RPTR, 1);
    }
#endif

    /* Test whether to reset curr_bit */
    if (masked_test_cnt == 0) {
        /* We just checked the last queue associated with curr_bit;
         * reset it. */
        cfg_vnic_bmsk &= ~(1 << curr_bit);
    }

    /* Test the bitmask to determine whether to reset the autopush. */
    if (cfg_vnic_bmsk == 0) {
        event_cls_autopush_filter_reset(
            VNIC_CFG_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            VNIC_CFG_EVENT_FILTER);
    }

    return vnic;
}

__intrinsic void
vnic_cfg_start_cfg_msg(struct vnic_cfg_msg *cfg_msg,
                       __remote SIGNAL *cfg_sig_remote,
                       unsigned int next_me, unsigned int rnum,
                       __dram void *rbase)
{
    struct vnic_cfg_msg cfg_msg_tmp;
    __xrw struct vnic_cfg_msg cfg_msg_wr;

    /* Copy cfg_msg to write transfer register with "msg_valid" set
     * XXX check that this results in an
     * "alu[xfer, msg, or, 1, <<n]" type instruction */
    cfg_msg_tmp.__raw = 0;
    cfg_msg_tmp.msg_valid = 1;
    cfg_msg_wr.__raw = cfg_msg->__raw | cfg_msg_tmp.__raw;

    mem_ring_put(rnum, mem_ring_get_addr(rbase), &cfg_msg_wr,
                 sizeof cfg_msg_wr);

    send_interthread_sig(next_me, 0,
                         __signal_number(cfg_sig_remote, next_me));
}

__intrinsic void
vnic_cfg_app_check_cfg_msg(SIGNAL *cfg_sig, unsigned int rnum,
                           __dram void *rbase)
{
    if (signal_test(cfg_sig)) {
        __xread struct vnic_cfg_msg cfg_msg;

        __implicit_write(cfg_sig);

        mem_ring_get(rnum, mem_ring_get_addr(rbase), &cfg_msg,
                     sizeof cfg_msg);

        local_csr_write(NFP_MECSR_MAILBOX_0, cfg_msg.vnic);
        local_csr_write(NFP_MECSR_MAILBOX_1, 0x11223344);
    }
}

__intrinsic void
vnic_cfg_check_cfg_msg(struct vnic_cfg_msg *cfg_msg, SIGNAL *cfg_sig,
                       unsigned int rnum, __dram void *rbase)
{
    /* XXX should this method read the vnic config BAR? */
    if (signal_test(cfg_sig)) {
        int ret;
        __xread struct vnic_cfg_msg cfg_msg_rd;

        __implicit_write(cfg_sig);

        ret = mem_ring_get(rnum, mem_ring_get_addr(rbase), &cfg_msg_rd,
                           sizeof cfg_msg_rd);

        if (ret == 0) {
            *cfg_msg = cfg_msg_rd;
        }
    }
}

__intrinsic void
vnic_cfg_complete_cfg_msg(struct vnic_cfg_msg *cfg_msg,
                          __remote SIGNAL *cfg_sig_remote,
                          unsigned int next_me,
                          unsigned int rnum_out, __dram void *rbase_out,
                          unsigned int rnum_in, __dram void *rbase_in)
{
    struct vnic_cfg_msg cfg_msg_tmp;
    __xrw struct vnic_cfg_msg cfg_msg_wr;
    __xread struct vnic_cfg_msg cfg_msg_rd;
    SIGNAL_PAIR put_sig;
    SIGNAL_PAIR get_sig;

    /* Copy cfg_msg to write transfer register with "msg_valid" set
     * XXX check that this results in an
     * "alu[xfer, msg, or, 1, <<n]" type instruction */
    cfg_msg_tmp.__raw = 0;
    cfg_msg_tmp.msg_valid = 1;
    cfg_msg_wr.__raw = cfg_msg->__raw | cfg_msg_tmp.__raw;

    /* Put is guaranteed to succeed by design (the ring larger than
     * the number of possible vNICs). */
    __mem_ring_put(rnum_out, mem_ring_get_addr(rbase_out), &cfg_msg_wr,
                   sizeof cfg_msg_wr, sizeof cfg_msg_wr, sig_done, &put_sig);
    __mem_ring_get(rnum_in, mem_ring_get_addr(rbase_in), &cfg_msg_rd,
                   sizeof cfg_msg_rd, sizeof cfg_msg_rd, sig_done, &get_sig);

    wait_for_all_single(&put_sig.even, &put_sig.odd, &get_sig.even);

    /* XXX don't check put return, assume put succeeded. */

    if (!signal_test(&get_sig.odd)) {
        *cfg_msg = cfg_msg_rd;
    }

    send_interthread_sig(next_me, 0,
                         __signal_number(cfg_sig_remote, next_me));
}
