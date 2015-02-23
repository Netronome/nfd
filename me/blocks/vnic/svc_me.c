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

__visible SIGNAL nfd_cfg_sig_svc_me0;
__visible SIGNAL nfd_cfg_sig_svc_me1;
__visible SIGNAL nfd_cfg_sig_svc_me2;
__visible SIGNAL nfd_cfg_sig_svc_me3;

#if NFD_USE_PCIE0
NFD_CFG_BASE_DECLARE(0);
#endif

#if NFD_USE_PCIE1
NFD_CFG_BASE_DECLARE(1);
#endif

#if NFD_USE_PCIE2
NFD_CFG_BASE_DECLARE(2);
#endif

#if NFD_USE_PCIE3
NFD_CFG_BASE_DECLARE(3);
#endif

int
main(void)
{
    __xread unsigned int cfg_bar_data[6];
#ifdef SVC_ME_MSIX_EN
    __xread unsigned int rx_ring_vector_data[16];
#define VNIC_CONFIG_RX_VECS 0xa40
#endif

    struct nfd_cfg_msg cfg_msg;
    int i = 0;
    int ncfg = 0;

    if (ctx() == 0) {

#if NFD_USE_PCIE0
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me0, &cfg_msg);
#endif

#if NFD_USE_PCIE1
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me1, &cfg_msg);
#endif

#if NFD_USE_PCIE2
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me2, &cfg_msg);
#endif

#if NFD_USE_PCIE3
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_svc_me3, &cfg_msg);
#endif

    } 

#ifdef SVC_ME_MSIX_EN

#if NFD_USE_PCIE0
    if (ctx() == 1) {
        rx_queue_monitor_init(0);
    }
#endif

#if NFD_USE_PCIE1
    if (ctx() == 2) {
        rx_queue_monitor_init(1);
    }
#endif

#if NFD_USE_PCIE2
    if (ctx() == 3) {
        rx_queue_monitor_init(2);
    }
#endif

#if NFD_USE_PCIE3
    if (ctx() == 4) {
        rx_queue_monitor_init(3);
    }
#endif

#endif

    for (;;) {
        if (ctx() == 0) {
           switch (i) {

#if NFD_USE_PCIE0            
           case 0:  
               nfd_cfg_check_cfg_msg(&cfg_msg, &nfd_cfg_sig_svc_me0,
                                     NFD_CFG_RING_NUM(0, 2));
   
               if (cfg_msg.msg_valid) {
                   ncfg++;
                   mem_read64(cfg_bar_data,
                              NFD_CFG_BAR_ISL(0, cfg_msg.vnic),
                              sizeof cfg_bar_data);
   
#ifdef SVC_ME_MSIX_EN
                   mem_read64(rx_ring_vector_data,
                              (NFD_CFG_BAR_ISL(0, cfg_msg.vnic) + VNIC_CONFIG_RX_VECS),
                              sizeof rx_ring_vector_data);
#endif

                   local_csr_write(NFP_MECSR_MAILBOX_0, 0x0CF);
                   local_csr_write(NFP_MECSR_MAILBOX_1, cfg_msg.vnic);
                   local_csr_write(NFP_MECSR_MAILBOX_2, cfg_bar_data[0]);
                   local_csr_write(NFP_MECSR_MAILBOX_3, ncfg);

#ifdef SVC_ME_MSIX_EN
                   rx_queue_monitor_update_config(0, cfg_msg.vnic, cfg_bar_data, rx_ring_vector_data);
#endif
                   /* Complete the message */
                   cfg_msg.msg_valid = 0;
                   nfd_cfg_complete_cfg_msg(&cfg_msg,
                                            &NFD_CFG_SIG_NEXT_ME_0,
                                            NFD_CFG_NEXT_ME_0,
                                            NFD_CFG_RING_NUM(0, 3),
                                            NFD_CFG_RING_NUM(0, 2));
               }
               break;
#endif

#if NFD_USE_PCIE1            
           case 1:
               nfd_cfg_check_cfg_msg(&cfg_msg, &nfd_cfg_sig_svc_me1,
                                     NFD_CFG_RING_NUM(1, 2));

               if (cfg_msg.msg_valid) {
                   ncfg++;
                   mem_read64(cfg_bar_data,
                              NFD_CFG_BAR_ISL(1, cfg_msg.vnic),
                              sizeof cfg_bar_data);

#ifdef SVC_ME_MSIX_EN
                   mem_read64(rx_ring_vector_data,
                              (NFD_CFG_BAR_ISL(1, cfg_msg.vnic) + VNIC_CONFIG_RX_VECS),
                              sizeof rx_ring_vector_data);
#endif

                   local_csr_write(NFP_MECSR_MAILBOX_0, 0x1CF);
                   local_csr_write(NFP_MECSR_MAILBOX_1, cfg_msg.vnic);
                   local_csr_write(NFP_MECSR_MAILBOX_2, cfg_bar_data[0]);
                   local_csr_write(NFP_MECSR_MAILBOX_3, ncfg);

#ifdef SVC_ME_MSIX_EN
                   rx_queue_monitor_update_config(1, cfg_msg.vnic, cfg_bar_data, rx_ring_vector_data);
#endif
                   /* Complete the message */
                   cfg_msg.msg_valid = 0;
                   nfd_cfg_complete_cfg_msg(&cfg_msg,
                                            &NFD_CFG_SIG_NEXT_ME_1,
                                            NFD_CFG_NEXT_ME_1,
                                            NFD_CFG_RING_NUM(1, 3),
                                            NFD_CFG_RING_NUM(1, 2));
               }
               break;
#endif
            
#if NFD_USE_PCIE2            
           case 2:
               nfd_cfg_check_cfg_msg(&cfg_msg, &nfd_cfg_sig_svc_me2,
                                     NFD_CFG_RING_NUM(2, 2));

               if (cfg_msg.msg_valid) {
                   ncfg++;
                   mem_read64(cfg_bar_data,
                              NFD_CFG_BAR_ISL(2, cfg_msg.vnic),
                              sizeof cfg_bar_data);

#ifdef SVC_ME_MSIX_EN
                   mem_read64(rx_ring_vector_data,
                              (NFD_CFG_BAR_ISL(2, cfg_msg.vnic) + VNIC_CONFIG_RX_VECS),
                              sizeof rx_ring_vector_data);
#endif

                   local_csr_write(NFP_MECSR_MAILBOX_0, 0x2CF);
                   local_csr_write(NFP_MECSR_MAILBOX_1, cfg_msg.vnic);
                   local_csr_write(NFP_MECSR_MAILBOX_2, cfg_bar_data[0]);
                   local_csr_write(NFP_MECSR_MAILBOX_3, ncfg);

#ifdef SVC_ME_MSIX_EN
                   rx_queue_monitor_update_config(2, cfg_msg.vnic, cfg_bar_data, rx_ring_vector_data);
#endif
                   /* Complete the message */
                   cfg_msg.msg_valid = 0;
                   nfd_cfg_complete_cfg_msg(&cfg_msg,
                                            &NFD_CFG_SIG_NEXT_ME_2,
                                            NFD_CFG_NEXT_ME_2,
                                            NFD_CFG_RING_NUM(2, 3),
                                            NFD_CFG_RING_NUM(2, 2));
               }
               break;
#endif
            
#if NFD_USE_PCIE3            
           case 3:
               nfd_cfg_check_cfg_msg(&cfg_msg, &nfd_cfg_sig_svc_me3,
                                     NFD_CFG_RING_NUM(3, 2));

               if (cfg_msg.msg_valid) {
                   ncfg++;
                   mem_read64(cfg_bar_data,
                              NFD_CFG_BAR_ISL(3, cfg_msg.vnic),
                              sizeof cfg_bar_data);

#ifdef SVC_ME_MSIX_EN
                   mem_read64(rx_ring_vector_data,
                              (NFD_CFG_BAR_ISL(3, cfg_msg.vnic) + VNIC_CONFIG_RX_VECS),
                              sizeof rx_ring_vector_data);
#endif

                   local_csr_write(NFP_MECSR_MAILBOX_0, 0x3CF);
                   local_csr_write(NFP_MECSR_MAILBOX_1, cfg_msg.vnic);
                   local_csr_write(NFP_MECSR_MAILBOX_2, cfg_bar_data[0]);
                   local_csr_write(NFP_MECSR_MAILBOX_3, ncfg);

#ifdef SVC_ME_MSIX_EN
                   rx_queue_monitor_update_config(3, cfg_msg.vnic, cfg_bar_data, rx_ring_vector_data);
#endif
                   /* Complete the message */
                   cfg_msg.msg_valid = 0;
                   nfd_cfg_complete_cfg_msg(&cfg_msg,
                                            &NFD_CFG_SIG_NEXT_ME_3,
                                            NFD_CFG_NEXT_ME_3,
                                            NFD_CFG_RING_NUM(3, 3),
                                            NFD_CFG_RING_NUM(3, 2));
               }
               break;
#endif
            default:
               cfg_msg.msg_valid = 0;
               break;

            }

            i +=1 ;
            if (i == 4)
                i = 0;

        }

#ifdef SVC_ME_MSIX_EN

#if NFD_USE_PCIE0
        if (ctx() == 1) {
            rx_queue_monitor(0);
        }
#endif

#if NFD_USE_PCIE1
        if (ctx() == 2) {
            rx_queue_monitor(1);
        }
#endif

#if NFD_USE_PCIE2
        if (ctx() == 3) {
            rx_queue_monitor(2);
        }
#endif

#if NFD_USE_PCIE3
        if (ctx() == 4) {
            rx_queue_monitor(3);
        }
#endif

#endif

        ctx_swap();

    }
}

