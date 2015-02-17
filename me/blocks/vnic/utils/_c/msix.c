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
 * @brief  MSI/MSI-X library 
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
#include <nfp6000/nfp_pcie.h>
#include <pcie.h>

#define PCIE_XPB_TARGET_ID_COMP_CFG_REGS	(0x10) 
#define PCIE_XPB_TARGET_ID_PF_REGS      	(0x20)
#define PCIE_XPB_TARGET_ID_RC_REGS      	(0x21)
#define PCIE_XPB_TARGET_ID_LM_REGS      	(0x22)
#define PCIE_XPB_TARGET_ID_VF_REGS      	(0x23)

// !!! COPIED from /opt/netronome/include/nfp6000/nfp_pcie.h
//#define NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_PEND          0x000000a4
#define NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MASK             0x000000a0
#define NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MSG_LOW_ADDR  	0x00000094
#define NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MSG_HI_ADDR      0x00000098
#define NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MSG_DATA      	0x0000009c

/*

  temporary here

*/

__intrinsic unsigned int
pcie_c2p_barcfg_addr(unsigned int addr_hi, unsigned int addr_lo, unsigned int req_id)
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
 */
enum pcie_cpp2pci_bar {
    PCIE_CPP2PCI_MSIX = 0,
    PCIE_CPP2PCI_MSI,
    PCIE_CPP2PCI_FREE2,
    PCIE_CPP2PCI_FREE3,
    PCIE_CPP2PCI_FREE4,
    PCIE_CPP2PCI_FREE5,
    PCIE_CPP2PCI_FREE6,
    PCIE_CPP2PCI_FREE7
};

__gpr unsigned int msi_cur_cpp2pci_addr = -1;

/*
 -----------------------------------------------------------------------------
 MSI support
 Includes only support for MSI on VFs
 -----------------------------------------------------------------------------
*/
unsigned int msi_vf_status(unsigned int pcie_nr, unsigned int vf_nr, unsigned int vec_nr) 
{
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xread  uint32_t rdata;

    SIGNAL r_sig;

    // configure address to access PCIe internal PF Registers
    addr_hi = 0;

    addr_lo = (PCIE_XPB_TARGET_ID_VF_REGS << 16) +
              NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MASK +
              (vf_nr << 12);
    addr_lo |= (pcie_nr << 24);
    
    __asm ct[xpb_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[r_sig]

    return ((rdata >> vec_nr) & 1);

}

/**
  * Sets the per vf, per-vector mask bit 
  * @param pcie_nr   the PCIe cluster number
  * @param vec_nr    interrupt vector number
  */
void msi_vf_mask(unsigned int pcie_nr, unsigned int vf_nr, unsigned int vec_nr)
{
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xread  uint32_t rdata;
    __xwrite uint32_t wdata;

    SIGNAL rw_sig;

    // configure address to access PCIe internal PF Registers
    addr_hi = 0;

    addr_lo = (PCIE_XPB_TARGET_ID_VF_REGS << 16) +
              NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MASK +
              (vf_nr << 12);
    addr_lo |= (pcie_nr << 24);


    __asm ct[xpb_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]

    // mask the vector
    wdata = rdata | (1 << vec_nr);

    // write back
    __asm ct[xpb_write, wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]

}


#ifdef MSI_USE_HW
void msi_vf_send(unsigned int pcie_nr, unsigned int vf_nr,  unsigned int vec_nr, unsigned int mask_en)
{

    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xwrite uint32_t msi_wdata;

    SIGNAL msi_sig;

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;

    // MSI Address is 0x60040+fn_number
    addr_lo = 0x60040 + vf_nr;

    msi_wdata = vec_nr;
    __asm pcie[write_pci, msi_wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[msi_sig]

    if (mask_en==0) {
       return;
    }

    msi_vf_mask(pcie_nr, vf_nr, vec_nr);

}

#else
void msi_vf_send(unsigned int pcie_nr, unsigned int vf_nr,  unsigned int vec_nr, unsigned int mask_en)
{

    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xwrite uint32_t msi_wdata;
    __xread  uint32_t rdata;

    __gpr unsigned int msi_addr_hi;
    __gpr unsigned int msi_addr_lo;
    __gpr unsigned int msi_data;

    __gpr unsigned int bar_addr;

    SIGNAL msi_sig;

    // configure address to access PCIe internal PF Registers
    //addr_hi = pcie_nr << 30;
    addr_hi = 0;

    // read MSI low address
    addr_lo = (pcie_nr << 24) +
              (PCIE_XPB_TARGET_ID_VF_REGS << 16) +
	      NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MSG_LOW_ADDR +
              (vf_nr << 12);
    
    __asm ct[xpb_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[msi_sig]
    msi_addr_lo = rdata;


    // read MSI high address
    addr_lo = (pcie_nr << 24) +
              (PCIE_XPB_TARGET_ID_VF_REGS << 16) +
	      NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MSG_HI_ADDR +
              (vf_nr << 12);
   
    __asm ct[xpb_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[msi_sig]
    msi_addr_hi = rdata;

    // read MSI data
    addr_lo = (pcie_nr << 24) +
              (PCIE_XPB_TARGET_ID_VF_REGS << 16) +
              NFP_PCIEX_VF_i_vf_MSI_cap_struct_I_MSI_MSG_DATA +
              (vf_nr << 12);
   
    __asm ct[xpb_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[msi_sig]
    msi_data = rdata;


    /* Check if we need to re-configure the CPP2PCI BAR */
    bar_addr = pcie_c2p_barcfg_addr(msi_addr_hi, msi_addr_lo, (vf_nr + 64));
    if (bar_addr != msi_cur_cpp2pci_addr) {
        pcie_c2p_barcfg(pcie_nr, PCIE_CPP2PCI_MSI, msi_addr_hi, msi_addr_lo, (vf_nr + 64));
        msi_cur_cpp2pci_addr = bar_addr;
    }


    /* Send the MSI-X and automask.  We overlap the commands so that
     * they happen roughly at the same time. */
    msi_wdata = msi_data + vec_nr;
    pcie_write(&msi_wdata, pcie_nr, PCIE_CPP2PCI_MSI, msi_addr_hi, msi_addr_lo, sizeof(msi_wdata));

    if (mask_en==0) {
       return;
    }

    msi_vf_mask(pcie_nr, vf_nr, vec_nr);
}
#endif


/*
 -----------------------------------------------------------------------------
 MSI-X support
 NOTE API Currently only supports MSI-X on PF
 -----------------------------------------------------------------------------
*/
unsigned int msix_status(unsigned int pcie_nr, unsigned int vec_nr) 
{
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xread uint32_t rdata;

    SIGNAL r_sig;

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;
    addr_lo = vec_nr << 4; // 4 32-bit words/table entry
    addr_lo = addr_lo | 0xc;  // vector mask bit is bit 96 (4th byte)

    // vector mask bit is in bit 96
    __asm pcie[read_pci, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[r_sig]
   
    return (rdata & 1);

}

/**
  * Sets the per-vector mask bit in the MSI-X vector table located
  * in PCIe SRAM
  * @param pcie_nr   the PCIe cluster number
  * @param vec_nr    interrupt vector number
  */
void msix_mask(unsigned int pcie_nr, unsigned int vec_nr)
{
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;

    __xread  uint32_t rdata;
    __xwrite uint32_t wdata;

    SIGNAL rw_sig;

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;
    addr_lo = vec_nr << 4; // 4 32-bit words/table entry
    addr_lo = addr_lo | 0xc;  // vector mask bit is bit 96 (4th byte)

    // vector mask bit is in bit 96
    __asm pcie[read_pci, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]
     
    // mask the vector
    wdata = rdata | 1;

    // write back
    __asm pcie[write_pci, wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rw_sig]
 
}

void msix_send(unsigned int pcie_nr, unsigned int vec_nr, unsigned int mask_en)
{
    
    __gpr uint32_t addr_hi;
    __gpr uint32_t addr_lo;

    __xwrite uint32_t msix_wdata;
   
    SIGNAL msix_sig;

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;

    // MSI-X Address is 0x60000
    addr_lo = 0x60000;
 
    msix_wdata = vec_nr;
    __asm pcie[write_pci, msix_wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[msix_sig]
   
    if (mask_en==0) {
       return;
    }

    msix_mask(pcie_nr, vec_nr);

} 

