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

#include <vnic/pci_out/cache_desc.h>
#include <vnic/pci_out/cache_desc_status.h>
#include <vnic/pci_out/send_desc.h>
#include <vnic/shared/vnic_cfg.h>

VNIC_CFG_DECLARE(vnic_cfg_sig_pci_out, VNIC_CFG_SIG_NEXT_ME);

struct vnic_cfg_msg cfg_msg;

int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        ctassert(MAX_VNICS * MAX_VNIC_QUEUES <= 64);

        vnic_cfg_init_cfg_msg(&vnic_cfg_sig_pci_out, &cfg_msg);

        cache_desc_setup_shared();

        send_desc_setup_shared();

        cache_desc_status_setup();

    } else {

    }

    /*
     * Work loop
     */
    if (ctx() == 0) {
        /* CTX0 main loop */
        for (;;) {
            cache_desc_status();

            if (!cfg_msg.msg_valid) {
                vnic_cfg_check_cfg_msg(&cfg_msg, &vnic_cfg_sig_pci_out,
                                       VNIC_CFG_RING_NUM(PCIE_ISL, 0),
                                       &VNIC_CFG_RING_ADDR(PCIE_ISL, 0));
            }

            if (cfg_msg.msg_valid) {

                cache_desc_vnic_setup((void *) &cfg_msg, PCIE_QC_SZ_8k + 8);

                if (!cfg_msg.msg_valid) {
                    vnic_cfg_complete_cfg_msg(&cfg_msg,
                                              &VNIC_CFG_SIG_NEXT_ME,
                                              VNIC_CFG_NEXT_ME,
                                              VNIC_CFG_RING_NUM(PCIE_ISL, 1),
                                              &VNIC_CFG_RING_ADDR(PCIE_ISL, 1),
                                              VNIC_CFG_RING_NUM(PCIE_ISL, 0),
                                              &VNIC_CFG_RING_ADDR(PCIE_ISL, 0));
                }
            }

            /* Yield thread */
            ctx_swap();
        }
    } else {
        /* Worker main loop */
        for (;;) {

            /* Yield thread */
            ctx_swap();
        }
    }
}
