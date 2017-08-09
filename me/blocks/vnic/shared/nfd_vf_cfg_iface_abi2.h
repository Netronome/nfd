/*
 * Copyright (C) 2017,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/shared/nfd_vf_cfg_iface_abi2.h
 * @brief         ABI v2 for per VF configuration for *ndo_set/get functions
 */
#ifndef _BLOCKS__SHARED_NFD_VF_CFG_IFACE_ABI2_H_
#define _BLOCKS__SHARED_NFD_VF_CFG_IFACE_ABI2_H_

/* VF config mailbox */
#define NFD_VF_CFG_MB_SZ                16
#define NFD_VF_CFG_MB_ofs               0x0
#define NFD_VF_CFG_MB_RET_ofs           0x2    /* rd/wr8_le access */
#define NFD_VF_CFG_MB_RET_fld           0, 31, 16
#define NFD_VF_CFG_MB_RET_msk           0xffff
#define NFD_VF_CFG_MB_RET_shf           16
#define NFD_VF_CFG_MB_RET_wrd           0
#define NFD_VF_CFG_MB_CAP_fld           0, 15, 0
#define NFD_VF_CFG_MB_CAP_msk           0xffff
#define NFD_VF_CFG_MB_CAP_shf           0
#define NFD_VF_CFG_MB_CAP_wrd           0
/* capabilities/update bits */
#define   NFD_VF_CFG_MB_CAP_MAC         (1 << 0)
#define   NFD_VF_CFG_MB_CAP_VLAN        (1 << 1)
#define   NFD_VF_CFG_MB_CAP_SPOOF       (1 << 2)
#define   NFD_VF_CFG_MB_CAP_LINK_STATE  (1 << 3)
#define NFD_VF_CFG_MB_VF_NUM_fld        1, 31, 24
#define NFD_VF_CFG_MB_VF_NUM_msk        0xff
#define NFD_VF_CFG_MB_VF_NUM_shf        24
#define NFD_VF_CFG_MB_VF_NUM_wrd        1
#define NFD_VF_CFG_MB_UPD_ofs           0x4
#define NFD_VF_CFG_MB_UPD_fld           1, 15, 0
#define NFD_VF_CFG_MB_UPD_msk           0xffff
#define NFD_VF_CFG_MB_UPD_shf           0
#define NFD_VF_CFG_MB_UPD_wrd           1

/* VF config entry */
#define NFD_VF_CFG_SZ                       16
#define NFD_VF_CFG_MAC_SZ                   6
#define NFD_VF_CFG_MAC_ofs                  0x0
#define   NFD_VF_CFG_MAC_HI_fld             0, 31, 0
#define   NFD_VF_CFG_MAC_HI_msk             0xffffffff
#define   NFD_VF_CFG_MAC_HI_shf             0
#define   NFD_VF_CFG_MAC_HI_wrd             0
#define   NFD_VF_CFG_MAC_LO_fld             1, 31, 16
#define   NFD_VF_CFG_MAC_LO_msk             0xffff
#define   NFD_VF_CFG_MAC_LO_shf             16
#define   NFD_VF_CFG_MAC_LO_wrd             1
#define NFD_VF_CFG_CTRL_ofs                 0x4
#define NFD_VF_CFG_CTRL_fld                 1, 7, 0
#define NFD_VF_CFG_CTRL_msk                 0xff
#define NFD_VF_CFG_CTRL_shf                 0
#define NFD_VF_CFG_CTRL_wrd                 1
#define   NFD_VF_CFG_CTRL_TRUSTED_fld       1, 4, 4
#define   NFD_VF_CFG_CTRL_TRUSTED_msk       0x1
#define   NFD_VF_CFG_CTRL_TRUSTED_shf       4
#define   NFD_VF_CFG_CTRL_TRUSTED_wrd       1
#define   NFD_VF_CFG_CTRL_RSS_fld           1, 3, 3
#define   NFD_VF_CFG_CTRL_RSS_msk           0x1
#define   NFD_VF_CFG_CTRL_RSS_shf           3
#define   NFD_VF_CFG_CTRL_RSS_wrd           1
#define   NFD_VF_CFG_CTRL_SPOOF_fld         1, 2, 2
#define   NFD_VF_CFG_CTRL_SPOOF_msk         0x1
#define   NFD_VF_CFG_CTRL_SPOOF_shf         2
#define   NFD_VF_CFG_CTRL_SPOOF_wrd         1
#define   NFD_VF_CFG_CTRL_LINK_STATE_fld    1, 1, 0
#define   NFD_VF_CFG_CTRL_LINK_STATE_msk    0x3
#define   NFD_VF_CFG_CTRL_LINK_STATE_shf    0
#define   NFD_VF_CFG_CTRL_LINK_STATE_wrd    1
#define     NFD_VF_CFG_CTRL_LINK_STATE_AUTO     0
#define     NFD_VF_CFG_CTRL_LINK_STATE_ENABLE   1
#define     NFD_VF_CFG_CTRL_LINK_STATE_DISABLE  2
#define NFD_VF_CFG_VLAN_ofs                 0x8
#define NFD_VF_CFG_VLAN_PROTO_fld           2, 31, 16
#define NFD_VF_CFG_VLAN_PROTO_msk           0xffff
#define NFD_VF_CFG_VLAN_PROTO_shf           16
#define NFD_VF_CFG_VLAN_PROTO_wrd           2
#define NFD_VF_CFG_VLAN_fld                 2, 15, 0
#define NFD_VF_CFG_VLAN_msk                 0xffff
#define NFD_VF_CFG_VLAN_shf                 0
#define NFD_VF_CFG_VLAN_wrd                 2
#define  NFD_VF_CFG_VLAN_QOS_fld            2, 15, 13
#define  NFD_VF_CFG_VLAN_QOS_msk            0x7
#define  NFD_VF_CFG_VLAN_QOS_shf            13
#define  NFD_VF_CFG_VLAN_QOS_wrd            2
#define  NFD_VF_CFG_VLAN_VID_fld            2, 11, 0
#define  NFD_VF_CFG_VLAN_VID_msk            0xfff
#define  NFD_VF_CFG_VLAN_VID_shf            0
#define  NFD_VF_CFG_VLAN_VID_wrd            2


#define NFD_VF_CFG_MAX_VFS_SZ   4

#define NFD_VF_CFG_OFF(_vf) (NFD_VF_CFG_MB_SZ + (NFD_VF_CFG_SZ * (_vf)))


#if defined(__NFP_LANG_MICROC)
/* Expose NFD_MAX_VFS to the host */
#define NFD_VF_CFG_MAX_VFS                                    \
    __asm { .alloc_mem nfd_vf_cfg_max_vfs emem global 8 8 }   \
    __asm { .init nfd_vf_cfg_max_vfs NFD_MAX_VFS }

#define NFD_VF_CFG_BASE_IND2(_isl, _abi) _pf##_isl##_net_vf_cfg##_abi
#define NFD_VF_CFG_BASE_IND1(_isl, _abi) NFD_VF_CFG_BASE_IND2(_isl, _abi)
#define NFD_VF_CFG_BASE(_isl) NFD_VF_CFG_BASE_IND1(_isl, NFD_VF_CFG_ABI_VER)

/* MicroC declaration of per island symbol */
#if (NFD_VF_CFG_MB_SZ != NFD_VF_CFG_SZ)
#error "NFD_VF_CFG_MB_SZ must match NFD_VF_CFG_SZ"
#endif
#define NFD_VF_CFG_DECLARE(_isl)                                    \
    __asm{ .alloc_mem NFD_VF_CFG_BASE(_isl) emem global             \
            (NFD_VF_CFG_MB_SZ + (NFD_MAX_VFS * NFD_VF_CFG_SZ))      \
            NFD_VF_CFG_SZ }

/* MicroC initialisation of mailbox capability field */
#ifndef NFD_VF_CFG_CAP
#error "NFD_VF_CFG_CAP must be specified for NFD_VF_CFG_ABI_VER 2"
#endif
#if (NFD_VF_CFG_CAP > NFD_VF_CFG_MB_CAP_msk)
#error "NFD_VF_CFG_CAP seems invalid"
#endif
#define NFD_VF_CFG_INIT(_isl)                            \
    __asm{ .init NFD_VF_CFG_BASE(_isl)+NFD_VF_CFG_MB_ofs \
            ((NFD_VF_CFG_MB_CAP_msk & (NFD_VF_CFG_CAP))  \
            << NFD_VF_CFG_MB_CAP_shf)}

/* MicroC access to per island symbol */
#define NFD_VF_CFG_BASE_LINK_IND1(_sym)         \
    ((__emem char *) _link_sym(_sym))
#define NFD_VF_CFG_BASE_LINK_IND0(_isl)                 \
    NFD_VF_CFG_BASE_LINK_IND1(NFD_VF_CFG_BASE(_isl))
#define NFD_VF_CFG_BASE_LINK(_isl) NFD_VF_CFG_BASE_LINK_IND0(_isl)

/* MicroC access to per VF data address */
#define NFD_VF_CFG_ADDR(_base, _vf)           \
    ((_base) + NFD_VF_CFG_OFF(_vf))

/* MicroC VF config mailbox and entry structs */
struct sriov_mb {
    union {
        struct {
            unsigned int resp:16;
            unsigned int cap_flags:16;
            unsigned int vf:8;
            unsigned int resvd1:8;
            unsigned int update_flags:16;
            unsigned int spare0;
            unsigned int spare1;
        };
        unsigned int __raw[4];
    };
};

struct sriov_cfg {
    union {
        struct {
            unsigned int mac_hi;
            union {
                struct {
                    unsigned short mac_lo;
                    unsigned char resvd0;
                    /* ctrl_flags */
                    unsigned char ctrl_resvd0:3;
                    unsigned char ctrl_trusted:1;
                    unsigned char ctrl_rss:1;
                    unsigned char ctrl_spoof:1;
                    unsigned char ctrl_link_state:2;
                };
                struct {
                    unsigned int :24;
                    unsigned int ctrl_flags:8;
                };
            };
            union {
                struct {
                    unsigned short vlan_proto;
                    /* vlan_tag */
                    unsigned short vlan_qos:3;
                    unsigned short vlan_resvd:1;
                    unsigned short vlan_id:12;
                };
                struct {
                    unsigned int :16;
                    unsigned int vlan_tag:16;
                };
            };
            unsigned int spare0;
        };
        unsigned int __raw[4];
    };
};

#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__SHARED_NFD_VF_CFG_IFACE_ABI2_H_ */

