/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/issue_dma_status.c
 * @brief         Display the state of the issue_dma block
 */

#include <nfp.h>

#include <nfp/me.h>
#include <std/reg_utils.h>

#include <vnic/pci_out/issue_dma_status.h>

#include <vnic/pci_out_cfg.h>


/* NB PCI.OUT issue_dma has no per queue state */

/**
 * Issue DMA state
 */
extern __shared __gpr unsigned int data_dma_seq_issued;
extern __shared __gpr unsigned int data_dma_seq_compl;
extern __shared __gpr unsigned int data_dma_seq_served;
extern __shared __gpr unsigned int data_dma_seq_safe;


/**
 * Xfers to display state
 */
static __xwrite struct nfd_out_issue_dma_status status_issued = {0, 0, 0, 0};

SIGNAL status_throttle;


void
issue_dma_status_setup()
{
    /* Fix the transfer registers used */
    __assign_relative_register(&status_issued, STATUS_ISSUE_DMA_START);

    set_alarm(NFD_OUT_DBG_ISSUE_DMA_INTVL, &status_throttle);
}

void
issue_dma_status()
{
    if (signal_test(&status_throttle))
    {
        __implicit_read(&status_issued, sizeof status_issued);

        /*
         * Collect the independent data from various sources
         */
        status_issued.data_dma_seq_issued = data_dma_seq_issued;
        status_issued.data_dma_seq_compl = data_dma_seq_compl;
        status_issued.data_dma_seq_served = data_dma_seq_served;
        status_issued.data_dma_seq_safe = data_dma_seq_safe;

        /*
         * Reset the alarm
         */
        set_alarm(NFD_OUT_DBG_ISSUE_DMA_INTVL, &status_throttle);
    }
}
