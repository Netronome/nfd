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
 * @file          
 * @brief         Handle MSI/ MSI-X support
 */


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

#include <nfp.h>
#include <stdint.h>
#include <types.h>
#include <nfp/me.h>
#include <nfp6000/nfp_me.h>

#define PCIE_XPB_TARGET_ID_COMP_CFG_REGS	(0x10) 
#define PCIE_XPB_TARGET_ID_PF_REGS      	(0x20)
#define PCIE_XPB_TARGET_ID_RC_REGS      	(0x21)
#define PCIE_XPB_TARGET_ID_LM_REGS      	(0x22)
#define PCIE_XPB_TARGET_ID_VF_REGS      	(0x23)

// !!! COPIED from /opt/netronome/include/nfp6000/nfp_pcie.h
#define NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_PEND          0x000000a4
#define NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MASK          0x000000a0

/*
 -----------------------------------------------------------------------------
 MSI support
 Includes only support for MSI on VFs
 -----------------------------------------------------------------------------
*/
__gpr uint32_t msi_vf_status(__gpr uint32_t pcie_nr, __gpr uint32_t vf_fn_nr, __gpr uint32_t intrpt_nr) 
{
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xread  uint32_t rdata;

    SIGNAL r_sig;

    // configure address to access PCIe internal PF Registers
    addr_hi = pcie_nr << 30;

    addr_lo = PCIE_XPB_TARGET_ID_VF_REGS << 32 + 
              NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_PEND +
              vf_fn_nr << 12;
    
    __asm ct[xpb_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[r_sig]

    return ((rdata >> intrpt_nr) & 1);

}

void msi_vf_mask(__gpr int32_t pcie_nr, __gpr uint32_t vf_fn_nr, __gpr uint32_t intrpt_nr)
{
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xread  uint32_t rdata;
    __xwrite uint32_t wdata;

    SIGNAL rw_sig;

    // configure address to access PCIe internal PF Registers
    addr_hi = pcie_nr << 30;

    addr_lo = PCIE_XPB_TARGET_ID_VF_REGS << 32 +
              NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MASK +
              vf_fn_nr << 12;

    __asm ct[xpb_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]

    // mask the vector
    wdata = rdata | (1 << intrpt_nr);

    // write back
    __asm ct[xpb_write, wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]

}

void msi_vf_send(__gpr uint32_t pcie_nr, __gpr uint32_t intrpt_nr, __gpr uint32_t vf_fn_nr, __gpr uint32_t mask_intrpt)
{

    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xwrite uint32_t msi_wdata;

    SIGNAL msi_sig;

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;

    // MSI Address is 0x60040+fn_number
    addr_lo = 0x600040 + vf_fn_nr;

    msi_wdata = intrpt_nr;
    __asm pcie[write_pci, msi_wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[msi_sig]

    if (mask_intrpt==0) {
       return;
    }

    msi_vf_mask(pcie_nr, vf_fn_nr, intrpt_nr);

}

/*
 -----------------------------------------------------------------------------
 MSI-X support
 NOTE API Currently only supports MSI-X on PF
 -----------------------------------------------------------------------------
*/
__gpr uint32_t msix_status(__gpr uint32_t pcie_nr, __gpr uint32_t intrpt_nr) 
{
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xread uint32_t rdata;

    SIGNAL r_sig;

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;
    addr_lo = intrpt_nr << 4; // 4 32-bit words/table entry
    addr_lo = addr_lo | 0xc;  // vector mask bit is bit 96 (4th byte)

    // vector mask bit is in bit 96
    __asm pcie[read_pci, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[r_sig]
   
    return (rdata & 1);

}

void msix_mask(__gpr int32_t pcie_nr, __gpr uint32_t intrpt_nr)
{
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xread  uint32_t rdata;
    __xwrite uint32_t wdata;

    SIGNAL rw_sig;

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;
    addr_lo = intrpt_nr << 4; // 4 32-bit words/table entry
    addr_lo = addr_lo | 0xc;  // vector mask bit is bit 96 (4th byte)

    // vector mask bit is in bit 96
    __asm pcie[read_pci, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]
     
    // mask the vector
    wdata = rdata | 1;

    // write back
    __asm pcie[write_pci, wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]
 
}

void msix_un_mask(__gpr int32_t pcie_nr, __gpr uint32_t intrpt_nr)
{
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xread  uint32_t rdata;
    __xwrite uint32_t wdata;

    SIGNAL rw_sig;

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;
    addr_lo = intrpt_nr << 4; // 4 32-bit words/table entry
    addr_lo = addr_lo | 0xc;  // vector mask bit is bit 96 (4th byte)

    // vector mask bit is in bit 96
    __asm pcie[read_pci, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]
     
    // mask the vector
    wdata = rdata & 0xfffffffe;

    // write back
    __asm pcie[write_pci, wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]
 
}

void msix_send(__gpr uint32_t pcie_nr, __gpr uint32_t intrpt_nr, __gpr uint32_t mask_intrpt)
{
    
    __gpr uint32_t addr_hi;
    __gpr uint32_t addr_lo;

    __xwrite uint32_t msix_wdata;
   
    SIGNAL msix_sig;

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;

    // MSI-X Address is 0x60000
    addr_lo = 0x60000;
 
    msix_wdata = intrpt_nr;
    __asm pcie[write_pci, msix_wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[msix_sig]
   
    if (mask_intrpt==0) {
       return;
    }

    msix_mask(pcie_nr, intrpt_nr);

} 




