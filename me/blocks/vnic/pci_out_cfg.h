/*
 * Copyright (C) 2012-2013 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out_cfg.h
 * @brief         PCI.IN compile time configuration options
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_CFG_H_
#define _BLOCKS__VNIC_PCI_OUT_CFG_H_

#define MAX_RX_QUEUES       64

#define RXQ_EVENT_START     4
#define RXQ_START           128
#define RXQ_EVENT_DATA      (3<<4)

/* Additional check queue constants */
#define RX_MAX_RETRIES      5
#define RX_FL_BATCH_SZ      8   /* Match configured watermark! */
#define RX_PENDING_TEST     (RX_BATCH_SZ - 1)

/* DMAConfigReg index allocations */
#define RX_FL_CFG_REG       4
#define RX_DATA_CFG_REG     6

/* DMA defines */
#define RX_FL_FETCH_MAX_IN_FLIGHT       16
#define RX_DATA_MAX_IN_FLIGHT           32
#define RX_FL_FETCH_DMA_QUEUE           NFP_PCIE_DMA_FROMPCI_HI
#define RX_DATA_DMA_QUEUE               NFP_PCIE_DMA_FROMPCI_LO
#define RX_DATA_DMA_TOKEN               2

#define RX_FL_CACHE_BUFS_PER_QUEUE      256

#define RX_FL_FETCH_EVENT_TYPE          13
#define RX_DATA_EVENT_TYPE              14

#define RX_FL_FETCH_EVENT_FILTER        11
#define RX_DATA_EVENT_FILTER            12



#endif /* !_BLOCKS__VNIC_PCI_OUT_CFG_H_ */
