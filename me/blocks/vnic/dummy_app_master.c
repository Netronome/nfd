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
#include <vnic/pci_in_cfg.h>        /* Fake BLM */

__visible SIGNAL vnic_cfg_sig_app_master1;

struct vnic_cfg_msg cfg_msg;

/* Fake BLM */
__export __emem_n(0) __align(4096 * 4) unsigned int fake_blm_q[4096];
__shared __gpr int ring_wait = 0;
__xwrite unsigned int fake_bufs[8];
__shared __gpr mem_ring_addr_t fake_blm_addr;


int
main(void)
{

    if (ctx() == 0) {
        fake_blm_addr = MEM_RING_CONFIGURE(fake_blm_q, TX_BLM_POOL); /* TEMP */

        ring_wait = 1;

        vnic_cfg_init_cfg_msg(&vnic_cfg_sig_app_master1, &cfg_msg);
    } else {
        /* TEMP */
        fake_bufs[0] = 0x195fc000 | (ctx() << 1);
        fake_bufs[1] = 0x195fc200 | (ctx() << 1);
        fake_bufs[2] = 0x195fc400 | (ctx() << 1);
        fake_bufs[3] = 0x195fc600 | (ctx() << 1);
        fake_bufs[4] = 0x195fc800 | (ctx() << 1);
        fake_bufs[5] = 0x195fca00 | (ctx() << 1);
        fake_bufs[6] = 0x195fcc00 | (ctx() << 1);
        fake_bufs[7] = 0x195fce00 | (ctx() << 1);

        while (ring_wait == 0) {
            ctx_swap();
        }

        mem_ring_put(TX_BLM_POOL, fake_blm_addr, fake_bufs, sizeof fake_bufs);
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

