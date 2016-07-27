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
 * @file          blocks/vnic/pci_in/issue_dma_status.h
 * @brief         Display the state of the issue_dma blocks
 */
#ifndef _BLOCKS__VNIC_PCI_IN_ISSUE_DMA_STATUS_H_
#define _BLOCKS__VNIC_PCI_IN_ISSUE_DMA_STATUS_H_

#define STATUS_ISSUE_DMA_START   24
#define STATUS_QUEUE_START1      16 /* XXX avoid name clash, add rtsym rather */
#define STATUS_Q_SEL_START1      63 /* XXX avoid name clash, add rtsym rather */

struct nfd_in_issue_dma_status {
    unsigned int gather_dma_seq_compl;
    unsigned int gather_dma_seq_serv;
    unsigned int bufs_avail;
    unsigned int resv_dma_avail;
    unsigned int data_dma_seq_issued;
    unsigned int data_dma_seq_compl;
    unsigned int data_dma_seq_served;
    unsigned int data_dma_seq_safe;
    unsigned int jumbo_cnt;
    unsigned int spare0;
    unsigned int spare1;
    unsigned int spare2;
};


#if defined (__NFP_LANG_MICROC)

/**
 * Initialise the issue_dma status output
 */
extern void issue_dma_status_setup();

/**
 * Show issue_dma status in transfer registers
 */
extern void issue_dma_status();

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_PCI_IN_ISSUE_DMA_STATUS_H_ */
