/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_internal.h
 * @brief         An API to manage access to NFD configuration data
 */
#ifndef _BLOCKS__SHARED_NFD_INTERNAL_H_
#define _BLOCKS__SHARED_NFD_INTERNAL_H_


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
#define NFD_IN_MAX_BATCH_SZ     4
#define NFD_IN_DESC_BATCH_Q_SZ  128
#define NFD_IN_ISSUED_RING_SZ   128
#define NFD_IN_ISSUED_RING_RES  32
#define NFD_IN_ISSUED_RING_NUM  15

#define NFD_IN_BUF_STORE_SZ     64
#define NFD_IN_BUF_RECACHE_WM   16

/* #define NFD_IN_Q_EVENT_START    0 */
/* #define NFD_IN_Q_START          0 */
/* #define NFD_IN_Q_EVENT_DATA     (1<<4) */

#define NFD_IN_TX_QUEUE         1

#define NFD_IN_ISSUE_START_CTX  1
#define NFD_IN_NOTIFY_START_CTX 2

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
/* NB: PCI.IN may also have up to 16 extra jumbo frame DMAs in flight */
#define NFD_IN_DATA_MAX_IN_FLIGHT   112
#define NFD_IN_GATHER_DMA_QUEUE     NFP_PCIE_DMA_FROMPCI_HI
#define NFD_IN_DATA_DMA_QUEUE       NFP_PCIE_DMA_FROMPCI_LO
#define NFD_IN_DATA_DMA_TOKEN       2
#define NFD_IN_DATA_ROUND           4

#define NFD_IN_GATHER_EVENT_TYPE    5
#define NFD_IN_DATA_EVENT_TYPE      6
#define NFD_IN_DATA_IGN_EVENT_TYPE  7

#define NFD_IN_GATHER_EVENT_FILTER  9
#define NFD_IN_DATA_EVENT_FILTER    10

/* XXX use! */
#define NFD_IN_DMA_SPLIT_LEN        4096

/* Debug defines */
#define NFD_IN_DBG_GATHER_INTVL     1000000
#define NFD_IN_DBG_ISSUE_DMA_INTVL  1000000


/* nfd_out */
#define NFD_OUT_MAX_BATCH_SZ            4

#define NFD_OUT_CREDITS_BASE            0

/* #define NFD_OUT_Q_EVENT_START           4 */
/* #define NFD_OUT_Q_START                 128 */
/* #define NFD_OUT_Q_EVENT_DATA            (3<<4) */

#define NFD_OUT_FL_QUEUE                0

/* Additional check queue constants */
#define NFD_OUT_FL_BATCH_SZ             32   /* Match configured watermark! */


#define NFD_OUT_STAGE_START_CTX         1
#define NFD_OUT_STAGE_WAIT_CYCLES       200
#define NFD_OUT_ISSUE_START_CTX         1


/* DMAConfigReg index allocations */
#define NFD_OUT_FL_CFG_REG              4
#define NFD_OUT_DESC_CFG_REG            8

#define NFD_OUT_DATA_CFG_REG            6
#define NFD_OUT_DATA_CFG_REG_SIG_ONLY   7

/* DMA defines */
#define NFD_OUT_FL_MAX_IN_FLIGHT        16
#define NFD_OUT_DATA_MAX_IN_FLIGHT      64
#define NFD_OUT_RESV_MAX_IN_FLIGHT      64
#define NFD_OUT_DESC_MAX_IN_FLIGHT      64

#define NFD_OUT_MAX_RESV_PER_PKT        3

#define NFD_OUT_FL_DMA_QUEUE            NFP_PCIE_DMA_FROMPCI_HI
#define NFD_OUT_DATA_DMA_QUEUE          NFP_PCIE_DMA_TOPCI_LO
#define NFD_OUT_DESC_DMA_QUEUE          NFP_PCIE_DMA_TOPCI_MED
#define NFD_OUT_DATA_DMA_TOKEN          2

#define NFD_OUT_FL_BUFS_PER_QUEUE       256
#define NFD_OUT_FL_SOFT_THRESH          64

/* XXX Check event type assignments for conflicts with autogenerated events */
#define NFD_OUT_FL_EVENT_TYPE           13
#define NFD_OUT_DATA_EVENT_TYPE         14
#define NFD_OUT_DATA_IGN_EVENT_TYPE     7
#define NFD_OUT_DESC_EVENT_TYPE         3

#define NFD_OUT_FL_EVENT_FILTER         11
#define NFD_OUT_DATA_EVENT_FILTER       12
#define NFD_OUT_DESC_EVENT_FILTER       13

#define NFD_OUT_DMA_SPLIT_LEN           2500    /* XXX optimise */
#define NFD_OUT_DMA_SPLIT_MIN           2048    /* XXX optimise */

/* Ring defines */
#define NFD_OUT_DESC_BATCH_RING_BAT     128
#define NFD_OUT_CPP_BATCH_RING_BAT      64

/* Debug defines */
#define NFD_OUT_DBG_CACHE_DESC_INTVL    1000000
#define NFD_OUT_DBG_ISSUE_DMA_INTVL     1000000



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


struct nfd_in_dma_state {
    unsigned int sp0:24;
    unsigned int rid:8;
    unsigned int cont:1;
    unsigned int up:1;
    unsigned int sp1:1;
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
 * Word  +-+-------------+---------------+---------------+---+-----------+
 *    0  |E|    offset   |      sp0      |   num_batch   |sp1|   q_num   |
 *       +-+-------------+---------------+---------------+---+-----------+
 *    1  |                           buf_addr                            |
 *       +---------------+---------------+-------------------------------+
 *    2  |     flags     |   l4_offset   |               lso             |
 *       +---------------+---------------+-------------------------------+
 *    3  |            data_len           |              vlan             |
 *       +-------------------------------+-------------------------------+
 *
 *      sp0 - sp3 -> spare
 *      E -> End of packet
 */
struct nfd_in_issued_desc {
    union {
        struct {
            unsigned int eop:1;
            unsigned int offset:7;
            unsigned int sp0:8;
            unsigned int num_batch:8;
            unsigned int sp1:2;
            unsigned int q_num:6;

            unsigned int buf_addr:32;

            unsigned int flags:8;
            unsigned int l4_offset:8;
            unsigned int lso:16;

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
    unsigned int fl_u;
    unsigned int rx_w;
};

#define NFD_OUT_QUEUE_INFO_RING_SZ_MSK  2
#define NFD_OUT_QUEUE_INFO_BITFIELD     3
#define NFD_OUT_QUEUE_INFO_RID_msk      0xff
#define NFD_OUT_QUEUE_INFO_RID_shf      24
#define NFD_OUT_QUEUE_INFO_UP_shf       8
#define NFD_OUT_QUEUE_INFO_RING_HI_msk  0xff
#define NFD_OUT_QUEUE_INFO_RING_HI_shf  0
#define NFD_OUT_QUEUE_INFO_RING_LO      4
#define NFD_OUT_QUEUE_INFO_FLA          5
#define NFD_OUT_QUEUE_INFO_FLU          6
#define NFD_OUT_QUEUE_INFO_RXW          7

#define NFD_OUT_QUEUE_INFO_SZ           32
#define NFD_OUT_QUEUE_INFO_SZ_lg2       5
#define NFD_OUT_FL_CACHE_Q_SZ_lg2       11
#define NFD_OUT_FL_ENTRY_SZ_lg2         3

#if defined(__NFP_LANG_MICROC)
#include <vnic/pci_out.h>
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


/**
 * Message format used between "stage_batch" and "send_desc"
 * "num" provides the number of packets in this batch.  RX descriptors
 * are only DMA'ed if "send_pktX" is set.
 */
struct nfd_out_desc_batch_msg {
    union {
        struct {
            unsigned int send_pkt0:1;
            unsigned int send_pkt1:1;
            unsigned int send_pkt2:1;
            unsigned int send_pkt3:1;
            unsigned int queue_pkt0:6;
            unsigned int queue_pkt1:6;
            unsigned int queue_pkt2:6;
            unsigned int queue_pkt3:6;
            unsigned int num:4;
        };
        unsigned int __raw;
    };
};

#define NFD_OUT_DESC_SEND_PKT0_shf  31
#define NFD_OUT_DESC_SEND_PKT1_shf  30
#define NFD_OUT_DESC_SEND_PKT2_shf  29
#define NFD_OUT_DESC_SEND_PKT3_shf  28
#define NFD_OUT_DESC_QUEUE_PKT0_shf 22
#define NFD_OUT_DESC_QUEUE_PKT1_shf 16
#define NFD_OUT_DESC_QUEUE_PKT2_shf 10
#define NFD_OUT_DESC_QUEUE_PKT3_shf 4
#define NFD_OUT_DESC_QUEUE_msk      0x3f
#define NFD_OUT_DESC_NUM_msk        0xf

/**
 * Batch header used on the "stage_batch" to "issue_dma" NN ring
 */
struct nfd_out_data_batch_msg {
    union {
        struct {
            unsigned int num;
        };
        unsigned int __raw;
    };
};


/**
 * Message format to pass issue_dma processing on to free_buf
 */
struct nfd_out_free_buf_desc {
    unsigned int isl_hi:8;          /* 0x80 | isl */
    unsigned int resv_dma_cnt:8;    /* Number of reserve DMAs issued */
    unsigned int pktnum:16;         /* pktnum packed into 16bits */

    unsigned int bls:3;             /* Concatenated NBI:bls field */
    unsigned int mu_addr:29;        /* mu_addr left in place */
};

#define NFD_OUT_FREE_CTM_INDEX0         0
#define NFD_OUT_FREE_MU_INDEX0          1
#define NFD_OUT_FREE_CTM_INDEX1         2
#define NFD_OUT_FREE_MU_INDEX1          3
#define NFD_OUT_FREE_CTM_INDEX2         4
#define NFD_OUT_FREE_MU_INDEX2          5
#define NFD_OUT_FREE_CTM_INDEX3         6
#define NFD_OUT_FREE_MU_INDEX3          7


#define NFD_OUT_FREE_CTM_ISL_HI_msk     0xff
#define NFD_OUT_FREE_CTM_ISL_HI_shf     24
#define NFD_OUT_FREE_CTM_RESV_msk       0xff
#define NFD_OUT_FREE_CTM_RESV_shf       16
#define NFD_OUT_FREE_CTM_PKTNUM_msk     0xffff
#define NFD_OUT_FREE_CTM_PKTNUM_shf     0
#define NFD_OUT_FREE_MU_BLS_msk         0x7
#define NFD_OUT_FREE_MU_BLS_shf         29
#define NFD_OUT_FREE_MU_BUF_msk         0x1fffffff
#define NFD_OUT_FREE_MU_BUF_shf         0


#define NFD_OUT_CPP_CTM_INDEX0          0
#define NFD_OUT_CPP_MU_INDEX0           1
#define NFD_OUT_CPP_CTM_INDEX1          2
#define NFD_OUT_CPP_MU_INDEX1           3
#define NFD_OUT_CPP_CTM_INDEX2          4
#define NFD_OUT_CPP_MU_INDEX2           5
#define NFD_OUT_CPP_CTM_INDEX3          6
#define NFD_OUT_CPP_MU_INDEX3           7

#define NFD_OUT_CPP_CTM_ISL_msk        0x3f
#define NFD_OUT_CPP_CTM_ISL_shf        26
#define NFD_OUT_CPP_CTM_CTM_ONLY_shf   25
#define NFD_OUT_CPP_CTM_PKTNUM_msk     0x1ff
#define NFD_OUT_CPP_CTM_PKTNUM_shf     16
#define NFD_OUT_CPP_CTM_SPLIT_msk      0x3
#define NFD_OUT_CPP_CTM_SPLIT_shf      14
#define NFD_OUT_CPP_CTM_RESERVED_shf   13
#define NFD_OUT_CPP_CTM_OFFSET_msk     0x1fff
#define NFD_OUT_CPP_CTM_OFFSET_shf     0
#define NFD_OUT_CPP_MU_NBI_shf         31
#define NFD_OUT_CPP_MU_BLS_msk         0x3
#define NFD_OUT_CPP_MU_BLS_shf         29
#define NFD_OUT_CPP_MU_BUF_msk         0x1fffffff
#define NFD_OUT_CPP_MU_BUF_shf         0

/**
 * Union to facilitate converting issue_dma input to free_buf input
 */
struct nfd_out_cpp_desc_ring {
    union {
        struct nfd_out_free_buf_desc free;
        struct nfd_out_cpp_desc cpp;
    };
};


struct nfd_out_issue_rx_desc {
    union {
        struct {
            unsigned int dd:1;              /* Descriptor done, must be set */
            unsigned int meta_len:7;        /* Length of meta data prepended */
            unsigned int rid:8;             /* Requester ID for the pkt */
            unsigned int data_len:16;       /* Total data length */
        };
        unsigned int __raw;
    };
};

#define NFD_OUT_RX_DESC_QUEUE_msk   0x3f
#define NFD_OUT_RX_DESC_QUEUE_shf   16

/**
 * Descriptor passed between "stage_batch" and "issue_dma"
 * XXX redundant?
 */
struct nfd_out_data_dma_info {
    union {
        struct {
            struct nfd_out_cpp_desc cpp;        /* CPP descriptor */
            struct nfd_out_issue_rx_desc rxd;   /* Modified RX descriptor */
            unsigned int fl_cache_index;        /* FL descriptor index */
        };
        unsigned int __raw[4];
    };
};


/**
 * Structure for a batch of "stage_batch" to "issue_dma" descriptors
 */
struct nfd_out_data_batch {
    struct nfd_out_data_dma_info pkt0;
    struct nfd_out_data_dma_info pkt1;
    struct nfd_out_data_dma_info pkt2;
    struct nfd_out_data_dma_info pkt3;
};


#if defined(__NFP_LANG_MICROC)

#include <vnic/shared/nfcc_chipres.h>

/* Helper macros */
/* XXX can provide an extra _pool parameter here if required */
#define NFD_BLM_Q_ALLOC_IND(_name)                  \
    ASM(.alloc_resource _name BLQ_EMU_RINGS global 1)
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

/* NFD halt error codes */
#define NFD_CFG_PCIE_LINK_DOWN          0x800
#define NFD_CFG_PCIE_MON_ABI_MISMATCH   0x801

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__SHARED_NFD_INTERNAL_H_ */
