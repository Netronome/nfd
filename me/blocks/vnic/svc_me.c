/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/svc_me.c
 * @brief         Service ME
 */

#include <assert.h>
#include <vnic/shared/nfcc_chipres.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_bulk.h>

#include <nfp6000/nfp_me.h>

#include "shared/nfd_cfg.h"
#include "utils/msix_gen.h"

#ifdef NFD_PCIE0_EMEM
__visible SIGNAL nfd_cfg_sig_svc_me0;
__remote SIGNAL NFD_CFG_SIG_NEXT_ME0;
__xread unsigned int cfg_bar_data0[6];
struct nfd_cfg_msg cfg_msg0;

NFD_CFG_BASE_DECLARE(0);
#endif

#ifdef NFD_PCIE1_EMEM
__visible SIGNAL nfd_cfg_sig_svc_me1;
__remote SIGNAL NFD_CFG_SIG_NEXT_ME1;
__xread unsigned int cfg_bar_data1[6];
struct nfd_cfg_msg cfg_msg1;

NFD_CFG_BASE_DECLARE(1);
#endif

#ifdef NFD_PCIE2_EMEM
__visible SIGNAL nfd_cfg_sig_svc_me2;
__remote SIGNAL NFD_CFG_SIG_NEXT_ME2;
__xread unsigned int cfg_bar_data2[6];
struct nfd_cfg_msg cfg_msg2;

NFD_CFG_BASE_DECLARE(2);
#endif

#ifdef NFD_PCIE3_EMEM
__visible SIGNAL nfd_cfg_sig_svc_me3;
__remote SIGNAL NFD_CFG_SIG_NEXT_ME3;
__xread unsigned int cfg_bar_data3[6];
struct nfd_cfg_msg cfg_msg3;

NFD_CFG_BASE_DECLARE(3);
#endif


#define CHECK_CFG_MSG(_isl)                                             \
do {                                                                    \
    nfd_cfg_check_cfg_msg(&cfg_msg##_isl, &nfd_cfg_sig_svc_me##_isl,    \
                          NFD_CFG_RING_NUM(_isl, 2));                   \
    if (cfg_msg##_isl.msg_valid) {                                      \
        ncfg++;                                                         \
        mem_read64(cfg_bar_data##_isl,                                  \
                   NFD_CFG_BAR_ISL(_isl, cfg_msg##_isl.vnic),           \
                   sizeof cfg_bar_data##_isl);                          \
                                                                        \
        local_csr_write(NFP_MECSR_MAILBOX_0, (0x0CF | (_isl << 8)));    \
        local_csr_write(NFP_MECSR_MAILBOX_1, cfg_msg##_isl.vnic);       \
        local_csr_write(NFP_MECSR_MAILBOX_2, cfg_bar_data##_isl[0]);    \
        local_csr_write(NFP_MECSR_MAILBOX_3, cfg_bar_data##_isl[1]);    \
                                                                        \
        msix_reconfig(_isl, cfg_msg##_isl.vnic,                         \
                      NFD_CFG_BAR_ISL(_isl, cfg_msg##_isl.vnic),        \
                      cfg_bar_data##_isl);                              \
                                                                        \
        /* Complete the message */                                      \
        cfg_msg##_isl.msg_valid = 0;                                    \
        nfd_cfg_svc_complete_cfg_msg(&cfg_msg##_isl,                    \
                                     &NFD_CFG_SIG_NEXT_ME##_isl,        \
                                     NFD_CFG_NEXT_ME##_isl,             \
                                     NFD_CFG_RING_NUM(_isl, 3),         \
                                     NFD_CFG_RING_NUM(_isl, 2));        \
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
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me0, &cfg_msg0);
#endif

#ifdef NFD_PCIE1_EMEM
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me1, &cfg_msg1);
#endif

#ifdef NFD_PCIE2_EMEM
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me2, &cfg_msg2);
#endif

#ifdef NFD_PCIE3_EMEM
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me3, &cfg_msg3);
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
    if (ctx() == 1) {
        msix_qmon_init(0);
        msix_qmon_loop(0);
    }
#endif

#ifdef NFD_PCIE1_EMEM
    if (ctx() == 2) {
        msix_qmon_init(1);
        msix_qmon_loop(1);
    }
#endif

#ifdef NFD_PCIE2_EMEM
    if (ctx() == 3) {
        msix_qmon_init(2);
        msix_qmon_loop(2);
    }
#endif

#ifdef NFD_PCIE3_EMEM
    if (ctx() == 4) {
        msix_qmon_init(3);
        msix_qmon_loop(3);
    }
#endif

    /* Catch and kill unused threads */
    for (;;) {
        ctx_wait(kill);
    }
}
