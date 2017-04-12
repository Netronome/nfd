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
 * @file          blocks/vnic/shared/nfd_ctrl.h
 * @brief         NFD control message definitions
 */
#ifndef _BLOCKS__VNIC_SHARED_NFD_CTRL_H_
#define _BLOCKS__VNIC_SHARED_NFD_CTRL_H_

#define NFD_MAX_CTRL_QUEUES 1

#define NFD_CFG_CTRL_CAP                                        \
    (NFP_NET_CFG_CTRL_ENABLE | NFP_NET_CFG_CTRL_PROMISC |       \
     NFP_NET_CFG_CTRL_MSIXAUTO | NFP_NET_CFG_CTRL_GATHER |      \
     NFP_NET_CFG_CTRL_IRQMOD)


#ifdef NFD_USE_CTRL
#define NFD_MAX_CTRL 1
#else
#define NFD_MAX_CTRL 0
#endif


#endif /* !_BLOCKS__VNIC_SHARED_NFD_CTRL_H_ */
