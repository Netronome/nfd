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

// MSI/MSI-X
//#include <msix_gen.h>

__visible SIGNAL nfd_cfg_sig_app_master1;

struct nfd_cfg_msg cfg_msg;


/* Fake BLM */
/* Support 8 BLM rings (not all need to be used). */
/* All rings are on one EMEM unit */
#define BLM_RING_SZ     16384

emem1_queues_DECL;
ASM(.declare_resource BLQ_EMU_RINGS global 8 emem1_queues+4);
ASM(.alloc_mem fake_blm_mem emem1 global (8 * BLM_RING_SZ) (8 * BLM_RING_SZ));

#define DUMMY_BLM_INIT(_qid, _num, _base)                               \
    ASM(.alloc_resource _qid BLQ_EMU_RINGS+##_num global 1)             \
    ASM(.declare_resource _base##_res global BLM_RING_SZ fake_blm_mem)  \
    ASM(.alloc_resource _base _base##_res global BLM_RING_SZ BLM_RING_SZ) \
    ASM(.init_mu_ring _qid _base)

DUMMY_BLM_INIT(BLM_NBI8_BLQ0_EMU_QID, 0, BLM_NBI8_BLQ0_EMU_Q_BASE);
DUMMY_BLM_INIT(BLM_NBI8_BLQ1_EMU_QID, 1, BLM_NBI8_BLQ1_EMU_Q_BASE);
DUMMY_BLM_INIT(BLM_NBI8_BLQ2_EMU_QID, 2, BLM_NBI8_BLQ2_EMU_Q_BASE);
DUMMY_BLM_INIT(BLM_NBI8_BLQ3_EMU_QID, 3, BLM_NBI8_BLQ3_EMU_Q_BASE);
DUMMY_BLM_INIT(BLM_NBI9_BLQ0_EMU_QID, 4, BLM_NBI9_BLQ0_EMU_Q_BASE);
DUMMY_BLM_INIT(BLM_NBI9_BLQ1_EMU_QID, 5, BLM_NBI9_BLQ1_EMU_Q_BASE);
DUMMY_BLM_INIT(BLM_NBI9_BLQ2_EMU_QID, 6, BLM_NBI9_BLQ2_EMU_Q_BASE);
DUMMY_BLM_INIT(BLM_NBI9_BLQ3_EMU_QID, 7, BLM_NBI9_BLQ3_EMU_Q_BASE);


__gpr mem_ring_addr_t fake_blm_addr;

/* Fake buffers */
#define BUF_SZ  (10 * 1024)
#define BUF_NUM 1024
__xwrite unsigned int fake_bufs[8];
__export __emem __align16K char bufs_array[BUF_NUM * BUF_SZ];
__shared __gpr unsigned int buf_cnt = 0;

__xread unsigned int cfg_bar_data[6];

NFD_CFG_BASE_DECLARE(PCIE_ISL);

int
main(void)
{
    if (ctx() == 0) {
        nfd_cfg_init_cfg_msg(&nfd_cfg_sig_app_master1, &cfg_msg);

    } else {
        unsigned int buf_base;
        unsigned int rnum;

        ctassert(__is_aligned(BUF_NUM, 8));

        buf_base = ((unsigned long long) bufs_array>>11) & 0xffffffff;

        fake_blm_addr =
            (unsigned long long) (__LoadTimeConstant("__addr_emem1")) >> 8;
        rnum = _link_sym(BLM_NBI8_BLQ0_EMU_QID);

        while (buf_cnt < BUF_NUM) {
            int i = 0;
            for (; i < 8; i++) {
                fake_bufs[i] = buf_base + buf_cnt * (BUF_SZ >> 11);
                buf_cnt++;
            }
            mem_ring_put(rnum, fake_blm_addr, fake_bufs, sizeof fake_bufs);
        }
    }

    if (ctx() == 1) {
       // msix_gen_init();
    }

    for (;;) {
        if (ctx() == 0) {
            if (!cfg_msg.msg_valid) {
                nfd_cfg_master_chk_cfg_msg(&cfg_msg, &nfd_cfg_sig_app_master1,
                                           PCIE_ISL);

                if (cfg_msg.msg_valid) {
                    mem_read64(cfg_bar_data,
                               NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg.vnic),
                               sizeof cfg_bar_data);
                }
            } else {
                __xwrite unsigned int link_state;

                __implicit_read(cfg_bar_data, 6);

                local_csr_write(NFP_MECSR_MAILBOX_0, cfg_msg.vnic);
                local_csr_write(NFP_MECSR_MAILBOX_1, cfg_bar_data[0]);

                /* Set link state */
                if (!cfg_msg.error && (cfg_bar_data[NS_VNIC_CFG_CTRL] &
                                       NS_VNIC_CFG_CTRL_ENABLE)) {
                    link_state = NS_VNIC_CFG_STS_LINK;
                } else {
                    link_state = 0;
                }
                mem_write32(&link_state,
                            (NFD_CFG_BAR_ISL(PCIE_ISL, cfg_msg.vnic) +
                             NS_VNIC_CFG_STS),
                            sizeof link_state);

                /* Complete the message */
                cfg_msg.msg_valid = 0;
                nfd_cfg_app_complete_cfg_msg(&cfg_msg,
                                             NFD_CFG_BASE_LINK(PCIE_ISL));
            }

            ctx_swap();
        } else {
            if (ctx() == 1) {
                //msix_gen_loop();
                ctx_swap();
            } else {
                ctx_swap();
            }
        }
    }
}




