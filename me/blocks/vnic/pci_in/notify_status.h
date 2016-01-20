/*
 * Copyright (C) 2015-2016,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/pci_in/notify_status.h
 * @brief         Display the state of the notify block
 */
#ifndef _BLOCKS__VNIC_PCI_IN_NOTIFY_STATUS_H_
#define _BLOCKS__VNIC_PCI_IN_NOTIFY_STATUS_H_

#define STATUS_NOTIFY_START      60


/* XXX the dma_served is already in xfers for the reflect */
struct nfd_in_notify_status {
    unsigned int dma_compl;
    unsigned int dma_served;
};

#if defined (__NFP_LANG_MICROC)

/**
 * Initialise the notify status output
 */
extern void notify_status_setup();

/**
 * Show notify status in transfer registers
 */
extern void notify_status();

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__VNIC_PCI_IN_NOTIFY_STATUS_H_ */
