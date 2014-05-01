/*
 * Copyright (C) 2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/utils/pcie.h
 * @brief         NFP PCIe interface
 */

#ifndef _BLOCKS__VNIC_UTILS_PCIE_H_
#define _BLOCKS__VNIC_UTILS_PCIE_H_

#include <nfp6000/nfp_pcie.h>

struct nfp_pcie_dma_cfg_one {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved:3;
            unsigned int signal_only:1;
            unsigned int end_pad:2;
            unsigned int start_pad:2;
            unsigned int id_based_order:1;
            unsigned int relaxed_order:1;
            unsigned int no_snoop:1;
            unsigned int target_64:1;
            unsigned int cpp_target:4;
#           else
            unsigned int cpp_target:4;
            unsigned int target_64:1;
            unsigned int no_snoop:1;
            unsigned int relaxed_order:1;
            unsigned int id_based_order:1;
            unsigned int start_pad:2;
            unsigned int end_pad:2;
            unsigned int signal_only:1;
            unsigned int __reserved:3;
#           endif
        };
        unsigned short __raw;
    };
};

/**
 * Configure DMADescrConfig for one index
 * @param pcie_isl          PCIe island (0-3) to address
 * @param index             4 bit index for the register to configure,
 *                          as per the DMA descriptor DmaConfigReg Index
 * @param new_cfg           Configuration to apply
 *
 * This method accepts an index in the same format as the DMA descriptor, and
 * applies the given configuration to that index only.  A read-modify-write is
 * performed to ensure the other configuration in the pair is unchanged.  It is
 * the caller's responsibility to ensure that calls for neighbours in a pair
 * do not overlap.
 */
__intrinsic void set_DMA_config(unsigned char pcie_isl, unsigned int index,
                                struct nfp_pcie_dma_cfg_one new_cfg);

/**
 * Configure a pair of DMADescrConfig indices
 * @param pcie_isl          PCIe island (0-3) to address
 * @param index             4 bit index for one of the registers to configure,
 *                          as per the DMA descriptor DmaConfigReg Index
 * @param new_cfg           configuration to apply
 * @param sync              type of synchronization
 * @param sig               signal to use
 *
 * This method accepts an index in the same format as the DMA descriptor, and
 * applies the configuration to that index and it's pair.  sig_done and
 * ctx_swap sync options are supported.
 */
__intrinsic void __set_DMA_config_pair(
    unsigned char pcie_isl, unsigned int index,
    __xwrite struct nfp_pcie_dma_cfg *new_cfg, sync_t sync, SIGNAL *sig);

__intrinsic void set_DMA_config_pair(unsigned char pcie_isl, unsigned int index,
                                     __xwrite struct nfp_pcie_dma_cfg *new_cfg);

/**
 * Enqueue a DMA descriptor
 * @param pcie_isl          PCIe island (0-3) to address
 * @param cmd               DMA command to send.
 * @param queue             Queue to use, e.g. NFP_PCIE_DMA_TOPCI_HI,
 *                          see nfp_pcie.h
 */
__intrinsic void __pcie_dma_enq(unsigned char pcie_isl,
                                __xwrite struct nfp_pcie_dma_cmd *cmd,
                                unsigned int queue, sync_t sync, SIGNAL *sig);

__intrinsic void pcie_dma_enq(unsigned char pcie_isl,
                              __xwrite struct nfp_pcie_dma_cmd *cmd,
                              unsigned int queue);

#endif /* _BLOCKS__UTILS_PCIE_H_ */

