/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out_me1.c
 * @brief         Code to deploy on PCIe ME1 in NFD with 2 MEs per direction
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>

#include <vnic/pci_out/issue_dma.h>

int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        ctassert(MAX_VNICS * MAX_VNIC_QUEUES <= 64);

        issue_dma_setup_shared();

    } else {
        issue_dma_setup();
    }

    /*
     * Work loop
     */
    if (ctx() == 0) {
        /* CTX0 main loop */
        for (;;) {
            issue_dma_check_compl();

            /* Yield thread */
            ctx_swap();
        }
    } else {
        /* Worker main loop */
        for (;;) {
            issue_dma();

            /* Yield thread */
            ctx_swap();
        }
    }
}
