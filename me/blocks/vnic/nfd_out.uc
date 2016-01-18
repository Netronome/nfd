/*
 * Copyright (C) 2014 - 2016 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/nfd_out.uc
 * @brief         Microcode interface to PCI.OUT
 */

#ifndef __NFD_OUT_UC
#define __NFD_OUT_UC

#include <nfp_chipres.h>
#include <stdmac.uc>

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

    .alloc_mem nfd_out_ring_info lm me (NFD_MAX_ISL * 4) (NFD_MAX_ISL * 4)

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
    #if (isnum(in_pcie))
        move(addr_hi, (((in_pcie + NFD_PCIE_ISL_BASE) | 0x80) << 24))
    #else
        alu[addr_hi, in_pcie, +, (NFD_PCIE_ISL_BASE | __NFD_DIRECT_ACCESS)]
        alu[addr_hi, --, B, addr_hi, <<24]
    #endif

    #if (isnum(in_qid))
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


#macro nfd_out_fill_desc(io_nfd_desc, in_ctm_isl, in_ctm_pnum, in_ctm_split, \
                         in_bls, in_muptr, in_offset, in_len, in_meta_len)
.begin
    .reg tmp
    .reg eoff

    /* sanity */
    move(io_nfd_desc[0], 0)
    move(io_nfd_desc[1], 0)
    move(io_nfd_desc[2], 0)
    move(io_nfd_desc[3], 0)

    /* word 0 */
    bits_set(BF_A(io_nfd_desc, NFD_OUT_CTM_ISL_fld),
             BF_L(NFD_OUT_CTM_ISL_fld), in_ctm_isl)
    bits_set(BF_A(io_nfd_desc, NFD_OUT_PKTNUM_fld),
             BF_L(NFD_OUT_PKTNUM_fld), in_ctm_pnum)
    bits_set(BF_A(io_nfd_desc, NFD_OUT_SPLIT_fld),
             BF_L(NFD_OUT_SPLIT_fld), in_ctm_split)
    bits_set(BF_A(io_nfd_desc, NFD_OUT_OFFSET_fld),
             BF_L(NFD_OUT_OFFSET_fld), in_offset)

    /* Not CTM only if it's MU-buffer only */
    alu[--, --, B, in_ctm_isl]
    beq[not_ctm_only#]

    /* check whether the ending offset of the packet */
    /* goes past the end of the CTM buffer.  If so, */
    /* set the CTM_ONLY bit in the packet. */
    alu[eoff, in_len, +, in_offset]
    #if (is_ct_const(in_ctm_split))
        move(tmp, (256 << in_ctm_split))
    #else
        move(tmp, 256)
        alu[--, in_ctm_split, OR, 0]
        alu[tmp, --, B, tmp, <<indirect]
    #endif
    alu[--, eoff, -, tmp]
    bgt[not_ctm_only#]
    bits_set(BF_A(io_nfd_desc, NFD_OUT_CTM_ONLY_fld),
             BF_L(NFD_OUT_CTM_ONLY_fld), 1)

not_ctm_only#:
    /* word 1 */
    /* XXX for now fill in NBI field as 0 */
    bits_set(BF_A(io_nfd_desc, NFD_OUT_BLS_fld),
             BF_L(NFD_OUT_BLS_fld), in_bls)
    bits_set(BF_A(io_nfd_desc, NFD_OUT_MUADDR_fld),
             BF_L(NFD_OUT_MUADDR_fld), in_muptr)

    /* word2 */
    bits_set(BF_A(io_nfd_desc, NFD_OUT_DD_fld),
             BF_L(NFD_OUT_DD_fld), 1)

/*
 * FIXME
 * This no longer means what it used to mean.
 * It now needs to reflect the bits for RSS and input port.
 * instead of a starting amount to back up on the host.
 * Eventually update to properly parse those flag bits.
 */
#if 0
    bits_set(BF_A(io_nfd_desc, NFD_OUT_METALEN_fld),
             BF_L(NFD_OUT_METALEN_fld), in_meta_len)
#endif

    bits_set(BF_A(io_nfd_desc, NFD_OUT_LEN_fld),
             BF_L(NFD_OUT_LEN_fld), in_len)
.end
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

    immed[addr_lo, nfd_out_ring_info]
    alu[addr_lo, addr_lo, OR, in_pcie, <<2]
    local_csr_wr[ACTIVE_LM_ADDR_/**/LM_CTX, addr_lo]

    bits_set(BF_A(io_desc, NFD_OUT_QID_fld),
             BF_L(NFD_OUT_QID_fld), in_qid)
    bitfield_extract(total_len, BF_AML(io_desc, NFD_OUT_LEN_fld))

    alu[addr_hi, *l$index/**/LM_CTX, AND, 0xFF, <<24]
    ld_field_w_clr[addr_lo, 0011, *l$index/**/LM_CTX]

    move(io_xnfd[0], io_desc[0])
    move(io_xnfd[1], io_desc[1])
    move(io_xnfd[2], io_desc[2])
    move(io_xnfd[3], io_desc[3])

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
