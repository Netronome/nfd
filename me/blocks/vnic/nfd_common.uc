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
 * @file          blocks/vnic/nfd_common.uc
 * @brief         Microcode access to NFD defines and macros
 */

#ifndef __NFD_COMMON_UC
#define __NFD_COMMON_UC

#include <stdmac.uc>
#include <nfd_user_cfg.h>

#include <vnic/nfd_common.h>
#include <vnic/shared/nfd_cfg.uc>

#define NFD_MAX_ISL             4

#define __NFD_EMU_BASE_ISL      24
#define __NFD_DIRECT_ACCESS     0x80

#ifndef NFD_PCIE_ISL_BASE
#define NFD_PCIE_ISL_BASE 4
#endif /* NFD_PCIE_ISL_BASE */


#define _NFD_TOTAL_VFQS (NFD_MAX_VFS * NFD_MAX_VF_QUEUES)


#macro nfd_q_extract(out_ispf, out_f, out_fq, in_q)
.begin

    #if (streq('out_ispf', '--') && streq('out_f', '--') && streq('out_fq', '--'))
        #error "At least one of out_ispf, out_f or out_fq must be specified"
    #endif

    #if (isnum(in_q))

        #if (in_q < _NFD_TOTAL_VFQS)

            #if (!streq('out_ispf', '--'))
                move(out_ispf, 0)
            #endif

            #if (!streq('out_f', '--'))
                move(out_f, (in_q / NFD_MAX_VF_QUEUES))
            #endif

            #if (!streq('out_fq', '--'))
                move(out_fq, (in_q % NFD_MAX_VF_QUEUES))
            #endif

        #else

            #if (!streq('out_ispf', '--'))
                move(out_ispf, 1)
            #endif

            #if (!streq('out_f', '--'))
                move(out_f, 0)
            #endif

            #if (!streq('out_fq', '--'))
                move(out_fq, ((in_q - _NFD_TOTAL_VFQS) % NFD_MAX_PF_QUEUES))
            #endif

        #endif

    #else

        .if (in_q < _NFD_TOTAL_VFQS)

            #if (!streq('out_ispf', '--'))
                move(out_ispf, 0)
            #endif

            #if (!streq('out_f', '--'))
                alu[out_f, --, B, in_q, >>(log2(NFD_MAX_VF_QUEUES))]
            #endif

            #if (!streq('out_fq', '--'))
                alu[out_fq, in_q, AND, (NFD_MAX_VF_QUEUES - 1)]
            #endif

        .else

            #if (!streq('out_ispf', '--'))
                move(out_ispf, 1)
            #endif

            #if (!streq('out_f', '--') || !streq('out_fq', '--'))

                #if (!streq('out_f', '--'))
                    move(out_f, 0)
                #endif

                #if (!streq('out_fq', '--'))
                    alu[out_fq, in_q, -, _NFD_TOTAL_VFQS]
                #endif

            #endif

        .endif

    #endif
.end
#endm


#macro nfd_build_vf_q(out_q, in_vf, in_vfq)
.begin
    #if (isnum(in_vf) && isnum(in_vfq))
        move(out_q, NFD_BUILD_QID(in_vf, in_vfq))
    #else
        alu[out_q, in_vfq, OR, in_vf, <<(log2(NFD_MAX_VF_QUEUES))]
    #endif
.end
#endm


#macro nfd_build_pf_q(out_q, in_pfq)
.begin
    #if (isnum(in_pf) && isnum(in_pfq))
        move(out_q, (_NFD_TOTAL_VFQS + in_pfq))
    #else
        alu[out_q, _NFD_TOTAL_VFQS, +, in_pfq]
    #endif
.end
#endm


#macro nfd_build_q(out_q, in_ispf, in_f, in_fq)
.begin
    #if (isnum(in_ispf))

        #if (in_ispf)
            nfd_build_pf_q(out_q, in_fq)
        #else
            nfd_build_vf_q(out_q, in_f, in_fq)
        #endif

    #else

        .if (BIT(in_ispf, 0) == 0)

            nfd_build_vf_q(out_q, in_f, in_fq)

        .else

            nfd_build_pf_q(out_q, in_fq)

        .endif

    #endif
.end
#endm


#endif /* __NFD_COMMON_UC */
