/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/pci_out_internal.h
 * @brief         Internal structure and message definitions for PCI.OUT
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_INTERNAL_H_
#define _BLOCKS__VNIC_PCI_OUT_INTERNAL_H_

struct rx_queue_info {
    unsigned int fl_w;
    unsigned int fl_s;
    unsigned int ring_sz_msk;
    unsigned int requester_id;
    unsigned int spare0:24;
    unsigned int ring_base_hi:8;
    unsigned int ring_base_lo;
    unsigned int rx_w;
    unsigned int dummy[1];
};

/* NB: this struct must be compatible with vnic_cfg_msg */
struct pci_out_cfg_msg {
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

#endif /* !_BLOCKS__VNIC_PCI_OUT_INTERNAL_H_ */
