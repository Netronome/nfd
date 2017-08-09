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


#endif /* !_BLOCKS__SHARED_NFD_VF_CFG_IFACE_H_ */
