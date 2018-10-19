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
#ifndef _BLOCKS__SHARED_NFD_VF_CFG_IFACE_H_
#define _BLOCKS__SHARED_NFD_VF_CFG_IFACE_H_


#if defined(__NFP_LANG_MICROC) || defined (__NFP_LANG_ASM)
#include <nfd_user_cfg.h>
#include <assert.h>
#endif

/* ABI version can be overridden in nfd_user_cfg.h */
#ifndef NFD_VF_CFG_ABI_VER
#define NFD_VF_CFG_ABI_VER      1
#endif

#if (NFD_VF_CFG_ABI_VER == 1)
#include "nfd_vf_cfg_iface_abi1.h"
#elif (NFD_VF_CFG_ABI_VER == 2)
#include "nfd_vf_cfg_iface_abi2.h"
#endif

/* Helper defines for use with nfd_vf_cfg_base() */
#define NFD_VF_CFG_SEL_MB   1
#define NFD_VF_CFG_SEL_VF   0

/**
 * @param isl           PCIe island, in the range 0..3
 * @param vf            VF number
 * @param mb            Set true to reference mailbox
 *
 *
 * If "mb" is true, the VF number is ignored and address returned points
 * to the PCIe island VF config mailbox.  "mb" must be a compile time
 * constant.  "NFD_VF_CFG_SEL_MB" and "NFD_VF_CFG_SEL_VF" defines are
 * provided for use with this function.
 *
 * This function is not intended for use on the data plane as it is
 * expensive to extract the pointer.  Supplying an isl not in the
 * range 0..3 or for a PCIe island not in use with NFD is illegal.
 * It is caught with a halt().
 */
__intrinsic __emem char *nfd_vf_cfg_base(unsigned int isl, unsigned int vf,
                                         unsigned int mb) {
    __emem char *vf_cfg_base;

    ctassert(__is_ct_const(mb));
#if (NFD_VF_CFG_ABI_VER == 1)
    /* XXX abi 1 doesn't use a mailbox */
    ctassert(mb == 0);
#endif

    switch (isl) {
    case 0:
        #ifdef NFD_PCIE0_EMEM
            vf_cfg_base = NFD_VF_CFG_BASE_LINK(0);
        #else
            goto err;
        #endif
        break;
    case 1:
        #ifdef NFD_PCIE1_EMEM
            vf_cfg_base = NFD_VF_CFG_BASE_LINK(1);
        #else
            goto err;
        #endif
        break;
    case 2:
        #ifdef NFD_PCIE2_EMEM
            vf_cfg_base = NFD_VF_CFG_BASE_LINK(2);
        #else
            goto err;
        #endif
        break;
    case 3:
        #ifdef NFD_PCIE3_EMEM
            vf_cfg_base = NFD_VF_CFG_BASE_LINK(3);
        #else
            goto err;
        #endif
        break;
    default:
        goto err;
        break;
    };

    if (!mb) {
        vf_cfg_base += NFD_VF_CFG_OFF(vf);
    }

    return vf_cfg_base;

err:
    /* XXX we are halting on this error because it indicates a serious error
     * the return won't actually execute, and 0 is as good as any other
     * value.  0x809 == NFD_CFG_BAR_BASE_ISL_INVALID */
    local_csr_write(local_csr_mailbox_0, 0x809);
    local_csr_write(local_csr_mailbox_1, isl);
    halt();
    return 0;
};

#endif /* !_BLOCKS__SHARED_NFD_VF_CFG_IFACE_H_ */
