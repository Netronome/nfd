/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/pci_in_internal.h
 * @brief         Internal structure and message definitions for PCI.IN
 */
#ifndef _BLOCKS__VNIC_PCI_IN_INTERNAL_H_
#define _BLOCKS__VNIC_PCI_IN_INTERNAL_H_

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


struct batch_desc {
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


/* NB: this struct must be compatible with vnic_cfg_msg */
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


#endif /* !_BLOCKS__VNIC_PCI_IN_INTERNAL_H_ */
