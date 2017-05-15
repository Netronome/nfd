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
 * @file          blocks/vnic/nfd_in.uc
 * @brief         Microcode interface to PCI.IN
 */

#ifndef __NFD_IN_UC
#define __NFD_IN_UC

#include <nfp_chipres.h>
#include <stdmac.uc>
#include <bitfields.uc>

#include <nfd_user_cfg.h>

#include "shared/nfd_net.h"
#include "shared/nfd_api_common.h"
#include "nfd_common.uc"
#include "nfd_stats.uc"


#ifndef NFD_IN_DATA_OFFSET
#define NFD_IN_DATA_OFFSET      64
#endif

#ifndef NFD_IN_WQ_SZ
#error "NFD_IN_WQ_SZ must be defined by the user"
#endif

#ifndef NFD_IN_NUM_WQS
#define NFD_IN_NUM_WQS          8
#endif

#ifndef NFD_IN_BLM_REG_BLS
#error "NFD_IN_BLM_REG_BLS must be defined by the user"
#endif

#ifndef NFD_IN_BLM_REG_POOL
#error "NFD_IN_BLM_REG_POOL must be defined by the user"
#endif

#ifndef NFD_IN_BLM_REG_SIZE
#error "NFD_IN_BLM_REG_SIZE must be defined by the user"
#endif

#ifndef NFD_IN_BLM_JUMBO_BLS
#error "NFD_IN_BLM_JUMBO_BLS must be defined by the user"
#endif

#ifndef NFD_IN_BLM_JUMBO_POOL
#error "NFD_IN_BLM_JUMBO_POOL must be defined by the user"
#endif

#ifndef NFD_IN_BLM_JUMBO_SIZE
#error "NFD_IN_BLM_JUMBO_SIZE must be defined by the user"
#endif

#ifndef NFD_IN_BLM_RADDR
#error "NFD_IN_BLM_RADDR must be defined by the user"
#endif


#define NFD_IN_MAX_QUEUES       64


#ifndef NFD_IN_NUM_SEQRS
#define NFD_IN_NUM_SEQRS 1
#endif

#ifndef NFD_IN_SEQR_QSHIFT
#define NFD_IN_SEQR_QSHIFT 0
#endif

#if (NFD_IN_NUM_SEQRS < 1 || NFD_IN_NUM_SEQRS > 64 || \
    (NFD_IN_NUM_SEQRS & (NFD_IN_NUM_SEQRS - 1)) != 0)
#error "NFD_IN_NUM_SEQRS must be a power of 2 between 1 and 64"
#endif

#define NFD_IN_SEQR_NUM(_qnum) \
    (((_qnum) >> NFD_IN_SEQR_QSHIFT) & (NFD_IN_NUM_SEQRS - 1))


/**
 * PCI.in Packet descriptor format
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+-------------+-------------------------------+---+-----------+
 *    0  |S|    offset   |           reserved            |itf|   q_num   |
 *       +-+-+-+---------+-------------------------------+---+-----------+
 *    1  |I|J|S|                       buf_addr                          |
 *       +-+-+-+---------+---------------+-+-+---------------------------+
 *    2  |     flags     |   l4_offset   |L|S|           mss             |
 *       +---------------+---------------+-+-+---------------------------+
 *    3  |            data_len           |              vlan             |
 *       +-------------------------------+-------------------------------+
 *
 *       Flag bits (31-24) expanded:
 *          31       30      29      28      27      26      25     24
 *       +-------+-------+-------+-------+-------+-------+-------+-------+
 *    2  |TX_CSUM|IPV4_CS|TCP_CS |UDP_CS |TX_VLAN|TX_LSO |VXLAN  |GRE    |
 *       +-------+-------+-------+-------+-------+-------+-------+-------+
 *       This corresponds to nfp_net_pmd.h, TX descriptor format
 *       (lines 152-160).
 *
 *       31  TX_CSUM -> PCIE_DESC_TX_CSUM
 *       30  IPV4_CS -> PCIE_DESC_TX_IP4_CSUM
 *       29  TCP_CS  -> PCIE_DESC_TX_TCP_CSUM
 *       28  UDP_CS  -> PCIE_DESC_TX_UDP_CSUM
 *       27  TX_VLAN -> PCIE_DESC_TX_VLAN
 *       26  TX_LSO  -> PCIE_DESC_TX_LSO
 *       25  VXLAN   -> PCIE_DESC_TX_ENCAP_VXLAN
 *       24  GRE     -> PCIE_DESC_TX_ENCAP_GRE
 *
 *      S -> sp0 (spare)
 *    itf -> intf
 */
#define NFD_IN_OFFSET_fld       0, 30, 24
#define NFD_IN_SEQN_fld         0, 23, 8
#define NFD_IN_INTF_fld         0, 7, 6
#define NFD_IN_QID_fld          0, 5, 0
#define NFD_IN_INVALID_fld      1, 31, 31
#define NFD_IN_JUMBO_fld        1, 30, 30
#define NFD_IN_BUFADDR_fld      1, 28, 0
#define NFD_IN_FLAGS_fld        2, 31, 24
#define NFD_IN_FLAGS_TX_CSUM_fld        2, 31, 31
#define NFD_IN_FLAGS_TX_IPV4_CSUM_fld   2, 30, 30
#define NFD_IN_FLAGS_TX_TCP_CSUM_fld    2, 29, 29
#define NFD_IN_FLAGS_TX_UDP_CSUM_fld    2, 28, 28
#define NFD_IN_FLAGS_TX_VLAN_fld        2, 27, 27
#define NFD_IN_FLAGS_TX_LSO_fld         2, 26, 26
#define NFD_IN_FLAGS_TX_ENCAP_VXLAN_fld 2, 25, 25
#define NFD_IN_FLAGS_TX_ENCAP_GRE_fld   2, 24, 24
#define NFD_IN_L4OFF_fld        2, 23, 16
#define NFD_IN_LSO_fld          2, 23, 0
#define NFD_IN_LSO_SEQ_CNT_fld  2, 23, 16
#define NFD_IN_LSO_END_fld      2, 15, 15
#define NFD_IN_LSO_MSS_fld      2, 13, 0
#define NFD_IN_DATALEN_fld      3, 31, 16
#define NFD_IN_VLAN_fld         3, 15, 0
#define NFD_IN_LSO2_L3_OFFS_fld 3, 7, 0
#define NFD_IN_LSO2_L4_OFFS_fld 3, 15, 8
#define NFD_IN_META_SIZE        16
#define NFD_IN_META_SIZE_LW     4


#define NFD_IN_RING_HI_fld      0, 31, 24
#define NFD_IN_RING_SP_fld      0, 23, 12
#define NFD_IN_RING_LO_fld      0, 11, 0

#define NFD_IN_RING_INFO_ITEM_SZ   4



#macro nfd_in_recv_init()

    .alloc_mem nfd_in_ring_info lm me \
        (NFD_MAX_ISL * NFD_IN_RING_INFO_ITEM_SZ) \
        (NFD_MAX_ISL * NFD_IN_RING_INFO_ITEM_SZ)

    nfd_stats_declare_in()

#ifdef NFD_IN_WQ_SHARED

    #define __EMEM_NUM
    #define_eval __EMEM_NUM strright('NFD_IN_WQ_SHARED', 1)

    #if defined(NFD_PCIE0_EMEM) || defined(NFD_PCIE1_EMEM) || defined(NFD_PCIE2_EMEM) || defined(NFD_PCIE3_EMEM)

        .alloc_resource nfd_in_ring_nums0 NFD_IN_WQ_SHARED/**/_queues global NFD_IN_NUM_WQS NFD_IN_NUM_WQS
        .declare_resource nfd_in_ring_nums_res0 global NFD_IN_NUM_WQS nfd_in_ring_nums0
        .alloc_resource nfd_in_ring_num00 nfd_in_ring_nums_res0 global 1

    #endif /* defined(NFD_PCIE0_EMEM) || defined(NFD_PCIE1_EMEM) || defined(NFD_PCIE2_EMEM) || defined(NFD_PCIE3_EMEM) */

    #ifdef NFD_PCIE0_EMEM

        .init nfd_in_ring_info+0 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_in_ring_num00)

    #endif /* NFD_PCIE0_EMEM */

    #ifdef NFD_PCIE1_EMEM

        .init nfd_in_ring_info+4 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_in_ring_num00)

    #endif /* NFD_PCIE1_EMEM */

    #ifdef NFD_PCIE2_EMEM

        .init nfd_in_ring_info+8 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_in_ring_num00)

    #endif /* NFD_PCIE2_EMEM */

    #ifdef NFD_PCIE3_EMEM

        .init nfd_in_ring_info+12 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_in_ring_num00)

    #endif /* NFD_PCIE1_EMEM */

    #undef __EMEM_NUM

#else /* NFD_IN_WQ_SHARED */

    #ifdef NFD_PCIE0_EMEM

        .alloc_resource nfd_in_ring_nums0 NFD_PCIE0_EMEM/**/_queues global NFD_IN_NUM_WQS NFD_IN_NUM_WQS
        .declare_resource nfd_in_ring_nums_res0 global NFD_IN_NUM_WQS nfd_in_ring_nums0
        .alloc_resource nfd_in_ring_num00 nfd_in_ring_nums_res0 global 1

        #define __EMEM_NUM
        #define_eval __EMEM_NUM strright('NFD_PCIE0_EMEM', 1)
        .init nfd_in_ring_info+0 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_in_ring_num00)
        #undef __EMEM_NUM

    #endif /* NFD_PCIE0_EMEM */

    #ifdef NFD_PCIE1_EMEM

        .alloc_resource nfd_in_ring_nums1 NFD_PCIE1_EMEM/**/_queues global NFD_IN_NUM_WQS NFD_IN_NUM_WQS
        .declare_resource nfd_in_ring_nums_res1 global NFD_IN_NUM_WQS nfd_in_ring_nums1
        .alloc_resource nfd_in_ring_num10 nfd_in_ring_nums_res1 global 1

        #define __EMEM_NUM
        #define_eval __EMEM_NUM strright('NFD_PCIE1_EMEM', 1)
        .init nfd_in_ring_info+4 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_in_ring_num10)
        #undef __EMEM_NUM

    #endif /* NFD_PCIE1_EMEM */

    #ifdef NFD_PCIE2_EMEM

        .alloc_resource nfd_in_ring_nums2 NFD_PCIE2_EMEM/**/_queues global NFD_IN_NUM_WQS NFD_IN_NUM_WQS
        .declare_resource nfd_in_ring_nums_res2 global NFD_IN_NUM_WQS nfd_in_ring_nums2
        .alloc_resource nfd_in_ring_num20 nfd_in_ring_nums_res2 global 1

        #define __EMEM_NUM
        #define_eval __EMEM_NUM strright('NFD_PCIE2_EMEM', 1)
        .init nfd_in_ring_info+8 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_in_ring_num20)
        #undef __EMEM_NUM

    #endif /* NFD_PCIE2_EMEM */

    #ifdef NFD_PCIE3_EMEM

        .alloc_resource nfd_in_ring_nums3 NFD_PCIE3_EMEM/**/_queues global NFD_IN_NUM_WQS NFD_IN_NUM_WQS
        .declare_resource nfd_in_ring_nums_res3 global NFD_IN_NUM_WQS nfd_in_ring_nums3
        .alloc_resource nfd_in_ring_num30 nfd_in_ring_nums_res3 global 1

        #define __EMEM_NUM
        #define_eval __EMEM_NUM strright('NFD_PCIE3_EMEM', 1)
        .init nfd_in_ring_info+12 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_in_ring_num30)
        #undef __EMEM_NUM

    #endif /* NFD_PCIE1_EMEM */

#endif /* NFD_IN_WQ_SHARED */

#endm



#macro nfd_in_recv(out_nfd_meta, in_pcie_isl, in_recvq, LM_CTX, SIGNAL, SIGTYPE)
.begin
    .reg addr_hi
    .reg addr_lo

    #if (is_ct_const(in_pcie_isl))
        immed[addr_lo, (nfd_in_ring_info +
                        (in_pcie_isl << log2(NFD_IN_RING_INFO_ITEM_SZ)))]
    #else
        passert(nfd_in_ring_info,  "MULTIPLE_OF",
                (NFD_MAX_ISL * NFD_IN_RING_INFO_ITEM_SZ))
        immed[addr_lo, nfd_in_ring_info]
        alu[addr_lo, addr_lo, OR, in_pcie_isl,
            <<(log2(NFD_IN_RING_INFO_ITEM_SZ))]
    #endif
    local_csr_wr[ACTIVE_LM_ADDR_/**/LM_CTX, addr_lo]
    nop
    nop
    nop
    ld_field_w_clr[addr_hi, 1000, *l$index/**/LM_CTX]
    alu[addr_lo, in_recvq, +16, *l$index/**/LM_CTX]

    #if (streq('SIGTYPE', 'SIG_DONE'))
        mem[qadd_thread, out_nfd_meta[0], addr_hi, <<8, addr_lo, NFD_IN_META_SIZE_LW], sig_done[SIGNAL]
    #elif (streq('SIGTYPE', 'SIG_WAIT'))
        mem[qadd_thread, out_nfd_meta[0], addr_hi, <<8, addr_lo, NFD_IN_META_SIZE_LW], ctx_swap[SIGNAL]
    #else
        #error "Unknown signal handling type"
    #endif
.end
#endm


#macro nfd_in_recv(io_nfd_meta, in_pcie_isl, in_recvq, LM_CTX)
.begin
    .reg pktlen
    .reg pcinum
    .reg qid

    .sig nfd_in_sig
    nfd_in_recv(io_nfd_meta, in_pcie_isl, in_recvq, LM_CTX, nfd_in_sig,
                SIG_WAIT)

    nfd_in_get_pkt_len(pktlen, io_nfd_meta)
    nfd_in_get_pcie(pcinum, io_nfd_meta)
    nfd_in_get_qid(qid, io_nfd_meta)
    nfd_stats_update_received(pcinum, qid, pktlen)
.end
#endm


#macro nfd_in_pkt_meta(out_pkt_meta, in_nfd_meta)
.begin
    .reg v
    .reg len
    .reg off
    .reg bls

    bitfield_extract(len, BF_AML(in_nfd_meta, NFD_IN_DATALEN_fld))

#if (NFD_IN_BLM_REG_BLS == NFD_IN_BLM_JUMBO_BLS)
    bitfield_insert(v, 0, NFD_IN_BLM_REG_BLS, BF_ML(PKT_META_BUFLIST_bf))
#else
   move(bls, NFD_IN_BLM_JUMBO_BLS)
   .if (BIT(BF_A(in_nfd_meta, NFD_IN_JUMBO_fld), BL_L(in_nfd_meta, NFD_IN_JUMBO_fld)) == 0)
       move(bls, NFD_IN_BLM_REG_BLS)
   .endif
    bitfield_insert(v, 0, bls, BF_ML(PKT_META_BUFLIST_bf))
#endif

    bitfield_extract(off, BF_AML(in_nfd_meta, NFD_IN_OFFSET_fld))
    alu[len, len, -, off]
    bitfield_insert(BF_A(out_pkt_meta, PKT_META_LEN_bf), v, len, BF_ML(PKT_META_LEN_bf))
    bitfield_extract(v, BF_AML(in_nfd_meta, NFD_IN_BUFADDR_fld))
    bitfield_insert(BF_A(out_pkt_meta, PKT_META_MUPTR_bf), 0, v, BF_ML(PKT_META_MUPTR_bf))
.end
#endm


#macro nfd_in_qid_to_seqr(out_seqr, in_qid)
.begin
    alu[out_seqr, (NFD_IN_NUM_SEQRS - 1), AND, in_qid, >>NFD_IN_SEQR_QSHIFT]
.end
#endm


#macro nfd_in_get_seqr(out_seqr, in_nfd_meta)
.begin
    .reg qid
    nfd_in_get_qid(qid, in_nfd_meta)
    nfd_in_qid_to_seqr(out_seqr, qid)
.end
#endm


#macro nfd_in_get_seqn(out_seqn, in_nfd_meta)
.begin
    bitfield_extract(out_seqn, BF_AML(in_nfd_meta, NFD_IN_SEQN_fld))
.end
#endm


#macro nfd_in_get_pcie(out_pcie, in_nfd_meta)
.begin
    bitfield_extract(out_pcie, BF_AML(in_nfd_meta, NFD_IN_INTF_fld))
.end
#endm


#macro nfd_in_get_qid(out_qid, in_nfd_meta)
.begin
    bitfield_extract(out_qid, BF_AML(in_nfd_meta, NFD_IN_QID_fld))
.end
#endm


#macro nfd_in_get_muptr(out_muptr, in_nfd_meta)
.begin
    bitfield_extract(out_muptr, BF_AML(in_nfd_meta, NFD_IN_BUFADDR_fld))
.end
#endm


#macro nfd_in_get_metalen(out_metalen, in_nfd_meta)
.begin
    bitfield_extract(out_metalen, BF_AML(in_nfd_meta, NFD_IN_OFFSET_fld))
.end
#endm


#macro nfd_in_get_flags(out_flags, in_nfd_meta)
.begin
    bitfield_extract(out_flags, BF_AML(in_nfd_meta, NFD_IN_FLAGS_fld))
.end
#endm

#macro nfd_in_get_tx_csum(out_value, in_nfd_meta)
.begin
    bitfield_extract__sz1(out_value, BF_AML(in_nfd_meta, NFD_IN_FLAGS_TX_CSUM_fld))
.end
#endm
#macro nfd_in_get_tx_ipv4_csum(out_value, in_nfd_meta)
.begin
    bitfield_extract__sz1(out_value, BF_AML(in_nfd_meta, NFD_IN_FLAGS_TX_IPV4_CSUM_fld))
.end
#endm
#macro nfd_in_get_tx_tcp_csum(out_value, in_nfd_meta)
.begin
    bitfield_extract__sz1(out_value, BF_AML(in_nfd_meta, NFD_IN_FLAGS_TX_TCP_CSUM_fld))
.end
#endm
#macro nfd_in_get_tx_udp_csum(out_value, in_nfd_meta)
.begin
    bitfield_extract__sz1(out_value, BF_AML(in_nfd_meta, NFD_IN_FLAGS_TX_UDP_CSUM_fld))
.end
#endm
#macro nfd_in_get_tx_vlan(out_value, in_nfd_meta)
.begin
    bitfield_extract__sz1(out_value, BF_AML(in_nfd_meta, NFD_IN_FLAGS_TX_VLAN_fld))
.end
#endm
#macro nfd_in_get_tx_lso(out_value, in_nfd_meta)
.begin
    bitfield_extract__sz1(out_value, BF_AML(in_nfd_meta, NFD_IN_FLAGS_TX_LSO_fld))
.end
#endm
#macro nfd_in_get_tx_encap_vxlan(out_value, in_nfd_meta)
.begin
    bitfield_extract__sz1(out_value, BF_AML(in_nfd_meta, NFD_IN_FLAGS_TX_ENCAP_VXLAN_fld))
.end
#endm
#macro nfd_in_get_tx_encap_gre(out_value, in_nfd_meta)
.begin
    bitfield_extract__sz1(out_value, BF_AML(in_nfd_meta, NFD_IN_FLAGS_TX_ENCAP_GRE_fld))
.end
#endm

#macro nfd_in_get_data_len(out_data_len, in_nfd_meta)
.begin
    bitfield_extract(out_data_len, BF_AML(in_nfd_meta, NFD_IN_DATALEN_fld))
.end
#endm


#macro nfd_in_get_vlan(out_vlan, in_nfd_meta)
.begin
    bitfield_extract(out_vlan, BF_AML(in_nfd_meta, NFD_IN_VLAN_fld))
.end
#endm


#macro nfd_in_get_pkt_len(out_pkt_len, in_nfd_meta)
.begin
    .reg len
    .reg off
    bitfield_extract(len, BF_AML(in_nfd_meta, NFD_IN_DATALEN_fld))
    bitfield_extract(off, BF_AML(in_nfd_meta, NFD_IN_OFFSET_fld))
    alu[out_pkt_len, len, -, off]
.end
#endm


#macro nfd_in_get_offset(out_pkt_off, in_nfd_meta)
.begin
    move(out_pkt_off, NFD_IN_DATA_OFFSET)
.end
#endm



#endif /* __NFD_IN_UC */
