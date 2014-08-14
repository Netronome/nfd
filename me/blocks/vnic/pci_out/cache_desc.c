/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_out/cache_desc.c
 * @brief         Code to cache FL descriptors from pending queues
 */

#include <assert.h>
#include <nfp.h>

#include <std/event.h>

#include <nfp6000/nfp_cls.h>
#include <nfp6000/nfp_event.h>
#include <nfp6000/nfp_me.h>
#include <nfp6000/nfp_qc.h>

#include <vnic/pci_out/cache_desc.h>

#include <vnic/pci_out.h>
#include <vnic/pci_out_cfg.h>
#include <vnic/pci_out/pci_out_internal.h>
#include <vnic/shared/qc.h>
#include <vnic/shared/vnic_cfg.h>
#include <vnic/utils/dma_seqn.h>
#include <vnic/utils/pcie.h>
#include <vnic/utils/qcntl.h>


/* #define NFD_PCI_OUT_CREDITS_HOST_ISSUED */
#define NFD_PCI_OUT_CREDITS_NFP_CACHED

#define RX_FL_CACHE_SZ_PER_QUEUE   \
    (RX_FL_CACHE_BUFS_PER_QUEUE * sizeof(struct nfd_pci_out_fl_desc))

/*
 * State variables for PCI.OUT queue controller accesses
 */
static __xread struct qc_xfers rx_ap_xfers;

static volatile SIGNAL rx_ap_s0;
static volatile SIGNAL rx_ap_s1;
static volatile SIGNAL rx_ap_s2;
static volatile SIGNAL rx_ap_s3;

__shared __gpr struct qc_bitmask active_bmsk;
__shared __gpr struct qc_bitmask urgent_bmsk;


/*
 * Memory for PCI.OUT
 */
__shared __lmem struct rx_queue_info queue_data[MAX_RX_QUEUES];

static __shared __lmem unsigned int fl_cache_pending[RX_FL_FETCH_MAX_IN_FLIGHT];

/* NB: MAX_RX_QUEUES * sizeof(unsigned int) <= 256 */
__export __ctm __align256 unsigned int nfd_pci_out_credits[MAX_RX_QUEUES];

__export __ctm __align(MAX_RX_QUEUES * RX_FL_CACHE_SZ_PER_QUEUE)
    struct nfd_pci_out_fl_desc
    fl_cache_mem[MAX_RX_QUEUES][RX_FL_CACHE_BUFS_PER_QUEUE];

static __gpr unsigned int fl_cache_mem_addr_lo;
static __gpr unsigned int fl_cache_credits_base = 0;


/*
 * Sequence numbers and update variables
 */
static __gpr unsigned int fl_cache_dma_seq_issued = 0;
static __gpr unsigned int fl_cache_dma_seq_compl = 0;
static __gpr unsigned int fl_cache_dma_seq_served = 0;
static volatile __xread unsigned int fl_cache_event_xfer;
static SIGNAL fl_cache_event_sig;

static __gpr struct nfp_pcie_dma_cmd descr_tmp;


/**
 * Access the MEM atomic "add_imm" instruction
 * @param base      Start address of structure to increment
 * @param offset    Offset within structure to increment
 * @param val       Value to add
 *
 * XXX replace this command with suitable flowenv alternative when available.
 */
__intrinsic void
_add_imm(unsigned int base, unsigned int offset, unsigned int val)
{
    unsigned int ind;

    ind = (NFP_MECSR_PREV_ALU_LENGTH(8) | NFP_MECSR_PREV_ALU_OV_LEN |
           NFP_MECSR_PREV_ALU_OVE_DATA(2));

    __asm alu[--, ind, or, val, <<16];
    __asm mem[add_imm, --, base, <<8, offset, 1], indirect_ref;
}


/**
 * Zero data using MEM atomic engine
 * @param base      Start address of structure to zero
 * @param offset    Offset within structure to zero
 *
 * XXX replace this command with suitable flowenv alternative when available.
 */
__intrinsic void
_zero_imm(unsigned int base, unsigned int offset)
{
    unsigned int ind;

    ind = (NFP_MECSR_PREV_ALU_LENGTH(8) | NFP_MECSR_PREV_ALU_OV_LEN |
           NFP_MECSR_PREV_ALU_OVE_DATA(2));

    __asm alu[--, --, B, ind];
    __asm mem[atomic_write_imm, --, base, <<8, offset, 1], indirect_ref;
}


/**
 * Perform once off, CTX0-only initialisation of the FL descriptor cacher
 */
void
cache_desc_setup_shared()
{
    struct pcie_dma_cfg_one cfg;

    /* Zero bitmasks */
    init_bitmasks(&active_bmsk);
    init_bitmasks(&urgent_bmsk);

    /* Configure RXQ autopush filters */
    init_bitmask_filters(&rx_ap_xfers, &rx_ap_s0, &rx_ap_s1, &rx_ap_s2,
                         &rx_ap_s3,(RXQ_EVENT_DATA<<6) | RXQ_START,
                         NFP_EVENT_TYPE_FIFO_ABOVE_WM,
                         RXQ_EVENT_START);

    dma_seqn_ap_setup(RX_FL_FETCH_EVENT_FILTER, RX_FL_FETCH_EVENT_FILTER,
                      RX_FL_FETCH_EVENT_TYPE, &fl_cache_event_xfer,
                      &fl_cache_event_sig);

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
    pcie_dma_cfg_set_one(PCIE_ISL, RX_FL_CFG_REG, cfg);


    /*
     * Initialise a DMA descriptor template
     * RequesterID (rid), CPP address, and PCIe address will be
     * overwritten per transaction.
     * For dma_mode, we technically only want to overwrite the "source"
     * field, i.e. 12 of the 16 bits.
     */
    descr_tmp.length = RX_FL_BATCH_SZ * sizeof(struct nfd_pci_out_fl_desc) - 1;
    descr_tmp.rid_override = 1;
    descr_tmp.trans_class = 0;
    descr_tmp.cpp_token = 0;
    descr_tmp.dma_cfg_index = RX_FL_CFG_REG;
    descr_tmp.cpp_addr_hi = (((unsigned long long) fl_cache_mem >> 8) &
                             0xff000000);

    /* Initialise addresses of the FL cache and credits */
    fl_cache_mem_addr_lo = ((unsigned long long) fl_cache_mem & 0xffffffff);
    fl_cache_credits_base = (((unsigned long long) nfd_pci_out_credits >> 8) &
                             0xffffffff);
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
    fl_cache_mem_addr_lo = ((unsigned long long) fl_cache_mem & 0xffffffff);
}


/**
 * Setup PCI.OUT configuration fro the vNIC specified in cfg_msg
 * @param cfg_msg   Standard configuration message
 *
 * This method handles all PCI.OUT configuration related to bringing a vNIC up
 * or down.
 */
__intrinsic void
cache_desc_vnic_setup(struct vnic_cfg_msg *cfg_msg)
{
    struct qc_queue_config rxq;
    unsigned int queue_s;
    unsigned char ring_sz;
    unsigned int ring_base[2];
    __gpr unsigned int bmsk_queue;

    vnic_cfg_proc_msg(cfg_msg, &queue_s, &ring_sz, ring_base, VNIC_CFG_PCI_OUT);

    if (cfg_msg->error || !cfg_msg->interested) {
        return;
    }

    queue_s += cfg_msg->vnic * MAX_VNIC_QUEUES;
    bmsk_queue = map_natural_to_bitmask(queue_s);

    rxq.watermark    = NFP_QC_STS_HI_WATERMARK_8; /* XXX use 16 instead? */
    rxq.event_data   = RXQ_EVENT_DATA;
    rxq.ptr          = 0;

    if (cfg_msg->up_bit) {
        /* Up the queue:
         * - Set ring size and requester ID info
         * - (Re)clear queue pointers in case something changed them
         *   while down */
        queue_data[bmsk_queue].fl_w = 0;
        queue_data[bmsk_queue].fl_s = 0;
        queue_data[bmsk_queue].ring_sz_msk = ((1 << ring_sz) - 1);
        queue_data[bmsk_queue].requester_id = cfg_msg->vnic;
        queue_data[bmsk_queue].spare0 = 0;
        queue_data[bmsk_queue].up = 1;
        queue_data[bmsk_queue].ring_base_hi = ring_base[1] & 0xFF;
        queue_data[bmsk_queue].ring_base_lo = ring_base[0];
        queue_data[bmsk_queue].fl_a = 0;
        queue_data[bmsk_queue].fl_u = 0;
        queue_data[bmsk_queue].rx_w = 0;

        /* Reset credits */
        _zero_imm(fl_cache_credits_base, bmsk_queue);

        rxq.event_type   = NFP_QC_STS_LO_EVENT_TYPE_HI_WATERMARK;
        rxq.size         = ring_sz - 8; /* XXX add define for size shift */
        qc_init_queue(PCIE_ISL, (queue_s<<1) | RXQ_START, &rxq);

    } else {
        /* XXX consider what is required for PCI.OUT! */
        /* Down the queue:
         * - Prevent it issuing events
         * - Clear active_msk bit
         * - Clear pending_msk bit
         * - Clear the proc bitmask bit?
         * - Clear tx_w and tx_s
         * - Try to count pending packets? Host responsibility? */

        /* Clear active and urgent bitmask bits */
        clear_queue(&bmsk_queue, &active_bmsk);
        clear_queue(&bmsk_queue, &urgent_bmsk);

        /* Clear queue LM state */
        /* XXX check what is required for recycling host buffers */
        queue_data[bmsk_queue].fl_w = 0;
        queue_data[bmsk_queue].fl_s = 0;
        queue_data[bmsk_queue].up = 0;
        queue_data[bmsk_queue].fl_a = 0;
        queue_data[bmsk_queue].fl_u = 0;
        queue_data[bmsk_queue].rx_w = 0;

        /* Reset credits */
        _zero_imm(fl_cache_credits_base, bmsk_queue);

        /* Set QC queue to safe state (known size, no events, zeroed ptrs) */
        rxq.event_type   = NFP_QC_STS_LO_EVENT_TYPE_NEVER;
        rxq.size         = 0;
        qc_init_queue(PCIE_ISL, (queue_s<<1) | RXQ_START, &rxq);
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
    unsigned int fl_cache_off;
    __xwrite struct nfp_pcie_dma_cmd descr;
    SIGNAL qc_sig;
    int space_chk;

    qc_queue = (map_bitmask_to_natural(*queue) << 1) | RXQ_START;

    /* Is there a batch to get from this queue?
     * If the queue is active or urgent there should be. */
    if ((queue_data[*queue].fl_w - queue_data[*queue].fl_s) < RX_FL_BATCH_SZ) {
        __xread unsigned int wptr_raw;
        struct nfp_qc_sts_hi wptr;
        unsigned int ptr_inc;

        /* Reread fl_w and repeat check */
        __qc_read(PCIE_ISL, qc_queue, QC_WPTR, &wptr_raw, ctx_swap, &qc_sig);
        wptr.__raw = wptr_raw;

        ptr_inc = (unsigned int) wptr.writeptr - queue_data[*queue].fl_w;
        ptr_inc &= queue_data[*queue].ring_sz_msk;
        queue_data[*queue].fl_w += ptr_inc;
#ifdef NFD_PCI_OUT_CREDITS_HOST_ISSUED
        _add_imm(fl_cache_credits_base, *queue * 4, ptr_inc);
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
            qc_ping_queue(PCIE_ISL, qc_queue, RXQ_EVENT_DATA,
                          NFP_QC_STS_LO_EVENT_TYPE_HI_WATERMARK);

            /* Indicate work done on queue */
            return 0;
        }
    }

    /* We have a batch available, is there space to put it?
     * Space = ring size - (fl_s - fl_u). We require
     * space >= batch size. */
    space_chk = ((RX_FL_CACHE_BUFS_PER_QUEUE - RX_FL_BATCH_SZ) +
                 queue_data[*queue].fl_u - queue_data[*queue].fl_s);
    if (space_chk >= 0) {
        __xwrite unsigned int qc_xfer;
        unsigned int pending_slot;
        SIGNAL dma_sig;

        /* Increment fl_cache_dma_seq_issued upfront
         * to avoid ambiguity about sequence number zero */
        fl_cache_dma_seq_issued++;

        /* Compute DMA address offsets */
        pcie_addr_off = (queue_data[*queue].fl_s &
                         queue_data[*queue].ring_sz_msk);
        pcie_addr_off = pcie_addr_off * sizeof(struct nfd_pci_out_fl_desc);

        /* Complete descriptor */
        descr_tmp.pcie_addr_hi = queue_data[*queue].ring_base_hi;
        descr_tmp.pcie_addr_lo = (queue_data[*queue].ring_base_lo |
                                  pcie_addr_off);

        descr_tmp.cpp_addr_lo =
            cache_desc_compute_fl_addr(queue, queue_data[*queue].fl_s);
        descr_tmp.rid = queue_data[*queue].requester_id;
        /* Can replace with ld_field instruction if 8bit seqn is enough */
        pcie_dma_set_event(&descr_tmp, RX_FL_FETCH_EVENT_TYPE,
                           fl_cache_dma_seq_issued);
        descr = descr_tmp;

        /* Increment fl_s and QC FL.R before swapping */
        queue_data[*queue].fl_s += RX_FL_BATCH_SZ;
        __qc_add_to_ptr(PCIE_ISL, qc_queue, QC_RPTR, RX_FL_BATCH_SZ, &qc_xfer,
                        sig_done, &qc_sig);

        /* Add batch message to LM queue
         * XXX check defer slots filled */
        pending_slot = fl_cache_dma_seq_issued & (RX_FL_FETCH_MAX_IN_FLIGHT -1);
        fl_cache_pending[pending_slot] = *queue;

        /* Issue DMA */
        __pcie_dma_enq(PCIE_ISL, &descr, RX_FL_FETCH_DMA_QUEUE,
                       sig_done, &dma_sig);
        wait_for_all(&dma_sig, &qc_sig);

        /* Indicate work done on queue */
        return 0;
    }
    /* XXX clear urgent bit if the queue FL cache has become full! */

    /* Indicate no work done on queue */
    return -1;
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
    unsigned int queue_c;
    unsigned int pending_slot;

    if (signal_test(&fl_cache_event_sig)) {
        dma_seqn_advance(&fl_cache_event_xfer, &fl_cache_dma_seq_compl);

        event_cls_autopush_filter_reset(
            RX_FL_FETCH_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            RX_FL_FETCH_EVENT_FILTER);
        __implicit_write(&fl_cache_event_sig);

        /* XXX how many updates can we receive at once? Do we need to
         * throttle this? */
        while (fl_cache_dma_seq_compl != fl_cache_dma_seq_served) {
            /* Increment fl_cache_dma_seq_served upfront
             * to avoid ambiguity about sequence number zero */
            fl_cache_dma_seq_served++;

            /* Extract queue from the fl_cache_pending message */
            pending_slot = (fl_cache_dma_seq_served &
                            (RX_FL_FETCH_MAX_IN_FLIGHT -1));
            queue_c = fl_cache_pending[pending_slot];

#ifdef NFD_PCI_OUT_CREDITS_NFP_CACHED
            _add_imm(fl_cache_credits_base, queue_c * 4, RX_FL_BATCH_SZ);
#endif

            /* Increment queue available pointer by one batch
             * NB: If NFP cached credits are not used, there is nothing to
             * fill the LM pointer usage slots */
            queue_data[queue_c].fl_a += RX_FL_BATCH_SZ;
        }
    }
}


/**
 * Cache FL descriptors
 *
 * This method implements the overall caching strategy, calling
 * "_complete_fetch" and "_fetch_fl" for detailed implementation.
 *
 * The general strategy is to poll up to RX_MAX_RETRIES queues via
 * bitmasks to identify queues that may have pending FL entries.
 * Queues are selected from the "urgent" bitmask first, and then the
 * "active" bitmask.
 *
 * This method also updates the "active" bitmask, and checks for completed
 * fetches.
 */
void
cache_desc()
{
    __gpr unsigned int queue;
    unsigned int count = 0;
    int ret = 0;

    /* Check bitmasks */
    check_bitmask_filters(&active_bmsk, &rx_ap_xfers, &rx_ap_s0, &rx_ap_s1,
              &rx_ap_s2, &rx_ap_s3, RXQ_EVENT_START);

    /* Process up to the latest fl_cache_dma_seq_compl */
    _complete_fetch();

    if ((fl_cache_dma_seq_issued - fl_cache_dma_seq_served) <
        RX_FL_FETCH_MAX_IN_FLIGHT) {
        __critical_path();

        /* Check urgent queues */
        do {
            count++;

            /* Look for an urgent queue */
            ret = select_queue(&queue, &urgent_bmsk);
            if (ret) {
                /* No urgent queues found */
                break;
            }

            /* Try to work on that queue */
            ret = _fetch_fl(&queue);
            if (ret == 0) {
                /* Work has been done on a queue */
                break;
            }
        } while (count < RX_MAX_RETRIES);

        /* Check active queues */
        while (count < RX_MAX_RETRIES) {
            count++;

            /* Simultaneously test last active test and prior urgent tests */
            if (ret == 0) {
                /* Work has been done on a queue */
                break;
            }

            /* Look for an active queue */
            ret = select_queue(&queue, &active_bmsk);
            if (ret) {
                /* No active queues found */
                break;
            }

            /* Try to work on that queue */
            ret = _fetch_fl(&queue);
        }
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

    ret = seq & (RX_FL_CACHE_BUFS_PER_QUEUE - 1);
    ret *= sizeof(struct nfd_pci_out_fl_desc);
    ret |= (*queue * RX_FL_CACHE_SZ_PER_QUEUE );
    ret |= fl_cache_mem_addr_lo;

    return ret;
}
