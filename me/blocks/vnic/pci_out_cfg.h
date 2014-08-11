/*
 * Copyright (C) 2012-2013 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out_cfg.h
 * @brief         PCI.IN compile time configuration options
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_CFG_H_
#define _BLOCKS__VNIC_PCI_OUT_CFG_H_

#define MAX_RX_QUEUES       64

#define MAX_RX_BATCH_SZ     4

#define RX_BLM_POOL_START   BLM_NBI8_BLQ0_EMU_QID
#define RX_BLM_RADDR        __LoadTimeConstant("__addr_emem1")

#define RXQ_EVENT_START     4
#define RXQ_START           128
#define RXQ_EVENT_DATA      (3<<4)

/* Additional check queue constants */
#define RX_MAX_RETRIES      5
#define RX_FL_BATCH_SZ      8   /* Match configured watermark! */
#define RX_PENDING_TEST     (RX_BATCH_SZ - 1)

#define RX_STAGE_START_CTX              1
#define RX_STAGE_WAIT_CYCLES            200

/* DMAConfigReg index allocations */
#define RX_FL_CFG_REG                   4
#define RX_DESC_CFG_REG                 8

#define RX_DATA_CFG_REG                 6
#define RX_DATA_CFG_REG_SIG_ONLY        7

/* DMA defines */
#define RX_FL_FETCH_MAX_IN_FLIGHT       16
#define RX_DATA_MAX_IN_FLIGHT           64
#define RX_DESC_MAX_IN_FLIGHT           32
#define RX_FL_FETCH_DMA_QUEUE           NFP_PCIE_DMA_FROMPCI_HI
#define RX_DATA_DMA_QUEUE               NFP_PCIE_DMA_TOPCI_LO
#define RX_DESC_DMA_QUEUE               NFP_PCIE_DMA_TOPCI_MED
#define RX_DATA_DMA_TOKEN               2

#define RX_FL_CACHE_BUFS_PER_QUEUE      256
#define RX_FL_CACHE_SOFT_THRESH         (RX_FL_CACHE_BUFS_PER_QUEUE / 2)

/* XXX Check event type assignments for conflicts with autogenerated events */
#define RX_FL_FETCH_EVENT_TYPE          13
#define RX_DATA_EVENT_TYPE              14
#define RX_DATA_IGN_EVENT_TYPE          7
#define RX_DESC_EVENT_TYPE              9

#define RX_FL_FETCH_EVENT_FILTER        11
#define RX_DATA_EVENT_FILTER            12
#define RX_DESC_EVENT_FILTER            13


/* Ring defines */
/* XXX should _SZ always indicate a byte size? */
#define RX_DESC_BATCH_RING_BAT          32
#define RX_DATA_BATCH_RING_PKTS         256
#define RX_CPP_BATCH_RING_BAT           32
#define RX_DATA_BATCH_RING_NUM  14  /* XXX use generic resource allocation */


#endif /* !_BLOCKS__VNIC_PCI_OUT_CFG_H_ */
