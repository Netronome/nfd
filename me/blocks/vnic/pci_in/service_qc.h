/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/service_qc.h
 * @brief         Code to maintain and access the TX.W mask from the queue
 *                controller
 */
#ifndef _BLOCKS__VNIC_PCI_IN_SERVICE_QC_H_
#define _BLOCKS__VNIC_PCI_IN_SERVICE_QC_H_

/**
 * Initialise the PCI.IN queue controller queues
 */
extern void init_service_qc();


/** Parameters list to be filled out as extended */
extern void service_qc();

#endif /* !_BLOCKS__VNIC_PCI_IN_SERVICE_QC_H_ */

