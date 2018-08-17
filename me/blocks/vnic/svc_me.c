/*
 * Copyright (C) 2015-2016,  Netronome Systems, Inc.  All rights reserved.
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
#include <assert.h>
#include <nfp.h>
#include <nfp_chipres.h>

#include <nfp/me.h>
#include <nfp/mem_bulk.h>

#include <nfp6000/nfp_me.h>

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

#ifdef NFD_PCIE0_EMEM
__visible SIGNAL nfd_cfg_sig_svc_me0;
__remote SIGNAL NFD_CFG_SIG_NEXT_ME0;
__xread unsigned int cfg_bar_data0[6];
struct nfd_cfg_msg cfg_msg0;

NFD_CFG_BASE_DECLARE(0);
NFD_VF_CFG_DECLARE(0);
NFD_VF_CFG_INIT(0);
#ifdef NFD_USE_TLV
NFD_TLV_BASE_DECLARE(0);
#endif

PCIE_C2P_BAR_ALLOC_OFF(nfd_scv_qmon_bar0, me, 0, PCIE_CPP2PCIE_QMON, 1);
#endif

#ifdef NFD_PCIE1_EMEM
__visible SIGNAL nfd_cfg_sig_svc_me1;
__remote SIGNAL NFD_CFG_SIG_NEXT_ME1;
__xread unsigned int cfg_bar_data1[6];
struct nfd_cfg_msg cfg_msg1;

NFD_CFG_BASE_DECLARE(1);
NFD_VF_CFG_DECLARE(1);
NFD_VF_CFG_INIT(1);
#ifdef NFD_USE_TLV
NFD_TLV_BASE_DECLARE(1);
#endif

PCIE_C2P_BAR_ALLOC_OFF(nfd_svc_qmon_bar1, me, 1, PCIE_CPP2PCIE_QMON, 1);
#endif

#ifdef NFD_PCIE2_EMEM
__visible SIGNAL nfd_cfg_sig_svc_me2;
__remote SIGNAL NFD_CFG_SIG_NEXT_ME2;
__xread unsigned int cfg_bar_data2[6];
struct nfd_cfg_msg cfg_msg2;

NFD_CFG_BASE_DECLARE(2);
NFD_VF_CFG_DECLARE(2);
NFD_VF_CFG_INIT(2);
#ifdef NFD_USE_TLV
NFD_TLV_BASE_DECLARE(2);
#endif

PCIE_C2P_BAR_ALLOC_OFF(nfd_svc_qmon_bar2, me, 2, PCIE_CPP2PCIE_QMON, 1);
#endif

#ifdef NFD_PCIE3_EMEM
__visible SIGNAL nfd_cfg_sig_svc_me3;
__remote SIGNAL NFD_CFG_SIG_NEXT_ME3;
__xread unsigned int cfg_bar_data3[6];
struct nfd_cfg_msg cfg_msg3;

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

#define CHECK_CFG_MSG(_isl)                                             \
do {                                                                    \
    nfd_cfg_check_cfg_msg(&cfg_msg##_isl, &nfd_cfg_sig_svc_me##_isl,    \
                          NFD_CFG_RING_NUM(_isl, 4));                   \
    if (cfg_msg##_isl.msg_valid) {                                      \
        ncfg++;                                                         \
        mem_read64(cfg_bar_data##_isl,                                  \
                   NFD_CFG_BAR_ISL(_isl, cfg_msg##_isl.vid),            \
                   sizeof cfg_bar_data##_isl);                          \
                                                                        \
        msix_qmon_reconfig(_isl, cfg_msg##_isl.vid,                     \
                           NFD_CFG_BAR_ISL(_isl, cfg_msg##_isl.vid),    \
                           cfg_bar_data##_isl);                         \
                                                                        \
        /* Handle FLRs */                                               \
        if (cfg_bar_data##_isl[1] & NFP_NET_CFG_UPDATE_RESET) {         \
                                                                        \
            /* NB: This function writes ~8K of data */                  \
            nfd_flr_clr_bar(NFD_CFG_BAR_ISL(_isl, cfg_msg##_isl.vid));  \
            nfd_flr_init_cfg_queue(_isl, cfg_msg##_isl.vid,             \
                                   PCIE_QC_EVENT_NOT_EMPTY);            \
                                                                        \
            if (NFD_VID_IS_PF(cfg_msg##_isl.vid)) {                     \
                /* We have a PF FLR */                                  \
                nfd_flr_init_pf_cfg_bar(_isl, cfg_msg##_isl.vid);       \
                                                                        \
            } else if (NFD_VID_IS_VF(cfg_msg##_isl.vid)) {              \
                /* We have a VF FLR */                                  \
                nfd_flr_init_vf_cfg_bar(NFD_VF_CFG_BASE_LINK(_isl),     \
                                        _isl, cfg_msg##_isl.vid);	\
                                                                        \
            } else {                                                    \
                /* We have a PF/CTRL FLR */                             \
                nfd_flr_init_ctrl_cfg_bar(_isl, cfg_msg##_isl.vid);     \
                                                                        \
            }                                                           \
        }                                                               \
                                                                        \
        /* Complete the message */                                      \
        cfg_msg##_isl.msg_valid = 0;                                    \
        nfd_cfg_complete_cfg_msg(&cfg_msg##_isl,                        \
                                 &nfd_cfg_sig_svc_me##_isl,             \
                                 &NFD_CFG_SIG_NEXT_ME##_isl,            \
                                 NFD_CFG_NEXT_ME##_isl,                 \
                                 NFD_CFG_RING_NUM(_isl, 5));            \
    }                                                                   \
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
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me0, &cfg_msg0);
        msix_qmon_init(0);
#endif

#ifdef NFD_PCIE1_EMEM
        MSIX_INIT_ISL(1);
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me1, &cfg_msg1);
        msix_qmon_init(1);
#endif

#ifdef NFD_PCIE2_EMEM
        MSIX_INIT_ISL(2);
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me2, &cfg_msg2);
        msix_qmon_init(2);
#endif

#ifdef NFD_PCIE3_EMEM
        MSIX_INIT_ISL(3);
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me3, &cfg_msg3);
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
