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
 * @file          blocks/vnic/shared/nfd_internal.h
 * @brief         An API to manage access to NFD configuration data
 */
#ifndef _BLOCKS__SHARED_NFD_INTERNAL_H_
#define _BLOCKS__SHARED_NFD_INTERNAL_H_

#if (defined(NFD_IN_LSO_CNTR_ENABLE) && defined(__NFP_LANG_MICROC))
#include <std/cntrs.h>
#endif

#if (defined(__NFP_LANG_ASM) || defined(__NFP_LANG_MICROC))
/* Exclude nfd_user_cfg.h for nfd_dbg */
#include <nfd_user_cfg.h>
#endif

/* Tuning constants */

/* NFD event filter defines */

#define NFD_EVENT_DATA          (1<<4)
#define NFD_EVENT_FILTER_START  0

/* nfd_cfg */
#define NFD_CFG_QUEUE           2
/* #define NFD_CFG_EVENT_DATA      (1<<4) */
/* #define NFD_CFG_EVENT_FILTER    14 */
#define NFD_CFG_RING_SZ         (4 * 512)
#define NFD_CFG_VF_OFFSET       64

#define NFD_CFG_FLR_EVENT_FILTER 15


/* nfd_in */
#define NFD_IN_MAX_BATCH_SZ     8
#define NFD_IN_FAST_PATH_BATCH_SZ 8
#define NFD_IN_MAX_NON_FAST_PATH_BATCH_SZ 4
#define NFD_IN_DESC_BATCH_Q_SZ  128
#define NFD_IN_ISSUED_RING0_SZ  128
#define NFD_IN_ISSUED_RING0_RES 32
#define NFD_IN_ISSUED_RING0_NUM 14
#define NFD_IN_ISSUED_RING1_SZ  128
#define NFD_IN_ISSUED_RING1_RES 32
#define NFD_IN_ISSUED_RING1_NUM 15

#define NFD_IN_BUF_STORE_SZ     96
#define NFD_IN_BUF_RECACHE_WM   24
#define NFD_IN_BUF_RING_POP_SZ  12

#define NFD_IN_JUMBO_STORE_SZ       16
#define NFD_IN_JUMBO_RECACHE_MAX    8
#define NFD_IN_JUMBO_RECACHE_MIN    4

#define NFD_IN_DATA_EVENT_XFER_ASSIGN 0
/* #define NFD_IN_Q_EVENT_START    0 */
/* #define NFD_IN_Q_START          0 */
/* #define NFD_IN_Q_EVENT_DATA     (1<<4) */

#define NFD_IN_TX_QUEUE         1


#define NFD_IN_ISSUE_START_CTX  2
#define NFD_IN_ISSUE_END_CTX    6
#define NFD_IN_NOTIFY_START_CTX 0
#define NFD_IN_NOTIFY_END_CTX   6

#define NFD_IN_DSTQ_MSK         0x7

/* Additional check queue constants */
#define NFD_IN_MAX_RETRIES      5
#define NFD_IN_BATCH_SZ         4
#define NFD_IN_PENDING_TEST     0

/* DMAConfigReg index allocations */
#define NFD_IN_GATHER_CFG_REG           0
#define NFD_IN_DATA_CFG_REG             2
#define NFD_IN_DATA_CFG_REG_SIG_ONLY    3


/* DMA defines */
#define NFD_IN_GATHER_MAX_IN_FLIGHT 32
#define NFD_IN_DATA_MAX_IN_FLIGHT   56
#define NFD_IN_JUMBO_MAX_IN_FLIGHT  8
#define NFD_IN_GATHER_DMA_QUEUE     NFP_PCIE_DMA_FROMPCI_HI
#define NFD_IN_DATA_DMA_QUEUE       NFP_PCIE_DMA_FROMPCI_LO
#define NFD_IN_DATA_DMA_TOKEN       2
#define NFD_IN_DATA_DMA_TRANS_CLASS 0
#define NFD_IN_DATA_ROUND           4

#define NFD_IN_GATHER_EVENT_TYPE    5
#define NFD_IN_DATA_EVENT_TYPE      6
#define NFD_IN_DATA_IGN_EVENT_TYPE  7
#define NFD_IN_JUMBO_EVENT_TYPE     14

#define NFD_IN_GATHER_EVENT_FILTER  10
#define NFD_IN_DATA0_EVENT_FILTER   11
#define NFD_IN_DATA1_EVENT_FILTER   12
#define NFD_IN_JUMBO0_EVENT_FILTER  13
#define NFD_IN_JUMBO1_EVENT_FILTER  14

#define NFD_IN_DMA_SPLIT_THRESH     (3 * 1024)
#define NFD_IN_DMA_SPLIT_LEN        2048

#define NFD_IN_DMA_INVALID_LEN      64

/* Debug defines */
#define NFD_IN_DBG_GATHER_INTVL     1000000
#define NFD_IN_DBG_ISSUE_DMA_INTVL  1000000
#define NFD_IN_DBG_NOTIFY_INTVL     1000000

/* Defines for gather/issue_dma CLS rings */
#define NFD_IN_BATCH_RING0_ADDR         0
#define NFD_IN_BATCH_RING0_SIZE_LW      64
#define NFD_IN_BATCH_RING0_NUM          1
#define NFD_IN_BATCH_RING1_ADDR         512
#define NFD_IN_BATCH_RING1_SIZE_LW      64
#define NFD_IN_BATCH_RING1_NUM          2

/* LSO defines */
/* Ring is sized to hold the worst case of (NFD_IN_MAX_LSO_SEQ_CNT + 1)
 * descriptors per slot in nfd_in_issued_ringX.  Each descriptor is 16B.
 * NFD_IN_ISSUED_RINGX_SZ (128) is in descriptors, and for full batches
 * of LSO descriptors each could generate the full number of segments.
 * Hence 128 * 65 * 16, rounded up to 128 * 128 * 16 = 256kB */
#define NFD_IN_ISSUED_LSO_RING0_NUM 0
#define NFD_IN_ISSUED_LSO_RING0_SZ  262144
#define NFD_IN_ISSUED_LSO_RING1_NUM 1
#define NFD_IN_ISSUED_LSO_RING1_SZ  262144
#define NFD_IN_MAX_LSO_HDR_SZ       256

#ifndef NFD_IN_MIN_LSO_HDR_SZ
#define NFD_IN_MIN_LSO_HDR_SZ       54
#endif

#ifndef NFD_IN_MAX_LSO_SEQ_CNT
#define NFD_IN_MAX_LSO_SEQ_CNT      64
#endif

#if defined(__NFP_LANG_MICROC) || defined(NFD_DBG)
/* NFD IN LSO Debug Counters. */
enum NFD_IN_LSO_CNTR_IDX {
    NFD_IN_LSO_CNTR_T_ISSUED_ALL_TX_DESC = 0,
    NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_EOP_TX_DESC,
    NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_EOP_JUMBO_TX_DESC,
    NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_CONT_TX_DESC,
    NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_CONT_JUMBO_TX_DESC,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_ALL_TX_DESC,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_EOP_TX_DESC,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_CONT_TX_DESC,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_JUMBO_TX_DESC,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_JUMBO_DBL_DMA,
    NFD_IN_LSO_CNTR_T_ISSUED_NOT_Q_UP_TX_DESC,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_HDR_READ,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_BLM_BUF_ALLOC_FAILED,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_ALL_PKT_TO_NOTIFY_RING,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_END_PKT_TO_NOTIFY_RING,
    NFD_IN_LSO_CNTR_T_ISSUED_LSO_EXC_PKT_TO_NOTIFY_RING,
    NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_MSS,
    NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_HDRLEN,
    NFD_IN_LSO_CNTR_T_NOTIFY_ALL_PKT_DESC,
    NFD_IN_LSO_CNTR_T_NOTIFY_NON_LSO_PKT_DESC,
    NFD_IN_LSO_CNTR_T_NOTIFY_LSO_PKT_DESC,
    NFD_IN_LSO_CNTR_T_NOTIFY_OTHER_PKT_DESC,
    NFD_IN_LSO_CNTR_T_NOTIFY_ALL_PKT_FM_LSO_RING,
    NFD_IN_LSO_CNTR_T_NOTIFY_LAST_PKT_FM_LSO_RING,
    NFD_IN_LSO_CNTR_T_NOTIFY_ALL_LSO_PKTS_TO_ME_WQ,
    NFD_IN_LSO_CNTR_T_NOTIFY_LSO_END_PKTS_TO_ME_WQ,
    NFD_IN_LSO_CNTR_T_NOTIFY_LSO_EOP_PKT_TO_ME_WQ,
    NFD_IN_LSO_CNTR_T_ME_ALL_HOST_PKTS,
    NFD_IN_LSO_CNTR_T_ME_NON_LSO_HOST_PKTS,
    NFD_IN_LSO_CNTR_T_ME_LSO_HOST_PKTS,
    NFD_IN_LSO_CNTR_T_ME_LAST_LSO_HOST_PKTS,
    NFD_IN_LSO_CNTR_T_ME_FM_HOST_PKT_TX_TO_WIRE,
    NFD_IN_LSO_CNTR_T_ME_FM_HOST_PROC_TO_WIRE_DROP,
    NFD_IN_LSO_CNTR_T_ME_FM_HOST_PKT_TX_TO_WIRE_DROP,
    NFD_IN_LSO_CNTR_LENGTH /*Last element indicates total number of counters*/
};

static const char *nfd_in_lso_cntr_names[] = {
    "NFD_IN_LSO_CNTR_T_ISSUED_ALL_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_EOP_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_EOP_JUMBO_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_CONT_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_CONT_JUMBO_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_ALL_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_EOP_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_CONT_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_JUMBO_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_JUMBO_DBL_DMA",
    "NFD_IN_LSO_CNTR_T_ISSUED_NOT_Q_UP_TX_DESC",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_HDR_READ",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_BLM_BUF_ALLOC_FAILED",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_ALL_PKT_TO_NOTIFY_RING",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_END_PKT_TO_NOTIFY_RING",
    "NFD_IN_LSO_CNTR_T_ISSUED_LSO_EXC_PKT_TO_NOTIFY_RING",
    "NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_MSS",
    "NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_HDRLEN",
    "NFD_IN_LSO_CNTR_T_NOTIFY_ALL_PKT_DESC",
    "NFD_IN_LSO_CNTR_T_NOTIFY_NON_LSO_PKT_DESC",
    "NFD_IN_LSO_CNTR_T_NOTIFY_LSO_PKT_DESC",
    "NFD_IN_LSO_CNTR_T_NOTIFY_OTHER_PKT_DESC",
    "NFD_IN_LSO_CNTR_T_NOTIFY_ALL_PKT_FM_LSO_RING",
    "NFD_IN_LSO_CNTR_T_NOTIFY_LAST_PKT_FM_LSO_RING",
    "NFD_IN_LSO_CNTR_T_NOTIFY_ALL_LSO_PKTS_TO_ME_WQ",
    "NFD_IN_LSO_CNTR_T_NOTIFY_LSO_END_PKTS_TO_ME_WQ",
    "NFD_IN_LSO_CNTR_T_NOTIFY_LSO_EOP_PKT_TO_ME_WQ",
    "NFD_IN_LSO_CNTR_T_ME_ALL_HOST_PKTS",
    "NFD_IN_LSO_CNTR_T_ME_NON_LSO_HOST_PKTS",
    "NFD_IN_LSO_CNTR_T_ME_LSO_HOST_PKTS",
    "NFD_IN_LSO_CNTR_T_ME_LAST_LSO_HOST_PKTS",
    "NFD_IN_LSO_CNTR_T_ME_FM_HOST_PKT_TX_TO_WIRE",
    "NFD_IN_LSO_CNTR_T_ME_FM_HOST_PROC_TO_WIRE_DROP",
    "NFD_IN_LSO_CNTR_T_ME_FM_HOST_PKT_TX_TO_WIRE_DROP"
};

/* XXX remove counter enable when done developing LSO */
#if defined(NFD_IN_LSO_CNTR_ENABLE)
    CNTRS64_DECLARE(nfd_in_lso_cntrs, NFD_IN_LSO_CNTR_LENGTH, __emem_n(0));
    #define NFD_IN_LSO_CNTR_INCR(_addr, _idx) cntr64_incr(_addr, _idx)
    #define NFD_IN_LSO_CNTR_CLR(_addr, _idx) cntr64_clr(_addr, _idx)
    #define NFD_IN_LSO_CNTR_ADD(_addr, _idx, _val) cntr64_add(_addr, _idx, _val)
#else
    #define NFD_IN_LSO_CNTR_INCR(_addr, _idx)
    #define NFD_IN_LSO_CNTR_CLR(_addr, _idx)
    #define NFD_IN_LSO_CNTR_ADD(_addr, _idx, _val)
#endif
#endif

/* nfd_out */
#define NFD_OUT_MAX_BATCH_SZ            4

#define NFD_OUT_CREDITS_BASE            0

/* #define NFD_OUT_Q_EVENT_START           4 */
/* #define NFD_OUT_Q_START                 128 */
/* #define NFD_OUT_Q_EVENT_DATA            (3<<4) */

#define NFD_OUT_FL_QUEUE                0

/* Additional check queue constants */
#define NFD_OUT_FL_BATCH_SZ             32   /* Match configured watermark! */


#define NFD_OUT_STAGE_START_CTX         2
#define NFD_OUT_STAGE_WAIT_CYCLES       200
#define NFD_OUT_ISSUE_START_CTX         1


/* DMAConfigReg index allocations */
#define NFD_OUT_FL_CFG_REG              4
#define NFD_OUT_DESC_CFG_REG            8

#define NFD_OUT_DATA_CFG_REG            6
#define NFD_OUT_DATA_CFG_REG_SIG_ONLY   7

/* DMA defines */
#define NFD_OUT_FL_MAX_IN_FLIGHT        16
#define NFD_OUT_DESC_MAX_IN_FLIGHT      64
#define NFD_OUT_DESC_MAX_BATCH_SZ       128

#define NFD_OUT_FL_DMA_QUEUE            NFP_PCIE_DMA_FROMPCI_HI
#define NFD_OUT_DATA_DMA_QUEUE          NFP_PCIE_DMA_TOPCI_LO
#define NFD_OUT_DESC_DMA_QUEUE          NFP_PCIE_DMA_TOPCI_MED
#define NFD_OUT_DATA_DMA_TOKEN          2

#ifndef NFD_OUT_FL_BUFS_PER_QUEUE
#define NFD_OUT_FL_BUFS_PER_QUEUE       256
#endif

#if ((NFD_OUT_FL_BUFS_PER_QUEUE & (NFD_OUT_FL_BUFS_PER_QUEUE - 1)) != 0)
#error "NFD_OUT_FL_BUFS_PER_QUEUE must be a power of two"
#endif

#if (NFD_OUT_FL_BUFS_PER_QUEUE < 256)
#error "NFD_OUT_FL_BUFS_PER_QUEUE must be 256 or greater"
#endif

#define NFD_OUT_FL_SOFT_THRESH          (NFD_OUT_FL_BUFS_PER_QUEUE / 4)


#ifndef NFD_PCIE0_FL_CACHE_MEM
#define NFD_PCIE0_FL_CACHE_MEM          pcie0.ctm
#endif

#ifndef NFD_PCIE1_FL_CACHE_MEM
#define NFD_PCIE1_FL_CACHE_MEM          pcie1.ctm
#endif

#ifndef NFD_PCIE2_FL_CACHE_MEM
#define NFD_PCIE2_FL_CACHE_MEM          pcie2.ctm
#endif

#ifndef NFD_PCIE3_FL_CACHE_MEM
#define NFD_PCIE3_FL_CACHE_MEM          pcie3.ctm
#endif


/* XXX Check event type assignments for conflicts with autogenerated events */
#define NFD_OUT_FL_EVENT_TYPE           13
#define NFD_OUT_FL_EXT_TYPE             0
#define NFD_OUT_DESC_EVENT_TYPE         13
#define NFD_OUT_DESC_EXT_TYPE           1

#define NFD_OUT_FL_EVENT_FILTER         8
#define NFD_OUT_DESC_EVENT_FILTER       9

/* Ring defines */
#define NFD_OUT_CPP_BATCH_RING_BAT      64

/* Debug defines */
#define NFD_OUT_DBG_CACHE_DESC_INTVL    1000000


#if !defined(__NFP_LANG_ASM)

/* nfd_cfg internal structures */
enum nfd_cfg_component {
    NFD_CFG_PCI_IN0,
    NFD_CFG_PCI_IN1,
    NFD_CFG_PCI_OUT
};


/* nfd_in internal structures */
struct nfd_in_queue_info {
    unsigned int tx_w;
    unsigned int tx_s;
    unsigned int ring_sz_msk;
    unsigned int requester_id;
    unsigned int spare0:23;
    unsigned int up:1;
    unsigned int ring_base_hi:8;
    unsigned int ring_base_lo;
    unsigned int dummy[2];
};


#define NFD_IN_DMA_STATE_UP_msk             1
#define NFD_IN_DMA_STATE_UP_shf             31
#define NFD_IN_DMA_STATE_UP_wrd             0
#define NFD_IN_DMA_STATE_CONT_msk           1
#define NFD_IN_DMA_STATE_CONT_shf           30
#define NFD_IN_DMA_STATE_CONT_wrd           0
#define NFD_IN_DMA_STATE_LOCKED_msk         1
#define NFD_IN_DMA_STATE_LOCKED_shf         29
#define NFD_IN_DMA_STATE_LOCKED_wrd         0
#define NFD_IN_DMA_STATE_LSO_OFFHDR_msk     0xFF
#define NFD_IN_DMA_STATE_LSO_OFFHDR_shf     16
#define NFD_IN_DMA_STATE_LSO_OFFHDR_wrd     0
#define NFD_IN_DMA_STATE_LSO_SEQ_CNT_msk    0xFF
#define NFD_IN_DMA_STATE_LSO_SEQ_CNT_shf    8
#define NFD_IN_DMA_STATE_LSO_SEQ_CNT_wrd    0
#define NFD_IN_DMA_STATE_RID_msk            0xFF
#define NFD_IN_DMA_STATE_RID_shf            0
#define NFD_IN_DMA_STATE_RID_wrd            0

#define NFD_IN_DMA_STATE_INVALID_msk        1
#define NFD_IN_DMA_STATE_INVALID_shf        31
#define NFD_IN_DMA_STATE_INVALID_wrd        1
#define NFD_IN_DMA_STATE_JUMBO_msk          1
#define NFD_IN_DMA_STATE_JUMBO_shf          30
#define NFD_IN_DMA_STATE_JUMBO_wrd          1
#define NFD_IN_DMA_STATE_CURR_BUF_msk       0x1FFFFFFF
#define NFD_IN_DMA_STATE_CURR_BUF_shf       0
#define NFD_IN_DMA_STATE_CURR_BUF_wrd       1

#define NFD_IN_DMA_STATE_FLAGS_msk          0xFF
#define NFD_IN_DMA_STATE_FLAGS_shf          24
#define NFD_IN_DMA_STATE_FLAGS_wrd          2
#define NFD_IN_DMA_STATE_LSO_HDRLEN_msk     0xFF
#define NFD_IN_DMA_STATE_LSO_HDRLEN_shf     16
#define NFD_IN_DMA_STATE_LSO_HDRLEN_wrd     2
#define NFD_IN_DMA_STATE_LSO_RES_msk        3
#define NFD_IN_DMA_STATE_LSO_RES_shf        14
#define NFD_IN_DMA_STATE_LSO_RES_wrd        2
#define NFD_IN_DMA_STATE_LSO_MSS_msk        0x3FFF
#define NFD_IN_DMA_STATE_LSO_MSS_shf        0
#define NFD_IN_DMA_STATE_LSO_MSS_wrd        2

#define NFD_IN_DMA_STATE_OFFSET_msk         0x3f
#define NFD_IN_DMA_STATE_OFFSET_shf         24
#define NFD_IN_DMA_STATE_OFFSET_wrd         3
#define NFD_IN_DMA_STATE_DATA_LEN_msk       0xFFFFFF
#define NFD_IN_DMA_STATE_DATA_LEN_shf       0
#define NFD_IN_DMA_STATE_DATA_LEN_wrd       3
#define NFD_IN_DMA_STATE_DATA_LEN_ORIG_msk  0xFFFF

#define NFD_IN_DMA_STATE_BYTES_DMAED_wrd    4
#define NFD_IN_DMA_STATE_LSO_PAYLOAD_wrd    5
#define NFD_IN_DMA_STATE_LSO_OFFSET_wrd     6


struct nfd_in_dma_state {
    union {
        struct {
            unsigned int up:1;
            unsigned int cont:1;
            unsigned int locked:1;
            unsigned int sp0:5;
            unsigned int lso_offhdr:8;  /* length of offset + header
                                           if zero indicates no header
                                           in progress. Used in issue_dma */
            unsigned int lso_seq_cnt:8; /* last sequence count for
                                           lso segments sent to NFP.
                                           Used in notify. */
            unsigned int rid:8;

            unsigned int invalid:1;
            unsigned int jumbo:1;
            unsigned int sp1:1;
            unsigned int curr_buf:29;

            unsigned int flags:8;       /* Original flags for the packet */
            unsigned int lso_hdrlen:8;  /* Original LSO header len */
            unsigned int res:2;         /* Not tested for consistency */
            unsigned int mss:14;        /* Original LSO MSS */

            unsigned int offset:8;      /* Original TX desc offset (meta len) */
            unsigned int data_len:24;   /* Original TX desc data_len */

            unsigned int bytes_dmaed;   /* Total bytes dmaed for data_len */
            unsigned int lso_payload_len;   /* Bytes dmaed for current mss */

            unsigned int spare0;
            unsigned int spare1;
        };
        unsigned int __raw[8];
    };
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
 * Word  +-+-------------+---------------+---------------+---+-----------+
 *    0  |E|    offset   | lso_issued_cnt|   num_batch   |sp1|   q_num   |
 *       +-+-------------+---------------+---------------+---+-----------+
 *    1  |                           buf_addr                            |
 *       +---------------+---------------+-+-+---------------------------+
 *    2  |     flags     |  lso_seq_cnt  |L|S|           mss             |
 *       +---------------+---------------+-+-+---------------------------+
 *    3  |            data_len           |              vlan             |
 *       +-------------------------------+-------------------------------+
 *
 *      sp0 - sp2 -> spare
 *      E -> End of packet
 *      L -> Last packet in a series of LSO packets
 *      S -> sp1
 */
struct nfd_in_issued_desc {
    union {
        struct {
            unsigned int eop:1;
            unsigned int offset:7;
            unsigned int lso_issued_cnt:8;
            unsigned int num_batch:8;
            unsigned int sp1:2;
            unsigned int q_num:6;

            unsigned int buf_addr:32;

            unsigned int flags:8;
            unsigned int lso_seq_cnt:8;
            unsigned int lso_end:1;
            unsigned int sp2:1;
            unsigned int mss:14;

            unsigned int data_len:16;
            unsigned int vlan:16;
        };
        unsigned int __raw[4];
    };
};


/* nfd_out internal structures */
struct nfd_out_queue_info {
    unsigned int fl_w;
    unsigned int fl_s;
    unsigned int ring_sz_msk;
    unsigned int requester_id:8;
    unsigned int spare0:15;
    unsigned int up:1;
    unsigned int ring_base_hi:8;
    unsigned int ring_base_lo;
    unsigned int fl_a;
    unsigned int rx_s;
    unsigned int rx_w;
};

#define NFD_OUT_QUEUE_INFO_SZ           32

/* Host code may include this file, so provide a host-friendly path
 * to dependencies */
#if defined(__NFP_LANG_MICROC)
#include "../pci_out.h"
#else
#include <pci_out.h>
#endif

/*
 * Freelist descriptor format
 */
struct nfd_out_fl_desc {
    union {
        struct {
            unsigned int dd:1;          /* Must be zero */
            unsigned int spare:23;
            unsigned int dma_addr_hi:8; /* High bits of the buf address */

            unsigned int dma_addr_lo;   /* Low bits of the buffer address */
        };
        unsigned int __raw[2];
    };
};


struct nfd_out_send_desc_msg {
    union {
        struct {
            unsigned int spare:16;         /* Unused */
            unsigned int count:8;          /* Number of descriptors sent */
            unsigned int queue:8;          /* Queue serviced */
        };
        unsigned int __raw;
    };
};


#if defined(__NFP_LANG_MICROC)

#include <nfp_chipres.h>

/* Helper macros */
/* XXX can provide an extra _pool parameter here if required */
#define NFD_BLM_Q_ALLOC_IND(_name)                  \
    _NFP_CHIPRES_ASM(.alloc_resource _name BLQ_EMU_RINGS global 1)
#define NFD_BLM_Q_ALLOC(_name) NFD_BLM_Q_ALLOC_IND(_name)

#define NFD_BLM_Q_LINK_IND(_name)                  \
    _link_sym(_name)
#define NFD_BLM_Q_LINK(_name) NFD_BLM_Q_LINK_IND(_name)

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

#endif /* __NFP_LANG_MICROC */

#endif /* !__NFP_LANG_ASM */

/* NFD halt error codes */
#define NFD_CFG_PCIE_LINK_DOWN          0x800
#define NFD_CFG_PCIE_MON_ABI_MISMATCH   0x801
#define NFD_IN_NOTIFY_MU_PTR_INVALID    0x802
#define NFD_IN_ISSUE_DMA_MU_PTR_INVALID 0x803
#define NFD_IN_GATHER_INVALID_IDMA      0x804
#define NFD_IN_ISSUE_DMA_BATCH_INVALID  0x805
#define NFD_OUT_PD_INVALID_LENGTH       0x806
#define NFD_OUT_PD_UNMARKED_CTM_ONLY    0x807
#define NFD_OUT_USER_CTX_ERROR          0x808
#define NFD_CFG_BAR_BASE_ISL_INVALID    0x809

#endif /* !_BLOCKS__SHARED_NFD_INTERNAL_H_ */
