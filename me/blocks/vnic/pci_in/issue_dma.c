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
 * @file          blocks/vnic/pci_in/issue_dma.c
 * @brief         Code to DMA packet data to the NFP
 */

#include <assert.h>
#include <nfp.h>
#include <nfp_chipres.h>
#include <types.h>

#include <nfp/cls.h>
#include <nfp/me.h>
#include <nfp/mem_pe.h>
#include <nfp/pcie.h>
#include <std/reg_utils.h>
#include <std/cntrs.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_in.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/shared/nfd_internal.h>

#include <vnic/utils/cls_ring.h>
#include <vnic/utils/ctm_ring.h>
#include <vnic/utils/ordering.h>
#include <vnic/utils/qc.h>

#ifndef _link_sym
#define _link_sym(x) __link_sym(#x)
#endif

/* TODO Make this test cover precache_bufs.c as well */
#ifndef PCI_IN_ISSUE_DMA_IDX
#warning "PCI_IN_ISSUE_DMA_IDX not defined.  Defaulting to 0.  Make sure there is only one instance"
#define PCI_IN_ISSUE_DMA_IDX 0
#endif

#define NFD_IN_Q_STATE_PTR *l$index2

#if (PCI_IN_ISSUE_DMA_IDX == 0)
#define NFD_IN_DATA_EVENT_FILTER NFD_IN_DATA0_EVENT_FILTER
#define NFD_IN_JUMBO_EVENT_FILTER NFD_IN_JUMBO0_EVENT_FILTER
#else
#define NFD_IN_DATA_EVENT_FILTER NFD_IN_DATA1_EVENT_FILTER
#define NFD_IN_JUMBO_EVENT_FILTER NFD_IN_JUMBO1_EVENT_FILTER
#endif

struct _tx_desc_batch {
    struct nfd_in_tx_desc pkt0;
    struct nfd_in_tx_desc pkt1;
    struct nfd_in_tx_desc pkt2;
    struct nfd_in_tx_desc pkt3;
    struct nfd_in_tx_desc pkt4;
    struct nfd_in_tx_desc pkt5;
    struct nfd_in_tx_desc pkt6;
    struct nfd_in_tx_desc pkt7;
};

struct _issued_pkt_batch {
    struct nfd_in_issued_desc pkt0;
    struct nfd_in_issued_desc pkt1;
    struct nfd_in_issued_desc pkt2;
    struct nfd_in_issued_desc pkt3;
    struct nfd_in_issued_desc pkt4;
    struct nfd_in_issued_desc pkt5;
    struct nfd_in_issued_desc pkt6;
    struct nfd_in_issued_desc pkt7;
};

struct _dma_desc_batch {
    struct nfp_pcie_dma_cmd pkt0;
    struct nfp_pcie_dma_cmd pkt1;
    struct nfp_pcie_dma_cmd pkt2;
    struct nfp_pcie_dma_cmd pkt3;
    struct nfp_pcie_dma_cmd pkt4;
    struct nfp_pcie_dma_cmd pkt5;
    struct nfp_pcie_dma_cmd pkt6;
    struct nfp_pcie_dma_cmd pkt7;
};

NFD_BLM_Q_ALLOC(NFD_IN_BLM_REG_POOL);
NFD_BLM_Q_ALLOC(NFD_IN_BLM_JUMBO_POOL);

#define NFD_IN_DESC_RING_SZ (NFD_IN_MAX_BATCH_SZ * NFD_IN_DESC_BATCH_Q_SZ * \
                      sizeof(struct nfd_in_tx_desc))
static __shared __gpr unsigned int desc_ring_base;


__xread struct _tx_desc_batch tx_desc;
__gpr struct nfd_in_issued_desc issued_tmp;

/* Storage declarations */
__shared __lmem struct nfd_in_dma_state queue_data[NFD_IN_MAX_QUEUES];

#ifdef NFD_IN_LSO_CNTR_ENABLE
static unsigned int nfd_in_lso_cntr_addr = 0;
#endif
/* storage for LSO header on a per queue basis */
__export __shared __ctm40 __align(NFD_IN_MAX_LSO_HDR_SZ) unsigned char
    lso_hdr_data[NFD_IN_MAX_LSO_HDR_SZ * NFD_IN_MAX_QUEUES];

static __shared __gpr unsigned int lso_hdr_data_base;

/* Sequence number declarations */
__shared __gpr unsigned int gather_dma_seq_compl = 0;
__shared __gpr unsigned int gather_dma_seq_serv = 0;

__shared __gpr unsigned int data_dma_seq_issued = 0;
extern __shared __gpr unsigned int data_dma_seq_safe;

__shared __gpr unsigned int jumbo_dma_seq_issued = 0;
__shared __gpr unsigned int jumbo_dma_seq_compl = 0;


/* Ring declarations */
/* TODO: use generic resource management to sanity check these rings */
#if (PCI_IN_ISSUE_DMA_IDX == 0)

__export __shared __cls __align(NFD_IN_DESC_RING_SZ) struct nfd_in_tx_desc
    desc_ring0[NFD_IN_MAX_BATCH_SZ * NFD_IN_DESC_BATCH_Q_SZ];

__export __ctm40
    __align(sizeof(struct nfd_in_issued_desc) * NFD_IN_ISSUED_RING0_SZ)
    struct nfd_in_issued_desc nfd_in_issued_ring0[NFD_IN_ISSUED_RING0_SZ];

/* Signals and transfer registers for managing
 * gather_dma_seq_compl updates */
__visible volatile __xread unsigned int nfd_in_gather_compl_refl_in0;
__visible volatile SIGNAL nfd_in_gather_compl_refl_sig0;

#define nfd_in_gather_compl_refl_in nfd_in_gather_compl_refl_in0
#define nfd_in_gather_compl_refl_sig nfd_in_gather_compl_refl_sig0
#define desc_ring desc_ring0
#define nfd_in_issued_ring nfd_in_issued_ring0
#define NFD_IN_ISSUED_RING_NUM NFD_IN_ISSUED_RING0_NUM
#define NFD_IN_ISSUED_LSO_RING_NUM NFD_IN_ISSUED_LSO_RING0_NUM
#define NFD_IN_ISSUED_LSO_RING_SZ NFD_IN_ISSUED_LSO_RING0_SZ

/*
 * Reserve PCIe Resources for DMA Queues
 */
PCIE_DMA_ALLOC(nfd_in_data_dma, me, PCIE_ISL, frompci_lo,
               NFD_IN_DATA_MAX_IN_FLIGHT);
PCIE_DMA_ALLOC(nfd_in_data_jumbo_dma, me, PCIE_ISL, frompci_lo,
               NFD_IN_JUMBO_MAX_IN_FLIGHT);

#elif (PCI_IN_ISSUE_DMA_IDX == 1)

__export __shared __cls __align(NFD_IN_DESC_RING_SZ) struct nfd_in_tx_desc
    desc_ring1[NFD_IN_MAX_BATCH_SZ * NFD_IN_DESC_BATCH_Q_SZ];

__export __ctm40
    __align(sizeof(struct nfd_in_issued_desc) * NFD_IN_ISSUED_RING1_SZ)
    struct nfd_in_issued_desc nfd_in_issued_ring1[NFD_IN_ISSUED_RING1_SZ];

/* Signals and transfer registers for managing
 * gather_dma_seq_compl updates */
__visible volatile __xread unsigned int nfd_in_gather_compl_refl_in1;
__visible volatile SIGNAL nfd_in_gather_compl_refl_sig1;

#define nfd_in_gather_compl_refl_in nfd_in_gather_compl_refl_in1
#define nfd_in_gather_compl_refl_sig nfd_in_gather_compl_refl_sig1
#define desc_ring desc_ring1
#define nfd_in_issued_ring nfd_in_issued_ring1
#define NFD_IN_ISSUED_RING_NUM NFD_IN_ISSUED_RING1_NUM
#define NFD_IN_ISSUED_LSO_RING_NUM NFD_IN_ISSUED_LSO_RING1_NUM
#define NFD_IN_ISSUED_LSO_RING_SZ NFD_IN_ISSUED_LSO_RING1_SZ

/*
 * Reserve PCIe Resources for DMA Queues
 */
PCIE_DMA_ALLOC(nfd_in_data_dma, me, PCIE_ISL, frompci_lo,
               NFD_IN_DATA_MAX_IN_FLIGHT);
PCIE_DMA_ALLOC(nfd_in_data_jumbo_dma, me, PCIE_ISL, frompci_lo,
               NFD_IN_JUMBO_MAX_IN_FLIGHT);

#else /* invalid PCI_IN_ISSUE_DMA_IDX */

#error "Invalid PCI_IN_ISSUE_DMA_IDX.  Must be 0 or 1."

#endif

#define NFD_IN_ISSUED_LSO_RING_INIT_IND2(_isl, _emem, _num)               \
    _NFP_CHIPRES_ASM(.alloc_mem nfd_in_issued_lso_ring_mem##_isl##_num    \
                     _emem global                                         \
                     NFD_IN_ISSUED_LSO_RING_SZ NFD_IN_ISSUED_LSO_RING_SZ) \
    _NFP_CHIPRES_ASM(.init_mu_ring nfd_in_issued_lso_ring_num##_isl##_num \
                     nfd_in_issued_lso_ring_mem##_isl##_num)
#define NFD_IN_ISSUED_LSO_RING_INIT_IND1(_isl, _emem, _num)     \
    NFD_IN_ISSUED_LSO_RING_INIT_IND2(_isl, _emem, _num)
#define NFD_IN_ISSUED_LSO_RING_INIT_IND0(_isl, _num)                    \
    NFD_IN_ISSUED_LSO_RING_INIT_IND1(_isl, NFD_PCIE##_isl##_EMEM, _num)
#define NFD_IN_ISSUED_LSO_RING_INIT(_isl, _num)         \
    NFD_IN_ISSUED_LSO_RING_INIT_IND0(_isl, _num)

NFD_IN_ISSUED_LSO_RING_INIT(PCIE_ISL, NFD_IN_ISSUED_LSO_RING_NUM);

#define NFD_IN_ISSUED_LSO_RING_ADDR_IND(_isl, _num)     \
    _link_sym(nfd_in_issued_lso_ring_mem##_isl##_num)
#define NFD_IN_ISSUED_LSO_RING_ADDR(_isl, _num) \
    NFD_IN_ISSUED_LSO_RING_ADDR_IND(_isl, _num)

static __gpr mem_ring_addr_t nfd_in_issued_lso_ring_addr = 0;
static __gpr unsigned int nfd_in_issued_lso_ring_num = 0;

/* DMA descriptor template */
static __gpr unsigned int cpp_hi_no_sig_part;
static __gpr unsigned int cpp_hi_event_part;


/* Output transfer registers */
static __xwrite struct _dma_desc_batch dma_out;
static __xwrite struct _issued_pkt_batch batch_out;

/* Signalling */
static SIGNAL tx_desc_sig, msg_sig0, desc_order_sig, dma_order_sig;
static SIGNAL last_of_batch_dma_sig;
static SIGNAL msg_sig1;
static SIGNAL batch_sig;
static SIGNAL_MASK wait_msk;

unsigned int next_ctx;

/* CLS ring of batch information from the gather() block */
_NFP_CHIPRES_ASM(.alloc_resource nfd_in_batch_ring0_num \
                 cls_rings+NFD_IN_BATCH_RING0_NUM island 1 1);
_NFP_CHIPRES_ASM(.alloc_resource nfd_in_batch_ring1_num \
                 cls_rings+NFD_IN_BATCH_RING1_NUM island 1 1);


/*
 * Reserve PCIe Resources for DMA Configuration Registers
 */
PCIE_DMA_CFG_ALLOC_OFF(nfd_in_data_dma_cfg, island, PCIE_ISL,
                       NFD_IN_DATA_CFG_REG, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_in_data_sig_only_dma_cfg, island, PCIE_ISL,
                       NFD_IN_DATA_CFG_REG_SIG_ONLY, 1);


/* Enable B0 DMA ByteMask swapping to ensure that DMAs with the byte
 * swap token complete correctly for DMAs that aren't 4B multiples in size. */
void
_issue_dma_enable_DmaByteMaskSwap(unsigned char pcie_isl)
{
/* XXX nfp6000/nfp_pcie.h should provide this */
#define NFP_PCIE_DMA_DBG_REG_0          0x400f0

    __xwrite unsigned int data = 0x80000000;
    __gpr unsigned int addr_hi = pcie_isl << 30;
    unsigned int dma_dbg_reg_0_addr = NFP_PCIE_DMA_DBG_REG_0;
    SIGNAL sig;

    __asm pcie[write_pci, data, addr_hi, <<8, dma_dbg_reg_0_addr, 1], \
        ctx_swap[sig]
}


/**
 * Test a bit in the queue_data struct
 */
__intrinsic int
issue_dma_queue_state_bit_set_test(int bit_num)
{
    int result = 1;

    ctassert(__is_ct_const(bit_num));

    __asm {
        br_bset[NFD_IN_Q_STATE_PTR[0], __ct_const_val(bit_num), match];
        alu[result, --, B, 0];
        match:
    }

    return result;
}


/**
 * Test whether the queue state indicates an LSO packet in process
 */
__intrinsic int
issue_dma_queue_state_proc_lso()
{
    int result = 1;
    unsigned int bit_num;

    bit_num = (NFD_IN_DMA_STATE_FLAGS_shf + __log2(PCIE_DESC_TX_LSO));
    __asm {
        br_bset[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_FLAGS_wrd], \
                __ct_const_val(bit_num, match];
        alu[result, --, B, 0];
        match:
    }

    return result;
}


/**
 * Add a length to a PCIe address, with carry to PCIe HI
 * @param pcie_hi_word     hi part of the address to be updated
 * @param pcie_lo_word     lo part of the address to be updated
 * @param dma_len          length to add to the address
 */
__intrinsic void
_add_to_pcie_addr(unsigned int *pcie_hi_word, unsigned int *pcie_addr_lo,
                  unsigned int dma_len)
{
    /* We need to use the +carry op to update the 8bit PCIe HI value.
     * This field is in the low 8bits of pcie_hi_word.  Therefore we
     * use inline asm. */
    __asm { alu[*pcie_addr_lo, *pcie_addr_lo, +, dma_len] }
    __asm { alu[*pcie_hi_word, *pcie_hi_word, +carry, 0] }
}


/**
 * Perform shared configuration for issue_dma
 */
void
issue_dma_setup_shared()
{
    struct nfp_pcie_dma_cfg cfg_tmp;
    __xwrite struct nfp_pcie_dma_cfg cfg;
    __xwrite uint32_t lso_hdr_data_init_xw = 0xDEADBEEF;
    __gpr uint32_t i;

    ctm_ring_setup(NFD_IN_ISSUED_RING_NUM, nfd_in_issued_ring,
                   sizeof nfd_in_issued_ring);

/* Enable B0 DMA ByteMask swapping to ensure that DMAs with the byte
 * swap token complete correctly for DMAs that aren't 4B multiples in size. */
#if __REVISION_MIN >= __REVISION_B0
    _issue_dma_enable_DmaByteMaskSwap(PCIE_ISL);
#endif


    /*
     * Initialise the CLS TX descriptor ring
     */
    desc_ring_base = ((unsigned int) &desc_ring) & 0xFFFFFFFF;

    /* Initialize the CTM LSO header data */
    lso_hdr_data_base = ((unsigned int) &lso_hdr_data) & 0xFFFFFFFF;
    for (i = 0; i < ((NFD_IN_MAX_LSO_HDR_SZ >> 2) * NFD_IN_MAX_QUEUES); i++) {
        lso_hdr_data_init_xw = 0xDEADBEEF;
        mem_write32(&lso_hdr_data_init_xw,
                    (__mem40 void *)&lso_hdr_data[i * 4],
                    sizeof(lso_hdr_data_init_xw));
    }

    /*
     * Setup the DMA configuration registers
     * XXX PCI.IN and PCI.OUT use the same settings,
     * could share configuration registers.
     */
    cfg_tmp.__raw = 0;
    /* Signal only configuration for null messages */
    cfg_tmp.signal_only_odd = 1;
    cfg_tmp.target_64_odd = 1;
    cfg_tmp.cpp_target_odd = 7;
    /* Regular configuration */
#ifdef NFD_VNIC_NO_HOST
    /* Use signal_only for seqn num generation
     * Don't actually DMA data */
    cfg_tmp.signal_only_even = 1;
#else
    cfg_tmp.signal_only_even = 0;
#endif
    cfg_tmp.end_pad_even = 0;
    cfg_tmp.start_pad_even = 0;
    cfg_tmp.target_64_even = 1;
    cfg_tmp.cpp_target_even = 7;
    cfg = cfg_tmp;

    pcie_dma_cfg_set_pair(PCIE_ISL, NFD_IN_DATA_CFG_REG, &cfg);

    /* Kick off ordering */
    reorder_start(NFD_IN_ISSUE_START_CTX, &desc_order_sig);
    reorder_start(NFD_IN_ISSUE_START_CTX, &dma_order_sig);
}


/**
 * Setup PCI.IN configuration for the vNIC specified in cfg_msg
 * @param cfg_msg   Standard configuration message
 *
 * This method handles all PCI.IN configuration related to bringing a vNIC up
 * or down on the "issue_dma" ME.
 */
__intrinsic void
issue_dma_vnic_setup(struct nfd_cfg_msg *cfg_msg)
{
    unsigned int queue;
    unsigned int bmsk_queue;
    struct nfd_cfg_msg cfg_msg_cp;

    ctassert(__is_log2(NFD_MAX_VF_QUEUES));
    ctassert(__is_log2(NFD_MAX_PF_QUEUES));

    /* save aside cfg_msg in case we need to revert the queue processing */
    cfg_msg_cp = *cfg_msg;

    nfd_cfg_next_queue(cfg_msg, &queue);

    if (cfg_msg->error || !cfg_msg->interested) {
        return;
    }

    bmsk_queue = NFD_VID2QID(cfg_msg->vid, queue);

    if (queue_data[bmsk_queue].locked) {
        /* The queue is locked by the worker contexts so
         * we can't change it's configuration.  Revert the
         * queue selection process so that the queue will be
         * selected again on the next round. */
        *cfg_msg = cfg_msg_cp;

    } else if (cfg_msg->up_bit && !queue_data[bmsk_queue].up) {
        /* Initialise queue state */
        queue_data[bmsk_queue].sp0 = 0;
        queue_data[bmsk_queue].lso_offhdr = 0;
        queue_data[bmsk_queue].lso_seq_cnt = 0;
        queue_data[bmsk_queue].rid = 0;
        if (NFD_VID_IS_VF(cfg_msg->vid)) {
            queue_data[bmsk_queue].rid = cfg_msg->vid + NFD_CFG_VF_OFFSET;
        }
        queue_data[bmsk_queue].cont = 0;
        queue_data[bmsk_queue].up = 1;
        queue_data[bmsk_queue].jumbo = 0;
        queue_data[bmsk_queue].sp1 = 0;
        queue_data[bmsk_queue].curr_buf = 0;
        queue_data[bmsk_queue].__raw[2] = 0;
        queue_data[bmsk_queue].__raw[3] = 0;
        queue_data[bmsk_queue].__raw[4] = 0;
        queue_data[bmsk_queue].__raw[5] = 0;
        queue_data[bmsk_queue].__raw[6] = 0;
        queue_data[bmsk_queue].__raw[7] = 0;

    } else if (!cfg_msg->up_bit && queue_data[bmsk_queue].up) {
        /* Free the MU buffer */
        if (queue_data[bmsk_queue].curr_buf != 0) {
            unsigned int blm_raddr;
            unsigned int blm_rnum;

            /* XXX possibly move BLM constants to GPRs
             * if some are available */
            blm_raddr = (((unsigned long long) NFD_IN_BLM_RADDR >> 8) &
                         0xff000000);
            if (queue_data[bmsk_queue].jumbo) {
                blm_rnum = NFD_BLM_Q_LINK(NFD_IN_BLM_JUMBO_POOL);
            } else {
                blm_rnum = NFD_BLM_Q_LINK(NFD_IN_BLM_REG_POOL);
            }
            mem_ring_journal_fast(blm_rnum, blm_raddr,
                                  queue_data[bmsk_queue].curr_buf);
        }

        /* Clear queue state */
        queue_data[bmsk_queue].sp0 = 0;
        queue_data[bmsk_queue].lso_offhdr = 0;
        queue_data[bmsk_queue].lso_seq_cnt = 0;
        /* Leave RID configured after first set */
        /* "cont" is used as part of the "up" signalling,
         * to move the "up" test off the fast path. */
        queue_data[bmsk_queue].cont = 1;
        queue_data[bmsk_queue].up = 0;
        queue_data[bmsk_queue].jumbo = 0;
        queue_data[bmsk_queue].sp1 = 0;
        queue_data[bmsk_queue].curr_buf = 0;

        queue_data[bmsk_queue].__raw[2] = 0;
        queue_data[bmsk_queue].__raw[3] = 0;
        queue_data[bmsk_queue].__raw[4] = 0;
        queue_data[bmsk_queue].__raw[5] = 0;
        queue_data[bmsk_queue].__raw[6] = 0;
        queue_data[bmsk_queue].__raw[7] = 0;

    }
}


/**
 * Perform per context initialisation (for CTX 1 to 7)
 */
void
issue_dma_setup()
{
    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, PCIe address,
     * and dma_mode will be overwritten per transaction.
     */

    /* Prepare partial descriptors for the DMA CPP hi word */
    cpp_hi_no_sig_part = (NFP_PCIE_DMA_CMD_MODE_SEL(0) |
                          NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN) |
                          NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG));
    cpp_hi_event_part = dma_seqn_init_event(NFD_IN_DATA_EVENT_TYPE,
                                            PCI_IN_ISSUE_DMA_IDX);
    cpp_hi_event_part |= (NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN) |
                          NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG));

    /* wait_msk initially only needs batch_sig, tx_desc_sig and dma_order_sig
     * No DMAs or messages have been issued at this stage */
    wait_msk = __signals(&batch_sig, &tx_desc_sig, &dma_order_sig);
    next_ctx = reorder_get_next_ctx(NFD_IN_ISSUE_START_CTX,
                                    NFD_IN_ISSUE_END_CTX);

#ifdef NFD_IN_LSO_CNTR_ENABLE
    nfd_in_lso_cntr_addr =
        cntr64_get_addr((__mem40 void *) nfd_in_lso_cntrs);
#endif
    nfd_in_issued_lso_ring_num = NFD_RING_LINK(PCIE_ISL, nfd_in_issued_lso,
                                               NFD_IN_ISSUED_LSO_RING_NUM);
    nfd_in_issued_lso_ring_addr = ((((unsigned long long)
                                      NFD_EMEM_LINK(PCIE_ISL)) >> 32) << 24);
}


/**
 * Check for gather_dma_seq_compl updates
 *
 * The gather ME tracks gather DMA completions and reflects the
 * full sequence number to this ME.  The value must be copied from
 * transfer registers to shared GPRs for the worker threads.  This
 * function runs on CTX0 only.
 */
__intrinsic void
issue_dma_gather_seq_recv()
{
    if (signal_test(&nfd_in_gather_compl_refl_sig)) {
        gather_dma_seq_compl = nfd_in_gather_compl_refl_in;
    }
}


/* XXX temporarily enable _ISSUE_PROC_MU_CHK even without debug checks.
 * This gives us extra protection of the CTM counters while PCI.OUT is not
 * double checking credits. */
#ifndef NFD_MU_PTR_DBG_MSK
#define NFD_MU_PTR_DBG_MSK 0x1f000000
#endif

/* performance hit if NFD_IN_ISSUE_DMA_DBG_CHKS is enabled in makefile.
 * The MU_PTR check done in notify by default which hides the cost of the
 * check. */
#ifdef NFD_IN_ISSUE_DMA_DBG_CHKS
#define _ISSUE_PROC_MU_CHK(_val)                                        \
    if ((_val & NFD_MU_PTR_DBG_MSK) == 0) {                             \
        /* Write the error we read to Mailboxes for debug purposes */   \
        local_csr_write(local_csr_mailbox_0,                            \
                        NFD_IN_ISSUE_DMA_MU_PTR_INVALID);               \
        local_csr_write(local_csr_mailbox_1, _val);                     \
                                                                        \
        halt();                                                         \
    }
#else
#define _ISSUE_PROC_MU_CHK(_val)
#endif


#ifdef NFD_IN_LSO_CNTR_ENABLE
#define _LSO_TX_DESC_TYPE_CNTR(_pkt)                                \
    if (tx_desc.pkt##_pkt##.eop) {                                  \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                  \
                          NFD_IN_LSO_CNTR_T_ISSUED_LSO_EOP_TX_DESC);\
    } else {                                                        \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                  \
                         NFD_IN_LSO_CNTR_T_ISSUED_LSO_CONT_TX_DESC);\
    }
#else
#define _LSO_TX_DESC_TYPE_CNTR(_pkt)
#endif


/**
 * Clean up LM queue LSO state
 * If an LSO gather series is accidentally or maliciously truncated,
 * we may see the LSO state in the regular gather DMA branch.  If this
 * happens, we need to clean up the queue state.
 */
__noinline void
issue_dma_cleanup_lso_state()
{
    unsigned int curr_buf;
    /* XXX leave CONT bit untouched, it gets cleared on EOP descriptors */

    /* clear LSO specific state */
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd],    \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd],    \
                AND~, NFD_IN_DMA_STATE_LSO_OFFHDR_msk,                  \
                <<NFD_IN_DMA_STATE_LSO_OFFHDR_shf] }
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_SEQ_CNT_wrd],   \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_SEQ_CNT_wrd],   \
                AND~, NFD_IN_DMA_STATE_LSO_SEQ_CNT_msk,                 \
                <<NFD_IN_DMA_STATE_LSO_SEQ_CNT_shf] }
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_FLAGS_wrd],         \
                --, b, 0] }
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_PAYLOAD_wrd],   \
                --, B, 0] }
    /* clear state shared with gather code */
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd],   \
                --, b, 0] }
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_OFFSET_wrd],        \
                --, B, 0] }

    /* ensure that we have a valid MU buffer */
    /* XXX the gather code sends TX descriptors to the application
     * with the invalid bit set on EOP, and the application frees the
     * MU buffer. */
    __asm { alu[curr_buf, --, B,                                        \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd]] }
    if ((curr_buf & NFD_IN_DMA_STATE_CURR_BUF_msk) == 0) {
        /* We can use the regular buffer associated with
         * this _pkt slot in the batch. */
        curr_buf = precache_bufs_use();
    }
    _ISSUE_PROC_MU_CHK(curr_buf);
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd],      \
                --, B, curr_buf] }
}


/* This macro issues a DMA for a NFD_IN_DMA_SPLIT_LEN chunk of the
 * current packet. It updates the addresses and remaining dma_len before
 * exiting.
 *
 * DMAs are tracked from a separate sequence space (jumbo_dma_seq_issued
 * and jumbo_dma_seq_compl).
 */
#define _ISSUE_PROC_JUMBO(_pkt, _buf)                                   \
do {                                                                    \
    int jumbo_seq_test;                                                 \
                                                                        \
    /* data_dma_seq_issued was pre-incremented once we could */         \
    /* process batch.  Since we are going to swap, we */                \
    /* decrement it temporarily to ensure */                            \
    /* precache_bufs_compute_seq_safe will give a pessimistic */        \
    /* safe count. */                                                   \
    data_dma_seq_issued--;                                              \
                                                                        \
    /* Take a jumbo frame sequence number and */                        \
    /* check it is safe to use */                                       \
    jumbo_dma_seq_issued++;                                             \
                                                                        \
    /* Lock the queue state so we can swap freely */                    \
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],        \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],        \
                OR, 1, <<NFD_IN_DMA_STATE_LOCKED_shf] }                 \
                                                                        \
    jumbo_seq_test = (NFD_IN_JUMBO_MAX_IN_FLIGHT - jumbo_dma_seq_issued); \
    while ((int) (jumbo_seq_test + jumbo_dma_seq_compl) <= 0) {         \
        /* The queue state is locked so it is safe to simply swap */    \
        /* for CTX0 to advance jumbo_dma_seq_compl. */                  \
        ctx_swap();                                                     \
    }                                                                   \
                                                                        \
    dma_out.pkt##_pkt##.__raw[0] = cpp_addr_lo + NFD_IN_DATA_OFFSET;    \
                                                                        \
    cpp_hi_word = dma_seqn_init_event(NFD_IN_JUMBO_EVENT_TYPE,          \
                                      PCI_IN_ISSUE_DMA_IDX);            \
    cpp_hi_word = dma_seqn_set_seqn(cpp_hi_word, jumbo_dma_seq_issued); \
    cpp_hi_word |= NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN);   \
    cpp_hi_word |=                                                      \
        NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG);            \
    dma_out.pkt##_pkt##.__raw[1] =                                      \
        cpp_hi_word | NFP_PCIE_DMA_CMD_CPP_ADDR_HI(_buf >> 21);         \
                                                                        \
    dma_out.pkt##_pkt##.__raw[2] = pcie_addr_lo;                        \
    dma_out.pkt##_pkt##.__raw[3] =                                      \
        pcie_hi_word | NFP_PCIE_DMA_CMD_LENGTH(NFD_IN_DMA_SPLIT_LEN - 1); \
                                                                        \
    pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt, NFD_IN_DATA_DMA_QUEUE);  \
                                                                        \
    _add_to_pcie_addr(&pcie_hi_word, &pcie_addr_lo, NFD_IN_DMA_SPLIT_LEN); \
    cpp_addr_lo += NFD_IN_DMA_SPLIT_LEN;                                \
    dma_len -= NFD_IN_DMA_SPLIT_LEN;                                    \
                                                                        \
    /* Re-increment data_dma_seq_issued and unlock the queue state */   \
    data_dma_seq_issued++;                                              \
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],        \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],        \
                AND~, 1, <<NFD_IN_DMA_STATE_LOCKED_shf] }               \
} while (0)

#define _ISSUE_PROC_LSO_JUMBO(_pkt, _buf)                                    \
do {                                                                         \
    int jumbo_seq_test;                                                      \
                                                                             \
    /* Take a jumbo frame sequence number and */                             \
    /* check it is safe to use */                                            \
    /* XXX the queue is locked before this macro is invoked, */              \
    /* in the LSO function the queue is locked during payload DMA */         \
    /* by default. */                                                        \
    jumbo_dma_seq_issued++;                                                  \
                                                                             \
    jumbo_seq_test = (NFD_IN_JUMBO_MAX_IN_FLIGHT - jumbo_dma_seq_issued);    \
    while ((int) (jumbo_seq_test + jumbo_dma_seq_compl) <= 0) {              \
        /* It is safe to simply swap for CTX0 */                             \
        /* to advance jumbo_dma_seq_compl. */                                \
        ctx_swap();                                                          \
    }                                                                        \
                                                                             \
    dma_out.pkt##_pkt##.__raw[0] = cpp_addr_lo + NFD_IN_DATA_OFFSET;         \
                                                                             \
    cpp_hi_word = dma_seqn_init_event(NFD_IN_JUMBO_EVENT_TYPE,               \
                                      PCI_IN_ISSUE_DMA_IDX);                 \
    cpp_hi_word = dma_seqn_set_seqn(cpp_hi_word, jumbo_dma_seq_issued);      \
    cpp_hi_word |= NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN);        \
    cpp_hi_word |=                                                           \
        NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG);                 \
    dma_out.pkt##_pkt##.__raw[1] =                                           \
        cpp_hi_word | NFP_PCIE_DMA_CMD_CPP_ADDR_HI(_buf >> 21);              \
    dma_out.pkt##_pkt##.__raw[2] = pcie_addr_lo;                             \
    dma_out.pkt##_pkt##.__raw[3] =                                           \
        pcie_hi_word | NFP_PCIE_DMA_CMD_LENGTH(NFD_IN_DMA_SPLIT_LEN - 1);    \
                                                                             \
    pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt, NFD_IN_DATA_DMA_QUEUE);       \
                                                                             \
    _add_to_pcie_addr(&pcie_hi_word, &pcie_addr_lo, NFD_IN_DMA_SPLIT_LEN);   \
    cpp_addr_lo += NFD_IN_DMA_SPLIT_LEN;                                     \
    dma_length -= NFD_IN_DMA_SPLIT_LEN;                                      \
} while (0)


/* These functions issue DMAs for LSO packet.
 *
 */
#define DECLARE_PROC_LSO(_pkt)                                               \
__noinline void issue_proc_lso##_pkt(unsigned int queue,                     \
                                     unsigned int pcie_hi_word_part,         \
                                     unsigned int type)                      \
{                                                                            \
    unsigned int dma_len;                                                    \
    unsigned int lso_dma_index = 0;                                          \
    unsigned int lso_issued_cnt = 0;                                         \
    int jumbo_seq_test;                                                      \
    unsigned int cpp_hi_word;                                                \
    unsigned int cpp_addr_lo;                                                \
    unsigned int pcie_hi_word;                                               \
    unsigned int pcie_addr_lo;                                               \
    __gpr unsigned int buf_addr;                                             \
    SIGNAL lso_hdr_dma_sig;                                                  \
    SIGNAL lso_hdr_enq_sig;                                                  \
    SIGNAL lso_enq_sig;                                                      \
    SIGNAL lso_hdr_sig;                                                      \
    SIGNAL lso_journal_sig;                                                  \
    __mem40 void *hdr_pkt_ptr;                                               \
    unsigned int mu_buf_left;                                                \
    unsigned int dma_left;                                                   \
    unsigned int dma_length;                                                 \
    unsigned int mode;                                                       \
    unsigned int header_to_read;                                             \
    unsigned int hdr_remainder;                                              \
    __gpr unsigned int curr_buf;                                             \
    __gpr unsigned int lso_offhdr;                                           \
    __gpr unsigned int lso_payload_len;                                      \
    unsigned int data_len;                                                   \
    unsigned int offset;                                                     \
    unsigned int lso_hdrlen;                                                 \
    unsigned int mss;                                                        \
    unsigned int lso_req_wrd;                                                \
    unsigned int lso_seq_cnt;                                                \
    unsigned int bytes_dmaed;                                                \
                                                                             \
                                                                             \
    dma_len = tx_desc.pkt##_pkt##.dma_len;                                   \
                                                                             \
    /* data_dma_seq_issued was pre-incremented once we could */              \
    /* process batch.  Since we are going to swap, we */                     \
    /* decrement it temporarily to ensure */                                 \
    /* precache_bufs_compute_seq_safe will give a pessimistic */             \
    /* safe count. */                                                        \
    data_dma_seq_issued--;                                                   \
                                                                             \
    if (issue_dma_queue_state_bit_set_test(NFD_IN_DMA_STATE_CONT_shf) == 0) { \
        unsigned int hdr_len_chk;                                            \
                                                                             \
        /* This is a new LSO packet, prepare internal state */               \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],           \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],           \
                    OR, 1, <<NFD_IN_DMA_STATE_CONT_shf] }                    \
                                                                             \
        /* Initialise continuation data */                                   \
        data_len = tx_desc.pkt##_pkt##.data_len;                             \
        offset = tx_desc.pkt##_pkt##.offset;                                 \
        if (data_len <= offset) {                                            \
            /* LSO packet size can be up to 64kB (inclusive), */             \
            /* but we only have 16 bits in the descriptor for data_len. */   \
            /* data_len = meta_len + pkt_len, where meta_len == offset */    \
            /* Hence the full data_len could be 0x10000 + offset, and */     \
            /* the extreme wrapped value would be "offset". */               \
            /* We unwrap the value by adding 64kB data_len. */               \
            data_len += (64 * 1024);                                         \
        }                                                                    \
        __asm {                                                              \
            alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_OFFSET_wrd],             \
                --, b, offset, <<NFD_IN_DMA_STATE_OFFSET_shf] }              \
        __asm {                                                              \
            alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_DATA_LEN_wrd],           \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_DATA_LEN_wrd],           \
                or, data_len, <<NFD_IN_DMA_STATE_DATA_LEN_shf] }             \
                                                                             \
        lso_hdrlen = tx_desc.pkt##_pkt##.lso_hdrlen;                         \
        mss = tx_desc.pkt##_pkt##.mss;                                       \
        lso_req_wrd = tx_desc.pkt##_pkt##.__raw[2];                          \
        /* Copy aside the LSO request parameters for consistency checks */   \
        /* Mask out the reserved bits during copy to avoid false positives */ \
        __asm {                                                              \
            alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_FLAGS_wrd],              \
                lso_req_wrd, and~,                                           \
                NFD_IN_DMA_STATE_LSO_RES_msk,                                \
                <<NFD_IN_DMA_STATE_LSO_RES_shf] }                            \
                                                                             \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd],     \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd],     \
                    AND~, NFD_IN_DMA_STATE_LSO_OFFHDR_msk,                   \
                    <<NFD_IN_DMA_STATE_LSO_OFFHDR_shf] }                     \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_SEQ_CNT_wrd],    \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_SEQ_CNT_wrd],    \
                    AND~, NFD_IN_DMA_STATE_LSO_SEQ_CNT_msk,                  \
                    <<NFD_IN_DMA_STATE_LSO_SEQ_CNT_shf] }                    \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_PAYLOAD_wrd],    \
                    --, B, 0] }                                              \
                                                                             \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd],    \
                    --, b, 0] }                                              \
                                                                             \
                                                                             \
        /* Sanity check internal state */                                    \
        if (mss == 0) {                                                      \
            /* MSS == 0 leaves no room for payload data */                   \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
        if (lso_hdrlen < NFD_IN_MIN_LSO_HDR_SZ) {                            \
            /* The lso_hdrlen is too small for TCP */                        \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
        hdr_len_chk = offset + lso_hdrlen;                                   \
        if (data_len <= hdr_len_chk) {                                       \
            /* The total length doesn't leave any data after accounting */   \
            /* for lso_hdrlen and meta data length */                        \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
        if (hdr_len_chk > NFD_IN_MAX_LSO_HDR_SZ) {                           \
            /* The meta length consumes some space in the */                 \
            /* LSO header cache so drivers which prepend meta data */        \
            /* must restrict the header length to account for it. */         \
            /* This could be an attack aiming at overwriting the */          \
            /* header of another LSO packet in a different queue. */         \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
        if ((mss + lso_hdrlen) >                                             \
            (NFD_IN_BLM_JUMBO_SIZE - NFD_IN_DATA_OFFSET)) {                  \
            /* The generated packets won't fit in the available packets. */  \
            /* This might mean that the host didn't respect the MTU that */  \
            /* the device advertised. */                                     \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
                                                                             \
    } else {                                                                 \
        unsigned int mss_msk = NFD_IN_DMA_STATE_LSO_MSS_msk;                 \
                                                                             \
        /* This is an existing LSO packet, extract and sanity check */       \
        /* internal state */                                                 \
        __asm {                                                              \
            alu[offset, NFD_IN_DMA_STATE_OFFSET_msk, and,                    \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_OFFSET_wrd],             \
                >>NFD_IN_DMA_STATE_OFFSET_shf] }                             \
        __asm { ld_field_w_clr[                                              \
            data_len, 7, NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_DATA_LEN_wrd]] } \
        __asm {                                                              \
            alu[lso_hdrlen, NFD_IN_DMA_STATE_LSO_HDRLEN_msk, and,            \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_HDRLEN_wrd],         \
                >>NFD_IN_DMA_STATE_LSO_HDRLEN_shf] }                         \
        __asm { alu[mss, mss_msk, and,                                       \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_MSS_wrd],        \
                    >>NFD_IN_DMA_STATE_LSO_MSS_shf] }                        \
                                                                             \
        /* Sanity check internal state */                                    \
        if (issue_dma_queue_state_proc_lso() == 0) {                         \
            /* The previous descriptor was not LSO.  We can't do anything */ \
            /* meaningful with this descriptor so simply flag it invalid. */ \
            /* XXX curr_buf may hold an MU pointer, but that case is */      \
            /* already handled. */                                           \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
                                                                             \
        lso_req_wrd = tx_desc.pkt##_pkt##.__raw[2];                          \
        lso_req_wrd &= ~(NFD_IN_DMA_STATE_LSO_RES_msk <<                     \
                         NFD_IN_DMA_STATE_LSO_RES_shf);                      \
        __asm { alu[lso_req_wrd, lso_req_wrd, -,                             \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_MSS_wrd]] }      \
        if (lso_req_wrd != 0) {                                              \
            /* Something has changed in the LSO request parameters */        \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
        if (offset != tx_desc.pkt##_pkt##.offset) {                          \
            /* offset doesn't match original request */                      \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
        if ((data_len & NFD_IN_DMA_STATE_DATA_LEN_ORIG_msk) !=               \
            tx_desc.pkt##_pkt##.data_len) {                                  \
            /* data_len doesn't match original request */                    \
            /* XXX only test the low 16bits because data_len was unwrapped */ \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
                                                                             \
    }                                                                        \
                                                                             \
    __asm {                                                                  \
        alu[bytes_dmaed, --, b,                                              \
            NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd]] }          \
    if (tx_desc.pkt##_pkt##.eop) {                                           \
        /* This descriptor must finish the LSO transfer */                   \
        if ((bytes_dmaed + dma_len) != data_len) {                           \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
    } else {                                                                 \
        /* This descriptor does not finish the LSO transfer */               \
        if ((bytes_dmaed + dma_len) >= data_len) {                           \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],    \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }             \
        }                                                                    \
    }                                                                        \
                                                                             \
    if (dma_len == 0) {                                                      \
        /* Flag the packet as invalid and set dma_len */                     \
        /* to a harmless value. */                                           \
        dma_len = NFD_IN_DMA_INVALID_LEN;                                    \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],        \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd],        \
                    or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }                 \
    }                                                                        \
                                                                             \
    /* We need to :                                             */           \
    /* Per LSO handling                                         */           \
    /* 1. Load the header to CTM buffer                         */           \
    /* Per generated packet                                     */           \
    /* 2. Get an MU buffer                                      */           \
    /* 3. Copy the header                                       */           \
    /* 4. Issue the data DMA (use Jumbo seq numbers)            */           \
    /* 5. Send a message to notify (emem ring)                  */           \
    /* Again per LSO handling                                   */           \
    /* 6. Send the message to notify (cls ring)                 */           \
                                                                             \
    /* get queue_data[queue].curr_buf */                                     \
    __asm { alu[curr_buf, --, B,                                             \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd]] }         \
    /* get the queue_data[queue].lso_offhdr */                               \
    __asm { alu[lso_offhdr, NFD_IN_DMA_STATE_LSO_OFFHDR_msk, AND,            \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd],         \
                >>NFD_IN_DMA_STATE_LSO_OFFHDR_shf] }                         \
    /* 1. Load the header to CTM buffer */                                   \
    if ((lso_offhdr != (lso_hdrlen + offset)) &&                             \
        !(curr_buf & NFD_IN_DMA_STATE_INVALID_shf)) {                        \
        /* We use the DMA slot granted to this tx_descp. We are issuing */   \
        /* a "signal" based DMA since we must wait for the header loading */ \
        /* to complete.*/                                                    \
                                                                             \
        buf_addr = &lso_hdr_data[(queue << __log2(NFD_IN_MAX_LSO_HDR_SZ)) +  \
                                 lso_offhdr];                                \
        /*cpp_hi_word = NFP_PCIE_DMA_CMD_CPP_ADDR_HI(__ISLAND | (2 << 6));*/ \
        cpp_hi_word = 0;                                                     \
        cpp_addr_lo = buf_addr & 0xFFFFFFFF;                                 \
        pcie_hi_word = pcie_hi_word_part |                                   \
              NFP_PCIE_DMA_CMD_PCIE_ADDR_HI(tx_desc.pkt##_pkt##.dma_addr_hi);\
        pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;                      \
        cpp_hi_word |= NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN);    \
        cpp_hi_word |= NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG);  \
                                                                             \
        if (dma_len >= (offset + lso_hdrlen - lso_offhdr)) {                 \
            /* we have full header in this TX descriptor */                  \
            dma_length = offset + lso_hdrlen - lso_offhdr;                   \
        } else {                                                             \
            /* not enough header available */                                \
            dma_length = dma_len;                                            \
        }                                                                    \
        pcie_hi_word |= NFP_PCIE_DMA_CMD_LENGTH(dma_length - 1);             \
                                                                             \
        /* should be using pcie_dma_set_sig but we don't use the "cmd"*/     \
        /* needed for it.*/                                                  \
        mode =  ((__MEID & 0xF) << 13) | (((__MEID >> 4) & 0x3F) << 7);      \
        mode |= ((ctx() & 0x7) << 4);                                        \
        mode |= __signal_number(&lso_hdr_dma_sig);                           \
        cpp_hi_word |= (mode << NFP_PCIE_DMA_CMD_DMA_MODE_shf);              \
        dma_out.pkt##_pkt##.__raw[0] = cpp_addr_lo;                          \
        dma_out.pkt##_pkt##.__raw[1] = cpp_hi_word;                          \
        dma_out.pkt##_pkt##.__raw[2] = pcie_addr_lo;                         \
        dma_out.pkt##_pkt##.__raw[3] = pcie_hi_word;                         \
        __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##, NFD_IN_DATA_DMA_QUEUE,\
                       sig_done, &lso_hdr_enq_sig);                          \
        lso_offhdr += dma_length;                                            \
        /* set queue_data[queue].lso_offhdr */                               \
        ctassert(NFD_IN_DMA_STATE_LSO_OFFHDR_shf == 16);                     \
        ctassert(NFD_IN_DMA_STATE_LSO_OFFHDR_msk == 0xFF);                   \
        __asm { ld_field[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd], \
                         4, lso_offhdr, <<NFD_IN_DMA_STATE_LSO_OFFHDR_shf] } \
        /* update bytes_dmaed */                                             \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd],    \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd], +, \
                    dma_length] }                                            \
        lso_dma_index += dma_length;                                         \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                           \
                             NFD_IN_LSO_CNTR_T_ISSUED_LSO_HDR_READ);         \
        __wait_for_all(&lso_hdr_enq_sig, &lso_hdr_dma_sig);                  \
        /* We need to optimize here and kick the first LSO packet's data */  \
        /* including the header directly to emem buffer.*/                   \
        /* For later parts of the LSO we will use the PE DMA to load the */  \
        /* header followed by a data DMA for the payload*/                   \
    }                                                                        \
                                                                             \
                                                                             \
    /* Load running variables from LM */                                     \
    /* get queue_data[queue].lso_seq_cnt */                                  \
    __asm { alu[lso_seq_cnt, NFD_IN_DMA_STATE_LSO_SEQ_CNT_msk, and,          \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_SEQ_CNT_wrd],        \
                >>NFD_IN_DMA_STATE_LSO_SEQ_CNT_shf] }                        \
    /* get lso_payload_len */                                                \
    __asm { alu[lso_payload_len, --, B,                                      \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_PAYLOAD_wrd]] }      \
                                                                             \
    while (lso_dma_index < dma_len) {                                        \
        SIGNAL_MASK lso_wait_msk;                                            \
                                                                             \
        /* We are starting to work on MU buffers, so lock the queue state */ \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],         \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],         \
                    OR, 1, <<NFD_IN_DMA_STATE_LOCKED_shf] }                  \
                                                                             \
        /* XXX deliberately test "invalid" together with MU buffer */        \
        /* If the descriptor is invalid, we suppress buffer allocation */    \
        /* and header copy. */                                               \
        if (curr_buf == 0) {                                                 \
            /* We are starting a new buffer */                               \
            /* Ensure we haven't exceeded NFD_IN_MAX_LSO_SEQ_CNT */          \
            /* and that the queue is still up. */                            \
            lso_seq_cnt++;                                                   \
            if ((lso_seq_cnt <= NFD_IN_MAX_LSO_SEQ_CNT) &&                   \
                issue_dma_queue_state_bit_set_test(                          \
                    NFD_IN_DMA_STATE_UP_shf)) {                              \
                /* 2. Get an MU buffer */                                    \
                while (precache_bufs_jumbo_use(&curr_buf) != 0) {            \
                    NFD_IN_LSO_CNTR_INCR(                                    \
                        nfd_in_lso_cntr_addr,                                \
                        NFD_IN_LSO_CNTR_T_ISSUED_LSO_BLM_BUF_ALLOC_FAILED);  \
                    ctx_swap();                                              \
                }                                                            \
                _ISSUE_PROC_MU_CHK(curr_buf);                                \
                __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd], \
                            --, B, curr_buf] }                               \
                                                                             \
                /* 3. Copy the header */                                     \
                hdr_pkt_ptr = (((uint64_t)curr_buf << 11) |                  \
                               (NFD_IN_DATA_OFFSET - offset));               \
                                                                             \
                header_to_read = ((lso_offhdr + 0x3F) & ~0x3F);              \
                __mem_pe_dma_ctm_to_mu(                                      \
                    (__mem40 void *)hdr_pkt_ptr,                             \
                    (__ctm40 void *)&lso_hdr_data[                           \
                        (queue << __log2(NFD_IN_MAX_LSO_HDR_SZ))],           \
                    header_to_read, sig_done, &lso_hdr_sig);                 \
                                                                             \
                /* clear lso_payload_len */                                  \
                lso_payload_len = 0;                                         \
                                                                             \
                /* wait for header copy to complete */                       \
                __wait_for_all(&lso_hdr_sig);                                \
                                                                             \
            } else {                                                         \
                /* The LSO descriptors have either generated too many */     \
                /* segments or the queue has gone down while processing */   \
                /* the descriptor.  Either could be malicious. */            \
                curr_buf |= (1 << NFD_IN_DMA_STATE_INVALID_shf);             \
                                                                             \
            }                                                                \
        }                                                                    \
                                                                             \
        /* Handle invalid at a single point */                               \
        if (curr_buf & (1 << NFD_IN_DMA_STATE_INVALID_shf)) {                \
            NFD_IN_LSO_CNTR_INCR(                                            \
                nfd_in_lso_cntr_addr,                                        \
                NFD_IN_LSO_CNTR_T_ISSUED_LSO_EXC_PKT_TO_NOTIFY_RING);        \
                                                                             \
            if ((curr_buf & NFD_IN_DMA_STATE_CURR_BUF_msk) == 0) {           \
                /* We haven't yet allocated an MU buffer, but the issued */  \
                /* ring must always hold legitimate MU buffers.   We can */  \
                /* use the regular buffer associated with this _pkt slot */  \
                /* in the batch. */                                          \
                curr_buf = precache_bufs_use();                              \
                _ISSUE_PROC_MU_CHK(curr_buf);                                \
                curr_buf |= (1 << NFD_IN_DMA_STATE_INVALID_shf);             \
            }                                                                \
            /* Build the ring message more or less as normal, so that */     \
            /* whatever debugging info may be available is preserved. */     \
            /* Setting EOP ensures that the app gets this message. */        \
            issued_tmp.eop = 1;                                              \
            issued_tmp.sp1 = 0;                                              \
            issued_tmp.offset = offset;                                      \
            issued_tmp.buf_addr = curr_buf;                                  \
            issued_tmp.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];              \
            issued_tmp.lso_seq_cnt = lso_seq_cnt;                            \
            issued_tmp.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];              \
            /* send the lso pkt desc to the lso ring */                      \
            batch_out.pkt##_pkt## = issued_tmp;                              \
            __mem_ring_journal(nfd_in_issued_lso_ring_num,                   \
                               nfd_in_issued_lso_ring_addr,                  \
                               &batch_out.pkt##_pkt##,                       \
                               sizeof(struct nfd_in_issued_desc),            \
                               sizeof(struct nfd_in_issued_desc),            \
                               ctx_swap, &lso_journal_sig);                  \
            /* used to track how many desc we have put on ring will */       \
            /* set value in the batch out for the packet so that */          \
            /* notify how how many to read in  */                            \
            lso_issued_cnt++;                                                \
                                                                             \
            if (tx_desc.pkt##_pkt##.eop ||                                   \
                !issue_dma_queue_state_bit_set_test(                         \
                    NFD_IN_DMA_STATE_UP_shf)) {                              \
                /* Either this looks like an end of gather descriptor */     \
                /* or the queue is in a down state. */                       \
                /* In either case, it is a safe place to try and get */      \
                /* the queue to a non-error state, so clean up everything. */ \
                                                                             \
                /* clear curr_buf, including the invalid bit */              \
                curr_buf = 0;                                                \
                /* clear CONT bit */                                         \
                __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],   \
                            NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],   \
                            AND~, 1, <<NFD_IN_DMA_STATE_CONT_shf] }          \
                /* clear LSO specific state */                               \
                __asm {                                                      \
                    alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd], \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd], \
                        AND~, NFD_IN_DMA_STATE_LSO_OFFHDR_msk,               \
                        <<NFD_IN_DMA_STATE_LSO_OFFHDR_shf] }                 \
                lso_seq_cnt = 0;                                             \
                __asm {                                                      \
                    alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_FLAGS_wrd],      \
                        --, b, 0] }                                          \
                lso_payload_len = 0;                                         \
                /* clear state shared with gather code */                    \
                __asm {                                                      \
                    alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd], \
                        --, b, 0] }                                          \
                __asm {                                                      \
                    alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_OFFSET_wrd],     \
                        --, B, 0] }                                          \
            } else {                                                         \
                /* clear curr_buf, but leave invalid set */                  \
                curr_buf = (1 << NFD_IN_DMA_STATE_INVALID_shf);              \
            }                                                                \
                                                                             \
            /* break out of while loop to prevent further processing */      \
            break;                                                           \
        }                                                                    \
                                                                             \
        /* 4. Issue the data DMA (use Jumbo seq numbers) */                  \
        /* get the length left in mu buffer */                               \
        mu_buf_left = mss - lso_payload_len;                                 \
        /* get the length left to DMA. */                                    \
        dma_left = dma_len - lso_dma_index;                                  \
        /* more to dma than we have buffer for */                            \
        if (mu_buf_left <= dma_left) {                                       \
            /* fill in descriptor length to dma */                           \
            dma_length = mu_buf_left;                                        \
        }                                                                    \
        /* more mu buffer than we have dma for */                            \
        else {                                                               \
            /* fill in descriptor length to dma */                           \
            dma_length = dma_left;                                           \
        }                                                                    \
                                                                             \
        /* Update bytes_dmaed for how much will be transfered */             \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd],    \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd],    \
                    +, dma_length] }                                         \
                                                                             \
        cpp_addr_lo = curr_buf << 11;                                        \
        cpp_addr_lo -= offset;                                               \
        cpp_addr_lo += lso_offhdr;                                           \
        cpp_addr_lo += lso_payload_len;                                      \
        pcie_hi_word = (pcie_hi_word_part |                                  \
             NFP_PCIE_DMA_CMD_PCIE_ADDR_HI(tx_desc.pkt##_pkt##.dma_addr_hi));\
        pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;                      \
        _add_to_pcie_addr(&pcie_hi_word, &pcie_addr_lo, lso_dma_index);      \
                                                                             \
        /* account for how much will be read */                              \
        lso_dma_index += dma_length;                                         \
        lso_payload_len += dma_length;                                       \
                                                                             \
        /* Check for and handle large (jumbo) packets  */                    \
        while (dma_length > NFD_IN_DMA_SPLIT_THRESH) {                       \
            _ISSUE_PROC_LSO_JUMBO(_pkt, curr_buf);                           \
            NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                       \
                                 NFD_IN_LSO_CNTR_T_ISSUED_LSO_JUMBO_TX_DESC);\
        }                                                                    \
                                                                             \
        /* Get a DMA slot, we're using the DMAs allocated for jumbos */      \
        /* also for LSO. */                                                  \
        /* Take a jumbo frame sequence number and */                         \
        /* check it is safe to use */                                        \
        jumbo_dma_seq_issued++;                                              \
        jumbo_seq_test = (NFD_IN_JUMBO_MAX_IN_FLIGHT - jumbo_dma_seq_issued);\
        while ((int) (jumbo_seq_test + jumbo_dma_seq_compl) <= 0) {          \
            ctx_swap();                                                      \
        }                                                                    \
        /* set up the descr_tmp dma values */                                \
        cpp_hi_word = dma_seqn_init_event(NFD_IN_JUMBO_EVENT_TYPE,           \
                                          PCI_IN_ISSUE_DMA_IDX);             \
        cpp_hi_word = dma_seqn_set_seqn(cpp_hi_word, jumbo_dma_seq_issued);  \
        cpp_hi_word |= NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN);    \
        cpp_hi_word |= NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG);  \
        pcie_hi_word |= NFP_PCIE_DMA_CMD_LENGTH(dma_length - 1);             \
        dma_out.pkt##_pkt##.__raw[0] = cpp_addr_lo + NFD_IN_DATA_OFFSET;     \
        dma_out.pkt##_pkt##.__raw[1] =                                       \
            cpp_hi_word | NFP_PCIE_DMA_CMD_CPP_ADDR_HI(curr_buf >> 21);      \
        dma_out.pkt##_pkt##.__raw[2] = pcie_addr_lo;                         \
        dma_out.pkt##_pkt##.__raw[3] = (pcie_hi_word |                       \
                                        NFP_PCIE_DMA_CMD_LENGTH(             \
                                            dma_length - 1));                \
        /* XXX Issue DMA with ctx_swap to ensure we can reuse the XFERs */   \
        /* (dma_out.pkt##_pkt##) when we wake up again.  */                  \
        __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,                       \
                       NFD_IN_DATA_DMA_QUEUE, sig_done, &lso_enq_sig);       \
        lso_wait_msk = __signals(&lso_enq_sig);                              \
                                                                             \
        /* if we are at end of mu_buf, or the end of the LSO buffer */       \
        if ((lso_payload_len == mss) ||                                      \
            ((lso_dma_index == dma_len) && tx_desc.pkt##_pkt##.eop)) {       \
            /* put finished mu buffer on lso_ring to notify */               \
            issued_tmp.eop = 1;                                              \
            issued_tmp.sp1 = 0;                                              \
            issued_tmp.offset = offset;                                      \
            issued_tmp.buf_addr = curr_buf;                                  \
            issued_tmp.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];              \
            issued_tmp.lso_seq_cnt = lso_seq_cnt;                            \
            issued_tmp.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];              \
            issued_tmp.data_len = lso_offhdr + lso_payload_len;              \
            /* if last of LSO segment set lso end flag and we have no */     \
            /* more dma data */                                              \
            if ((lso_dma_index == dma_len) && tx_desc.pkt##_pkt##.eop) {     \
                issued_tmp.lso_end = 1;                                      \
                NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                   \
                        NFD_IN_LSO_CNTR_T_ISSUED_LSO_END_PKT_TO_NOTIFY_RING);\
                                                                             \
                /* clear curr_buf, including the invalid bit */              \
                curr_buf = 0;                                                \
                /* clear CONT bit */                                         \
                __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],   \
                            NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],   \
                            AND~, 1, <<NFD_IN_DMA_STATE_CONT_shf] }          \
                /* clear LSO specific state */                               \
                __asm {                                                      \
                    alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd], \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_OFFHDR_wrd], \
                        AND~, NFD_IN_DMA_STATE_LSO_OFFHDR_msk,               \
                        <<NFD_IN_DMA_STATE_LSO_OFFHDR_shf] }                 \
                lso_seq_cnt = 0;                                             \
                __asm {                                                      \
                    alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_FLAGS_wrd],      \
                        --, b, 0] }                                          \
                lso_payload_len = 0;                                         \
                /* clear state shared with gather code */                    \
                __asm {                                                      \
                    alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd], \
                        --, b, 0] }                                          \
                __asm {                                                      \
                    alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_OFFSET_wrd],     \
                        --, B, 0] }                                          \
            } else {                                                         \
                /* clear curr_buf, leaving invalid untouched */              \
                curr_buf &= (1 << NFD_IN_DMA_STATE_INVALID_shf);             \
            }                                                                \
            /* send the lso pkt desc to the lso ring */                      \
            batch_out.pkt##_pkt## = issued_tmp;                              \
            __mem_ring_journal(nfd_in_issued_lso_ring_num,                   \
                            nfd_in_issued_lso_ring_addr,                     \
                            &batch_out.pkt##_pkt##,                          \
                            sizeof(struct nfd_in_issued_desc),               \
                            sizeof(struct nfd_in_issued_desc), sig_done,     \
                            &lso_journal_sig);                               \
            lso_wait_msk |= __signals(&lso_journal_sig);                     \
                                                                             \
            NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                       \
                        NFD_IN_LSO_CNTR_T_ISSUED_LSO_ALL_PKT_TO_NOTIFY_RING);\
            /* clear curr_buf, leaving invalid untouched */                  \
            curr_buf &= (NFD_IN_DMA_STATE_INVALID_msk <<                     \
                         NFD_IN_DMA_STATE_INVALID_shf);                      \
            /* used to track how many desc we have put on ring will */       \
            /* set value in the batch out for the packet so that */          \
            /* notify how how many to read in  */                            \
            lso_issued_cnt++;                                                \
                                                                             \
            /* Save (cleared) curr_buf back to LM */                         \
            /* set queue_data[queue].curr_buf */                             \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd],   \
                        --, B, curr_buf] }                                   \
                                                                             \
            /* Unlock the queue state to give a chance to process */         \
            /* config messages for the queue during the next swap. */        \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],     \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],     \
                        AND~, 1, <<NFD_IN_DMA_STATE_LOCKED_shf] }            \
        } /* End of ring put if statement  */                                \
                                                                             \
        /* Wait for IO to complete */                                        \
        wait_sig_mask(lso_wait_msk);                                         \
        __implicit_read(&lso_journal_sig);                                   \
        __implicit_read(&lso_enq_sig);                                       \
                                                                             \
    } /* while */                                                            \
                                                                             \
    NFD_IN_LSO_CNTR_CLR(nfd_in_lso_cntr_addr,                                \
                        NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_MSS);              \
    NFD_IN_LSO_CNTR_ADD(nfd_in_lso_cntr_addr,                                \
                       NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_MSS,                \
                       mss);                                                 \
    NFD_IN_LSO_CNTR_CLR(nfd_in_lso_cntr_addr,                                \
                        NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_HDRLEN);           \
    NFD_IN_LSO_CNTR_ADD(nfd_in_lso_cntr_addr,                                \
                        NFD_IN_LSO_CNTR_X_ISSUED_LAST_LSO_HDRLEN,            \
                        lso_hdrlen);                                         \
                                                                             \
    /* Save running variables back to LM */                                  \
    /* set queue_data[queue].curr_buf */                                     \
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd],           \
                --, B, curr_buf] }                                           \
    /* set queue_data[queue].lso_seq_cnt */                                  \
    ctassert(NFD_IN_DMA_STATE_LSO_SEQ_CNT_shf == 8);                         \
    ctassert(NFD_IN_DMA_STATE_LSO_SEQ_CNT_msk == 0xFF);                      \
    __asm { ld_field[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_SEQ_CNT_wrd],   \
                     2, lso_seq_cnt, <<NFD_IN_DMA_STATE_LSO_SEQ_CNT_shf] }   \
    /* set lso_payload_len */                                                \
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LSO_PAYLOAD_wrd],        \
                --, B, lso_payload_len] }                                    \
                                                                             \
                                                                             \
    /* prep batch_out for LSO segment info */                                \
    issued_tmp.eop = 0;                                                      \
    issued_tmp.offset = 0;                                                   \
    issued_tmp.lso_issued_cnt = lso_issued_cnt & 0xFF;                       \
    batch_out.pkt##_pkt## = issued_tmp;                                      \
    issued_tmp.lso_issued_cnt = 0;                                           \
    /* Re-increment data_dma_seq_issued */                                   \
    data_dma_seq_issued++;                                                   \
    /* Handle the DMA sequence numbers */                                    \
    if (type == NFD_IN_DATA_EVENT_TYPE) {                                    \
        cpp_hi_word = dma_seqn_init_event(NFD_IN_DATA_EVENT_TYPE,            \
                                          PCI_IN_ISSUE_DMA_IDX);             \
        cpp_hi_word = dma_seqn_set_seqn(cpp_hi_word, data_dma_seq_issued);   \
        cpp_hi_word |= NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN);    \
        cpp_hi_word |=                                                       \
            NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG_SIG_ONLY);    \
                                                                             \
        dma_out.pkt##_pkt##.__raw[0] = 0;                                    \
        dma_out.pkt##_pkt##.__raw[1] = cpp_hi_word;                          \
        dma_out.pkt##_pkt##.__raw[2] = 0;                                    \
        dma_out.pkt##_pkt##.__raw[3] = (pcie_hi_word_part |                  \
                                        NFP_PCIE_DMA_CMD_LENGTH(0));         \
        cpp_hi_word |= NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG);  \
        __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt,                         \
                       NFD_IN_DATA_DMA_QUEUE,                                \
                       sig_done, &last_of_batch_dma_sig);                    \
    }                                                                        \
                                                                             \
    /* We're done with the descriptor so unlock the queue state */           \
    __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],             \
                NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd],             \
                AND~, 1, <<NFD_IN_DMA_STATE_LOCKED_shf] }                    \
}
DECLARE_PROC_LSO(0);
DECLARE_PROC_LSO(1);
DECLARE_PROC_LSO(2);
DECLARE_PROC_LSO(3);
DECLARE_PROC_LSO(4);
DECLARE_PROC_LSO(5);
DECLARE_PROC_LSO(6);
DECLARE_PROC_LSO(7);


#if __REVISION_MIN < __REVISION_B0
#error "A0 chips not supported"
#endif


/* Setup _ISSUE_PROC_JUMBO_TEST, a fast path test value to identify
 * jumbo frames.  We may branch off the fast path to swap out a
 * buf_store buffer for a jumbo_store buffer, or to issue separate
 * NFD_IN_DMA_SPLIT_LEN DMAs, or both.  _ISSUE_PROC_JUMBO_TEST
 * takes the "min" of the two thresholds. */
#if ((NFD_IN_BLM_REG_SIZE - NFD_IN_DATA_OFFSET) < NFD_IN_DMA_SPLIT_THRESH)
/* We will need to replace the MU buffer before we need to split packets
 * into multiple DMAs. */
#define _ISSUE_PROC_JUMBO_TEST (NFD_IN_BLM_REG_SIZE - NFD_IN_DATA_OFFSET)
#else
/* We will need to split the packet into multiple DMAs before we need to
 * replace the MU buffer. */
#define _ISSUE_PROC_JUMBO_TEST NFD_IN_DMA_SPLIT_THRESH
#endif


/* Setup _ISSUE_PROC_LSO_TEST, used to disable LSO if the capability
 * is not advertised on either the PF or VFs. */
#if ((NFD_CFG_VF_CAP & NFP_NET_CFG_CTRL_LSO_ANY) || \
     (NFD_CFG_PF_CAP & NFP_NET_CFG_CTRL_LSO_ANY))
#define _ISSUE_PROC_LSO_TEST(_pkt)                                  \
    ((tx_desc.pkt##_pkt##.__raw[2] &                                \
      (PCIE_DESC_TX_LSO << NFD_IN_DMA_STATE_FLAGS_shf)) &&          \
     (issue_dma_queue_state_bit_set_test(NFD_IN_DMA_STATE_UP_shf)))
#else
#define _ISSUE_PROC_LSO_TEST(_pkt) (0)
#endif


#define _ISSUE_PROC(_pkt, _type, _src, _priority)                       \
do {                                                                    \
    unsigned int dma_len;                                               \
    __gpr unsigned int buf_addr;                                        \
    __gpr unsigned int curr_buf;                                        \
    __gpr unsigned int temp;                                            \
    unsigned int cpp_hi_word;                                           \
    unsigned int cpp_addr_lo;                                           \
    unsigned int pcie_hi_word;                                          \
    unsigned int pcie_addr_lo;                                          \
                                                                        \
    NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                          \
                         NFD_IN_LSO_CNTR_T_ISSUED_ALL_TX_DESC);         \
                                                                        \
    /* if tx descr is LSO */                                            \
    if (_ISSUE_PROC_LSO_TEST(_pkt)) {                                   \
        /* do the lso function processing */                            \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                      \
                             NFD_IN_LSO_CNTR_T_ISSUED_LSO_ALL_TX_DESC); \
        /* ADD CALL TO LSO FUNCTION HERE */                             \
        issue_proc_lso##_pkt(queue, pcie_hi_word_part, _type);          \
        _LSO_TX_DESC_TYPE_CNTR(_pkt);                                   \
                                                                        \
    } else if (tx_desc.pkt##_pkt##.eop &&                               \
        (issue_dma_queue_state_bit_set_test(NFD_IN_DMA_STATE_CONT_shf)  \
         == 0)) {                                                       \
        unsigned int cpp_hi_addr;                                       \
                                                                        \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                      \
                      NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_EOP_TX_DESC);    \
        /* Fast path, use buf_store data */                             \
                                                                        \
        /* Set NFP buffer address and offset */                         \
        buf_addr = precache_bufs_use();                                 \
        _ISSUE_PROC_MU_CHK(buf_addr);                                   \
        cpp_addr_lo = buf_addr << 11;                                   \
        cpp_addr_lo -= tx_desc.pkt##_pkt##.offset;                      \
                                                                        \
        pcie_hi_word =                                                  \
            (pcie_hi_word_part |                                        \
             NFP_PCIE_DMA_CMD_PCIE_ADDR_HI(tx_desc.pkt##_pkt##.dma_addr_hi)); \
        pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;                 \
                                                                        \
                                                                        \
        /* Get the data/dma length */                                   \
        /* This is a simple packet so use the data_len, which is */     \
        /* passed on to the application.  dma_len in the descriptor */  \
        /* is ignored. */                                               \
        /* XXX Store the value as "length - 1" so that */               \
        /* dma_len = 0 is implicitly checked in the oversize */         \
        /* packet checks. */                                            \
        dma_len = tx_desc.pkt##_pkt##.data_len - 1;                     \
                                                                        \
        /* Check for and handle large (jumbo) packets  */               \
        if (dma_len > (_ISSUE_PROC_JUMBO_TEST - 1)) {                   \
            /* We may need to swap the buffer allocated from */         \
            /* buf_store for one allocated from jumbo_store. */         \
            /* This must be done before context swapping. */            \
            /* We calculate the amount of data that will fit */         \
            /* using the dma_len which is immediately available */      \
            /* rather than the packet length.  This is slightly */      \
            /* pessimistic but efficient in terms of code store. */     \
            if (dma_len > (NFD_IN_BLM_REG_SIZE - NFD_IN_DATA_OFFSET - 1)) { \
                unsigned int pkt_len;                                   \
                                                                        \
                precache_bufs_return(buf_addr);                         \
                while (precache_bufs_jumbo_use(&buf_addr) != 0) {       \
                    /* Allow service context to run */                  \
                    /* to refill jumbo_store */                         \
                    /* Lock the queue state while we're swapped */      \
                    __asm {                                             \
                        alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd], \
                            NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd], \
                            OR, 1, <<NFD_IN_DMA_STATE_LOCKED_shf] }     \
                    ctx_swap();                                         \
                    __asm {                                             \
                        alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd], \
                            NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd], \
                            AND~, 1, <<NFD_IN_DMA_STATE_LOCKED_shf] }   \
                }                                                       \
                _ISSUE_PROC_MU_CHK(buf_addr);                           \
                                                                        \
                /* cpp_addr_lo is now stale and must be recomputed */   \
                cpp_addr_lo = buf_addr << 11;                           \
                cpp_addr_lo -= tx_desc.pkt##_pkt##.offset;              \
                                                                        \
                /* Check that the packet will fit within */             \
                /* a buffer of NFD_IN_BLM_JUMBO_SIZE.  This is */       \
                /* determined by the packet length rather than the */   \
                /* DMA length, because the amount of meta data */       \
                /* the DMA start address, but not the packet start */   \
                /* address. */                                          \
                pkt_len = dma_len - tx_desc.pkt##_pkt##.offset;         \
                if (pkt_len > (NFD_IN_BLM_JUMBO_SIZE -                  \
                               NFD_IN_DATA_OFFSET - 1)) {               \
                    /* Flag the packet as invalid and set dma_len */    \
                    /* to a harmless value. */                          \
                    buf_addr |= (1 << NFD_IN_DMA_STATE_INVALID_shf);    \
                    dma_len = NFD_IN_DMA_INVALID_LEN - 1;               \
                }                                                       \
            }                                                           \
                                                                        \
            /* We may need to break this packet up into */              \
            /* multiple DMAS. */                                        \
            /* XXX if we aren't splitting packets, then we will */      \
            /* only refill the jumbo_store when it tests empty */       \
            /* in precache_bufs_jumbo_use() above.  */                  \
            if (dma_len > NFD_IN_DMA_SPLIT_THRESH - 1) {                \
                do {                                                    \
                    _ISSUE_PROC_JUMBO(_pkt, buf_addr);                  \
                    NFD_IN_LSO_CNTR_INCR(                               \
                        nfd_in_lso_cntr_addr,                           \
                        NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_EOP_JUMBO_TX_DESC); \
                } while (dma_len > NFD_IN_DMA_SPLIT_LEN - 1);           \
            }                                                           \
        } else {                                                        \
            /* Tag the inner most branch of the critical path */        \
            /* Use a _priority tag to distinguish between full */       \
            /* partial batches. */                                      \
            __critical_path(_priority);                                 \
        }                                                               \
                                                                        \
        /* Do smart rounding of the DMA length, if the packet */        \
        /* is large enough to justify it */                             \
        if (dma_len > (128 + 8 - 1)) {                                  \
            unsigned int start_off;                                     \
            unsigned int len_adj;                                       \
                                                                        \
            /* dma_len stores "length - 1" */                           \
            /* round8(v) = 1 + (v - 1) + 7 - ((v - 1) & 7) */           \
            /* hence round_dma(dma_len) = dma_len + 7 - dma_len & 7 */  \
            /* it is safe to round dma_len if: */                       \
            /* 1: start_off == 0; rounding up can't cross an 8B boundary */ \
            /* 2: sign(start_off + length & 7 - 8) == */                \
            /*       sign(start_off + 8 - 8) == "+ve" */                \
            /* Hence start_off > (8 - length & 7) */                    \
            /* or start_off > len_adj */                                \
            len_adj = 7 - (dma_len & 7);                                \
            start_off = pcie_addr_lo & 7;                               \
            if ((start_off == 0) || (start_off > len_adj)) {            \
                /* XXX inspecting the list file, it's better to test */ \
                /* start_off == 0 first so that the len_adj calc */     \
                /* can be done in the defer shadow */                   \
                dma_len += len_adj;                                     \
            }                                                           \
        }                                                               \
                                                                        \
        /* Issue final DMA for the packet */                            \
        dma_out.pkt##_pkt##.__raw[0] = cpp_addr_lo + NFD_IN_DATA_OFFSET; \
        dma_out.pkt##_pkt##.__raw[2] = pcie_addr_lo;                    \
        dma_out.pkt##_pkt##.__raw[3] = (pcie_hi_word |                  \
                                        NFP_PCIE_DMA_CMD_LENGTH(dma_len)); \
                                                                        \
        /* Use asm to ensure this generates one fast path cycle */      \
        __asm { alu[cpp_hi_addr, NFP_PCIE_DMA_CMD_CPP_ADDR_HI_msk, AND, \
                    buf_addr, >>21] }                                   \
        if (_type == NFD_IN_DATA_IGN_EVENT_TYPE) {                      \
            dma_out.pkt##_pkt##.__raw[1] = cpp_hi_addr | cpp_hi_no_sig_part; \
            pcie_dma_enq_no_sig(PCIE_ISL, &dma_out.pkt##_pkt##,         \
                                NFD_IN_DATA_DMA_QUEUE);                 \
        } else {                                                        \
            cpp_hi_word = dma_seqn_set_seqn(cpp_hi_event_part, _src);   \
            dma_out.pkt##_pkt##.__raw[1] = cpp_hi_addr | cpp_hi_word;   \
            __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,              \
                           NFD_IN_DATA_DMA_QUEUE,                       \
                           sig_done, &last_of_batch_dma_sig);           \
        }                                                               \
                                                                        \
        /* Set up notify message */                                     \
        /* NB: EOP is required for all packets */                       \
        /*     q_num is must be set on pkt0 */                          \
        /*     notify technically doesn't use the rest unless */        \
        /*     EOP is set */                                            \
        issued_tmp.eop = tx_desc.pkt##_pkt##.eop;                       \
        issued_tmp.offset = tx_desc.pkt##_pkt##.offset;                 \
                                                                        \
        /* Apply a standard "recipe" to complete the DMA issue */       \
        batch_out.pkt##_pkt## = issued_tmp;                             \
        batch_out.pkt##_pkt##.__raw[1] = buf_addr;                      \
        batch_out.pkt##_pkt##.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];  \
        batch_out.pkt##_pkt##.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];  \
                                                                        \
    } else if (issue_dma_queue_state_bit_set_test(NFD_IN_DMA_STATE_UP_shf) \
               == 0) {                                                  \
        NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                      \
                             NFD_IN_LSO_CNTR_T_ISSUED_NOT_Q_UP_TX_DESC);\
        /* Handle down queues off the fast path. */                     \
        /* As all packets in a batch come from one queue and are */     \
        /* processed without swapping, all the packets in the batch */  \
        /* will receive the same treatment.  The batch will still */    \
        /* use its slot in the DMA sequence numbers and the */          \
        /* nfd_in_issued_ring. */                                       \
                                                                        \
        /* Setting "cont" when the queue is down ensures */             \
        /* that this processing happens off the fast path. */           \
                                                                        \
        /* Flag the packet for notify. */                               \
        /* Zero EOP and num_batch so that the notify block will not */  \
        /* produce output to the work queues, and will have no */       \
        /* effect on the queue controller queue. */                     \
        /* NB: the rest of the message will be stale. */                \
        issued_tmp.eop = 0;                                             \
        issued_tmp.offset = 0;                                          \
        issued_tmp.lso_issued_cnt = 0;                                  \
        issued_tmp.num_batch = 0;                                       \
        issued_tmp.lso_end = 0;                                         \
        issued_tmp.sp1 = 0;                                             \
        batch_out.pkt##_pkt##.__raw[0] = issued_tmp.__raw[0];           \
                                                                        \
        /* Handle the DMA sequence numbers for the batch */             \
        if (_type == NFD_IN_DATA_EVENT_TYPE) {                          \
            cpp_hi_word = dma_seqn_init_event(NFD_IN_DATA_EVENT_TYPE,   \
                                              PCI_IN_ISSUE_DMA_IDX);    \
            cpp_hi_word = dma_seqn_set_seqn(cpp_hi_word, _src);         \
            cpp_hi_word |= NFP_PCIE_DMA_CMD_CPP_TOKEN(NFD_IN_DATA_DMA_TOKEN); \
            cpp_hi_word |=                                              \
                NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG_SIG_ONLY); \
                                                                        \
            dma_out.pkt##_pkt##.__raw[0] = 0;                           \
            dma_out.pkt##_pkt##.__raw[1] = cpp_hi_word;                 \
            dma_out.pkt##_pkt##.__raw[2] = 0;                           \
            dma_out.pkt##_pkt##.__raw[3] = (pcie_hi_word_part |         \
                                            NFP_PCIE_DMA_CMD_LENGTH(0)); \
            cpp_hi_word |= NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(NFD_IN_DATA_CFG_REG); \
            __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt,                \
                           NFD_IN_DATA_DMA_QUEUE,                       \
                           sig_done, &last_of_batch_dma_sig);           \
        }                                                               \
                                                                        \
    } else {                                                            \
        unsigned int data_len;                                          \
        unsigned int offset;                                            \
        int len_chk;                                                    \
                                                                        \
        if (issue_dma_queue_state_bit_set_test(NFD_IN_DMA_STATE_CONT_shf) \
            == 0) {                                                     \
            NFD_IN_LSO_CNTR_INCR(nfd_in_lso_cntr_addr,                  \
                         NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_CONT_TX_DESC);\
            /* Initialise continuation data */                          \
            data_len = tx_desc.pkt##_pkt##.data_len;                    \
            offset = tx_desc.pkt##_pkt##.offset;                        \
            __asm {                                                     \
                alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_OFFSET_wrd],    \
                    --, b, offset, <<NFD_IN_DMA_STATE_OFFSET_shf] }     \
            __asm {                                                     \
                alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_DATA_LEN_wrd],  \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_DATA_LEN_wrd],  \
                    or, data_len, <<NFD_IN_DMA_STATE_DATA_LEN_shf] }    \
                                                                        \
            /* XXX check efficiency */                                  \
            if (data_len > (NFD_IN_BLM_REG_SIZE - NFD_IN_DATA_OFFSET)) { \
                unsigned int pkt_len;                                   \
                                                                        \
                while (precache_bufs_jumbo_use(&curr_buf) != 0) {       \
                    /* Allow service context to run */                  \
                    /* to refill jumbo_store */                         \
                    /* Lock the queue state while we're swapped */      \
                    __asm {                                             \
                        alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd], \
                            NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd], \
                            OR, 1, <<NFD_IN_DMA_STATE_LOCKED_shf] }     \
                    ctx_swap();                                         \
                    __asm {                                             \
                        alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd], \
                            NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_LOCKED_wrd], \
                            AND~, 1, <<NFD_IN_DMA_STATE_LOCKED_shf] }   \
                }                                                       \
                                                                        \
                /* Check that the packet will fit within */             \
                /* a buffer of NFD_IN_BLM_JUMBO_SIZE.  This is */       \
                /* determined by the packet length rather than the */   \
                /* DMA length, because the amount of meta data */       \
                /* the DMA start address, but not the packet start */   \
                /* address. */                                          \
                pkt_len = data_len - offset;                            \
                if (pkt_len > (NFD_IN_BLM_JUMBO_SIZE -                  \
                               NFD_IN_DATA_OFFSET)) {                   \
                    /* Flag the packet as invalid */                    \
                    curr_buf |= (1 << NFD_IN_DMA_STATE_INVALID_shf);    \
                }                                                       \
                                                                        \
            } else {                                                    \
                curr_buf = precache_bufs_use();                         \
                                                                        \
                if (data_len == 0) {                                    \
                    /* Flag the packet as invalid */                    \
                    curr_buf |= (1 << NFD_IN_DMA_STATE_INVALID_shf);    \
                }                                                       \
            }                                                           \
            _ISSUE_PROC_MU_CHK(curr_buf);                               \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],  \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],  \
                        OR, 1, <<NFD_IN_DMA_STATE_CONT_shf] }           \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd], \
                        --, B, curr_buf] }                              \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd], \
                        --, b, 0] }                                     \
        } else {                                                        \
            /* We have existing packet state, check it isn't LSO  */    \
            if (issue_dma_queue_state_proc_lso() == 1) {                \
                /* It is LSO, call a helper function to clean up */     \
                /* and set the invalid bit */                           \
                /* XXX the helper function ensures we have a */         \
                /* valid buffer available */                            \
                issue_dma_cleanup_lso_state();                          \
                __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd], \
                            NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd], \
                            or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }    \
            }                                                           \
        }                                                               \
                                                                        \
        /* Get and check the data/dma length */                         \
        /* This is a gather packet so use the dma_len from the */       \
        /* descriptor is used for this segment. */                      \
        dma_len = tx_desc.pkt##_pkt##.dma_len;                          \
        if (dma_len == 0) {                                             \
            /* Set invalid bit in curr_buf */                           \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd], \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_INVALID_wrd], \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }        \
        }                                                               \
                                                                        \
        /* Use continuation data */                                     \
        /* cpp_addr_lo = ((curr_buf << 11) + */                         \
        /*                - offset + bytes_dmaed) */                    \
        /* XXX NFD_IN_DATA_OFFSET is added while copying to xfers */    \
        __asm { alu[cpp_addr_lo, --, b,                                 \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd ], \
                    <<11] }                                             \
        __asm { alu[offset, NFD_IN_DMA_STATE_OFFSET_msk, and,           \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_OFFSET_wrd],    \
                    >>NFD_IN_DMA_STATE_OFFSET_shf] }                    \
        cpp_addr_lo -= offset;                                          \
        __asm { alu[cpp_addr_lo, cpp_addr_lo, +,                        \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd]] } \
                                                                        \
        /* Update bytes_dmaed, and check it's <= data_len */            \
        __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd], \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd], \
                    +, dma_len] }                                       \
        __asm { ld_field_w_clr[                                         \
            data_len, 7, NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_DATA_LEN_wrd]] } \
        __asm { alu[len_chk, data_len, -,                               \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd]] } \
        if (len_chk < 0) {                                              \
            /* Set invalid bit in curr_buf */                           \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd], \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd], \
                        or, 1, <<NFD_IN_DMA_STATE_INVALID_shf] }        \
        }                                                               \
                                                                        \
        /* We have finished the per segment host checks */              \
        /* Copy curr_buf into a GPR for the rest of the processing. */  \
        __asm { alu[curr_buf, --, b,                                    \
                    NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd]] } \
                                                                        \
        /* Test whether any host sanity checks have failured so far */  \
        if ((curr_buf & (1 << NFD_IN_DMA_STATE_INVALID_shf)) == 0) {    \
            /* Perform the segment DMA */                               \
            pcie_hi_word =                                              \
                (pcie_hi_word_part |                                    \
                 NFP_PCIE_DMA_CMD_PCIE_ADDR_HI(                         \
                     tx_desc.pkt##_pkt##.dma_addr_hi));                 \
            pcie_addr_lo = tx_desc.pkt##_pkt##.dma_addr_lo;             \
                                                                        \
                                                                        \
            /* Check for and handle large (jumbo) packets  */           \
            if (dma_len > NFD_IN_DMA_SPLIT_THRESH) {                    \
                do {                                                    \
                    _ISSUE_PROC_JUMBO(_pkt, curr_buf);                  \
                    NFD_IN_LSO_CNTR_INCR(                               \
                        nfd_in_lso_cntr_addr,                           \
                        NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_CONT_JUMBO_TX_DESC); \
                } while (dma_len > NFD_IN_DMA_SPLIT_LEN);               \
            }                                                           \
                                                                        \
            /* Issue final DMA for the packet */                        \
            dma_out.pkt##_pkt##.__raw[0] = cpp_addr_lo + NFD_IN_DATA_OFFSET; \
            dma_out.pkt##_pkt##.__raw[2] = pcie_addr_lo;                \
            dma_out.pkt##_pkt##.__raw[3] = (pcie_hi_word |              \
                                            NFP_PCIE_DMA_CMD_LENGTH(    \
                                                dma_len - 1));          \
                                                                        \
            if (_type == NFD_IN_DATA_IGN_EVENT_TYPE) {                  \
                dma_out.pkt##_pkt##.__raw[1] = (                        \
                    cpp_hi_no_sig_part |                                \
                    NFP_PCIE_DMA_CMD_CPP_ADDR_HI(curr_buf >> 21));      \
                pcie_dma_enq_no_sig(PCIE_ISL, &dma_out.pkt##_pkt##,     \
                                    NFD_IN_DATA_DMA_QUEUE);             \
            } else {                                                    \
                cpp_hi_word = dma_seqn_set_seqn(cpp_hi_event_part, _src); \
                dma_out.pkt##_pkt##.__raw[1] = (                        \
                    cpp_hi_word |                                       \
                    NFP_PCIE_DMA_CMD_CPP_ADDR_HI(curr_buf >> 21));      \
                __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt##,          \
                               NFD_IN_DATA_DMA_QUEUE,                   \
                               sig_done, &last_of_batch_dma_sig);       \
            }                                                           \
        } else {                                                        \
            /* Suppress segment DMA */                                  \
            /* Handle the DMA sequence numbers for the batch */         \
            if (_type == NFD_IN_DATA_EVENT_TYPE) {                      \
                cpp_hi_word = dma_seqn_init_event(NFD_IN_DATA_EVENT_TYPE, \
                                                  PCI_IN_ISSUE_DMA_IDX); \
                cpp_hi_word = dma_seqn_set_seqn(cpp_hi_word, _src);     \
                cpp_hi_word |= NFP_PCIE_DMA_CMD_CPP_TOKEN(              \
                    NFD_IN_DATA_DMA_TOKEN);                             \
                cpp_hi_word |=                                          \
                    NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(                     \
                        NFD_IN_DATA_CFG_REG_SIG_ONLY);                  \
                                                                        \
                dma_out.pkt##_pkt##.__raw[0] = 0;                       \
                dma_out.pkt##_pkt##.__raw[1] = cpp_hi_word;             \
                dma_out.pkt##_pkt##.__raw[2] = 0;                       \
                dma_out.pkt##_pkt##.__raw[3] = (pcie_hi_word_part |     \
                                                NFP_PCIE_DMA_CMD_LENGTH(0)); \
                cpp_hi_word |= NFP_PCIE_DMA_CMD_DMA_CFG_INDEX(          \
                    NFD_IN_DATA_CFG_REG);                               \
                __pcie_dma_enq(PCIE_ISL, &dma_out.pkt##_pkt,            \
                               NFD_IN_DATA_DMA_QUEUE,                   \
                               sig_done, &last_of_batch_dma_sig);       \
            }                                                           \
        }                                                               \
                                                                        \
        /* Set up notify message */                                     \
        /* NB: EOP is required for all packets */                       \
        /*     q_num is must be set on pkt0 */                          \
        /*     notify technically doesn't use the rest unless */        \
        /*     EOP is set */                                            \
        issued_tmp.eop = tx_desc.pkt##_pkt##.eop;                       \
        issued_tmp.offset = tx_desc.pkt##_pkt##.offset;                 \
                                                                        \
        /* Apply a standard "recipe" to complete the DMA issue */       \
        batch_out.pkt##_pkt## = issued_tmp;                             \
        batch_out.pkt##_pkt##.__raw[1] = curr_buf;                      \
        batch_out.pkt##_pkt##.__raw[2] = tx_desc.pkt##_pkt##.__raw[2];  \
        batch_out.pkt##_pkt##.__raw[3] = tx_desc.pkt##_pkt##.__raw[3];  \
                                                                        \
        /* Do final host checks and clear continuation data on EOP */   \
        if (tx_desc.pkt##_pkt##.eop) {                                  \
            int info_chk;                                               \
                                                                        \
            data_len = tx_desc.pkt##_pkt##.data_len;                    \
            offset = tx_desc.pkt##_pkt##.offset;                        \
                                                                        \
            /* Check data_len and offset match original */              \
            info_chk = offset << NFD_IN_DMA_STATE_OFFSET_shf;           \
            info_chk |= data_len;                                       \
            __asm { alu[info_chk,                                       \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_OFFSET_wrd], \
                        -, info_chk] }                                  \
            if (info_chk != 0) {                                        \
                /* The original information supplied does not */        \
                /* match the final information supplied. */             \
                curr_buf |= (1 << NFD_IN_DMA_STATE_INVALID_shf);        \
            }                                                           \
                                                                        \
            /* Check the amount of data matches the requested */        \
            __asm { alu[info_chk,                                       \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd], \
                        -, data_len] }                                  \
            if (info_chk != 0) {                                        \
                /* The amount of data actually DMAed does not match */  \
                /* the amount of data advertised.  */                   \
                curr_buf |= (1 << NFD_IN_DMA_STATE_INVALID_shf);        \
            }                                                           \
                                                                        \
            /* Rewrite the curr_buf message in case it changed above */ \
            batch_out.pkt##_pkt##.__raw[1] = curr_buf;                  \
                                                                        \
            /* Clear state data */                                      \
            NFD_IN_LSO_CNTR_INCR(                                       \
                nfd_in_lso_cntr_addr,                                   \
                NFD_IN_LSO_CNTR_T_ISSUED_NON_LSO_EOP_TX_DESC);          \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],  \
                        NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CONT_wrd],  \
                        AND~, 1, <<NFD_IN_DMA_STATE_CONT_shf] }         \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_CURR_BUF_wrd], \
                        --, B, 0] }                                     \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_BYTES_DMAED_wrd], \
                        --, B, 0] }                                     \
            __asm { alu[NFD_IN_Q_STATE_PTR[NFD_IN_DMA_STATE_OFFSET_wrd], \
                        --, B, 0] }                                     \
        }                                                               \
    }                                                                   \
                                                                        \
} while (0)


#define _ISSUE_CLR(_pkt)                                                \
do {                                                                    \
    /* Do minimal clean up so local signalling works and */             \
    /* notify block ignores the message */                              \
    batch_out.pkt##_pkt##.__raw[0] = 0;                                 \
} while (0)


/*
 * Helper macro to implement logic to determine where to branch
 * from processing packets in part-filled batches to "clearing"
 * the batch entry.
 */
#define _ISSUE_FLOW_CTRL(_num, _test, _label)       \
do {                                                \
    if (_num <= (_test + 1)) {                      \
        if (_num == _test) {                        \
            goto _label;                            \
        }                                           \
        event_type = NFD_IN_DATA_EVENT_TYPE;        \
    }                                               \
} while (0)


/**
 * Fetch batch messages from the CLS ring and process them, issuing up to
 * PCI_IN_MAX_BATCH_SZ DMAs, and placing a batch of messages onto the
 * "nfd_in_issued_ring".  Messages are only dequeued from the CLS ring when the
 * "gather_dma_seq_compl" sequence number indicates that it is safe to do so.
 * The message processing stalls until "data_dma_seq_safe" and
 * "data_dma_seq_issued" indicate that it is safe to continue.  Two ordering
 * stages ensure that packet DMAs are issued in sequence.
 */
void
issue_dma()
{
    __cls void *desc_ring_addr;
    unsigned int desc_ring_off;

    unsigned int pcie_hi_word_part;

    static __xread struct nfd_in_batch_desc batch;
    unsigned int queue;
    unsigned int num;

    for (;;) {

        reorder_test_swap(&desc_order_sig);

        /* Check "DMA" completed and we can read the batch
         * If so, the CLS ring MUST have a batch descriptor for us
         * NB: only one ctx can execute this at any given time */
        while (gather_dma_seq_compl == gather_dma_seq_serv) {
            ctx_swap(); /* Yield while waiting for work */
        }

        reorder_done_opt(&next_ctx, &desc_order_sig);

        /*
         * Increment gather_dma_seq_serv upfront to avoid ambiguity
         * about sequence number zero
         */
        gather_dma_seq_serv++;

        /* Read the batch descriptor */
        cls_ring_get(NFD_IN_BATCH_RING0_NUM + PCI_IN_ISSUE_DMA_IDX,
                     &batch, sizeof(batch), &batch_sig);

        /* Read the batch */
        desc_ring_off = ((gather_dma_seq_serv * sizeof(tx_desc)) &
                         (NFD_IN_DESC_RING_SZ - 1));
        desc_ring_addr = (__cls void *) (desc_ring_base | desc_ring_off);
        __cls_read(&tx_desc, desc_ring_addr, sizeof tx_desc, sizeof tx_desc,
                   sig_done, &tx_desc_sig);

        /* Start of dma_order_sig reorder stage */
        __asm {
            ctx_arb[--], defer[1];
            local_csr_wr[local_csr_active_ctx_wakeup_events, wait_msk];
        }

        wait_msk = __signals(&last_of_batch_dma_sig, &batch_sig, &tx_desc_sig,
                             &msg_sig0, &msg_sig1, &dma_order_sig);
        __implicit_read(&last_of_batch_dma_sig);
        __implicit_read(&msg_sig1);
        __implicit_read(&msg_sig0);
        __implicit_read(&batch_sig);
        __implicit_read(&tx_desc_sig);
        __implicit_read(&dma_order_sig);
        __implicit_read(&dma_out, sizeof dma_out);

        while ((int)(data_dma_seq_issued - data_dma_seq_safe) >= 0) {
            /* We can't process this batch yet.
             * Swap then recompute seq_safe.
             * NB: only one ctx can execute this at any given time */
            ctx_swap();
            precache_bufs_compute_seq_safe();
        }

        /* We can start to process this batch but may need to issue multiple
         * DMAs and swap for large packets, so don't let other batches start
         * just yet. */

        queue = batch.queue;
        num = batch.num;
        data_dma_seq_issued++;

        local_csr_write(local_csr_active_lm_addr_2, &queue_data[queue]);

        issued_tmp.lso_issued_cnt = 0;
        issued_tmp.num_batch = num;   /* Only needed in pkt0 */
        issued_tmp.lso_end = 0;
        issued_tmp.sp1 = 0;
        issued_tmp.q_num = queue;
        pcie_hi_word_part =
            (NFP_PCIE_DMA_CMD_RID_OVERRIDE |
             NFP_PCIE_DMA_CMD_TRANS_CLASS(NFD_IN_DATA_DMA_TRANS_CLASS) |
             NFP_PCIE_DMA_CMD_RID(queue_data[queue].rid));

        /* Maybe add "full" bit */
        if (num == 8) {
            /* Full batches are the critical path */
            /* XXX assign _ISSUE_PROC max priorty (100) on this branch. */
            _ISSUE_PROC(0, NFD_IN_DATA_IGN_EVENT_TYPE, 0, 100);
            _ISSUE_PROC(1, NFD_IN_DATA_IGN_EVENT_TYPE, 0, 100);
            _ISSUE_PROC(2, NFD_IN_DATA_IGN_EVENT_TYPE, 0, 100);
            _ISSUE_PROC(3, NFD_IN_DATA_IGN_EVENT_TYPE, 0, 100);
            _ISSUE_PROC(4, NFD_IN_DATA_IGN_EVENT_TYPE, 0, 100);
            _ISSUE_PROC(5, NFD_IN_DATA_IGN_EVENT_TYPE, 0, 100);
            _ISSUE_PROC(6, NFD_IN_DATA_IGN_EVENT_TYPE, 0, 100);
            _ISSUE_PROC(7, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued, 100);
        } else if (num <= 4) {
            static unsigned int exc_cnt = 0;

            /* We have a partial batch, so we compromise between */
            /* cycle budget and code store by handling all cases */
            /* in one semi-unwrapped loop. */
            /* _ISSUE_FLOW_CTRL() sets event_type and seqn if */
            /* _num == _test + 1, and branches out if _num == _test. */
            /* XXX assign _ISSUE_PROC lower priorty (50) on this branch. */
            unsigned int event_type = NFD_IN_DATA_IGN_EVENT_TYPE;

            exc_cnt++;
            local_csr_write(local_csr_mailbox_2, exc_cnt);

            _ISSUE_FLOW_CTRL(num, 0, batch_error);
            _ISSUE_PROC(0, event_type, data_dma_seq_issued, 50);

            _ISSUE_FLOW_CTRL(num, 1, issue_clr1);
            _ISSUE_PROC(1, event_type, data_dma_seq_issued, 50);

            _ISSUE_FLOW_CTRL(num, 2, issue_clr2);
            _ISSUE_PROC(2, event_type, data_dma_seq_issued, 50);

            if (num == 3) {
                goto issue_clr3;
            }
            _ISSUE_PROC(3, NFD_IN_DATA_EVENT_TYPE, data_dma_seq_issued, 50);
            goto issue_clr4;

        issue_clr1:
            _ISSUE_CLR(1);
        issue_clr2:
            _ISSUE_CLR(2);
        issue_clr3:
            _ISSUE_CLR(3);
        issue_clr4:
            _ISSUE_CLR(4);
            _ISSUE_CLR(5);
            _ISSUE_CLR(6);
            _ISSUE_CLR(7);

        } else {
        batch_error:
            local_csr_write(local_csr_mailbox_0,
                            NFD_IN_ISSUE_DMA_BATCH_INVALID);
            local_csr_write(local_csr_mailbox_1, batch.__raw);
            halt();
        }

        /* We have finished processing the batch, let the next continue */
        reorder_done_opt(&next_ctx, &dma_order_sig);

        ctm_ring_put(0, NFD_IN_ISSUED_RING_NUM, &batch_out.pkt0,
                     (sizeof(struct nfd_in_issued_desc) * 4), &msg_sig0);
        ctm_ring_put(0, NFD_IN_ISSUED_RING_NUM, &batch_out.pkt4,
                     (sizeof(struct nfd_in_issued_desc) * 4), &msg_sig1);
    }
}
