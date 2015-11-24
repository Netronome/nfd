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

#ifndef NFD_IN_BLM_BLS
#error "NFD_IN_BLM_BLS must be defined by the user"
#endif

#ifndef NFD_IN_BLM_POOL
#error "NFD_IN_BLM_POOL must be defined by the user"
#endif

#ifndef NFD_IN_BLM_BUF_SZ
#error "NFD_IN_BLM_BUF_SZ must be defined by the user"
#endif

#ifndef NFD_IN_BLM_JUMBO_BLS
#error "NFD_IN_BLM_JUMBO_BLS must be defined by the user"
#endif

#ifndef NFD_IN_BLM_JUMBO_POOL
#error "NFD_IN_BLM_JUMBO_POOL must be defined by the user"
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
 *       +-+-------------+-------------------------------+---+-----------+
 *    1  |                           buf_addr                            |
 *       +---------------+---------------+-------------------------------+
 *    2  |     flags     |   l4_offset   |               lso             |
 *       +---------------+---------------+-------------------------------+
 *    3  |            data_len           |              vlan             |
 *       +-------------------------------+-------------------------------+
 *
 *      S -> sp0 (spare)
 *    itf -> intf
 */
#define NFD_IN_OFFSET_fld       0, 30, 24
#define NFD_IN_SEQN_fld         0, 23, 8
#define NFD_IN_INTF_fld         0, 7, 6
#define NFD_IN_QID_fld          0, 5, 0
#define NFD_IN_BUFADDR_fld      1, 31, 0
#define NFD_IN_FLAGS_fld        2, 31, 24
#define NFD_IN_L4OFF_fld        2, 23, 16
#define NFD_IN_LSO_fld          2, 15, 0
#define NFD_IN_SEQ_fld          2, 15, 0
#define NFD_IN_DATALEN_fld      3, 31, 16
#define NFD_IN_VLAN_fld         3, 15, 0
#define NFD_IN_META_SIZE        16
#define NFD_IN_META_SIZE_LW     4


#define NFD_IN_RING_HI_fld      0, 31, 24
#define NFD_IN_RING_SP_fld      0, 23, 12
#define NFD_IN_RING_LO_fld      0, 11, 0



#macro nfd_in_recv_init()

    .alloc_mem nfd_in_ring_info lm me (NFD_MAX_ISL * 4) (NFD_MAX_ISL * 4)

    nfd_stats_declare_in()

#ifdef NFD_IN_WQ_SHARED

    #define __EMEM_NUM
    #define_eval __EMEM_NUM strright('NFD_IN_WQ_SHARED', 1)

    #if defined(NFD_PCIE0_EMEM) || defined(NFD_PCIE0_EMEM) || defined(NFD_PCIE0_EMEM) || defined(NFD_PCIE0_EMEM)

        .alloc_resource nfd_in_ring_nums0 NFD_IN_WQ_SHARED/**/_queues global NFD_IN_NUM_WQS NFD_IN_NUM_WQS
        .declare_resource nfd_in_ring_nums_res0 global NFD_IN_NUM_WQS nfd_in_ring_nums0
        .alloc_resource nfd_in_ring_num00 nfd_in_ring_nums_res0 global 1

    #endif /* defined(NFD_PCIE0_EMEM) || defined(NFD_PCIE0_EMEM) || defined(NFD_PCIE0_EMEM) || defined(NFD_PCIE0_EMEM) */

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

    immed[addr_lo, nfd_in_ring_info]
    alu[addr_lo, addr_lo, OR, in_pcie_isl, <<2]
    local_csr_wr[ACTIVE_LM_ADDR_/**/LM_CTX, addr_lo]
    nop
    nop
    nop
    ld_field_w_clr[addr_hi, 1000, *l$index/**/LM_CTX]
    ld_field_w_clr[addr_lo, 0011, *l$index/**/LM_CTX]
    alu[addr_lo, addr_lo, +, in_recvq]

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
    .reg split_thresh

    bitfield_extract(len, BF_AML(in_nfd_meta, NFD_IN_DATALEN_fld))

#if (NFD_IN_BLM_BLS == NFD_IN_BLM_JUMBO_BLS)
    bitfield_insert(v, 0, NFD_IN_BLM_BLS, BF_ML(PKT_META_BUFLIST_bf))
#else
    move(split_thresh, (NFD_IN_BLM_BUF_SZ - NFD_IN_DATA_OFFSET))
    .if (len > split_thresh)
        bitfield_insert(v, 0, NFD_IN_BLM_JUMBO_BLS, BF_ML(PKT_META_BUFLIST_bf))
    .else
        bitfield_insert(v, 0, NFD_IN_BLM_BLS, BF_ML(PKT_META_BUFLIST_bf))
    .endif
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
