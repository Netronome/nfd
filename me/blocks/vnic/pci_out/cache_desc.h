/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc.c
 * @brief         Code to cache FL descriptors from pending queues
 */
#ifndef _BLOCKS__VNIC_PCI_OUT_CACHE_DESC_H_
#define _BLOCKS__VNIC_PCI_OUT_CACHE_DESC_H_

#include <vnic/shared/nfd_cfg.h>

/* XXX TEMP */
#define NFD_CFG_VF_OFFSET       64

__intrinsic void nfd_cfg_proc_msg(struct nfd_cfg_msg *cfg_msg,
                                  unsigned int *queue,
                                  unsigned char *ring_sz,
                                  unsigned int ring_base[2],
                                  enum nfd_cfg_component comp);


extern void cache_desc_setup_shared();

extern void cache_desc_setup();

__intrinsic void cache_desc_vnic_setup(struct nfd_cfg_msg *cfg_msg_in);

extern void cache_desc();

__intrinsic unsigned int cache_desc_compute_fl_addr(__gpr unsigned int *queue,
                                                    unsigned int seq);

#endif /* !_BLOCKS__VNIC_PCI_OUT_CACHE_DESC_H_ */

