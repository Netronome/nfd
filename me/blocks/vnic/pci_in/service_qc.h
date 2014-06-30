/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/service_qc.h
 * @brief         Code to maintain and access the TX.W mask from the queue
 *                controller
 */
#ifndef _BLOCKS__VNIC_PCI_IN_SERVICE_QC_H_
#define _BLOCKS__VNIC_PCI_IN_SERVICE_QC_H_

#include <vnic/shared/vnic_cfg.h>

/**
 * Initialise the PCI.IN queue controller queues
 */
extern void init_service_qc();

/**
 * Change the configuration of the queues and rings associated with a vNIC
 * @param cfg_msg       configuration information concerning the change
 *
 * This method performs changes to the local state for a vNIC.  The 'cfg_msg'
 * struct is used in conjunction with 'vnic_cfg_proc_msg' and internal vnic_cfg
 * state to determine a particular queue to change each time this method is
 * called.  See vnic_cfg.h for further information.
 */
__intrinsic void service_qc_vnic_setup(struct vnic_cfg_msg *cfg_msg);

/**
 * Use API provided by shared/qc to update queue state
 */
extern void service_qc();

#endif /* !_BLOCKS__VNIC_PCI_IN_SERVICE_QC_H_ */

