/*
 * Copyright (C) 2018,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/shared/nfd_tlv.uc
 * @brief         NFD macros for defining BAR TLVs
 */
#ifndef _BLOCKS__VNIC_SHARED_NFD_TLV_UC_
#define _BLOCKS__VNIC_SHARED_NFD_TLV_UC_

// inhibit pollution of codeless list file with illegal island scoped resources
#define __NFP_CHIPRES_H__

#include <nfd_common.h>
#include <nfd_cfg.uc>


/* TODO decide whether to test for NFD_USE_TLV in this file
 * Users don't have to include it if they don't want to use it?
 */


#ifndef NFD_CFG_TLV_BLOCK_SZ
    #error "NFD_CFG_TLV_BLOCK_SZ must be defined if TLVs are used"
#endif

#if ((NFD_CFG_TLV_BLOCK_SZ % 4) != 0)
#error "NFD_CFG_TLV_BLOCK_SZ must be a 4B multiple"
#endif

#ifndef NFD_CFG_TLV_BLOCK_OFF
    #error "NFD_CFG_TLV_BLOCK_OFF must be defined if TLVs are used"
#endif

#if ((NFD_CFG_TLV_BLOCK_OFF % 4) != 0)
#error "NFD_CFG_TLV_BLOCK_OFF must be a 4B multiple"
#endif

#ifdef _ISL
#warning "_ISL is being redefined"
#endif
#define _ISL 0
#while (_ISL < 4)
    #if (strstr(NFD_PCIE/**/_ISL/**/_EMEM, emem) == 1)
        .alloc_mem nfd_cfg_tlv_template/**/_ISL emem global \
             ((NFD_MAX_VFS + NFD_MAX_CTRL + NFD_MAX_PFS) * \
             NFD_CFG_TLV_BLOCK_SZ) 4
    #endif
    #define_eval _ISL (_ISL + 1)
#endloop
#undef _ISL

#define __nfd_tlv_offsets ""

#macro _nfd_tlv_extract_offset(key)
    #define_eval _NFD_TLV_OFFSET strstr('__nfd_tlv_offsets', '@/**/key')
    #if (_NFD_TLV_OFFSET <= 0)
        #define_eval _NFD_TLV_OFFSET 0
    #else
        #define_eval __nteo_left strleft('__nfd_tlv_offsets', _NFD_TLV_OFFSET - 1)
        #define_eval __nteo_right strright('__nfd_tlv_offsets', -_NFD_TLV_OFFSET)
        #define_eval _NFD_TLV_OFFSET \
            strright('__nteo_right', -strstr('__nteo_right', ':'))
        #if (strstr('__nteo_right', '@') <= 0)
            #define_eval __nfd_tlv_offsets '__nteo_left'
        #else
            #define_eval _NFD_TLV_OFFSET \
                strleft('_NFD_TLV_OFFSET', strstr('_NFD_TLV_OFFSET', '@') - 1)
            #define_eval __nteo_right \
                strright('__nteo_right', -strstr('__nteo_right', '@'))
            #define_eval __nfd_tlv_offsets '__nteo_left/**/@/**/__nteo_right'
        #endif
    #endif
#endm

#macro _nfd_tlv_store_offset(key, value)
    _nfd_tlv_extract_offset(key)
    #define_eval __nfd_tlv_offsets '__nfd_tlv_offsets/**/@/**/key/**/:/**/value'
#endm


#macro nfd_tlv_init(PCIE_ISL, vid, type, length, value)
    #define_eval __NFD_TLV_VID (vid)
    _nfd_tlv_extract_offset(p/**/PCIE_ISL/**/v/**/__NFD_TLV_VID)

    #define_eval __nfd_tlv_template_addr \
        ((vid * NFD_CFG_TLV_BLOCK_SZ) + _NFD_TLV_OFFSET)
    .init nfd_cfg_tlv_template/**/PCIE_ISL/**/+__nfd_tlv_template_addr \
        ((type << 16) | length)

    #define_eval __nfd_tlv_bar_addr \
        ((vid * NFP_NET_CFG_BAR_SZ) + NFD_CFG_TLV_BLOCK_OFF +  _NFD_TLV_OFFSET)
    .init nfd_cfg_base/**/PCIE_ISL/**/+__nfd_tlv_bar_addr \
        ((type << 16) | length)

    #if (length > 0)
        #define_eval __nfd_tlv_template_addr (__nfd_tlv_template_addr + 4)
        .init nfd_cfg_tlv_template/**/PCIE_ISL/**/+__nfd_tlv_template_addr value

        #define_eval __nfd_tlv_bar_addr (__nfd_tlv_bar_addr + 4)
        .init nfd_cfg_base/**/PCIE_ISL/**/+__nfd_tlv_bar_addr value
    #endif
    #define_eval _NFD_TLV_OFFSET (_NFD_TLV_OFFSET + 4 + length)
    #if (type == NFP_NET_CFG_TLV_TYPE_END)
        #if (length != 0)
            #error "NFP_NET_CFG_TLV_TYPE_END must have length = 0"
        #endif
    #endif
    _nfd_tlv_store_offset(p/**/PCIE_ISL/**/v/**/__NFD_TLV_VID, _NFD_TLV_OFFSET)
    #undef __NFD_TLV_VID
#endm

#endif /* _BLOCKS__VNIC_SHARED_NFD_TLV_UC_ */
