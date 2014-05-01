/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/gather_status.h
 * @brief         Display the state of the gather and service_qc blocks
 */
#ifndef _BLOCKS__VNIC_PCI_IN_GATHER_STATUS_H_
#define _BLOCKS__VNIC_PCI_IN_GATHER_STATUS_H_

#define STATUS_Q_INDEP_START    16
#define STATUS_Q_INFO_START     24
#define STATUS_Q_SEL_START      31


struct tx_gather_status {
    unsigned int actv_bmsk_hi;
    unsigned int actv_bmsk_lo;
    unsigned int actv_bmsk_proc;
    unsigned int pend_bmsk_hi;
    unsigned int pend_bmsk_lo;
    unsigned int pend_bmsk_proc;
    unsigned int dma_issued;
    unsigned int dma_compl;
};

extern void init_gather_status();

extern void gather_status();

#endif /* !_BLOCKS__VNIC_PCI_IN_GATHER_STATUS_H_ */

