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

#include <vnic/shared/nfd_cfg.h>

#ifdef SVC_ME_MSIX_EN
#include <vnic/utils/msix_gen.h>
__xread unsigned int rx_ring_vector_data[16];
#define VNIC_CONFIG_RX_VECS 0xa40
#endif

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


#ifdef SVC_ME_MSIX_EN
#define MSIX_CFG_MSG_PROC_IND(_isl)                                     \
    mem_read64(rx_ring_vector_data,                                     \
               (NFD_CFG_BAR_ISL(_isl, cfg_msg##_isl.vnic) +             \
                VNIC_CONFIG_RX_VECS),                                   \
               sizeof rx_ring_vector_data);                             \
    rx_queue_monitor_update_config(_isl, cfg_msg##_isl.vnic,            \
                                   cfg_bar_data##_isl, rx_ring_vector_data);

#else
#define MSIX_CFG_MSG_PROC_IND(_isl)
#endif


#define CHECK_CFG_MSG(_isl)                                             \
do {                                                                    \
    nfd_cfg_check_cfg_msg(&cfg_msg##_isl, &nfd_cfg_sig_svc_me##_isl,    \
                          NFD_CFG_RING_NUM(_isl, 2));                   \
                                                                        \
    if (cfg_msg##_isl.msg_valid) {                                      \
        ncfg++;                                                         \
        mem_read64(cfg_bar_data##_isl,                                  \
                   NFD_CFG_BAR_ISL(_isl, cfg_msg##_isl.vnic),           \
                   sizeof cfg_bar_data##_isl);                          \
                                                                        \
        MSIX_CFG_MSG_PROC_IND(_isl);                                    \
                                                                        \
        local_csr_write(NFP_MECSR_MAILBOX_0, (0x0CF | (_isl << 8)));    \
        local_csr_write(NFP_MECSR_MAILBOX_1, cfg_msg##_isl.vnic);       \
        local_csr_write(NFP_MECSR_MAILBOX_2, cfg_bar_data##_isl[0]);    \
        local_csr_write(NFP_MECSR_MAILBOX_3, ncfg);                     \
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


#ifdef SVC_ME_MSIX_EN

#ifdef NFD_PCIE0_EMEM
    if (ctx() == 1) {
        rx_queue_monitor_init(0);

        for (;;) {
            rx_queue_monitor(0);

            ctx_swap();
        }
    }
#endif

#ifdef NFD_PCIE1_EMEM
    if (ctx() == 2) {
        rx_queue_monitor_init(1);

        for (;;) {
            rx_queue_monitor(1);

            ctx_swap();
        }
    }
#endif

#ifdef NFD_PCIE2_EMEM
    if (ctx() == 3) {
        rx_queue_monitor_init(2);

        for (;;) {
            rx_queue_monitor(2);

            ctx_swap();
        }
    }
#endif

#ifdef NFD_PCIE3_EMEM
    if (ctx() == 4) {
        rx_queue_monitor_init(3);

        for (;;) {
            rx_queue_monitor(3);

            ctx_swap();
        }
    }
#endif

#endif


    /* Catch and kill unused threads */
    for (;;) {
        ctx_wait(kill);
    }
}

