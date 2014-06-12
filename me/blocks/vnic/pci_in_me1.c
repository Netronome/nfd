/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in_cfg.h
 * @brief         Code to deploy on PCIe ME3 in NFD with 2 MEs per direction
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>

#include <vnic/pci_in/gather_seq_recv.h>
#include <vnic/pci_in/issue_dma.h>
#include <vnic/shared/vnic_cfg.h>


int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        /* init_buffer_precache(); */

        init_gather_seq_recv();

        issue_dma_setup_shared();

    } else {
        issue_dma_setup();
    }

    /* Kickoff ordering stages! */

    /*
     * Work loop
     */
    if (ctx() == 0) {
        /* CTX0 main loop */
        for (;;) {
            gather_seq_recv();

            /* Yield thread */
            ctx_swap();
        }
    } else {
        /* Worker main loop */
        for (;;) {
            issue_dma();

            /* /\* Yield thread *\/ */
            /* ctx_swap(); */
        }
    }
}
