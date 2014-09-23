/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/dummy_app_master.c
 * @brief         Dummy code for ME serving as the NFD app master
 */

#include <assert.h>
#include <nfcc_chipres.h>
#include <nfp.h>

#include <nfp/me.h>

#include <vnic/shared/vnic_cfg.h>

__visible SIGNAL vnic_cfg_sig_app_master1;

struct vnic_cfg_msg cfg_msg;

/* Cheap ordering mechanism */
__shared __gpr int ring_wait = 0;


/* Fake BLM */
/* Support 8 BLM rings (not all need to be used). */
/* All rings are on one EMEM unit */
_declare_resource("BLQ_EMU_RINGS global 8 emem1_queues+4");
#define BLM_RING_SZ (4 * 4096)
__export __emem_n(1) __align(8 * BLM_RING_SZ)
    unsigned int fake_blm_mem[8 * BLM_RING_SZ / sizeof(unsigned int)];
__shared __gpr mem_ring_addr_t fake_blm_addr;

/* Fake buffers */
#define BUF_SZ  (10 * 1024)
#define BUF_NUM 128
__xwrite unsigned int fake_bufs[8];
__export __imem __align2M char bufs_array[BUF_NUM * BUF_SZ];


int
main(void)
{
    if (ctx() == 0) {
        unsigned int rnum;

        rnum = _alloc_resource(BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS global 1);
        mem_ring_setup(rnum, &fake_blm_mem[0], BLM_RING_SZ);
        rnum = _alloc_resource(BLM_NBI8_BLQ1_EMU_QID BLQ_EMU_RINGS global 1);
        mem_ring_setup(rnum, &fake_blm_mem[BLM_RING_SZ / 4], BLM_RING_SZ);
        rnum = _alloc_resource(BLM_NBI8_BLQ2_EMU_QID BLQ_EMU_RINGS global 1);
        mem_ring_setup(rnum, &fake_blm_mem[2 * BLM_RING_SZ / 4], BLM_RING_SZ);
        rnum = _alloc_resource(BLM_NBI8_BLQ3_EMU_QID BLQ_EMU_RINGS global 1);
        mem_ring_setup(rnum, &fake_blm_mem[3 * BLM_RING_SZ / 4], BLM_RING_SZ);
        rnum = _alloc_resource(BLM_NBI9_BLQ0_EMU_QID BLQ_EMU_RINGS global 1);
        mem_ring_setup(rnum, &fake_blm_mem[4 * BLM_RING_SZ / 4], BLM_RING_SZ);
        rnum = _alloc_resource(BLM_NBI9_BLQ1_EMU_QID BLQ_EMU_RINGS global 1);
        mem_ring_setup(rnum, &fake_blm_mem[5 * BLM_RING_SZ / 4], BLM_RING_SZ);
        rnum = _alloc_resource(BLM_NBI9_BLQ2_EMU_QID BLQ_EMU_RINGS global 1);
        mem_ring_setup(rnum, &fake_blm_mem[6 * BLM_RING_SZ / 4], BLM_RING_SZ);
        rnum = _alloc_resource(BLM_NBI9_BLQ3_EMU_QID BLQ_EMU_RINGS global 1);
        fake_blm_addr = mem_ring_setup(rnum, &fake_blm_mem[7 * BLM_RING_SZ / 4],
                                       BLM_RING_SZ);

        ring_wait = 1;

        vnic_cfg_init_cfg_msg(&vnic_cfg_sig_app_master1, &cfg_msg);
    } else {
        unsigned int buf_base;
        unsigned int ctx_of;
        unsigned int rnum;

        ctassert(SZ_2M > (BUF_NUM * BUF_SZ));

        buf_base = ((unsigned long long) bufs_array>>11) & 0xffffffff;
        ctx_of = ctx() * 8 * (BUF_SZ >> 11);

        /* TEMP */
        fake_bufs[0] = buf_base + ctx_of + 0 * (BUF_SZ >> 11);
        fake_bufs[1] = buf_base + ctx_of + 1 * (BUF_SZ >> 11);
        fake_bufs[2] = buf_base + ctx_of + 2 * (BUF_SZ >> 11);
        fake_bufs[3] = buf_base + ctx_of + 3 * (BUF_SZ >> 11);
        fake_bufs[4] = buf_base + ctx_of + 4 * (BUF_SZ >> 11);
        fake_bufs[5] = buf_base + ctx_of + 5 * (BUF_SZ >> 11);
        fake_bufs[6] = buf_base + ctx_of + 6 * (BUF_SZ >> 11);
        fake_bufs[7] = buf_base + ctx_of + 7 * (BUF_SZ >> 11);

        rnum = _alloc_resource(BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS global 1);

        while (ring_wait == 0) {
            ctx_swap();
        }

        /* TEMP restrict buffers further by reducing ctx putting to the ring */
        if (ctx() < 8) {
            mem_ring_put(rnum, fake_blm_addr, fake_bufs, sizeof fake_bufs);
        }
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

