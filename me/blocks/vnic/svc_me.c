/*
 * Copyright (C) 2015-2019,  Netronome Systems, Inc.  All rights reserved.
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
 * @file   svc_me.c
 * @brief  Main entry point for the service ME
 */

#include <nfp6000/nfp_me.h>

#include <assert.h>
#include <nfp.h>
#include <nfp_chipres.h>

#include <nfp/me.h>
#include <nfp/mem_bulk.h>
#include <nfp/mem_ring.h>

#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_flr.c>
#include <vnic/shared/nfd_vf_cfg_iface.h>
#include <vnic/svc/msix.h>


/* Signal used for reconfiguration synchronisation */
#define SVC_RECONFIG_SIG_NUM    15
#include <vnic/svc/msix_qmon.c>


/*
 * The service ME performs a number of different functions.
 *
 * Context  0:   Handling configuration messages
 * Contexts 1-4: Monitor RX/TX queues and generate MSI-X (one per PCIe island)
 */


/* Journal CFG messages that the service ME sees */
#ifndef NFD_CFG_MSG_JRNL_SZ
#define NFD_CFG_MSG_JRNL_SZ 65536
#endif
MEM_RING_INIT(nfd_cfg_msg_jrnl, (NFD_CFG_MSG_JRNL_SZ));



struct nfd_cfg_msg cfg_msg;
__xread unsigned int cfg_bar_data[6];


#ifdef NFD_PCIE0_EMEM
SIGNAL nfd_cfg_sig_svc_me0;
__xread struct nfd_cfg_msg cfg_msg_rd0;

NFD_CFG_BASE_DECLARE(0);
NFD_VF_CFG_DECLARE(0);
NFD_VF_CFG_INIT(0);
#ifdef NFD_USE_TLV
NFD_TLV_BASE_DECLARE(0);
#endif

PCIE_C2P_BAR_ALLOC_OFF(nfd_scv_qmon_bar0, me, 0, PCIE_CPP2PCIE_QMON, 1);
#endif

#ifdef NFD_PCIE1_EMEM
SIGNAL nfd_cfg_sig_svc_me1;
__xread struct nfd_cfg_msg cfg_msg_rd1;

NFD_CFG_BASE_DECLARE(1);
NFD_VF_CFG_DECLARE(1);
NFD_VF_CFG_INIT(1);
#ifdef NFD_USE_TLV
NFD_TLV_BASE_DECLARE(1);
#endif

PCIE_C2P_BAR_ALLOC_OFF(nfd_svc_qmon_bar1, me, 1, PCIE_CPP2PCIE_QMON, 1);
#endif

#ifdef NFD_PCIE2_EMEM
SIGNAL nfd_cfg_sig_svc_me2;
__xread struct nfd_cfg_msg cfg_msg_rd2;

NFD_CFG_BASE_DECLARE(2);
NFD_VF_CFG_DECLARE(2);
NFD_VF_CFG_INIT(2);
#ifdef NFD_USE_TLV
NFD_TLV_BASE_DECLARE(2);
#endif

PCIE_C2P_BAR_ALLOC_OFF(nfd_svc_qmon_bar2, me, 2, PCIE_CPP2PCIE_QMON, 1);
#endif

#ifdef NFD_PCIE3_EMEM
SIGNAL nfd_cfg_sig_svc_me3;
__xread struct nfd_cfg_msg cfg_msg_rd3;

NFD_CFG_BASE_DECLARE(3);
NFD_VF_CFG_DECLARE(3);
NFD_VF_CFG_INIT(3);
#ifdef NFD_USE_TLV
NFD_TLV_BASE_DECLARE(3);
#endif

PCIE_C2P_BAR_ALLOC_OFF(nfd_svc_qmon_bar3, me, 3, PCIE_CPP2PCIE_QMON, 1);
#endif

NFD_FLR_DECLARE;
NFD_VF_CFG_MAX_VFS;

#define CHECK_CFG_MSG(_isl)                                                 \
do {                                                                        \
    nfd_cfg_check_cfg_msg(&cfg_msg, NFD_CFG_RING_NUM(_isl, 4),              \
                          &cfg_msg_rd##_isl, &nfd_cfg_sig_svc_me##_isl);    \
    if (cfg_msg.msg_valid) {                                                \
        ncfg++;                                                             \
        mem_read64(cfg_bar_data,                                            \
                   NFD_CFG_BAR_ISL(_isl, cfg_msg.vid),                      \
                   sizeof cfg_bar_data);                                    \
                                                                            \
        /* Journal cfg messages */                                          \
        /* XXX Tag the message with "_isl + ring num" */                    \
        JDBG(nfd_cfg_msg_jrnl, ((_isl) << 4) | 4);                          \
        JDBG(nfd_cfg_msg_jrnl, cfg_msg.__raw);                              \
        JDBG(nfd_cfg_msg_jrnl, cfg_bar_data[0]);                            \
        JDBG(nfd_cfg_msg_jrnl, cfg_bar_data[1]);                            \
                                                                            \
        msix_qmon_reconfig(_isl, cfg_msg.vid,                               \
                           NFD_CFG_BAR_ISL(_isl, cfg_msg.vid),              \
                           cfg_bar_data);                                   \
                                                                            \
        /* Handle FLRs */                                                   \
        if (cfg_bar_data[1] & NFP_NET_CFG_UPDATE_RESET) {                   \
                                                                            \
            /* NB: This function writes ~8K of data */                      \
            nfd_flr_clr_bar(NFD_CFG_BAR_ISL(_isl, cfg_msg.vid));            \
            nfd_flr_init_cfg_queue(_isl, cfg_msg.vid,                       \
                                   PCIE_QC_EVENT_NOT_EMPTY);                \
                                                                            \
            if (NFD_VID_IS_PF(cfg_msg.vid)) {                               \
                nfd_flr_init_pf_cfg_bar(_isl, cfg_msg.vid);                 \
            } else if (NFD_VID_IS_VF(cfg_msg.vid)) {                        \
                nfd_flr_init_vf_cfg_bar(NFD_VF_CFG_BASE_LINK(_isl), _isl,   \
                                        cfg_msg.vid);                       \
            } else {                                                        \
                nfd_flr_init_ctrl_cfg_bar(_isl, cfg_msg.vid);               \
            }                                                               \
        }                                                                   \
                                                                            \
        /* Handle PCIe island resets */                                     \
        if (cfg_msg.pci_reset) {                                            \
            /* NB: This function writes ~8K of data */                      \
            nfd_flr_clr_bar(NFD_CFG_BAR_ISL(_isl, cfg_msg.vid));            \
                                                                            \
            if (NFD_VID_IS_PF(cfg_msg.vid)) {                               \
                nfd_flr_init_pf_cfg_bar(_isl, cfg_msg.vid);                 \
            } else if (NFD_VID_IS_VF(cfg_msg.vid)) {                        \
                nfd_flr_init_vf_cfg_bar(NFD_VF_CFG_BASE_LINK(_isl), _isl,   \
                                        cfg_msg.vid);                       \
            } else {                                                        \
                nfd_flr_init_ctrl_cfg_bar(_isl, cfg_msg.vid);               \
            }                                                               \
                                                                            \
            if (cfg_msg.vid == 0) {                                         \
                /* This is the start of PCIe island reset */                \
                /* processing. */                                           \
                                                                            \
                /* Enable MasterDropIfDisabled to allow pcie[write] */      \
                /* to complete while the island is in reset */              \
                nfd_flr_update_mstr_drop_if_disabled(_isl, 1);              \
            }                                                               \
                                                                            \
            if (cfg_msg.vid == NFD_LAST_PF) {                               \
                /* This is the end of PCIe island reset */                  \
                /* processing. */                                           \
                                                                            \
                /* Clear msix_cur_cpp2pci_addr in case the BAR is reset */  \
                msix_rst_curr_cpp2pci_addr(_isl);                           \
            }                                                               \
        }                                                                   \
                                                                            \
                                                                            \
        /* Complete the message */                                          \
        cfg_msg.msg_valid = 0;                                              \
        nfd_cfg_complete_cfg_msg(&cfg_msg, NFD_CFG_RING_NUM(_isl, 5));      \
    }                                                                       \
} while (0)


int
main(void)
{
    /* Check configuration messages */
    if (ctx() == 0) {
        int ncfg = 0;

        /* Initialisation */
#ifdef NFD_PCIE0_EMEM
        MSIX_INIT_ISL(0);
        nfd_cfg_init_cfg_msg(&cfg_msg, NFD_CFG_RING_NUM(0, 4), &cfg_msg_rd0,
                             &nfd_cfg_sig_svc_me0);
        msix_qmon_init(0);
#endif

#ifdef NFD_PCIE1_EMEM
        MSIX_INIT_ISL(1);
        nfd_cfg_init_cfg_msg(&cfg_msg, NFD_CFG_RING_NUM(1, 4), &cfg_msg_rd1,
                             &nfd_cfg_sig_svc_me1);
        msix_qmon_init(1);
#endif

#ifdef NFD_PCIE2_EMEM
        MSIX_INIT_ISL(2);
        nfd_cfg_init_cfg_msg(&cfg_msg, NFD_CFG_RING_NUM(2, 4), &cfg_msg_rd2,
                             &nfd_cfg_sig_svc_me2);
        msix_qmon_init(2);
#endif

#ifdef NFD_PCIE3_EMEM
        MSIX_INIT_ISL(3);
        nfd_cfg_init_cfg_msg(&cfg_msg, NFD_CFG_RING_NUM(3, 4), &cfg_msg_rd3,
                             &nfd_cfg_sig_svc_me3);
        msix_qmon_init(3);
#endif


        /* Main loop */
        for (;;) {
#ifdef NFD_PCIE0_EMEM
            CHECK_CFG_MSG(0);
#endif

#ifdef NFD_PCIE1_EMEM
            CHECK_CFG_MSG(1);
#endif

#ifdef NFD_PCIE2_EMEM
            CHECK_CFG_MSG(2);
#endif

#ifdef NFD_PCIE3_EMEM
            CHECK_CFG_MSG(3);
#endif

            ctx_swap();
        }
    }

#ifdef NFD_PCIE0_EMEM
    if (ctx() == 1)
        msix_qmon_loop(0);
#endif

#ifdef NFD_PCIE1_EMEM
    if (ctx() == 2)
        msix_qmon_loop(1);
#endif

#ifdef NFD_PCIE2_EMEM
    if (ctx() == 3)
        msix_qmon_loop(2);
#endif

#ifdef NFD_PCIE3_EMEM
    if (ctx() == 4)
        msix_qmon_loop(3);
#endif

    /* Catch and kill unused threads */
    for (;;) {
        ctx_wait(kill);
    }
}
