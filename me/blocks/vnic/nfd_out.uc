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
 * @file          blocks/vnic/nfd_out.uc
 * @brief         Microcode interface to PCI.OUT
 */

#ifndef __NFD_OUT_UC
#define __NFD_OUT_UC

#include <nfp_chipres.h>
#include <stdmac.uc>
#include <ov.uc>
#include <passert.uc>

#include <nfd_user_cfg.h>

#include "shared/nfd_net.h"
#include "shared/nfd_api_common.h"
#include "nfd_common.uc"
#include "nfd_stats.uc"

/**
 * Packet prepend format for packets going to the host that need to
 * include RSS or input port information.
 *
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-----------+-+-----------------+---+-+-------------------------+
 *    0  |  CTM ISL  |C|  Packet Number  |SPL|0|     Starting Offset     |
 *       +-+---+-----+-+-----------------+---+-+-------------------------+
 *    1  |N|BLS|           MU Buffer Address [39:11]                     |
 *       +-+---+---------+---------------+-------------------------------+
 *    2  |D| Meta Length |  RX Queue     |           Data Length         |
 *       +-+-------------+---------------+-------------------------------+
 *    3  |             VLAN              |             Flags             |
 *       +-------------------------------+-------------------------------+
 */

#define NFD_OUT_CTM_ISL_fld     0, 31, 26       /* Island of packet CTM buf */
#define NFD_OUT_CTM_ISL_wrd     0
#define NFD_OUT_CTM_ISL_shf     26
#define NFD_OUT_CTM_ISL_msk     0x3F
#define NFD_OUT_CTM_ONLY_fld    0, 25, 25       /* Packet data in CTM only */
#define NFD_OUT_CTM_ONLY_wrd    0
#define NFD_OUT_CTM_ONLY_shf    25
#define NFD_OUT_CTM_ONLY_msk    0x1
#define NFD_OUT_PKTNUM_fld      0, 24, 16       /* CTM packet number */
#define NFD_OUT_PKTNUM_wrd      0
#define NFD_OUT_PKTNUM_shf      16
#define NFD_OUT_PKTNUM_msk      0x1FF
#define NFD_OUT_SPLIT_fld       0, 15, 14       /* split length of packet */
#define NFD_OUT_SPLIT_wrd       0
#define NFD_OUT_SPLIT_shf       14
#define NFD_OUT_SPLIT_msk       0x3
#define NFD_OUT_OFFSET_fld      0, 12, 0        /* Offset where packet starts */
#define NFD_OUT_OFFSET_wrd      0
#define NFD_OUT_OFFSET_shf      0
#define NFD_OUT_OFFSET_msk      0x1FFF
#define NFD_OUT_NBI_fld         1, 31, 31       /* Receiving NBI (don't use) */
#define NFD_OUT_NBI_wrd         1
#define NFD_OUT_NBI_shf         31
#define NFD_OUT_NBI_msk         0x1
#define NFD_OUT_BLS_fld         1, 30, 29       /* Buffer list of MU buf */
#define NFD_OUT_BLS_wrd         1
#define NFD_OUT_BLS_shf         29
#define NFD_OUT_BLS_msk         0x3
#define NFD_OUT_MUADDR_fld      1, 28, 0        /* MU addr right-shifted 11 */
#define NFD_OUT_MUADDR_wrd      1
#define NFD_OUT_MUADDR_shf      0
#define NFD_OUT_MUADDR_msk      0x1FFFFFFF
#define NFD_OUT_DD_fld          2, 31, 31       /* Must be 1 */
#define NFD_OUT_DD_wrd          2
#define NFD_OUT_DD_shf          31
#define NFD_OUT_DD_msk          0x1
#define NFD_OUT_METALEN_fld     2, 30, 24       /* Length of prepended meta */
#define NFD_OUT_METALEN_wrd     2
#define NFD_OUT_METALEN_shf     24
#define NFD_OUT_METALEN_msk     0x7F
#define NFD_OUT_QID_fld         2, 23, 16       /* Queue to send to */
#define NFD_OUT_QID_wrd         2
#define NFD_OUT_QID_shf         16
#define NFD_OUT_QID_msk         0xFF
#define NFD_OUT_LEN_fld         2, 15, 0        /* Total length of packet */
#define NFD_OUT_LEN_wrd         2
#define NFD_OUT_LEN_shf         0
#define NFD_OUT_LEN_msk         0xFFFF
#define NFD_OUT_VLAN_fld        3, 31, 16       /* Stripped vlan of packet */
#define NFD_OUT_VLAN_wrd        3
#define NFD_OUT_VLAN_shf        16
#define NFD_OUT_VLAN_msk        0xFFFF
#define NFD_OUT_FLAGS_fld       3, 15, 0        /* RX flags */
#define NFD_OUT_FLAGS_wrd       3
#define NFD_OUT_FLAGS_shf       0
#define NFD_OUT_FLAGS_msk       0xFFFF

/**
 * Packet prepend format for packets going to the host that need to
 * include RSS or input port information.
 *
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-----------------------------------------------+---------------+
 *    0  |             Input Port                        | RSS Hash Type |
 *       +-----------------------------------------------+---------------+
 *    1  |                         RSS Hash                              |
 *       +---------------------------------------------------------------+
 */

#define NFD_OUT_PRE_INPORT_fld          0, 31, 8
#define NFD_OUT_PRE_INPORT_wrd          0
#define NFD_OUT_PRE_INPORT_shf          8
#define NFD_OUT_PRE_INPORT_msk          0xFFFFFF
#define NFD_OUT_PRE_RSS_HTYPE_fld       0, 7, 0
#define NFD_OUT_PRE_RSS_HTYPE_wrd       0
#define NFD_OUT_PRE_RSS_HTYPE_shf       0
#define NFD_OUT_PRE_RSS_HTYPE_msk       0xFF
#define NFD_OUT_PRE_RSS_HASH_fld        1, 31, 0
#define NFD_OUT_PRE_RSS_HASH_wrd        1
#define NFD_OUT_PRE_RSS_HASH_shf        0
#define NFD_OUT_PRE_RSS_HASH_msk        0xFFFFFFFF
#define NFD_OUT_PRE_FULL_SIZE_LW        2
#define NFD_OUT_PRE_FULL_SIZE           8

#define NFD_OUT_MAX_QUEUES      64


#define NFD_OUT_DESC_SIZE_LW    4
#define NFD_OUT_XFER_SIZE_LW    4

#define NFD_OUT_ATOMICS_SZ          16
#define NFD_OUT_ATOMICS_SZ_LG2      4
#define NFD_OUT_ATOMICS_CREDIT      0
#define NFD_OUT_ATOMICS_SENT        4
#define NFD_OUT_ATOMICS_DMA_DONE    8

#define NFD_OUT_RING_INFO_ITEM_SZ   4


/* Define the default maximum length in bytes of prepended chained metadata.
 * Assume one 32-bit word is used to encode the metadata types in a chain and
 * that each metadata value for a corresponding metadata type is 4 bytes
 * long. */
#ifndef NFD_OUT_MAX_META_LEN
#define NFD_OUT_MAX_META_LEN (4 * 32 / NFP_NET_META_FIELD_SIZE + 4)
#endif

/* Maximum length of a single meta data item that will be prepended using
 * nfd_out_metadata_push() */
#ifndef NFD_OUT_MAX_META_ITEM_LEN
#define NFD_OUT_MAX_META_ITEM_LEN 4
#endif


#macro nfd_out_ring_declare()

    #ifndef __NFD_OUT_RINGS_DECLARED
    #define __NFD_OUT_RINGS_DECLARED

        #ifdef NFD_PCIE0_EMEM

            .alloc_resource nfd_out_ring_num00 NFD_PCIE0_EMEM/**/_queues global 1 1

        #endif /* NFD_PCIE0_EMEM */

        #ifdef NFD_PCIE1_EMEM

            .alloc_resource nfd_out_ring_num10 NFD_PCIE1_EMEM/**/_queues global 1 1

        #endif /* NFD_PCIE1_EMEM */

        #ifdef NFD_PCIE2_EMEM

            .alloc_resource nfd_out_ring_num20 NFD_PCIE2_EMEM/**/_queues global 1 1

        #endif /* NFD_PCIE2_EMEM */

        #ifdef NFD_PCIE3_EMEM

            .alloc_resource nfd_out_ring_num30 NFD_PCIE3_EMEM/**/_queues global 1 1

        #endif /* NFD_PCIE3_EMEM */

    #endif /* __NFD_OUT_RINGS_DECLARED */

#endm


#macro nfd_out_send_init()

    nfd_out_ring_declare()
    nfd_stats_declare_out()

    .alloc_mem nfd_out_ring_info lm me \
        (NFD_MAX_ISL * NFD_OUT_RING_INFO_ITEM_SZ) \
        (NFD_MAX_ISL * NFD_OUT_RING_INFO_ITEM_SZ)

    #ifdef NFD_PCIE0_EMEM

        #define __EMEM_NUM
        #define_eval __EMEM_NUM strright('NFD_PCIE0_EMEM', 1)
        .init nfd_out_ring_info+0 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_out_ring_num00)
        #undef __EMEM_NUM

    #endif /* NFD_PCIE0_EMEM */

    #ifdef NFD_PCIE1_EMEM

        #define __EMEM_NUM
        #define_eval __EMEM_NUM strright('NFD_PCIE1_EMEM', 1)
        .init nfd_out_ring_info+4 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_out_ring_num10)
        #undef __EMEM_NUM

    #endif /* NFD_PCIE1_EMEM */

    #ifdef NFD_PCIE2_EMEM

        #define __EMEM_NUM
        #define_eval __EMEM_NUM strright('NFD_PCIE2_EMEM', 1)
        .init nfd_out_ring_info+8 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_out_ring_num20)
        #undef __EMEM_NUM

    #endif /* NFD_PCIE2_EMEM */

    #ifdef NFD_PCIE3_EMEM

        #define __EMEM_NUM
        #define_eval __EMEM_NUM strright('NFD_PCIE3_EMEM', 1)
        .init nfd_out_ring_info+12 \
            ((((__NFD_EMU_BASE_ISL+__EMEM_NUM) | __NFD_DIRECT_ACCESS) << 24) | nfd_out_ring_num30)
        #undef __EMEM_NUM

    #endif /* NFD_PCIE3_EMEM */

#endm


#macro nfd_out_get_credits(io_cred_avail, in_pcie, in_qid, in_ncred, SIGPAIR, SIGTYPE)
.begin
    .reg addr_hi
    .reg addr_lo

    // XXX NFD assumes credits live at address 0 in the CTM of the PCIe island
    #if (is_ct_const(in_pcie))
        move(addr_hi, (((in_pcie + NFD_PCIE_ISL_BASE) | 0x80) << 24))
    #else
        alu[addr_hi, in_pcie, +, (NFD_PCIE_ISL_BASE | __NFD_DIRECT_ACCESS)]
        alu[addr_hi, --, B, addr_hi, <<24]
    #endif

    #if (is_ct_const(in_qid))
        move(addr_lo, (in_qid << log2(NFD_OUT_ATOMICS_SZ)))
    #else
        alu[addr_lo, --, B, in_qid, <<(log2(NFD_OUT_ATOMICS_SZ))]
    #endif

    move(io_cred_avail, in_ncred)
    mem[test_subsat, io_cred_avail, addr_hi, <<8, addr_lo, 1], sig_done[SIGPAIR]
    #if (streq('SIGTYPE', 'SIG_DONE'))
    #elif (streq('SIGTYPE', 'SIG_WAIT'))
        ctx_arb[SIGPAIR]
    #else
        #error "Unknown signal handling type"
    #endif
.end
#endm


#macro nfd_out_get_credits(io_ncred, in_pcie, in_nfd_queue)
.begin
    .sig nfd_out_credit_sig
    .reg $x
    nfd_out_get_credits($x, in_pcie, in_nfd_queue, io_ncred, nfd_out_credit_sig, SIG_WAIT)
    .if ($x < io_ncred)
        move(io_ncred, $x)
    .endif
.end
#endm


/**
 * Prepend packet with metadata.
 * @param io_meta_len       Length of metadata currently prepended
 * @param in_meta_types     Type of metadata being prepended
 * @param in_meta_val       Optionally an array of metadata to prepend
 * @param in_meta_type_num  Number of metadata type fields being prepended
 * @param in_meta_val_len   Length of metadata value in bytes
 * @param in_pkt_start_hi   Upper bits of packet start address
 * @param in_pkt_start_lo   Lower bits of packet start address
 * @param IN_ERROR_LABEL    Branch to label if metadata cannot be prepended
 *
 * The application firmware initialises in_meta_len to zero and it is then
 * updated by this macro as metadata is prepended. Branches to error label
 * if maximum supported metadata length is exceeded.
 *
 * @note Set in_meta_val_len to -- when in_meta_val is a scalar value
 */
#macro nfd_out_metadata_push(io_meta_len, in_meta_types, in_meta_val, \
                             in_meta_type_num, in_meta_val_len, \
                             in_pkt_start_hi, in_pkt_start_lo, IN_ERROR_LABEL)
.begin

    .reg _pkt_start_lo
    .reg meta_offset
    .reg meta_len_diff
    .reg $meta_info

    passert(is_ct_const(in_meta_type_num))
    #if (!streq('in_meta_val_len', '--'))
        passert(is_ct_const(in_meta_val_len))
        passert(in_meta_val_len <= NFD_MAX_META_VAL_LEN)
        passert(in_meta_val_len <= NFD_OUT_MAX_META_ITEM_LEN)
        passert(in_meta_val_len % 4 == 0)
        #define_eval _META_VAL_LEN in_meta_val_len
    #else
        #define _META_VAL_LEN 4
    #endif

    .reg $meta_data[(1 + (_META_VAL_LEN >> 2))]
    .xfer_order $meta_data
    .sig sig_meta
    .set $meta_data[0]

    move(_pkt_start_lo, in_pkt_start_lo)

    .if (io_meta_len == 0)

        /* This is the first word of metadata being prepended. */
        move($meta_data[0], in_meta_types)
        alu[io_meta_len, --, B, 4]

    .elif (io_meta_len <= (NFD_OUT_MAX_META_LEN - _META_VAL_LEN))

        alu[meta_offset, _pkt_start_lo, -, io_meta_len]
        mem[read32, $meta_info, in_pkt_start_hi, <<8, meta_offset, 1], \
           ctx_swap[sig_meta]
        alu[$meta_data[0], in_meta_types, OR, $meta_info, \
            <<(NFP_NET_META_FIELD_SIZE * in_meta_type_num)]

    .else

        #if (!streq('IN_ERROR_LABEL', '--'))
            br[IN_ERROR_LABEL]
        #else
            br[meta_push_err#]
        #endif

    .endif

    #if (!streq('in_meta_val_len', '--'))
        aggregate_copy($meta_data, 1, in_meta_val, 0, \
                       (_META_VAL_LEN>>2))
    #else
        move($meta_data[1], in_meta_val)
    #endif

    alu[io_meta_len, io_meta_len, +, _META_VAL_LEN]
    alu[meta_offset, _pkt_start_lo, -, io_meta_len]

    #define_eval _META_LW (1 + (_META_VAL_LEN >> 2))
    ov_single(OV_LENGTH, _META_LW, OVF_SUBTRACT_ONE)
    mem[write32, $meta_data[0], in_pkt_start_hi, <<8, meta_offset, \
        max_/**/_META_LW], indirect_ref, ctx_swap[sig_meta]

meta_push_err#:

    #undef _META_VAL_LEN
    #undef _META_LW

.end
#endm


/* Call nfd_out_metadata_push() with in_meta_val as a scalar */
#macro nfd_out_metadata_push(io_meta_len, in_meta_types, in_meta_val, \
                             in_meta_type_num, \
                             in_pkt_start_hi, in_pkt_start_lo, IN_ERROR_LABEL)
    nfd_out_metadata_push(io_meta_len, in_meta_types, in_meta_val, \
                          in_meta_type_num, --, \
                          in_pkt_start_hi, in_pkt_start_lo, IN_ERROR_LABEL)
#endm


#macro nfd_out_fill_desc(io_nfd_desc, in_ctm_isl, in_ctm_pnum, in_bls, \
                         in_muptr, in_pkt_len, in_nbi, in_ctm_split, \
                         in_pkt_offset, in_meta_len)
.begin
    .reg tmp
    .reg eoff
    .reg data_len
    .reg data_offset

    /* sanity */
    // io_nfd_desc[0]..[2] are zeroed with alu "B" ops below
    move(io_nfd_desc[3], 0)

    #if (is_ct_const(in_pkt_offset) && (in_pkt_offset >= 256))
        #error "nfd_out_fill_desc in_pkt_offset must be <256"
    #endif

    /* word 0 */
    passert(BF_W(NFD_OUT_OFFSET_fld), "EQ", 0)
    #if (is_ct_const(in_meta_len))
        #if (in_meta_len != 0)
            #if (is_ct_const(in_pkt_offset))
                alu[BF_A(io_nfd_desc, NFD_OUT_OFFSET_fld), --, B,
                    (in_pkt_offset - in_meta_len), <<BF_L(NFD_OUT_OFFSET_fld)]
            #else
                alu[data_offset, in_pkt_offset, -, in_meta_len]
                alu[BF_A(io_nfd_desc, NFD_OUT_OFFSET_fld), --, B,
                    data_offset, <<BF_L(NFD_OUT_OFFSET_fld)]
            #endif
        #else /* in_meta_len != 0 */
            /* XXX alu_shf supports parameters in reg or up to 8bit immed */
            alu[BF_A(io_nfd_desc, NFD_OUT_OFFSET_fld), --, B, in_pkt_offset,
                <<BF_L(NFD_OUT_OFFSET_fld)]
        #endif /* in_meta_len != 0 */
    #else /* is_ct_const(in_meta_len) */
        #if (is_ct_const(in_pkt_offset))
            alu[data_offset, in_pkt_offset, -, in_meta_len]
        #else
            /* XXX pkt_len, meta_len, and pkt_off all interact
             * so we need an intermediate variable to resolve
             * A-B conflicts.  This is the best place to put it. */
            alu[data_offset, --, B, in_pkt_offset]
            alu[data_offset, data_offset, -, in_meta_len]
        #endif
        alu[BF_A(io_nfd_desc, NFD_OUT_OFFSET_fld), --, B,
            data_offset, <<BF_L(NFD_OUT_OFFSET_fld)]
    #endif /* is_ct_const(in_meta_len) */

    #if (!is_ct_const(in_ctm_isl) || (in_ctm_isl != 0))
        bits_set__sz1(BF_AL(io_nfd_desc, NFD_OUT_CTM_ISL_fld), in_ctm_isl)
    #endif
    #if (!is_ct_const(in_ctm_pnum) || (in_ctm_pnum != 0))
        bits_set__sz1(BF_AL(io_nfd_desc, NFD_OUT_PKTNUM_fld), in_ctm_pnum)
    #endif
    #if (!is_ct_const(in_ctm_split) || (in_ctm_split != 0))
        bits_set__sz1(BF_AL(io_nfd_desc, NFD_OUT_SPLIT_fld), in_ctm_split)
    #endif

    /* Not CTM only if it's MU-buffer only */
    alu[--, --, B, in_ctm_isl]
    beq[not_ctm_only#]

    /* check whether the ending offset of the packet */
    /* goes past the end of the CTM buffer.  If so, */
    /* set the CTM_ONLY bit in the packet. */
    alu[eoff, in_pkt_len, +, in_pkt_offset]
    #if (is_ct_const(in_ctm_split))
        immed[tmp, (256 << in_ctm_split)]
    #else
        alu[tmp, in_ctm_split, B, 1, <<8]
        alu[tmp, --, B, tmp, <<indirect]
    #endif
    alu[--, eoff, -, tmp]
    bgt[not_ctm_only#]
    bits_set__sz1(BF_AL(io_nfd_desc, NFD_OUT_CTM_ONLY_fld), 1)

not_ctm_only#:
    /* word 1 */
    passert(BF_W(NFD_OUT_BLS_fld), "EQ", 1)
    passert(BF_W(NFD_OUT_MUADDR_fld), "EQ", 1)
    passert(BF_L(NFD_OUT_MUADDR_fld), "EQ", 0)

    #define_eval __NFD_OUT_MUADDR_inv_msk \
        ((1 << (31 - BF_M(NFD_OUT_MUADDR_fld))) - 1)

    alu[BF_A(io_nfd_desc, NFD_OUT_MUADDR_fld), in_muptr, and~, \
        __NFD_OUT_MUADDR_inv_msk, <<(BF_M(NFD_OUT_MUADDR_fld) + 1)]

    #undef __NFD_OUT_MUADDR_inv_msk

    #if (!is_ct_const(in_nbi) || (in_nbi != 0))
        bits_set__sz1(BF_AL(io_nfd_desc, NFD_OUT_NBI_fld), in_nbi)
    #endif

    bits_set__sz1(BF_AL(io_nfd_desc, NFD_OUT_BLS_fld), in_bls)

    /* word2 */
    passert(BF_W(NFD_OUT_DD_fld), "EQ", 2)
    alu[BF_A(io_nfd_desc, NFD_OUT_DD_fld), --, B, 1, <<BF_L(NFD_OUT_DD_fld)]

    #if (!is_ct_const(in_meta_len) || (in_meta_len != 0))
        bits_set__sz1(BF_AL(io_nfd_desc, NFD_OUT_METALEN_fld), in_meta_len)
        alu[data_len, in_pkt_len, +, in_meta_len]
        bits_set__sz1(BF_AL(io_nfd_desc, NFD_OUT_LEN_fld), data_len)
    #else
        bits_set__sz1(BF_AL(io_nfd_desc, NFD_OUT_LEN_fld), in_pkt_len)
    #endif

.end
#endm


#macro nfd_out_fill_desc(io_nfd_desc, in_ctm_isl, in_ctm_pnum, in_bls, \
                         in_muptr, in_pkt_len, in_ctm_split, in_pkt_offset, \
                         in_meta_len)
    nfd_out_fill_desc(io_nfd_desc, in_ctm_isl, in_ctm_pnum, in_bls, \
                      in_muptr, in_pkt_len, 0, in_ctm_split, \
                      in_pkt_offset, in_meta_len)
#endm


#macro nfd_out_set_flags(io_nfd_desc, in_flags)
.begin
    bitfield_insert(BF_A(io_nfd_desc, NFD_OUT_FLAGS_fld),
                    BF_A(io_nfd_desc, NFD_OUT_FLAGS_fld), in_flags,
                    BF_ML(NFD_OUT_FLAGS_fld))
.end
#endm


#macro nfd_out_send(io_xnfd, io_desc, in_pcie, in_qid, LM_CTX, SIG, SIGTYPE)
.begin
    .reg addr_lo
    .reg addr_hi
    .reg total_len

    #if (is_ct_const(in_pcie))
        immed[addr_lo, (nfd_out_ring_info +
                        (in_pcie << log2(NFD_OUT_RING_INFO_ITEM_SZ)))]
    #else
        passert(nfd_out_ring_info,  "MULTIPLE_OF",
                (NFD_MAX_ISL * NFD_OUT_RING_INFO_ITEM_SZ))
        immed[addr_lo, nfd_out_ring_info]
        alu[addr_lo, addr_lo, OR, in_pcie, <<(log2(NFD_OUT_RING_INFO_ITEM_SZ))]
    #endif
    local_csr_wr[ACTIVE_LM_ADDR_/**/LM_CTX, addr_lo]

    #if (!is_ct_const(in_qid) || (in_qid != 0))
        bits_set(BF_A(io_desc, NFD_OUT_QID_fld),
                 BF_L(NFD_OUT_QID_fld), in_qid)
    #endif
    bitfield_extract(total_len, BF_AML(io_desc, NFD_OUT_LEN_fld))

    move(io_xnfd[0], io_desc[0])
    move(io_xnfd[1], io_desc[1])
    move(io_xnfd[2], io_desc[2])
    move(io_xnfd[3], io_desc[3])

    alu[addr_hi, *l$index/**/LM_CTX, AND, 0xFF, <<24]
    ld_field_w_clr[addr_lo, 0011, *l$index/**/LM_CTX]

    #if (streq('SIGTYPE', 'SIG_DONE'))
        mem[qadd_work, io_xnfd[0], addr_hi, <<8, addr_lo, 4], sig_done[SIG]
    #elif (streq('SIGTYPE', 'SIG_WAIT'))
        mem[qadd_work, io_xnfd[0], addr_hi, <<8, addr_lo, 4], ctx_swap[SIG]
    #else
        #error "Unknown signal handling type"
    #endif

    nfd_stats_update_sent(in_pcie, in_qid, total_len)
.end
#endm


#macro nfd_out_send(io_desc, in_pcie, in_qid, LM_CTX)
.begin
    .reg $xnfd[4]
    .xfer_order $xnfd
    .sig nfd_send_sig
    nfd_out_send($xnfd, io_desc, in_pcie, in_qid, LM_CTX, nfd_send_sig,
                 SIG_WAIT)
.end
#endm

#endif /* __NFD_OUT_UC */
