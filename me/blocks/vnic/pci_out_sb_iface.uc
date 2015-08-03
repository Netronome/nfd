#ifndef __PCI_OUT_SB_IFACE_UC
#define __PCI_OUT_SB_IFACE_UC

#include <nfd_common.h>
#include <nfd_out.uc>   /* for NFD_OUT_MAX_QUEUES only */

/* REMOVE ME */
#define SB_USE_MU_WORK_QUEUES 1


#define PCI_OUT_SB_WQ_CREDIT_SIG_NUM       13


#macro pci_out_sb_declare()

    // Ticket release bitmaps
    .alloc_mem nfd_out_sb_release/**/PCIE_ISL ctm island \
        (NFD_OUT_MAX_QUEUES * 16) (NFD_OUT_MAX_QUEUES * 16)

    // WQ credits
    .alloc_mem nfd_out_sb_wq_credits/**/PCIE_ISL ctm island 4 4

    #if SB_USE_MU_WORK_QUEUES

        // MU work queues
        #define NFD_OUT_SB_WQ_SIZE_LW  1024
        #define_eval __EMEM 'NFD_PCIE/**/PCIE_ISL/**/_EMEM'

        .alloc_resource nfd_out_sb_ring_num/**/PCIE_ISL __EMEM/**/_queues global 1 1
        .alloc_mem nfd_out_sb_ring_mem/**/PCIE_ISL __EMEM global \
            (NFD_OUT_SB_WQ_SIZE_LW * 4) (NFD_OUT_SB_WQ_SIZE_LW * 4)
        .init_mu_ring nfd_out_sb_ring_num/**/PCIE_ISL nfd_out_sb_ring_mem/**/PCIE_ISL

        #undef __EMEM

    #else /* SB_USE_MU_WORK_QUEUES */

        // CLS work queues
        #define NFD_OUT_SB_WQ_SIZE_LW  1024
        #define NFD_OUT_SB_WQ_OFF 0
        #define NFD_OUT_SB_WQ_NUM 15
        #define_eval __THIS_ISL (PCIE_ISL + NFD_PCIE_ISL_BASE) 

        .alloc_mem nfd_out_sb_wq_mem/**/PCIE_ISL cls+NFD_OUT_SB_WQ_OFF \
            island (NFD_OUT_SB_WQ_SIZE_LW * 4) (NFD_OUT_SB_WQ_SIZE_LW * 4)
        .alloc_resource nfd_out_sb_ring_num/**/PCIE_ISL \
            cls_rings+NFD_OUT_SB_WQ_NUM island 1 1
        .init_csr cls:i/**/__THIS_ISL/**/.Rings.RingBase/**/NFD_OUT_SB_WQ_NUM \
            (((NFD_OUT_SB_WQ_OFF >> 7) << 0) | \
             ((log2(NFD_OUT_SB_WQ_SIZE_LW) - 5) << 16))
        .init_csr cls:i/**/__THIS_ISL/**/.Rings.RingPtrs/**/NFD_OUT_SB_WQ_NUM 0

        #undef __THIS_ISL

    #endif /* SB_USE_MU_WORK_QUEUES */

#endm

#endif /* __PCI_OUT_SB_IFACE_UC */
