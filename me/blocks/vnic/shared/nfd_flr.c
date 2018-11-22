/*
 * Copyright (C) 2014-2018,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/shared/nfd_flr.c
 * @brief         An internal API to perform NFD FLR resets
 */

#include <nfp6000/nfp_qc.h>

#include <assert.h>
#include <nfp.h>

/* XXX nfp/xpb.h doesn't support a sig_done form of the commands */
#include <nfp/xpb.h>
#include <nfp/me.h>
#include <nfp/mem_atomic.h>
#include <nfp/mem_bulk.h>

#include <std/reg_utils.h>

#include <vnic/nfd_common.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/shared/nfd_vf_cfg_iface.h>
#include <vnic/shared/nfd_xpb.h>
#include <vnic/utils/qcntl.h>

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
 * NFD FLR handling consists of 3 main components: a part that notices new
 * FLRs by receiving events and examining HW CSRs, a part that issues
 * reconfiguration messages for each pending FLR in turn, using the regular
 * NFD configuration message passing mechanisms, and a part that notices
 * the FLR configuration messages and acknowledges them to the HW.
 *
 * The HW will record an FLR as "in progress" from the time it was issued
 * until it is finally acked by the FW.  We only Ack the FLR once all NFD
 * owned MEs have had a chance to reconfigure based on the FLR, so we track
 * the FLRs that we have "seen" using atomics.  The first stage sets the
 * atomic for each new FLR that it notices, and the last stage clears the
 * atomic after it has acked the FLR.  Clearing the atomic only after the
 * ack ensures that we do not reissue the reconfiguration for the same FLR.
 */


/* Defines to control how much data we should write with bulk methods */
/* XXX confirm whether or not the FLR reset should write MSIX tables to zero.
 * XXX nfp_net_ctrl.h doesn't seem to have suitable defines to use currently. */
#define NFD_FLR_CLR_START   0x50
#define NFD_FLR_CLR_SZ      (SZ_8K - 0x50)


/* Functions called from the service ME */

/** Clear the bulk of the CFG BAR
 * @param addr      start address of the vNIC CFG BAR
 *
 * This function performs the bulk write of data that won't be reset
 * by other functions (e.g. nfd_flr_init_pf_cfg_bar() and
 * nfd_flr_init_vf_cfg_bar()).  "addr" should be obtained via the
 * appropriate API, e.g. NFD_CFG_BAR_ISL.
 */
__intrinsic void
nfd_flr_clr_bar(__emem char *addr)
{
    __xwrite unsigned int zero[16];
    unsigned int copied_bytes;

    ctassert(__is_aligned(NFD_FLR_CLR_SZ, 8));
    ctassert(sizeof zero > NFP_NET_CFG_VERSION - NFP_NET_CFG_TXRS_ENABLE);
    ctassert(__is_log2(sizeof zero));

    reg_zero(zero, sizeof zero);

    /* Clear the data below the RO fields */
    mem_write64(zero, addr + NFP_NET_CFG_TXRS_ENABLE,
                NFP_NET_CFG_VERSION - NFP_NET_CFG_TXRS_ENABLE);

    addr += NFD_FLR_CLR_START;

    /* Clear the data after the RO fields and up to
     * a convenient alignment */
    copied_bytes = NFD_FLR_CLR_SZ & (sizeof zero - 1);
    mem_write64(zero, addr, copied_bytes);
    addr += copied_bytes;

    /* Clear the balance of the data */
    for (; copied_bytes < NFD_FLR_CLR_SZ; copied_bytes += sizeof zero,
             addr += sizeof zero) {
        mem_write64(zero, addr, sizeof zero);
    }
}


/** Reset the vNIC CFG QC queue to its initial state
 * @param pcie_isl      PCIe island to reset
 * @param vid           vid of vNIC to reset
 * @param event_type    event type to configure
 *
 * Reset the CFG QC to a clean state, in case the previous
 * user left it broken.  This is done twice, once when the
 * FLR is first noticed to set the queue in a state where
 * it cannot generate any new configuration messages, and
 * again after the CFG BAR has been zeroed out to renable
 * configuration message events.
 */
__intrinsic void
nfd_flr_init_cfg_queue(unsigned int pcie_isl, unsigned int vid,
                       unsigned int event_type)
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
    nfd_cfg_queue.event_type = event_type;
    nfd_cfg_queue.ptr        = 0;

    qc_init_queue(pcie_isl, NFD_NATQ2QC(NFD_VID2NATQ(vid, 0), NFD_CFG_QUEUE),
                  &nfd_cfg_queue);
}


/** Init the TLVs in the CFG BAR from the template
 * @param pcie          PCIe island, in the range 0..3
 * @param vid           vid number on the PCIe island
 *
 */
#ifdef NFD_USE_TLV
void
nfd_flr_init_tlv(const unsigned int pcie, unsigned int vid)
{
    __xwrite unsigned int tlv_wr = (NFP_NET_CFG_TLV_TYPE_RESERVED << 16) | \
        (NFD_CFG_TLV_BLOCK_OFF - NFP_NET_CFG_TLV_BASE - 4);
    __emem char *bar_base = nfd_cfg_bar_base(pcie, vid);
    __emem char *tlv_base = nfd_cfg_tlv_tml_base(pcie, vid);

    unsigned int i, offset, size;
    __xread unsigned int read_block[16];
    __xwrite unsigned int write_block[16];

    mem_write32(&tlv_wr, bar_base + NFP_NET_CFG_TLV_BASE, sizeof(tlv_wr));

    for (offset = 0; offset < NFD_CFG_TLV_BLOCK_SZ;
         offset += sizeof(read_block)) {
        mem_read32(read_block, tlv_base + offset, sizeof(read_block));

        reg_cp(write_block, read_block, sizeof(read_block));

        size = NFD_CFG_TLV_BLOCK_SZ - offset;
        if (size > sizeof(write_block))
            size = sizeof(write_block);

        mem_write32(write_block, bar_base + NFD_CFG_TLV_BLOCK_OFF + offset,
                    size);
    }
}
#endif

/** Init the non-zero parts of the PF control BAR
 * @param pcie          PCIe island, in the range 0..3
 * @param vid           vid number on the PCIe island
 *
 */
void
nfd_flr_init_pf_cfg_bar(unsigned int pcie, unsigned int vid)
{
#if (NFD_MAX_PFS != 0)
    unsigned int q_base = NFD_VID2NATQ(vid, 0);
    __emem char *bar_base = nfd_cfg_bar_base(pcie, vid);
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
    mem_write64(&cfg, bar_base + NFP_NET_CFG_VERSION, sizeof cfg);

    mem_write8(&exn_lsc, bar_base + NFP_NET_CFG_LSC, sizeof exn_lsc);

    mem_write8(&cfg2, bar_base + NFP_NET_CFG_RX_OFFSET, sizeof cfg2);
#ifdef NFD_BPF_CAPABLE
    mem_write8(&bpf_cfg, bar_base + NFP_NET_CFG_BPF_ABI, sizeof bpf_cfg);
#endif

#ifdef NFD_USE_TLV_PF
    nfd_flr_init_tlv(pcie, vid);
#endif

#endif
}


/** Init the non-zero parts of the CTRL control BAR
 * @param pcie          PCIe island, in the range 0..3
 * @param vid           vid number on the PCIe island
 *
 * "isl_base" should be obtained via the appropriate API,
 * e.g. NFD_CFG_BASE_LINK.
 */
void
nfd_flr_init_ctrl_cfg_bar(unsigned int pcie, unsigned int vid)
{
#ifdef NFD_USE_CTRL
    unsigned int q_base = NFD_VID2NATQ(vid, 0);
    __emem char *bar_base = nfd_cfg_bar_base(pcie, vid);
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

    mem_write64(&cfg, bar_base + NFP_NET_CFG_VERSION, sizeof cfg);

    mem_write8(&exn_lsc, bar_base + NFP_NET_CFG_LSC, sizeof exn_lsc);

    mem_write8(&cfg2, bar_base + NFP_NET_CFG_RX_OFFSET, sizeof cfg2);

#ifdef NFD_USE_TLV_CTRL
    nfd_flr_init_tlv(pcie, vid);
#endif

#endif
}



/** Init the non-zero parts of the VF control BAR
 * @param vf_cfg_base   start address of the VF CFG symbol for the PCIe island
 * @param pcie          PCIe island, in the range 0..3
 * @param vid           vid number on the PCIe island
 *
 */
void
nfd_flr_init_vf_cfg_bar(__emem char *vf_cfg_base, unsigned int pcie,
                        unsigned int vid)
{
#if ((NFD_MAX_VFS != 0) && (NFD_MAX_VF_QUEUES != 0))
#ifdef NFD_NO_ISOLATION
    unsigned int q_base = NFD_VID2NATQ(vid, 0);
#else
    unsigned int q_base = 0;
#endif
    __emem char *bar_base = nfd_cfg_bar_base(pcie, vid);
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
    __xread unsigned int vf_cfg_rd[2];
    __xwrite unsigned int vf_cfg_wr[2];

    mem_write64(&cfg, bar_base + NFP_NET_CFG_VERSION, sizeof cfg);

    mem_write8(&exn_lsc, bar_base + NFP_NET_CFG_LSC, sizeof exn_lsc);

    mem_write8(&cfg2, bar_base + NFP_NET_CFG_RX_OFFSET, sizeof cfg2);

    /* XXX should vid technically be vf below? */
    mem_read8(&vf_cfg_rd, NFD_VF_CFG_ADDR(vf_cfg_base, vid),
              NFD_VF_CFG_MAC_SZ);
    reg_cp(vf_cfg_wr, vf_cfg_rd, sizeof vf_cfg_rd);
    mem_write8(&vf_cfg_wr, bar_base + NFP_NET_CFG_MACADDR, NFD_VF_CFG_MAC_SZ);

#ifdef NFD_USE_TLV_VF
    nfd_flr_init_tlv(pcie, vid);
#endif

#endif
}


/* Functions called from PCI.IN ME0 */
/**
 * Test whether our PCIe link is up according
 * to PCIeCntrlrConfig0.LinkPowerState
 * Returns true if the link is up
 */
__intrinsic unsigned int
nfd_flr_link_up()
{
    __xread unsigned int pcie_sts_raw;
    unsigned int pcie_sts;
    unsigned int pcie_sts_addr;
    SIGNAL pcie_sts_sig;
    unsigned int up;

#define NFD_FLR_XPB_TIMEOUT_VAL 0xffffffff

    pcie_sts_addr = NFP_PCIEX_LINK_POWER_STATE;
    __asm ct[xpb_read, pcie_sts_raw, pcie_sts_addr, 0,  \
             1], ctx_swap[pcie_sts_sig];
    if (pcie_sts_raw == NFD_FLR_XPB_TIMEOUT_VAL) {
        up = 0;
    } else {
        pcie_sts =
            ((pcie_sts_raw >> NFP_PCIEX_LINK_POWER_STATE_shf) &
             NFP_PCIEX_LINK_POWER_STATE_msk);
        up = pcie_sts;
    }

    return up;
}


/* Check whether we are already processing a PCIe reset,
 * and if not, start one. */
__intrinsic void
nfd_flr_throw_link_rst(unsigned int pcie_isl,
                 __shared __gpr unsigned int *flr_pend_status)
{
    __xread unsigned int seen_flr;
    __mem40 char *atomic_addr;
    SIGNAL atomic_sig;
    unsigned int pf_atomic_data = 1 << NFD_FLR_PCIE_RESET_shf;

    atomic_addr = (NFD_FLR_LINK(pcie_isl) +
                   sizeof pf_atomic_data * NFD_FLR_PF_ind);
    mem_read_atomic(&seen_flr, atomic_addr, sizeof seen_flr);

    if ((seen_flr & pf_atomic_data) == 0) {
        /* We have found an unseen PCIe reset, mark it in local and
         * atomic state. */
        *flr_pend_status |= (1 << NFD_FLR_PCIE_RESET_ind);
        *flr_pend_status |= (1 << NFD_FLR_PEND_BUSY_shf);

        mem_bitset_imm(pf_atomic_data, atomic_addr);
    }
}


/* Check and update LinkPowerState value
 * If link goes down, start PCIe reset */
__intrinsic void
nfd_flr_check_link(unsigned int pcie_isl,
                   __shared __gpr unsigned int *flr_pend_status,
                   int state_clean)
{
    unsigned int link_up;

    link_up = nfd_flr_link_up();
    if (link_up != 0) {
        *flr_pend_status |= (1 << NFD_FLR_PCIE_STATE_ind);
    } else {
        /* Only try to start a PCIe reset message if we were up,
         * and in a "clean" processing state */
        if (!state_clean &&
            (*flr_pend_status & (1 << NFD_FLR_PCIE_STATE_ind))) {
            nfd_flr_throw_link_rst(pcie_isl, flr_pend_status);
        }
        *flr_pend_status &= ~(1 << NFD_FLR_PCIE_STATE_ind);
    }
}


/** Read the HW FLR and nfd_flr_atomic state
 * @param pcie_isl              PCIe island (0..3)
 * @param flr_pend_status    Internal state for FLR processing
 *
 * See nfd_cfg_internal.c for a description of the format of
 * "flr_pend_status".
 */
__intrinsic void
nfd_flr_check_pf(unsigned int pcie_isl,
                 __shared __gpr unsigned int *flr_pend_status)
{
    __xread unsigned int seen_flr;
    __mem40 char *atomic_addr;
    SIGNAL atomic_sig;
    __xread unsigned int flr_data;
    unsigned int xpb_addr;
    SIGNAL xpb_sig;
    unsigned int pf_atomic_data = 1 << NFD_FLR_PF_shf;

    /* Read state of FLR hardware and seen atomics */
    atomic_addr = (NFD_FLR_LINK(pcie_isl) +
                   sizeof pf_atomic_data * NFD_FLR_PF_ind);
    xpb_addr = NFP_PCIEX_FLR_CSR;

    __mem_read_atomic(&seen_flr, atomic_addr, sizeof seen_flr, sizeof seen_flr,
                      sig_done, &atomic_sig);
    __asm ct[xpb_read, flr_data, xpb_addr, 0, 1], sig_done[xpb_sig];

    wait_for_all(&atomic_sig, &xpb_sig);

    /* Test state for an unseen PF FLR */
    if (flr_data & (1 << NFP_PCIEX_FLR_CSR_PF_FLR_IN_PROGRESS_shf)) {
        if ((seen_flr & pf_atomic_data) == 0) {
            /* We have found an unseen PF FLR, mark it in local and
             * atomic state. */
            *flr_pend_status |= (1 << NFD_FLR_PF_ind);
            *flr_pend_status |= (1 << NFD_FLR_PEND_BUSY_shf);

            mem_bitset_imm(pf_atomic_data, atomic_addr);
        }
    }
}


/** Read the HW FLR and nfd_flr_atomic state
 * @param pcie_isl          PCIe island (0..3)
 * @param flr_pend_status   Internal state for FLR processing
 * @param flr_pend_vf       VF specific internal FLR state
 *
 * See nfd_cfg_internal.c for a description of the format of
 * "flr_pend_status" and "flr_pend_vf".
 */
__intrinsic void
nfd_flr_check_vfs(unsigned int pcie_isl,
                  __shared __gpr unsigned int *flr_pend_status,
                  __shared __gpr unsigned int flr_pend_vf[2])
{
    __xread unsigned int seen_flr[2];
    __xread unsigned int hw_flr[2];
    __mem40 char *atomic_addr;
    SIGNAL atomic_sig;
    unsigned int xpb_addr;
    SIGNAL xpb_sig0, xpb_sig1;

    unsigned int new_flr;
    __xwrite unsigned int new_flr_wr[2];

    /* Read state of FLR hardware and seen atomics */
    atomic_addr = NFD_FLR_LINK(pcie_isl);
    __mem_read_atomic(seen_flr, atomic_addr, sizeof seen_flr, sizeof seen_flr,
                      sig_done, &atomic_sig);
    xpb_addr = NFP_PCIEX_PCIE_VF_FLR_IN_PROGRESS0;
    __asm ct[xpb_read, hw_flr + 0, xpb_addr, 0, 1], sig_done[xpb_sig0];
    xpb_addr = NFP_PCIEX_PCIE_VF_FLR_IN_PROGRESS1;
    __asm ct[xpb_read, hw_flr + 4, xpb_addr, 0, 1], sig_done[xpb_sig1];

    wait_for_all(&atomic_sig, &xpb_sig0, &xpb_sig1);

    /* Handle VFs 0 to 31 */
    new_flr = hw_flr[0] & ~seen_flr[NFD_FLR_VF_LO_ind];
    flr_pend_vf[NFD_FLR_VF_LO_ind] |= new_flr;
    if (flr_pend_vf[NFD_FLR_VF_LO_ind] != 0) {
        *flr_pend_status |= (1 << NFD_FLR_VF_LO_ind);
        *flr_pend_status |= (1 << NFD_FLR_PEND_BUSY_shf);
    }
    new_flr_wr[NFD_FLR_VF_LO_ind] = new_flr;

    /* Handle VFs 32 to 63 */
    new_flr = hw_flr[1] & ~seen_flr[NFD_FLR_VF_HI_ind];
    flr_pend_vf[NFD_FLR_VF_HI_ind] |= new_flr;
    if (flr_pend_vf[NFD_FLR_VF_HI_ind] != 0) {
        *flr_pend_status |= (1 << NFD_FLR_VF_HI_ind);
        *flr_pend_status |= (1 << NFD_FLR_PEND_BUSY_shf);
    }
    new_flr_wr[NFD_FLR_VF_HI_ind] = new_flr;

    /* Update the nfd_flr_seen atomic */
    mem_bitset(new_flr_wr, atomic_addr, sizeof new_flr_wr);
}


/** Write the CFG BAR to indicate an FLR is in process
 * @param isl_base      start address of the CFG BARs for the PCIe island
 * @param vid           vNIC ID on the PCIe island
 * @param update        update value to set (see defines below)
 *
 * NFP_NET_CFG_CTRL is cleared so that the vNIC will be disabled, and
 * NFP_NET_CFG_UPDATE is set to the value specified in "update".  The
 * defines NFD_FLR_UPDATE_FLR or NFD_FLR_UPDATE_PCI_RST should be used.
 * They both specify "NFP_NET_CFG_UPDATE_GEN | NFP_NET_CFG_UPDATE_MSIX"
 * so that MEs can respond to it as simply a vNIC being downed.  For FLRs,
 * NFP_NET_CFG_UPDATE_RESET is set as well.  For PCIe resets, a bit in the
 * cfg_msg is used to indicate the reset (so malicious hosts or VMs can't
 * fake a reset).
 *
 * This method can be called for both the PF and the VFs, with suitable
 * vnic values.
 */
#define NFD_FLR_UPDATE_FLR      (NFP_NET_CFG_UPDATE_GEN |   \
                                 NFP_NET_CFG_UPDATE_RESET | \
                                 NFP_NET_CFG_UPDATE_MSIX)
#define NFD_FLR_UPDATE_PCI_RST  (NFP_NET_CFG_UPDATE_GEN |   \
                                 NFP_NET_CFG_UPDATE_MSIX)
__intrinsic void
nfd_flr_write_cfg_msg(__emem char *isl_base, unsigned int vid,
                      unsigned int update)
{
    __xwrite unsigned int cfg_bar_msg[2] = {0, 0};

    cfg_bar_msg[1] = update;
    mem_write64(cfg_bar_msg, NFD_CFG_BAR(isl_base, vid),
                sizeof cfg_bar_msg);
}


/** Acknowledge PCIe island resets by clearing the "nfd_flr_seen" bit
 * @param pcie_isl      PCIe island (0..3)
 */
__intrinsic void
nfd_flr_ack_link_reset(unsigned int pcie_isl)
{
    unsigned int atomic_data;
    __mem40 char *atomic_addr;

    atomic_addr = (NFD_FLR_LINK(pcie_isl) +
                   sizeof atomic_data * NFD_FLR_PF_ind);
    atomic_data = (1 << NFD_FLR_PCIE_RESET_shf);
    mem_bitclr_imm(atomic_data, atomic_addr);

    /* Set NFD_CFG_FLR_AP_SIG_NO so that we recheck the FLR state */
    signal_ctx(NFD_CFG_FLR_AP_CTX_NO, NFD_CFG_FLR_AP_SIG_NO);
}


/** Acknowledge the FLR to the hardware, and clear "nfd_flr_seen" bit
 * @param pcie_isl      PCIe island (0..3)
 *
 * This method issues an XPB write to acknowledge the FLR and an
 * atomic bit clear back-to-back to minimise the likelihood of
 * PCI.IN ME0 seeing an intermediate state.
 */
__intrinsic void
nfd_flr_ack_pf(unsigned int pcie_isl)
{
    __xwrite unsigned int flr_data;
    unsigned int flr_addr;

    unsigned int atomic_data;
    __mem40 char *atomic_addr;


    flr_addr = ((NFP_PCIEX_ISL_BASE | NFP_PCIEX_FLR_CSR) |
            (pcie_isl << NFP_PCIEX_ISL_shf));
    flr_data = (1 << NFP_PCIEX_FLR_CSR_PF_FLR_DONE_shf);

    atomic_addr = (NFD_FLR_LINK(pcie_isl) +
                   sizeof atomic_data * NFD_FLR_PF_ind);
    atomic_data = (1 << NFD_FLR_PF_shf);

    /* Issue the FLR ack and then the atomic clear.  This ensures that
     * the atomic is set until the VF FLR in progress bit is cleared. */
    xpb_write(flr_addr, flr_data);
    mem_bitclr_imm(atomic_data, atomic_addr);

    /* Set NFD_CFG_FLR_AP_SIG_NO so that we recheck the FLR state */
    signal_ctx(NFD_CFG_FLR_AP_CTX_NO, NFD_CFG_FLR_AP_SIG_NO);
}


/** Acknowledge the FLR to the hardware, and clear "nfd_flr_seen" bit
 * @param pcie_isl      PCIe island (0..3)
 * @param vf            VF number on the PCIe island
 *
 * This method issues an XPB write to acknowledge the FLR and an
 * atomic bit clear back-to-back to minimise the likelihood of
 * PCI.IN ME0 seeing an intermediate state.
 */
__intrinsic void
nfd_flr_ack_vf(unsigned int pcie_isl, unsigned int vf)
{
    unsigned int flr_data;
    unsigned int flr_addr;

    __xwrite unsigned int atomic_data;
    __mem40 char *atomic_addr;

    flr_addr = ((NFP_PCIEX_ISL_BASE | NFP_PCIEX_FLR_CSR) |
            (pcie_isl << NFP_PCIEX_ISL_shf));
    flr_data = (1 << NFP_PCIEX_FLR_CSR_VF_FLR_DONE_shf);
    flr_data |= ((vf & NFP_PCIEX_FLR_CSR_VF_FLR_DONE_CHANNEL_msk) <<
                 NFP_PCIEX_FLR_CSR_VF_FLR_DONE_CHANNEL_shf);

    /* nfd_flr_seen is a 64bit mask, sorted from LSB to MSB by NFP
     * address definitions.  This places VFs 0..31 in the 4B from
     * offset 0, and VFs 32..63 from offset 4.  "atomic_addr" should
     * be 4B aligned, so we divide vf by 32 and multiply by 4 to
     * obtain the byte offset. */
    atomic_addr = (NFD_FLR_LINK(pcie_isl) + ((vf / 32) * 4));
    atomic_data = 1 << (vf & (32 - 1));

    /* Issue the FLR ack and then the atomic clear.  This ensures that
     * the atomic is set until the VF FLR in progress bit is cleared. */
    xpb_write(flr_addr, flr_data);
    mem_bitclr(&atomic_data, atomic_addr, sizeof atomic_data);

    /* Set NFD_CFG_FLR_AP_SIG_NO so that we recheck the FLR state */
    signal_ctx(NFD_CFG_FLR_AP_CTX_NO, NFD_CFG_FLR_AP_SIG_NO);
}


/** Check the value of MasterDropIfDisabled and update if it differs
 * @param pcie_isl      PCIe island (0..3)
 * @param value         Desired value for MasterDropIfDisabled
 *
 * This function performs an XPB read of Cntrlr1 to check the current
 * value of the MasterDropIfDisabled bit.  If the requested value
 * differs, then it performs a modify-write to update the value to
 * the requested value.
 *
 * NB: The caller must ensure that nothing else is trying to modify
 * the Cntrlr1 state at the same time (esp unrelated bits).
 */
__intrinsic void
nfd_flr_update_mstr_drop_if_disabled(unsigned int pcie_isl,
                                     unsigned int value)
{
    __xwrite unsigned int bus_mstr_wr;
    unsigned int bus_mstr_rd;
    unsigned int bus_mstr_addr;
    unsigned int curr_val;

    bus_mstr_addr = ((NFP_PCIEX_ISL_BASE | NFP_PCIEX_BUS_MSTR_DISABLED_CFG) |
                    (pcie_isl << NFP_PCIEX_ISL_shf));
    bus_mstr_rd = xpb_read(bus_mstr_addr);
    curr_val =
        (bus_mstr_rd >> NFP_PCIEX_BUS_MSTR_DROP_IF_DISABLED_shf) & 1;

    /* Check requested value and update the CSR if the current
     * doesn't match the requested. */
    if (value) {
        if (curr_val != 1) {
            bus_mstr_rd |=
                (1 << NFP_PCIEX_BUS_MSTR_DROP_IF_DISABLED_shf);
            bus_mstr_wr = bus_mstr_rd;
            xpb_write(bus_mstr_addr, bus_mstr_wr);
        }
    } else {
        if (curr_val != 0) {
            bus_mstr_rd &=
                ~(1 << NFP_PCIEX_BUS_MSTR_DROP_IF_DISABLED_shf);
            bus_mstr_wr = bus_mstr_rd;
            xpb_write(bus_mstr_addr, bus_mstr_wr);
        }
    }
}
