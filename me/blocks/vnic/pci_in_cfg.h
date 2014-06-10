/*
 * Copyright (C) 2012-2013 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in_cfg.h
 * @brief         PCI.IN compile time configuration options
 */
#ifndef _BLOCKS__VNIC_PCI_IN_CFG_H_
#define _BLOCKS__VNIC_PCI_IN_CFG_H_

#define MAX_TX_QUEUES       64

#define MAX_TX_BATCH_SZ     4
#define DESC_BATCH_Q_SZ     128
#define TX_ISSUED_RING_SZ   128
#define TX_ISSUED_RING_NUM  15      /* XXX use generic resource allocation */

#ifndef NFD_WQ_SZ
#define NFD_WQ_SZ           (16 * 1024)
#endif

#define NFD_NUM_WQS         8

/* XXX use generic resource allocation */
#define NFD_WQS_NUM_PCIE0   32
#define NFD_WQS_NUM_PCIE1   40
#define NFD_WQS_NUM_PCIE2   48
#define NFD_WQS_NUM_PCIE3   56

#define TXQ_EVENT_START     0
#define TXQ_START           0
#define TXQ_EVENT_DATA      (1<<4)

/* Additional check queue constants */
#define TX_MAX_RETRIES      5
#define TX_BATCH_SZ         4
#define TX_PENDING_TEST     0

/* DMAConfigReg index allocations */
#define TX_DATA_CFG_REG     0
#define TX_GATHER_CFG_REG   1

/* DMA defines */
#define TX_GATHER_MAX_IN_FLIGHT 16
#define TX_DATA_MAX_IN_FLIGHT   32

#define TX_GATHER_EVENT_TYPE    5
#define TX_DATA_EVENT_TYPE      6
/* #define RX_FL_FETCH_EVENT_TYPE  13 */
/* #define RX_DATA_EVENT_TYPE      14 */

#define TX_GATHER_EVENT_FILTER          9
#define TX_DATA_EVENT_FILTER            10
/* #define RX_FL_FETCH_EVENT_FILTER        11 */
/* #define RX_DATA_EVENT_FILTER            12 */

#define VNIC_CFG_PCI_OUT(_x)    __nfp_idstr2meid("pcie##_x##.me0")

#endif /* !_BLOCKS__VNIC_PCI_IN_CFG_H_ */
