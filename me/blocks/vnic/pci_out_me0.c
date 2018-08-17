/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/pci_out_me0.c
 * @brief         Code to deploy on PCIe ME0 in NFD with 2 MEs per direction
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>

#include <nfp6000/nfp_qc.h>

#include <vnic/pci_out/cache_desc.c>
#include <vnic/pci_out/cache_desc_status.c>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_cfg_internal.c>

NFD_CFG_DECLARE(nfd_cfg_sig_pci_out, NFD_CFG_SIG_NEXT_ME);
NFD_INIT_DONE_DECLARE;

struct nfd_cfg_msg cfg_msg;

#ifdef NFD_USER_CTX_DECL
NFD_USER_CTX_DECL(PCIE_ISL);
#endif


int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        nfd_cfg_check_pcie_link(); /* Will halt ME on failure */

        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_pci_out, &cfg_msg);

        /* Must run before PCI.OUT host interaction, and before stage_batch */
        cache_desc_setup_shared();

        cache_desc_status_setup();

        NFD_INIT_DONE_SET(PCIE_ISL, 0);     /* XXX Remove? */
    } else if (ctx() == 1) {
        send_desc_setup_shared();

        /* send_desc uses cache_desc_compute_fl_addr
         * as a service function */
        cache_desc_setup();
#ifdef NFD_USER_CTX_INIT
    } else if (ctx() == 2) {
        NFD_USER_CTX_INIT(PCIE_ISL);
#endif
    } else {
        /* Remaining CTXs  are unused currently */
        ctx_wait(kill);
    }

    /*
     * Work loop
     */
    if (ctx() == 0) {
        /* CTX0 main loop */
        for (;;) {
            cache_desc_complete_fetch(); /* Swaps once */

            cache_desc_check_urgent(); /* Swaps at least once */

            cache_desc_status();
            ctx_swap();

            cache_desc_check_active(); /* Swaps at least once */

            /* Either check for a message, or perform one tick of processing
             * on the message each loop iteration */
            if (!cfg_msg.msg_valid) {
                nfd_cfg_check_cfg_msg(&cfg_msg, &nfd_cfg_sig_pci_out,
                                      NFD_CFG_RING_NUM(PCIE_ISL, 2));

                if (cfg_msg.msg_valid) {
                    nfd_cfg_parse_msg((void *) &cfg_msg, NFD_CFG_PCI_OUT);
                }
            } else {
                cache_desc_vnic_setup((void *) &cfg_msg);

                if (!cfg_msg.msg_valid) {
                    nfd_cfg_complete_cfg_msg(&cfg_msg, &nfd_cfg_sig_pci_out,
                                             &NFD_CFG_SIG_NEXT_ME,
                                             NFD_CFG_NEXT_ME,
                                             NFD_CFG_RING_NUM(PCIE_ISL, 3));
                }
            }

            /* Yield thread */
            ctx_swap();
        }
     } else if (ctx() == 1) {
        for (;;) {
            /* CTX1 main loop */
            send_desc_complete_send();  /* Swaps once */

            send_desc_check_cached();   /* Swaps at least once */

            send_desc_check_cached();   /* Swaps at least once */

            send_desc_check_pending();   /* Swaps at least once */

            /* ctx_swap(); */
        }
#ifdef NFD_USER_CTX_RUN
    } else if (ctx() == 2) {
        NFD_USER_CTX_RUN(PCIE_ISL);
#endif
    } else {
        /* Worker main loop */
        for (;;) {
            /* Remaining CTXs  are unused currently */
            ctx_wait(kill);
        }
    }

    /* Some context has fallen through the work loop,
     * which should be illegal, but might happen if
     * the user context misbehaves. */
    local_csr_write(local_csr_mailbox_0, NFD_OUT_USER_CTX_ERROR);
    halt();
}
