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
#define BUF_SZ  (10 * 1024)
#define BUF_NUM 128
__export __emem_n(1) __align(4096 * 4) unsigned int fake_blm_q[4096];
__shared __gpr int ring_wait = 0;
__xwrite unsigned int fake_bufs[8];
__shared __gpr mem_ring_addr_t fake_blm_addr;
__export __emem __align2M char bufs_array[BUF_NUM * BUF_SZ];



int
main(void)
{

    if (ctx() == 0) {
        fake_blm_addr = MEM_RING_CONFIGURE(fake_blm_q, TX_BLM_POOL); /* TEMP */

        ring_wait = 1;

        vnic_cfg_init_cfg_msg(&vnic_cfg_sig_app_master1, &cfg_msg);
    } else {
        unsigned int buf_base;
        unsigned int ctx_of;

        ctassert(SZ_2M > (BUF_NUM * BUF_SZ));

        buf_base = ((unsigned long long) bufs_array>>11) & 0xffffffff;
        ctx_of = ctx() * 8 * (BUF_SZ >> 11);

        /* TEMP */
        fake_bufs[0] = buf_base | ctx_of | 0 * (BUF_SZ >> 11);
        fake_bufs[1] = buf_base | ctx_of | 1 * (BUF_SZ >> 11);
        fake_bufs[2] = buf_base | ctx_of | 2 * (BUF_SZ >> 11);
        fake_bufs[3] = buf_base | ctx_of | 3 * (BUF_SZ >> 11);
        fake_bufs[4] = buf_base | ctx_of | 4 * (BUF_SZ >> 11);
        fake_bufs[5] = buf_base | ctx_of | 5 * (BUF_SZ >> 11);
        fake_bufs[6] = buf_base | ctx_of | 6 * (BUF_SZ >> 11);
        fake_bufs[7] = buf_base | ctx_of | 7 * (BUF_SZ >> 11);

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

