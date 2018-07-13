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
#include <vnic/shared/nfd_rst_state.h>

NFD_INIT_DONE_DECLARE;

SIGNAL cfg_msg_sig;
struct nfd_cfg_msg cfg_msg;
__xread struct nfd_cfg_msg cfg_msg_rd;


#ifdef NFD_USER_CTX_DECL
NFD_USER_CTX_DECL(PCIE_ISL);
#endif

#ifndef NFD_OUT_PD_ME0
#error "NFD_OUT_PD_ME0 is not defined."
#endif

#ifndef NFD_OUT_PD_ME1
#error "NFD_OUT_PD_ME1 is not defined."
#endif

#ifdef NFD_OUT_3_PD_MES
#ifndef NFD_OUT_PD_ME2
#error "NFD_OUT_PD_ME2 is not defined."
#endif
#endif


/**
 * Define mask values to reflect requirements of the ct[nn_write] command
 * Define macros to construct appropriate address
 *
 * address[29:24]  Island number of target XPB device
 * address[20:17]  ME ID with the Island
 * address[16:10]  Signal number within the ME to set on completion
 * address[9]      Address mode
 * address[8:2]    NN register number
 */
#define CT_REFLECT_ISL_MASK    ((1 << 6) - 1)
#define CT_REFLECT_MASTER_MASK ((1 << 4) - 1)
#define CT_REFLECT_SIGNAL_MASK ((1 << 7) - 1)
#define CT_REFLECT_NNREG_MASK  ((1 << 7) - 1)

#define PCI_OUT_PD_ADDR(_meid)                          \
    ((((_meid >> 4) & CT_REFLECT_ISL_MASK) << 24) |     \
    (((_meid & 0xf) & CT_REFLECT_MASTER_MASK) << 17))

#define PCI_OUT_NN_ADDR(_sig, _abs, _addr)              \
    (((_abs) & 1) << 9) |                               \
    (((_sig) & CT_REFLECT_SIGNAL_MASK) << 10) |         \
    (((_addr) & CT_REFLECT_NNREG_MASK) << 2)


#define PCI_OUT_MSG_UP  0
#define PCI_OUT_MSG_RST (1 << NFD_OUT_PD_RST_BIT)

/**
 * Message PD MEs with current island state (up / reset)
 * @param isl_reset     True if island is reset
 *
 * This function swaps once for all NN reflects
 */
__intrinsic void
pci_out_msg_pd(unsigned int isl_reset)
{
    unsigned int addr;
    unsigned int addr_part;
    unsigned int sig_no;
    __xwrite unsigned int data;
    SIGNAL nn_sig;

    if (isl_reset) {
        sig_no = (NFD_OUT_PD_RST_CTX << 4) | NFD_OUT_PD_RST_SIG_NO;
        data = (1 << NFD_OUT_PD_RST_BIT);
    } else {
        sig_no = 0;
        data = 0;
    }

    addr_part = PCI_OUT_NN_ADDR(sig_no, 1,
                                ((NFD_OUT_PD_RST_CTX << 4) |
                                 NFD_OUT_PD_RST_NN_OFF));

    addr = PCI_OUT_PD_ADDR(NFD_OUT_PD_ME0) | addr_part;

    __asm { ct[ctnn_write, data, 0, addr, 1] };

    addr = PCI_OUT_PD_ADDR(NFD_OUT_PD_ME1) | addr_part;

#ifndef NFD_OUT_PD_ME2
    __asm { ct[ctnn_write, data, 0, addr, 1], ctx_swap[nn_sig] };
#else
    __asm { ct[ctnn_write, data, 0, addr, 1] };

    addr = PCI_OUT_PD_ADDR(NFD_OUT_PD_ME2) | addr_part;

    __asm { ct[ctnn_write, data, 0, addr, 1], ctx_swap[nn_sig] };
#endif
}


int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        nfd_cfg_check_pcie_link(); /* Will halt ME on failure */

        nfd_cfg_init_cfg_msg(&cfg_msg, NFD_CFG_RING_NUM(PCIE_ISL, 2),
                             &cfg_msg_rd, &cfg_msg_sig);


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

            cache_desc_check_urgent(); /* Swaps min once */

            cache_desc_status();
            ctx_swap();

            cache_desc_check_active(); /* Swaps min once */

            /* Either check for a message, or perform one tick of processing
             * on the message each loop iteration */
            if (!cfg_msg.msg_valid) {
                nfd_cfg_check_cfg_msg(&cfg_msg, NFD_CFG_RING_NUM(PCIE_ISL, 2),
                                      &cfg_msg_rd, &cfg_msg_sig);

                if (cfg_msg.msg_valid) {
                    if (cfg_msg.pci_reset) {
                        if (cfg_msg.vid == 0) {
                            /* Set reset state and message PD */
                            nfd_rst_state_set_rst(PCIE_ISL);
                            pci_out_msg_pd(PCI_OUT_MSG_RST);
                        }
                    } else {
                        if (NFD_RST_STATE_TEST_RST(PCIE_ISL)) {
                            /* Clear reset state and message PD */
                            nfd_rst_state_set_up(PCIE_ISL);
                            pci_out_msg_pd(PCI_OUT_MSG_UP);
                        }
                    }

                    nfd_cfg_parse_msg((void *) &cfg_msg, NFD_CFG_PCI_OUT);
                }
            } else {
                cache_desc_vnic_setup((void *) &cfg_msg);

                if (!cfg_msg.msg_valid) {
                    if (cfg_msg.pci_reset && (cfg_msg.vid == NFD_LAST_PF)) {
                        cache_desc_compl_rst();
                    }

                    nfd_cfg_complete_cfg_msg(&cfg_msg,
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

            send_desc_check_cached(); /* Swaps min once */

            send_desc_check_cached(); /* Swaps min once */

            send_desc_check_pending();  /* Swaps min once */

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
