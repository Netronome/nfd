/*
 * Copyright (C) 2016-2017,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/shared/nfd_vf_cfg_iface.h
 * @brief         Interface to per VF configuration for *ndo_set/get functions
 */

#ifndef _BLOCKS__SHARED_NFD_VF_CFG_IFACE_UC_
#define _BLOCKS__SHARED_NFD_VF_CFG_IFACE_UC_

#include <stdmac.uc>

#include <nfd_user_cfg.h>

#include <vnic/shared/nfd_vf_cfg_iface.h>

/* Microcode declaration of per island symbol */
#macro nfd_vf_cfg_declare(isl)
    .alloc_mem _pf/**/isl/**/_net_vf_cfg/**/NFD_VF_CFG_ABI_VER \
         emem global (NFD_VF_CFG_MB_SZ + (NFD_MAX_VFS * NFD_VF_CFG_SZ)) \
         NFD_VF_CFG_SZ
#endm

#macro nfd_vf_cfg_declare_all()
    #ifdef NFD_PCIE0_EMEM
        nfd_vf_cfg_declare(0)
    #endif

    #ifdef NFD_PCIE1_EMEM
        nfd_vf_cfg_declare(1)
    #endif

    #ifdef NFD_PCIE2_EMEM
        nfd_vf_cfg_declare(2)
    #endif

    #ifdef NFD_PCIE3_EMEM
        nfd_vf_cfg_declare(3)
    #endif
#endm

/* Microcode access to per island start address */
#macro nfd_vf_cfg_base(addr_hi, addr_lo, isl)
#if (!is_ct_const(isl))
    #error "nfd_vf_cfg_base: isl must be compile time constant"
#endif

    immed40(addr_hi, addr_lo, _pf/**/isl/**/_net_vf_cfg/**/NFD_VF_CFG_ABI_VER)
#endm

/* Microcode access to per VF data offset within per island symbol */
#macro nfd_vf_cfg_offset(off, vf)
    alu[off, --, b, vf, <<(log2(NFD_VF_CFG_SZ))]
#if (NFD_VF_CFG_MB_SZ > 0)
    alu[off, off, +, NFD_VF_CFG_MB_SZ]
#endif
#endm

/* Microcode access to per VF data address */
#macro nfd_vf_cfg_addr(addr_hi, addr_lo, isl, vf)
.begin
    .reg offset

    nfd_vf_cfg_base(addr_hi, addr_lo, isl)
    nfd_vf_cfg_offset(offset, vf)
    alu[addr_lo, addr_lo, +, offset]
    alu[addr_hi, addr_hi, +carry, 0]
.end
#endm

#endif /* _BLOCKS__SHARED_NFD_VF_CFG_IFACE_UC_ */
