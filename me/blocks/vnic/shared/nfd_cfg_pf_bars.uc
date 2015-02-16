#ifndef _BLOCKS__SHARED_NFD_CFG_PF_BARS_UC_
#define _BLOCKS__SHARED_NFD_CFG_PF_BARS_UC_

#include "nfd_user_cfg.h"
#include <ns_vnic_ctrl.h>

#if NFD_MAX_PF_QUEUES != 0
#define NFD_MAX_PFS 1
#else
#define NFD_MAX_PFS 0
#endif

/* XXX Remove _pfX_net_bar2/4 */

/* XXX Use values exposed by nfd_internal.h */
#define NFD_OUT_Q_START                 128

#define_eval NFD_TOTAL_VNICS (NFD_MAX_VFS + NFD_MAX_PFS)
#define_eval NFD_CFG_BAR_SZ (NFD_TOTAL_VNICS * NS_VNIC_CFG_BAR_SZ)
#define_eval NFD_CFG_QC_SZ (NFD_MAX_PF_QUEUES * 2)
#define_eval NFD_CFG_BAR0_OFF (NFD_MAX_VFS * NS_VNIC_CFG_BAR_SZ)
#define_eval NFD_CFG_BAR2_OFF (NFD_MAX_VFS * NFD_MAX_VF_QUEUES * 2)
#define_eval NFD_CFG_BAR4_OFF (NFD_OUT_Q_START + \
                               NFD_MAX_VFS * NFD_MAX_VF_QUEUES * 2)

#macro nfd_define_pf_bars(_isl)
.alloc_mem nfd_cfg_base/**/_isl NFD_PCIE/**/_isl/**/_EMEM global \
    NFD_CFG_BAR_SZ 0x200000
.declare_resource nfd_cfg_base/**/_isl/**/_res global NFD_CFG_BAR_SZ \
    nfd_cfg_base/**/_isl
.alloc_resource _pf/**/_isl/**/_net_bar0 \
    nfd_cfg_base/**/_isl/**/_res+NFD_CFG_BAR0_OFF global \
    NS_VNIC_CFG_BAR_SZ

.declare_resource nfd_cfg_qc/**/_isl/**/_res global 0x100000
.alloc_resource _pf/**/_isl/**/_net_bar2 \
    nfd_cfg_qc/**/_isl/**/_res+NFD_CFG_BAR2_OFF global NFD_CFG_QC_SZ
.alloc_resource _pf/**/_isl/**/_net_bar4 \
    nfd_cfg_qc/**/_isl/**/_res+NFD_CFG_BAR4_OFF global NFD_CFG_QC_SZ
#endm


#ifdef NFD_PCIE0_EMEM
nfd_define_pf_bars(0)
#endif

#ifdef NFD_PCIE1_EMEM
nfd_define_pf_bars(1)
#endif

#ifdef NFD_PCIE2_EMEM
nfd_define_pf_bars(2)
#endif

#ifdef NFD_PCIE3_EMEM
nfd_define_pf_bars(3)
#endif

#undef NFD_TOTAL_VNICS
#undef NFD_CFG_BAR_SZ
#undef NFD_CFG_QC_SZ
#undef NFD_CFG_BAR0_OFF
#undef NFD_CFG_BAR2_OFF
#undef NFD_CFG_BAR4_OFF

#endif /* !_BLOCKS__SHARED_NFD_CFG_PF_BARS_UC_ */
