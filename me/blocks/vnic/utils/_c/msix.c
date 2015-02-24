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

#include <assert.h>
#include <nfp.h>
#include <stdint.h>
#include <types.h>
#include <nfp/me.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>
#include <nfp/mem_bulk.h>
#include <vnic/shared/nfd_cfg.h>
//#include <vnic/shared/nfd_cfg_internal.c>
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

__gpr unsigned int msi_cur_cpp2pci_addr  = -1;
__gpr unsigned int msix_cur_cpp2pci_addr = -1;

__emem char* get_cfg_bar_vf_base(unsigned int pcie_nr, unsigned int vf_nr) 
{
    __emem char* cfg_bar_vf_addr;

    switch(pcie_nr-4) {
    case 0:
        cfg_bar_vf_addr = NFD_CFG_BAR_ISL(0, vf_nr);
        break;
    case 1:
        cfg_bar_vf_addr = NFD_CFG_BAR_ISL(1, vf_nr);
        break;
    case 2:
        cfg_bar_vf_addr = NFD_CFG_BAR_ISL(2, vf_nr);
        break;
    case 3:
        cfg_bar_vf_addr = NFD_CFG_BAR_ISL(3, vf_nr);
        break;
    default:
        cfg_bar_vf_addr = NFD_CFG_BAR_ISL(0, vf_nr); // is that the corrrect default to have ?
        break;
    }
   
    return cfg_bar_vf_addr;
     
}

/*
 -----------------------------------------------------------------------------
 MSI support
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
 -----------------------------------------------------------------------------
*/

/**
  * Returns MSI-X mask status for given interrupt vector number
  * @param pcie_nr     - PCIe cluster number
  * @param vec_nr      - interrupt vector number
  * @return            - the status of the interrupt vector
  *                      0 - interrupt not masked
  *                      1 - interrupt masked
  */
unsigned int msix_pf_status(unsigned int pcie_nr, unsigned int vec_nr) 
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
void msix_pf_mask(unsigned int pcie_nr, unsigned int vec_nr)
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

int msix_pf_send(unsigned int pcie_nr, unsigned int vec_nr, unsigned int mask_en)
{
    
    __gpr uint32_t addr_hi;
    __gpr uint32_t addr_lo;
    int tmp;

    __xwrite uint32_t msix_wdata;
   
    SIGNAL msix_sig;
 
    tmp = msix_pf_status(pcie_nr, vec_nr);

    if (tmp != 0) {
        /* vector is masked */
        return -1;
    }

    // calculate SRAM address for given vector definition
    addr_hi = pcie_nr << 30;

    // MSI-X Address is 0x60000
    addr_lo = 0x60000;
 
    msix_wdata = vec_nr;
    __asm pcie[write_pci, msix_wdata, addr_hi, <<8, addr_lo, 1], ctx_swap[msix_sig]
   
    if (mask_en==0) {
       return 0;
    }

    msix_pf_mask(pcie_nr, vec_nr);

    return 0;
} 

//__forceinline static __gpr int
int
msix_vf_send(unsigned int pcie_nr, unsigned int vf_nr, unsigned int vec_nr, unsigned int mask_en)
{
    __gpr unsigned int data;
    __gpr unsigned int addr_hi;
    __gpr unsigned int addr_lo;
    __gpr unsigned int flags;
    __gpr unsigned int bar_addr;
    __emem char* cfg_bar_vf_addr;

    __xread uint32_t tmp[PCI_MSIX_TBL_ENTRY_SZ32];
    __xwrite uint32_t msix_data;
    __xwrite uint32_t mask_data;

    SIGNAL msix_sig, mask_sig;

    cfg_bar_vf_addr = get_cfg_bar_vf_base(pcie_nr, vf_nr);
    mem_read8(tmp, cfg_bar_vf_addr + 0x2000 + (vec_nr*0x10), sizeof(tmp));

    flags =   tmp[PCI_MSIX_TBL_MSG_FLAGS_IDX32];
    data =    tmp[PCI_MSIX_TBL_MSG_DATA_IDX32];
    addr_hi = tmp[PCI_MSIX_TBL_MSG_ADDR_HI_IDX32];
    addr_lo = tmp[PCI_MSIX_TBL_MSG_ADDR_LO_IDX32];

    if (flags != 0) {
	/* vector is masked */
        return -1;
    }

    /* Check if we need to re-configure the CPP2PCI BAR */
    bar_addr = pcie_c2p_barcfg_addr(addr_hi, addr_lo, (vf_nr + 64));
    if (bar_addr != msix_cur_cpp2pci_addr) {
        pcie_c2p_barcfg(pcie_nr, PCIE_CPP2PCI_MSIX, addr_hi, addr_lo, (vf_nr + 64));
        msix_cur_cpp2pci_addr = bar_addr;
    }

    /* Send the MSI-X and automask.  We overlap the commands so that
     * they happen roughly at the same time. */
    msix_data = data;
    __pcie_write(&msix_data, pcie_nr, PCIE_CPP2PCI_MSIX, addr_hi, addr_lo,  
                 sizeof(msix_data), sizeof(msix_data), sig_done, &msix_sig);

    if (mask_en != 0) {
        mask_data = PCIE_MSIX_FLAGS_MASKED;
        __mem_write8(&mask_data, cfg_bar_vf_addr +
                     0x2000 + (vec_nr*0x10) +
                     PCI_MSIX_TBL_MSG_FLAGS,
                     sizeof(mask_data), sizeof(mask_data), sig_done, &mask_sig);

        wait_for_all(&msix_sig, &mask_sig);
    } else{
        wait_for_all(&msix_sig);
    }

    return 0;
}

#if 0

/* NOTE: In ovs-nfp.hg/me/blocks/pcie/svc_msix.c when MSI-X are sent, 
         the following logic is added to check if vector was masked, then
         that information is stored for later:

        ret = msix_vf_send(pcie_nr, vf_nr, vec_nr, mask_en);
        if (ret)
            msix_rx_rings_pending |= (1ull << ring);
        else
            msix_rx_rings_pending &= ~(1ull << ring);


	The above is done for each type of transaction:
        - msix_rx_rings_pending - for issuing RX ints
        - msix_tx_rings_pending - for issuing TX ints
	- msix_lsc_pending - for issuing LSC ints

	Then, there is the function (provided below) for testing any
        pending interrupts and if no longer masked, the interrupt is 
        issued

	My guess is that each PF/VF needs to have copy
*/


#if 0
/* PROVIDED FOR REFERENCE */

/*
 * Process any pending MSI-X, entries which were masked in the past
 *
 * Here we go through the bitmasks for rings which have pending MSI-X,
 * starting with the RX rings.
 *
 * For the RX rings we check if the queue is still non-empty and if it
 * does, attempt to send the MSI-X.  If it succeeded, we clear the
 * pending bit.
 *
 * With @msix_tried and @msix_sent we keep track for which entries we
 * already tried and sent an MSI-X, so that we don't retry for TX
 * rings.  If a given entry is shared between a RX and a TX ring and
 * the entry gets unmasked between processing RX rings and TX rings,
 * we would potentially generate multiple MSI-X when it is not
 * necessary.
 *
 * @msix_tried and @msix_sent are bitmasks, (entry >> 5) is a divide
 * by 32 to select the correct word and (entry & 0x1f) gives us the
 * modulo 32 to select the bit in the entry.
 */
__forceinline static void
svc_msix_process_pending(void)
{
    __gpr uint64_t rings;
    __gpr int ring;
    __gpr int entry;
    __gpr unsigned int status;
    __gpr int ret;

    __lmem uint32_t msix_tried[8];
    __lmem uint32_t msix_sent[8];

    /* If nothing is pending, return */
    if (msix_rx_rings_pending == 0 && msix_tx_rings_pending == 0)
        return;

    reg_zero(msix_tried, sizeof(msix_tried));
    reg_zero(msix_sent, sizeof(msix_sent));

    rings = msix_rx_rings_pending;
    while (rings) {
        ring = ffs64(rings);
        entry = msix_rx_entries[ring];
        rings &= ~(1ull << ring);

        /* If ring is empty, no need to send MSI-X */
        status = qcp_read(PCIE_RXR2QCPQ(ring), NFP_QC_QUEUE_STS_LO);
        if (status & NFP_QC_QUEUE_STS_LO_EMPTY) {
            msix_rx_rings_pending &= ~(1ull << ring);
            continue;
        }

        PCIE_SVC_MSIX_DBG(0xbeef0000 | ring << 8 | entry);

        /* Mark that we tried this entry so that we don't retry for TX later */
        msix_tried[entry >> 5] |= (1 << (entry & 0x1f));

        /* Try sending an MSI-X */
        ret = svc_msix_send(entry);
        if (ret)
            continue;

        /* MSI-X sent. Mark that we did and remove pending bit */
        msix_sent[entry >> 5] |= (1 << (entry & 0x1f));
        msix_rx_rings_pending &= ~(1ull << ring);
    }

    PCIE_SVC_MSIX_DBG(0xbeefffff);
    PCIE_SVC_MSIX_DBG((uint32_t)(msix_rx_rings_pending >> 32));
    PCIE_SVC_MSIX_DBG((uint32_t)msix_rx_rings_pending);

    rings = msix_tx_rings_pending;
    while (rings) {
        ring = ffs64(rings);
        entry = msix_tx_entries[ring];
        rings &= ~(1ull << ring);

        /* Stash the read pointer away for watchdog */
        status = qcp_read(PCIE_TXR2QCPQ(ring), NFP_QC_QUEUE_STS_LO);
        msix_tx_rd_qptrs[ring] = status & NFP_QC_QUEUE_STS_LO_READPTR_mask;

        /* If we already sent a MSI-X, don't do it again */
        if (msix_sent[entry >> 5] & (1 << (entry & 0x1f))) {
            msix_tx_rings_pending &= ~(1ull << ring);
            continue;
        }

        /* If we already tried, don't bother again this time round */
        if (msix_tried[entry >> 5] & (1 << (entry & 0x1f)))
            continue;

        PCIE_SVC_MSIX_DBG(0xb11f0000 | ring << 8 | entry);

        /* Try send MSI-X */
        ret = svc_msix_send(entry);
        if (ret)
            continue;

        /* MSI-X succeed. Remove pending bit */
        msix_tx_rings_pending &= ~(1ull << ring);
    }

    PCIE_SVC_MSIX_DBG(0xbeefeeee);
    PCIE_SVC_MSIX_DBG((uint32_t)(msix_tx_rings_pending >> 32));
    PCIE_SVC_MSIX_DBG((uint32_t)msix_tx_rings_pending);

    if (msix_lsc_pending && (msix_lsc_entry != 0xff)) {
        ret = svc_msix_send(msix_lsc_entry);
        if (ret)
            msix_lsc_pending = 1;
        else
            msix_lsc_pending = 0;
    }
}
#endif

#endif
