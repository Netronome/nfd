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

#include <vnic/pci_in/distr_seqn.c>
#include <vnic/pci_in/gather.c>
#include <vnic/pci_in/gather_status.c>
#include <vnic/pci_in/notify.c>
#include <vnic/pci_in/service_qc.c>
#include <vnic/shared/nfd_cfg_internal.c>

/*
 * Temporary header includes
 */
#include <std/event.h>                  /* TEMP */

NFD_CFG_DECLARE(nfd_cfg_sig_pci_in0, nfd_cfg_sig_pci_in1);
NFD_INIT_DONE_DECLARE;

struct nfd_cfg_msg cfg_msg;

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
        ctassert(NFD_MAX_VNICS * NFD_MAX_VNIC_QUEUES <= 64);

        /* Initialisation that does not swap */
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_pci_in0, &cfg_msg);
        gather_setup_shared();
        gather_status_setup();

        /* Initialisation that swaps and takes longer */
#ifdef NFD_VNIC_PF
        nfd_cfg_setup_pf();
#endif

#ifdef NFD_VNIC_VF
        nfd_cfg_setup_vf();
#endif

        service_qc_setup();
        distr_seqn_setup();
        nfd_cfg_setup();

        notify_setup_shared();

        /* TEMP: Mark initialisation complete */
        status |= (1<<STATUS_INIT_DONE_BIT);
        /* TEMP: Trigger user event (easy to look for) */
        event_cls_user_event(0x1234);

        NFD_INIT_DONE_SET(PCIE_ISL, 2);     /* XXX Remove? */
    } else {
        gather_setup();

        notify_setup();
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

            nfd_cfg_check_cfg_ap();

            gather_status();

            /* Either check for a message, or perform one tick of processing
             * on the message each loop iteration */
            if (!cfg_msg.msg_valid) {
                int curr_vnic;

                curr_vnic = nfd_cfg_next_vnic();

                if (curr_vnic >= 0) {
                    cfg_msg.__raw = 0;
                    cfg_msg.vnic = curr_vnic;
                    cfg_msg.msg_valid = 1;

                    nfd_cfg_parse_msg((void *) &cfg_msg, NFD_CFG_PCI_IN0);
                }
            } else {
                service_qc_vnic_setup(&cfg_msg);

                if (!cfg_msg.msg_valid) {
                    nfd_cfg_start_cfg_msg(&cfg_msg,
                                          &nfd_cfg_sig_pci_in1,
                                          NFD_CFG_NEXT_ME(PCIE_ISL, 3),
                                          NFD_CFG_RING_NUM(PCIE_ISL, 0));
                }
            }

            /* Yield thread */
            ctx_swap();
        }
    } else {
        /* Worker main loop */
        for (;;) {
            gather();

            notify();

            /* Yield thread */
            /* ctx_swap(); */
        }
    }
}
