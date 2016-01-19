/*
 * Copyright (C) 2015-2016,  Netronome Systems, Inc.  All rights reserved.
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
