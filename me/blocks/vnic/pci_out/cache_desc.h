/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc.c
 * @brief         Code to cache FL descriptors from pending queues
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_CACHE_DESC_H_
#define _BLOCKS__VNIC_PCI_OUT_CACHE_DESC_H_

#include <vnic/shared/vnic_cfg.h>

extern void cache_desc_setup_shared();

extern void cache_desc_setup();

__intrinsic void cache_desc_vnic_setup(struct vnic_cfg_msg *cfg_msg_in);

__intrinsic unsigned int cache_desc_compute_fl_addr(__gpr unsigned int *queue,
                                                    unsigned int seq);

extern void cache_desc();

#endif /* !_BLOCKS__VNIC_PCI_OUT_CACHE_DESC_H_ */

