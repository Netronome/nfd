/*
 * Copyright (C) 2014-2019,  Netronome Systems, Inc.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file          blocks/vnic/shared/nfd_cfg_internal.c
 * @brief         An internal API to access to NFD configuration data
 */

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>
#include <nfp6000/nfp_qc.h>

#include <assert.h>
#include <nfp.h>
#include <nfp_chipres.h>

#include <nfp/me.h>
#include <nfp/mem_atomic.h>
#include <nfp/mem_bulk.h>
#include <nfp/mem_ring.h>
#include <nfp/pcie.h>
#include <nfp/xpb.h>
#include <std/event.h>

#include <vnic/nfd_common.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_ctrl.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/qc.h>
#include <vnic/utils/qcntl.h>

#include <vnic/shared/nfd_cfg_pcie_monitor.c>
#include <vnic/shared/nfd_flr.c>

#include <nfp_net_ctrl.h>


#ifndef NFD_OUT_RX_OFFSET
#warning "NFD_OUT_RX_OFFSET not defined: defaulting to NFP_NET_RX_OFFSET which is sub-optimal"
#define NFD_OUT_RX_OFFSET NFP_NET_RX_OFFSET
#endif /* NFD_OUT_RX_OFFSET */

#ifndef NFD_RSS_HASH_FUNC
#if ((NFD_CFG_PF_CAP | NFP_CFG_VF_CAP) & NFP_NET_CFG_CTRL_RSS)
#warning "NFD_RSS_HASH_FUNC not defined: defaulting to Toeplitz"
#endif /* ((NFD_CFG_PF_CAP | NFP_CFG_VF_CAP) & NFP_NET_CFG_CTRL_RSS) */
#define NFD_RSS_HASH_FUNC NFP_NET_CFG_RSS_TOEPLITZ
#endif /* NFD_RSS_HASH_FUNC */


/*
 * Reserve PCIe DMA CFG registers
 * These are owned by this file so they can rewritten on PCIe reset
 */
PCIE_DMA_CFG_ALLOC_OFF(nfd_in_gather_dma_cfg, island, PCIE_ISL,
                       NFD_IN_GATHER_CFG_REG, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_in_data_dma_cfg, island, PCIE_ISL,
                       NFD_IN_DATA_CFG_REG, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_in_data_sig_only_dma_cfg, island, PCIE_ISL,
                       NFD_IN_DATA_CFG_REG_SIG_ONLY, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_out_fl_desc_dma_cfg, island, PCIE_ISL,
                       NFD_OUT_FL_CFG_REG, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_out_rx_desc_dma_cfg, island, PCIE_ISL,
                       NFD_OUT_DESC_CFG_REG, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_out_data_dma_cfg, island, PCIE_ISL,
                       NFD_OUT_DATA_CFG_REG, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_out_data_sig_only_dma_cfg, island, PCIE_ISL,
                       NFD_OUT_DATA_CFG_REG_SIG_ONLY, 1);


_NFP_CHIPRES_ASM(.alloc_mem nfd_cfg_ring_mem NFD_CFG_RING_EMEM global \
                 (NFD_MAX_ISL * NFD_CFG_NUM_RINGS * NFD_CFG_RING_SZ)  \
                 (NFD_MAX_ISL * NFD_CFG_NUM_RINGS * NFD_CFG_RING_SZ))

#define NFD_CFG_RINGS_INIT_IND(_isl)                                    \
    _NFP_CHIPRES_ASM(.declare_resource nfd_cfg_ring_mem##_isl           \
                     global 12288 nfd_cfg_ring_mem)                     \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_mem##_isl##0          \
                     nfd_cfg_ring_mem##_isl global 2048 2048)           \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_mem##_isl##1          \
                     nfd_cfg_ring_mem##_isl global 2048 2048)           \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_mem##_isl##2          \
                     nfd_cfg_ring_mem##_isl global 2048 2048)           \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_mem##_isl##3          \
                     nfd_cfg_ring_mem##_isl global 2048 2048)           \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_mem##_isl##4          \
                     nfd_cfg_ring_mem##_isl global 2048 2048)           \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_mem##_isl##5          \
                     nfd_cfg_ring_mem##_isl global 2048 2048)           \
                                                                        \
    _NFP_CHIPRES_ASM(.init_mu_ring nfd_cfg_ring_num##_isl##0            \
                     nfd_cfg_ring_mem##_isl##0)                         \
    _NFP_CHIPRES_ASM(.init_mu_ring nfd_cfg_ring_num##_isl##1            \
                     nfd_cfg_ring_mem##_isl##1)                         \
    _NFP_CHIPRES_ASM(.init_mu_ring nfd_cfg_ring_num##_isl##2            \
                     nfd_cfg_ring_mem##_isl##2)                         \
    _NFP_CHIPRES_ASM(.init_mu_ring nfd_cfg_ring_num##_isl##3            \
                     nfd_cfg_ring_mem##_isl##3)                         \
    _NFP_CHIPRES_ASM(.init_mu_ring nfd_cfg_ring_num##_isl##4            \
                     nfd_cfg_ring_mem##_isl##4)                         \
    _NFP_CHIPRES_ASM(.init_mu_ring nfd_cfg_ring_num##_isl##5            \
                     nfd_cfg_ring_mem##_isl##5)


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


#if (NFD_MAX_VFS != 0)
#define NFD_CFG_VF_DECLARE_IND(_isl)                                    \
    NFD_CFG_BASE_DECLARE(_isl)                                          \
    _NFP_CHIPRES_ASM(.declare_resource nfd_cfg_base##_isl##_res global  \
                     ((NFD_MAX_VFS + NFD_MAX_PFS + NFD_MAX_CTRL) *      \
                       NFP_NET_CFG_BAR_SZ)                              \
                     nfd_cfg_base##_isl)                                \
    _NFP_CHIPRES_ASM(.alloc_resource _pf##_isl##_net_vf_bar             \
                     nfd_cfg_base##_isl##_res+0                         \
                     global (NFD_MAX_VFS * NFP_NET_CFG_BAR_SZ))         \

#else
#define NFD_CFG_VF_DECLARE_IND(_isl)
#endif

#define NFD_CFG_VF_DECLARE(_isl) NFD_CFG_VF_DECLARE_IND(_isl)


#if (defined(NFD_USE_CTRL) && (PCIE_ISL == 0))
#define NFD_CFG_CTRL_DECLARE_IND(_isl)                                      \
    NFD_CFG_BASE_DECLARE(_isl)                                              \
    _NFP_CHIPRES_ASM(.declare_resource nfd_cfg_base##_isl##_res global      \
                     ((NFD_MAX_VFS + NFD_MAX_PFS + NFD_MAX_CTRL) *          \
                       NFP_NET_CFG_BAR_SZ)                                  \
                     nfd_cfg_base##_isl)                                    \
    _NFP_CHIPRES_ASM(.alloc_resource _pf##_isl##_net_ctrl_bar               \
                     nfd_cfg_base##_isl##_res+(NFD_MAX_VFS *                \
                                               NFP_NET_CFG_BAR_SZ)          \
                     global (NFD_MAX_CTRL * NFP_NET_CFG_BAR_SZ))
#else
#define NFD_CFG_CTRL_DECLARE_IND(_isl)
#endif

#define NFD_CFG_CTRL_DECLARE(_isl) NFD_CFG_CTRL_DECLARE_IND(_isl)


#if NFD_MAX_PFS != 0
#define NFD_CFG_PF_DECLARE_IND(_isl)                                    \
    NFD_CFG_BASE_DECLARE(_isl)                                          \
    _NFP_CHIPRES_ASM(.declare_resource nfd_cfg_base##_isl##_res global  \
                     ((NFD_MAX_VFS + NFD_MAX_PFS + NFD_MAX_CTRL) *      \
                       NFP_NET_CFG_BAR_SZ)                              \
                     nfd_cfg_base##_isl)                                \
    _NFP_CHIPRES_ASM(.alloc_resource _pf##_isl##_net_bar0               \
                     nfd_cfg_base##_isl##_res+((NFD_MAX_VFS + NFD_MAX_CTRL) * \
                                               NFP_NET_CFG_BAR_SZ)      \
                     global (NFD_MAX_PFS * NFP_NET_CFG_BAR_SZ))         \
    _NFP_CHIPRES_ASM(.alloc_mem nfd_cfg_pf##_isl##_num_ports emem global 8 8)  \
    _NFP_CHIPRES_ASM(.init nfd_cfg_pf##_isl##_num_ports NFD_MAX_PFS)

#else
#define NFD_CFG_PF_DECLARE_IND(_isl)
#endif

#define NFD_CFG_PF_DECLARE(_isl) NFD_CFG_PF_DECLARE_IND(_isl)


#ifdef NFD_NET_APP_ID
#define NFD_NET_APP_ID_DECLARE_IND(_isl) \
    _NFP_CHIPRES_ASM(.alloc_mem _pf##_isl##_net_app_id ctm global 8 8) \
    _NFP_CHIPRES_ASM(.init _pf##_isl##_net_app_id+0 (NFD_NET_APP_ID))
#else
#define NFD_NET_APP_ID_DECLARE_IND(_isl)
#endif

#define NFD_NET_APP_ID_DECLARE(_isl) NFD_NET_APP_ID_DECLARE_IND(_isl)


_NFP_CHIPRES_ASM(.alloc_mem _abi_pcie_error_cnt emem global 64 64)
#define NFD_CFG_PCIE_ERROR_ISL_SZ       16
#define NFD_CFG_PCIE_ERROR_CORR         0x0
#define NFD_CFG_PCIE_ERROR_NON_FATAL    0x4
#define NFD_CFG_PCIE_ERROR_FATAL        0x8
#define NFD_CFG_PCIE_ERROR_LOCAL        0xc


NFD_FLR_DECLARE;


__shared __gpr struct qc_bitmask cfg_queue_bmsk;

/*
 * "flr_pend_status" is a mixed register containing processing state
 * The bits indicate whether an FLR is pending service, and which types.
 * They are:
 *
 * NFD_FLR_PF_ind: A PF FLR is pending or in progress.
 * NFD_FLR_VF_LO_ind: A VF FLR is pending on at least one of VFs 0..31.
 *                    flr_pend_vf[NFD_FLR_VF_LO_ind] must be examined
 *                    to determine which VFs have pending FLRs.
 * NFD_FLR_VF_HI_ind: A VF FLR is pending on at least one of VFs 32..63.
 *                    flr_pend_vf[NFD_FLR_VF_HI_ind] must be examined
 *                    to determine which VFs have pending FLRs.
 * NFD_FLR_PCIE_RESET_ind:  A PCIe reset is underway
 * NFD_FLR_PCIE_STATE_ind:  State of the PCIe island: 0 == down, 1 == up
 * NFD_FLR_PEND_BUSY_shf: One of the above bits, excluding
 *                        NFD_FLR_PCIE_STATE_ind, is set.  Used to short
 *                        circuit nfd_cfg_next_flr() if no FLRs pending.
 *
 * The other processing state indicates which, if any, PF vNIC should be
 * processed next.  The number stored is the NFD vnic number so in the
 * range 0..63.
 */
static __shared __gpr unsigned int flr_pend_status = 0;

/* Fields for tracking progress on PF FLRs and PCIe island resets.
 * Separate fields are required so that PCIe resets can interrupt
 * PF FLRs */
#define NFD_CFG_FLR_NEXT_PF_shf     8
#define NFD_CFG_FLR_NEXT_PF_msk     0x3f
#define NFD_CFG_FLR_NEXT_VID_shf    14
#define NFD_CFG_FLR_NEXT_VID_msk    0x3f

/*
 * "flr_pend_vf" consists of two 32-bit bitmask covering the up-to 64 VFs
 * in the system.  flr_pend_vf[NFD_FLR_VF_LO_ind] covers the low 32 VFs, and
 * flr_pend_vf[NFD_FLR_VF_HI_ind] covers the high 32 VFs.  Within each bitmask,
 * the bit offset of a vf can be computed by "1 << (vf & 31)".
 */
static __shared __gpr unsigned int flr_pend_vf[2] = {0, 0};
static SIGNAL flr_ap_sig;
static __xread unsigned int flr_ap_xfer;


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
 * Enable B0 DMA ByteMask swapping to ensure that DMAs with the byte
 * swap token complete correctly for DMAs that aren't 4B multiples in size.
 */
void
_nfd_cfg_dma_enable_DmaByteMaskSwap()
{
/* XXX nfp6000/nfp_pcie.h should provide this */
#define NFP_PCIE_DMA_DBG_REG_0          0x400f0

    __gpr unsigned int addr_hi = PCIE_ISL << 30;
    unsigned int dma_dbg_reg_0_addr = NFP_PCIE_DMA_DBG_REG_0;
    __xwrite unsigned int data = 0x80000000;
    SIGNAL sig;

    __asm pcie[write_pci, data, addr_hi, <<8, dma_dbg_reg_0_addr, 1], \
        ctx_swap[sig]
}


/**
 * XXX This function can be used for development when the host doesn't
 * use CPP APIs to move BARs etc
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
    bar_tmp.target = 7; /* MU CPP target */
    bar_tmp.token = 0;
    bar_tmp.map_type = NFP_PCIE_BARCFG_P2C_MAP_TYPE_BULK;
    bar_tmp.len = NFP_PCIE_BARCFG_P2C_LEN_64BIT;
    bar_base_addr = NFP_PCIE_BARCFG_P2C(0, 0);
    _bar_addr(&bar_tmp, NFD_CFG_BASE_LINK(PCIE_ISL));
    bar = bar_tmp;

    __asm pcie[write_pci, bar, addr_hi, <<8, bar_base_addr, 1],    \
        ctx_swap[sig];

    /* BAR1 (resource2) PCI.IN queues  */
    bar_tmp.target = 0; /* Internal PCIe Target */
    bar_tmp.token = 0;
    bar_tmp.len = NFP_PCIE_BARCFG_P2C_LEN_32BIT;
    _bar_addr(&bar_tmp, 0x80000);

    bar_base_addr = NFP_PCIE_BARCFG_P2C(1, 0);
    bar = bar_tmp;
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

#if (NFD_MAX_VFS != 0)
    init_qc_queues(PCIE_ISL, &nfd_cfg_queue, NFD_CFG_QUEUE,
                   4 * NFD_MAX_VF_QUEUES, NFD_MAX_VFS);
#endif

#ifdef NFD_USE_CTRL
    init_qc_queues(PCIE_ISL, &nfd_cfg_queue,
                   NFD_CFG_QUEUE + (4 * NFD_CTRL_QUEUE),
                   4 * NFD_MAX_CTRL_QUEUES, NFD_MAX_CTRL);
#endif

#if (NFD_MAX_PFS != 0)
    init_qc_queues(PCIE_ISL, &nfd_cfg_queue,
                   NFD_CFG_QUEUE + (4 * NFD_FIRST_PF_QUEUE),
                   4 * NFD_MAX_PF_QUEUES, NFD_MAX_PFS);
#endif
}

/*
 * Initialise the VF and PF control BAR
 */
static void
_nfd_cfg_init_vf_cfg_bar(unsigned int vid)
{
#if ((NFD_MAX_VFS != 0) && (NFD_MAX_VF_QUEUES != 0))
#ifdef NFD_NO_ISOLATION
    unsigned int q_base = NFD_VID2NATQ(vid, 0);
#else
    unsigned int q_base = 0;
#endif
    __xwrite unsigned int cfg[] = {NFD_CFG_VERSION(VF),
                                   (NFP_NET_CFG_STS_LINK_RATE_UNSUPPORTED
                                    << NFP_NET_CFG_STS_LINK_RATE_SHIFT) | 0,
                                   NFD_CFG_VF_CAP,
                                   NFD_MAX_VF_QUEUES, NFD_MAX_VF_QUEUES,
                                   NFD_CFG_MAX_MTU,
                                   NFD_NATQ2QC(q_base, NFD_IN_TX_QUEUE),
                                   NFD_NATQ2QC(q_base, NFD_OUT_FL_QUEUE)};
    __xwrite unsigned int exn_lsc = 0xffffffff;
    __xwrite unsigned int cfg2[] = {NFD_OUT_RX_OFFSET,
                                    NFD_RSS_HASH_FUNC};
#ifdef NFD_USE_TLV_VF
    __xwrite unsigned int tlv_wr = (NFP_NET_CFG_TLV_TYPE_RESERVED << 16) | \
        (NFD_CFG_TLV_BLOCK_OFF - NFP_NET_CFG_TLV_BASE - 4);
#endif

    mem_write64(&cfg, NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_VERSION,
                sizeof cfg);

    mem_write8(&exn_lsc, NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_LSC,
               sizeof exn_lsc);

    mem_write8(&cfg2,
               NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_RX_OFFSET,
               sizeof cfg2);

#ifdef NFD_USE_TLV_VF
    mem_write32(&tlv_wr,
        NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_TLV_BASE,
        sizeof(tlv_wr));
#endif

#endif
}


static void
_nfd_cfg_init_ctrl_cfg_bar(unsigned int vid)
{
    unsigned int q_base =  NFD_VID2NATQ(vid, 0);
    __xwrite unsigned int cfg[] = {NFD_CFG_VERSION(CTRL),
                                   (NFP_NET_CFG_STS_LINK_RATE_UNSUPPORTED
                                    << NFP_NET_CFG_STS_LINK_RATE_SHIFT) | 0,
                                   NFD_CFG_CTRL_CAP,
                                   NFD_MAX_CTRL_QUEUES, NFD_MAX_CTRL_QUEUES,
                                   NFD_CFG_MAX_MTU,
                                   NFD_NATQ2QC(q_base, NFD_IN_TX_QUEUE),
                                   NFD_NATQ2QC(q_base, NFD_OUT_FL_QUEUE)};
    __xwrite unsigned int exn_lsc = 0xffffffff;
    __xwrite unsigned int cfg2[] = {NFD_OUT_RX_OFFSET,
                                    NFD_RSS_HASH_FUNC};
#ifdef NFD_USE_TLV_CTRL
    __xwrite unsigned int tlv_wr = (NFP_NET_CFG_TLV_TYPE_RESERVED << 16) | \
        (NFD_CFG_TLV_BLOCK_OFF - NFP_NET_CFG_TLV_BASE - 4);
#endif


    mem_write64(&cfg, NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_VERSION,
                sizeof cfg);

    mem_write8(&exn_lsc, NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_LSC,
               sizeof exn_lsc);

    mem_write8(&cfg2, NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_RX_OFFSET,
               sizeof cfg2);

#ifdef NFD_USE_TLV_CTRL
    mem_write32(&tlv_wr,
        NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_TLV_BASE,
        sizeof(tlv_wr));
#endif

}


static void
_nfd_cfg_init_pf_cfg_bar(unsigned int vid)
{
    unsigned int q_base =  NFD_VID2NATQ(vid, 0);
    __xwrite unsigned int cfg[] = {NFD_CFG_VERSION(PF),
                                   (NFP_NET_CFG_STS_LINK_RATE_UNSUPPORTED
                                    << NFP_NET_CFG_STS_LINK_RATE_SHIFT) | 0,
                                   NFD_CFG_PF_CAP,
                                   NFD_MAX_PF_QUEUES, NFD_MAX_PF_QUEUES,
                                   NFD_CFG_MAX_MTU,
                                   NFD_NATQ2QC(q_base, NFD_IN_TX_QUEUE),
                                   NFD_NATQ2QC(q_base, NFD_OUT_FL_QUEUE)};
    __xwrite unsigned int exn_lsc = 0xffffffff;
    __xwrite unsigned int cfg2[] = {NFD_OUT_RX_OFFSET,
                                    NFD_RSS_HASH_FUNC};
#ifdef NFD_USE_TLV_PF
    __xwrite unsigned int tlv_wr = (NFP_NET_CFG_TLV_TYPE_RESERVED << 16) | \
        (NFD_CFG_TLV_BLOCK_OFF - NFP_NET_CFG_TLV_BASE - 4);
#endif

#ifdef NFD_BPF_CAPABLE
#ifndef NFD_BPF_ABI
#define NFD_BPF_ABI (NFP_NET_BPF_ABI)
#endif
#ifndef NFD_BPF_CAPS
#define NFD_BPF_CAPS 0
#endif
#ifndef NFD_BPF_MAX_LEN
#define NFD_BPF_MAX_LEN (8 * 1024 - NFD_BPF_START_OFF)
#endif
#ifndef NFD_BPF_STACK_SZ
#define NFD_BPF_STACK_SZ 0
#endif

    __xwrite unsigned int bpf_cfg[] =
        { (NFD_BPF_ABI | (NFD_BPF_CAPS << 8) | (NFD_BPF_MAX_LEN << 16)),
          NFD_BPF_START_OFF | NFD_BPF_DONE_OFF << 16,
          NFD_BPF_STACK_SZ / 64 | 30 << 8 /* CTM buf size / 64 */ };
#endif

#ifdef NFD_USE_OVERSUBSCRIPTION
    /* Adjust the number of queues advertised to the host
     * for the last PF */
    if (vid == NFD_LAST_PF) {
        cfg[(NFP_NET_CFG_MAX_TXRINGS - NFP_NET_CFG_VERSION) / 4] =
            NFD_LAST_PF_MAX_QUEUES;
        cfg[(NFP_NET_CFG_MAX_RXRINGS - NFP_NET_CFG_VERSION) / 4] =
            NFD_LAST_PF_MAX_QUEUES;
    }
#endif
    mem_write64(&cfg, NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_VERSION,
                sizeof cfg);

    mem_write8(&exn_lsc, NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_LSC,
               sizeof exn_lsc);

    mem_write8(&cfg2, NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_RX_OFFSET,
               sizeof cfg2);

#ifdef NFD_BPF_CAPABLE
    mem_write8(&bpf_cfg,
               NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_BPF_ABI,
               sizeof bpf_cfg);
#endif

#ifdef NFD_USE_TLV_PF
    mem_write32(&tlv_wr,
        NFD_CFG_BAR_ISL(PCIE_ISL, vid) + NFP_NET_CFG_TLV_BASE,
        sizeof(tlv_wr));
#endif
}


void
_nfd_cfg_dma_cfg_reg_setup()
{
    struct nfp_pcie_dma_cfg cfg_tmp;
    struct pcie_dma_cfg_one cfg_one;
    __xwrite struct nfp_pcie_dma_cfg cfg;

    /* PCI.IN desc DMAs */
    cfg_one.__raw = 0;
    cfg_one.signal_only = 0;
    cfg_one.end_pad     = 0;
    cfg_one.start_pad   = 0;
    /* Ordering settings? */
    cfg_one.target_64   = 0;
    cfg_one.cpp_target  = 15;
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_IN_GATHER_CFG_REG, cfg_one);

    /* PCI.IN data DMAs */
    cfg_tmp.__raw = 0;
    /* Signal only configuration for null messages */
    cfg_tmp.signal_only_odd = 1;
    cfg_tmp.target_64_odd = 1;
    cfg_tmp.cpp_target_odd = 7;
    /* Regular configuration */
    cfg_tmp.signal_only_even = 0;
    cfg_tmp.end_pad_even = 0;
    cfg_tmp.start_pad_even = 0;
    cfg_tmp.target_64_even = 1;
    cfg_tmp.cpp_target_even = 7;
    cfg = cfg_tmp;

    pcie_dma_cfg_set_pair(PCIE_ISL, NFD_IN_DATA_CFG_REG, &cfg);

    /* PCI.OUT FL desc DMAs */
    cfg_one.__raw = 0;
    cfg_one.signal_only = 0;
    cfg_one.end_pad     = 0;
    cfg_one.start_pad   = 0;
    /* Ordering settings? */
    cfg_one.target_64   = 1;
    cfg_one.cpp_target  = 7;
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_OUT_FL_CFG_REG, cfg_one);

    /* PCI.OUT RX desc DMAs use the same settings */
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_OUT_DESC_CFG_REG, cfg_one);

    /* PCI.OUT data DMAs use the same settings */
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_OUT_DATA_CFG_REG, cfg_one);
}


/**
 * Perform per PCIe island nfd_cfg initialisation
 */
void
nfd_cfg_setup()
{
    unsigned int vid;
    unsigned int ring;

    /* XXX remove once .declare_resource and .alloc_resource support
     * bracketed expressions. */
    ctassert(NFD_CFG_NUM_RINGS == 8);
    ctassert(NFD_CFG_RING_SZ == 2048);

    /*
     * Write compile time configured NFD_MAX_VF_QUEUES and
     * NFD_MAX_PF_QUEUES to mem.
     * XXX Could be .init'ed?
     */
#if NFD_MAX_VFS != 0
    for (vid = 0; vid < NFD_MAX_VFS; vid++) {
        _nfd_cfg_init_vf_cfg_bar(vid);
    }
#endif

#if (defined(NFD_USE_CTRL) && (PCIE_ISL == 0))
    _nfd_cfg_init_ctrl_cfg_bar(NFD_CTRL_VNIC);
#endif

#if (NFD_MAX_PFS != 0)
    for (vid = NFD_FIRST_PF; vid <= NFD_LAST_PF; vid++) {
        _nfd_cfg_init_pf_cfg_bar(vid);
    }
#endif

}


/**
 * Perform per PCIe island nfd_cfg setup in PCI power domain
 */
void
nfd_cfg_pci_reconfig()
{
#if (NFD_MAX_VFS > 0)
    nfd_cfg_setup_vf();
#endif

    /* Setup the configuration queues */
    _nfd_cfg_queue_setup();

    /* Setup the DMA configuration registers */
    _nfd_cfg_dma_cfg_reg_setup();

    /* Clear MasterDropIfDisabled, that may have been set while
     * processing PCIe resets */
    nfd_flr_update_mstr_drop_if_disabled(PCIE_ISL, 0);

#if defined(__NFP_IS_6XXX)
    /* Enable NFP6XXX B0 DMA ByteMask swapping */
    _nfd_cfg_dma_enable_DmaByteMaskSwap();
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

    __assign_relative_register(&flr_ap_sig, NFD_CFG_FLR_AP_SIG_NO);

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
 * The PF and VF FLR state are checked separately, using nfd_flr.c APIs.
 * PCIe state changes are associated with FLRs (both on start and completion),
 * so we also monitor and acknowledge PCIe state changes.  This ensures that
 * we will be aware of VF FLRs that occur, even if we are already processing
 * an FLR.
 */
void
nfd_cfg_check_flr_ap(int state_clean)
{
    if (signal_test(&flr_ap_sig)) {
        __xread unsigned int flr_sent[3];
        __xread unsigned int pcie_state_change_stat;
        __xread unsigned int pf_csr;
        __xread unsigned int vf_csr[2];
        unsigned int vendor_msg;
        unsigned int state_change_ack;
        unsigned int pcie_error_ack;
        unsigned int int_mgr_status;
        int vf;

        local_csr_write(local_csr_mailbox_0, flr_ap_xfer);

        /* Call nfd_flr.c API to update the FLR pending state */
        nfd_flr_check_link(PCIE_ISL, &flr_pend_status, state_clean);
        nfd_flr_check_flr(PCIE_ISL, &flr_pend_status, flr_pend_vf);

        /* Acknowledge PCIE vendor defined messages */
        vendor_msg = xpb_read(NFP_PCIEX_VENDOR_MSG);
        if (vendor_msg & (1 << NFP_PCIEX_VENDOR_MSG_VALID_shf)) {
            /* "PcieMsgValid" is write one to clear */
            vendor_msg |= (1 << NFP_PCIEX_VENDOR_MSG_VALID_shf);
            xpb_write(NFP_PCIEX_VENDOR_MSG, vendor_msg);
        }

        /* Acknowledge PCIe state changes */
        state_change_ack = xpb_read(NFP_PCIEX_PCIE_STATE_CHANGE_STAT);
        state_change_ack &= NFP_PCIEX_PCIE_STATE_CHANGE_STAT_msk;
        if (state_change_ack) {
            xpb_write(NFP_PCIEX_PCIE_STATE_CHANGE_STAT, state_change_ack);
        }

        /* Count and acknowledge PCIe errors */
        pcie_error_ack = xpb_read(NFP_PCIEX_PCIE_ERR);
        pcie_error_ack &= NFP_PCIEX_PCIE_ERR_msk;
        if (pcie_error_ack) {
            __emem char *error_cnt;

            error_cnt = (__emem char *) _link_sym(_abi_pcie_error_cnt);
            error_cnt += PCIE_ISL * NFD_CFG_PCIE_ERROR_ISL_SZ;
            mem_add32_imm(NFP_PCIEX_PCIE_ERR_CORR(pcie_error_ack),
                          error_cnt + NFD_CFG_PCIE_ERROR_CORR);
            mem_add32_imm(NFP_PCIEX_PCIE_ERR_NON_FATAL(pcie_error_ack),
                          error_cnt + NFD_CFG_PCIE_ERROR_NON_FATAL);
            mem_add32_imm(NFP_PCIEX_PCIE_ERR_FATAL(pcie_error_ack),
                          error_cnt + NFD_CFG_PCIE_ERROR_FATAL);
            mem_add32_imm(NFP_PCIEX_PCIE_ERR_LOCAL(pcie_error_ack),
                          error_cnt + NFD_CFG_PCIE_ERROR_LOCAL);

            xpb_write(NFP_PCIEX_PCIE_ERR, pcie_error_ack);
        }

        /* Recheck InterruptManager.Status  */
        int_mgr_status = xpb_read(NFP_PCIEX_PCIE_INT_MGR_STATUS);
        if ((int_mgr_status & NFP_PCIEX_PCIE_INT_MGR_STATUS_RECHK_msk) != 0) {
            /* We have a non-zero status that is NOT due to outstanding FLRs
             * or unexpected events that we can't handle.  Therefore, we flag
             * the AP signal again so that the interrupt status is rechecked.
             * The FLR bits are excluded as they take time to complete, and
             * the FLR acknowledgement methods reset the AP signal as well.
             */
            signal_ctx(NFD_CFG_FLR_AP_CTX_NO, NFD_CFG_FLR_AP_SIG_NO);
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
 * Ack the FLR autopush, ignoring interrupt and resetting event filter
 *
 * When the PCIe monitor is servicing PCIe interrupts, we ignore autopushes
 * and simply rearm for the next event.
 */
__intrinsic void
nfd_cfg_ack_flr_ap()
{
    if (signal_test(&flr_ap_sig)) {
        local_csr_write(local_csr_mailbox_0, flr_ap_xfer);

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
 * Ack outstanding FLRs without starting CFG messages
 *
 * When we are waiting for LinkPowerState to become good, we need
 * to ensure that any unhandled FLRs are acked.  This is not necessary
 * if the GPIO indicates we are down, as the ARM resets the PCIe island
 * before we leave that state.
 */
__intrinsic void
nfd_cfg_ack_pending_flr()
{
    int vf;

    /* Our island is in a clean state, so just ack the FLRs */
    if (bit_test(flr_pend_status, NFD_FLR_PF_ind)) {
        /* The PF gets priority over VFs */
        nfd_flr_ack_pf(PCIE_ISL);
        flr_pend_status &= ~(1 << NFD_FLR_PF_ind);

    } else if (bit_test(flr_pend_status, NFD_FLR_VF_LO_ind)) {
        vf = _ffs(flr_pend_vf[NFD_FLR_VF_LO_ind]);

        if (vf >= 0) {
            flr_pend_vf[NFD_FLR_VF_LO_ind] &= ~(1 << vf);
        } else {
            flr_pend_status &= ~(1 << NFD_FLR_VF_LO_ind);
            return -1;
        }

        nfd_flr_ack_vf(PCIE_ISL, vf);

    } else if (bit_test(flr_pend_status, NFD_FLR_VF_HI_ind)) {
        vf = _ffs(flr_pend_vf[NFD_FLR_VF_HI_ind]);

        if (vf >= 0) {
            flr_pend_vf[NFD_FLR_VF_HI_ind] &= ~(1 << vf);

        } else {
            flr_pend_status &= ~(1 << NFD_FLR_VF_HI_ind);
            return -1;
        }

        vf |= 32;

        nfd_flr_ack_vf(PCIE_ISL, vf);
    }
}


/**
 * Update the GPIO state in this ME
 *
 * If GPIO indicates link down, start PCIe reset
 */
__intrinsic void
nfd_cfg_check_gpio(SIGNAL *gpio_sig, int state_clean)
{
    if (signal_test(gpio_sig)) {
        if (nfd_cfg_pcie_monitor_link_up()) {
            flr_pend_status |= (1 << NFD_FLR_GPIO_STATE_ind);
        } else {
            flr_pend_status &= ~(1 << NFD_FLR_GPIO_STATE_ind);
            if (!state_clean) {
                nfd_flr_throw_link_rst(PCIE_ISL, &flr_pend_status);
            }
        }
    }
}


/* Returns true when the PCIe reset has been ACKed */
__intrinsic int
nfd_cfg_poll_rst_ack(volatile SIGNAL *poll_sig)
{
    __mem40 char *rst_ack_addr;
    __xread unsigned int rst_ack_xfer;
    int ret;

    if (signal_test(poll_sig)) {
        rst_ack_addr = NFD_FLR_LINK(PCIE_ISL) + sizeof(int) * NFD_FLR_PF_ind;
        mem_read_atomic(&rst_ack_xfer, rst_ack_addr, sizeof(rst_ack_xfer));

        /* If PCIe reset not yet acknowledged, rearm alarm;
         * otherwise return success. */
        if (bit_test(rst_ack_xfer, NFD_FLR_PCIE_RESET_shf)) {
            set_alarm(NFD_CFG_RST_POLL_CYCLES, poll_sig);
            ret = 0;
        } else {
            ret = 1;
        }

    } else {
        ret = 0;
    }

    return ret;
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
    int vid;
    int ret;

    /* Get a bmsk queue number from the configuration queue bmsk */
    ret = select_queue(&queue, &cfg_queue_bmsk);
    if (ret) {
        /* We haven't found a configuration queue to service */
        vid = -1;
    } else {
        __xread unsigned int wptr_raw;
        struct nfp_qc_sts_hi wptr;
        unsigned int qc_queue;

        /* We have a configuration queue to service */

        /* Compute the vNIC vid and QC queue associated with that CFG queue */
        vid = NFD_CFGQ2VID(queue);
        qc_queue = NFD_NATQ2QC(queue, NFD_CFG_QUEUE);

        /* Confirm that the queue is not empty */
        wptr.__raw = qc_read(PCIE_ISL, qc_queue, QC_WPTR);

        if (!wptr.empty) {
            qc_add_to_ptr(PCIE_ISL, qc_queue, QC_RPTR, 1);
        } else {
            /* The qc queue was already empty, which might indicate
             * an FLR has occurred after the CFG message was started.
             * There is nothing to do on this CFG queue. */
            vid = -1;
        }

        /* Clear the bit in the bitmask so the queue isn't picked again,
         * and increment the QC queue read pointer */
        clear_queue(&queue, &cfg_queue_bmsk);
   }

    return vid;
}


/**
 * Add a cfg_msg to the start of the ring pipeline
 * @param cfg_msg           message to add
 * @param rnum              ring number to use for the ring journal
 */
__intrinsic void
nfd_cfg_start_cfg_msg(struct nfd_cfg_msg *cfg_msg, unsigned int rnum)
{
    struct nfd_cfg_msg cfg_msg_tmp;
    __xwrite struct nfd_cfg_msg cfg_msg_wr;
    mem_ring_addr_t ring_addr = (unsigned long long) NFD_CFG_EMEM >> 8;

    /* Clear the internal state fields and set msg_valid before sending  */
    cfg_msg_tmp.__raw = 0;
    cfg_msg_tmp.msg_valid = 1;
    cfg_msg_tmp.error = cfg_msg->error;
    cfg_msg_tmp.pci_reset = cfg_msg->pci_reset;
    cfg_msg_tmp.vid = cfg_msg->vid;
    cfg_msg_wr.__raw = cfg_msg_tmp.__raw;

    mem_workq_add_work(rnum, ring_addr, &cfg_msg_wr, sizeof cfg_msg_wr);
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
    if (bit_test(flr_pend_status, NFD_FLR_PEND_BUSY_shf)) {
        if (bit_test(flr_pend_status, NFD_FLR_PCIE_RESET_ind)) {
            /* PCIe resets get the highest priority */
            unsigned int vid;

            vid = ((flr_pend_status >> NFD_CFG_FLR_NEXT_VID_shf) &
                   NFD_CFG_FLR_NEXT_VID_msk);

            /* Set the last queue to service for this VID */
            cfg_msg->__raw = 0;
            cfg_msg->queue = NFD_VID_MAXQS(vid) - 1;

            /* Setup the remaining parse_msg info */
            cfg_msg->msg_valid = 1;
            cfg_msg->pci_reset = 1;
            cfg_msg->interested = 1;
            cfg_msg->vid = vid;

            cfg_ring_enables[0] = 0;
            cfg_ring_enables[1] = 0;

            /* Rewrite the CFG BAR for other components */
            nfd_flr_write_cfg_msg(NFD_CFG_BASE_LINK(PCIE_ISL), vid,
                                  NFD_FLR_UPDATE_PCI_RST);
            nfd_flr_init_cfg_queue(PCIE_ISL, vid,
                                   NFP_QC_STS_LO_EVENT_TYPE_NEVER);

            /* Check whether we have finished with the PCIe reset */
            if (vid == NFD_LAST_PF) {
                /* This was the last PF vNIC, so clear the state */
                flr_pend_status &= ~(NFD_CFG_FLR_NEXT_VID_msk <<
                                     NFD_CFG_FLR_NEXT_VID_shf);
                flr_pend_status &= ~(1 << NFD_FLR_PCIE_RESET_ind);

            } else {
                /* We have more vNICs to service, so increment vid and
                 * leave NFD_FLR_PCIE_RESET_ind set. */
                flr_pend_status &= ~(NFD_CFG_FLR_NEXT_VID_msk <<
                                     NFD_CFG_FLR_NEXT_VID_shf);
                vid++;
                flr_pend_status |= (vid << NFD_CFG_FLR_NEXT_VID_shf);
            }

        } else if (bit_test(flr_pend_status, NFD_FLR_PF_ind)) {
            /* The PF gets priority over VFs */
            unsigned int vid;

#if ((NFD_MAX_PFS != 0) || defined(NFD_USE_CTRL))

            vid = ((flr_pend_status >> NFD_CFG_FLR_NEXT_PF_shf) &
                   NFD_CFG_FLR_NEXT_PF_msk);

            /* vid == 0 means nothing in progress, so pick the starting
             * new starting vid. */
            if (vid == 0) {
#if (defined(NFD_USE_CTRL) && (PCIE_ISL == 0))
                vid = NFD_CTRL_VNIC;
#else
                vid = NFD_FIRST_PF;
#endif
            }

            /* Set the last queue to service for this VID */
            cfg_msg->__raw = 0;
            cfg_msg->queue = NFD_VID_MAXQS(vid) - 1;

            /* Setup the remaining parse_msg info */
            cfg_msg->msg_valid = 1;
            cfg_msg->interested = 1;
            cfg_msg->vid = vid;

            cfg_ring_enables[0] = 0;
            cfg_ring_enables[1] = 0;

            /* Rewrite the CFG BAR for other components */
            nfd_flr_write_cfg_msg(NFD_CFG_BASE_LINK(PCIE_ISL), vid,
                                  NFD_FLR_UPDATE_FLR);
            nfd_flr_init_cfg_queue(PCIE_ISL, vid,
                                   NFP_QC_STS_LO_EVENT_TYPE_NEVER);

            /* Check whether we have finished with the PF FLR */
            if (vid == NFD_LAST_PF) {
                /* This was the last PF vNIC, so clear the state */
                flr_pend_status &= ~(NFD_CFG_FLR_NEXT_PF_msk <<
                                     NFD_CFG_FLR_NEXT_PF_shf);
                flr_pend_status &= ~(1 << NFD_FLR_PF_ind);

            } else {
                /* We have more vNICs to service, so increment vid and
                 * leave NFD_FLR_PF_ind set. */
                flr_pend_status &= ~(NFD_CFG_FLR_NEXT_PF_msk <<
                                     NFD_CFG_FLR_NEXT_PF_shf);
                vid++;
                flr_pend_status |= (vid << NFD_CFG_FLR_NEXT_PF_shf);
            }

#else
            /* We're not using the PF, just ACK. */
            nfd_flr_ack_pf(PCIE_ISL);
            flr_pend_status &= ~(1 << NFD_FLR_PF_ind);
#endif

        } else if (bit_test(flr_pend_status, NFD_FLR_VF_LO_ind)) {
            int vf;

            vf = _ffs(flr_pend_vf[NFD_FLR_VF_LO_ind]);

            if (vf >= 0) {
                flr_pend_vf[NFD_FLR_VF_LO_ind] &= ~(1 << vf);

            } else {
                flr_pend_status &= ~(1 << NFD_FLR_VF_LO_ind);
                return -1;
            }

            if (vf < NFD_MAX_VFS) {
                /* Setup the parse_msg info */
                cfg_msg->__raw = 0;
                cfg_msg->msg_valid = 1;
                cfg_msg->interested = 1;
                cfg_msg->queue = NFD_MAX_VF_QUEUES - 1;
                cfg_msg->vid = vf;

                cfg_ring_enables[0] = 0;
                cfg_ring_enables[1] = 0;

                /* Rewrite the CFG BAR for other components */
                nfd_flr_write_cfg_msg(NFD_CFG_BASE_LINK(PCIE_ISL),
                                      NFD_VNIC2VID(NFD_VNIC_TYPE_VF, vf),
                                      NFD_FLR_UPDATE_FLR);
                nfd_flr_init_cfg_queue(PCIE_ISL,
                                       NFD_VNIC2VID(NFD_VNIC_TYPE_VF, vf),
                                       NFP_QC_STS_LO_EVENT_TYPE_NEVER);

            } else {

                /* We aren't using this VF, simply acknowledge the FLR. */
                nfd_flr_ack_vf(PCIE_ISL, vf);
            }

        } else if (bit_test(flr_pend_status, NFD_FLR_VF_HI_ind)) {
            int vf;

            vf = _ffs(flr_pend_vf[NFD_FLR_VF_HI_ind]);

            if (vf >= 0) {
                flr_pend_vf[NFD_FLR_VF_HI_ind] &= ~(1 << vf);

            } else {
                flr_pend_status &= ~(1 << NFD_FLR_VF_HI_ind);
                return -1;
            }

            vf |= 32;

            if (vf < NFD_MAX_VFS) {
                /* Setup the parse_msg info */
                cfg_msg->__raw = 0;
                cfg_msg->msg_valid = 1;
                cfg_msg->interested = 1;
                cfg_msg->queue = NFD_MAX_VF_QUEUES - 1;
                cfg_msg->vid = vf;

                cfg_ring_enables[0] = 0;
                cfg_ring_enables[1] = 0;

                /* Rewrite the CFG BAR for other components */
                nfd_flr_write_cfg_msg(NFD_CFG_BASE_LINK(PCIE_ISL),
                                      NFD_VNIC2VID(NFD_VNIC_TYPE_VF, vf),
                                      NFD_FLR_UPDATE_FLR);
                nfd_flr_init_cfg_queue(PCIE_ISL,
                                       NFD_VNIC2VID(NFD_VNIC_TYPE_VF, vf),
                                       NFP_QC_STS_LO_EVENT_TYPE_NEVER);


            } else {
                /* We aren't using this VF, simply acknowledge the FLR. */
                nfd_flr_ack_vf(PCIE_ISL, vf);
            }

        } else {
            /* NFD_FLR_PEND_BUSY_shf is set but we aren't actually busy
             * processing an FLR.  */
            flr_pend_status  &= ~(1 << NFD_FLR_PEND_BUSY_shf);
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
                   NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vid) + NFP_NET_CFG_CTRL,
                   6 * sizeof(unsigned int));
    } else {
        /* Only need TXRS_ENABLES, at 0x08 */
        mem_read64(cfg_bar_data,
                   NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vid) + NFP_NET_CFG_CTRL,
                   4 * sizeof(unsigned int));
    }

    /* Check if change affects this component */
    /* Only interested in the change if it contains a ring update */
    if (cfg_bar_data[NFP_NET_CFG_UPDATE >> 2] &
        (NFP_NET_CFG_UPDATE_RING | NFP_NET_CFG_UPDATE_GEN)) {
        cfg_msg->interested = 1;
    }

    /* Set the queue to process to the final queue */
    cfg_msg->queue = NFD_VID_MAXQS(cfg_msg->vid) - 1;

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
                       NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vid) + addr_off,
                       sizeof(cfg_ring_addr));
            mem_read32(&cfg_ring_sizes,
                       NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vid) + sz_off,
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
                   NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vid) + next_addr_off,
                   sizeof(cfg_ring_addr));
    }

    /* Reread the queue sizes if necessary.
     * Sizes packed 4 per register and wee start from the last queue,
     * so need to reread when the low bits are "3". */
    if ((cfg_msg->queue & 3) == 3) {
        next_sz_off &= ~3;
        mem_read32(&cfg_ring_sizes,
                   NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg->vid) + next_sz_off,
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
 *
 * This function "busy waits" on the PCIe link check to ensure that no other
 * contexts can execute while it tests the link.
 */
__intrinsic void
nfd_cfg_check_pcie_clock()
{
#define RESET_CTRL_CORE_msk     0x1
#define RESET_CTRL_PCI_msk      0x2
#define RESET_CTRL_MEG0_msk     0x4
#define RESET_CTRL_MEG1_msk     0x8
#define RESET_CTRL_NON_PCI_msk  (RESET_CTRL_CORE_msk | \
                                 RESET_CTRL_MEG0_msk | \
                                 RESET_CTRL_MEG1_msk)

    __xread unsigned int pcie_sts_raw;
    unsigned int pcie_sts;
    unsigned int pcie_sts_addr;
    SIGNAL pcie_sts_sig;

    /* Check the ClockResetControl value */
    pcie_sts_addr = (NFP_PCIEX_CLOCK_RESET_CTRL |
                     (PCIE_ISL << NFP_PCIEX_ISL_shf));
    __asm ct[xpb_read, pcie_sts_raw, pcie_sts_addr, 0,  \
             1], sig_done[pcie_sts_sig];

    /* Busy wait on the read signal to prevent other threads executing */
    while (!signal_test(&pcie_sts_sig));

    pcie_sts = ((pcie_sts_raw >> NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_shf) &
                NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_msk);

    if ((pcie_sts & RESET_CTRL_NON_PCI_msk) != RESET_CTRL_NON_PCI_msk) {
        /* Write the raw value we read to Mailboxes for debugging purposes */
        local_csr_write(local_csr_mailbox_0, NFD_CFG_PCIE_LINK_DOWN);
        local_csr_write(local_csr_mailbox_1, pcie_sts_raw);

        /* Nothing more to do on this PCIe island, stop the ME */
        halt();
    }

#ifndef NFD_USE_MULTI_HOST
    if ((pcie_sts & RESET_CTRL_PCI_msk) != RESET_CTRL_PCI_msk) {
        /* Write the raw value we read to Mailboxes for debugging purposes */
        local_csr_write(local_csr_mailbox_0, NFD_CFG_PCIE_LINK_DOWN);
        local_csr_write(local_csr_mailbox_1, pcie_sts_raw);

        /* Nothing more to do on this PCIe island, stop the ME */
        halt();
    }
#endif
}



/*
 * Check the link power state and GPIO state to set an initial value in
 * flr_pend_status, NFD_FLR_PCIE_STATE_ind and NFD_FLR_GPIO_STATE_ind
 */
__intrinsic void
nfd_cfg_set_curr_pcie_sts()
{
    __xread unsigned int pcie_sts_raw;
    unsigned int pcie_sts;
    unsigned int pcie_sts_addr;
    SIGNAL pcie_sts_sig;

#define RESET_CTRL_PCI_msk      0x2


    /* Test the ClockResetControl for reset removed
     * and then check LinkPowerState if not in reset */
    pcie_sts_addr = (NFP_PCIEX_CLOCK_RESET_CTRL |
                     (PCIE_ISL << NFP_PCIEX_ISL_shf));
    __asm ct[xpb_read, pcie_sts_raw, pcie_sts_addr, 0,  \
             1], ctx_swap[pcie_sts_sig];
    pcie_sts = ((pcie_sts_raw >> NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_shf) &
                NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_msk);
    if (pcie_sts & RESET_CTRL_PCI_msk) {
        /* Test LinkPowerState ASAP after seeing reset removed */
        if (nfd_flr_link_up()) {
            flr_pend_status |= (1 << NFD_FLR_PCIE_STATE_ind);
        } else {
            flr_pend_status &= ~(1 << NFD_FLR_PCIE_STATE_ind);
        }
    } else {
        /* The island is in reset, so mark it down */
        flr_pend_status &= ~(1 << NFD_FLR_PCIE_STATE_ind);
    }

    /* Set an initial value for NFD_FLR_GPIO_STATE_ind */
    if (nfd_cfg_pcie_monitor_link_up()) {
        flr_pend_status |= (1 << NFD_FLR_GPIO_STATE_ind);
    } else {
        flr_pend_status &= ~(1 << NFD_FLR_GPIO_STATE_ind);
    }
}
