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
#include <nfp6000/nfp_pcie.h>
#include <nfp6000/nfp_qc.h>

#include <vnic/shared/vnic_cfg.h>

#include <vnic/shared/qc.h>
#include <vnic/utils/mem_bulk32.h>
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

static unsigned int cfg_ring_enables[2] = {0, 0};
__xread unsigned int cfg_ring_addr[2] = {0, 0};
/* cfg_ring_sizes is treated as a char[4] array
 * but it is managed by hand because:
 * (1) the desired ordering of entries is opposite to NFCC ordering, and
 * (2) char[] arrays with non-const index trigger the use of the
 * T-INDEX in NFCC, which is unnecessary in this case. */
__xread unsigned int  cfg_ring_sizes = 0;

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


/* Providing a separate test to manage cfg_ring_enables (declared as an array)
 * gives us more control over the code generated than trying to use a 64bit
 * type and regular c instructions. */
__intrinsic int
_ring_enables_test(struct vnic_cfg_msg *cfg_msg)
{
    /* XXX handle a few special cases efficiently: <= 32 queues, and 1 queue */
    /* 32 bits per unsigned int */
    unsigned int mod_queue = cfg_msg->queue & (32-1);
    int ret;

    /* Test which unsigned int bitmask the queue is stored in,
     * then do a test on that bitmask. */
    if (cfg_msg->queue & 32) {
        ret = (1 & (cfg_ring_enables[1] >> mod_queue));
    } else {
        ret = (1 & (cfg_ring_enables[0] >> mod_queue));
    }

    return ret;
}


/*
 * @param cfg_msg   vnic_cfg_msg to extract queue from
 *
 * cfg_ring_sizes holds size data for 4 rings, starting from base_q
 * where base_q % 4 = 0.
 * The data is stored as [base_q + 3, base_q + 2, base_q + 1, base_q].
 * This method extracts data from the correct offset within the 32bit
 * word.  The correct 32bit word must have been read previously. */
__intrinsic unsigned char
_get_ring_sz(struct vnic_cfg_msg *cfg_msg)
{
    unsigned char ring_sz;
    unsigned int offset = ((cfg_msg->queue & 3) * 8);

    ring_sz = (unsigned char) (cfg_ring_sizes >> offset);
    return ring_sz;
}


/* XXX TEMP compute BAR address fields for PF */
__intrinsic void
_bar_addr(struct nfp_pcie_barcfg_p2c *bar, unsigned long long data)
{
    unsigned int addr_tmp = (data >> 19) & 0x1FFFFF;
    bar->actaddr = addr_tmp >> 16;
    bar->base = addr_tmp; /* XXX & 0xFFFF in this line causes error! */
}

/* XXX TEMP configure wide PF BARs to access config mem and QC  */
__intrinsic void
vnic_cfg_setup_pf()
{
    __gpr unsigned int addr_hi =  PCIE_ISL << 30;
    unsigned int bar_base_addr;
    struct nfp_pcie_barcfg_p2c bar_tmp;
    __xwrite struct nfp_pcie_barcfg_p2c bar;
    SIGNAL sig;

    /* BAR0 (resource0) config mem */
    bar_tmp.map_type = NFP_PCIE_BARCFG_P2C_MAP_TYPE_BULK;
    bar_tmp.len = NFP_PCIE_BARCFG_P2C_LEN_64BIT;
    bar_tmp.target = 7; /* MU CPP target */
    bar_tmp.token = 0;
    _bar_addr(&bar_tmp, (unsigned long long) VNIC_CFG_BASE(PCIE_ISL));
    bar = bar_tmp;

    bar_base_addr = NFP_PCIE_BARCFG_P2C(0, 0);
    __asm pcie[write_pci, bar, addr_hi, <<8, bar_base_addr, 1],    \
        ctx_swap[sig];

    /* BAR1 (resource2) PCI.IN queues  */
    bar_tmp.len = NFP_PCIE_BARCFG_P2C_LEN_32BIT;
    bar_tmp.target = 0; /* Internal PCIe Target */
    _bar_addr(&bar_tmp, 0x80000);
    bar = bar_tmp;

    bar_base_addr = NFP_PCIE_BARCFG_P2C(1, 0);
    __asm pcie[write_pci, bar, addr_hi, <<8, bar_base_addr, 1],    \
        ctx_swap[sig];

    /* BAR2 (resource4) PCI.OUT queues */
    _bar_addr(&bar_tmp, 0x80000 + 128 * 0x800);
    bar = bar_tmp;

    bar_base_addr = NFP_PCIE_BARCFG_P2C(2, 0);
    __asm pcie[write_pci, bar, addr_hi, <<8, bar_base_addr, 1],    \
        ctx_swap[sig];
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
                   2 * MAX_VNIC_QUEUES, MAX_VNICS);

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

    /* Clear the internal state fields and set msg_valid before sending  */
    cfg_msg_tmp.__raw = 0;
    cfg_msg_tmp.msg_valid = 1;
    cfg_msg_tmp.error = cfg_msg->error;
    cfg_msg_tmp.vnic = cfg_msg->vnic;
    cfg_msg_wr.__raw = cfg_msg_tmp.__raw;

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

        /* XXX move out of this method */
        vnic_cfg_app_complete_cfg_msg(&cfg_msg);

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
vnic_cfg_app_complete_cfg_msg(__xread struct vnic_cfg_msg *cfg_msg)
{
    __xwrite unsigned int result;

    if (cfg_msg->error) {
        result = NS_VNIC_CFG_UPDATE_ERR;
    } else {
        /* XXX add NS_VNIC_CFG_UPDATE_SUCCESS value to ns_vnic_ctrl.h */
        result = 0;
    }

    mem_write32(&result,
               VNIC_CFG_BASE(PCIE_ISL)[cfg_msg->vnic] + NS_VNIC_CFG_UPDATE,
               sizeof(result));
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

    /* Clear the internal state fields and set msg_valid before sending  */
    cfg_msg_tmp.__raw = 0;
    cfg_msg_tmp.msg_valid = 1;
    cfg_msg_tmp.error = cfg_msg->error;
    cfg_msg_tmp.vnic = cfg_msg->vnic;
    cfg_msg_wr.__raw = cfg_msg_tmp.__raw;

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

__intrinsic void
vnic_cfg_parse_msg(struct vnic_cfg_msg *cfg_msg, enum vnic_cfg_component comp)
{
    /* XXX rewrite to set a signal mask so that we can leave
     * address and size access to complete. */

    __xread unsigned int cfg_bar_data[6];

    ctassert(__is_ct_const(comp));

    /* XXX do we want to use this method for the APP master as well? */
    if (comp == VNIC_CFG_PCI_OUT) {
        /* Need RXRS_ENABLES, at 0x10 */
        mem_read64(cfg_bar_data,
                 VNIC_CFG_BASE(PCIE_ISL)[cfg_msg->vnic] + NS_VNIC_CFG_CTRL,
                 6 * sizeof(unsigned int));
    } else {
        /* Only need TXRS_ENABLES, at 0x08 */
        mem_read64(cfg_bar_data,
                 VNIC_CFG_BASE(PCIE_ISL)[cfg_msg->vnic] + NS_VNIC_CFG_CTRL,
                 4 * sizeof(unsigned int));
    }

    /* Check capabilities */
    if (cfg_bar_data[NS_VNIC_CFG_CTRL] & ~VNIC_CFG_CAP) {
        /* Mark an error and abort processing */
        cfg_msg->error = 1;
        return;
    }

    /* Check if change affects this component */
    if (comp == VNIC_CFG_PCI_IN || comp == VNIC_CFG_PCI_OUT) {
        /* Only interested in the change if it contains a general update */
        if (cfg_bar_data[NS_VNIC_CFG_UPDATE >> 2] & NS_VNIC_CFG_UPDATE_GEN) {
            cfg_msg->interested = 1;
        }
    } else {
        /* XXX handle App MEs */
        cfg_msg->interested = 1;
    }

    /* Set the queue to process to zero */
    cfg_msg->queue = 0;

    /* Copy ring configs
     * If the vNIC is not up, set all ring enables to zero */
    if (cfg_bar_data[NS_VNIC_CFG_CTRL] & NS_VNIC_CFG_CTRL_ENABLE) {
        unsigned int enables_ind, addr_off, sz_off;

        if (comp == VNIC_CFG_PCI_OUT) {
            enables_ind = NS_VNIC_CFG_RXRS_ENABLE >> 2;
            addr_off =    NS_VNIC_CFG_RXR_ADDR(0);
            sz_off =      NS_VNIC_CFG_RXR_SZ(0);
        } else {
            enables_ind = NS_VNIC_CFG_TXRS_ENABLE >> 2;
            addr_off =    NS_VNIC_CFG_TXR_ADDR(0);
            sz_off =      NS_VNIC_CFG_TXR_SZ(0);
        }

        /* Access ring enables */
        cfg_ring_enables[0] = cfg_bar_data[enables_ind];
        cfg_ring_enables[1] = cfg_bar_data[enables_ind + 1];

        /* Cache next ring address and size */
        mem_read64(&cfg_ring_addr,
                   VNIC_CFG_BASE(PCIE_ISL)[cfg_msg->vnic] + addr_off,
                   sizeof(cfg_ring_addr));
        mem_read32(&cfg_ring_sizes,
                   VNIC_CFG_BASE(PCIE_ISL)[cfg_msg->vnic] + sz_off,
                   sizeof(cfg_ring_sizes));
    } else {
        /* All rings are set disabled, we won't need any addresses or sizes */
        cfg_ring_enables[0] = 0;
        cfg_ring_enables[1] = 0;
    }
}

__intrinsic void
vnic_cfg_proc_msg(struct vnic_cfg_msg *cfg_msg, unsigned char *queue,
                  unsigned char *ring_sz, unsigned int ring_base[2],
                  enum vnic_cfg_component comp)
{
    unsigned int next_addr_off, next_sz_off;

    /* XXX rewrite to set a signal mask so that we can leave
     * address and size access to complete. */

    /* Mark packet complete if an error is detected,
     * or if the component is not interested in the change.
     * XXX Journal the error? */
    if (cfg_msg->error || !cfg_msg->interested) {
        cfg_msg->msg_valid = 0;
        return;
    }

    *queue = cfg_msg->queue;

    /* Set up values for current queue */
    if (_ring_enables_test(cfg_msg)) {
        cfg_msg->up_bit = 1;
        ring_base[0] = cfg_ring_addr[0];
        ring_base[1] = cfg_ring_addr[1];
        *ring_sz = _get_ring_sz(cfg_msg);
    } else {
        cfg_msg->up_bit = 0;
    }

    cfg_msg->queue++;
    if (cfg_msg->queue == MAX_VNIC_QUEUES) {
        /* This queue is the last */
        cfg_msg->msg_valid = 0;
        return;
    }

    /* Read values for next queue(s) */
    if (comp == VNIC_CFG_PCI_OUT) {
        next_addr_off = NS_VNIC_CFG_RXR_ADDR(cfg_msg->queue);
        next_sz_off =   NS_VNIC_CFG_RXR_SZ(cfg_msg->queue);
    } else {
        next_addr_off = NS_VNIC_CFG_TXR_ADDR(cfg_msg->queue);
        next_sz_off =   NS_VNIC_CFG_TXR_SZ(cfg_msg->queue);
    }

    /* We only use the address if the ring is enabled.
     * Otherwise suppress read to save CPP bandwidth. */
    if (_ring_enables_test(cfg_msg)) {
        mem_read64(&cfg_ring_addr,
                   VNIC_CFG_BASE(PCIE_ISL)[cfg_msg->vnic] + next_addr_off,
                   sizeof(cfg_ring_addr));
    }

    /* Sizes packed 4 per register, so reread every 4th queue */
    if ((cfg_msg->queue & 3) == 0) {
        mem_read32(&cfg_ring_sizes,
                   VNIC_CFG_BASE(PCIE_ISL)[cfg_msg->vnic] + next_sz_off,
                   sizeof(cfg_ring_sizes));
    }

}
