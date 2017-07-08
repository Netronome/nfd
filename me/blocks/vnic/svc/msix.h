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
 * @file   msix.h
 * @brief  MSI-X library
 */

#ifndef _BLOCKS__VNIC_SVC_MSIX_H_
#define _BLOCKS__VNIC_SVC_MSIX_H_

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

#define PCIE_MSIX_MAX_ISL               4

/*
 * Offset of the MSI-X table in the VF control BAR
 */
#define NFD_VF_MSIX_TABLE_OFF   0x2000


#if defined(__NFP_LANG_MICROC)

/* Declare array of NFD CFG BAR pointers and a global variable to
 * cache the current CPP 2 PCIe BAR */
#define MSIX_DECLARE                                                    \
    __shared __lmem volatile uint64_t msix_cfg_bars[PCIE_MSIX_MAX_ISL];


/* Initialise the value of msix_cfg_bars[] for the specified island.
 * This macro must be called for each PCIe island in use, and
 * NFD_CFG_BASE_DECLARE(_isl) must have been called previously
 * within the compilation block. */
#define MSIX_INIT_ISL_IND(_isl)                                 \
do {                                                            \
    msix_cfg_bars[_isl] = (uint64_t)NFD_CFG_BASE_LINK(_isl);    \
} while (0)

#define MSIX_INIT_ISL(_isl) MSIX_INIT_ISL_IND(_isl)


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
__intrinsic int msix_pf_send(unsigned int pcie_nr, unsigned int bar_nr,
                             unsigned int entry_nr, unsigned int mask_en);


/**
 * Send MSI-X interrupt for specified virtual function and optionally mask
 * @param pcie_nr     PCIe island number (0 to 3)
 * @param bar_nr      CPP2PCIe bar to use
 * @param vf_nr       Virtual function number (0 to 63)
 * @param entry_nr    MSI-X table entry number
 * @param mask_en     Boolean, should interrupt be masked after sending.
 * @return            0 on success, else the interrupt was masked.
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
__intrinsic int msix_vf_send(unsigned int pcie_nr, unsigned int bar_nr,
                             unsigned int vf_nr, unsigned int entry_nr,
                             unsigned int mask_en);

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_SVC_MSIX_H_ */
