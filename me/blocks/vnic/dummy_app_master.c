/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/dummy_app_master.c
 * @brief         Dummy code for ME serving as the NFD app master
 */

#include <assert.h>
#include <vnic/shared/nfcc_chipres.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_bulk.h>

#include <nfp6000/nfp_me.h>

#include <vnic/shared/nfd_cfg.h>

__visible SIGNAL nfd_cfg_sig_app_master1;

struct nfd_cfg_msg cfg_msg;

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
#define BUF_SZ  (2 * 1024)
#define BUF_NUM 1024
__xwrite unsigned int fake_bufs[8];
__export __imem __align2M char bufs_array[BUF_NUM * BUF_SZ];
__shared __gpr unsigned int buf_cnt = 0;

__xread unsigned int cfg_bar_data[6];

NFD_CFG_BASE_DECLARE(PCIE_ISL);


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

        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_app_master1, &cfg_msg);
    } else {
        unsigned int buf_base;
        /* unsigned int ctx_of; */
        unsigned int rnum;

        ctassert(SZ_2M >= (BUF_NUM * BUF_SZ));
        ctassert(__is_aligned(BUF_NUM, 8));

        buf_base = ((unsigned long long) bufs_array>>11) & 0xffffffff;

        rnum = _alloc_resource(BLM_NBI8_BLQ0_EMU_QID BLQ_EMU_RINGS global 1);

        while (ring_wait == 0) {
            ctx_swap();
        }

        while (buf_cnt < BUF_NUM) {
            int i = 0;
            for (; i < 8; i++) {
                fake_bufs[i] = buf_base + buf_cnt * (BUF_SZ >> 11);
                buf_cnt++;
            }
            mem_ring_put(rnum, fake_blm_addr, fake_bufs, sizeof fake_bufs);
        }
    }

    for (;;) {
        if (ctx() == 0) {
            if (!cfg_msg.msg_valid) {
                nfd_cfg_check_cfg_msg(&cfg_msg, &nfd_cfg_sig_app_master1,
                                      NFD_CFG_RING_NUM(PCIE_ISL, 2));

                if (cfg_msg.msg_valid) {
                    mem_read64(cfg_bar_data,
                               NFD_CFG_BASE(PCIE_ISL)[cfg_msg.vnic],
                               sizeof cfg_bar_data);
                }
            } else {
                __implicit_read(cfg_bar_data, 6);

                local_csr_write(NFP_MECSR_MAILBOX_0, cfg_msg.vnic);
                local_csr_write(NFP_MECSR_MAILBOX_1, cfg_bar_data[0]);

                cfg_msg.msg_valid = 0;
                nfd_cfg_app_complete_cfg_msg(&cfg_msg, NFD_CFG_BASE(PCIE_ISL));
            }

            ctx_swap();
        } else {

            ctx_swap();
        }
    }
}

