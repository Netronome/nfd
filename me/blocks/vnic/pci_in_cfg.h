/*
 * Copyright (C) 2012-2013 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in_cfg.h
 * @brief         PCI.IN compile time configuration options
 */
#ifndef _BLOCKS__VNIC_PCI_IN_CFG_H_
#define _BLOCKS__VNIC_PCI_IN_CFG_H_

#define MAX_TX_QUEUES       64

#define TX_PCIE0_WQ_EMEM    0
#define TX_PCIE1_WQ_EMEM    0
#define TX_PCIE2_WQ_EMEM    1
#define TX_PCIE3_WQ_EMEM    1

#define MAX_TX_BATCH_SZ     4
#define DESC_BATCH_Q_SZ     128
#define TX_ISSUED_RING_SZ   128
#define TX_ISSUED_RING_RES  32
#define TX_ISSUED_RING_NUM  15      /* XXX use generic resource allocation */


#define TX_BUF_STORE_SZ     64


/* XXX use BLM generic resource allocation */
#define TX_BLM_POOL         75
#define TX_BLM_RADDR        __LoadTimeConstant("__addr_emem1")
#define TX_BUF_RECACHE_WM   16

#define TXQ_EVENT_START     0
#define TXQ_START           0
#define TXQ_EVENT_DATA      (1<<4)

#define TX_ISSUE_START_CTX  1

/* Additional check queue constants */
#define TX_MAX_RETRIES      5
#define TX_BATCH_SZ         4
#define TX_PENDING_TEST     0

/* DMAConfigReg index allocations */
#define TX_GATHER_CFG_REG   0
#define TX_DATA_CFG_REG     2


/* DMA defines */
#define TX_GATHER_MAX_IN_FLIGHT 16
#define TX_DATA_MAX_IN_FLIGHT   32
#define TX_GATHER_DMA_QUEUE     NFP_PCIE_DMA_FROMPCI_HI
#define TX_DATA_DMA_QUEUE       NFP_PCIE_DMA_FROMPCI_LO
#define TX_DATA_DMA_TOKEN       2
#define TX_DATA_OFFSET          64

#define TX_GATHER_EVENT_TYPE    5
#define TX_DATA_EVENT_TYPE      6
#define TX_DATA_IGN_EVENT_TYPE  7

#define TX_GATHER_EVENT_FILTER          9
#define TX_DATA_EVENT_FILTER            10

#define VNIC_CFG_PCI_OUT(_x)    __nfp_idstr2meid("pcie##_x##.me0")

#endif /* !_BLOCKS__VNIC_PCI_IN_CFG_H_ */
