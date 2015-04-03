/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc_status.h
 * @brief         Display the state of the cache_desc block
 */
#ifndef _BLOCKS__VNIC_PCI_IN_CACHE_DESC_STATUS_H_
#define _BLOCKS__VNIC_PCI_IN_CACHE_DESC_STATUS_H_

#define STATUS_Q_CACHE_START    8
#define STATUS_Q_STAGE_START    16
#define STATUS_Q_INFO_START     24
#define STATUS_Q_SEL_START      31


struct nfd_out_cache_desc_status {
    unsigned int active_bmsk_hi;
    unsigned int active_bmsk_lo;
    unsigned int urgent_bmsk_hi;
    unsigned int urgent_bmsk_lo;
    unsigned int fl_cache_issued;
    unsigned int fl_cache_compl;
    unsigned int fl_cache_served;
    unsigned int spare;
};

struct nfd_out_stage_batch_status {
    unsigned int batch_issued;
    unsigned int batch_safe;
    unsigned int data_dma_compl;
    unsigned int desc_batch_served;
    unsigned int desc_dma_issued;
    unsigned int desc_dma_compl;
    unsigned int desc_dma_safe;
    unsigned int desc_batch_compl;
};


#if defined (__NFP_LANG_MICROC)

extern void cache_desc_status_setup();

extern void cache_desc_status();

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_PCI_OUT_CACHE_DESC_STATUS_H_ */

