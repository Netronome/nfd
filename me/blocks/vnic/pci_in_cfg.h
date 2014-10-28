/*
 * Copyright (C) 2012-2013 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in_cfg.h
 * @brief         PCI.IN compile time configuration options
 */
#ifndef _BLOCKS__VNIC_PCI_IN_CFG_H_
#define _BLOCKS__VNIC_PCI_IN_CFG_H_

#define NFD_IN_MAX_QUEUES   64

#define NFD_IN_PCIE0_WQ_EMEM    0
#define NFD_IN_PCIE1_WQ_EMEM    0
#define NFD_IN_PCIE2_WQ_EMEM    1
#define NFD_IN_PCIE3_WQ_EMEM    1

#define NFD_IN_MAX_BATCH_SZ     4
#define NFD_IN_DESC_BATCH_Q_SZ  128
#define NFD_IN_ISSUED_RING_SZ   128
#define NFD_IN_ISSUED_RING_RES  32
#define NFD_IN_ISSUED_RING_NUM  15


#define NFD_IN_BUF_STORE_SZ     64


/* XXX use BLM generic resource allocation */
/* XXX expose to the APP */
#define NFD_IN_BLM_BLS          0
#define NFD_IN_BLM_POOL         BLM_NBI8_BLQ0_EMU_QID
#define NFD_IN_BLM_RADDR        __LoadTimeConstant("__addr_emem1")
#define NFD_IN_BUF_RECACHE_WM   16

#define NFD_IN_Q_EVENT_START    0
#define NFD_IN_Q_START          0
#define NFD_IN_Q_EVENT_DATA     (1<<4)

#define NFD_IN_ISSUE_START_CTX  1

/* Additional check queue constants */
#define NFD_IN_MAX_RETRIES      5
#define NFD_IN_BATCH_SZ         4
#define NFD_IN_PENDING_TEST     0

/* DMAConfigReg index allocations */
#define NFD_IN_GATHER_CFG_REG   0
#define NFD_IN_DATA_CFG_REG     2


/* DMA defines */
#define NFD_IN_GATHER_MAX_IN_FLIGHT 16
#define NFD_IN_DATA_MAX_IN_FLIGHT   32
#define NFD_IN_GATHER_DMA_QUEUE     NFP_PCIE_DMA_FROMPCI_HI
#define NFD_IN_DATA_DMA_QUEUE       NFP_PCIE_DMA_FROMPCI_LO
#define NFD_IN_DATA_DMA_TOKEN       2
#define NFD_IN_DATA_OFFSET          64
#define NFD_IN_DATA_ROUND           4

#define NFD_IN_GATHER_EVENT_TYPE    5
#define NFD_IN_DATA_EVENT_TYPE      6
#define NFD_IN_DATA_IGN_EVENT_TYPE  7

#define NFD_IN_GATHER_EVENT_FILTER  9
#define NFD_IN_DATA_EVENT_FILTER    10

/* XXX move */
#define VNIC_CFG_PCI_OUT(_x)    __nfp_idstr2meid("pcie##_x##.me0")


/* Debug defines */
#define NFD_IN_DBG_GATHER_INTVL     1000000
#define NFD_IN_DBG_ISSUE_DMA_INTVL  1000000

#endif /* !_BLOCKS__VNIC_PCI_IN_CFG_H_ */
