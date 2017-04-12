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
 * @file          blocks/vnic/shared/nfd_cfg.uc
 * @brief         Microcode access to CFG BAR
 */

#ifndef _BLOCKS__SHARED_NFD_CFG_UC_
#define _BLOCKS__SHARED_NFD_CFG_UC_

#include "nfd_user_cfg.h"
#include <nfp_net_ctrl.h>
#include <vnic/shared/nfd_cfg.h>

#ifndef NFD_MAX_PFS
#error "NFD_MAX_PFS is not defined but is required"
#endif

#define_eval NFD_TOTAL_VNICS (NFD_MAX_VFS + NFD_MAX_PFS + NFD_MAX_CTRL)
#define_eval NFD_CFG_BAR_SZ (NFD_TOTAL_VNICS * NFP_NET_CFG_BAR_SZ)
#define_eval NFD_CFG_BAR0_OFF (NFD_MAX_VFS * NFP_NET_CFG_BAR_SZ)


#macro nfd_cfg_define_bars(_isl)
.alloc_mem nfd_cfg_base/**/_isl NFD_PCIE/**/_isl/**/_EMEM global \
    NFD_CFG_BAR_SZ 0x200000
#endm


#ifdef NFD_PCIE0_EMEM
nfd_cfg_define_bars(0)
#endif

#ifdef NFD_PCIE1_EMEM
nfd_cfg_define_bars(1)
#endif

#ifdef NFD_PCIE2_EMEM
nfd_cfg_define_bars(2)
#endif

#ifdef NFD_PCIE3_EMEM
nfd_cfg_define_bars(3)
#endif

#undef NFD_TOTAL_VNICS
#undef NFD_CFG_BAR_SZ
#undef NFD_CFG_BAR0_OFF


#macro nfd_cfg_get_bar_addr(out_hi, out_lo, in_vid, ISL)
.begin
    .reg tmp_lo
    .reg off

    #if (!is_ct_const(ISL))
        #error "nfd_cfg_get_bar_addr: ISL must be CT const"
    #endif

    move(out_hi, ((nfd_cfg_base/**/ISL >> 8) & 0xFF000000))
    move(tmp_lo, (nfd_cfg_base/**/ISL & 0xFFFFFFFF))
    alu[off, --, B, in_vid, <<(log2(NFP_NET_CFG_BAR_SZ))]
    alu[out_lo, tmp_lo, +, off]

.end
#endm


#endif /* !_BLOCKS__SHARED_NFD_CFG_UC_ */
