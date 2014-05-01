/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/dummy_app_master.c
 * @brief         Dummy code for ME serving as the NFD app master
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>

#include <vnic/shared/vnic_cfg.h>

__visible SIGNAL vnic_cfg_sig_app_master1;

struct vnic_cfg_msg cfg_msg;

int
main(void)
{

    if (ctx() == 0) {
        vnic_cfg_init_cfg_msg(&vnic_cfg_sig_app_master1, &cfg_msg);
    }

    for (;;) {
        if (ctx() == 0) {
            vnic_cfg_app_check_cfg_msg(&vnic_cfg_sig_app_master1,
                                       VNIC_CFG_RING_NUM(PCIE_ISL, 1),
                                       &VNIC_CFG_RING_ADDR(PCIE_ISL, 1));

            ctx_swap();
        } else {

            ctx_swap();
        }
    }
}

