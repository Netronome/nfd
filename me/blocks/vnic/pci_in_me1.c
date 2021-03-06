/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file          blocks/vnic/pci_in_cfg.h
 * @brief         Code to deploy on PCIe ME3 in NFD with 2 MEs per direction
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>

#include <vnic/pci_in/issue_dma.c>
#include <vnic/pci_in/issue_dma_status.c>
#include <vnic/pci_in/precache_bufs.c>
#include <vnic/shared/nfd_cfg_internal.c>
#include <vnic/shared/nfd_rst_state.h>

/* Determine which configuration rings to use, and where to send the
 * next configuration message.  The choice depends on whether this is
 * PCI_IN_ISSUE_DMA_IDX 0 or 1, and whether NFD_IN_HAS_ISSUE0 and/or
 * NFD_IN_HAS_ISSUE1 are set.  To simplify the configuration choices,
 * we require that PCI_IN_ISSUE_DMA_IDX==0 is the first issue_dma ME
 * used, hence NFD_IN_HAS_ISSUE0 must be set. */

#if PCI_IN_ISSUE_DMA_IDX == 0

#define CFG_RING_IN     0

#ifdef NFD_IN_HAS_ISSUE1
/* Route the configuration message via issue1 */
#define CFG_RING_OUT    1

#else
/* Route the configuration message to cache_desc */
#define CFG_RING_OUT    2

#endif

#else   /* PCI_IN_ISSUE_DMA_IDX == 1 */
/* Route the configuration message to cache_desc */
#define CFG_RING_IN     1
#define CFG_RING_OUT    2
#endif

NFD_INIT_DONE_DECLARE;

SIGNAL cfg_msg_sig;
struct nfd_cfg_msg cfg_msg;
__xread struct nfd_cfg_msg cfg_msg_rd;


int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == NFD_IN_ISSUE_MANAGER) {
        nfd_cfg_check_pcie_clock(); /* Will halt ME on failure */

        nfd_cfg_init_cfg_msg(&cfg_msg, NFD_CFG_RING_NUM(PCIE_ISL, CFG_RING_IN),
                             &cfg_msg_rd, &cfg_msg_sig);

        precache_bufs_setup();

        distr_precache_bufs_setup_shared();

        issue_dma_status_setup();

        issue_dma_setup_shared();
#if PCI_IN_ISSUE_DMA_IDX == 0
        NFD_INIT_DONE_SET(PCIE_ISL, 2);     /* XXX Remove? */
#else
        NFD_INIT_DONE_SET(PCIE_ISL, 3);     /* XXX Remove? */
#endif
    } else {
        issue_dma_setup();
    }

    /* Kickoff ordering stages! */

    /*
     * Work loop
     */
    if (ctx() == NFD_IN_ISSUE_MANAGER) {
        __xwrite unsigned int distr_wr0, distr_wr1;
        SIGNAL distr_sig0, distr_sig1;
        SIGNAL_MASK distr_wait_msk = 0;

        /* CTX0 main loop */
        for (;;) {
            /* This loop may only swap once, at the end to ensure that
             * the service tasks are not starved out.  Actually processing
             * a ring configuration message is the one exception as they
             * should arrive infrequently. */

            /* Either check for a message, or perform one tick of processing
             * on the message each loop iteration */
            if (!cfg_msg.msg_valid) {
                /* XXX extract ring number once and save aside */
                nfd_cfg_check_cfg_msg(&cfg_msg,
                                      NFD_CFG_RING_NUM(PCIE_ISL, CFG_RING_IN),
                                      &cfg_msg_rd, &cfg_msg_sig);

                if (cfg_msg.msg_valid) {
                    if (cfg_msg.pci_reset) {
                        if (cfg_msg.vid == 0) {
                            /* Set reset state */
                            nfd_rst_state_set_rst(PCIE_ISL);
                            issue_dma_start_rst();
                        }
                    } else {
                        if (NFD_RST_STATE_TEST_RST(PCIE_ISL)) {
                            /* Clear reset state */
                            nfd_rst_state_set_up(PCIE_ISL);
                            issue_dma_end_rst();
                        }
                    }

                    nfd_cfg_parse_msg((void *) &cfg_msg, NFD_CFG_PCI_IN1);
                }
            } else {
                issue_dma_vnic_setup((void *) &cfg_msg);

                if (!cfg_msg.msg_valid) {
                    nfd_cfg_complete_cfg_msg(
                        &cfg_msg, NFD_CFG_RING_NUM(PCIE_ISL, CFG_RING_OUT));
                }
            }


            /* XXX Make this sig test fall through on no set */
            issue_dma_status();

            /* Service tasks
             * These must run through without swapping as they
             * will refill exactly the amount of resources as
             * are consumed for minimum sized packets each time
             * a set of full batches is processed. */
            issue_dma_gather_seq_recv(); /* sig test and copy */

            precache_bufs();

            distr_precache_bufs(&distr_wr0, &distr_wr1, &distr_wait_msk,
                                &distr_sig0, &distr_sig1);

            precache_bufs_compute_seq_safe();

            wait_sig_mask(distr_wait_msk);
            distr_wait_msk = 0;
            __implicit_read(&distr_sig0);
            __implicit_read(&distr_sig1);
        }
    } else {
        /* Worker main loop */
            issue_dma();
    }
}
