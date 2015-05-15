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

#include <vnic/pci_in/gather_seq_recv.c>
#include <vnic/pci_in/issue_dma.c>
#include <vnic/pci_in/issue_dma_status.c>
#include <vnic/pci_in/precache_bufs.c>
#include <vnic/shared/nfd_cfg_internal.c>


NFD_CFG_DECLARE(nfd_cfg_sig_pci_in1, nfd_cfg_sig_pci_out);
NFD_INIT_DONE_DECLARE;

struct nfd_cfg_msg cfg_msg;


int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        nfd_cfg_check_pcie_link(); /* Will halt ME on failure */

        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_pci_in1, &cfg_msg);

        precache_bufs_setup();

        issue_dma_status_setup();

        issue_dma_setup_shared();

        NFD_INIT_DONE_SET(PCIE_ISL, 3);     /* XXX Remove? */
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

            precache_bufs();

            issue_dma_status();

            /* Either check for a message, or perform one tick of processing
             * on the message each loop iteration */
            if (!cfg_msg.msg_valid) {
                nfd_cfg_check_cfg_msg(&cfg_msg, &nfd_cfg_sig_pci_in1,
                                      NFD_CFG_RING_NUM(PCIE_ISL, 0));

                if (cfg_msg.msg_valid) {
                    nfd_cfg_parse_msg((void *) &cfg_msg, NFD_CFG_PCI_IN1);
                }
            } else {
                issue_dma_vnic_setup((void *) &cfg_msg);

                if (!cfg_msg.msg_valid) {
                    nfd_cfg_complete_cfg_msg(&cfg_msg,
                                             &nfd_cfg_sig_pci_out,
                                             NFD_CFG_NEXT_ME(PCIE_ISL, 0),
                                             NFD_CFG_RING_NUM(PCIE_ISL, 1),
                                             NFD_CFG_RING_NUM(PCIE_ISL, 0));
                }
            }

            /* Yield thread */
            ctx_swap();
        }
    } else {
        /* Worker main loop */

        for (;;) {
            issue_dma();

        }
    }
}
