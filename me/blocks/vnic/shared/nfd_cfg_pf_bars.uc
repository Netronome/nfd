/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_cfg_pf_bars.uc
 * @brief         Microcode access to CFG BAR defines
 */

#ifndef _BLOCKS__SHARED_NFD_CFG_PF_BARS_UC_
#define _BLOCKS__SHARED_NFD_CFG_PF_BARS_UC_

#include "nfd_user_cfg.h"
#include <nfp_net_ctrl.h>

#if NFD_MAX_PF_QUEUES != 0
#define NFD_MAX_PFS 1
#else
#define NFD_MAX_PFS 0
#endif

#define_eval NFD_TOTAL_VNICS (NFD_MAX_VFS + NFD_MAX_PFS)
#define_eval NFD_CFG_BAR_SZ (NFD_TOTAL_VNICS * NFP_NET_CFG_BAR_SZ)
#define_eval NFD_CFG_BAR0_OFF (NFD_MAX_VFS * NFP_NET_CFG_BAR_SZ)


#macro nfd_define_pf_bars(_isl)
.alloc_mem nfd_cfg_base/**/_isl NFD_PCIE/**/_isl/**/_EMEM global \
    NFD_CFG_BAR_SZ 0x200000
#if NFD_MAX_PF_QUEUES != 0
.declare_resource nfd_cfg_base/**/_isl/**/_res global NFD_CFG_BAR_SZ \
    nfd_cfg_base/**/_isl
.alloc_resource _pf/**/_isl/**/_net_bar0 \
    nfd_cfg_base/**/_isl/**/_res+NFD_CFG_BAR0_OFF global \
    NFP_NET_CFG_BAR_SZ
#endif
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
#undef NFD_CFG_BAR0_OFF

#endif /* !_BLOCKS__SHARED_NFD_CFG_PF_BARS_UC_ */
