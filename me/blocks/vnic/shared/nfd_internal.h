/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_internal.h
 * @brief         An API to manage access to NFD configuration data
 */
#ifndef _BLOCKS__SHARED_NFD_INTERNAL_H_
#define _BLOCKS__SHARED_NFD_INTERNAL_H_


#define NFD_CFG_QUEUE           1
#define NFD_CFG_EVENT_DATA      (2<<4)
#define NFD_CFG_EVENT_FILTER    14
#define NFD_CFG_RING_SZ         (4 * 512)
#define NFD_CFG_VF_OFFSET       64


/* XXX uncomment */
/* enum nfd_cfg_component { */
/*     NFD_CFG_PCI_IN, */
/*     NFD_CFG_PCI_OUT */
/* }; */


#endif /* !_BLOCKS__SHARED_NFD_INTERNAL_H_ */
