#ifndef __PCI_OUT_SB_UC
#define __PCI_OUT_SB_UC

#include <stdmac.uc>
#include <bitfields.uc>
#include <aggregate.uc>

#include <nfd_common.h>
#include <nfd_cfg_pf_bars.uc>
#include <nfd_out.uc>   /* for definitions only */


/**
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
 * Word  +-+---------------+-------------+---------------+---------------+
 *    0  |E| Requester ID  |   Unused    | Sequence Num  | HostBuf[39:32]|
 *       +-+---------------+-------------+---------------+---------------+
 *    1  |                       Host Buffer [31:0]                      |
 *       +---------------------------------------------------------------+
 *    2  |  CTM ISL  |C|  Packet Number  |SPL|0|     Starting Offset     |
 *       +-+---+-----+-+-----------------+---+-+-------------------------+
 *    3  |N|BLS|           MU Buffer Address [39:11]                     |
 *       +-+---+---------+---------------+-------------------------------+
 *    4  |D| Meta Length |  RX Queue     |           Data Length         |
 *       +-+-------------+---------------+-------------------------------+
 */

// Fields not defined in the base descriptor definitions
#define SB_WQ_ENABLED_bf        0, 31, 31
#define SB_WQ_RID_bf            0, 30, 23
#define SB_WQ_SEQ_bf            0, 15, 8



#define NFD_OUT_FL_DESC_SIZE            8
#define NFD_OUT_FL_DESC_SIZE_lg2        (log2(NFD_OUT_FL_DESC_SIZE))

#ifndef NFD_OUT_FL_CACHE_SIZE
#error "NFD_OUT_FL_CACHE_SIZE must be defined"
#endif

#if (NFD_OUT_FL_CACHE_SIZE & (NFD_OUT_FL_CACHE_SIZE - 1))
#error "NFD_OUT_FL_CACHE_SIZE must be a power of 2"
#endif

#ifndef STAGE_BATCH_MANAGER_CTX
#error "STAGE_BATCH_MANAGER_CTX must be defined"
#endif

#ifndef STAGE_BATCH_FIRST_WORKER
#error "STAGE_BATCH_FIRST_WORKER must be defined"
#endif

#ifndef STAGE_BATCH_NUM_WORKERS
#error "STAGE_BATCH_NUM_WORKERS must be defined"
#endif

#define STAGE_BATCH_LAST_WORKER (STAGE_BATCH_FIRST_WORKER + STAGE_BATCH_NUM_WORKERS - 1)


#define NUM_IO_BLOCKS           5
#define PCI_OUT_SB_WQ_CREDIT_SIG_NUM       13
#define PCI_OUT_SB_CFG_SIG_NUM  14
#define ORDER_SIG_NUM           15

#define EMEM_ADDR(_x) (streq('_x','emem0') ? __ADDR_EMEM0 : \
                       (streq('_x','emem1') ? __ADDR_EMEM1 : \
                       (streq('_x','emem2') ? __ADDR_EMEM2 : NOEMEM)))

/* REMOVE ME */
#define USE_MU_WORK_QUEUES 1

/* REMOVE ME:  in nfd_internal.h ... but this is not uc-safe */
#define NFD_CFG_VF_OFFSET       64
#define NFD_OUT_FL_BUFS_PER_QUEUE       256

/**
 * LM state:
 *
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +---------------------------------------------------------------+
 *    0  |                         Sequence Number                       |
 *       +---------------------------------------------+-+---------------+
 *    1  |                 Unused                      |E| Requester ID  |
 *       +---------------------------------------------+-+---------------+
 */

#define LM_QSTATE_SEQ_bf        0, 31, 0
#define LM_QSTATE_ENABLED_bf    1, 8, 8
#define LM_QSTATE_RID_bf        1, 7, 0
#define LM_QSTATE_SIZE          16
#define LM_QSTATE_SIZE_LW       (LM_QSTATE_SIZE / 4)
#define LM_QSTATE_SIZE_lg2      (log2(LM_QSTATE_SIZE))
#define LM_QSTATE_CSR           ACTIVE_LM_ADDR_0
#define LM_QSTATE_PTR           *l$index0
#define LM_QSTATE               LM_QSTATE_PTR
#define LM_SEQ_wrd              0
#define LM_STATUS_wrd           1
#define LM_SEQ                  LM_QSTATE_PTR[LM_SEQ_wrd]
#define LM_STATUS               LM_QSTATE_PTR[LM_STATUS_wrd]


#define LM_WQ_CREDIT_CSR        ACTIVE_LM_ADDR_1
#define LM_WQ_CREDIT_PTR        *l$index1
#define LM_WQ_CREDITS           LM_WQ_CREDIT_PTR

// LMEM data structures
.alloc_mem sb_ctx_base lmem+0 me (LM_QSTATE_SIZE * NFD_OUT_MAX_QUEUES)
.alloc_mem sb_wq_credits lmem me 4

// Input ring
#define_eval __EMEM 'NFD_PCIE/**/PCIE_ISL/**/_EMEM'
.alloc_resource nfd_out_ring_num/**/PCIE_ISL/**/0 __EMEM/**/_queues global 1 1
.alloc_mem nfd_out_ring_mem/**/PCIE_ISL __EMEM global NFD_OUT_RING_SZ NFD_OUT_RING_SZ
.init_mu_ring nfd_out_ring_num/**/PCIE_ISL/**/0 nfd_out_ring_mem/**/PCIE_ISL

#if USE_MU_WORK_QUEUES

// MU work queues
#define_eval __EMEM 'NFD_PCIE/**/PCIE_ISL/**/_EMEM'
.alloc_resource nfd_out_sb_ring_num/**/PCIE_ISL __EMEM/**/_queues global 1 1
.alloc_mem nfd_out_sb_ring_mem/**/PCIE_ISL __EMEM global 2048 2048
.init_mu_ring nfd_out_sb_ring_num/**/PCIE_ISL nfd_out_sb_ring_mem/**/PCIE_ISL
#undef __EMEM

#else /* USE_MU_WORK_QUEUES */

// CLS work queues
#define NFD_OUT_SB_WQ_SIZE_LW  1024
#define NFD_OUT_SB_WQ_OFF 0
#define NFD_OUT_SB_WQ_NUM 15
.alloc_mem nfd_out_sb_wq_mem/**/PCIE_ISL cls+NFD_OUT_SB_WQ_OFF \
    island (NFD_OUT_SB_WQ_SIZE_LW * 4) (NFD_OUT_SB_WQ_SIZE_LW * 4)
.alloc_resource nfd_out_sb_ring_num/**/PCIE_ISL \
    cls_rings+NFD_OUT_SB_WQ_NUM island 1 1
.init_csr cls:i/**/THIS_ISL/**/.Rings.RingBase/**/NFD_OUT_SB_WQ_NUM \
    (((NFD_OUT_SB_WQ_OFF >> 7) << 0) | ((log2(NFD_OUT_SB_WQ_SIZE_LW) - 5) << 16))
.init_csr cls:i/**/THIS_ISL/**/.Rings.RingPtrs/**/NFD_OUT_SB_WQ_NUM 0

#endif /* USE_MU_WORK_QUEUES */

// Cache memory
#define_eval NFD_OUT_CACHE_ISL (NFD_PCIE_ISL_BASE + PCIE_ISL)
#define_eval NFD_OUT_CACHE_SIZE (NFD_OUT_FL_DESC_SIZE * NFD_OUT_MAX_QUEUES * NFD_OUT_FL_BUFS_PER_QUEUE)
#define_eval NFD_OUT_CACHE_ALIGN (NFD_OUT_MAX_QUEUES * NFD_OUT_FL_BUFS_PER_QUEUE * 8)
.alloc_mem fl_cache_mem/**/PCIE_ISL i/**/NFD_OUT_CACHE_ISL/**/.ctm global NFD_OUT_CACHE_SIZE NFD_OUT_CACHE_ALIGN

// Ticket release bitmaps
.alloc_mem nfd_out_sb_release/**/PCIE_ISL ctm island \
    (NFD_OUT_MAX_QUEUES * 16) (NFD_OUT_MAX_QUEUES * 16)

// WQ credits
.alloc_mem nfd_out_sb_wq_credits/**/PCIE_ISL ctm global 4 4

// Config rings
.alloc_resource nfd_cfg_ring_nums NFD_CFG_RING_EMEM/**/_queues global 32
.declare_resource nfd_cfg_ring_nums/**/PCIE_ISL global 8 nfd_cfg_ring_nums
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/0 nfd_cfg_ring_nums/**/PCIE_ISL global 1
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/1 nfd_cfg_ring_nums/**/PCIE_ISL global 1
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/2 nfd_cfg_ring_nums/**/PCIE_ISL global 1
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/3 nfd_cfg_ring_nums/**/PCIE_ISL global 1
.alloc_resource nfd_cfg_ring_num/**/PCIE_ISL/**/4 nfd_cfg_ring_nums/**/PCIE_ISL global 1

// Config message field declarations
#define NFD_CFG_MSG_VALID_bf            0, 31, 31
#define NFD_CFG_MSG_VALID_bit           31
#define NFD_CFG_MSG_ERR_bf              0, 30, 30
#define NFD_CFG_MSG_INTERESTED_bf       0, 29, 29
#define NFD_CFG_MSG_UP_bf               0, 28, 28
#define NFD_CFG_MSG_QUEUE_bf            0, 15, 8
#define NFD_CFG_MSG_VNIC_bf             0, 7, 0


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


#macro _set_queue_state(in_vnic, in_q, in_up)
.begin

    .reg qid
    .reg lma
    .reg changed
    .reg rid
    .reg currently_up

    alu[qid, in_q, OR, in_vnic, <<(log2(NFD_MAX_VF_QUEUES))]

    // Load the queue state for that queue
    alu[lma, --, B, qid, <<LM_QSTATE_SIZE_lg2]
    local_csr_wr[LM_QSTATE_CSR, lma]
    nop
    nop
    nop

    // If the queue state changed, then update the LM entry
    bitfield_extract__sz1(currently_up, F_AML(LM_QSTATE, LM_QSTATE_ENABLED_bf))
    .if (in_up != currently_up)

        .if (in_up != 0)

            alu[rid, in_vnic, +, NFD_CFG_VF_OFFSET]
            bitfield_insert__sz1(F_AML(LM_QSTATE, LM_QSTATE_RID_bf), rid)
            bitfield_insert__sz1(F_AML(LM_QSTATE, LM_QSTATE_ENABLED_bf), 1)
            move(LM_SEQ, 0)
            _reset_ticket_bitmap(qid)

        .else

            bitfield_clear__sz1(F_AML(LM_QSTATE, LM_QSTATE_RID_bf))
            bitfield_clear__sz1(F_AML(LM_QSTATE, LM_QSTATE_ENABLED_bf))

        .endif


    .endif

.end
#endm


#macro _get_bar_addr(out_hi, out_lo, in_vnic)
.begin

    .reg tmp_lo
    .reg off

    move(out_hi, ((nfd_cfg_base/**/PCIE_ISL >> 8) & 0xFF000000))
    move(tmp_lo, (nfd_cfg_base/**/PCIE_ISL & 0xFFFFFFFF))
    alu[off, --, B, in_vnic, <<(log2(NFP_NET_CFG_BAR_SZ))]
    alu[out_lo, tmp_lo, +, off]

.end
#endm


#macro _check_vnic_state(in_vnic)
.begin

    .reg bar_addr_hi
    .reg bar_addr_lo
    .reg q
    .reg up
    .reg maxqs

    .reg read $bar[6]
    .xfer_order $bar

    .sig read_sig

    _get_bar_addr(bar_addr_hi, bar_addr_lo, in_vnic)
    mem[read32, $bar[0], bar_addr_hi, <<8, bar_addr_lo, 6], ctx_swap[read_sig]

    .if (in_vnic < NFD_MAX_VFS)
        move(maxqs, NFD_MAX_VF_QUEUES)
    .else
        move(maxqs, NFD_MAX_PF_QUEUES)
    .endif


    .if ($bar[NFP_NET_CFG_CTRL] & NFP_NET_CFG_CTRL_ENABLE == 0)

        move(up, 0)
        move(q, 0)
        .while (q < maxqs)
            
            _set_queue_state(in_vnic, q, up)
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

            _set_queue_state(in_vnic, q, up)
            alu[q, q, +, 1]

        .endw

    .endif

    local_csr_wr[MAILBOX3, maxqs]
    
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

    local_csr_wr[MAILBOX1, addr]

    ct[interthread_signal, --, 0, addr]

.end
#endm


// REMOVE ME
.reg @CONFIGS
.init @CONFIGS 0

/**
 * Process reconfiguation messages.
 */
#macro process_reconfig()
.begin

    .reg ring_addr_hi
    .reg ring_in_addr_lo
    .reg ring_out_addr_lo
    .reg vnic

    .reg $cmsg[1]
    .xfer_order $cmsg

    .sig get_sig
    .sig put_sig

    move(ring_addr_hi, (EMEM_ADDR(NFD_CFG_RING_EMEM) >> 8))
    move(ring_in_addr_lo, nfd_cfg_ring_num/**/PCIE_ISL/**/2)
    move(ring_out_addr_lo, nfd_cfg_ring_num/**/PCIE_ISL/**/3)

    .while (1)

        .io_completed get_sig
        mem[get, $cmsg[0], ring_addr_hi, <<8, ring_in_addr_lo, 1], sig_done[get_sig]
        ctx_arb[get_sig[0]]

        .if (SIGNAL(get_sig[1]))
        .endif

        .if (BIT($cmsg[0], NFD_CFG_MSG_VALID_bit) == 0)
            .break
        .endif

        local_csr_wr[MAILBOX2, $cmsg[0]]

        // Extract the queue ID that is going up or down
        bitfield_extract__sz1(vnic, F_AML($cmsg, NFD_CFG_MSG_VNIC_bf))

        _check_vnic_state(vnic)

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


/**
 * Main processing loop for the manager context.  This context is responsible
 * for updating work queue credits and for processing configuration messages.
 */
#macro manager_main_loop()
.begin

    .reg lma

    .sig volatile visible _nfd_credit_sig_sb
    .addr _nfd_credit_sig_sb PCI_OUT_SB_WQ_CREDIT_SIG_NUM

    .sig volatile visible _nfd_cfg_sig_sb
    .addr _nfd_cfg_sig_sb PCI_OUT_SB_CFG_SIG_NUM


    move(lma, sb_wq_credits)
    local_csr_wr[LM_WQ_CREDIT_CSR, lma]
    local_csr_wr[MAILBOX0, 0xFF]

    .while (1) 

        .set_sig _nfd_credit_sig_sb
        .set_sig _nfd_cfg_sig_sb

        ctx_arb[_nfd_credit_sig_sb, _nfd_cfg_sig_sb], ANY

        .if (SIGNAL(_nfd_credit_sig_sb))

            update_wq_credits()

        .endif

        .if (SIGNAL(_nfd_cfg_sig_sb))

            alu[@CONFIGS, @CONFIGS, +, 1]
            local_csr_wr[MAILBOX0, @CONFIGS]

            process_reconfig()

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
 *    the queue ID shifted F_L(NFD_OUT_QID_bf) - LM_QSTATE_SIZE_lg2
 *    bits to the right to obtain the LM offset of the per-queue state.
 *  - g_cache_addr_lo_mask will mask down a shifted sequence number into an
 *    offset in the FL desc cache/RX desc cache
 *  - g_in_wq_lo and g_in_wq_hi contain the 40-bit address required to
 *    request the next work item.
 *  - g_out_wq_lo and g_out_wq_hi contains the address for sending to the
 *    output work queue.
 *
 * This function should consume exactly 19 cycles of processing per
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

    .reg read $buf_desc[2]
    .xfer_order $buf_desc

    .sig fl_read_sig

    // Loop here if no WQ credits
test_ready_to_send#:
    br=byte[LM_WQ_CREDITS, 0, 0, maybe_flow_controlled#]

ready_to_send#:
    // Burn per-CTX GPR to make 1 cycle
    local_csr_wr[SAME_ME_SIGNAL, g_sig_next_worker]

    // XXX Assumes that the queue state starts at address 0
    alu[lma, g_lm_qstate_mask, AND, F_A(in_xfer, NFD_OUT_QID_fld),
        >>(F_L(NFD_OUT_QID_fld) - LM_QSTATE_SIZE_lg2)]
    local_csr_wr[LM_QSTATE_CSR, lma]

    // Copy descriptor. Put at the end so last word can be ommitted in WQ
    move(out_xfer[2], in_xfer[0])               // lm addr cycle 0
    move(out_xfer[3], in_xfer[1])		// lm addr cycle 1
    move(out_xfer[4], in_xfer[2])		// lm addr cycle 2
    move(out_xfer[5], in_xfer[3])

    /*
     * Swap freelist descriptor for RX descriptor.
     * - get next addr_lo from sequence number
     * - issue read then write
     */
    alu[addr_lo, g_cache_addr_lo_mask, AND, LM_SEQ, <<NFD_OUT_FL_DESC_SIZE_lg2]
    mem[read, $buf_desc[0], g_cache_addr_hi, <<8, addr_lo, 1], sig_done[fl_read_sig]
    mem[write, out_xfer[4], g_cache_addr_hi, <<8, addr_lo, 1], sig_done[cur_outsig]

    // Start next work queue dequeue
    mem[qadd_thread, in_xfer[0], g_in_wq_hi, <<8, g_in_wq_lo, 4], sig_done[cur_insig]

    /*
     * Wait for read/write operations to complete
     * In the defer shadow:
     * - increment the sequence number
     * - Sequence number is (addr_lo >> NFD_OUT_FL_DESC_SIZE_lg2).  We will
     *   shift it into place (bit 8) below.  Fill in the requester ID at the
     *   proper bit position so that when we shift it, the requester ID will
     *   end up at bit F_L(SB_WQ_RID_bf)
     */
    ctx_arb[fl_read_sig, cur_outsig], defer[2]
    alu[LM_SEQ, LM_SEQ, +, 1]
    alu[addr_lo, addr_lo, OR, LM_STATUS, <<(F_L(SB_WQ_RID_bf) - (F_L(SB_WQ_SEQ_bf) - NFD_OUT_FL_DESC_SIZE_lg2))]

    // Start sending the work to issue DMA, but see below: we're not quite done
    #if USE_MU_WORK_QUEUES

        mem[qadd_work, out_xfer[0], g_out_wq_hi, <<8, g_out_wq_lo, 5], sig_done[cur_outsig]

    #else

        cls[ring_workq_add_work, out_xfer[0], g_out_wq_hi, <<8, g_out_wq_lo, 5], sig_done[cur_outsig]

    #endif

    /*
     * Wait for least recent I/Os to complete.
     *
     * XXX in the defer shadow of this, copy freelist descriptor to xfer regs.
     * Not sure whether this is a real race because I don't think that
     * cls[qadd_work] goes to the wire until 3 cycles after it's issued.  But
     * even if it were a race, we would always win due to the CPP bus clocks
     * required before pulling the data from $buf_desc.
     */
    .set_sig ordersig
    #pragma warning(disable:5009)
    ctx_arb[nxt_insig, nxt_outsig, ordersig], defer[2], br[DONE_LABEL]
    alu[out_xfer[0], $buf_desc[0], OR, addr_lo, <<(F_L(SB_WQ_SEQ_bf) - NFD_OUT_FL_DESC_SIZE_lg2)]
    move(out_xfer[1], $buf_desc[1])
    #pragma warning(default:5009)

    /*
     * Slow path exception handler.
     *
     * Out of credits, or least significant byte == 0.  Even if there can
     * be more than 256 credits outstanding (requiring this test), we should
     * only hit this condition about 1 out of every 256 packets.  So the total
     * cost would be 9/256 = ~0.04 cycles per packet.  At the very worst it
     * be once out of every 64 packets meaning 9 / 64 = ~0.16 cycles per
     * packet.
     */
maybe_flow_controlled#:
    alu[--, --, B, LM_WQ_CREDITS]
    beq[flow_controlled#]
    br[ready_to_send#], defer[1]
    alu[LM_WQ_CREDITS, LM_WQ_CREDITS, -, 1]

    // No credits, yield and then branch back to the test
flow_controlled#:
    ctx_arb[voluntary], br[test_ready_to_send#]

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
    .reg volatile g_cache_addr_hi
    .reg volatile g_cache_addr_lo_mask
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
    move(g_cache_addr_hi, (fl_cache_mem/**/PCIE_ISL >> 8))
    move(g_cache_addr_lo_mask, ((NFD_OUT_FL_CACHE_SIZE - 1) << NFD_OUT_FL_DESC_SIZE_lg2))
    move(g_in_wq_hi, ((nfd_out_ring_mem/**/PCIE_ISL >> 8) & 0xFF000000))
    move(g_in_wq_lo, nfd_out_ring_num/**/PCIE_ISL/**/0)
    move(lma, sb_wq_credits)
    local_csr_wr[LM_WQ_CREDIT_CSR, lma]

    #if USE_MU_WORK_QUEUES

        move(g_out_wq_hi, ((nfd_out_sb_ring_mem/**/PCIE_ISL >> 8) & 0xFF000000))
        move(g_out_wq_lo, nfd_out_sb_ring_num/**/PCIE_ISL)

    #else /* USE_MU_WORK_QUEUES */

        move(g_out_wq_hi, 0)
        move(g_out_wq_lo, ((nfd_out_sb_ring_num/**/PCIE_ISL) << 2))

    #endif /* USE_MU_WORK_QUEUES */

    /*
     * ORDERED ADD TO THE WORK QUEUE
     */
    #define_eval __BLOCK 0
    #while (__BLOCK < NUM_IO_BLOCKS)

        .set_sig ordersig
        ctx_arb[ordersig]
        mem[qadd_thread, $in_/**/__BLOCK[0], g_in_wq_hi, <<8, g_in_wq_lo, 4], sig_done[insig_/**/__BLOCK]
        local_csr_wr[SAME_ME_SIGNAL, g_sig_next_worker]

        #define_eval __BLOCK (__BLOCK + 1)
    #endloop


    /*
     * MAIN WORKER LOOP
     */

    // Wait for the signal for the first block
    .set_sig ordersig
    ctx_arb[insig_0, ordersig]

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



main#:

    .if (ctx() == STAGE_BATCH_MANAGER_CTX)

        manager_main_loop()

    .else

        #if 1 /* XXX REMOVE ME when really integrated */

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

        worker_main_loop()

    .endif
    nop

#endif /* __PCI_OUT_SB_UC */
