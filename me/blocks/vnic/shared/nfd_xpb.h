/*
 * Copyright (C) 2018-2019,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/shared/nfd_xpb.h
 * @brief         An API to manage access to NFD configuration data
 */

#ifndef _BLOCKS__SHARED_NFD_XPB_H_
#define _BLOCKS__SHARED_NFD_XPB_H_

#if defined(__NFP_IS_6XXX)
    #include <vnic/shared/nfd_xpb_6000.h>
#elif defined(__NFP_IS_38XX)
    #include <vnic/shared/nfd_xpb_3800.h>
#else
    #error "Unsupported chip type"
#endif

#endif /* !_BLOCKS__SHARED_NFD_XPB_H_ */
