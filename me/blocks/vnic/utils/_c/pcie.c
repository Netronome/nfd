/*
 * Copyright (C) 2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/utils/_c/pcie.c
 * @brief         NFP PCIe interface
 */

#include <assert.h>
#include <nfp.h>

#include <nfp6000/nfp_pcie.h>

#include <vnic/utils/pcie.h>

struct DMA_cfg_word_access {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned short odd;
            unsigned short even;
#           else
            unsigned short even;
            unsigned short odd;

#           endif
        };
        unsigned int __raw;
    };
};

__intrinsic void
set_DMA_config(unsigned char pcie_isl, unsigned int index,
               struct nfp_pcie_dma_cfg_one new_cfg)
{
    __xrw unsigned int cfg;
    unsigned int count = ((sizeof cfg) >> 2);
    __gpr struct DMA_cfg_word_access cfg_tmp;
    SIGNAL sig;

    /* XXX nfp_pcie.h defines not convenient for this purpose */
    unsigned int reg_no = (((index >> 1) & 0x7) << 2);
    unsigned int addr_lo = NFP_PCIE_DMA_CFG0 + reg_no;
    __gpr unsigned int addr_hi = pcie_isl << 30;

    /* Read original configuration */
    __intrinsic_begin();
    __asm pcie[read_pci, cfg, addr_hi, <<8, addr_lo,  __ct_const_val(count)], \
        ctx_swap[sig];
    __intrinsic_end();

    /* Modify the configuration */
    cfg_tmp.__raw = cfg;
    if (index & 1) {
        /* Odd */
        cfg_tmp.odd = new_cfg.__raw;
    } else {
        /* Even */
        cfg_tmp.even = new_cfg.__raw;
    }
    cfg = cfg_tmp.__raw;


    /* Write back new configuration */
    __intrinsic_begin();
    __asm pcie[write_pci, cfg, addr_hi, <<8, addr_lo,  __ct_const_val(count)],\
        ctx_swap[sig];
    __intrinsic_end();
}

__intrinsic void
__set_DMA_config_pair(unsigned char pcie_isl, unsigned int index,
                      __xwrite struct nfp_pcie_dma_cfg *new_cfg, sync_t sync,
                      SIGNAL *sig)
{
   unsigned int count = (sizeof(struct nfp_pcie_dma_cfg) >> 2);
    /* XXX nfp_pcie.h defines not convenient for this purpose */
    unsigned int reg_no = (((index >> 1) & 0x7) << 2);
    unsigned int addr_lo = NFP_PCIE_DMA_CFG0 + reg_no;
    __gpr unsigned int addr_hi = pcie_isl << 30;

    __intrinsic_begin();
    if (sync == ctx_swap) {
        __asm pcie[write_pci, *new_cfg, addr_hi, <<8, addr_lo,  \
                   __ct_const_val(count)], ctx_swap[*sig];
    } else {
        __asm pcie[write_pci, *new_cfg, addr_hi, <<8, addr_lo,  \
                   __ct_const_val(count)], sig_done[*sig];
    }
    __intrinsic_end();
}

__intrinsic void
set_DMA_config_pair(unsigned char pcie_isl, unsigned int index,
                    __xwrite struct nfp_pcie_dma_cfg *new_cfg)
{
    SIGNAL sig;

    __set_DMA_config_pair(pcie_isl, index, new_cfg, ctx_swap, &sig);
}

__intrinsic void __pcie_dma_enq(unsigned char pcie_isl,
                                  __xwrite struct nfp_pcie_dma_cmd *cmd,
                                  unsigned int queue,
                                  sync_t sync, SIGNAL *sig)
{
    unsigned int count = (sizeof(struct nfp_pcie_dma_cmd) >> 2);
    __gpr unsigned int addr_hi = pcie_isl << 30;

    ctassert(__is_write_reg(cmd));
    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    __intrinsic_begin();
    if (sync == ctx_swap) {
        __asm pcie[write_pci, *cmd, addr_hi, <<8, queue, \
                   __ct_const_val(count)], ctx_swap[*sig];
    } else {
        __asm pcie[write_pci, *cmd, addr_hi, <<8, queue, \
                   __ct_const_val(count)], sig_done[*sig];
    }
    __intrinsic_end();
}

__intrinsic void
pcie_dma_enq(unsigned char pcie_isl, __xwrite struct nfp_pcie_dma_cmd *cmd,
             unsigned int queue)
{
    SIGNAL sig;
    __pcie_dma_enq(pcie_isl, cmd, queue, ctx_swap, &sig);
}
