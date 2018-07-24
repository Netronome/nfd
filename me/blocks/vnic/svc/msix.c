/*
 * Copyright (C) 2015-2016,  Netronome Systems, Inc.  All rights reserved.
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
#include <vnic/svc/msix.h>


MSIX_DECLARE;


/* Global variable to cache the current CPP 2 PCIe BAR */
volatile __shared __lmem unsigned int msix_cur_cpp2pci_addr[NFD_MAX_ISL] = {0};


/**
 * Send MSI-X interrupt for a PF and optionally mask the interrupt
 *
 * Returns 0 on success and non-zero when the entry is masked.
 *
 * @param pcie_nr     PCIe island number (0 to 3)
 * @param bar_nr      CPP2PCIe bar to use
 * @param entry_nr    MSI-X table entry number
 * @param mask_en     Boolean, should interrupt be masked after sending.
 * @return            0 on success, else the interrupt was masked.
 *
 * The PF MSI-X table is in SRAM in the PCIe Island and the hardware
 * supports a mechanism for generating a MSI-X via a CSR write.
 * However, if using said CSR, pending MSI-X are not handled correctly
 * and when using auto-masking there is a race condition between the
 * HW generating the MSI-X and this code masking it (the PCIe block
 * does not guarantee ordering).  We therefore manually generate a
 * MSI-X using a PCIe write as this gives better control.
 *
 * The steps are as follows:
 * - Read the table entry
 * - Check if the entry is masked
 * - Reprogram the CPP2PCIe BAR if necessary
 * - If not, generate an interrupt (by performing a PCIe write)
 * - If the caller asks us to mask, mask the entry.
 *
 * For a given PCI Island and CPP2PCIe BAR this function is only safe
 * to be called from within a single context. If multiple contexts (or
 * MEs) are used to send MSI-X interrupts from the same PCIe Island
 * they must use separate CPP2PCIe BARs.
 *
 * Note, there is a race potential race between reading the status and
 * generating the interrupt, but this race can only happen if the
 * driver masks the interrupt in between the ME reading the MSI-X
 * control word and attempting to send the interrupt.  Since the
 * driver is not masking the interrupt the race should not happen.
 */
__intrinsic int
msix_pf_send(unsigned int pcie_nr, unsigned int bar_nr,
             unsigned int entry_nr, unsigned int mask_en)
{
    uint32_t entry_addr_hi;
    uint32_t entry_addr_lo;

    unsigned int addr_hi;
    unsigned int addr_lo;
    unsigned int data;
    unsigned int flags;

    unsigned int bar_addr;

    __xread uint32_t tmp[PCI_MSIX_TBL_ENTRY_SZ32];
    __xwrite uint32_t msix_data;
    __xwrite uint32_t flags_w;

    SIGNAL msix_sig, mask_sig;

    int ret = 1;

    /* Calculate address for MSI-X table entry in PCIe SRAM */
    entry_addr_hi = pcie_nr << 30;
    entry_addr_lo = NFP_PCIE_SRAM_BASE;
    entry_addr_lo += PCI_MSIX_TBL_ENTRY_OFF(entry_nr);

    /* Check if the entry is currently masked */
    __asm {
        pcie[read_pci, tmp, entry_addr_hi, <<8, entry_addr_lo, \
             PCI_MSIX_TBL_ENTRY_SZ32], ctx_swap[msix_sig]
    }

    addr_lo = tmp[PCI_MSIX_TBL_MSG_ADDR_LO_IDX32];
    addr_hi = tmp[PCI_MSIX_TBL_MSG_ADDR_HI_IDX32];
    data =    tmp[PCI_MSIX_TBL_MSG_DATA_IDX32];
    flags =   tmp[PCI_MSIX_TBL_MSG_FLAGS_IDX32];

    /* If masked, we are done */
    if (flags & PCIE_MSIX_FLAGS_MASKED)
        goto out;

    /* Check if we need to re-configure the CPP2PCI BAR */
    bar_addr = pcie_c2p_barcfg_val(addr_hi, addr_lo, 0);
    if (bar_addr != msix_cur_cpp2pci_addr[pcie_nr]) {
        pcie_c2p_barcfg_set_expl(pcie_nr, bar_nr, bar_addr);
        msix_cur_cpp2pci_addr[pcie_nr] = bar_addr;
    }

    /* Send the MSI-X and automask.  We overlap the commands so that
     * they happen roughly at the same time. */
    msix_data = data;
    __pcie_write(&msix_data, pcie_nr, bar_nr, addr_hi, addr_lo,
                 sizeof(msix_data), sizeof(msix_data), sig_done, &msix_sig);

    if (mask_en) {
        flags_w = flags | PCIE_MSIX_FLAGS_MASKED;
        entry_addr_lo += PCI_MSIX_TBL_MSG_FLAGS;
        __asm {
            pcie[write_pci, flags_w, entry_addr_hi, <<8, entry_addr_lo, 1], \
                sig_done[mask_sig]
        }

        wait_for_all(&msix_sig, &mask_sig);
    } else {
        wait_for_all(&msix_sig);
    }

    ret = 0;

out:
    return ret;
}


/**
 * Send MSI-X interrupt for specified virtual function and optionally mask
 * @param pcie_nr     PCIe island number (0 to 3)
 * @param bar_nr      CPP2PCIe bar to use
 * @param vid         Virtual function number (0 to 63)
 * @param entry_nr    MSI-X table entry number
 * @param mask_en     Boolean, should interrupt be masked after sending.
 * @return            0 on success, else the interrupt was masked.
 *
 * There is no hardware support for MSI-X in VFs so this is
 * implemented entirely in software.  The MSI-X table for each VF is
 * located in its control BAR.  We use a CPP2PCIe BAR for performing
 * the PCI write to generate a MSI-X.  We cache its config so we don't
 * need to re-program the CPP2PCIe BAR for every MSI-X.
 *
 * For a given PCI Island and CPP2PCIe BAR this function is only safe
 * to be called from within a single context. If multiple contexts (or
 * MEs) are used to send MSI-X interrupts from the same PCIe Island
 * they must use separate CPP2PCIe BARs.
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
msix_vf_send(unsigned int pcie_nr, unsigned int bar_nr, unsigned int vid,
             unsigned int entry_nr, unsigned int mask_en)
{
    unsigned int addr_hi;
    unsigned int addr_lo;
    unsigned int data;
    unsigned int flags;
    unsigned int bar_addr;

    __emem char *msix_table_addr;
    __xread uint32_t tmp[PCI_MSIX_TBL_ENTRY_SZ32];

    __xwrite uint32_t msix_data;
    __xwrite uint32_t flags_w;

    SIGNAL msix_sig, mask_sig;

    int ret = 1;

    msix_table_addr =
        (__emem char *)NFD_CFG_BAR(msix_cfg_bars[pcie_nr], vid);
    msix_table_addr += NFD_VF_MSIX_TABLE_OFF;

    /* Read the full table entry */
    mem_read8(tmp,
              msix_table_addr + PCI_MSIX_TBL_ENTRY_OFF(entry_nr), sizeof(tmp));
    addr_lo = tmp[PCI_MSIX_TBL_MSG_ADDR_LO_IDX32];
    addr_hi = tmp[PCI_MSIX_TBL_MSG_ADDR_HI_IDX32];
    data =    tmp[PCI_MSIX_TBL_MSG_DATA_IDX32];
    flags =   tmp[PCI_MSIX_TBL_MSG_FLAGS_IDX32];

    /* If masked, we are done */
    if (flags & PCIE_MSIX_FLAGS_MASKED)
        goto out;

    /* Check if we need to re-configure the CPP2PCI BAR */
    /* XXX vid is equal to VF num currently */
    bar_addr = pcie_c2p_barcfg_val(addr_hi, addr_lo, (vid + 64));
    if (bar_addr != msix_cur_cpp2pci_addr[pcie_nr]) {
        pcie_c2p_barcfg_set_expl(pcie_nr, bar_nr, bar_addr);
        msix_cur_cpp2pci_addr[pcie_nr] = bar_addr;
    }

    /* Send the MSI-X and automask.  We overlap the commands so that
     * they happen roughly at the same time. */
    msix_data = data;
    __pcie_write(&msix_data, pcie_nr, bar_nr, addr_hi, addr_lo,
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
