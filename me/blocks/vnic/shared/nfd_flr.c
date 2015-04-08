/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_flr.c
 * @brief         An internal API to perform NFD FLR resets
 */

#include <assert.h>
#include <nfp.h>

/* XXX nfp/xpb.h doesn't support a sig_done form of the commands */
#include <nfp/xpb.h>
#include <nfp/mem_atomic.h>
#include <nfp/mem_bulk.h>

#include <std/reg_utils.h>

#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_internal.h>

#include <nfp_net_ctrl.h>


#define NFP_PCIEX_ISL_BASE                                   0x04000000
#define NFP_PCIEX_ISL_shf                                    24
/* XXX temp defines that loosely match the BSP pcie_monitor_api.h */
#define NFP_PCIEX_COMPCFG_CNTRLR3                            0x0010006c
#define NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_CHANNEL_msk    0x3f
#define NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_CHANNEL_shf    16
#define NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_shf            15
#define NFP_PCIEX_COMPCFG_CNTRLR3_FLR_DONE_shf               14
#define NFP_PCIEX_COMPCFG_CNTRLR3_FLR_IN_PROGRESS_shf        13
#define NFP_PCIEX_COMPCFG_PCIE_VF_FLR_IN_PROGRESS0           0x00100080
#define NFP_PCIEX_COMPCFG_PCIE_VF_FLR_IN_PROGRESS1           0x00100084
#define NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT             0x001000cc
#define NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT_msk         0x3f


/*
 * Defines that set the structure of "nfd_flr_sent".  These are also
 * reused for FLR state in nfd_cfg_internal.c.
 */
#define NFD_FLR_VF_LO_ind   0
#define NFD_FLR_VF_HI_ind   1
#define NFD_FLR_PF_ind      2
#define NFD_FLR_PF_shf      0

/*
 * The "nfd_flr_sent" atomic is used to track which FLRs have been started
 * by PCI.IN ME0 but have not been completed by the service ME.  It prevents
 * us reissuing an FLR message if something else causes us to check the
 * hardware pending mask.
 *
 * "nfd_flr_sent" is structured as an "array" of
 * 16B entries per PCIe island.  Each entry consists of two 32bit bitmasks
 * "VF_LO" and "VF_HI", and an extra 32bit mask with just one bit used for
 * the PF (selected by NFD_FLR_PF_shf).  There is one byte of padding per
 * PCIe island too.
 *
 * Users outside of this file would generally call APIs from this file, so
 * they should not depend on this structure.
 */
#define NFD_FLR_DECLARE                                                 \
    ASM(.alloc_mem nfd_flr_atomic NFD_CFG_RING_EMEM global 64 64)       \
    ASM(.declare_resource nfd_flr_atomic_mem global 64 nfd_flr_atomic)  \
    ASM(.alloc_resource nfd_flr_sent nfd_flr_atomic_mem global 64 64)

#define NFD_FLR_LINK_IND(_isl)                                  \
    ((__mem char *) _link_sym(nfd_flr_sent) + ((_isl) * 16))
#define NFD_FLR_LINK(_isl) NFD_FLR_LINK_IND(_isl)


/* Defines to control how much data we should write with bulk methods */
/* XXX confirm whether or not the FLR reset should write MSIX tables to zero.
 * XXX nfp_net_ctrl.h doesn't seem to have suitable defines to use currently. */
#define NFD_FLR_CLR_START   0x50
#define NFD_FLR_CLR_SZ      (SZ_8K - 0x50)


/* Functions called from the service ME */

/** Clear the bulk of the CFG BAR
 * @param addr      start address of the vNIC CFG BAR
 *
 * This method performs the bulk write of data that won't be reset
 * by other methods (e.g. nfd_flr_write_pf_cap and nfd_flr_write_vf_cap.
 * "addr" should be obtained via the appropriate API, e.g. NFD_CFG_BAR_ISL.
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


/** Rewrite the PF capabilities
 * @param isl_base      start address of the CFG BARs for the PCIe island
 *
 * "isl_base" should be obtained via the appropriate API,
 * e.g. NFD_CFG_BASE_LINK.
 */
void
nfd_flr_write_pf_cap(__emem char *isl_base)
{
    unsigned int tx_q_off = (NFD_MAX_VF_QUEUES * NFD_MAX_VFS * 2);
    __xwrite unsigned int cfg[] = {NFD_CFG_VERSION, 0, NFD_CFG_PF_CAP,
                                   NFD_MAX_PF_QUEUES, NFD_MAX_PF_QUEUES,
                                   NFD_CFG_MAX_MTU, tx_q_off,
                                   NFD_OUT_Q_START + tx_q_off};

    mem_write64(&cfg,
                NFD_CFG_BAR(isl_base, NFD_MAX_VFS) + NFP_NET_CFG_VERSION,
                sizeof cfg);
}


/** Rewrite the VF capabilities
 * @param isl_base      start address of the CFG BARs for the PCIe island
 * @param vf            VF number on the PCIe island
 *
 * "isl_base" should be obtained via the appropriate API,
 * e.g. NFD_CFG_BASE_LINK.
 */
void
nfd_flr_write_vf_cap(__emem char *isl_base, unsigned int vf)
{
    unsigned int tx_q_off = (NFD_MAX_VF_QUEUES * vf * 2);
    __xwrite unsigned int cfg[] = {NFD_CFG_VERSION, 0, NFD_CFG_VF_CAP,
                                   NFD_MAX_VF_QUEUES, NFD_MAX_VF_QUEUES,
                                   NFD_CFG_MAX_MTU, tx_q_off,
                                   NFD_OUT_Q_START + tx_q_off};

    mem_write64(&cfg, NFD_CFG_BAR(isl_base, vf) + NFP_NET_CFG_VERSION,
                sizeof cfg);
}


/** Acknowledge the FLR to the hardware, and clear "nfd_flr_sent" bit
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
    __mem char *atomic_addr;


    flr_addr = ((NFP_PCIEX_ISL_BASE | NFP_PCIEX_COMPCFG_CNTRLR3) |
            (pcie_isl << NFP_PCIEX_ISL_shf));
    flr_data = (1 << NFP_PCIEX_COMPCFG_CNTRLR3_FLR_DONE_shf);

    atomic_addr = (NFD_FLR_LINK(pcie_isl) +
                   sizeof atomic_data * NFD_FLR_PF_ind);
    atomic_data = (1 << NFD_FLR_PF_shf);

    /* Issue the FLR ack and the atomic clear back to back
     * so that they are more likely to complete simultaneously. */
    mem_bitclr_imm(atomic_data, atomic_addr);
    xpb_write(flr_addr, flr_data);
}


/** Acknowledge the FLR to the hardware, and clear "nfd_flr_sent" bit
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

    unsigned int atomic_data;
    __mem char *atomic_addr;

    flr_addr = ((NFP_PCIEX_ISL_BASE | NFP_PCIEX_COMPCFG_CNTRLR3) |
            (pcie_isl << NFP_PCIEX_ISL_shf));
    flr_data = (1 << NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_shf);
    flr_data |= ((vf & NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_CHANNEL_msk) <<
                 NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_CHANNEL_shf);

    /* nfd_flr_sent is a 64bit mask, sorted from LSB to MSB by NFP
     * address definitions, so we can use the NFP byte addressing. */
    atomic_addr = (NFD_FLR_LINK(pcie_isl) + (vf / 8));
    atomic_data = 1 << (vf & (8 - 1));

    /* Issue the FLR ack and the atomic clear back to back
     * so that they are more likely to complete simultaneously. */
    mem_bitclr_imm(atomic_data, atomic_addr);
    xpb_write(flr_addr, flr_data);
}



/* Functions called from PCI.IN ME0 */

/** Perform an atomic read of nfd_flr_atomic
 * @param pcie_isl      PCIe island (0..3)
 * @param flr_sent      read xfers for the result
 */
__intrinsic void
nfd_flr_read_sent(unsigned int pcie_isl, __xread unsigned int flr_sent[3])
{
    mem_read_atomic(flr_sent, NFD_FLR_LINK(pcie_isl), sizeof flr_sent);
}


/** Write the CFG BAR to indicate an FLR is in process
 * @param isl_base      start address of the CFG BARs for the PCIe island
 * @param vnic          vNIC number on the PCIe island
 *
 * NFP_NET_CFG_CTRL is cleared so that the vNIC will be disabled, and
 * NFP_NET_CFG_UPDATE is set to "NFP_NET_CFG_UPDATE_GEN |
 * NFP_NET_CFG_UPDATE_RESET | NFP_NET_CFG_UPDATE_MSIX".  This means that 
 * MEs processing the message can respond to it as an FLR if required, or 
 * simply behave as if the vNIC was being downed.
 *
 * This method can be called for both the PF and the VFs, with suitable
 * vnic values.
 */
__intrinsic void
nfd_flr_write_cfg_msg(__emem char *isl_base, unsigned int vnic)
{
    __xwrite unsigned int cfg_bar_msg[2] = {0, 0};

    cfg_bar_msg[1] = NFP_NET_CFG_UPDATE_GEN | NFP_NET_CFG_UPDATE_RESET | \
	             NFP_NET_CFG_UPDATE_MSIX;

    mem_write64(cfg_bar_msg, NFD_CFG_BAR(isl_base, vnic),
                sizeof cfg_bar_msg);
}


/** Set "nfd_flr_sent" bit for the PF
 * @param pcie_isl      PCIe island (0..3)
 */
__intrinsic void
nfd_flr_set_sent_pf(unsigned int pcie_isl)
{
    unsigned int atomic_data;
    __mem char *atomic_addr;

    atomic_addr = (NFD_FLR_LINK(pcie_isl) +
                   sizeof atomic_data * NFD_FLR_PF_ind);
    atomic_data = (1 << NFD_FLR_PF_shf);

    mem_bitset_imm(atomic_data, atomic_addr);
}


/** Set "nfd_flr_sent" bit for the VF
 * @param pcie_isl      PCIe island (0..3)
 * @param vf            VF number on the PCIe island
 */
__intrinsic void
nfd_flr_set_sent_vf(unsigned int pcie_isl, unsigned int vf)
{
    unsigned int atomic_data;
    __mem char *atomic_addr;

    /* nfd_flr_sent is a 64bit mask, sorted from LSB to MSB by NFP
     * address definitions, so we can use the NFP byte addressing. */
    atomic_addr = (NFD_FLR_LINK(pcie_isl) + (vf / 8));
    atomic_data = 1 << (vf & (8 - 1));

    mem_bitset_imm(atomic_data, atomic_addr);
}
