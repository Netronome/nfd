/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/pci_out_internal.h
 * @brief         Internal structure and message definitions for PCI.OUT
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_INTERNAL_H_
#define _BLOCKS__VNIC_PCI_OUT_INTERNAL_H_


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


#if defined (__NFP_LANG_MICROC)

#include <vnic/pci_out.h>

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


/* NB: this struct must be compatible with nfd_cfg_msg */
struct nfd_out_cfg_msg {
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


/*
 * Treat struct pci_out_data_dma_info as 16B when allocating space
 * and aligning the space.
 * XXX unnecessary...
 */
#define NFD_OUT_DATA_DMA_INFO_SZ     16

/**
 * Descriptor passed between "stage_batch" and "issue_dma"
 */
struct nfd_out_data_dma_info {
    union {
        struct {
            struct nfd_out_cpp_desc cpp;    /* CPP descriptor */
            unsigned int rid:8;             /* Requester ID for the pkt */
            unsigned int spare:8;
            unsigned int data_len:16;       /* Total data length */
            unsigned int fl_cache_index;    /* FL descriptor index */
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

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_PCI_OUT_INTERNAL_H_ */
