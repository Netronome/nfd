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
#include <vnic/shared/nfd_cfg_internal.c>

#ifdef SVC_ME_MSIX_EN
#include <msix_gen.h>
#endif

__visible SIGNAL nfd_cfg_sig_svc_me1;

struct nfd_cfg_msg cfg_msg;

__xread unsigned int cfg_bar_data[6];
#ifdef SVC_ME_MSIX_EN
__xread unsigned int rx_ring_vector_data[16];
#endif

NFD_CFG_BASE_DECLARE(PCIE_ISL);

int
main(void)
{
    if (ctx() == 0) {
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me1, &cfg_msg);

    } 

#ifdef SVC_ME_MSIX_EN
    if (ctx() == 1) {
        rx_queue_monitor_init();
    }
#endif

    for (;;) {
        if (ctx() == 0) {
            if (!cfg_msg.msg_valid) {
                nfd_cfg_check_cfg_msg(&cfg_msg, &nfd_cfg_sig_svc_me1,
                                           NFD_CFG_RING_NUM(PCIE_ISL, 2));

                if (cfg_msg.msg_valid) {
                    mem_read64(cfg_bar_data,
                               NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg.vnic),
                               sizeof cfg_bar_data);

#ifdef SVC_ME_MSIX_EN
                    mem_read64(rx_ring_vector_data,
                               NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg.vnic)+0xa40,
                               sizeof rx_ring_vector_data);
#endif
                }
            } else {
              
 
                __implicit_read(cfg_bar_data, 6);
#ifdef SVC_ME_MSIX_EN
                rx_queue_monitor_update_config(cfg_msg.vnic, cfg_bar_data, rx_ring_vector_data);
#endif
                local_csr_write(NFP_MECSR_MAILBOX_0, cfg_msg.vnic);
                local_csr_write(NFP_MECSR_MAILBOX_1, cfg_bar_data[0]);

                /* Complete the message */
                cfg_msg.msg_valid = 0;
                nfd_cfg_complete_cfg_msg(&cfg_msg,
                                         &NFD_CFG_SIG_NEXT_ME,
                                         NFD_CFG_NEXT_ME,
                                         NFD_CFG_RING_NUM(PCIE_ISL, 3),
                                         NFD_CFG_RING_NUM(PCIE_ISL, 2));
            }
        }

#ifdef SVC_ME_MSIX_EN
        if (ctx() == 1) {
            rx_queue_monitor();
        }
#endif

        ctx_swap();

    }
}




