/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out_me0.c
 * @brief         Code to deploy on PCIe ME0 in NFD with 2 MEs per direction
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>

#include <nfp6000/nfp_qc.h>

#include <vnic/pci_out/cache_desc.c>
#include <vnic/pci_out/cache_desc_status.c>
#include <vnic/pci_out/stage_batch.c>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_cfg_internal.c>

NFD_CFG_DECLARE(nfd_cfg_sig_pci_out, NFD_CFG_SIG_NEXT_ME);

struct nfd_cfg_msg cfg_msg;

int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        ctassert(MAX_VNICS * MAX_VNIC_QUEUES <= 64);

        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_pci_out, &cfg_msg);

        /* Ensure rings are safe to use ASAP */
        stage_batch_setup_rings();

        /* Must run before PCI.OUT host interaction, and before stage_batch */
        cache_desc_setup_shared();

        cache_desc_status_setup();

        /* This method must complete before stage_batch may run.
         * (stage_batch permits issue_dma and send_desc to go.) */
        distr_seqn_setup_shared();

        send_desc_setup_shared();

        /* CTX 1-7 will stall until this starts ordering stages */
        stage_batch_setup_shared();

    } else {
        /* These methods do not have dependencies on the
         * setup_shared() methods. */
        cache_desc_setup();

        stage_batch_setup();

        send_desc_setup();
    }

    /*
     * Work loop
     */
    if (ctx() == 0) {
        /* CTX0 main loop */
        for (;;) {
            cache_desc();

            cache_desc_status();

            distr_seqn();

            /* Either check for a message, or perform one tick of processing
             * on the message each loop iteration */
            if (!cfg_msg.msg_valid) {
                nfd_cfg_check_cfg_msg(&cfg_msg, &nfd_cfg_sig_pci_out,
                                      NFD_CFG_RING_NUM(PCIE_ISL, 0));

                if (cfg_msg.msg_valid) {
                    nfd_cfg_parse_msg((void *) &cfg_msg, NFD_CFG_PCI_OUT);
                }
            } else {
                cache_desc_vnic_setup((void *) &cfg_msg);

                if (!cfg_msg.msg_valid) {
                    nfd_cfg_complete_cfg_msg(&cfg_msg,
                                             &NFD_CFG_SIG_NEXT_ME,
                                             NFD_CFG_NEXT_ME,
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
            /* This method will stall until stage_batch_setup_shared
             * has completed. */
            stage_batch();

            /* This method won't start until stage_batch has
             * processed a batch. */
            send_desc();

            /* /\* Yield thread *\/ */
            /* ctx_swap(); */
        }
    }
}
