/*
 * Copyright 2015 Netronome, Inc.
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
 * @file   msix.c
 * @brief  MSI-X library
 */

#ifndef _BLOCKS__VNIC_SVC_MSIX_C_
#define _BLOCKS__VNIC_SVC_MSIX_C_

/*
 * NOTE
 *
 * Sending a MSI-X is basically a PCIe write from the device of a
 * specific 32bit value to a specified host address.  The host OSes
 * places the 64bit address as well as the 32bit value in the MSI-X
 * table, which also contains a 32bit flags word, of which only one
 * bit is used for masking the MSI-X vector.  If the bit is set, the
 * device is not supposed to generate a MSI-X interrupt.
 *
 * A MSI-X table may contain many entries (up to 256) and the PCIe
 * code is told by the driver which entry to use for a given RX and TX
 * ring.
 *
 */

#include <assert.h>
#include <nfp.h>
#include <stdint.h>
#include <types.h>
#include <nfp/me.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>
#include <nfp/mem_bulk.h>
#include <nfp/pcie.h>
#include <vnic/shared/nfd_cfg.h>

/* Imports this variable in the event this file is compiled separate from
 * svc_me.c */
__shared __lmem volatile uint64_t svc_cfg_bars[NFD_MAX_ISL];

/*
 * MSI-X definitions:
 * @PCI_MSIX_TBL_ENTRY_SZ          Size in Bytes for a MSI-X table entry
 * @PCI_MSIX_TBL_ENTRY_SZ32        Size in 32bit words for a MSI-X table entry
 * @PCI_MSIX_TBL_ENTRY_OFF         Offset in MSI-X table for a given vector
 *
 * @PCI_MSIX_TBL_MSG_FLAGS_IDX32   Word index for flags in an entry
 * @PCI_MSIX_TBL_MSG_DATA_IDX32    Word index for message data in an entry
 * @PCI_MSIX_TBL_MSG_ADDR_HI_IDX32 Word index for message address in an entry
 * @PCI_MSIX_TBL_MSG_ADDR_LO_IDX32 Word index for message address in an entry
 *
 * @PCI_MSIX_FLAGS_MASKED          Bit the flags to mask an entry
 *
 * Note, the order for the TBL entries seems wrong, but that's how
 * they pop up in the read transfer registers.
 */
#define PCI_MSIX_TBL_ENTRY_SZ           (4 * 4)
#define PCI_MSIX_TBL_ENTRY_SZ32         (PCI_MSIX_TBL_ENTRY_SZ / 4)
#define PCI_MSIX_TBL_ENTRY_OFF(_x)      (PCI_MSIX_TBL_ENTRY_SZ * (_x))

#define PCI_MSIX_TBL_MSG_ADDR_LO        (0)
#define PCI_MSIX_TBL_MSG_ADDR_LO_IDX32  (PCI_MSIX_TBL_MSG_ADDR_LO / 4)
#define PCI_MSIX_TBL_MSG_ADDR_HI        (4)
#define PCI_MSIX_TBL_MSG_ADDR_HI_IDX32  (PCI_MSIX_TBL_MSG_ADDR_HI / 4)
#define PCI_MSIX_TBL_MSG_DATA           (8)
#define PCI_MSIX_TBL_MSG_DATA_IDX32     (PCI_MSIX_TBL_MSG_DATA / 4)
#define PCI_MSIX_TBL_MSG_FLAGS          (12)
#define PCI_MSIX_TBL_MSG_FLAGS_IDX32    (PCI_MSIX_TBL_MSG_FLAGS / 4)

#define PCIE_MSIX_FLAGS_MASKED          (1 << 0)

/*
 * Offset of the MSI-X table in the VF control BAR
 */
#define NFD_VF_MSIX_TABLE_OFF   0x2000

/**
 * Read the contents of the specified PCIe C2P BAR
 * @param pcie_nr    PCIe island number
 * @param bar_idx    The PCIe CppToPcie BAR index
 *
 * XXX This function should go to flowenv
 */
__intrinsic static unsigned int
pcie_c2p_barcfg_read(unsigned int pcie_nr, unsigned char bar_idx)
{
    unsigned int isl, bar_addr, tmp;
    __xread unsigned int bar_val;
    SIGNAL sig;

    isl = pcie_nr << 30;
    bar_addr = NFP_PCIE_BARCFG_C2P(bar_idx);

    __asm pcie[read_pci, bar_val, isl, <<8, bar_addr, 1], ctx_swap[sig];

    tmp = (unsigned int)bar_val;

    return tmp;
}

/*
 * Calculate the CPP2PCIe bar value (should be somewhere else)
 */
__intrinsic static unsigned int
pcie_c2p_barcfg_addr(unsigned int addr_hi,
                     unsigned int addr_lo, unsigned int req_id)
{
    unsigned int tmp;

    __asm dbl_shf[tmp, addr_hi, addr_lo, >>27];
    tmp = tmp & NFP_PCIE_BARCFG_C2P_ADDR_msk;

    /* Configure RID if req_id is non-zero or not constant */
    if ((!__is_ct_const(req_id)) || (req_id != 0)) {
        tmp |= NFP_PCIE_BARCFG_C2P_ARI_ENABLE;
        tmp |= NFP_PCIE_BARCFG_C2P_ARI(req_id);
    }

    return tmp;
}

/*
 * CPP2PCIe BAR allocation
 * XXX Are these defined as resources somewhere?
 */
enum pcie_cpp2pci_bar {
    PCIE_CPP2PCI_MSIX = 0,
    PCIE_CPP2PCI_FREE1,
    PCIE_CPP2PCI_FREE2,
    PCIE_CPP2PCI_FREE3,
    PCIE_CPP2PCI_FREE4,
    PCIE_CPP2PCI_FREE5,
    PCIE_CPP2PCI_FREE6,
    PCIE_CPP2PCI_FREE7
};

/* Global variable to cache the current CPP 2 PCIe BAR */
__gpr static unsigned int msix_cur_cpp2pci_addr = 0;


/**
 * Send MSI-X interrupt for a PF, and optionally mask the interrupt
 *
 * Returns 0 on success and non-zero when the entry is masked.
 *
 * @param pcie_nr     PCIe cluster number
 * @param entry_nr    MSI-X table entry number
 * @param mask_en     Boolean, should interrupt be masked after sending.
 * @return            0 on success, else the interrupt was masked.
 *
 * The PF MSI-X table is in SRAM in the PCIe Island and the hardware support a 
 * mechanism for generating a MSI-X via a CSR write.  The hardware currently 
 * does not handle pending MSI-X correctly, so we check the MSI-X control word 
 * manually.
 *
 * The steps are as follows: - Check if the entry is masked - If not, generate 
 * an interrupt (by writing to the appropriate CSR) - If the caller asks us to 
 * mask, mask the entry.
 *
 * Note, there is a race potential race between reading the status and
 * generating the interrupt, but this race can only happen if the driver masks 
 * the interrupt in between the ME reading the MSI-X control word and 
 * attempting to send the interrupt.  Since the driver is not masking the 
 * interrupt the race should not happen.
 */
__intrinsic int
msix_pf_send(unsigned int pcie_nr, unsigned int entry_nr, unsigned int mask_en)
{
    uint32_t entry_addr_hi;
    uint32_t entry_addr_lo;

    uint32_t gen_addr_hi;
    uint32_t gen_addr_lo;

    __xwrite uint32_t msix_wdata;
    __xread uint32_t flags_r;
    __xwrite uint32_t flags_w;

    SIGNAL sig;

    int ret = 1;

    /* Calculate address for MSI-X table entry. The table is located
     * in PCIe SRAM offset 0 */
    entry_addr_hi = pcie_nr << 30;
    entry_addr_lo = NFP_PCIE_SRAM_BASE;
    entry_addr_lo += PCI_MSIX_TBL_ENTRY_OFF(entry_nr) + PCI_MSIX_TBL_MSG_FLAGS;

    /* Check if the entry is currently masked */
    __asm {
        pcie[read_pci, flags_r, entry_addr_hi, <<8, entry_addr_lo, 1], \
            ctx_swap[sig]
    }

    /* If vector masked, return */
    if (flags_r & PCIE_MSIX_FLAGS_MASKED)
        goto out;

    /* Send the interrupt. */
    gen_addr_hi = pcie_nr << 30;
    gen_addr_lo = NFP_PCIE_DMA_MSIX_INSTR_GEN(0);
    msix_wdata = entry_nr;
    __asm {
        pcie[write_pci, msix_wdata, gen_addr_hi, <<8, gen_addr_lo, 1], \
            ctx_swap[sig]
    }

    if (mask_en) {
        flags_w = flags_r | PCIE_MSIX_FLAGS_MASKED;
        __asm {
            pcie[write_pci, flags_w, entry_addr_hi, <<8, entry_addr_lo, 1], \
                ctx_swap[sig]
        }
    }

    ret = 0;

out:
    return ret;
}

/**
 * Send MSI-X interrupt for specified virtual function and optionally mask
 * @param pcie_nr     PCIe cluster number
 * @param vf_nr       Virtual function number (0 to 15)
 * @param entry_nr    MSI-X table entry number
 * @param mask_en     Boolean, should interrupt be masked after sending.
 * @return            0 on success, else the interrupt was masked.
 *
 * There is no hardware support for MSI-X in VFs so this is
 * implemented entirely in software.  The MSI-X table for each VF is
 * located in its control BAR.  We use a single CPP-2-PCIe BAR for
 * performing the PCI write to generate a MSI-X.  We cache its config
 * so we don't need to re-program the CPP2PCIe BAR for every MSI-X.
 *
 * The steps are as follows:
 * - Read the table entry
 * - Check if the entry is masked
 * - Reprogram the CPP2PCIe BAR if necessary
 * - If not, generate an interrupt (by performing a PCIe write)
 * - If the caller asks us to mask, mask the entry.
 *
 * The same potential race as for the PF exists in this code too.
 */
__intrinsic int
msix_vf_send(unsigned int pcie_nr, unsigned int vf_nr,
             unsigned int entry_nr, unsigned int mask_en)
{
    unsigned int data;
    unsigned int addr_hi;
    unsigned int addr_lo;
    unsigned int flags;
    unsigned int bar_addr;

    __emem char *msix_table_addr;
    __xread uint32_t tmp[PCI_MSIX_TBL_ENTRY_SZ32];

    __xwrite uint32_t msix_data;
    __xwrite uint32_t flags_w;

    SIGNAL msix_sig, mask_sig;

    int ret = 1;

    msix_table_addr =
        (__emem char *)NFD_CFG_BAR(svc_cfg_bars[pcie_nr - 4], vf_nr);
    msix_table_addr += NFD_VF_MSIX_TABLE_OFF;

    /* Read the full table entry */
    mem_read8(tmp,
              msix_table_addr + PCI_MSIX_TBL_ENTRY_OFF(entry_nr), sizeof(tmp));
    flags =   tmp[PCI_MSIX_TBL_MSG_FLAGS_IDX32];
    data =    tmp[PCI_MSIX_TBL_MSG_DATA_IDX32];
    addr_hi = tmp[PCI_MSIX_TBL_MSG_ADDR_HI_IDX32];
    addr_lo = tmp[PCI_MSIX_TBL_MSG_ADDR_LO_IDX32];

    /* If masked, we are done */
    if (flags & PCIE_MSIX_FLAGS_MASKED)
        goto out;

    /* Check if we need to re-configure the CPP2PCI BAR */
    bar_addr = pcie_c2p_barcfg_addr(addr_hi, addr_lo, (vf_nr + 64));
    if (bar_addr != msix_cur_cpp2pci_addr) {
        pcie_c2p_barcfg(pcie_nr, PCIE_CPP2PCI_MSIX,
                        addr_hi, addr_lo, (vf_nr + 64));
        msix_cur_cpp2pci_addr = bar_addr;

        /* Read back the BAR config to make sure it has been written */
        pcie_c2p_barcfg_read(pcie_nr, PCIE_CPP2PCI_MSIX);
    }

    /* Send the MSI-X and automask.  We overlap the commands so that
     * they happen roughly at the same time. */
    msix_data = data;
    __pcie_write(&msix_data, pcie_nr, PCIE_CPP2PCI_MSIX, addr_hi, addr_lo,
                 sizeof(msix_data), sizeof(msix_data), sig_done, &msix_sig);

    if (mask_en) {
        flags_w = flags | PCIE_MSIX_FLAGS_MASKED;
        __mem_write8(&flags_w,
                     msix_table_addr + PCI_MSIX_TBL_ENTRY_OFF(entry_nr) +
                     PCI_MSIX_TBL_MSG_FLAGS,
                     sizeof(flags_w), sizeof(flags_w), sig_done, &mask_sig);

        wait_for_all(&msix_sig, &mask_sig);
    } else {
        wait_for_all(&msix_sig);
    }

    ret = 0;

out:
    return ret;
}

#endif /* !_BLOCKS__VNIC_SVC_MSIX_C_ */
