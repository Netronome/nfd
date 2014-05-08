/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in_me0.c
 * @brief         Code to deploy on PCIe ME2 in NFD with 2 MEs per direction
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>

#include <nfp6000/nfp_qc.h>

#include <vnic/pci_in/distr_seqn.h>
#include <vnic/pci_in/gather.h>
#include <vnic/pci_in/gather_status.h>
#include <vnic/pci_in/service_qc.h>
#include <vnic/shared/vnic_cfg.h>

/*
 * Temporary header includes
 */
#include <std/event.h>                  /* TEMP */

VNIC_CFG_DECLARE(vnic_cfg_sig_pci_in, vnic_cfg_sig_pci_out);

struct vnic_cfg_msg cfg_msg;

int
main(void)
{
    /*
     * TEMP use a bit in a "status" GR to make threads wait
     * for initialisation to complete
     */
    #define STATUS_INIT_DONE_BIT 0
    __shared __gpr unsigned int status = 0;

    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        ctassert(MAX_VNICS * MAX_VNIC_QUEUES <= 64);

        /* Initialisation that does not swap */
        vnic_cfg_init_cfg_msg(&vnic_cfg_sig_pci_out, &cfg_msg);
        dummy_init_gather_shared();
        init_gather_status();

        /* Initialisation that swaps and takes longer */
        init_service_qc();
        init_distr_seqn();
        vnic_cfg_setup();

        /* TEMP: Mark initialisation complete */
        status |= (1<<STATUS_INIT_DONE_BIT);
        /* TEMP: Trigger user event (easy to look for) */
        event_cls_user_event(0x1234);
    } else {
        dummy_init_gather();    /* Should this be strictly after
                                 * dummy_init_gather_shared? */
    }

    /* Perform general initialisation */

    /*
     * TEMP: Wait for initialisation to be completed
     * NB: there is a bit_test intrinsic that explicitly uses br_bset,
     * but the compiler will also replace this with a br_bset instruction
     * if it is more efficient.
     * THSDK-1184 workaround. Use bit_test intrinsic to ensure @GPR reread.
     */
/*    while ((status & (1<<STATUS_INIT_DONE_BIT)) == 0) {*/
    while (!bit_test(status, STATUS_INIT_DONE_BIT)) {
       ctx_swap();
    }


    /*
     * Work loop
     */
    if (ctx() == 0) {
        /* CTX0 main loop */
        for (;;) {
            service_qc();

            distr_seqn();

            vnic_cfg_check_cfg_ap();

            gather_status();

            /* Either check for a message, or perform one tick of processing
             * on the message each loop iteration */
            if (!cfg_msg.msg_valid) {
                int curr_vnic;

                curr_vnic = vnic_cfg_next_vnic();

                if (curr_vnic >= 0) {
                    cfg_msg.__raw = 0;
                    cfg_msg.vnic = curr_vnic;
                    cfg_msg.msg_valid = 1;

                    vnic_cfg_parse_msg((void *) &cfg_msg, VNIC_CFG_PCI_IN);
                }
            } else {
                service_qc_vnic_setup(&cfg_msg);

                if (!cfg_msg.msg_valid) {
                    vnic_cfg_start_cfg_msg(&cfg_msg,
                                           &vnic_cfg_sig_pci_out,
                                           VNIC_CFG_NEXT_ME(PCIE_ISL, 0),
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
            gather();

            /* Yield thread */
            ctx_swap();
        }
    }
}
