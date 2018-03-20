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
 * @file          blocks/vnic/pci_out_pd.uc
 * @brief         Implement "packet DMA" (PD) task for PCI.OUT
 */


#ifndef __PCI_OUT_PD_UC
#define __PCI_OUT_PD_UC

#include <stdmac.uc>
#include <cycle.uc>

#include <blm/blm_api.uc>

#include <nfp6000/nfp_pcie.h>

#include "wsm.uc"
#include "pci_out_sb.h"
#include "pci_out_sb_iface.uc"
#include "nfd_common.h"
#include "shared/nfd_internal.h"


#ifndef STAGE_BATCH_MANAGER_CTX
#define STAGE_BATCH_MANAGER_CTX 0
#endif

#ifndef NFD_OUT_RX_OFFSET
#warning "NFD_OUT_RX_OFFSET not defined: defaulting to NFP_NET_RX_OFFSET which is sub-optimal"
#define NFD_OUT_RX_OFFSET NFP_NET_RX_OFFSET
#endif /* NFD_OUT_RX_OFFSET */

// Send sequence numbers for send_desc
#if (NFD_OUT_MAX_QUEUES < 64)
#error "NFD_OUT_MAX_QUEUES must be >= 64 for nfd_out_send_cntrs optimization to work"
#endif
#define_eval PCIE_ISL_NUM (NFD_PCIE_ISL_BASE + PCIE_ISL)

#define NFD_OUT_CREDITS_BASE            0
.alloc_mem nfd_out_atomics/**/PCIE_ISL \
    i/**/PCIE_ISL_NUM/**/.ctm+NFD_OUT_CREDITS_BASE \
    global (NFD_OUT_MAX_QUEUES * NFD_OUT_ATOMICS_SZ)

/* Required user configuration */
#ifndef NFD_OUT_BLM_POOL_START
#error "NFD_OUT_BLM_POOL_START must be defined by the user"
#endif

#ifndef NFD_OUT_BLM_RADDR_UC
#error "NFD_OUT_BLM_RADDR_UC must be defined by the user"
#endif

// This QID must be a multiple of four for our optimization of ORing in
// the BLS to work.
.assert((NFD_OUT_BLM_POOL_START & 0x3) == 0)

// See NFP Databook Section 9.2.2.1.2.10 "TicketRelease Command"
#define TICKET_ERROR                    255

#define NFD_OUT_MAX_PKT_BYTES           (10 * 1024)

// Debug parameters
#ifndef ONE_PKT_AT_A_TIME
#define ONE_PKT_AT_A_TIME 0
#endif

#ifndef COMPILE_ONLY_DEBUG
#define COMPILE_ONLY_DEBUG 0
#endif

/*
 * Reserve PCIe Resources
 */
#ifdef HAS_PCIE_DMA_GEN_RC
    #ifdef _NFD_OUT_DATA_MAX_IN_FLIGHT
        #warning "_NFD_OUT_DATA_MAX_IN_FLIGHT is being redefined."
    #endif

    #if ONE_PKT_AT_A_TIME
        #define_eval _NFD_OUT_DATA_MAX_IN_FLIGHT (1 * 2)
    #else
        #define_eval _NFD_OUT_DATA_MAX_IN_FLIGHT (3 * 2)
    #endif

    #ifdef NFD_OUT_3_PD_MES
        #define_eval _NFD_OUT_DATA_MAX_IN_FLIGHT \
            (7 * _NFD_OUT_DATA_MAX_IN_FLIGHT)
    #else
        #define_eval _NFD_OUT_DATA_MAX_IN_FLIGHT \
            (8 * _NFD_OUT_DATA_MAX_IN_FLIGHT)
    #endif

    PCIE_DMA_ALLOC(nfd_out_data_dma, me, PCIE_ISL, topci_lo, \
                   _NFD_OUT_DATA_MAX_IN_FLIGHT)

    #undef _NFD_OUT_DATA_MAX_IN_FLIGHT
#endif


/*
 * DMA Descriptor
 *
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +---------------------------------------------------------------+
 *    0  |                        CPP Address Low                        |
 *       +---+-------------------------------+---+-------+---------------+
 *    1  |Sel|          DMA Mode             |Tok|DMACFGI|  CPP Addr Hi  |
 *       +---+-------------------------------+---+-------+---------------+
 *    2  |                       PCIE Address Low                        |
 *       +-----------------------+---------------+-+-----+---------------+
 *    3  |   Transfer Length     | Requester ID  |O| XC  | PCIe Addr Hi  |
 *       +-----------------------+---------------+-+-----+---------------+
 *
 * DMA Descriptor: signal only
 *
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +---------------------------------------------------------------+
 *    0  |                        CPP Address Low                        |
 *       +-+-------+-----------+-----+-------+---+-------+---------------+
 *    1  |0| MstID |Master Isl | Ctx |Signum |Tok|DMACFGI|  CPP Addr Hi  |
 *       +-+-------+-----------+-----+-------+---+-------+---------------+
 *    2  |                       PCIE Address Low                        |
 *       +-----------------------+---------------+-+-----+---------------+
 *    3  |   Transfer Length     | Requester ID  |O| XC  | PCIe Addr Hi  |
 *       +-----------------------+---------------+-+-----+---------------+
 */

// Word 0
#define PCIE_DMA_CPPA_LO_bf     0, 31, 0
#define PCIE_DMA_CPPA_LO_wrd    0
#define PCIE_DMA_CPPA_LO_shf    0
#define PCIE_DMA_CPPA_LO_msk    0xFFFFFFFF

// Word 1
#define PCIE_DMA_MODE_SEL_bf    1, 31, 30
#define PCIE_DMA_MODE_SEL_wrd   1
#define PCIE_DMA_MODE_SEL_shf   30
#define PCIE_DMA_MODE_SEL_msk   0x3
#define PCIE_DMA_MASTER_ID_bf   1, 30, 27
#define PCIE_DMA_MASTER_ID_wrd  1
#define PCIE_DMA_MASTER_ID_shf  27
#define PCIE_DMA_MASTER_ID_msk  0xF
#define PCIE_DMA_MASTER_ISL_bf  1, 26, 21
#define PCIE_DMA_MASTER_ISL_wrd 1
#define PCIE_DMA_MASTER_ISL_shf 21
#define PCIE_DMA_MASTER_ISL_msk 0x3F
#define PCIE_DMA_SIGCTX_bf      1, 20, 18
#define PCIE_DMA_SIGCTX_wrd     1
#define PCIE_DMA_SIGCTX_shf     18
#define PCIE_DMA_SIGCTX_msk     0x7
#define PCIE_DMA_SIGNUM_bf      1, 17, 14
#define PCIE_DMA_SIGNUM_wrd     1
#define PCIE_DMA_SIGNUM_shf     14
#define PCIE_DMA_SIGNUM_msk     0xF
#define PCIE_DMA_TOKEN_bf       1, 13, 12
#define PCIE_DMA_TOKEN_wrd      1
#define PCIE_DMA_TOKEN_shf      12
#define PCIE_DMA_TOKEN_msk      0x3
#define PCIE_DMA_CFG_IDX_bf     1, 11, 8
#define PCIE_DMA_CFG_IDX_wrd    1
#define PCIE_DMA_CFG_IDX_shf    8
#define PCIE_DMA_CFG_IDX_msk    0xF
#define PCIE_DMA_CPPA_HI_bf     1, 7, 0
#define PCIE_DMA_CPPA_HI_wrd    1
#define PCIE_DMA_CPPA_HI_shf    0
#define PCIE_DMA_CPPA_HI_msk    0xFF

// Word 2
#define PCIE_DMA_PCIEA_LO_bf    2, 31, 0
#define PCIE_DMA_PCIEA_LO_wrd   2
#define PCIE_DMA_PCIEA_LO_shf   0
#define PCIE_DMA_PCIEA_LO_msk   0xFFFFFFFF

// Word 3
#define PCIE_DMA_XLEN_bf        3, 31, 20
#define PCIE_DMA_XLEN_wrd       3
#define PCIE_DMA_XLEN_shf       20
#define PCIE_DMA_XLEN_msk       0xFFF
#define PCIE_DMA_RID_bf         3, 19, 12
#define PCIE_DMA_RID_wrd        3
#define PCIE_DMA_RID_shf        12
#define PCIE_DMA_RID_msk        0xFF
#define PCIE_DMA_OVDRID_bf      3, 11, 11
#define PCIE_DMA_OVDRID_wrd     3
#define PCIE_DMA_OVDRID_shf     11
#define PCIE_DMA_OVDRID_msk     0x1
#define PCIE_DMA_XCLASS_bf      3, 10, 8
#define PCIE_DMA_XCLASS_wrd     3
#define PCIE_DMA_XCLASS_shf     8
#define PCIE_DMA_XCLASS_msk     0x7
#define PCIE_DMA_PCIEA_HI_bf    3, 7, 0
#define PCIE_DMA_PCIEA_HI_wrd   3
#define PCIE_DMA_PCIEA_HI_shf   0
#define PCIE_DMA_PCIEA_HI_msk   0xFF

#define PCIE_DMA_SIZE_LW        4

#define PCIE_DMA_MAX_LEN        2048

#define PCIE_DMA_WORD1_NOSIG_val \
    (NFD_OUT_DATA_CFG_REG | (NFD_OUT_DATA_DMA_TOKEN << 4))
#define PCIE_DMA_WORD1_NOSIG_shf 8

#define NFP_PCIE_DMA_TOPCI_LO   0x40040

#macro wait_br_next_state(in_sig0, in_sig1, LABEL)
    wait_br_next_state(in_sig0, in_sig1, LABEL, --)
#endm


/**
 * Wait one or two signals to arrive and on receiving them branch to
 * a given location.  If one signal is specified, the wait is
 * unconditional and consumes the signal.  If two signals are specified,
 * then we wait for either one to arrive but without clearing either.
 *
 * @param in_sig0       The first signal to wait on (must be real)
 * @param in_sig1       The second signal to wait on (may be '--' if none)
 * @param LABEL         Label to branch to on signal reception.
 * @param OPT_TOK       An optional token to be added to the command
 *                      (e.g. defer[1]).  If '--' no such token.
 */
#macro wait_br_next_state(in_sig0, in_sig1, LABEL, OPT_TOK)

    .set_sig in_sig0

    #if (streq('in_sig1', '--'))
        #if (!streq('OPT_TOK', '--'))
            ctx_arb[in_sig0], br[LABEL], OPT_TOK
        #else
            ctx_arb[in_sig0], br[LABEL]
        #endif
    #else
        .set_sig in_sig1
        #if (!streq('OPT_TOK', '--'))
            ctx_arb[in_sig0, in_sig1], ANY, br[LABEL], OPT_TOK
        #else
            ctx_arb[in_sig0, in_sig1], ANY, br[LABEL]
        #endif
    #endif
#endm


#macro signal_sb_credits_update()
.begin
    .reg addr

    #define_eval SB_SIG_ADDR (((NFD_OUT_STAGE_ME & 0x3F0)  << 20) | \
                             ((NFD_OUT_STAGE_ME & 0xF)     << 9)  | \
                             (STAGE_BATCH_MANAGER_CTX      << 6)  | \
                             (PCI_OUT_SB_WQ_CREDIT_SIG_NUM << 2))
    move(addr, SB_SIG_ADDR)

    ct[interthread_signal, --, 0, addr]

    #undef SB_SIG_ADDR

.end
#endm


#macro _build_mu_dma_word1(out_dma, in_work, in_sig)
.begin

    .reg word

    // TODO:  support NFP buffers crossing 4G boundaries

    #if (streq('in_sig', 'NOSIG'))

        ld_field_w_clr[word, 0001, in_work[3], >>21]
        alu[out_dma[1], word, OR, PCIE_DMA_WORD1_NOSIG_val, <<PCIE_DMA_WORD1_NOSIG_shf]

    #else

        // XXX the ld_field op deliberately over writes the "0x80" in
        // g_dma_word1_vals from the CTM addressing
        alu[word, --, b, g_dma_word1_vals]
        ld_field[word, 0001, in_work[3], >>21]
        alu[out_dma[1], word, OR, (&in_sig), <<PCIE_DMA_SIGNUM_shf]

    #endif

.end
#endm


/**
 * Issue the DMAs required to send a packet to a host buffer.  The parameters
 * for transmission are specified in 'in_work'.  The macro is given two
 * sets of transfer registers (out_dma[01]) and one signal (dma_sig) to
 * use for this.  It must consume no more than two DMAs in the low priority
 * to-PCIe DMA queue.  The last DMA must be completed with signal dma_sig
 * and must not be waited on within this macro.  After issuing the last
 * DMA, the macro must invoke wait_br_next_state() to wait for the next
 * signals to arrive for state transition (in_wait_sig0 and maybe
 * in_wait_sig1), branching to LABEL on completion.  This scheme allows
 * filling defer slots on these state transitions with instructions required
 * to issue DMAs.
 *
 * @param in_work       Read-only work queue entry from stage batch used
 *                      to tell how and where to issue the DMAs.
 *                      (see: pci_out_sb.h)
 * @param out_dma0      First block of write transfer registers to use for
 *                      DMAs.
 * @param out_dma1      Second block of write transfer registers to use for
 *                      DMAs.
 * @param dma_sig       Signal to use for DMAs
 * @param LABEL         LABEL to branch to after getting state transition
 *                      signal(s).
 * @param in_wait_sig0  First state transition signal.  Must be specified.
 * @param in_wait_sig1  Second state transition signal.  Optional.  '--' if
 *                      not used.
 */
#macro _issue_packet_dma(in_work, out_dma0, out_dma1, dma_sig, \
                         LABEL, in_wait_sig0, in_wait_sig1)
.begin

    .reg word
    .reg tmp
    .reg tmp2
    .reg isl
    .reg ovfl
    .reg len
    .reg addr_lo

    .reg pcie_hi_word
    .reg pcie_lo_start
    .reg mu_lo_start
    .reg ctm_bytes
    .reg split_len

    .io_completed in_work[0]
    .io_completed in_work[1]
    .io_completed in_work[2]
    .io_completed in_work[3]
    .io_completed in_work[4]
    .io_completed out_dma0[0]
    .io_completed out_dma0[1]
    .io_completed out_dma0[2]
    .io_completed out_dma0[3]
    .io_completed out_dma1[0]
    .io_completed out_dma1[1]
    .io_completed out_dma1[2]
    .io_completed out_dma1[3]

    alu[@ndequeued, @ndequeued, -, 1]
    blt[add_wq_credits#]

start_packet_dma#:
    wsm_test_bit_clr(in_work, SB_WQ_ENABLED, no_dma#)
    wsm_test_bit_clr(in_work, SB_WQ_CTM_ONLY, not_ctm_only#)

    // Super fast path
ctm_only#:
    // Word 0
    // XXX Optimization: packet number is already in place
    alu[word, in_work[2], AND, g_dma_word0_mask]
    alu[out_dma0[0], word, OR, 1, <<31] // Packet format bit

    // Word 1
    wsm_extract(word, in_work, SB_WQ_CTM_ISL)
    alu[word, word, OR, g_dma_word1_vals]
    alu[out_dma0[1], word, OR, (&dma_sig), <<PCIE_DMA_SIGNUM_shf]

    // Word 2
    #if NFD_OUT_RX_OFFSET > 0

        wsm_extract(tmp2, in_work, SB_WQ_METALEN)
        alu[tmp, NFD_OUT_RX_OFFSET, -, tmp2]
        alu[out_dma0[2], tmp, +, in_work[SB_WQ_HOST_ADDR_LO_wrd]]

    #else /* NFD_OUT_RX_OFFSET > 0 */

        alu[out_dma0[2], --, B, in_work[SB_WQ_HOST_ADDR_LO_wrd]]

    #endif /* NFD_OUT_RX_OFFSET > 0 */

    // Word 3
    alu[word, g_dma_word3_vals, +8, in_work[SB_WQ_HOST_ADDR_HI_wrd]], no_cc
    #if NFD_OUT_RX_OFFSET > 0
        alu[word, word, +carry, 0]
        // FIXME: possibly carry past 40-bit address into traffic class?
    #endif /* NFD_OUT_RX_OFFSET > 0 */
    wsm_extract(tmp, in_work,  SB_WQ_RID)
    sm_set_noclr(word, PCIE_DMA_RID, tmp)
    wsm_extract(len, in_work, SB_WQ_DATALEN)

    // Gambling on being fast enough to beat the DMA pull from these registers
    #pragma warning(disable:5117)
    #pragma warning(disable:4701)
    #pragma warning(disable:5009)

    pcie[write_pci, out_dma0[0], g_pcie_addr_hi, <<8, g_pcie_addr_lo, 4]

    #pragma warning(default:5117)
    // This wait() always has 2 defer slots following it
    wait_br_next_state(in_wait_sig0, in_wait_sig1, LABEL, defer[2])
    alu[len, len, -, 1]
    sm_set_noclr_to(out_dma0[3], word, PCIE_DMA_XLEN, len, 1)
    #pragma warning(default:4701)
    #pragma warning(default:5009)

not_ctm_only#:
    // Prepare data that is required for all further branches:

    // (1) the start address in host mem
    #if NFD_OUT_RX_OFFSET > 0

        wsm_extract(tmp2, in_work, SB_WQ_METALEN)
        alu[tmp, NFD_OUT_RX_OFFSET, -, tmp2]
        alu[pcie_lo_start, tmp, +, in_work[SB_WQ_HOST_ADDR_LO_wrd]]

    #else /* NFD_OUT_RX_OFFSET > 0 */

        alu[pcie_lo_start, --, B, in_work[SB_WQ_HOST_ADDR_LO_wrd]]

    #endif /* NFD_OUT_RX_OFFSET > 0 */

    // (2) the partial word 3 (pcie_hi_word).  Aside from the DMA length,
    // it is constant for all DMAs
    alu[pcie_hi_word, g_dma_word3_vals, +8, in_work[SB_WQ_HOST_ADDR_HI_wrd]], no_cc
    #if NFD_OUT_RX_OFFSET > 0
        alu[pcie_hi_word, pcie_hi_word, +carry, 0]
        // FIXME: possibly carry past 40-bit address into traffic class?
    #endif /* NFD_OUT_RX_OFFSET > 0 */

    wsm_extract(tmp, in_work,  SB_WQ_RID)
    sm_set_noclr(pcie_hi_word, PCIE_DMA_RID, tmp)

    // (3) the total data length to send
    wsm_extract(len, in_work, SB_WQ_DATALEN)

    // Knock MU only packets off the fall through path
    wsm_extract(isl, in_work, SB_WQ_CTM_ISL)
    beq[mu_only_dma#]


ctm_and_mu_dma#:
    // The priority at this point are packets which have only just
    // spilled over into MU.  Cycle budget on this path is still
    // critical, so we start with the work that is a priority for
    // those packets.

    // Compute how many bytes are in CTM from the split length
    // and starting offset
    move(tmp, 256)
    wsm_extract(tmp2, in_work, SB_WQ_CTM_SPLIT)
    alu[--, tmp2, or, 0]
    alu_shf[split_len, --, B, tmp, <<indirect]
    wsm_extract(tmp, in_work, SB_WQ_OFFSET)
    alu[ctm_bytes, split_len, -, tmp]
    ble[mu_only_dma#]

    // DMA0 Word 0: CPP address low
    // CTM address low, computation as for ctm_only
    // XXX Optimization: packet number is already in place
    alu[word, in_work[2], AND, g_dma_word0_mask]
    alu[out_dma0[0], word, OR, 1, <<31] // Packet format bit

    // DMA0 Word 1: CTM address hi and signals, computation as for ctm_only
    wsm_extract(word, in_work, SB_WQ_CTM_ISL)
    alu[word, word, OR, 0x80]                   ; Direct access
    alu[out_dma0[1], word, OR, PCIE_DMA_WORD1_NOSIG_val, <<PCIE_DMA_WORD1_NOSIG_shf]

    // DMA0 Word 2: PCIe addresses
    alu[out_dma0[2], --, b, pcie_lo_start]

    // DMA0 Word 3: length, RID, PCIE address HI
    alu[tmp, ctm_bytes, -, 1]
    sm_set_noclr_to(out_dma0[3], pcie_hi_word, PCIE_DMA_XLEN, tmp)

    // Issue the CTM DMA
    #pragma warning(disable:5117)
    pcie[write_pci, out_dma0[0], g_pcie_addr_hi, <<8, g_pcie_addr_lo, 4]
    #pragma warning(default:5117)

    // Compute the length remaining in MU
    // It must be greater than zero or else we have bad input
    alu[len, len, -, ctm_bytes]
    ble[ctm_only_not_flagged#]

    // DMA1 Word 0: MU address low, data starts split_len into the MU buffer
    alu[mu_lo_start, --, b, in_work[3], <<11]
    alu[mu_lo_start, mu_lo_start, +, split_len]
    move(out_dma1[0], mu_lo_start)

    // DMA1 Word 1: MU address hi and signals
    _build_mu_dma_word1(out_dma1, in_work, dma_sig)

    // DMA1 Word 2: PCIe addresses
    alu[pcie_lo_start, pcie_lo_start, +, ctm_bytes]
    alu[pcie_hi_word, pcie_hi_word, +carry, 0]
    move(out_dma1[2], pcie_lo_start)

    .if (len <= g_dma_max)

        // We can finish this packet with one CTM DMA and one MU DMA

        // Word 3: length, RID, PCIE address HI
        alu[tmp, len, -, 1]
        sm_set_noclr_to(out_dma1[3], pcie_hi_word, PCIE_DMA_XLEN, tmp, 1)

        #pragma warning(disable:5117)
        pcie[write_pci, out_dma1[0], g_pcie_addr_hi, <<8, g_pcie_addr_lo, 4]
        #pragma warning(default:5117)

    .else /* CTM+MU Jumbo */

        // Check for oversize packets off the fast path.
        // len no longer holds the full packet length, so we re-extract
        // it to a temporary variable
        wsm_extract(tmp, in_work, SB_WQ_DATALEN)
        .if (tmp > NFD_OUT_MAX_PKT_BYTES)
            .while (1)
                move(tmp, NFD_OUT_PD_INVALID_LENGTH)
                local_csr_wr[MAILBOX0, tmp]
                local_csr_wr[MAILBOX1, in_work[2]]
                local_csr_wr[MAILBOX2, in_work[3]]
                local_csr_wr[MAILBOX3, in_work[4]]
                ctx_arb[bpt]
            .endw
        .endif

        // Finish second DMA which must be g_dma_max bytes long

        // Word 3: length, RID, PCIE address HI
        alu[tmp, g_dma_max, -, 1]
        sm_set_noclr_to(out_dma1[3], pcie_hi_word, PCIE_DMA_XLEN, tmp, 1)

        // MU DMA, using out_dma1
        #pragma warning(disable:5117)
        pcie[write_pci, out_dma1[0], g_pcie_addr_hi, <<8, g_pcie_addr_lo, 4]
        #pragma warning(default:5117)

        // Adjust offsets and lengths
        alu[len, len, -, g_dma_max]
        alu[mu_lo_start, mu_lo_start, +, g_dma_max]
        alu[pcie_lo_start, pcie_lo_start, +, g_dma_max]
        alu[pcie_hi_word, pcie_hi_word, +carry, 0]

        .repeat

            // Swap on the DMA completion, which implicitly signals the xfers
            // are free again
            ctx_arb[dma_sig]
            .io_completed out_dma0[0]
            .io_completed out_dma0[1]
            .io_completed out_dma0[2]
            .io_completed out_dma0[3]
            .io_completed out_dma1[0]
            .io_completed out_dma1[1]
            .io_completed out_dma1[2]
            .io_completed out_dma1[3]

            // DMA0 word 0
            alu[out_dma0[0], --, B, mu_lo_start]

            // DMA0 Word 1
            _build_mu_dma_word1(out_dma0, in_work, dma_sig)

            // DMA0 Word 2
            alu[out_dma0[2], --, B, pcie_lo_start]

            // DMA0 Word 3
            .if (len <= g_dma_max)
                alu[tmp, len, -, 1]
                sm_set_noclr_to(out_dma0[3], pcie_hi_word, PCIE_DMA_XLEN, tmp, 1)
                move(len, 0)
            .else
                alu[tmp, g_dma_max, -, 1]
                sm_set_noclr_to(out_dma0[3], pcie_hi_word, PCIE_DMA_XLEN, tmp, 1)
                alu[len, len, -, g_dma_max]
                alu[mu_lo_start, mu_lo_start, +, g_dma_max]
                alu[pcie_lo_start, pcie_lo_start, +, g_dma_max]
                alu[pcie_hi_word, pcie_hi_word, +carry, 0]
            .endif

            #pragma warning(disable:5117)
            pcie[write_pci, out_dma0[0], g_pcie_addr_hi, <<8, g_pcie_addr_lo, 4]
            #pragma warning(default:5117)


        .until (len == 0)

    .endif

    wait_br_next_state(in_wait_sig0, in_wait_sig1, LABEL)


mu_only_dma#:
    // Compute the MU start address
    alu[mu_lo_start, --, b, in_work[3], <<11]
    wsm_extract(tmp, in_work,  SB_WQ_OFFSET)
    alu[mu_lo_start, mu_lo_start, +, tmp]

    // DMA0 Word 0
    move(out_dma0[0], mu_lo_start)

    // DMA0 Word 1: MU address hi and signal
    _build_mu_dma_word1(out_dma0, in_work, dma_sig)

    // DMA0 Word 2: PCIE address low
    move(out_dma0[2], pcie_lo_start)

    .if (len <= g_dma_max)

        // DMA0 Word 3: PCIE address high + data length
        alu[tmp, len, -, 1]
        sm_set_noclr_to(out_dma0[3], pcie_hi_word, PCIE_DMA_XLEN, tmp, 1)

        #pragma warning(disable:5117)
        pcie[write_pci, out_dma0[0], g_pcie_addr_hi, <<8, g_pcie_addr_lo, 4]
        #pragma warning(default:5117)

    .else

        // Check for oversize packets off the fast path.
        // len still holds the full packet length
        .if (len > NFD_OUT_MAX_PKT_BYTES)
            .while (1)
                move(tmp, NFD_OUT_PD_INVALID_LENGTH)
                local_csr_wr[MAILBOX0, tmp]
                local_csr_wr[MAILBOX1, in_work[2]]
                local_csr_wr[MAILBOX2, in_work[3]]
                local_csr_wr[MAILBOX3, in_work[4]]
                ctx_arb[bpt]
            .endw
        .endif

        // DMA0 Word 3: PCIE address high + data length
        alu[tmp, g_dma_max, -, 1]
        sm_set_noclr_to(out_dma0[3], pcie_hi_word, PCIE_DMA_XLEN, tmp, 1)

        #pragma warning(disable:5117)
        pcie[write_pci, out_dma0[0], g_pcie_addr_hi, <<8, g_pcie_addr_lo, 4]
        #pragma warning(default:5117)

        // Adjust offsets and lengths
        alu[len, len, -, g_dma_max]
        alu[mu_lo_start, mu_lo_start, +, g_dma_max]
        alu[pcie_lo_start, pcie_lo_start, +, g_dma_max]
        alu[pcie_hi_word, pcie_hi_word, +carry, 0]

        .repeat

            // Swap on the DMA completion, which implicitly signals the xfers
            // are free again
            ctx_arb[dma_sig]
            .io_completed out_dma0[0]
            .io_completed out_dma0[1]
            .io_completed out_dma0[2]
            .io_completed out_dma0[3]
            .io_completed out_dma1[0]
            .io_completed out_dma1[1]
            .io_completed out_dma1[2]
            .io_completed out_dma1[3]

            // DMA0 word 0
            alu[out_dma0[0], --, B, mu_lo_start]

            // DMA0 Word 1
            _build_mu_dma_word1(out_dma0, in_work, dma_sig)

            // DMA0 Word 2
            alu[out_dma0[2], --, B, pcie_lo_start]

            // DMA0 Word 3
            .if (len <= g_dma_max)
                alu[tmp, len, -, 1]
                sm_set_noclr_to(out_dma0[3], pcie_hi_word, PCIE_DMA_XLEN, tmp, 1)
                move(len, 0)
            .else
                alu[tmp, g_dma_max, -, 1]
                sm_set_noclr_to(out_dma0[3], pcie_hi_word, PCIE_DMA_XLEN, tmp, 1)
                alu[len, len, -, g_dma_max]
                alu[mu_lo_start, mu_lo_start, +, g_dma_max]
                alu[pcie_lo_start, pcie_lo_start, +, g_dma_max]
                alu[pcie_hi_word, pcie_hi_word, +carry, 0]
            .endif

            #pragma warning(disable:5117)
            pcie[write_pci, out_dma0[0], g_pcie_addr_hi, <<8, g_pcie_addr_lo, 4]
            #pragma warning(default:5117)

        .until (len == 0)

    .endif

    wait_br_next_state(in_wait_sig0, in_wait_sig1, LABEL)


no_dma#:
    // signal self w/ output signal:  "DMA" is completed
    local_csr_rd[ACTIVE_CTX_STS]
    immed[tmp, 0]
    alu[tmp, tmp, AND, 7]
    alu[tmp, tmp, OR, (&dma_sig), <<3]
    local_csr_wr[SAME_ME_SIGNAL, tmp]
    wait_br_next_state(in_wait_sig0, in_wait_sig1, LABEL)


    // Exception targets
ctm_only_not_flagged#:
    // We should only reach this point if the user did not flag
    // a packet as "ctm_only" correctly, or the input descriptor
    // was corrupt.  Either way, stop the ME.
    move(tmp, NFD_OUT_PD_UNMARKED_CTM_ONLY)
    local_csr_wr[MAILBOX0, tmp]
    local_csr_wr[MAILBOX1, in_work[2]]
    local_csr_wr[MAILBOX2, in_work[3]]
    local_csr_wr[MAILBOX3, in_work[4]]
    ctx_arb[bpt]
    br[ctm_only_not_flagged#]


add_wq_credits#:
    move(addr_lo, nfd_out_sb_wq_credits/**/PCIE_ISL)
    alu[--, g_add_imm_iref, OR, SB_WQ_CREDIT_BATCH, <<16]
    mem[add_imm, --, 0, addr_lo], indirect_ref

    signal_sb_credits_update()

    br[start_packet_dma#], defer[1]
    move(@ndequeued, (SB_WQ_CREDIT_BATCH - 1))

.end
#endm


/**
 * Process the completion of the last DMA for a packet.  This entails:
 *  * freeing the packet's CTM and MU buffers
 *  * issuing a mem[release_ticket] with the packet sequence number
 *    on the queue's ticket release bitmap.
 *  * atomically adding the return value from mem[release_ticket] to
 *    the send_desc ME's per queue sequence numbers (if non-zero)
 * Next, the macro issues a new request for work using the transfer
 * registers and signal that held the previous work request.  Finally,
 * this macro waits on one or two state transition signals and, on
 * reception, branches to a specified label to start processing for
 * the next state.
 *
 * @param io_work       Read transfer registers containing the original
 *                      parameters for DMAing the packet to the host.
 *                      After "completing" the DMA, this macro will
 *                      ask for more work from the work queue using
 *                      these transfer registers.
 * @param io_wq_sig     Signal used to indicate completion of the original
 *                      parameters for DMAing the packet to the host.
 *                      After "completing" the DMA, this macro will
 *                      ask for more work from the work queue using
 *                      this signal.
 * @param LABEL         Label to branch to after receiving state
 *                      transition signals.
 * @param in_wait_sig0  First state transition signal.  Must be specified.
 * @param in_wait_sig0  Second state transition signal.  Can be '--'
 *                      indicating no signal.
 */
#macro _complete_packet_dma(io_work, in_wq_sig, LABEL, in_wait_sig0, \
                            in_wait_sig1)
.begin

    .reg isl
    .reg qnum
    .reg addr_hi
    .reg addr_lo
    .reg ring_num
    .reg bitmap_lo
    .reg cntr_addr_lo
    .reg seq

    .reg $ticket
    .sig ticket_sig

    wsm_extract(qnum, io_work, SB_WQ_QNUM)
    alu[bitmap_lo, g_bitmap_base, OR, qnum, <<4]
    alu[$ticket, g_seq_mask, AND, io_work[SB_WQ_SEQ_wrd], >>SB_WQ_SEQ_shf]

    wsm_extract(isl, io_work, SB_WQ_CTM_ISL)
    beq[no_ctm_buffer#]

    // Free CTM buffer
    alu[addr_hi, g_pkt_free_hi, OR, isl, <<24]
    alu[addr_lo, g_pkt_num_mask, AND, io_work[SB_WQ_PKT_NUM_wrd], >>SB_WQ_PKT_NUM_shf]
    mem[packet_free, --, addr_hi, <<8, addr_lo]

no_ctm_buffer#:
    mem[release_ticket, $ticket, 0, bitmap_lo, 1], sig_done[ticket_sig]

    wsm_extract(addr_lo, io_work, SB_WQ_MUBUF)
    ctx_arb[ticket_sig], defer[2]
    alu[cntr_addr_lo, NFD_OUT_ATOMICS_DMA_DONE, OR, qnum, <<4]
    wsm_extract(ring_num, io_work, SB_WQ_BLS)

    // Free the MU buffer
    alu[-- , g_blm_iref, OR, ring_num, <<16]
    mem[fast_journal, --, g_blm_addr_hi, <<8, addr_lo], indirect_ref

ticket_ready#:
    br=byte[$ticket, 0, 0, complete_done#]
    br=byte[$ticket, 0, TICKET_ERROR, ticket_error#]

    alu[--, g_add_imm_iref, OR, $ticket, <<16]
    mem[add_imm, --, g_send_cntrs_addr_hi, <<8, cntr_addr_lo], indirect_ref

complete_done#:
    #pragma warning(disable:5009)
    pci_out_pd_request_work(io_work[0], in_wq_sig)
    #pragma warning(default:5009)
    wait_br_next_state(in_wait_sig0, in_wait_sig1, LABEL)

ticket_error#:
    alu[g_num_ticket_errors, g_num_ticket_errors, +, 1]
    local_csr_wr[MAILBOX0, g_num_ticket_errors]
    wsm_extract(seq, io_work, SB_WQ_SEQ)
    local_csr_wr[MAILBOX1, seq]
    cycle32_sleep(250)
    mem[release_ticket, $ticket, 0, bitmap_lo, 1], sig_done[ticket_sig]
    ctx_arb[ticket_sig], br[ticket_ready#]

.end
#endm


#macro issue_packet_dma(XNUM, LABEL, wsig0, wsig1)

    .io_completed work_sig/**/XNUM
    .io_completed dma_sig/**/XNUM

    _issue_packet_dma($work_in/**/XNUM,
                      $dma_out/**/XNUM,
                      $dma_out/**/XNUM/**/x,
                      dma_sig/**/XNUM,
                      LABEL,
                      wsig0,
                      wsig1)

#endm


#macro issue_packet_dma(XNUM, LABEL, wsig0)
    issue_packet_dma(XNUM, LABEL, wsig0, --)
#endm


#macro complete_packet_dma(XNUM, LABEL, wsig0, wsig1)

    _complete_packet_dma($work_in/**/XNUM,
                         work_sig/**/XNUM,
                        LABEL,
                        wsig0,
                        wsig1)

#endm


#macro complete_packet_dma(XNUM, LABEL, wsig0)
    complete_packet_dma(XNUM, LABEL, wsig0, --)
#endm


#macro kill_extra_threads()

    #if COMPILE_ONLY_DEBUG

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


    /*
     * If there are 3 packet DMA MEs in the system and if we were running
     * 8 threads, then the total number of committed slots in the
     * ToPCIELo DMA queue would be:
     *
     *    3 MEs * 8 thds/ME * 3 pkt blks/thd * 2 DMAs/ptk blk = 144 DMAs
     *
     * This is more than the 128 DMA slots in the low priority queue.  So
     * instead we use 7 threads per ME giving us:
     *
     *    3 MEs * 7 thds/ME * 3 pkt blks/thd * 2 DMAs/ptk blk = 126 DMAs
     *
     * We lose some small amount of hardware supported latency hiding
     * ability per thread in exchange for more latency hiding overall and
     * much more CPU power available.
     */
    #ifdef NFD_OUT_3_PD_MES
        .if (ctx() == 7)
            ctx_arb[kill]
        .endif
    #endif /* NFD_OUT_3_PD_MES */

#endm


main#:

    /*
     * DECLARATIONS
     */
    pci_out_sb_iface_declare()

    // Global per-context constants
    .reg volatile g_dma_word0_mask
    .reg volatile g_dma_word1_vals
    .reg volatile g_dma_word3_vals
    .reg volatile g_bitmap_base
    .reg volatile g_seq_mask
    .reg volatile g_pkt_num_mask
    .reg volatile g_pkt_free_hi
    .reg volatile g_send_cntrs_addr_hi
    .reg volatile g_blm_addr_hi
    .reg volatile g_blm_iref
    .reg volatile g_add_imm_iref
    .reg volatile g_in_wq_hi
    .reg volatile g_in_wq_lo
    .reg volatile g_pcie_addr_lo
    .reg volatile g_pcie_addr_hi
    .reg volatile g_dma_max
    .reg volatile g_num_ticket_errors

    .reg @ndequeued
    .init @ndequeued SB_WQ_CREDIT_BATCH

    // XFER and signals for work/DMA block 0
    .reg read $work_in0[SB_WQ_SIZE_LW]
    .xfer_order $work_in0
    .reg write $dma_out0[PCIE_DMA_SIZE_LW]
    .xfer_order $dma_out0
    .reg write $dma_out0x[PCIE_DMA_SIZE_LW]
    .xfer_order $dma_out0x
    .sig volatile work_sig0
    .sig volatile dma_sig0

    // XFER and signals for work/DMA block 1
    .reg $work_in1[SB_WQ_SIZE_LW]
    .xfer_order $work_in1
    .reg $dma_out1[PCIE_DMA_SIZE_LW]
    .xfer_order $dma_out1
    .reg $dma_out1x[PCIE_DMA_SIZE_LW]
    .xfer_order $dma_out1x
    .sig volatile work_sig1
    .sig volatile dma_sig1

    // XFER and signals for work/DMA block 2
    .reg $work_in2[SB_WQ_SIZE_LW]
    .xfer_order $work_in2
    .reg $dma_out2[PCIE_DMA_SIZE_LW]
    .xfer_order $dma_out2
    .reg $dma_out2x[PCIE_DMA_SIZE_LW]
    .xfer_order $dma_out2x
    .sig volatile work_sig2
    .sig volatile dma_sig2

    // General GPRs for initialization
    .reg tmp

    /*
     * Global initialization
     */
    .if (ctx() == 0)
        local_csr_wr[MAILBOX0, 0]
        local_csr_wr[MAILBOX1, 0]
        local_csr_wr[MAILBOX2, 0]
        local_csr_wr[MAILBOX3, 0]
    .endif

    /*
     * PER CONTEXT INITIALIZATION
     */

    pci_out_sb_iface_init()

    /*
     * Maps SB_WQ_PKT_NUM and SB_WQ_OFFSET into CPP address low bits.
     * These fields are at the right place so we only need to mask out
     * certain bits and OR in bit 31 to get a packet-addressing mode
     * address.
     */
    move(g_dma_word0_mask, 0x01FF1FFF)

    /*
     * Values to OR into word 1 of a DMA descriptor
     */
    /* XXX cache_desc.c configures the NFD_OUT_DATA_CFG_REG DMA register
     * on behalf of the pci_out_pd.uc MEs.  The PD MEs are not allowed
     * to touch the PCIe island until they receive work from SB, which
     * guarantees that the PCIe link must be clocked and is safe to
     * access.
     */
    /* Valid only for CTM addresses! */
    move(g_dma_word1_vals, 0x80)
    sm_set_noclr(g_dma_word1_vals, PCIE_DMA_CFG_IDX, NFD_OUT_DATA_CFG_REG)
    sm_set_noclr(g_dma_word1_vals,  PCIE_DMA_TOKEN, NFD_OUT_DATA_DMA_TOKEN)
    local_csr_rd[ACTIVE_CTX_STS]
    immed[tmp, 0]
    alu[tmp, tmp, AND, 7]
    sm_set_noclr(g_dma_word1_vals,  PCIE_DMA_SIGCTX, tmp)
    move(tmp, __ISLAND)
    sm_set_noclr(g_dma_word1_vals,  PCIE_DMA_MASTER_ISL, tmp)
    move(tmp, (__MEID & 0xf))
    sm_set_noclr(g_dma_word1_vals,  PCIE_DMA_MASTER_ID, tmp)

    /*
     * Fixed bits to OR into word 3 of a DMA descriptor
     */
    move(g_dma_word3_vals, ((0 << PCIE_DMA_XCLASS_shf) |
                            (1 << PCIE_DMA_OVDRID_shf)))

    // Base address in local island of release bitmap
    move(g_bitmap_base, nfd_out_sb_release/**/PCIE_ISL)

    // Mask of sequence number in incoming descriptor
    move(g_seq_mask, SB_WQ_SEQ_msk)

    // Mask of packet number in the incoming descriptor
    move(g_pkt_num_mask, SB_WQ_PKT_NUM_msk)

    // Used to quickly construct the packet free address
    // Sets "direct access mode" bit in MU address.
    move(g_pkt_free_hi, 0x80000000)

    // Base address of send_desc release counters
    move(g_send_cntrs_addr_hi, (nfd_out_atomics/**/PCIE_ISL >> 8))

    // (1 << 3) == OVE_DATA == 1 -> override full dataref with iref[31:16]
    move(g_blm_iref, (1 << 3))
    move(tmp, NFD_OUT_BLM_POOL_START)
    alu[g_blm_iref, g_blm_iref, OR, tmp, <<16]

    // XXX BLM really needs an interface to speed this up for
    // cycle tight access.
    move(g_blm_addr_hi, ((NFD_OUT_BLM_RADDR_UC >> 8) & 0xFF000000))

    /*
     * Indirect reference to perform 32-bit add_imm from
     * the data16 field of an indirect reference.
     *
     * override length  = (1 << 7)
     * override dataref = (2 << 3)
     * length[2] = 0 for 32-bit operations = (0 << 10)
     * length[3] = 1 for to pull operand from dataref = (1 << 11)
     */
    move(g_add_imm_iref, ((2 << 3) | (1 << 7) | (0 << 10) | (1 << 11)))


    move(g_pcie_addr_lo, NFP_PCIE_DMA_TOPCI_LO)
    move(g_pcie_addr_hi, (PCIE_ISL << 30))
    move(g_dma_max, PCIE_DMA_MAX_LEN)
    move(g_num_ticket_errors, 0)



    /*
     * STATE MACHINE
     *
     * (main loop)
     */

    kill_extra_threads()

#if ONE_PKT_AT_A_TIME

    kickstart#:
        pci_out_pd_request_work($work_in0[0], work_sig0)
        ctx_arb[work_sig0]

    one_packet_issue_dma#:
        issue_packet_dma(0, one_packet_complete_dma#, dma_sig0)
    one_packet_complete_dma#:
        complete_packet_dma(0, one_packet_issue_dma#, work_sig0)

#else /* ONE_PACKET_AT_A_TIME */

    kickstart#:
        pci_out_pd_request_work($work_in0[0], work_sig0)
        pci_out_pd_request_work($work_in1[0], work_sig1)
        pci_out_pd_request_work($work_in2[0], work_sig2)
        ctx_arb[work_sig0], br[state_e0n0#]

    /* ------------------------------ COLUMN #1 ------------------------------ */

    state_e0n0#:
        // No demux: fall through

    transition_e0n0_e1n1#:
        issue_packet_dma(0, state_e1n1#, work_sig1, dma_sig0)

    state_e1n1#:
        br_signal[dma_sig0, transition_e1n1_e0n1#]
        br_!signal[work_sig1, unreached#]
        // fall through

    transition_e1n1_e2n2#:
        issue_packet_dma(1, state_e2n2#, work_sig2, dma_sig0)

    state_e2n2#:
        br_signal[dma_sig0, transition_e2n2_e1n2#]
        br_!signal[work_sig2, unreached#]
        // fall through

    transition_e2n2_e3n0#:
        issue_packet_dma(2, state_e3n0#, dma_sig0)

    state_e3n0#:
        // No demux: fall through

    transition_e3n0_e2n0#:
        complete_packet_dma(0, state_e2n0#, work_sig0, dma_sig1)

    transition_e1n1_e0n1#:
        complete_packet_dma(0, state_e0n1#, work_sig1)

    transition_e2n2_e1n2#:
        complete_packet_dma(0, state_e1n2#, work_sig2, dma_sig1)


    /* ------------------------------ COLUMN #2 ------------------------------ */

    state_e0n1#:
        // No demux: fall through

    transition_e0n1_e1n2#:
        issue_packet_dma(1, state_e1n2#, work_sig2, dma_sig1)

    state_e1n2#:
        br_signal[dma_sig1, transition_e1n2_e0n2#]
        br_!signal[work_sig2, unreached#]
        // fall through

    transition_e1n2_e2n0#:
        issue_packet_dma(2, state_e2n0#, work_sig0, dma_sig1)

    state_e2n0#:
        br_signal[dma_sig1, transition_e2n0_e1n0#]
        br_!signal[work_sig0, unreached#]
        // fall through

    transition_e2n0_e3n1#:
        issue_packet_dma(0, state_e3n1#, dma_sig1)

    state_e3n1#:
        // No demux: fall through

    transition_e3n1_e2n1#:
        complete_packet_dma(1, state_e2n1#, work_sig1, dma_sig2)

    transition_e1n2_e0n2#:
        complete_packet_dma(1, state_e0n2#, work_sig2)

    transition_e2n0_e1n0#:
        complete_packet_dma(1, state_e1n0#, work_sig0, dma_sig2)


    /* ------------------------------ COLUMN #3 ------------------------------ */

    state_e0n2#:
        // No demux: fall through

    transition_e0n2_e1n0#:
        issue_packet_dma(2, state_e1n0#, work_sig0, dma_sig2)

    state_e1n0#:
        br_signal[dma_sig2, transition_e1n0_e0n0#]
        br_!signal[work_sig0, unreached#]
        // fall through

    transition_e1n0_e2n1#:
        issue_packet_dma(0, state_e2n1#, work_sig1, dma_sig2)

    state_e2n1#:
        br_signal[dma_sig2, transition_e2n1_e1n1#]
        br_!signal[work_sig1, unreached#]
        // fall through

    transition_e2n1_e3n2#:
        issue_packet_dma(1, state_e3n2#, dma_sig2)

    state_e3n2#:
        // No demux: fall through

    transition_e3n2_e2n2#:
        complete_packet_dma(2, state_e2n2#, work_sig2, dma_sig0)

    transition_e1n0_e0n0#:
        complete_packet_dma(2, state_e0n0#, work_sig0)

    transition_e2n1_e1n1#:
        complete_packet_dma(2, state_e1n1#, work_sig1, dma_sig0)

    unreached#:
        ctx_arb[bpt], br[unreached#]

#endif /* ONE_PACKET_AT_A_TIME */


#endif /* __PCI_OUT_PD_UC */
