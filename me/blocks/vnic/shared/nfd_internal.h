/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_internal.h
 * @brief         An API to manage access to NFD configuration data
 */
#ifndef _BLOCKS__SHARED_NFD_INTERNAL_H_
#define _BLOCKS__SHARED_NFD_INTERNAL_H_

#include <vnic/shared/nfcc_chipres.h>


/* Tuning constants */
/* nfd_cfg */
#define NFD_CFG_QUEUE           1
#define NFD_CFG_EVENT_DATA      (2<<4)
#define NFD_CFG_EVENT_FILTER    14
#define NFD_CFG_RING_SZ         (4 * 512)
#define NFD_CFG_VF_OFFSET       64


/* nfd_in */
#define NFD_IN_MAX_BATCH_SZ     4
#define NFD_IN_DESC_BATCH_Q_SZ  128
#define NFD_IN_ISSUED_RING_SZ   128
#define NFD_IN_ISSUED_RING_RES  32
#define NFD_IN_ISSUED_RING_NUM  15

#define NFD_IN_BUF_STORE_SZ     64
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
#define NFD_IN_DATA_ROUND           4

#define NFD_IN_GATHER_EVENT_TYPE    5
#define NFD_IN_DATA_EVENT_TYPE      6
#define NFD_IN_DATA_IGN_EVENT_TYPE  7

#define NFD_IN_GATHER_EVENT_FILTER  9
#define NFD_IN_DATA_EVENT_FILTER    10

/* Debug defines */
#define NFD_IN_DBG_GATHER_INTVL     1000000
#define NFD_IN_DBG_ISSUE_DMA_INTVL  1000000



/* Helper macros */
/* XXX can provide an extra _pool parameter here if required */
#define NFD_BLM_Q_ALLOC_IND(_name)                  \
    _alloc_resource(_name BLQ_EMU_RINGS global 1)
#define NFD_BLM_Q_ALLOC(_name) NFD_BLM_Q_ALLOC_IND(_name)

#define NFD_RING_BASE_IND(_isl, _comp)   _comp##_ring_isl##_isl
#define NFD_RING_BASE(_isl, _comp)       NFD_RING_BASE_IND(_isl, _comp)

#define NFD_RING_DECLARE_IND1(_isl, _emem, _comp, _sz)                  \
    __export __emem_n(_emem) __align(_sz)                               \
    unsigned char NFD_RING_BASE(_isl, _comp)##[_sz]
#define NFD_RING_DECLARE_IND0(_isl, _comp, _sz)                         \
    NFD_RING_DECLARE_IND1(_isl, NFD_PCIE##_isl##_EMEM, _comp, _sz)
#define NFD_RING_DECLARE(_isl, _comp, _sz)                              \
    NFD_RING_DECLARE_IND0(_isl, _comp, _sz)


/* Check for consistency of defines */
#if defined NFD_VNIC_PF && defined NFD_VNIC_VF
#error "Incompatible defines: NFD_VNIC_PF and NFD_VNIC_VF both set"
#endif

#if !defined NFD_VNIC_PF && !defined NFD_VNIC_VF
#error "Incompatible defines: Neither NFD_VNIC_PF nor NFD_VNIC_VF set"
#endif


/* nfd_cfg internal structures */
/* XXX uncomment */
/* enum nfd_cfg_component { */
/*     NFD_CFG_PCI_IN, */
/*     NFD_CFG_PCI_OUT */
/* }; */


/* nfd_in internal structures */
struct nfd_in_queue_info {
    unsigned int tx_w;
    unsigned int tx_s;
    unsigned int ring_sz_msk;
    unsigned int requester_id;
    unsigned int spare0:24;
    unsigned int ring_base_hi:8;
    unsigned int ring_base_lo;
    unsigned int dummy[2];
};


struct nfd_in_dma_state {
    unsigned int sp0:24;
    unsigned int rid:8;
    unsigned int cont:1;
    unsigned int sp1:2;
    unsigned int curr_buf:29;
    unsigned int offset;
    unsigned int sp2;
};


struct nfd_in_batch_desc {
    union {
        struct {
            unsigned int spare1:8;
            unsigned int spare2:8;
            unsigned int num:8;
            unsigned int queue:8;
        };
        unsigned int __raw;
    };
};


/**
 * PCI.in issued desc format
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+-+-----------+-------------------------+-----+---------------+
 *    0  |E|S|   q_num   |           sp1           |dst_q|   num_batch   |
 *       +-+-+-----------+-------------------------+-----+---------------+
 *    1  |                           buf_addr                            |
 *       +-+-------------+-------+-------+-------------------------------+
 *    2  |0|  offset     |  sp2  | flags |            data_len           |
 *       +-+-------------+-------+-------+-------------------------------+
 *    3  |             VLAN              |               sp3             |
 *       +-------------------------------+-------------------------------+
 *
 *      sp0 - sp3 -> spare
 *      E -> End of packet
 */
struct nfd_in_issued_desc {
    union {
        struct {
            unsigned int eop:1;
            unsigned int sp0:1;
            unsigned int q_num:6;
            unsigned int sp1:13;
            unsigned int dst_q:3;
            unsigned int num_batch:8;
            unsigned int buf_addr:32;
            unsigned int valid:1;
            unsigned int offset:7;
            unsigned int sp2:4;
            unsigned int flags:4;
            unsigned int data_len:16;
            unsigned int vlan:16;
            unsigned int sp3:16;
        };
        unsigned int __raw[4];
    };
};


struct nfd_in_me1_state {
    unsigned int pending_fetch:1;
    unsigned int recompute_seq_safe:1;
    unsigned int spare:30;
};


/* NB: this struct must be compatible with nfd_cfg_msg */
struct pci_in_cfg_msg {
    union {
        struct {
            unsigned int msg_valid:1;
            unsigned int error:1;
            unsigned int spare:22;
            unsigned int vnic:8;
        };
        unsigned int __raw;
    };
};


/* nfd_out internal structures */


#endif /* !_BLOCKS__SHARED_NFD_INTERNAL_H_ */
