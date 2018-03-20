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
 * @file          blocks/vnic/pci_out/cache_desc.c
 * @brief         Code to cache FL descriptors from pending queues
 */

#include <assert.h>
#include <nfp.h>
#include <nfp_chipres.h>

#include <nfp/me.h>
#include <nfp/pcie.h>
#include <std/event.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_event.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_pcie.h>
#include <nfp6000/nfp_qc.h>

#include <vnic/nfd_common.h>
#include <vnic/pci_out.h>
#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/shared/nfd_cfg.h>
#include <vnic/utils/dma_seqn.h>
#include <vnic/utils/qc.h>
#include <vnic/utils/qcntl.h>


/* Credit define consistency checks */
#if !defined(NFD_OUT_CREDITS_HOST_ISSUED) && !defined(NFD_OUT_CREDITS_NFP_CACHED)
#error "NFD credit type not specified"
#endif

#if defined(NFD_OUT_CREDITS_HOST_ISSUED) && defined(NFD_OUT_CREDITS_NFP_CACHED)
#error "Only one NFD credit type may be specified"
#endif

#ifdef NFD_OUT_CREDITS_HOST_ISSUED
#warning "NFD_OUT_CREDITS_HOST_ISSUED selected, use at own risk!"
#endif


#define NFD_OUT_FL_SZ_PER_QUEUE   \
    (NFD_OUT_FL_BUFS_PER_QUEUE * sizeof(struct nfd_out_fl_desc))


/*
 * Allocate memory at offset 0 in CTM for atomics.
 * Forcing this to zero on all PCIe islands makes code to access credits
 * simpler and more efficient throughout the system.
 */
#define NFD_ATOMICS_ALLOC_IND(_isl, _off)                               \
    _NFP_CHIPRES_ASM(.alloc_mem nfd_out_atomics##_isl pcie##_isl##.ctm+##_off global \
                     (NFD_OUT_ATOMICS_SZ * NFD_OUT_MAX_QUEUES))
#define NFD_ATOMICS_ALLOC(_isl, _off) NFD_ATOMICS_ALLOC_IND(_isl, _off)



__shared __gpr struct qc_bitmask active_bmsk;
__shared __gpr struct qc_bitmask urgent_bmsk;
__visible volatile SIGNAL nfd_out_cache_bmsk_sig;

NFD_OUT_ACTIVE_BMSK_DECLARE;

#define NFD_OUT_WIP_STAGE_BATCH
#ifdef NFD_OUT_WIP_STAGE_BATCH
__shared __lmem unsigned int nfd_out_fl_u[NFD_OUT_MAX_QUEUES];
#endif


/*
 * Memory for PCI.OUT
 */
/*
 * For PCI.OUT ME0, queue_data is forced to LMEM 0 so that it can be
 * accessed more efficiently.  We allocate the memory via the
 * ".alloc_mem lmem+offset" notation, and then declare queue_data
 * as a pointer, which we set to zero.
 * XXX NFCC doesn't seem to acknowledge linker allocated LMEM,
 * so we also specify -Qlm_start=512 on the command line to reserve
 * the LMEM from NFCC's perspective.
 */
__asm { .alloc_mem queue_data_mem lmem+0 me             \
        (NFD_OUT_QUEUE_INFO_SZ * NFD_OUT_MAX_QUEUES)};
__shared __lmem struct nfd_out_queue_info *queue_data;

__shared __lmem unsigned int fl_cache_pending[NFD_OUT_FL_MAX_IN_FLIGHT];

/* NFD credits are fixed at offset zero in CTM */
NFD_ATOMICS_ALLOC(PCIE_ISL, NFD_OUT_CREDITS_BASE);


#define FL_CACHE_SIZE (NFD_OUT_MAX_QUEUES * NFD_OUT_FL_BUFS_PER_QUEUE * 8)

#define FL_CACHE_MEM_ALLOC_IND2(_isl, _mem)             \
    _NFP_CHIPRES_ASM(.alloc_mem fl_cache_mem##_isl _mem \
                     global FL_CACHE_SIZE FL_CACHE_SIZE);
#define FL_CACHE_MEM_ALLOC_IND1(_isl, _mem) FL_CACHE_MEM_ALLOC_IND2(_isl, _mem)
#define FL_CACHE_MEM_ALLOC_IND0(_isl)                               \
    FL_CACHE_MEM_ALLOC_IND1(_isl, NFD_PCIE##_isl##_FL_CACHE_MEM)
#define FL_CACHE_MEM_ALLOC(_isl) FL_CACHE_MEM_ALLOC_IND0(_isl)

FL_CACHE_MEM_ALLOC(PCIE_ISL);


#define FL_CACHE_MEM_IND(_isl)                      \
    ((__mem40 char *) _link_sym(fl_cache_mem##_isl))
#define FL_CACHE_MEM(_isl) FL_CACHE_MEM_IND(_isl)


static __gpr unsigned int fl_cache_mem_addr_lo;


/*
 * send_desc variables
 */
__shared __lmem struct nfd_out_send_desc_msg
    rx_desc_pending[NFD_OUT_DESC_MAX_IN_FLIGHT];

__gpr unsigned int rx_desc_mem_addr_lo;
__gpr unsigned int inc_sent_msg_addr;

static __gpr struct nfp_pcie_dma_cmd rx_descr_tmp;
__shared __gpr unsigned int desc_dma_issued = 0;
__shared __gpr unsigned int desc_dma_compl = 0;
__shared __gpr unsigned int desc_dma_served = 0;
__shared __gpr unsigned int desc_dma_pkts_issued = 0;
__shared __gpr unsigned int desc_dma_pkts_served = 0;

static volatile __xread unsigned int desc_dma_event_xfer;
static SIGNAL desc_dma_event_sig;

__shared __gpr struct qc_bitmask cached_bmsk;
__shared __gpr struct qc_bitmask pending_bmsk;


/*
 * Sequence numbers and update variables
 */
__gpr unsigned int fl_cache_dma_seq_issued = 0;
__gpr unsigned int fl_cache_dma_seq_compl = 0;
__gpr unsigned int fl_cache_dma_seq_served = 0;
static volatile __xread unsigned int fl_cache_event_xfer;
static SIGNAL fl_cache_event_sig;

static __gpr struct nfp_pcie_dma_cmd descr_tmp;


/*
 * Reserve PCIe Resources
 */
PCIE_DMA_CFG_ALLOC_OFF(nfd_out_fl_desc_dma_cfg, island, PCIE_ISL,
                       NFD_OUT_FL_CFG_REG, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_out_rx_desc_dma_cfg, island, PCIE_ISL,
                       NFD_OUT_DESC_CFG_REG, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_out_data_dma_cfg, island, PCIE_ISL,
                       NFD_OUT_DATA_CFG_REG, 1);
PCIE_DMA_CFG_ALLOC_OFF(nfd_out_data_sig_only_dma_cfg, island, PCIE_ISL,
                       NFD_OUT_DATA_CFG_REG_SIG_ONLY, 1);
PCIE_DMA_ALLOC(nfd_out_fl_desc_dma, island, PCIE_ISL, frompci_hi,
               NFD_OUT_FL_MAX_IN_FLIGHT);
PCIE_DMA_ALLOC(nfd_out_rx_desc_dma, island, PCIE_ISL, topci_med,
               NFD_OUT_DESC_MAX_IN_FLIGHT);


/**
 *Increment an atomic counter stored in local CTM
 * @param base      Start address of structure to increment
 * @param queue     Queue within structure to increment
 * @param val       Value to add
 * @param counter   Counter to increment
 *
 * XXX replace this command with suitable flowenv alternative when available.
 */
__intrinsic void
_add_imm(unsigned int base, unsigned int queue, unsigned int val,
         unsigned int counter)
{
    unsigned int ind;
    ctassert(__is_ct_const(counter));

    queue = queue * NFD_OUT_ATOMICS_SZ | counter;
    ind = (NFP_MECSR_PREV_ALU_LENGTH(8) | NFP_MECSR_PREV_ALU_OV_LEN |
           NFP_MECSR_PREV_ALU_OVE_DATA(2));

    __asm alu[--, ind, or, val, <<16];
    __asm mem[add_imm, --, base, queue, 1], indirect_ref;
}


/**
 * Zero an atomic counter stored in local CTM
 * @param base      Start address of structure to zero
 * @param queue     Queue within structure to zero
 *
 * XXX replace this command with suitable flowenv alternative when available.
 */
__intrinsic void
_zero_imm(unsigned int base, unsigned int queue, size_t size)
{
    unsigned int ind;
    unsigned int count = size>>2;

    ctassert(__is_ct_const(size));
    ctassert(size <= 32);

    queue = queue * NFD_OUT_ATOMICS_SZ;
    ind = (NFP_MECSR_PREV_ALU_LENGTH(8 + (count - 1)) |
           NFP_MECSR_PREV_ALU_OV_LEN |
           NFP_MECSR_PREV_ALU_OVE_DATA(2));

    __asm alu[--, --, B, ind];
    __asm mem[atomic_write_imm, --, base, queue, \
              __ct_const_val(count)], indirect_ref;
}


/**
 * Perform once off, CTX0-only initialisation of the FL descriptor cacher
 */
void
cache_desc_setup_shared()
{
    struct pcie_dma_cfg_one cfg;

    queue_data = 0;

    /* Zero bitmasks */
    init_bitmasks(&active_bmsk);
    init_bitmasks(&urgent_bmsk);

    dma_seqn_ap_setup(NFD_OUT_FL_EVENT_FILTER, NFD_OUT_FL_EVENT_FILTER,
                      NFD_OUT_FL_EVENT_TYPE, NFD_OUT_FL_EXT_TYPE,
                      &fl_cache_event_xfer, &fl_cache_event_sig);

    /*
     * Set up RX_FL_CFG_REG DMA Config Register
     */
    cfg.__raw = 0;
#ifdef NFD_VNIC_NO_HOST
    /* Use signal_only for seqn num generation
     * Don't actually DMA data */
    cfg.signal_only = 1;
#else
    cfg.signal_only = 0;
#endif
    cfg.end_pad     = 0;
    cfg.start_pad   = 0;
    /* Ordering settings? */
    cfg.target_64   = 1;
    cfg.cpp_target  = 7;
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_OUT_FL_CFG_REG, cfg);

    /*
     * Set up NFD_OUT_DATA_CFG_REG DMA Config Register
     * This register is used by the pci_out_pd.uc MEs.  As
     * there are multiple PD MEs and they are not within
     * the PCIe island, cache_desc takes ownership of the
     * configuration register.  FL descriptors must be
     * cached before credits can be issued for packets,
     * so this configuration must have completed before
     * the PD MEs can attempt to DMA.
     */
    cfg.__raw = 0;
#ifdef NFD_VNIC_NO_HOST
    /* Use signal_only for seqn num generation
     * Don't actually DMA data */
    cfg.signal_only = 1;
#else
    cfg.signal_only = 0;
#endif
    cfg.end_pad     = 0;
    cfg.start_pad   = 0;
    /* Ordering settings? */
    cfg.target_64   = 1;
    cfg.cpp_target  = 7;
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_OUT_DATA_CFG_REG, cfg);


    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, and PCIe address will be
     * overwritten per transaction.
     * For dma_mode, we technically only want to overwrite the "source"
     * field, i.e. 12 of the 16 bits.
     */
    descr_tmp.length = NFD_OUT_FL_BATCH_SZ * sizeof(struct nfd_out_fl_desc) - 1;
    descr_tmp.rid_override = 1;
    descr_tmp.trans_class = 0;
    descr_tmp.cpp_token = 0;
    descr_tmp.dma_cfg_index = NFD_OUT_FL_CFG_REG;
    /* cpp_addr_hi = 0 will target local CTM, as desired. */
    descr_tmp.cpp_addr_hi =
        (((unsigned long long) FL_CACHE_MEM(PCIE_ISL) >> 32) & 0xff);

    /* Initialise addresses of the FL cache and credits */
    fl_cache_mem_addr_lo =
        ((unsigned long long) FL_CACHE_MEM(PCIE_ISL) & 0xffffffff);
}


/**
 * Perform per CTX configuration of the FL descriptor cacher.
 *
 * This method populates values required by threads calling
 * "cache_desc_compute_fl_addr" as a service method.
 */
void
cache_desc_setup()
{
    fl_cache_mem_addr_lo =
        ((unsigned long long) FL_CACHE_MEM(PCIE_ISL) & 0xffffffff);
}


/**
 * Setup PCI.OUT configuration fro the vNIC specified in cfg_msg
 * @param cfg_msg   Standard configuration message
 *
 * This method handles all PCI.OUT configuration related to bringing a vNIC up
 * or down.
 */
__intrinsic void
cache_desc_vnic_setup(struct nfd_cfg_msg *cfg_msg)
{
    struct qc_queue_config rxq;
    unsigned int queue_s;
    unsigned char ring_sz;
    unsigned int ring_base[2];
    __gpr unsigned int bmsk_queue;

    nfd_cfg_proc_msg(cfg_msg, &queue_s, &ring_sz, ring_base, NFD_CFG_PCI_OUT);

    if (cfg_msg->error || !cfg_msg->interested) {
        return;
    }

    queue_s = NFD_VID2NATQ(cfg_msg->vid, queue_s);
    bmsk_queue = NFD_NATQ2BMQ(queue_s);

    rxq.watermark    = NFP_QC_STS_HI_WATERMARK_32; /* XXX Tune */
    rxq.event_data   = NFD_EVENT_DATA;
    rxq.ptr          = 0;

    if (cfg_msg->up_bit && !queue_data[bmsk_queue].up) {
        /* Up the queue:
         * - Set ring size and requester ID info
         * - (Re)clear queue pointers in case something changed them
         *   while down */
        queue_data[bmsk_queue].fl_w = 0;
        queue_data[bmsk_queue].fl_s = 0;
        queue_data[bmsk_queue].ring_sz_msk = ((1 << ring_sz) - 1);
        queue_data[bmsk_queue].requester_id = 0;
        if (NFD_VID_IS_VF(cfg_msg->vid)) {
            queue_data[bmsk_queue].requester_id = (cfg_msg->vid +
                                                   NFD_CFG_VF_OFFSET);
        }
        queue_data[bmsk_queue].spare0 = 0;
        queue_data[bmsk_queue].up = 1;
        queue_data[bmsk_queue].ring_base_hi = ring_base[1] & 0xFF;
        queue_data[bmsk_queue].ring_base_lo = ring_base[0];
        queue_data[bmsk_queue].fl_a = 0;
        queue_data[bmsk_queue].rx_s = 0;
        queue_data[bmsk_queue].rx_w = 0;

#ifdef NFD_OUT_WIP_STAGE_BATCH
        nfd_out_fl_u[bmsk_queue] = 0;
#endif

        /* Reset credits */
        _zero_imm(NFD_OUT_CREDITS_BASE, bmsk_queue, NFD_OUT_ATOMICS_SZ);

        rxq.event_type   = NFP_QC_STS_LO_EVENT_TYPE_HI_WATERMARK;
        rxq.size         = ring_sz - 8; /* XXX add define for size shift */
        qc_init_queue(PCIE_ISL, NFD_NATQ2QC(queue_s, NFD_OUT_FL_QUEUE), &rxq);

    } else if (!cfg_msg->up_bit && queue_data[bmsk_queue].up) {
        /* XXX consider what is required for PCI.OUT! */
        /* Down the queue:
         * - Prevent it issuing events
         * - Clear active_msk bit
         * - Clear pending_msk bit
         * - Clear the proc bitmask bit?
         * - Clear tx_w and tx_s
         * - Try to count pending packets? Host responsibility? */

        /* Clear all bitmask bits */
        clear_queue(&bmsk_queue, &active_bmsk);
        clear_queue(&bmsk_queue, &urgent_bmsk);
        clear_queue(&bmsk_queue, &cached_bmsk);
        clear_queue(&bmsk_queue, &pending_bmsk);

        /* Clear queue LM state */
        /* XXX check what is required for recycling host buffers */
        queue_data[bmsk_queue].fl_w = 0;
        queue_data[bmsk_queue].fl_s = 0;
        queue_data[bmsk_queue].up = 0;
        queue_data[bmsk_queue].fl_a = 0;
        queue_data[bmsk_queue].rx_s = 0;
        queue_data[bmsk_queue].rx_w = 0;

#ifdef NFD_OUT_WIP_STAGE_BATCH
        nfd_out_fl_u[bmsk_queue] = 0;
#endif

        /* Reset credits */
        _zero_imm(NFD_OUT_CREDITS_BASE, bmsk_queue, NFD_OUT_ATOMICS_SZ);

        /* Set QC queue to safe state (known size, no events, zeroed ptrs) */
        /* XXX configure both queues without swapping? */
        rxq.event_type   = NFP_QC_STS_LO_EVENT_TYPE_NEVER;
        rxq.size         = 0;
        qc_init_queue(PCIE_ISL, NFD_NATQ2QC(queue_s, NFD_OUT_FL_QUEUE), &rxq);
    }
}


/**
 * Perform checks and issue a FL batch fetch
 * @param queue     Queue selected for the fetch
 *
 * This method uses and maintains LM queue state to determine whether to fetch
 * a batch of FL descriptors.  If the state indicates that there is a batch to
 * fetch and space to put it, then the fetch will proceed.  If not, the queue
 * controller queue is reread to update the state.  The "urgent" bit for the
 * queue is also cleared by this method.
 */
__intrinsic int
_fetch_fl(__gpr unsigned int *queue)
{
    unsigned int qc_queue;
    unsigned int pcie_addr_off;
    unsigned int pcie_addr_hi_tmp, pcie_addr_lo_tmp;
    unsigned int fl_cache_off;
    __xwrite struct nfp_pcie_dma_cmd descr;
    SIGNAL qc_sig;
    int space_chk;
    int ret;        /* Required to ensure __intrinsic_begin|end pairing */

    qc_queue = NFD_NATQ2QC(NFD_BMQ2NATQ(*queue), NFD_OUT_FL_QUEUE);

    /* Is there a batch to get from this queue?
     * If the queue is active or urgent there should be. */
    if ((queue_data[*queue].fl_w - queue_data[*queue].fl_s) <
        NFD_OUT_FL_BATCH_SZ) {
        __xread unsigned int wptr_raw;
        struct nfp_qc_sts_hi wptr;
        unsigned int ptr_inc;

        /* Reread fl_w and repeat check */
        __qc_read(PCIE_ISL, qc_queue, QC_WPTR, &wptr_raw, ctx_swap, &qc_sig);
        wptr.__raw = wptr_raw;

        ptr_inc = (unsigned int) wptr.writeptr - queue_data[*queue].fl_w;
        ptr_inc &= queue_data[*queue].ring_sz_msk;
        queue_data[*queue].fl_w += ptr_inc;
#ifdef NFD__OUT_CREDITS_HOST_ISSUED
        _add_imm(NFD_OUT_CREDITS_BASE, *queue, ptr_inc, NFD_OUT_ATOMICS_CREDIT);
#endif
        if (!wptr.wmreached) {
            /* Mark the queue not urgent
             * The credit schemes ensure that when the FL buffers available are
             * depleted, the queue is not entitled to have descriptors pending.
             * The queue will be (re)marked urgent as packets on the queue
             * arrive until the final buffers and credits are exhausted.
             */
            clear_queue(queue, &urgent_bmsk);

            /* Mark the queue not active */
            clear_queue(queue, &active_bmsk);
            qc_ping_queue(PCIE_ISL, qc_queue, NFD_EVENT_DATA,
                          NFP_QC_STS_LO_EVENT_TYPE_HI_WATERMARK);

            /* Indicate work done on queue */
            return 0;
        }
    }

    /* We have a batch available, is there space to put it?
     * Space = ring size - (fl_s - rx_w). We require
     * space >= batch size. */
    space_chk = ((NFD_OUT_FL_BUFS_PER_QUEUE - NFD_OUT_FL_BATCH_SZ) +
                 queue_data[*queue].rx_w - queue_data[*queue].fl_s);
    if (space_chk >= 0) {
        __xread unsigned int qc_xfer;
        unsigned int pending_slot;
        SIGNAL dma_sig;

        /* Increment fl_cache_dma_seq_issued upfront
         * to avoid ambiguity about sequence number zero */
        fl_cache_dma_seq_issued++;

        /* Compute DMA address offsets */
        pcie_addr_off = (queue_data[*queue].fl_s &
                         queue_data[*queue].ring_sz_msk);
        pcie_addr_off = pcie_addr_off * sizeof(struct nfd_out_fl_desc);

        /* Complete descriptor */
        /* Get 40bit PCIe address using an add with carry for the offset */
        pcie_addr_hi_tmp = queue_data[*queue].ring_base_hi;
        pcie_addr_lo_tmp = queue_data[*queue].ring_base_lo;
        __asm {
            /* XXX use ASM to perform +carry op explicitly */
            /* XXX semi-colons below technically start ASM comment */
            alu[pcie_addr_lo_tmp, pcie_addr_lo_tmp, +, pcie_addr_off];
            alu[pcie_addr_hi_tmp, pcie_addr_hi_tmp, +carry, 0];
        }
        descr_tmp.pcie_addr_hi = pcie_addr_hi_tmp;
        descr_tmp.pcie_addr_lo = pcie_addr_lo_tmp;

        /* Handle CPP portion of the descriptor */
        descr_tmp.cpp_addr_lo =
            cache_desc_compute_fl_addr(queue, queue_data[*queue].fl_s);
        descr_tmp.rid = queue_data[*queue].requester_id;
        /* Can replace with ld_field instruction if 8bit seqn is enough */
        dma_seqn_set_event(&descr_tmp, NFD_OUT_FL_EVENT_TYPE,
                           NFD_OUT_FL_EXT_TYPE, fl_cache_dma_seq_issued);
        descr = descr_tmp;

        /* Increment fl_s and QC FL.R before swapping
         * It is safe to increment fl_s because the host will not
         * overwrite the FL descriptor there, the firmware will overwrite
         * it once it has used the FL descriptor and written the RX descriptor.
         */
        queue_data[*queue].fl_s += NFD_OUT_FL_BATCH_SZ;
        __qc_add_to_ptr(PCIE_ISL, qc_queue, QC_RPTR, NFD_OUT_FL_BATCH_SZ,
                        &qc_xfer, sig_done, &qc_sig);

        /* Add batch message to LM queue
         * XXX check defer slots filled */
        pending_slot = (fl_cache_dma_seq_issued & (NFD_OUT_FL_MAX_IN_FLIGHT -1));
        fl_cache_pending[pending_slot] = *queue;

        /* Issue DMA */
        __pcie_dma_enq(PCIE_ISL, &descr, NFD_OUT_FL_DMA_QUEUE,
                       sig_done, &dma_sig);
        wait_for_all(&dma_sig, &qc_sig);
        __implicit_read(&qc_xfer);

        /* Indicate work done on queue */
        ret = 0;
    } else {
        /* The cache is full, so the queue is not "urgent". */
        clear_queue(queue, &urgent_bmsk);

        /* Swap to give other threads a chance to run */
        ctx_swap();

        /* Indicate no work done on queue */
        ret = -1;
    }

    return ret;
}


/**
 * Check whether fl_cache_dma_seq_compl can be advanced and, if so, process
 * the messages in the fl_cache_pending queue.  Two dependent LM accesses are
 * required to process each message, so cycles lost to LM pointer setup are
 * hard to avoid.
 */
void
_complete_fetch()
{
    __gpr unsigned int queue_c;
    unsigned int pending_slot;

    if (signal_test(&fl_cache_event_sig)) {
        dma_seqn_advance(&fl_cache_event_xfer, &fl_cache_dma_seq_compl);

        __implicit_write(&fl_cache_event_sig);
        event_cls_autopush_filter_reset(
            NFD_OUT_FL_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            NFD_OUT_FL_EVENT_FILTER);

        /* XXX how many updates can we receive at once? Do we need to
         * throttle this? */
        while (fl_cache_dma_seq_compl != fl_cache_dma_seq_served) {
            /* Increment fl_cache_dma_seq_served upfront
             * to avoid ambiguity about sequence number zero */
            fl_cache_dma_seq_served++;

            /* Extract queue from the fl_cache_pending message */
            pending_slot = (fl_cache_dma_seq_served &
                            (NFD_OUT_FL_MAX_IN_FLIGHT -1));
            queue_c = fl_cache_pending[pending_slot];

#ifdef NFD_OUT_CREDITS_NFP_CACHED
            _add_imm(NFD_OUT_CREDITS_BASE, queue_c, NFD_OUT_FL_BATCH_SZ,
                     NFD_OUT_ATOMICS_CREDIT);
#endif

            /* Increment queue available pointer by one batch
             * NB: If NFP cached credits are not used, there is nothing to
             * fill the LM pointer usage slots */
            queue_data[queue_c].fl_a += NFD_OUT_FL_BATCH_SZ;

            /* Set the queue in the cached_bmsk for send_desc */
            set_queue(&queue_c, &cached_bmsk);
        }
    } else {
        /* Swap to give other threads a chance to run */
        ctx_swap();
    }
}


/**
 * Check the bitmask filters and process completed fetches
 */
__forceinline void
cache_desc_complete_fetch()
{
    if (signal_test(&nfd_out_cache_bmsk_sig)) {
        __xrw unsigned int update[2] = {0xffffffff, 0xffffffff};

        mem_test_clr(update, NFD_OUT_ACTIVE_BMSK_LINK, sizeof update);
        active_bmsk.bmsk_lo |= update[0];
        active_bmsk.bmsk_hi |= update[1];
    } else {
        ctx_swap();
    }


    /* Process up to the latest fl_cache_dma_seq_compl */
    _complete_fetch();
}


/**
 * Check the active bitmask for a queue to service, and
 * attempt to work on that queue.  The "urgent" and "active"
 * bitmasks are updated as processing progresses.
 *
 * This method may be called multiple separate times from the
 * dispatch loop for finer balance of FL DMAs with other tasks.
 */
__forceinline void
cache_desc_check_active()
{
    __gpr unsigned int queue;
    int ret = 0;

    if ((fl_cache_dma_seq_issued - fl_cache_dma_seq_served) <
        NFD_OUT_FL_MAX_IN_FLIGHT) {
        __critical_path();

        /* Look for an active queue */
        ret = select_queue(&queue, &active_bmsk);
        if (ret) {
            /* No active queues found */
            return;
        }

        /* Try to work on that queue */
        ret = _fetch_fl(&queue);
    }
}


/**
 * Check the urgent bitmask for a queue to service, and
 * attempt to work on that queue.  The "urgent" and "active"
 * bitmasks are updated as processing progresses.
 *
 * This method may be called multiple separate times from the
 * dispatch loop for finer balance of FL DMAs with other tasks.
 */
__forceinline void
cache_desc_check_urgent()
{
    __gpr unsigned int queue;
    int ret = 0;

    if ((fl_cache_dma_seq_issued - fl_cache_dma_seq_served) <
        NFD_OUT_FL_MAX_IN_FLIGHT) {
        __critical_path();

        /* Look for an urgent queue */
        ret = select_queue(&queue, &urgent_bmsk);
        if (ret) {
            /* No urgent queues found */
            return;
        }

        /* Try to work on that queue */
        ret = _fetch_fl(&queue);
    }
}


/**
 * Service function to determine the address of a specific FL entry
 * @param queue     Bitmask numbered queue
 * @param seq       Current "fl_u" sequence number for the queue
 */
__intrinsic unsigned int
cache_desc_compute_fl_addr(__gpr unsigned int *queue, unsigned int seq)
{
    unsigned int ret;

    ret = seq & (NFD_OUT_FL_BUFS_PER_QUEUE - 1);
    ret *= sizeof(struct nfd_out_fl_desc);
    ret |= (*queue * NFD_OUT_FL_SZ_PER_QUEUE );
    ret |= fl_cache_mem_addr_lo;

    return ret;
}




/**
 * Perform once off, CTX0-only initialisation of the send_desc DMA config
 */
void
send_desc_setup_shared()
{
    struct pcie_dma_cfg_one cfg;

    /* Zero bitmasks */
    init_bitmasks(&cached_bmsk);
    init_bitmasks(&pending_bmsk);

    dma_seqn_ap_setup(NFD_OUT_DESC_EVENT_FILTER, NFD_OUT_DESC_EVENT_FILTER,
                      NFD_OUT_DESC_EVENT_TYPE, NFD_OUT_DESC_EXT_TYPE,
                      &desc_dma_event_xfer, &desc_dma_event_sig);

    /*
     * Set up RX_FL_CFG_REG DMA Config Register
     */
    cfg.__raw = 0;
#ifdef NFD_VNIC_NO_HOST
    /* Use signal_only for seqn num generation
     * Don't actually DMA data */
    cfg.signal_only = 1;
#else
    cfg.signal_only = 0;
#endif
    cfg.end_pad     = 0;
    cfg.start_pad   = 0;
    /* Ordering settings? */
    cfg.target_64   = 1;
    cfg.cpp_target  = 7;
    pcie_dma_cfg_set_one(PCIE_ISL, NFD_OUT_DESC_CFG_REG, cfg);

    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, and PCIe address and length
     * will be overwritten per transaction.
     * For dma_mode, we technically only want to overwrite the "source"
     * field, i.e. 12 of the 16 bits.
     */
    rx_descr_tmp.rid_override = 1;
    rx_descr_tmp.trans_class = 0;
    rx_descr_tmp.cpp_token = 0;
    rx_descr_tmp.dma_cfg_index = NFD_OUT_DESC_CFG_REG;
    /* cpp_addr_hi = 0 will target local CTM, as desired. */
    rx_descr_tmp.cpp_addr_hi =
        (((unsigned long long) FL_CACHE_MEM(PCIE_ISL) >> 32) & 0xff);
}


__intrinsic void
_start_send(__gpr unsigned int *queue)
{
    __xread unsigned int dma_done;
    unsigned int atomic_addr;
    SIGNAL atomic_sig;
    SIGNAL dma_sig;
    SIGNAL_MASK dma_sig_msk = 0;
    unsigned int rx_s;
    __xwrite struct nfp_pcie_dma_cmd descr;


    /* Read DMA done atomic */
    /* If we have picked a queue to service, we always read its atomics.
     * The queue may be down, or go down after we issue the read.  In either
     * case, we will unflag its state so that it doesn't get checked again. */
    atomic_addr = (*queue * NFD_OUT_ATOMICS_SZ) | NFD_OUT_ATOMICS_DMA_DONE;
    __asm mem[atomic_read, dma_done, 0, atomic_addr, 1], ctx_swap[atomic_sig];


    /* Check that the queue is up and abort processing if down,
     * clearing state */
    if (queue_data[*queue].up) {
        /* Check whether rx_s can be advanced */
        rx_s = queue_data[*queue].rx_s;
        if (rx_s != dma_done) {
            unsigned int rx_s_max;
            int dma_batch_correction;
            unsigned int dma_batch;
            unsigned int pcie_addr_off;
            unsigned int pcie_addr_hi_tmp, pcie_addr_lo_tmp;
            unsigned int dma_length;
            unsigned int pending_slot;
            struct nfd_out_send_desc_msg send_msg;

            /* Increment desc_dma_issued upfront
             * to avoid ambiguity about sequence number zero */
            desc_dma_issued++;

            /*
             * Compute the DMA batch size, strategy:
             * Round rx_s up to the next NFD_OUT_DESC_MAX_BATCH_SZ multiple
             * (1024B multiple).  This provides rx_s_max.  We then compute the
             * dma_batch that would produce this rx_s.  Subtracting the
             * rx_s_max from dma_done produces a correction value.
             * If positive or zero, leave dma_batch unchanged.  If negative,
             * correct rx_s and dma_batch by adding the negative error.
             */
            rx_s_max = NFD_OUT_DESC_MAX_BATCH_SZ + rx_s;
            rx_s_max &= ~(NFD_OUT_DESC_MAX_BATCH_SZ - 1);
            dma_batch_correction = dma_done - rx_s_max;
            dma_batch = (NFD_OUT_DESC_MAX_BATCH_SZ -
                         (rx_s & (NFD_OUT_DESC_MAX_BATCH_SZ - 1)));
            if (dma_batch_correction < 0) {
                dma_batch += dma_batch_correction;
            }

            pcie_addr_off = rx_s & queue_data[*queue].ring_sz_msk;
            pcie_addr_off = pcie_addr_off * sizeof(struct nfd_out_rx_desc);

            /* Complete descriptor */
            dma_length = (dma_batch * sizeof(struct nfd_out_rx_desc)) - 1;
            rx_descr_tmp.length = dma_length;
            /* Get 40bit PCIe address using an add with carry for the offset */
            pcie_addr_hi_tmp = queue_data[*queue].ring_base_hi;
            pcie_addr_lo_tmp = queue_data[*queue].ring_base_lo;
            __asm {
                /* XXX use ASM to perform +carry op explicitly */
                /* XXX semi-colons below technically start ASM comment */
                alu[pcie_addr_lo_tmp, pcie_addr_lo_tmp, +, pcie_addr_off];
                alu[pcie_addr_hi_tmp, pcie_addr_hi_tmp, +carry, 0];
            }
            rx_descr_tmp.pcie_addr_hi = pcie_addr_hi_tmp;
            rx_descr_tmp.pcie_addr_lo = pcie_addr_lo_tmp;

            /* Handle CPP portion of the descriptor */
            rx_descr_tmp.cpp_addr_lo =
                cache_desc_compute_fl_addr(queue, rx_s);
            rx_descr_tmp.rid = queue_data[*queue].requester_id;
            /* Can replace with ld_field instruction if 8bit seqn is enough */
            dma_seqn_set_event(&rx_descr_tmp, NFD_OUT_DESC_EVENT_TYPE,
                               NFD_OUT_DESC_EXT_TYPE, desc_dma_issued);
            descr = rx_descr_tmp;

            /* Increment rx_s and desc_dma_pkts_served */
            rx_s += dma_batch;
            desc_dma_pkts_issued += dma_batch;

            /* Add batch message to LM queue
             * XXX check defer slots filled */
            pending_slot = (desc_dma_issued & (NFD_OUT_DESC_MAX_IN_FLIGHT -1));
            send_msg.spare = 0;
            send_msg.count = dma_batch;
            send_msg.queue = *queue;
            rx_desc_pending[pending_slot] = send_msg;

            /* Issue DMA */
            dma_sig_msk = __signals(&dma_sig);
            __pcie_dma_enq(PCIE_ISL, &descr, NFD_OUT_DESC_DMA_QUEUE,
                           sig_done, &dma_sig);

        }

        /* Adjust the queue pending flag if necessary.
         * "pending" is set if our previous read still shows work to do. */
        if (rx_s == dma_done) {
            clear_queue(queue, &pending_bmsk);
        } else {
            set_queue(queue, &pending_bmsk);
        }

        /* Flag the queue urgent if necessary.  This provides feedback to
         * cache_desc about which queues have been used. */
        if (queue_data[*queue].fl_a - rx_s < NFD_OUT_FL_SOFT_THRESH) {
            set_queue(queue, &urgent_bmsk);

            if (queue_data[*queue].fl_a == rx_s) {
                /* We don't have any credits cached for this queue,
                 * so can't receive any more packets from it. */
                clear_queue(queue, &cached_bmsk);
            }
        }

        /* Write back the rx_s value to LM */
        queue_data[*queue].rx_s = rx_s;

    } else {
        /* The queue is down.  Clear the cached and pending state so that
         * it will not get checked again, but leave everything else
         * unchanged. */
        clear_queue(queue, &cached_bmsk);
        clear_queue(queue, &pending_bmsk);

        /* Swap to keep context swaps balanced in various paths */
        ctx_swap();
    }

    /* Wait on the DMA enqueue signal via a mask in case it wasn't issued */
    wait_sig_mask(dma_sig_msk);
    __implicit_read(&dma_sig);
}



/**
 * Check the cached bitmask for a queue to service, and
 * attempt to work on that queue.  The "cached" and "pending"
 * bitmasks are updated as processing progresses.  The "urgent"
 * bitmask is also updated as feedback to cache_desc.
 *
 * This method may be called multiple separate times from the
 * dispatch loop for finer balance of RX DMAs with other tasks.
 */
__forceinline void
send_desc_check_cached()
{
    __gpr unsigned int queue;
    int ret = 0;

    if ((desc_dma_issued - desc_dma_served) < NFD_OUT_DESC_MAX_IN_FLIGHT) {
        __critical_path();

        /* Look for a cached queue */
        ret = select_queue(&queue, &cached_bmsk);
        if (ret) {
            /* No cached queues found */
            return;
        }

        /* Try to work on that queue */
        _start_send(&queue);
    }
}


/**
 * Check the pending bitmask for a queue to service, and
 * attempt to work on that queue.  The "cached" and "pending"
 * bitmasks are updated as processing progresses.  The "urgent"
 * bitmask is also updated as feedback to cache_desc.
 *
 * This method may be called multiple separate times from the
 * dispatch loop for finer balance of RX DMAs with other tasks.
 */
__forceinline void
send_desc_check_pending()
{
    __gpr unsigned int queue;
    int ret = 0;

    if ((desc_dma_issued - desc_dma_served) < NFD_OUT_DESC_MAX_IN_FLIGHT) {
        __critical_path();

        /* Look for a pending queue */
        ret = select_queue(&queue, &pending_bmsk);
        if (ret) {
            /* No pending queues found */
            return;
        }

        /* Try to work on that queue */
        _start_send(&queue);
    }
}


/**
 * Check the desc_dma autopush, and if DMAs have completed, process the
 * send_desc message ring.  We increment NFD_OUT_ATOMICS_SENT for each
 * message to notify the MSIX generation code about the completed DMAs.
 * We do _not_ check whether each queue is up at this stage, if it had
 * been down when we were processing the send, the send would have been
 * aborted.  The MSIX code ignores sent packet counts on down queues.
 */
__intrinsic void
send_desc_complete_send()
{
    struct nfd_out_send_desc_msg send_msg;
    unsigned int pending_slot;

    if (signal_test(&desc_dma_event_sig)) {
        dma_seqn_advance(&desc_dma_event_xfer, &desc_dma_compl);

        __implicit_write(&desc_dma_event_sig);
        event_cls_autopush_filter_reset(
            NFD_OUT_DESC_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            NFD_OUT_DESC_EVENT_FILTER);

        /* XXX how many updates can we receive at once? Do we need to
         * throttle this? */
        while (desc_dma_compl != desc_dma_served) {
            /* Increment desc_dma_served upfront
             * to avoid ambiguity about sequence number zero */
            desc_dma_served++;

            /* Extract queue and count from the rx_desc_pending message */
            pending_slot = (desc_dma_served &
                            (NFD_OUT_DESC_MAX_IN_FLIGHT -1));
            send_msg = rx_desc_pending[pending_slot];

            /* Increment sent counters */
            queue_data[send_msg.queue].rx_w += send_msg.count;
            _add_imm(NFD_OUT_CREDITS_BASE, send_msg.queue, send_msg.count,
                     NFD_OUT_ATOMICS_SENT);
            desc_dma_pkts_served += send_msg.count;
        }

    } else {
        /* Swap to give other threads a chance to run */
        ctx_swap();
    }
}
