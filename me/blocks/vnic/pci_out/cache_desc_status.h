/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/pci_out/cache_desc_status.h
 * @brief         Display the state of the cache_desc block
 */
#ifndef _BLOCKS__VNIC_PCI_IN_CACHE_DESC_STATUS_H_
#define _BLOCKS__VNIC_PCI_IN_CACHE_DESC_STATUS_H_

#define STATUS_Q_CACHE_START    8
#define STATUS_Q_DMA_START      16
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
    unsigned int reserved;
};

struct nfd_out_desc_dma_status {
    unsigned int cached_bmsk_hi;
    unsigned int cached_bmsk_lo;
    unsigned int pending_bmsk_hi;
    unsigned int pending_bmsk_lo;
    unsigned int desc_dma_issued;
    unsigned int desc_dma_compl;
    unsigned int desc_dma_served;
    unsigned int desc_dma_pkts_served;
};


#if defined (__NFP_LANG_MICROC)

extern void cache_desc_status_setup();

extern void cache_desc_status();

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_PCI_OUT_CACHE_DESC_STATUS_H_ */

