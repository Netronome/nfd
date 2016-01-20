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
 * @file          blocks/vnic/pci_out_sb_iface.uc
 * @brief         "stage batch" (SB) interface macros
 */

#ifndef __PCI_OUT_SB_IFACE_UC
#define __PCI_OUT_SB_IFACE_UC

#include "nfd_common.h"
#include "nfd_out.uc"   /* for NFD_OUT_MAX_QUEUES only */

#define NFD_OUT_SB_WQ_SIZE_LW  1024

/* Debug parameter */
#ifndef SB_USE_MU_WORK_QUEUES
#define SB_USE_MU_WORK_QUEUES 0
#endif

#define PCI_OUT_SB_WQ_CREDIT_SIG_NUM       13


#macro pci_out_sb_iface_declare()

    // Ticket release bitmaps
    .alloc_mem nfd_out_sb_release/**/PCIE_ISL ctm island \
        (NFD_OUT_MAX_QUEUES * 16) (NFD_OUT_MAX_QUEUES * 16)

    // WQ credits
    .alloc_mem nfd_out_sb_wq_credits/**/PCIE_ISL ctm island 4 4

    #if SB_USE_MU_WORK_QUEUES

        // MU work queues
        #define_eval __EMEM 'NFD_PCIE/**/PCIE_ISL/**/_EMEM'

        .alloc_resource nfd_out_sb_ring_num/**/PCIE_ISL __EMEM/**/_queues global 1 1
        .alloc_mem nfd_out_sb_ring_mem/**/PCIE_ISL __EMEM global \
            (NFD_OUT_SB_WQ_SIZE_LW * 4) (NFD_OUT_SB_WQ_SIZE_LW * 4)
        .init_mu_ring nfd_out_sb_ring_num/**/PCIE_ISL nfd_out_sb_ring_mem/**/PCIE_ISL

        #undef __EMEM

    #else /* SB_USE_MU_WORK_QUEUES */

        #ifndef NFD_OUT_SB_WQ_NUM
        #error "NFD_OUT_SB_WQ_NUM must be defined in stage batch and packet DMA MEs"
        #endif

        .alloc_mem nfd_out_sb_wq_mem/**/PCIE_ISL cls \
            island (NFD_OUT_SB_WQ_SIZE_LW * 4) (NFD_OUT_SB_WQ_SIZE_LW * 4)
        .alloc_resource nfd_out_sb_ring_num/**/PCIE_ISL \
            cls_rings+NFD_OUT_SB_WQ_NUM island 1 1
        .init_csr cls:Rings.RingBase/**/NFD_OUT_SB_WQ_NUM \
            (((nfd_out_sb_wq_mem/**/PCIE_ISL >> 7) << 0) | \
             ((log2(NFD_OUT_SB_WQ_SIZE_LW) - 5) << 16))
        .init_csr cls:Rings.RingPtrs/**/NFD_OUT_SB_WQ_NUM 0

    #endif /* SB_USE_MU_WORK_QUEUES */

    .reg g_sbpd_wq_hi
    .reg g_sbpd_wq_lo

#endm


#macro pci_out_sb_iface_init()
.begin

    #if SB_USE_MU_WORK_QUEUES

        move(g_sbpd_wq_hi, ((nfd_out_sb_ring_mem/**/PCIE_ISL >> 8) & 0xFF000000))
        move(g_sbpd_wq_lo, nfd_out_sb_ring_num/**/PCIE_ISL)

    #else /* SB_USE_MU_WORK_QUEUES */

        move(g_sbpd_wq_hi, 0)
        move(g_sbpd_wq_lo, ((nfd_out_sb_ring_num/**/PCIE_ISL) << 2))

    #endif /* SB_USE_MU_WORK_QUEUES */

.end
#endm



#macro pci_out_sb_add_work(in_xfer, in_sig)

    #if SB_USE_MU_WORK_QUEUES

        mem[qadd_work, in_xfer, g_sbpd_wq_hi, <<8, g_sbpd_wq_lo, SB_WQ_SIZE_LW],
            sig_done[in_sig]

    #else

        cls[ring_workq_add_work, in_xfer, g_sbpd_wq_hi, <<8, g_sbpd_wq_lo, SB_WQ_SIZE_LW],
            sig_done[in_sig]

    #endif

#endm


#macro pci_out_pd_request_work(in_xfer, in_sig)

    #if SB_USE_MU_WORK_QUEUES

        mem[qadd_thread, in_xfer, g_sbpd_wq_hi, <<8, g_sbpd_wq_lo, SB_WQ_SIZE_LW],
            sig_done[in_sig]

    #else

        cls[ring_workq_add_thread, in_xfer, g_sbpd_wq_hi, <<8, g_sbpd_wq_lo, SB_WQ_SIZE_LW],
            sig_done[in_sig]

    #endif

#endm



#endif /* __PCI_OUT_SB_IFACE_UC */
