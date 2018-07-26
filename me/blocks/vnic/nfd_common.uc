/*
 * Copyright (C) 2014-2018,  Netronome Systems, Inc.  All rights reserved.
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


#macro nfd_extract_vf_q(out_t, out_f, out_fq, in_q, VF_TGT)
.begin
    #if (NFD_MAX_VF_QUEUES != 0)
        #if (!streq('out_t', '--'))
            move(out_t, NFD_VNIC_TYPE_VF)
        #endif

        #if (!streq('out_f', '--'))
            alu[out_f, --, B, in_q, >>(log2(NFD_MAX_VF_QUEUES))]
        #endif

        #if (!streq('out_fq', '--'))
            alu[out_fq, in_q, AND, (NFD_MAX_VF_QUEUES - 1)]
        #endif

        #if (!streq('VF_TGT', '--'))
            br[VF_TGT]
        #endif
    #else
        #error "Compile options don't support VF queues"
    #endif
.end
#endm


#macro nfd_extract_ctrl_q(out_t, out_f, out_fq, in_q, CTRL_TGT)
.begin
    #ifdef NFD_USE_CTRL
        #if (NFD_MAX_CTRL_QUEUES != 1 || NFD_MAX_CTRL != 1)
            #error "Unsupported CTRL vNIC values"
        #endif

        #if (!streq('out_t', '--'))
            move(out_t, NFD_VNIC_TYPE_CTRL)
        #endif

        #if (!streq('out_f', '--'))
            move(out_f, 0)
        #endif

        #if (!streq('out_fq', '--'))
            move(out_fq, 0)
        #endif

        #if (!streq('CTRL_TGT', '--'))
            br[CTRL_TGT]
        #endif
    #else
        #error "Compile options don't support CTRL queues"
    #endif
.end
#endm


#macro nfd_extract_pf_q(out_t, out_f, out_fq, in_q, PF_TGT)
.begin
    #if (NFD_MAX_PF_QUEUES != 0)
        #if (!streq('out_t', '--'))
            move(out_t, NFD_VNIC_TYPE_PF)
        #endif


        #if (isnum(in_q))
            #if (!streq('out_f', '--'))
                move(out_f, NFD_NATQ2PF(in_q))
            #endif

            #if (!streq('out_fq', '--'))
                move(out_fq, NFD_NATQ2PFQ(in_q))
            #endif
        #else
            #if (!streq('out_f', '--') || !streq('out_fq', '--'))
                .reg pf_q

                alu[pf_q, in_q, -, NFD_FIRST_PF_QUEUE]

                #if (!streq('out_f', '--'))
                    alu[out_f, --, b, pf_q, >>(log2(NFD_MAX_PF_QUEUES))]
                #endif

                #if (!streq('out_fq', '--'))
                    alu[out_fq, pf_q, and, (NFD_MAX_PF_QUEUES - 1)]
                #endif
            #endif
        #endif

        #if (!streq('PF_TGT', '--'))
            br[PF_TGT]
        #endif
    #else
        #error "Compile options don't support PF queues"
    #endif
.end
#endm


#macro nfd_q_extract(out_t, out_f, out_fq, in_q, VF_TGT, CTRL_TGT, PF_TGT)
.begin

    #if (streq('out_t', '--') && streq('out_f', '--') && streq('out_fq', '--'))
        #error "At least one of out_type, out_f or out_fq must be specified"
    #endif

    #if (isnum(in_q))

        #if (NFD_Q_IS_PF(in_q))
            nfd_extract_pf_q(out_t, out_f, out_fq, in_q, PF_TGT)
        #elif (NFD_Q_IS_VF(in_q))
            nfd_extract_vf_q(out_t, out_f, out_fq, in_q, VF_TGT)
        #elif (NFD_Q_IS_CTRL(in_q))
            nfd_extract_ctrl_q(out_t, out_f, out_fq, in_q, CTRL_TGT)
        #else
            #error "Invalid queue number"
        #endif

    #else

        #if ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (NFD_Q_IS_PF(in_q))
                nfd_extract_pf_q(out_t, out_f, out_fq, in_q, PF_TGT)
            .elif (NFD_Q_IS_VF(in_q))
                nfd_extract_vf_q(out_t, out_f, out_fq, in_q, VF_TGT)
            .else
                nfd_extract_ctrl_q(out_t, out_f, out_fq, in_q, CTRL_TGT)
            .endif
        #elif ((NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (NFD_Q_IS_PF(in_q))
                nfd_extract_pf_q(out_t, out_f, out_fq, in_q, PF_TGT)
            .else
                nfd_extract_ctrl_q(out_t, out_f, out_fq, in_q, CTRL_TGT)
            .endif
        #elif ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0))
            .if (NFD_Q_IS_PF(in_q))
                nfd_extract_pf_q(out_t, out_f, out_fq, in_q, PF_TGT)
            .else
                nfd_extract_vf_q(out_t, out_f, out_fq, in_q, VF_TGT)
            .endif
        #elif (NFD_MAX_PFS != 0)
            nfd_extract_pf_q(out_t, out_f, out_fq, in_q, PF_TGT)
        #elif (NFD_MAX_VFS != 0)
            nfd_extract_vf_q(out_t, out_f, out_fq, in_q, VF_TGT)
        #else
            #error "Unsupported vNIC parameters"
        #endif

    #endif
.end
#endm


#macro nfd_q_extract(out_t, out_f, out_fq, in_q)
    nfd_q_extract(out_t, out_f, out_fq, in_q, --, --, --)
#endm


#macro nfd_build_vf_q(out_q, in_vf, in_vfq)
.begin
    #if (NFD_MAX_VF_QUEUES != 0)
        #if (isnum(in_vf) && isnum(in_vfq))
            move(out_q, NFD_BUILD_QID(NFD_VNIC_TYPE_VF, in_vf, in_vfq))
        #else
            alu[out_q, in_vfq, OR, in_vf, <<(log2(NFD_MAX_VF_QUEUES))]
        #endif
    #else
        #error "Compile options don't support VF queues"
    #endif
.end
#endm


#macro nfd_build_ctrl_q(out_q, in_ctrlf, in_ctrlq)
.begin
    #ifdef NFD_USE_CTRL
        #if (isnum(in_ctrlf) && isnum(in_ctrlq))
            move(out_q, NFD_BUILD_QID(NFD_VNIC_TYPE_CTRL, in_ctrlf, in_ctrlq))
        #else
            #if (NFD_MAX_CTRL_QUEUES != 1 || NFD_MAX_CTRL != 1)
                #error "Unsupported CTRL vNIC values"
            #endif
            move(out_q, NFD_CTRL_QUEUE)
        #endif
    #else
        #error "Compile options don't support CTRL queues"
    #endif
.end
#endm


#macro nfd_build_pf_q(out_q, in_pf, in_pfq)
.begin
    #if (NFD_MAX_PF_QUEUES != 0)
        #if (isnum(in_pf) && isnum(in_pfq))
            move(out_q, NFD_BUILD_QID(NFD_VNIC_TYPE_PF, in_pf, in_pfq))
        #else
            alu[out_q, in_pfq, or, in_pf, <<(log2(NFD_MAX_PF_QUEUES))]
            alu[out_q, NFD_FIRST_PF_QUEUE, +, out_q]
        #endif
    #else
        #error "Compile options don't support PF queues"
    #endif
.end
#endm


#macro nfd_build_q(out_q, in_t, in_f, in_fq)
.begin
    #if (isnum(in_t))
        #if (in_t == NFD_VNIC_TYPE_PF)
            nfd_build_pf_q(out_q, in_f, in_fq)
        #elif (in_t == NFD_VNIC_TYPE_VF)
            nfd_build_vf_q(out_q, in_f, in_fq)
        #elif (in_t == NFD_VNIC_TYPE_CTRL)
            nfd_build_ctrl_q(out_q, in_f, in_fq)
        #else
            #error "Illegal vNIC type"
        #endif
    #else
        #if ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (in_t == NFD_VNIC_TYPE_PF)
                nfd_build_pf_q(out_q, in_f, in_fq)
            .elif (in_t == NFD_VNIC_TYPE_VF)
                nfd_build_vf_q(out_q, in_f, in_fq)
            .else
                nfd_build_ctrl_q(out_q, in_f, in_fq)
            .endif
        #elif ((NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (in_t == NFD_VNIC_TYPE_PF)
                nfd_build_pf_q(out_q, in_f, in_fq)
            .else
                nfd_build_ctrl_q(out_q, in_f, in_fq)
            .endif
        #elif ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0))
            .if (in_t == NFD_VNIC_TYPE_PF)
                nfd_build_pf_q(out_q, in_f, in_fq)
            .else
                nfd_build_vf_q(out_q, in_f, in_fq)
            .endif
        #elif (NFD_MAX_PFS != 0)
            nfd_build_pf_q(out_q, in_f, in_fq)
        #elif (NFD_MAX_VFS != 0)
            nfd_build_vf_q(out_q, in_f, in_fq)
        #else
            #error "Unsupported vNIC parameters"
        #endif
    #endif
.end
#endm


#macro nfd_vid2natq(out_natq, in_vid, in_q)
.begin
    #if (isnum(in_vid) && isnum(in_q))
        move(out_natq, NFD_VID2NATQ(in_vid, in_q))
    #else
        .reg vnic

        #if ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (NFD_VID_IS_PF(in_vid))
                alu[vnic, in_vid, -, NFD_FIRST_PF]
                nfd_build_pf_q(out_natq, vnic, in_q)
            .elif (NFD_VID_IS_VF(in_vid))
                nfd_build_vf_q(out_natq, in_vid, in_q)
            .else
                nfd_build_ctrl_q(out_natq, 0, 0)
            .endif
        #elif ((NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (NFD_VID_IS_PF(in_vid))
                alu[vnic, in_vid, -, NFD_FIRST_PF]
                nfd_build_pf_q(out_natq, vnic, in_q)
            .else
                nfd_build_ctrl_q(out_natq, 0, 0)
            .endif
        #elif ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0))
            .if (NFD_VID_IS_PF(in_vid))
                alu[vnic, in_vid, -, NFD_FIRST_PF]
                nfd_build_pf_q(out_natq, vnic, in_q)
            .else
                nfd_build_vf_q(out_natq, in_vid, in_q)
            .endif
        #elif (NFD_MAX_PFS != 0)
            alu[vnic, in_vid, -, NFD_FIRST_PF]
            nfd_build_pf_q(out_natq, vnic, in_q)
        #elif (NFD_MAX_VFS != 0)
                nfd_build_vf_q(out_natq, in_vid, in_q)
        #else
            #error "Unsupported vNIC parameters"
        #endif

    #endif
.end
#endm


#macro nfd_vid2qid(out_qid, in_vid, in_q)
    nfd_vid2natq(out_qid, in_vid, in_q)
#endm


#macro nfd_vnic2vid(out_vid, in_t, in_f)
.begin
    #ifdef NFD_USE_CTRL
        #if (NFD_MAX_CTRL_QUEUES != 1 || NFD_MAX_CTRL != 1)
            #error "Unsupported CTRL vNIC values"
        #endif
    #endif

    #if (isnum(in_t) && isnum(in_f))
        move(out_vid, NFD_VNIC2VID(in_t, in_f))
    #elif (isnum(in_t))
        #if (in_t == NFD_VNIC_TYPE_PF)
            alu[out_vid, in_f, +, NFD_FIRST_PF]
        #elif (in_t == NFD_VNIC_TYPE_VF)
            alu[out_vid, --, b, in_f]
        #elif (in_t == NFD_VNIC_TYPE_CTRL)
            #if (NFD_MAX_CTRL_QUEUES != 1 || NFD_MAX_CTRL != 1)
                #error "Unsupported CTRL vNIC values"
            #endif
            move(out_vid, NFD_CTRL_VNIC)
        #else
            #error "Illegal vNIC type"
        #endif
    #else
        #if ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (in_t == NFD_VNIC_TYPE_PF)
                alu[out_vid, in_f, +, NFD_FIRST_PF]
            .elif (in_t == NFD_VNIC_TYPE_VF)
                alu[out_vid, --, b, in_f]
            .else
                move(out_vid, NFD_CTRL_VNIC)
            .endif
        #elif ((NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (in_t == NFD_VNIC_TYPE_PF)
                alu[out_vid, in_f, +, NFD_FIRST_PF]
            .else
                move(out_vid, NFD_CTRL_VNIC)
            .endif
        #elif ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0))
            .if (in_t == NFD_VNIC_TYPE_PF)
                alu[out_vid, in_f, +, NFD_FIRST_PF]
            .else (in_t == NFD_VNIC_TYPE_VF)
                alu[out_vid, --, b, in_f]
            .endif
        #elif (NFD_MAX_PFS != 0)
                alu[out_vid, in_f, +, NFD_FIRST_PF]
        #elif (NFD_MAX_VFS != 0)
                alu[out_vid, --, b, in_f]
        #else
            #error "Unsupported vNIC parameters"
        #endif
    #endif
.end
#endm


#macro nfd_vid2vf(out_t, out_f, in_vid, VF_TGT)
.begin
    move(out_t, NFD_VNIC_TYPE_VF)
    move(out_f, in_vid)

    #if (!streq('VF_TGT', '--'))
        br[VF_TGT]
    #endif
.end
#endm


#macro nfd_vid2ctrl(out_t, out_f, in_vid, CTRL_TGT)
.begin
    #if (NFD_MAX_CTRL_QUEUES != 1 || NFD_MAX_CTRL != 1)
        #error "Unsupported CTRL vNIC values"
    #endif

    move(out_t, NFD_VNIC_TYPE_CTRL)
    move(out_f, 0)

    #if (!streq('CTRL_TGT', '--'))
        br[CTRL_TGT]
    #endif
.end
#endm


#macro nfd_vid2pf(out_t, out_f, in_vid, PF_TGT)
.begin
    move(out_t, NFD_VNIC_TYPE_PF)

    #if (isnum(in_vid))
        move(out_f, (in_vid - NFD_FIRST_PF))
    #else
        alu[out_f, in_vid, -, NFD_FIRST_PF]
    #endif

    #if (!streq('PF_TGT', '--'))
        br[PF_TGT]
    #endif
.end
#endm


#macro nfd_vid2vnic(out_t, out_f, in_vid, VF_TGT, CTRL_TGT, PF_TGT)
.begin
    #if (isnum(in_vid))

        #if (NFD_VID_IS_PF(in_vid))
            nfd_vid2pf(out_t, out_f, in_vid, PF_TGT)
        #elif (NFD_VID_IS_VF(in_vid))
            nfd_vid2vf(out_t, out_f, in_vid, VF_TGT)
        #elif (NFD_VID_IS_CTRL(in_vid))
            nfd_vid2ctrl(out_t, out_f, in_vid, CTRL_TGT)
        #else
            #error "Invalid vid number"
        #endif
    #else
        #if ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (NFD_VID_IS_PF(in_vid))
                nfd_vid2pf(out_t, out_f, in_vid, PF_TGT)
            .elif (NFD_VID_IS_VF(in_vid))
                nfd_vid2vf(out_t, out_f, in_vid, VF_TGT)
            .else
                nfd_vid2ctrl(out_t, out_f, in_vid, CTRL_TGT)
            .endif
        #elif ((NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (NFD_VID_IS_PF(in_vid))
                nfd_vid2pf(out_t, out_f, in_vid, PF_TGT)
            .else
                nfd_vid2ctrl(out_t, out_f, in_vid, CTRL_TGT)
            .endif
        #elif ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0))
            .if (NFD_VID_IS_PF(in_vid))
                nfd_vid2pf(out_t, out_f, in_vid, PF_TGT)
            .else
                nfd_vid2vf(out_t, out_f, in_vid, VF_TGT)
            .endif
        #elif (NFD_MAX_PFS != 0)
            nfd_vid2pf(out_t, out_f, in_vid, PF_TGT)
        #elif (NFD_MAX_VFS != 0)
            nfd_vid2vf(out_t, out_f, in_vid, VF_TGT)
        #else
            #error "Unsupported vNIC parameters"
        #endif

    #endif
.end
#endm


#macro nfd_vid2vnic(out_t, out_f, in_vid)
    nfd_vid2vnic(out_t, out_f, in_vid, --, --, --)
#endm


#macro nfd_vnic_maxqs(out_maxqs, in_t, in_f)
    #if (isnum(in_t) && isnum(in_f))
        move(out_maxqs, NFD_VNIC_MAXQS(in_t, in_f))
    #elif (isnum(in_t))
        #if (in_t == NFD_VNIC_TYPE_PF)
            alu[out_maxqs, --, B, NFD_MAX_PF_QUEUES]
            #ifdef NFD_USE_OVERSUBSCRIPTION
                .if (in_f == (NFD_MAX_PFS -1))
                    alu[out_maxqs, --, B, NFD_LAST_PF_MAX_QUEUES]
                .endif
            #endif
        #elif (in_t == NFD_VNIC_TYPE_VF)
            alu[out_maxqs, --, B, NFD_MAX_VF_QUEUES]
        #elif (in_t == NFD_VNIC_TYPE_CTRL)
            alu[out_maxqs, --, B, NFD_MAX_CTRL_QUEUES]
        #else
            #error "Illegal vNIC type"
        #endif
    #else
        #if ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (in_t == NFD_VNIC_TYPE_PF)
                alu[out_maxqs, --, B, NFD_MAX_PF_QUEUES]
                #ifdef NFD_USE_OVERSUBSCRIPTION
                    .if (in_f == (NFD_MAX_PFS -1))
                        alu[out_maxqs, --, B, NFD_LAST_PF_MAX_QUEUES]
                    .endif
                #endif
            .elif (in_t == NFD_VNIC_TYPE_VF)
                alu[out_maxqs, --, B, NFD_MAX_VF_QUEUES]
            .else
                alu[out_maxqs, --, B, NFD_MAX_CTRL_QUEUES]
            .endif
        #elif ((NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (in_t == NFD_VNIC_TYPE_PF)
                alu[out_maxqs, --, B, NFD_MAX_PF_QUEUES]
                #ifdef NFD_USE_OVERSUBSCRIPTION
                    .if (in_f == (NFD_MAX_PFS -1))
                        alu[out_maxqs, --, B, NFD_LAST_PF_MAX_QUEUES]
                    .endif
                #endif
            .else
                alu[out_maxqs, --, B, NFD_MAX_CTRL_QUEUES]
            .endif
        #elif ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0))
            .if (in_t == NFD_VNIC_TYPE_PF)
                alu[out_maxqs, --, B, NFD_MAX_PF_QUEUES]
                #ifdef NFD_USE_OVERSUBSCRIPTION
                    .if (in_f == (NFD_MAX_PFS -1))
                        alu[out_maxqs, --, B, NFD_LAST_PF_MAX_QUEUES]
                    .endif
                #endif
            .else (in_t == NFD_VNIC_TYPE_VF)
                alu[out_maxqs, --, B, NFD_MAX_VF_QUEUES]
            .endif
        #elif (NFD_MAX_PFS != 0)
            alu[out_maxqs, --, B, NFD_MAX_PF_QUEUES]
            #ifdef NFD_USE_OVERSUBSCRIPTION
                .if (in_f == (NFD_MAX_PFS -1))
                    alu[out_maxqs, --, B, NFD_LAST_PF_MAX_QUEUES]
                .endif
            #endif
        #elif (NFD_MAX_VFS != 0)
            alu[out_maxqs, --, B, NFD_MAX_VF_QUEUES]
        #else
            #error "Unsupported vNIC parameters"
        #endif
    #endif
#endm


#macro nfd_vf_maxqs(out_maxqs, in_vid, VF_TGT)
    alu[out_maxqs, --, B, NFD_MAX_VF_QUEUES]
    #if (!streq('VF_TGT', '--'))
        br[VF_TGT]
    #endif

#endm


#macro nfd_ctrl_maxqs(out_maxqs, in_vid, CTRL_TGT)
    alu[out_maxqs, --, B, NFD_MAX_CTRL_QUEUES]
    #if (!streq('CTRL_TGT', '--'))
        br[CTRL_TGT]
    #endif
#endm


#macro nfd_pf_maxqs(out_maxqs, in_vid, PF_TGT)
    alu[out_maxqs, --, B, NFD_MAX_PF_QUEUES]
    #ifdef NFD_USE_OVERSUBSCRIPTION
        .if (in_vid == NFD_LAST_PF)
            alu[out_maxqs, --, B, NFD_LAST_PF_MAX_QUEUES]
        .endif
    #endif
    #if (!streq('PF_TGT', '--'))
        br[PF_TGT]
    #endif
#endm


#macro nfd_vid_maxqs(out_maxqs, in_vid, VF_TGT, CTRL_TGT, PF_TGT)
.begin
    #if (isnum(in_vid))
        #if (NFD_VID_IS_PF(in_vid))
            nfd_pf_maxqs(out_maxqs, in_vid, PF_TGT)
        #elif (NFD_VID_IS_VF(in_vid))
            nfd_vf_maxqs(out_maxqs, in_vid, VF_TGT)
        #elif (NFD_VID_IS_CTRL(in_vid))
            nfd_ctrl_maxqs(out_maxqs, in_vid, CTRL_TGT)
        #else
            #error "Invalid vid number"
        #endif
    #else
        #if ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (NFD_VID_IS_PF(in_vid))
                nfd_pf_maxqs(out_maxqs, in_vid, PF_TGT)
            .elif (NFD_VID_IS_VF(in_vid))
                nfd_vf_maxqs(out_maxqs, in_vid, VF_TGT)
            .else
                nfd_ctrl_maxqs(out_maxqs, in_vid, CTRL_TGT)
            .endif
        #elif ((NFD_MAX_PFS != 0) && defined(NFD_USE_CTRL))
            .if (NFD_VID_IS_PF(in_vid))
                nfd_pf_maxqs(out_maxqs, in_vid, PF_TGT)
            .else
                nfd_ctrl_maxqs(out_maxqs, in_vid, CTRL_TGT)
            .endif
        #elif ((NFD_MAX_VFS != 0) && (NFD_MAX_PFS != 0))
            .if (NFD_VID_IS_PF(in_vid))
                nfd_pf_maxqs(out_maxqs, in_vid, PF_TGT)
            .else
                nfd_vf_maxqs(out_maxqs, in_vid, VF_TGT)
            .endif
        #elif (NFD_MAX_PFS != 0)
            nfd_pf_maxqs(out_maxqs, in_vid, PF_TGT)
        #elif (NFD_MAX_VFS != 0)
            nfd_vf_maxqs(out_maxqs, in_vid, VF_TGT)
        #else
            #error "Unsupported vNIC parameters"
        #endif

    #endif
.end
#endm

#macro nfd_vid_maxqs(out_maxqs, in_vid)
    nfd_vid_maxqs(out_maxqs, in_vid, --, --, --)
#endm

#endif /* __NFD_COMMON_UC */
