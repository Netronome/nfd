/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_cfg_internal.c
 * @brief         An internal API to access to NFD configuration data
 */

#include <assert.h>
#include <nfp.h>


#include <nfp/me.h>
#include <nfp/mem_atomic.h>
#include <nfp/mem_bulk.h>
#include <nfp/mem_ring.h>
#include <nfp/xpb.h>
#include <std/event.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>
#include <nfp6000/nfp_qc.h>

#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/qc.h>
#include <vnic/utils/qcntl.h>

#include <vnic/shared/nfd_flr.c>

#include <nfp_net_ctrl.h>


#define NFD_CFG_DECLARE(_sig, _next_sig)  \
    __visible SIGNAL _sig;                \
    __remote SIGNAL _next_sig;


#define NFD_CFG_NEXT_ME_IND1(_me_str) __nfp_idstr2meid(#_me_str)
#define NFD_CFG_NEXT_ME_IND0(_isl, _me)         \
    NFD_CFG_NEXT_ME_IND1(pcie##_isl##.me##_me)

#ifndef NFD_CFG_NEXT_ME
#define NFD_CFG_NEXT_ME(_isl, _me) NFD_CFG_NEXT_ME_IND0(_isl, _me)
#endif


ASM(.alloc_mem nfd_cfg_ring_mem NFD_CFG_RING_EMEM global \
    (NFD_MAX_ISL * NFD_CFG_NUM_RINGS * NFD_CFG_RING_SZ)  \
    (NFD_MAX_ISL * NFD_CFG_NUM_RINGS * NFD_CFG_RING_SZ))

#define NFD_CFG_RINGS_INIT_IND(_isl)                                    \
    ASM(.declare_resource nfd_cfg_ring_mem##_isl global 8192 nfd_cfg_ring_mem) \
    ASM(.alloc_resource nfd_cfg_ring_mem##_isl##0 nfd_cfg_ring_mem##_isl \
        global 2048 2048)                                               \
    ASM(.alloc_resource nfd_cfg_ring_mem##_isl##1 nfd_cfg_ring_mem##_isl \
        global 2048 2048)                                               \
    ASM(.alloc_resource nfd_cfg_ring_mem##_isl##2 nfd_cfg_ring_mem##_isl \
        global 2048 2048)                                               \
    ASM(.alloc_resource nfd_cfg_ring_mem##_isl##3 nfd_cfg_ring_mem##_isl \
        global 2048 2048)                                               \
                                                                        \
    ASM(.init_mu_ring nfd_cfg_ring_num##_isl##0 nfd_cfg_ring_mem##_isl##0) \
    ASM(.init_mu_ring nfd_cfg_ring_num##_isl##1 nfd_cfg_ring_mem##_isl##1) \
    ASM(.init_mu_ring nfd_cfg_ring_num##_isl##2 nfd_cfg_ring_mem##_isl##2) \
    ASM(.init_mu_ring nfd_cfg_ring_num##_isl##3 nfd_cfg_ring_mem##_isl##3)

#define NFD_CFG_RINGS_INIT(_isl) NFD_CFG_RINGS_INIT_IND(_isl)

#if _nfp_has_island("pcie0")
NFD_CFG_RINGS_INIT(0);
#endif


#if _nfp_has_island("pcie1")
NFD_CFG_RINGS_INIT(1);
#endif

#if _nfp_has_island("pcie2")
NFD_CFG_RINGS_INIT(2);
#endif


#if _nfp_has_island("pcie3")
NFD_CFG_RINGS_INIT(3);
#endif

#if NFD_MAX_PF_QUEUES != 0
#define NFD_CFG_PF_DECLARE_IND(_isl)                                \
    NFD_CFG_BASE_DECLARE(_isl)                                      \
    ASM(.declare_resource nfd_cfg_base##_isl##_res global           \
        ((NFD_MAX_VFS + NFD_MAX_PFS) * NFP_NET_CFG_BAR_SZ)          \
        nfd_cfg_base##_isl)                                         \
    ASM(.alloc_resource _pf##_isl##_net_bar0                        \
        nfd_cfg_base##_isl##_res+(NFD_MAX_VFS * NFP_NET_CFG_BAR_SZ) \
        global NFP_NET_CFG_BAR_SZ)
#else
#define NFD_CFG_PF_DECLARE_IND(_isl)
#endif

#define NFD_CFG_PF_DECLARE(_isl) NFD_CFG_PF_DECLARE_IND(_isl)


/* /\* XXX temp defines that loosely match the BSP pcie_monitor_api.h *\/ */
/* #define NFP_PCIEX_COMPCFG_CNTRLR3                            0x0010006c */
/* #define NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_CHANNEL_msk    0x3f */
/* #define NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_CHANNEL_shf    16 */
/* #define NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_shf            15 */
/* #define NFP_PCIEX_COMPCFG_CNTRLR3_FLR_DONE_shf               14 */
/* #define NFP_PCIEX_COMPCFG_CNTRLR3_FLR_IN_PROGRESS_shf        13 */
/* #define NFP_PCIEX_COMPCFG_PCIE_VF_FLR_IN_PROGRESS0           0x00100080 */
/* #define NFP_PCIEX_COMPCFG_PCIE_VF_FLR_IN_PROGRESS1           0x00100084 */
/* #define NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT             0x001000cc */
/* #define NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT_msk         0x3f */


NFD_FLR_DECLARE;


__shared __gpr struct qc_bitmask cfg_queue_bmsk;

static SIGNAL flr_ap_sig;
static __xread unsigned int flr_ap_xfer;
static __shared __gpr unsigned int flr_in_prog_vf[2] = {0, 0};

#define FLR_IN_PROG_BUSY_shf 31
static __shared __gpr unsigned int flr_in_prog_status = 0;


#ifdef NFD_CFG_SIG_NEXT_ME
__remote SIGNAL NFD_CFG_SIG_NEXT_ME;
#endif

NFD_CFG_BASE_DECLARE(PCIE_ISL);
/* XXX add the equivalent of nfd_cfg_pf_bars.uc in microC here. */

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
_ffs(unsigned int data)
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
_ring_enables_test(struct nfd_cfg_msg *cfg_msg)
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
 * @param cfg_msg   nfd_cfg_msg to extract queue from
 *
 * cfg_ring_sizes holds size data for 4 rings, starting from base_q
 * where base_q % 4 = 0.
 * The data is stored as [base_q + 3, base_q + 2, base_q + 1, base_q].
 * This method extracts data from the correct offset within the 32bit
 * word.  The correct 32bit word must have been read previously. */
__intrinsic unsigned char
_get_ring_sz(struct nfd_cfg_msg *cfg_msg)
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


/**
 * XXX formalise setup_pf and setup_vf methods
 * Configure the PF for use with NFD
 */
/* XXX TEMP configure wide PF BARs to access config mem and QC
 * NB not used with BSP drivers on PF */
__intrinsic void
nfd_cfg_setup_pf()
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
    _bar_addr(&bar_tmp, NFD_CFG_BASE_LINK(PCIE_ISL));
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


__intrinsic void
nfd_cfg_setup_vf()
{
    __gpr unsigned int addr_hi =  PCIE_ISL << 30;
    unsigned int bar_base_addr;
    struct nfp_pcie_barcfg_vf_p2c bar_tmp;
    __xwrite struct nfp_pcie_barcfg_vf_p2c bar;
    SIGNAL sig;

    /* Clean start state */
    bar_tmp.__raw = 0;

    /* BAR0 (resource0) config mem */
    bar_tmp.len = NFP_PCIE_BARCFG_VF_P2C_LEN_64BIT;
    bar_tmp.target = 7; /* MU CPP target */
    bar_tmp.token = 0;
    /* XXX this is A0 specific */
    bar_tmp.base =
        ((unsigned long long) NFD_CFG_BASE_LINK(PCIE_ISL)) >> (40 - 19);
    bar = bar_tmp;

    bar_base_addr = NFP_PCIE_BARCFG_VF_P2C(0);
    __asm pcie[write_pci, bar, addr_hi, <<8, bar_base_addr, 1],    \
        ctx_swap[sig];

    /* BAR1 (resource2) PCI.IN queues  */
    bar_tmp.len = NFP_PCIE_BARCFG_VF_P2C_LEN_32BIT;
    bar_tmp.target = 0; /* Internal PCIe Target */
    /* Both the A0 workaround and the B0 enhancement require base = 0
     * to access the QC. */
    bar_tmp.base = 0;
    bar = bar_tmp;

    bar_base_addr = NFP_PCIE_BARCFG_VF_P2C(1);
    __asm pcie[write_pci, bar, addr_hi, <<8, bar_base_addr, 1],    \
        ctx_swap[sig];
}


void
_nfd_cfg_queue_setup()
{
    struct qc_queue_config nfd_cfg_queue;

    /*
     * Config queues are small and issue events on not empty.
     * The confq for VNIC N is CONFQ_START + N * vnic_block_size.
     * All confqs are monitored by a single bitmask32 filter.
     */
    nfd_cfg_queue.watermark  = PCIE_QC_WM_4;
    nfd_cfg_queue.size       = PCIE_QC_SZ_256;
    nfd_cfg_queue.event_data = NFD_EVENT_DATA;
    nfd_cfg_queue.event_type = PCIE_QC_EVENT_NOT_EMPTY;
    nfd_cfg_queue.ptr        = 0;

#if NFD_MAX_VF_QUEUES != 0
    /* Init the VF config queues and possibly end with PF queue.
     * Each NFD queue uses a block of 4 QC queues. */
    init_qc_queues(PCIE_ISL, &nfd_cfg_queue, NFD_CFG_QUEUE,
                   4 * NFD_MAX_VF_QUEUES, NFD_MAX_VFS + NFD_MAX_PFS);
#else
    /* Just the PF to init */
    qc_init_queue(PCIE_ISL, NFD_CFG_QUEUE, &nfd_cfg_queue);
#endif
}

/*
 * Initialise the VF and PF control BAR
 */
static void
_nfd_cfg_init_vf_ctrl_bar(unsigned int vnic)
{
#if ((NFD_MAX_VFS != 0) && (NFD_MAX_VF_QUEUES != 0))
#ifdef NFD_NO_ISOLATION
    unsigned int q_base = NFD_MAX_VF_QUEUES * vnic;
#else
    unsigned int q_base = 0;
#endif
    __xwrite unsigned int cfg[] = {NFD_CFG_VERSION, 0, NFD_CFG_VF_CAP,
                                   NFD_MAX_VF_QUEUES, NFD_MAX_VF_QUEUES,
                                   NFD_CFG_MAX_MTU,
                                   NFD_NATQ2QC(q_base, NFD_IN_TX_QUEUE),
                                   NFD_NATQ2QC(q_base, NFD_OUT_FL_QUEUE)};
    __xwrite unsigned int exn_lsc = 0xffffffff;

    mem_write64(&cfg, NFD_CFG_BAR_ISL(PCIE_ISL, vnic) + NFP_NET_CFG_VERSION,
                sizeof cfg);

    mem_write8(&exn_lsc, NFD_CFG_BAR_ISL(PCIE_ISL, vnic) + NFP_NET_CFG_EXN,
               sizeof exn_lsc);
#endif
}


static void
_nfd_cfg_init_pf_ctrl_bar()
{
#if (NFD_MAX_PF_QUEUES != 0)
    unsigned int q_base = NFD_MAX_VF_QUEUES * NFD_MAX_VFS;
    __xwrite unsigned int cfg[] = {NFD_CFG_VERSION, 0, NFD_CFG_PF_CAP,
                                   NFD_MAX_PF_QUEUES, NFD_MAX_PF_QUEUES,
                                   NFD_CFG_MAX_MTU,
                                   NFD_NATQ2QC(q_base, NFD_IN_TX_QUEUE),
                                   NFD_NATQ2QC(q_base, NFD_OUT_FL_QUEUE)};
    __xwrite unsigned int exn_lsc = 0xffffffff;

    mem_write64(&cfg,
                (NFD_CFG_BAR_ISL(PCIE_ISL, NFD_MAX_VFS) +
                 NFP_NET_CFG_VERSION),
                sizeof cfg);

    mem_write8(&exn_lsc,
               NFD_CFG_BAR_ISL(PCIE_ISL, NFD_MAX_VFS) + NFP_NET_CFG_LSC,
               sizeof exn_lsc);
#endif
}


/**
 * Perform per PCIe island nfd_cfg initialisation
 */
void
nfd_cfg_setup()
{
    unsigned int vnic;
    unsigned int ring;

    /* XXX remove once .declare_resource and .alloc_resource support
     * bracketed expressions. */
    ctassert(NFD_CFG_NUM_RINGS == 4);
    ctassert(NFD_CFG_RING_SZ == 2048);

    /* Setup the configuration queues */
    _nfd_cfg_queue_setup();

    /*
     * Write compile time configured NFD_MAX_VF_QUEUES and
     * NFD_MAX_PF_QUEUES to mem.
     * XXX Could be .init'ed?
     */
#if NFD_MAX_VFS != 0
    for (vnic = 0; vnic < NFD_MAX_VFS; vnic++) {
        _nfd_cfg_init_vf_ctrl_bar(vnic);
    }
#endif

#if NFD_MAX_PF_QUEUES != 0
    _nfd_cfg_init_pf_ctrl_bar();
#endif
}



/**
 * Perform per PCIe island FLR event filter and autopush initialisation
 */
void
nfd_cfg_flr_setup()
{
    __cls struct event_cls_filter *event_filter;
    struct nfp_em_filter_status status;
    unsigned int pcie_provider = NFP_EVENT_PROVIDER_NUM(
        ((unsigned int) __MEID>>4), NFP_EVENT_PROVIDER_INDEX_PCIE);
    unsigned int event_mask, event_match;

    __implicit_write(&flr_ap_sig);
    __implicit_write(&flr_ap_xfer);

    event_mask = NFP_EVENT_MATCH(0xFF, 0x000, 0xF);
    event_match = NFP_EVENT_MATCH(pcie_provider, 0,
                                  NFP_EVENT_TYPE_STATUS_CHANGED);
    status.__raw = 0; /* No throttling used */

    event_filter = event_cls_filter_handle(NFD_CFG_FLR_EVENT_FILTER);

    event_cls_filter_setup(event_filter, NFP_EM_FILTER_MASK_TYPE_FIRSTEV,
                           event_match, event_mask, status);

    event_cls_autopush_signal_setup(NFD_CFG_FLR_EVENT_FILTER,
                                    (unsigned int) __MEID,
                                    ctx(),
                                    __signal_number(&flr_ap_sig),
                                    __xfer_reg_number(&flr_ap_xfer));

    event_cls_autopush_filter_reset(
        NFD_CFG_FLR_EVENT_FILTER,
        NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
        NFD_CFG_FLR_EVENT_FILTER);
}


/**
 * Update the FLR state in this ME
 *
 * The PF and VF FLR state are checked separately, using "nfd_flr_sent" to
 * determine whether we are already processing an FLR for the vNIC.  PCIe state
 * changes are associated with FLRs (both on start and completion), so we also
 * monitor and acknowledge PCIe state changes.  This ensures that we will be
 * aware of VF FLRs that occur, even if we are already processing an FLR.
 */
void
nfd_cfg_check_flr_ap()
{
    if (signal_test(&flr_ap_sig)) {
        __xread unsigned int flr_sent[3];
        unsigned int flr_status;
        int vf;

        local_csr_write(local_csr_mailbox_0, flr_ap_xfer);
        flr_status = xpb_read(NFP_PCIEX_COMPCFG_CNTRLR3);

        local_csr_write(local_csr_mailbox_1, flr_status);


        /* Read which VF/PF sent messages are still pending */
        nfd_flr_read_sent(PCIE_ISL, flr_sent);

        /* Handle the PF */
        if (flr_status & (1 << NFP_PCIEX_COMPCFG_CNTRLR3_FLR_IN_PROGRESS_shf)) {
            if (~flr_sent[NFD_FLR_PF_ind] &
                (1 << NFD_FLR_PF_shf)) {
                flr_in_prog_status |= (1 << NFD_FLR_PF_ind);
                flr_in_prog_status |= (1 << FLR_IN_PROG_BUSY_shf);
            }
        }

        /* Handle VFs 0 to 31 */
        flr_in_prog_vf[NFD_FLR_VF_LO_ind] =
            (xpb_read(NFP_PCIEX_COMPCFG_PCIE_VF_FLR_IN_PROGRESS0) &
             ~flr_sent[NFD_FLR_VF_LO_ind]);
        if (flr_in_prog_vf[NFD_FLR_VF_LO_ind] != 0) {
            flr_in_prog_status |= (1 << NFD_FLR_VF_LO_ind);
            flr_in_prog_status |= (1 << FLR_IN_PROG_BUSY_shf);
        }

        /* Handle VFs 32 to 63 */
        flr_in_prog_vf[NFD_FLR_VF_HI_ind] =
            (xpb_read(NFP_PCIEX_COMPCFG_PCIE_VF_FLR_IN_PROGRESS1) &
             ~flr_sent[NFD_FLR_VF_HI_ind]);
        if (flr_in_prog_vf[NFD_FLR_VF_HI_ind] != 0) {
            flr_in_prog_status |= (1 << NFD_FLR_VF_HI_ind);
            flr_in_prog_status |= (1 << FLR_IN_PROG_BUSY_shf);
        }

        /* Acknowledge PCIe state changes */
        flr_status = xpb_read(NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT);
        if (flr_status & NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT_msk) {
            flr_status &= NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT_msk;
            xpb_write(NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT, flr_status);
        }

        /* Mark the autopush signal and xfer as used
         * so that the compiler keeps them reserved. */
        __implicit_write(&flr_ap_sig);
        __implicit_write(&flr_ap_xfer);

        event_cls_autopush_filter_reset(
            NFD_CFG_FLR_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            NFD_CFG_FLR_EVENT_FILTER);
    }
}


/**
 * Find next vNIC to process configuration messages from. Negative return
 * values show no vNIC found.
 */
int
nfd_cfg_next_vnic()
{
    /* XXX throttle how often this function runs? */
    __gpr unsigned int queue;
    int vnic;
    int ret;

    /* Get a bmsk queue number from the configuration queue bmsk */
    ret = select_queue(&queue, &cfg_queue_bmsk);
    if (ret) {
        /* We haven't found a configuration queue to service */
        vnic = -1;
    } else {
        /* We have a configuration queue to service */

        /* Compute the vNIC associated with that queue */
        vnic = NFD_CFGQ2VNIC(queue);

        /* Clear the bit in the bitmask so the queue isn't picked again,
         * and increment the QC queue read pointer */
        clear_queue(&queue, &cfg_queue_bmsk);
        qc_add_to_ptr(PCIE_ISL, NFD_NATQ2QC(queue, NFD_CFG_QUEUE), QC_RPTR, 1);
    }

    return vnic;
}


/**
 * Add a cfg_msg to the start of the ring pipeline
 * @param cfg_msg           message to add
 * @param cfg_sig_remote    remote signal to set on success
 * @param next_me           ME to signal on success
 * @param rnum              ring number to use for the ring journal
 * @param rbase             base address of the ring to use
 */
__intrinsic void
nfd_cfg_start_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                       __remote SIGNAL *cfg_sig_remote,
                       unsigned int next_me, unsigned int rnum)
{
    struct nfd_cfg_msg cfg_msg_tmp;
    __xwrite struct nfd_cfg_msg cfg_msg_wr;
    mem_ring_addr_t ring_addr = (unsigned long long) NFD_CFG_EMEM >> 8;

    /* Clear the internal state fields and set msg_valid before sending  */
    cfg_msg_tmp.__raw = 0;
    cfg_msg_tmp.msg_valid = 1;
    cfg_msg_tmp.error = cfg_msg->error;
    cfg_msg_tmp.vnic = cfg_msg->vnic;
    cfg_msg_wr.__raw = cfg_msg_tmp.__raw;

    mem_ring_journal(rnum, ring_addr, &cfg_msg_wr, sizeof cfg_msg_wr);

    send_interthread_sig(next_me, 0,
                         __signal_number(cfg_sig_remote, next_me));
}


/**
 * Pass this message to the next stage, and check for a new message
 * @param cfg_msg           completed message, refilled with new message
 * @param cfg_sig_remote    signal to set for next recipient
 * @param next_me           ME to signal
 * @param rnum_out          output ring number
 * @param rbase_out         output ring address
 * @param rnum_in           input ring number
 * @param rbase_in          input ring address
 */
__intrinsic void
nfd_cfg_complete_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                          __remote SIGNAL *cfg_sig_remote,
                          unsigned int next_me, unsigned int rnum_out,
                          unsigned int rnum_in)
{
    struct nfd_cfg_msg cfg_msg_tmp;
    __xrw struct nfd_cfg_msg cfg_msg_wr;
    __xread struct nfd_cfg_msg cfg_msg_rd;
    mem_ring_addr_t ring_addr = (unsigned long long) NFD_CFG_EMEM >> 8;
    SIGNAL journal_sig;
    SIGNAL_PAIR get_sig;

    /* Clear the internal state fields and set msg_valid before sending  */
    cfg_msg_tmp.__raw = 0;
    cfg_msg_tmp.msg_valid = 1;
    cfg_msg_tmp.error = cfg_msg->error;
    cfg_msg_tmp.vnic = cfg_msg->vnic;
    cfg_msg_wr.__raw = cfg_msg_tmp.__raw;

    /* Journal is guaranteed to not overflow by design (it is larger than
     * the number of possible vNICs). */
    __mem_ring_journal(rnum_out, ring_addr, &cfg_msg_wr, sizeof cfg_msg_wr,
                       sizeof cfg_msg_wr, sig_done, &journal_sig);
    __mem_ring_get(rnum_in, ring_addr, &cfg_msg_rd, sizeof cfg_msg_rd,
                   sizeof cfg_msg_rd, sig_done, &get_sig);
    wait_for_all_single(&journal_sig, &get_sig.even);

    if (!signal_test(&get_sig.odd)) {
        *cfg_msg = cfg_msg_rd;
    }

    send_interthread_sig(next_me, 0,
                         __signal_number(cfg_sig_remote, next_me));
}


/**
 * Select the next FLR to respond to
 * @param cfg_msg       message to populate with FLR information
 *
 * This method checks the FLR state to determine whether there is an
 * FLR to respond to.  If there is, it populates the "cfg_msg", writes
 * NFP_NET_CFG_UPDATE for that vNIC and sets the appropriate bit of
 * "nfd_flr_sent".
 *
 * The method checks the PF, the lower 32 VFs, and the upper 32 VFs in that
 * order.  If a vNIC is not in use, then it simply acknowledges the FLR
 * immediately.
 */
__intrinsic int
nfd_cfg_next_flr(struct nfd_cfg_msg *cfg_msg)
{
    if (bit_test(flr_in_prog_status, FLR_IN_PROG_BUSY_shf)) {
        if (bit_test(flr_in_prog_status, NFD_FLR_PF_ind)) {
            /* The PF gets priority */

            flr_in_prog_status &= ~(1 << NFD_FLR_PF_ind);

#if NFD_MAX_PF_QUEUES != 0
            /* Setup the parse_msg info */
            cfg_msg->queue = NFD_MAX_PF_QUEUES - 1;
            cfg_msg->vnic = NFD_MAX_VFS;
            cfg_msg->interested = 1;
            cfg_msg->msg_valid = 1;

            cfg_ring_enables[0] = 0;
            cfg_ring_enables[1] = 0;

            /* Rewrite the CFG BAR for other components */
            nfd_flr_write_cfg_msg(NFD_CFG_BASE_LINK(PCIE_ISL), NFD_MAX_VFS);

            /* Set FLR sent atomic */
            nfd_flr_set_sent_pf(PCIE_ISL);

#else
            /* We're not using the PF, just ACK. */
            nfd_flr_ack_pf(PCIE_ISL);
#endif

        } else if (bit_test(flr_in_prog_status, NFD_FLR_VF_LO_ind)) {
            int vf;

            vf = _ffs(flr_in_prog_vf[NFD_FLR_VF_LO_ind]);

            if (vf >= 0) {
                flr_in_prog_vf[NFD_FLR_VF_LO_ind] &= ~(1 << vf);

            } else {
                flr_in_prog_status &= ~(1 << NFD_FLR_VF_LO_ind);
                return -1;
            }

            if (vf < NFD_MAX_VFS) {
                /* Setup the parse_msg info */
                cfg_msg->queue = NFD_MAX_VF_QUEUES - 1;
                cfg_msg->vnic = vf;
                cfg_msg->interested = 1;
                cfg_msg->msg_valid = 1;

                cfg_ring_enables[0] = 0;
                cfg_ring_enables[1] = 0;

                /* Rewrite the CFG BAR for other components */
                nfd_flr_write_cfg_msg(NFD_CFG_BASE_LINK(PCIE_ISL), vf);

                /* Set FLR sent atomic */
                nfd_flr_set_sent_vf(PCIE_ISL, vf);

            } else {

                /* We aren't using this VF, simply acknowledge the FLR. */
                nfd_flr_ack_vf(PCIE_ISL, vf);
            }

        } else if (bit_test(flr_in_prog_status, NFD_FLR_VF_HI_ind)) {
            int vf;

            vf = _ffs(flr_in_prog_vf[NFD_FLR_VF_HI_ind]);

            if (vf >= 0) {
                flr_in_prog_vf[NFD_FLR_VF_HI_ind] &= ~(1 << vf);

            } else {
                flr_in_prog_status &= ~(1 << NFD_FLR_VF_HI_ind);
                return -1;
            }

            vf |= 32;

            if (vf < NFD_MAX_VFS) {
                /* Setup the parse_msg info */
                cfg_msg->queue = NFD_MAX_VF_QUEUES - 1;
                cfg_msg->vnic = vf;
                cfg_msg->interested = 1;
                cfg_msg->msg_valid = 1;

                cfg_ring_enables[0] = 0;
                cfg_ring_enables[1] = 0;

                /* Rewrite the CFG BAR for other components */
                nfd_flr_write_cfg_msg(NFD_CFG_BASE_LINK(PCIE_ISL), vf);

                /* Set FLR sent atomic */
                nfd_flr_set_sent_vf(PCIE_ISL, vf);

            } else {
                /* We aren't using this VF, simply acknowledge the FLR. */
                nfd_flr_ack_vf(PCIE_ISL, vf);
            }

        } else {
            /* FLR_IN_PROG_BUSY_shf is set but we aren't actually busy
             * processing an FLR.  */
            flr_in_prog_status = 0;
            return -1;
        }

    } else {
        return -1;
    }

    return 0;
}


/**
 * Read configuration message from BAR and interpret fields
 * @param cfg_msg       holds state related to the configuration request
 * @param comp          which component to specialise code for
 *
 * This method performs basic consistency checks on the configuration BAR
 * information, and determines whether the message affects the current
 * component.  It sets up internal data such as caching ring enables, and
 * also reads the first ring address and ring sizes (if necessary).  This
 * prepares the internal state for the 'nfd_cfg_proc_msg' method.
 */
__intrinsic void
nfd_cfg_parse_msg(struct nfd_cfg_msg *cfg_msg, enum nfd_cfg_component comp)
{
    /* XXX rewrite to set a signal mask so that we can leave
     * address and size access to complete. */

    __xread unsigned int cfg_bar_data[6];

    ctassert(__is_ct_const(comp));

    if (comp == NFD_CFG_PCI_OUT) {
        /* Need RXRS_ENABLES, at 0x10 */
        mem_read64(cfg_bar_data,
                   NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vnic) + NFP_NET_CFG_CTRL,
                   6 * sizeof(unsigned int));
    } else {
        /* Only need TXRS_ENABLES, at 0x08 */
        mem_read64(cfg_bar_data,
                   NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vnic) + NFP_NET_CFG_CTRL,
                   4 * sizeof(unsigned int));
    }

    /* Check if change affects this component */
    /* Only interested in the change if it contains a ring update */
    if (cfg_bar_data[NFP_NET_CFG_UPDATE >> 2] &
        (NFP_NET_CFG_UPDATE_RING | NFP_NET_CFG_UPDATE_GEN)) {
        cfg_msg->interested = 1;
    }

    /* Set the queue to process to the final queue */
    if (cfg_msg->vnic == NFD_MAX_VFS) {
        cfg_msg->queue = NFD_MAX_PF_QUEUES - 1;
    } else {
        cfg_msg->queue = NFD_MAX_VF_QUEUES - 1;
    }

    /* Copy ring configs
     * If the vNIC is not up, set all ring enables to zero */
    if (cfg_bar_data[NFP_NET_CFG_CTRL] & NFP_NET_CFG_CTRL_ENABLE) {
        unsigned int enables_ind, addr_off, sz_off;

        if (comp == NFD_CFG_PCI_OUT) {
            enables_ind = NFP_NET_CFG_RXRS_ENABLE >> 2;
            addr_off =    NFP_NET_CFG_RXR_ADDR(cfg_msg->queue);
            sz_off =      NFP_NET_CFG_RXR_SZ(cfg_msg->queue);
        } else if (comp == NFD_CFG_PCI_IN0) {
            enables_ind = NFP_NET_CFG_TXRS_ENABLE >> 2;
            addr_off =    NFP_NET_CFG_TXR_ADDR(cfg_msg->queue);
            sz_off =      NFP_NET_CFG_TXR_SZ(cfg_msg->queue);
        } else if (comp == NFD_CFG_PCI_IN1) {
            enables_ind = NFP_NET_CFG_TXRS_ENABLE >> 2;
        } else {
            cterror("Invalid nfd_cfg_component value.");
        }

        /* Access ring enables */
        cfg_ring_enables[0] = cfg_bar_data[enables_ind];
        cfg_ring_enables[1] = cfg_bar_data[enables_ind + 1];

        if (comp == NFD_CFG_PCI_OUT || comp == NFD_CFG_PCI_IN0) {
            /* Cache next ring address and size
             * For size, we want the 4B aligned entry that holds
             * the end ring. */
            sz_off &= ~3;
            mem_read64(&cfg_ring_addr,
                       NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vnic) + addr_off,
                       sizeof(cfg_ring_addr));
            mem_read32(&cfg_ring_sizes,
                       NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vnic) + sz_off,
                       sizeof(cfg_ring_sizes));
        }
    } else {
        /* All rings are set disabled, we won't need any addresses or sizes */
        cfg_ring_enables[0] = 0;
        cfg_ring_enables[1] = 0;
    }
}


/**
 * Extract BAR information
 * @param cfg_msg       holds state related to the configuration request
 * @param queue         which queue to process next
 * @param ring_sz       size of the ring
 * @param ring_base     base address for the ring
 * @param comp          which component to specialise code for
 *
 * Test 'cfg_msg' and internal state to determine whether any queue
 * configuration must be changed in this configuration cycle.  If not, this is
 * indicated through flags in 'cfg_msg', otherwise at least queue will be valid.
 * If the queue must be "up'ed", 'ring_sz' and 'ring_base' will also be valid.
 */
__intrinsic void
nfd_cfg_proc_msg(struct nfd_cfg_msg *cfg_msg, unsigned int *queue,
                  unsigned char *ring_sz, unsigned int ring_base[2],
                  enum nfd_cfg_component comp)
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

    if (cfg_msg->queue == 0) {
        /* This queue is the last */
        cfg_msg->msg_valid = 0;
        return;
    }
    cfg_msg->queue--;

    /* Read values for next queue(s) */
    if (comp == NFD_CFG_PCI_OUT) {
        next_addr_off = NFP_NET_CFG_RXR_ADDR(cfg_msg->queue);
        next_sz_off =   NFP_NET_CFG_RXR_SZ(cfg_msg->queue);
    } else {
        next_addr_off = NFP_NET_CFG_TXR_ADDR(cfg_msg->queue);
        next_sz_off =   NFP_NET_CFG_TXR_SZ(cfg_msg->queue);
    }

    /* We only use the address if the ring is enabled.
     * Otherwise suppress read to save CPP bandwidth. */
    if (_ring_enables_test(cfg_msg)) {
        mem_read64(&cfg_ring_addr,
                   NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vnic) + next_addr_off,
                   sizeof(cfg_ring_addr));
    }

    /* Reread the queue sizes if necessary.
     * Sizes packed 4 per register and wee start from the last queue,
     * so need to reread when the low bits are "3". */
    if ((cfg_msg->queue & 3) == 3) {
        next_sz_off &= ~3;
        mem_read32(&cfg_ring_sizes,
                   NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vnic) + next_sz_off,
                   sizeof(cfg_ring_sizes));
    }
}


__intrinsic void
nfd_cfg_next_queue(struct nfd_cfg_msg *cfg_msg, unsigned int *queue)
{
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
    } else {
        cfg_msg->up_bit = 0;
    }

    if (cfg_msg->queue == 0) {
        /* This queue is the last */
        cfg_msg->msg_valid = 0;
    }
    cfg_msg->queue--;
}


/*
 * Check the ClockResetControl for the PCIe island to ensure PCIe link is up
 * If the PCIe link is not up, it is not safe to run NFD on the island, so
 * halt the ME immediately.
 */
__intrinsic void
nfd_cfg_check_pcie_link()
{
#define NFP_PCIEX_CLOCK_RESET_CTRL                  0x44045400
#define NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_msk     0xf
#define NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_shf     16
#define NFP_PCIEX_CLOCK_RESET_CTRL_ACTIVE           0xf

    unsigned int pcie_sts_raw;
    unsigned int pcie_sts;

    /* Check the ClockResetControl value */
    pcie_sts_raw = xpb_read(NFP_PCIEX_CLOCK_RESET_CTRL |
                            (PCIE_ISL << NFP_PCIEX_ISL_shf));
    pcie_sts = ((pcie_sts_raw >> NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_shf) &
                NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_msk);

    if (pcie_sts != NFP_PCIEX_CLOCK_RESET_CTRL_ACTIVE) {
        /* Write the raw value we read to Mailboxes for debugging purposes */
        local_csr_write(local_csr_mailbox_0, pcie_sts_raw);

        /* Nothing more to do on this PCIe island, stop the ME */
        halt();
    }
}
