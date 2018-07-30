/*
 * Copyright (C) 2014-2018,  Netronome Systems, Inc.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file          blocks/vnic/pci_in_me0.c
 * @brief         Code to deploy on PCIe ME2 in NFD with 2 MEs per direction
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/remote_me.h>

#include <nfp6000/nfp_qc.h>

#include <vnic/pci_in/gather.c>
#include <vnic/pci_in/gather_status.c>
#include <vnic/pci_in/service_qc.c>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/shared/nfd_cfg_internal.c>
#include <vnic/shared/nfd_cfg_pcie_monitor.c>
#include <vnic/shared/nfd_rst_state.h>

#if NFD_CFG_CLASS != NFD_CFG_CLASS_DEFAULT
#pragma message( "Non-zero (default) NFD_CFG_CLASS set!" )
#pragma message( "Please ensure that the above class has been correctly" )
#pragma message( "reserved for this product." )
#endif


/*
 * Temporary header includes
 */
#include <std/event.h>                  /* TEMP */

NFD_INIT_DONE_DECLARE;

NFD_CFG_VF_DECLARE(PCIE_ISL);
NFD_CFG_CTRL_DECLARE(PCIE_ISL);
NFD_CFG_PF_DECLARE(PCIE_ISL);

struct nfd_cfg_msg cfg_msg;

/* Signal used to poll for PCIe reset ack */
volatile SIGNAL nfd_in_gather_poll_rst_sig;
SIGNAL pcie_monitor_rst_sig;

/* Setup _pf%d_net_app_id */
NFD_NET_APP_ID_DECLARE(PCIE_ISL);


/* Data to reflect reset to Notify */
#define NFD_IN_MSG_NOTIFY_RST   (1 << NFD_IN_DMA_STATE_INVALID_shf)
#define NFD_IN_MSG_NOTIFY_UP    0
__xwrite unsigned int notify_reset_state;

__intrinsic void
pci_in_msg_notify(unsigned int isl_reset)
{
    if (isl_reset)
        notify_reset_state = NFD_IN_MSG_NOTIFY_RST;
    else
        notify_reset_state = NFD_IN_MSG_NOTIFY_UP;

    remote_me_reg_write_signal_local(&notify_reset_state,
                                     ((NFD_IN_NOTIFY_ME & 0xFF0) >> 4),
                                     ((NFD_IN_NOTIFY_ME & 0xF) - 4), 0,
                                     NFD_IN_NOTIFY_RESET_RD,
                                     sizeof notify_reset_state);
}


int
main(void)
{
    /*
     * TEMP use a bit in a "status" GR to make threads wait
     * for initialisation to complete
     */
    #define STATUS_INIT_DONE_BIT 0
    #define STATUS_ISL_READY_BIT 1
    volatile __shared __gpr unsigned int status = 0;

    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        nfd_cfg_check_pcie_clock(); /* Will halt ME on failure */

        /* Initialisation that does not swap */
        cfg_msg.__raw = 0;
        gather_setup_shared();
        gather_status_setup();

        /* Initialisation that swaps and takes longer */
        service_qc_setup();
        distr_gather_setup_shared();
        nfd_cfg_setup();

        /* TEMP: Mark initialisation complete */
        status |= (1<<STATUS_INIT_DONE_BIT);

        NFD_INIT_DONE_SET(PCIE_ISL, 1);     /* XXX Remove? */
    } else if (ctx() == 1) {
        __assign_relative_register(&pcie_monitor_rst_sig,
                                   NFD_CFG_PCIE_RST_SIG);
        __implicit_write(&pcie_monitor_rst_sig);

        nfd_cfg_pcie_monitor_ver_check();
        nfd_cfg_pcie_monitor_write_sig(__signal_number(&pcie_monitor_rst_sig));
        nfd_cfg_flr_setup();

    } else {
        gather_setup();

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
            /* CTX1 can block other contexts while waiting for
             * the PCIe island to be ready */
            while (!bit_test(status, STATUS_ISL_READY_BIT)) {
                ctx_swap();
            }

            service_qc();

            distr_gather();

            gather_status();

            /* Either check for a message, or perform one tick of processing
             * on the message each loop iteration */
            if (!cfg_msg.msg_valid) {
                int curr_vid;

                curr_vid = nfd_cfg_next_flr((void *) &cfg_msg);

                if (curr_vid < 0) {
                    /* No FLRs to process, look for a host message. */
                    curr_vid = nfd_cfg_next_vnic();

                    if (curr_vid >= 0) {
                        cfg_msg.__raw = 0;
                        cfg_msg.vid = curr_vid;
                        cfg_msg.msg_valid = 1;

                        nfd_cfg_parse_msg((void *) &cfg_msg, NFD_CFG_PCI_IN0);
                    }
                }
            } else {
                service_qc_vnic_setup(&cfg_msg);

                if (!cfg_msg.msg_valid) {
                    if (cfg_msg.pci_reset) {
                        if (cfg_msg.vid == 0) {
                            /* Set reset state and message notify */
                            nfd_rst_state_set_rst(PCIE_ISL);
                            /* Kick off PCIe reset ack polling */
                            signal_ctx(
                                NFD_IN_GATHER_CTX_RST_POLL,
                                __signal_number(&nfd_in_gather_poll_rst_sig));
                            pci_in_msg_notify(NFD_IN_MSG_NOTIFY_RST);
                        }
                    } else {
                        if (NFD_RST_STATE_TEST_RST(PCIE_ISL)) {
                            /* Clear reset state */
                            nfd_rst_state_set_up(PCIE_ISL);
                            pci_in_msg_notify(NFD_IN_MSG_NOTIFY_UP);
                        }
                    }

                    nfd_cfg_start_cfg_msg(&cfg_msg,
                                          NFD_CFG_RING_NUM(PCIE_ISL, 0));
                }
            }

            /* Yield thread */
            ctx_swap();
        }
    } else if (ctx() == 1) {
        for (;;) {
            int rst_ack;

            /* Wait for the link to be ready and finalise PCIe settings */
            while (!nfd_cfg_pcie_monitor_link_up()) {
                sleep(NFD_CFG_PCIE_POLL_CYCLES);
            }

            nfd_cfg_pcie_monitor_stop();
            nfd_cfg_pci_reconfig();
            nfd_cfg_set_curr_pcie_sts();

            /* Allow other threads to run */
            status |= (1<<STATUS_ISL_READY_BIT);

            /* The link is good, start regular processing */
            while (1) {
                nfd_cfg_check_flr_ap();
                if (signal_test(&pcie_monitor_rst_sig)) {
                    /* TEMP integration test
                     * halt if we find the link up at this point */
                    if (nfd_cfg_pcie_monitor_link_up()) {
                        /* Temp error value */
                        local_csr_write(local_csr_mailbox_0, 0x8aa);
                        halt();
                    }

                    nfd_flr_throw_link_rst(PCIE_ISL, &flr_pend_status);
                }

                rst_ack = nfd_cfg_poll_rst_ack(&nfd_in_gather_poll_rst_sig);
                if (rst_ack) {
                    /* We have finished a reset, finalise our ME
                     * and return to waiting for link */
                    gather_compl_rst();

                    /* Stop other threads running and then restart
                     * the PCIe monitor */
                    status &= ~(1<<STATUS_ISL_READY_BIT);
                    nfd_cfg_pcie_monitor_start();
                    break;
                }
                ctx_swap();
            }
        }
    } else {
        /* Worker main loop */
        for (;;) {
            /* CTX1 can block other contexts while waiting for
             * the PCIe island to be ready */
            while (!bit_test(status, STATUS_ISL_READY_BIT)) {
                ctx_swap();
            }

            gather();

            /* Yield thread */
            ctx_swap();
        }
    }
}
