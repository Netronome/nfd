/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/pci_in_internal.h
 * @brief         Internal structure and message definitions for PCI.IN
 */
#ifndef _BLOCKS__VNIC_PCI_IN_INTERNAL_H_
#define _BLOCKS__VNIC_PCI_IN_INTERNAL_H_

struct tx_queue_info {
    unsigned int tx_w;
    unsigned int tx_s;
    unsigned int ring_sz_msk;
    unsigned int requester_id;
    unsigned int ring_base_addr;
    unsigned int dummy[3];
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
