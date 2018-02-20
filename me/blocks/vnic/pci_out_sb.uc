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
 * @file          blocks/vnic/pci_out_sb.uc
 * @brief         Implement "stage batch" (SB) task for PCI.OUT
 */

#ifndef __PCI_OUT_SB_UC
#define __PCI_OUT_SB_UC

#include <stdmac.uc>
#include <aggregate.uc>

#include "wsm.uc"
#include "nfd_common.h"
#include "shared/nfd_internal.h"
#include "shared/nfd_cfg.uc"
#include "nfd_out.uc"   /* for definitions only */
#include "pci_out_sb.h"
#include "pci_out_sb_iface.uc"

#include "nfd_user_cfg.h"


/**
 * For reference
 *
 * Input descriptors (from clients):
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
 *
 *
 * Output descriptor (to Issue DMA MEs):
 *
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+---------------+---------+-------------------+---------------+
 *    0  |E| Requester ID  | Unused  |   Sequence Num    | HostBuf[39:32]|
 *       +-+---------------+---------+-------------------+---------------+
 *    1  |                       Host Buffer [31:0]                      |
 *       +---------------------------------------------------------------+
 *    2  |  CTM ISL  |C|  Packet Number  |SPL|0|     Starting Offset     |
 *       +-+---+-----+-+-----------------+---+-+-------------------------+
 *    3  |N|BLS|           MU Buffer Address [39:11]                     |
 *       +-+---+---------+---------------+-------------------------------+
 *    4  |D| Meta Length |  RX Queue     |           Data Length         |
 *       +-+-------------+---------------+-------------------------------+
 */

#define NFD_OUT_FL_DESC_SIZE            8
#define NFD_OUT_FL_DESC_SIZE_lg2        (log2(NFD_OUT_FL_DESC_SIZE))

#ifndef STAGE_BATCH_MANAGER_CTX
#define STAGE_BATCH_MANAGER_CTX 0
#endif

#ifndef STAGE_BATCH_FIRST_WORKER
#define STAGE_BATCH_FIRST_WORKER 1
#endif

#ifndef STAGE_BATCH_NUM_WORKERS
#define STAGE_BATCH_NUM_WORKERS 7
#endif

#define STAGE_BATCH_LAST_WORKER (STAGE_BATCH_FIRST_WORKER + STAGE_BATCH_NUM_WORKERS - 1)


#define NUM_IO_BLOCKS           5
//#define PCI_OUT_SB_WQ_CREDIT_SIG_NUM       13
#define PCI_OUT_SB_CFG_SIG_NUM  14
#define ORDER_SIG_NUM           15

#define EMEM_ADDR(_x) (streq('_x','emem0') ? __ADDR_EMEM0 : \
                       (streq('_x','emem1') ? __ADDR_EMEM1 : \
                       (streq('_x','emem2') ? __ADDR_EMEM2 : NOEMEM)))

#define NFD_OUT_FL_CACHE_SIZE_PER_QUEUE \
    (NFD_OUT_FL_BUFS_PER_QUEUE * NFD_OUT_FL_DESC_SIZE)
#define NFD_OUT_FL_CACHE_SIZE_PER_QUEUE_lg2 \
    (log2(NFD_OUT_FL_CACHE_SIZE_PER_QUEUE))


/**
 * LM state:
 *
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-----------------------------------------------+---------------+
 *    0  |                 Sequence Number               |     Zero      |
 *       +---------------------------------------------+-+---------------+
 *    1  |                 Unused                      |E| Requester ID  |
 *       +---------------------------------------------+-+---------------+
 *    2  |            RX desc cache address right shifted 8              |
 *       +---------------------------------------------------------------+
 *    3  |                            Unused                             |
 *       +---------------------------------------------------------------+
 *
 * The sequence number is shifted over 8 to enabled optimized use of the
 * alu[.., +8, ..] instruction to add in the host buffer's high 8 bits.
 */

#define LM_QSTATE_SEQ_bf        0, 31, 0
#define LM_QSTATE_SEQ_wrd       0
#define LM_QSTATE_SEQ_shf       0
#define LM_QSTATE_SEQ_msk       0xFFFFFFFF
#define LM_QSTATE_ENABLED_bf    1, 8, 8
#define LM_QSTATE_ENABLED_wrd   1
#define LM_QSTATE_ENABLED_shf   8
#define LM_QSTATE_ENABLED_msk   0x1
#define LM_QSTATE_ENABLED_bit   8
#define LM_QSTATE_RID_bf        1, 7, 0
#define LM_QSTATE_RID_wrd       1
#define LM_QSTATE_RID_shf       0
#define LM_QSTATE_RID_msk       0xFF
#define LM_QSTATE_CACHE_ADDR_RS8_bf  2, 31, 0
#define LM_QSTATE_CACHE_ADDR_RS8_wrd 2
#define LM_QSTATE_CACHE_ADDR_RS8_shf 0
#define LM_QSTATE_CACHE_ADDR_RS8_msk 0xFFFFFFFF

#define LM_QSTATE_SIZE          16
#define LM_QSTATE_SIZE_LW       (LM_QSTATE_SIZE / 4)
#define LM_QSTATE_SIZE_lg2      (log2(LM_QSTATE_SIZE))
#define LM_QSTATE_CSR           ACTIVE_LM_ADDR_0
#define LM_QSTATE_PTR           *l$index0
#define LM_QSTATE               LM_QSTATE_PTR
#define LM_SEQ_wrd              0
#define LM_STATUS_wrd           1
#define LM_CACHE_ADDR_RS8_wrd   2
#define LM_SEQ                  LM_QSTATE_PTR[LM_SEQ_wrd]
#define LM_STATUS               LM_QSTATE_PTR[LM_STATUS_wrd]
#define LM_CACHE_ADDR_RS8       LM_QSTATE_PTR[LM_CACHE_ADDR_RS8_wrd]

#define LM_WQ_CREDIT_CSR        ACTIVE_LM_ADDR_1
#define LM_WQ_CREDIT_PTR        *l$index1
#define LM_WQ_CREDITS           LM_WQ_CREDIT_PTR

#define LM_DEBUG_CSR            ACTIVE_LM_ADDR_2
#define LM_DEBUG_PTR            *l$index2


// LMEM data structures
.alloc_mem sb_ctx_base lmem+0 me (LM_QSTATE_SIZE * NFD_OUT_MAX_QUEUES)
.alloc_mem sb_wq_credits lmem me 4
.alloc_mem sb_debug_snapshot lmem me (LM_QSTATE_SIZE * NFD_OUT_MAX_QUEUES)

#define_eval __LOOP 0
#while (__LOOP < (LM_QSTATE_SIZE * NFD_OUT_MAX_QUEUES))
    .init sb_ctx_base+__LOOP 0
    #define_eval __LOOP (__LOOP + 4)
#endloop


// Input ring
#define_eval __EMEM 'NFD_PCIE/**/PCIE_ISL/**/_EMEM'
.alloc_resource nfd_out_ring_num/**/PCIE_ISL/**/0 __EMEM/**/_queues global 1 1
.alloc_mem nfd_out_ring_mem/**/PCIE_ISL __EMEM global NFD_OUT_RING_SZ NFD_OUT_RING_SZ
.init_mu_ring nfd_out_ring_num/**/PCIE_ISL/**/0 nfd_out_ring_mem/**/PCIE_ISL

// Cache memory
#define_eval NFD_OUT_CACHE_SIZE (NFD_OUT_FL_CACHE_SIZE_PER_QUEUE * NFD_OUT_MAX_QUEUES)
#define_eval NFD_OUT_CACHE_LOC 'NFD_PCIE/**/PCIE_ISL/**/_FL_CACHE_MEM'
.alloc_mem fl_cache_mem/**/PCIE_ISL NFD_OUT_CACHE_LOC global NFD_OUT_CACHE_SIZE NFD_OUT_CACHE_SIZE

// Config rings
.alloc_resource nfd_cfg_ring_nums NFD_CFG_RING_EMEM/**/_queues global 32
.declare_resource nfd_cfg_ring_nums/**/PCIE_ISL global 8 nfd_cfg_ring_nums
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/0 nfd_cfg_ring_nums/**/PCIE_ISL global 1
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/1 nfd_cfg_ring_nums/**/PCIE_ISL global 1
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/2 nfd_cfg_ring_nums/**/PCIE_ISL global 1
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/3 nfd_cfg_ring_nums/**/PCIE_ISL global 1
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/4 nfd_cfg_ring_nums/**/PCIE_ISL global 1
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/5 nfd_cfg_ring_nums/**/PCIE_ISL global 1


// Debug state
.alloc_mem nfd_out_sb_debug_state/**/PCIE_ISL emem0 global \
    (LM_QSTATE_SIZE * NFD_OUT_MAX_QUEUES + 16)


#macro _reset_ticket_bitmap(in_qid)
.begin

    .reg addr_hi
    .reg addr_lo

    .reg write $bitmap[4]
    .xfer_order $bitmap

    .sig bitmap_sig

    move(addr_hi, (nfd_out_sb_release/**/PCIE_ISL >> 8))
    alu[addr_lo, --, B, in_qid, <<4]    ; addr_lo = qid * sizeof(bitmap)
    aggregate_zero($bitmap, 4)
    mem[write32, $bitmap[0], addr_hi, <<8, addr_lo, 4], sig_done[bitmap_sig]

    // XXX wait without sleeping to make this absolutely atomic.  Overkill?
    .while (!SIGNAL(bitmap_sig))
    .endw

.end
#endm


#macro _set_queue_state(in_vid, in_q, in_up, in_rid)
.begin

    .reg qid
    .reg lma
    .reg changed
    .reg currently_up
    .reg base_addr
    .reg tmp

    nfd_vid2qid(qid, in_vid, in_q)

    // Load the queue state for that queue
    alu[lma, --, B, qid, <<LM_QSTATE_SIZE_lg2]
    local_csr_wr[LM_QSTATE_CSR, lma]
    nop
    nop
    nop

    // If the queue state changed, then update the LM entry
    wsm_extract(currently_up, LM_QSTATE, LM_QSTATE_ENABLED)
    .if (in_up != currently_up)

        .if (in_up != 0)

            wsm_set(LM_QSTATE, LM_QSTATE_RID, in_rid)
            wsm_set(LM_QSTATE, LM_QSTATE_ENABLED, 1)
            move(LM_SEQ, 0)
            // Precomputation to save cycles later
            move(base_addr, (fl_cache_mem/**/PCIE_ISL >> 8))
            alu[LM_CACHE_ADDR_RS8, base_addr, OR, qid, <<(NFD_OUT_FL_CACHE_SIZE_PER_QUEUE_lg2 - 8)]
            _reset_ticket_bitmap(qid)

        .else

            wsm_clear(LM_QSTATE, LM_QSTATE_RID)
            wsm_clear(LM_QSTATE, LM_QSTATE_ENABLED)

        .endif


    .endif

.end
#endm


#macro _check_vnic_state(in_vid)
.begin

    .reg bar_addr_hi
    .reg bar_addr_lo
    .reg q
    .reg up
    .reg maxqs
    .reg rid

    .reg read $bar[6]
    .xfer_order $bar

    .sig read_sig

    nfd_cfg_get_bar_addr(bar_addr_hi, bar_addr_lo, in_vid, PCIE_ISL)
    mem[read32, $bar[0], bar_addr_hi, <<8, bar_addr_lo, 6], ctx_swap[read_sig]

    /* Setup maxqs */
    nfd_vid_maxqs(maxqs, in_vid)

    /* Setup RIDS, PFs and the CTRL vNICs are on 0*/
    /* XXX nfd_vid_maxqs() supports branch optional parameters
     * that should avoid the need for this second test, but
     * using them in this context currently results in spurious
     * warnings */
    #if (NFD_MAX_PFS > 0 || defined(NFD_USE_CTRL))
        immed[rid, 0]
    #endif
    #if (NFD_MAX_VFS > 0)
    .if (NFD_VID_IS_VF(in_vid))
        alu[rid, in_vid, +, NFD_CFG_VF_OFFSET]
    .endif
    #endif

    .if ($bar[NFP_NET_CFG_CTRL] & NFP_NET_CFG_CTRL_ENABLE == 0)

        move(up, 0)
        move(q, 0)
        .while (q < maxqs)

            _set_queue_state(in_vid, q, up, rid)
            alu[q, q, +, 1]

        .endw

    .else

        move(q, 0)
        .while (q < maxqs)

            .if (q < 32)

                alu[--, q, OR, 0]
                alu[up, 1, AND, $bar[(NFP_NET_CFG_RXRS_ENABLE/4)], >>indirect]

            .else

                alu[--, q, OR, 0]       ; only 5 least significant bits are used
                alu[up, 1, AND, $bar[(NFP_NET_CFG_RXRS_ENABLE/4 + 1)], >>indirect]

            .endif

            _set_queue_state(in_vid, q, up, rid)
            alu[q, q, +, 1]

        .endw

    .endif

.end
#endm


#define NFD_CFG_NEXT_CTX 0
#macro signal_next_cfg_me()
.begin

    .reg meid
    .reg addr
    .reg tmp

    .sig remote NFD_CFG_SIG_NEXT_ME

    move(meid, (NFD_CFG_NEXT_ME))
    move(tmp, 0x3F0)
    alu[tmp, tmp, AND, meid]
    alu[addr, --, B, tmp, <<20]
    alu[tmp, 0xF, AND, meid]
    alu[addr, addr, OR, tmp, <<9]
    alu[addr, addr, OR, NFD_CFG_NEXT_CTX, <<6]
    move(tmp, (&remote(NFD_CFG_SIG_NEXT_ME, NFD_CFG_NEXT_ME)))
    alu[addr, addr, OR, tmp, <<2]

    ct[interthread_signal, --, 0, addr]

.end
#endm


#macro signal_ctx(in_ctx, in_signum)
.begin


    #if (isnum(in_ctx) && isnum(in_signum))
        local_csr_wr[SAME_ME_SIGNAL, (in_ctx | (in_signum <<3))]
    #else
        .reg sigval
        alu[sigval, in_ctx, OR, in_signum, <<3]
        local_csr_wr[SAME_ME_SIGNAL, sigval]
    #endif

.end
#endm


/**
 * Process reconfiguation messages.
 */
#macro process_reconfig()
.begin

    .reg ring_addr_hi
    .reg ring_in_addr_lo
    .reg ring_out_addr_lo
    .reg vid

    .reg $cmsg[1]
    .xfer_order $cmsg

    .sig get_sig
    .sig put_sig

    move(ring_addr_hi, (EMEM_ADDR(NFD_CFG_RING_EMEM) >> 8))
    move(ring_in_addr_lo, nfd_cfg_ring_num/**/PCIE_ISL/**/3)
    move(ring_out_addr_lo, nfd_cfg_ring_num/**/PCIE_ISL/**/4)

    .while (1)

        .io_completed get_sig
        mem[get, $cmsg[0], ring_addr_hi, <<8, ring_in_addr_lo, 1], sig_done[get_sig]
        ctx_arb[get_sig[0]]

        .if (SIGNAL(get_sig[1]))
        .endif

        .if (BIT($cmsg[0], NFD_CFG_MSG_VALID_bit) == 0)
            .break
        .endif

        // Extract the queue ID that is going up or down
        wsm_extract(vid, $cmsg, NFD_CFG_MSG_VID)

        _check_vnic_state(vid)

        // Propagate the message down the line
        move($cmsg[0], $cmsg[0])
        mem[journal, $cmsg[0], ring_addr_hi, <<8, ring_out_addr_lo, 1], ctx_swap[put_sig]
        signal_next_cfg_me()

    .endw

.end
#endm


/**
 * Update output work queue credits
 *
 * Should only fire every 64 packets or so.  This makes the
 * cost of this operation about 9 + loop overhead cycles per
 * refresh.  Assume 9 cycles of loop overhead.  Then, credit
 * update costs 18 / 64 = ~0.28 cycles per packet.
 */
#macro update_wq_credits()
.begin

    .reg $credits
    .reg addr_lo
    .sig iosig

    move(addr_lo, nfd_out_sb_wq_credits/**/PCIE_ISL)
    move($credits, 0xFFFFFFFF)
    mem[test_and_clr, $credits, 0, addr_lo, 1], sig_done[iosig]
    ctx_arb[iosig]
    alu[LM_WQ_CREDITS, LM_WQ_CREDITS, +, $credits]

.end
#endm


#macro set_alarm(in_sig, in_nticks)
.begin

    .reg count
    .reg amt

    move(amt, in_nticks)
    local_csr_rd[TIMESTAMP_LOW]
    immed[count, 0]
    alu[count, count, +, amt]
    immed_w1[count, 0]
    local_csr_wr[ACTIVE_FUTURE_COUNT_SIGNAL, (&in_sig)]
    local_csr_wr[ACTIVE_CTX_FUTURE_COUNT, count]
    .set_sig in_sig

.end
#endm


#macro dump_state(io_version)
.begin
    .reg addr_hi
    .reg addr_lo
    .reg lma
    .reg i

    .reg $out[(LM_QSTATE_SIZE_LW * 4)]
    .xfer_order $out

    .sig out_sig

    move(lma, sb_ctx_base)
    local_csr_wr[LM_QSTATE_CSR, lma]

    move(lma, sb_debug_snapshot)
    local_csr_wr[LM_DEBUG_CSR, lma]

    move(i, (LM_QSTATE_SIZE_LW * NFD_OUT_MAX_QUEUES))
    move(addr_hi, ((nfd_out_sb_debug_state/**/PCIE_ISL >> 8) & 0xFF000000))
    move(addr_lo, (nfd_out_sb_debug_state/**/PCIE_ISL & 0xFFFFFFFF))

    /* copy state out so it is an atomic snapshot */
copy_loop#:
    move(LM_DEBUG_PTR++, LM_QSTATE_PTR++)
    alu[i, i, -, 4]
    bgt[copy_loop#], defer[3]
    move(LM_DEBUG_PTR++, LM_QSTATE_PTR++)
    move(LM_DEBUG_PTR++, LM_QSTATE_PTR++)
    move(LM_DEBUG_PTR++, LM_QSTATE_PTR++)

    /* dump out the header */
    move($out[0], io_version)
    move($out[1], LM_WQ_CREDITS)
    move($out[2], 0)
    move($out[3], 0)
    mem[write, $out[0], addr_hi, <<8, addr_lo, 2], ctx_swap[out_sig]

    move(lma, sb_debug_snapshot)
    local_csr_wr[LM_DEBUG_CSR, lma]
    alu[addr_lo, addr_lo, +, 16]

    move(i, NFD_OUT_MAX_QUEUES)
    .while (i > 0)

        /* queue 0 */
        move($out[0], LM_DEBUG_PTR++)
        move($out[1], LM_DEBUG_PTR++)
        move($out[2], LM_DEBUG_PTR++)
        move($out[3], LM_DEBUG_PTR++)

        /* queue 1 */
        move($out[4], LM_DEBUG_PTR++)
        move($out[5], LM_DEBUG_PTR++)
        move($out[6], LM_DEBUG_PTR++)
        move($out[7], LM_DEBUG_PTR++)

        /* queue 2 */
        move($out[8], LM_DEBUG_PTR++)
        move($out[9], LM_DEBUG_PTR++)
        move($out[10], LM_DEBUG_PTR++)
        move($out[11], LM_DEBUG_PTR++)

        /* queue 3 */
        move($out[12], LM_DEBUG_PTR++)
        move($out[13], LM_DEBUG_PTR++)
        move($out[14], LM_DEBUG_PTR++)
        move($out[15], LM_DEBUG_PTR++)

        mem[write, $out[0], addr_hi, <<8, addr_lo, 8], ctx_swap[out_sig]

        alu[addr_lo, addr_lo, +, (16 * 4)]
        alu[i, i, -, 4]

    .endw

    alu[io_version, io_version, +, 1]
.end
#endm


/**
 * Main processing loop for the manager context.  This context is responsible
 * for updating work queue credits and for processing configuration messages.
 */
#macro manager_main_loop()
.begin

    .reg lma

    .reg state_version

    .sig volatile visible _nfd_credit_sig_sb
    .addr _nfd_credit_sig_sb PCI_OUT_SB_WQ_CREDIT_SIG_NUM

    .sig volatile visible _nfd_cfg_sig_sb
    .addr _nfd_cfg_sig_sb PCI_OUT_SB_CFG_SIG_NUM

    .sig volatile state_alarm_sig


    // Shared ME initialization
    move(state_version, 0)
    move(lma, sb_wq_credits)
    local_csr_wr[LM_WQ_CREDIT_CSR, lma]
    nop
    nop
    nop
    move(LM_WQ_CREDITS, (NFD_OUT_SB_WQ_SIZE_LW / SB_WQ_SIZE_LW))

    // Shared ME initialization finished: kickstart workers
    signal_ctx(STAGE_BATCH_FIRST_WORKER, ORDER_SIG_NUM)

    set_alarm(state_alarm_sig, 16384)

    .while (1)

        .set_sig _nfd_credit_sig_sb
        .set_sig _nfd_cfg_sig_sb

        ctx_arb[_nfd_credit_sig_sb, _nfd_cfg_sig_sb, state_alarm_sig], ANY

        .if (SIGNAL(_nfd_credit_sig_sb))

            update_wq_credits()
            .continue

        .endif

        .if (SIGNAL(_nfd_cfg_sig_sb))

            alu[@nconfigs, @nconfigs, +, 1]
            local_csr_wr[MAILBOX0, @nconfigs]

            process_reconfig()
            .continue

        .endif

        .if (SIGNAL(state_alarm_sig))

            dump_state(state_version)
            set_alarm(state_alarm_sig, 16384)
            .continue

        .endif

    .endw

.end
#endm



/**
 * Main processing function to handle one request from the NFD ingress
 * work queue.
 *
 * @param in_xfer       4 read transfer registers to use.  contain the
 *                      next request on macro invocation.
 * @param out_xfer      6 write xfer registers to use... all I/O on
 *                      them should be completed before entering the macro
 * @param cur_insig     input signal to use for current transfers
 * @param cur_outsig    output signal to use for current transfers
 * @param nxt_insig     input signal to wait on before leaving the macro
 * @param nxt_outsig    output signal to wait on before leaving the macro
 * @param ordersig      ordering signal...  The macro must issue this
 *                      once all operations that must be performed in order
 *                      are complete.  It must wait on this signal before
 *                      leaving the macro.
 * @param DONE_LABEL    Label to branch to when the macro is complete.
 *
 * This macro also depends on certain global pre-conditions:
 *  - it is assumed that LM pointer LM_QSTATE_CSR is free for use
 *  - it is assumed that LM pointer LM_WQ_CREDIT_CSR points to a shared
 *    credit pool for the transmission work queue
 *  - g_sig_next_worker must be a GPR that contains the information
 *    required to send the ordering signal to the next worker in sequence
 *  - the queue state table is located in local memory at address 0
 *  - g_lm_qstate_mask must be set to a mask that can be applied to
 *    the queue ID shifted NFD_OUT_QID_shf - LM_QSTATE_SIZE_lg2
 *    bits to the right to obtain the LM offset of the per-queue state.
 *  - g_cache_addr_lo_mask will mask down a shifted sequence number into an
 *    offset in the FL desc cache/RX desc cache
 *  - g_in_wq_lo and g_in_wq_hi contain the 40-bit address required to
 *    request the next work item.
 *  - g_out_wq_lo and g_out_wq_hi contains the address for sending to the
 *    output work queue.
 *
 * This function should consume exactly 20 cycles of processing per
 * invocation with zero cycles lost to defer slots as long as there are
 * work queue credits available to send (and if we are backed up due
 * to lack of credits, we have spare cycles to lose).  If one chains
 * 1+ of these together so that each invocation goes directly to the
 * next when done, then the worker threads will cosume exactly that many
 * cycles per packet plus some small delta for updating credits.
 */
#macro process_request(in_xfer, out_xfer, cur_insig, cur_outsig, \
                       nxt_insig, nxt_outsig, ordersig, DONE_LABEL)
.begin

    .reg lma
    .reg credits
    .reg addr_lo
    .reg out_word0

    .reg read $buf_desc[2]
    .xfer_order $buf_desc

    .sig fl_read_sig

    // Loop here if no WQ credits
test_ready_to_send#:
    alu[LM_WQ_CREDITS, LM_WQ_CREDITS, -, 1]
    blt[flow_controlled#]

    // Burn per-CTX GPR to make 1 cycle
    local_csr_wr[SAME_ME_SIGNAL, g_sig_next_worker]

    // XXX Assumes that the queue state starts at address 0
    alu[lma, g_lm_qstate_mask, AND, in_xfer[NFD_OUT_QID_wrd],
        >>(NFD_OUT_QID_shf - LM_QSTATE_SIZE_lg2)]
    local_csr_wr[LM_QSTATE_CSR, lma]

    // Copy descriptor. Put at the end so last word can be ommitted in WQ
    move(out_xfer[2], in_xfer[0])       // lm addr cycle 0
    move(out_xfer[3], in_xfer[1])       // lm addr cycle 1
    move(out_xfer[4], in_xfer[2])       // lm addr cycle 2
    move(out_xfer[5], in_xfer[3])

    /*
     * Swap freelist descriptor for RX descriptor.
     * - get next addr_lo from the (shifted) sequence number
     * - issue read then write
     */
    #if (SB_WQ_SEQ_shf < NFD_OUT_FL_DESC_SIZE_lg2)
        #error "SB_WQ_SEQ_shf < NFD_OUT_FL_DESC_SIZE_lg2: cache address computation incorrect"
    #endif
    alu[out_word0, g_seq_mask, AND, LM_SEQ]
    alu[addr_lo, g_cache_addr_lo_mask, AND, LM_SEQ, >>(SB_WQ_SEQ_shf - NFD_OUT_FL_DESC_SIZE_lg2)]
    mem[read, $buf_desc[0], LM_CACHE_ADDR_RS8, <<8, addr_lo, 1], sig_done[fl_read_sig]
    mem[write, out_xfer[4], LM_CACHE_ADDR_RS8, <<8, addr_lo, 1], sig_done[cur_outsig]

    // Start next work queue dequeue
    mem[qadd_thread, in_xfer[0], g_in_wq_hi, <<8, g_in_wq_lo, 4], sig_done[cur_insig]

    /*
     * Wait for read/write operations to complete
     * In the defer shadow:
     * - Increment the sequence number
     * - OR in the RID and the enabled bit
     */
    ctx_arb[fl_read_sig, cur_outsig], defer[2]
    alu[LM_SEQ, LM_SEQ, +, g_seq_incr]
    alu[out_word0, out_word0, OR, LM_STATUS, <<SB_WQ_RID_shf]

    // Start sending the work to issue DMA, but see below: we're not quite done
    pci_out_sb_add_work(out_xfer[0], cur_outsig)

    /*
     * Wait for least recent I/Os to complete.
     *
     * XXX in the defer shadow of this, copy freelist descriptor to xfer regs.
     * Not sure whether this is a real race because I don't think that
     * cls[qadd_work] goes to the wire until 3 cycles after it's issued.  But
     * even if it were a race, we would always win due to the CPP bus clocks
     * required before pulling the data from $buf_desc.
     */
    #if (SB_WQ_HOST_ADDR_HI_msk != 0xFF)
        #error "SB_WQ_HOST_ADDR_HI_msk != 0xFF: PD word0 construction will fail"
    #endif
    .set_sig ordersig
    #pragma warning(disable:5009)
    ctx_arb[nxt_insig, nxt_outsig, ordersig], defer[2], br[DONE_LABEL]
    alu[out_xfer[0], out_word0, +8, $buf_desc[0]]
    move(out_xfer[1], $buf_desc[1])
    #pragma warning(default:5009)

    // No credits, yield and then branch back to the test
flow_controlled#:
    ctx_arb[voluntary], defer[2], br[test_ready_to_send#]
    alu[LM_WQ_CREDITS, LM_WQ_CREDITS, +, 1]
    nop

.end
#endm


#macro worker_main_loop()
.begin

    /*
     * DECLARATIONS
     */

    #define_eval __BLOCK 0
    #while (__BLOCK < NUM_IO_BLOCKS)

        .reg volatile read $in_/**/__BLOCK/**/[4]
        .xfer_order $in_/**/__BLOCK
        .reg volatile write $out_/**/__BLOCK/**/[6]
        .xfer_order $out_/**/__BLOCK
        .sig volatile insig_/**/__BLOCK
        .sig volatile outsig_/**/__BLOCK

        #define_eval __BLOCK (__BLOCK + 1)
    #endloop

    // global per-context registers
    .reg volatile g_sig_next_worker
    .reg volatile g_lm_qstate_mask
    .reg volatile g_cache_addr_lo_mask
    .reg volatile g_seq_mask
    .reg volatile g_seq_incr
    .reg volatile g_in_wq_hi
    .reg volatile g_in_wq_lo
    .reg volatile g_out_wq_hi
    .reg volatile g_out_wq_lo

    // "regular" registers
    .reg ctx
    .reg next_ctx
    .reg lma

    // We must receive this signal to start pulling from the work queue
    .sig volatile ordersig
    .addr ordersig ORDER_SIG_NUM


    /*
     * PER CONTEXT INITIALIZATION
     */

    pci_out_sb_iface_init()

    // Get current context
    local_csr_rd[ACTIVE_CTX_STS]
    immed[ctx, 0]
    alu[ctx, ctx, AND, 7]

    // Any extra threads should die here
    .if (ctx < STAGE_BATCH_FIRST_WORKER || ctx > STAGE_BATCH_LAST_WORKER)
        ctx_arb[kill]
    .endif

    // Initialize g_sig_next_worker
    alu[next_ctx, ctx, +, 1]
    .if (next_ctx > STAGE_BATCH_LAST_WORKER)
        move(next_ctx, STAGE_BATCH_FIRST_WORKER)
    .endif
    alu[g_sig_next_worker, next_ctx, OR, (&ordersig), <<3]

    move(g_lm_qstate_mask, ((NFD_OUT_MAX_QUEUES - 1) << LM_QSTATE_SIZE_lg2))
    move(g_cache_addr_lo_mask, ((NFD_OUT_FL_BUFS_PER_QUEUE - 1) << NFD_OUT_FL_DESC_SIZE_lg2))

    /* Mask for the sequence number */
    move(g_seq_mask, (SB_WQ_SEQ_msk << (SB_WQ_SEQ_shf)))

    /* Shifted sequence number addent */
    move(g_seq_incr, (1 << (SB_WQ_SEQ_shf)))

    move(g_in_wq_hi, ((nfd_out_ring_mem/**/PCIE_ISL >> 8) & 0xFF000000))
    move(g_in_wq_lo, nfd_out_ring_num/**/PCIE_ISL/**/0)
    move(lma, sb_wq_credits)
    local_csr_wr[LM_WQ_CREDIT_CSR, lma]


    /*
     * ORDERED ADD TO THE WORK QUEUE
     */
    #define_eval __BLOCK 0
    #while (__BLOCK < NUM_IO_BLOCKS)

        .set_sig ordersig
        ctx_arb[ordersig]
        mem[qadd_thread, $in_/**/__BLOCK[0], g_in_wq_hi, <<8, g_in_wq_lo, 4], sig_done[insig_/**/__BLOCK]
        signal_ctx(ctx, (&outsig_/**/__BLOCK))
        local_csr_wr[SAME_ME_SIGNAL, g_sig_next_worker]

        #define_eval __BLOCK (__BLOCK + 1)
    #endloop


    /*
     * MAIN WORKER LOOP
     */

    // Wait for the signal for the first block
    .set_sig ordersig
    .set_sig outsig_0
    ctx_arb[insig_0, outsig_0, ordersig]

worker_loop#:

    #define_eval __BLOCK 0
    #while (__BLOCK < NUM_IO_BLOCKS)

        #if (__BLOCK != NUM_IO_BLOCKS - 1)
            #define_eval __NEXT_BLOCK (__BLOCK + 1)
        #else
            #define_eval __NEXT_BLOCK 0
        #endif

        process_request_/**/__BLOCK/**/#:
            process_request($in_/**/__BLOCK,
                            $out_/**/__BLOCK,
                            insig_/**/__BLOCK,
                            outsig_/**/__BLOCK,
                            insig_/**/__NEXT_BLOCK,
                            outsig_/**/__NEXT_BLOCK,
                            ordersig,
                            process_request_/**/__NEXT_BLOCK/**/#)

        #define_eval __BLOCK (__BLOCK + 1)
    #endloop

    #undef __BLOCK
    #undef __NEXT_BLOCK

.end
#endm


#macro die_if_debug()

    #if 0 /* XXX REMOVE ME when really integrated */

        // Fake out the assembler
        alu[--, --, B, 0]
        beq[die_now#]
        bne[die_now#]
        br[keep_going#]

    die_now#:
        ctx_arb[kill]

    keep_going#:
        // not reached
    #endif

#endm



main#:
    .reg @nconfigs
    .init @nconfigs 0

    pci_out_sb_iface_declare()

    .if (ctx() == STAGE_BATCH_MANAGER_CTX)

        manager_main_loop()

    .else

        die_if_debug()

        worker_main_loop()

    .endif
    nop

#endif /* __PCI_OUT_SB_UC */
