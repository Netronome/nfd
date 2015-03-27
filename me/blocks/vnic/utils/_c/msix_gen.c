#include <assert.h>
#include <vnic/shared/nfcc_chipres.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_bulk.h>

#include <nfp6000/nfp_me.h>
#include <stdint.h>
#include <types.h>

#include <vnic/shared/nfd_cfg.h>
#include <vnic/pci_out.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/msix.h>
#include <nfp/mem_atomic.h>
#include <std/reg_utils.h>

#include <ns_vnic_ctrl.h>

#include "nfd_common.h"

/*
 * TODO:
 * - TX interrupts
 * - Honour the automask config
 *   Have a 64bit bitmask (one bit per Q, bitmask shared between RX/TX). Set
 *   bit for all queues of VF/PF if automask is set
 * - interrupt moderation
 * - any other operation when link comes down?
 * - *maybe* read counters in bulk
 */

#define MAX_QUEUE_NUM (NFD_MAX_VFS * NFD_MAX_VF_QUEUES + NFD_MAX_PF_QUEUES - 1)
#define MAX_NUM_PCI_ISLS 4

/*
 * Return if a given queue belongs to the PF (or not).
 *
 * The first N queues are used by VFs and last ones are for the PF.
 */
#define MSIX_Q_IS_PF(_q) ((_q) >= (NFD_MAX_VF_QUEUES * NFD_MAX_VFS) ? 1 : 0)

/*
 * Create masks for PF and VF
 */
#define MSIX_RINGS_MASK(num_rings)  ((1 << (num_rings)) - 1)
#define MSIX_PF_RINGS_MASK          MSIX_RINGS_MASK(NFD_MAX_PF_QUEUES)
#define MSIX_VF_RINGS_MASK          MSIX_RINGS_MASK(NFD_MAX_VF_QUEUES)

/*
 * Per PCIe Island state.
 *
 * We need to maintain quite a bit of state for generating MSI-X:
 * - @msix_rx_enabled: Bitmask of which RX queues are enabled
 * - @msix_tx_enabled: Bitmask of which TX queues are enabled
 * - @msix_rx_pending: Bitmask of which RX queues have pending interrupts
 * - @msix_tx_pending: Bitmask of which TX queues have pending interrupts
 * - @msix_automask:   Bitmask of which RX/TX queues should automask MSI-X
 *
 * - @msix_rx_entries: Indexed by RX Q, MSI-X table entry for this Q
 * - @msix_tx_entries: Indexed by TX Q, MSI-X table entry for this Q
 * - @prev_rx_cnt:     Indexed by RX Q, with the number of packets received
 * - @prev_tx_cnt:     Indexed by TX Q, with the number of packets transmitted
 *
 * We keep the first group state variables in GPRs.  The second group
 * is currently kept in local memory, but, depending on configuration,
 * may consume *all* of local memory.  We need to re-consider this
 * option, especially once we support interrupt moderation as the
 * configuration for that also needs to be somewhere.
 */
__shared __gpr static uint64_t msix_rx_enabled[MAX_NUM_PCI_ISLS];
__shared __gpr static uint64_t msix_tx_enabled[MAX_NUM_PCI_ISLS];
__shared __gpr static uint64_t msix_rx_pending[MAX_NUM_PCI_ISLS];
__shared __gpr static uint64_t msix_tx_pending[MAX_NUM_PCI_ISLS];
__shared __gpr static uint64_t msix_automask[MAX_NUM_PCI_ISLS];

__shared __lmem uint8_t msix_rx_entries[MAX_NUM_PCI_ISLS][MAX_QUEUE_NUM + 1];
__shared __lmem uint8_t msix_tx_entries[MAX_NUM_PCI_ISLS][MAX_QUEUE_NUM + 1];
__shared __lmem uint32_t msix_prev_rx_cnt[MAX_NUM_PCI_ISLS][MAX_QUEUE_NUM + 1];
__shared __lmem uint32_t msix_prev_tx_cnt[MAX_NUM_PCI_ISLS][MAX_QUEUE_NUM + 1];

#ifdef NFD_SVC_MSIX_DEBUG
#define NFD_MSIX_DBG(_x) _x
__export __cls volatile uint64_t nfd_msix_rx_enabled[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_rx_qmask[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_rx_new[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_tx_enabled[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_tx_qmask[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_tx_new[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_automask[MAX_NUM_PCI_ISLS];
#else
#define NFD_MSIX_DBG(_x)
#endif

/*
 * XXX This comes from the compiler intrinsics.
 * XXX Fix this once flowenv has support for the new run-time library
 */
/* 64-bit shift left */
long long
_shl_64(long long x, unsigned int y)
{
    long long result;
    int thirtytwo = 32;
    int y1;

    /* truncate shift count to 6 bits */
    y &= 63;

    if (y >= thirtytwo)
        __asm {
            // alu         [y, thirtytwo, -, y]
            alu         [result+4, y, AND, 0]
            alu_shf     [result, --, B, x+4, <<indirect]
        }
    else if (y != 0)
        __asm {
            alu         [y1, thirtytwo, -, y]
            alu         [--, y1, OR, 0]
            dbl_shf     [result, x, x+4, >>indirect]
            alu         [--, y, OR, 0]
            alu_shf     [result+4, --, B, x+4, <<indirect]
        }
    else
        result = x;

    return result;
}


/*
 * Initialise the state.
 *
 * Global variables are initialised to zero so init only the ones
 * which are not 0. This can also be done via array init.
 */
void
msix_qmon_init(unsigned int pcie_isl)
{
    int qnum;

    for (qnum = 0; qnum <= MAX_QUEUE_NUM; qnum++) {
        msix_rx_entries[pcie_isl][qnum] = 0xff;
        msix_tx_entries[pcie_isl][qnum] = 0xff;
    }
}

/* ns_vnic_ctrl.h defined offsets as byte offsets.  We get them as
 * words in cfg_bar_data and define word indices for them here  */
#define NS_VNIC_CFG_CTRL_IDX          (NS_VNIC_CFG_CTRL / 4)
#define NS_VNIC_CFG_UPDATE_IDX        (NS_VNIC_CFG_UPDATE / 4)
#define NS_VNIC_CFG_TXRS_ENABLE_IDX   (NS_VNIC_CFG_TXRS_ENABLE / 4)
#define NS_VNIC_CFG_RXRS_ENABLE_IDX   (NS_VNIC_CFG_RXRS_ENABLE / 4)


/*
 * Reconfigure RX and TX rings
 *
 * @pcie_isl        PCIe Island this function is handling
 * @vnic            vNIC inside the island this function is handling
 * @cfg_bar         points to the control bar for the vnic
 * @rx_rings        Boolean, if set, handle RX rings, else RX rings
 * @vf_rings        Bitmask of enabled tings for the VF/vNIC.
 *
 * This function updates the internal state (used by other MEs) for
 * handling MSI-X generation for RX and TX rings. The logic is
 * identical for RX and TX rings, only different data structures are
 * updated.  This is a bit ugly, but the other option would be
 * significant code duplication, which isn't pretty either.
 */
__intrinsic static void
msix_reconfig_rings(unsigned int pcie_isl, unsigned int vnic,
                    __mem char *cfg_bar, int rx_rings, uint64_t vf_rings)
{
    unsigned int qnum;
    uint64_t rings;
    unsigned int ring;
    unsigned int temp;

    uint64_t queues;
    uint64_t new_queues_en;
    uint64_t vf_queue_mask;

    __xread unsigned int ring_entries_r[16];
    __lmem uint8_t ring_entries[64];

    /* If there are rings enabled, read in the vectors for the rings */
    if (vf_rings) {
        mem_read64(ring_entries_r,
                   cfg_bar + NS_VNIC_CFG_RXR_VEC(0), sizeof(ring_entries_r));
        reg_cp(ring_entries, ring_entries_r, sizeof(ring_entries));
    }

    /* Update the interrupt vector data, i.e. the MSI-X table entry
     * number, for all active rings. */
    rings = vf_rings;
    while (rings) {
        ring = ffs64(rings);
        rings &= ~(1ull << ring);

        /* Convert ring number to a queue number */
        qnum = ring + vnic * NFD_MAX_VF_QUEUES;

        /* Get MSI-X entry number and stash it into local memory */
        temp = (ring & ~3) + (3 - (ring & 3));

        if (rx_rings)
            msix_rx_entries[pcie_isl][qnum] = ring_entries[temp];
        else
            msix_tx_entries[pcie_isl][qnum] = ring_entries[temp];
    }

    /* Convert VF ring bitmask into Queue mask */
    queues = vf_rings << (vnic * NFD_MAX_VF_QUEUES);

    /* Work out which queues have been newly enabled and make sure
     * they don't have pending bits set. */
    if (rx_rings) {
        new_queues_en =
            (msix_rx_enabled[pcie_isl] | queues) ^ msix_rx_enabled[pcie_isl];
        msix_rx_pending[pcie_isl] &= ~new_queues_en;
    } else {
        new_queues_en =
            (msix_tx_enabled[pcie_isl] | queues) ^ msix_tx_enabled[pcie_isl];
        msix_tx_pending[pcie_isl] &= ~new_queues_en;
    }

    /* Zero the local packet count for newly enabled queues */
    while (new_queues_en) {
        qnum = ffs64(new_queues_en);
        new_queues_en &= ~(1ull << qnum);
        if (rx_rings)
            msix_prev_rx_cnt[pcie_isl][qnum] = 0;
        else
            msix_prev_tx_cnt[pcie_isl][qnum] = 0;
    }

    /* Update the enabled bit mask with queues for this VF.
     *
     * Note the update below should be executed atomically with
     * respect to other contexts.  Strictly, it doesn't have to, but
     * it's better. */
    if (vnic == NFD_MAX_VFS)
        vf_queue_mask = MSIX_PF_RINGS_MASK << (vnic * NFD_MAX_VF_QUEUES);
    else
        vf_queue_mask = MSIX_VF_RINGS_MASK << (vnic * NFD_MAX_VF_QUEUES);

    if (rx_rings) {
        __no_swap_begin();
        msix_rx_enabled[pcie_isl] &= ~vf_queue_mask;
        msix_rx_enabled[pcie_isl] |= queues;
        __no_swap_end();
    } else {
        __no_swap_begin();
        msix_tx_enabled[pcie_isl] &= ~vf_queue_mask;
        msix_tx_enabled[pcie_isl] |= queues;
        __no_swap_end();
    }

    NFD_MSIX_DBG(nfd_msix_rx_qmask[pcie_isl] = vf_queue_mask);
    NFD_MSIX_DBG(nfd_msix_rx_new[pcie_isl] = queues);
    NFD_MSIX_DBG(nfd_msix_rx_enabled[pcie_isl] = msix_rx_enabled[pcie_isl]);
    NFD_MSIX_DBG(nfd_msix_tx_qmask[pcie_isl] = vf_queue_mask);
    NFD_MSIX_DBG(nfd_msix_tx_new[pcie_isl] = queues);
    NFD_MSIX_DBG(nfd_msix_tx_enabled[pcie_isl] = msix_rx_enabled[pcie_isl]);
}


/*
 * Handle reconfiguration changes of RX queues
 *
 * @pcie_isl        PCIe Island this function is handling
 * @vnic            vNIC inside the island this function is handling
 * @cfg_bar         points to the control bar for the vnic
 * @cfg_bar_data[]  contains the first 6 words of the control bar.
 *
 * This function is called from context 0 of the service ME on
 * configuration. The MSI-X code runs on different contexts and this
 * function updates their data structures.
 */
__intrinsic void
msix_reconfig(unsigned int pcie_isl, unsigned int vnic, __mem char *cfg_bar,
              __xread unsigned int cfg_bar_data[6])
{
    unsigned int control, update;

    uint64_t vf_tx_rings_new;
    uint64_t vf_rx_rings_new;
    uint64_t queues;

    control = cfg_bar_data[NS_VNIC_CFG_CTRL_IDX];
    update = cfg_bar_data[NS_VNIC_CFG_UPDATE_IDX];

    /* If no MSI-X updates, return */
    if (!(update & NS_VNIC_CFG_UPDATE_MSIX))
        return;

    /* Check if we are up and rings have changed */
    if ((control & NS_VNIC_CFG_CTRL_ENABLE) &&
        (update & NS_VNIC_CFG_UPDATE_RING)) {
        vf_tx_rings_new = cfg_bar_data[NS_VNIC_CFG_TXRS_ENABLE_IDX];
        vf_rx_rings_new = cfg_bar_data[NS_VNIC_CFG_RXRS_ENABLE_IDX];

    } else if ((update & NS_VNIC_CFG_UPDATE_GEN) &&
               (!(control & NS_VNIC_CFG_CTRL_ENABLE))) {
        /* The device got disabled */
        vf_tx_rings_new = 0;
        vf_rx_rings_new = 0;
    }

    /* Make sure the vnic is not configuring rings it has no control over */
    if (vnic == NFD_MAX_VFS) {
        vf_tx_rings_new &= MSIX_PF_RINGS_MASK;
        vf_rx_rings_new &= MSIX_PF_RINGS_MASK;
    } else {
        vf_tx_rings_new &= MSIX_VF_RINGS_MASK;
        vf_rx_rings_new &= MSIX_VF_RINGS_MASK;
    }

    /* Set MSI-X automask bits.  We assume that a VF/PF has the same
     * number of RX and TX rings and simple set the auto-mask bits for
     * all queues of the VF/PF depending on the auto-mask bit in the
     * control word. */
    queues = vf_rx_rings_new << (vnic * NFD_MAX_VF_QUEUES);
    if (control & NS_VNIC_CFG_CTRL_MSIXAUTO)
        msix_automask[pcie_isl] |= queues;
    else
        msix_automask[pcie_isl] &= ~queues;
    NFD_MSIX_DBG(nfd_msix_automask[pcie_isl] = msix_automask[pcie_isl]);

    /* Reconfigure the RX/TX ring state */
    msix_reconfig_rings(pcie_isl, vnic, cfg_bar, 1, vf_rx_rings_new);
    msix_reconfig_rings(pcie_isl, vnic, cfg_bar, 0, vf_rx_rings_new);
}


/*
 * Read the number of packets received (and sent to host) from PCI.OUT.
 *
 * PCI.OUT maintains a table with two values per RX queue: a 32bit
 * credit counter and a 32bit packet counter.  These are atomically
 * incremented by PCI.OUT when packets are transmitted.  This function
 * reads the packet counter for a given queue.
 *
 * The index into the table is "a bit" strange. The table is organised
 * as follows:
 * q0(2x4B) q32(2x4B) q1(2x4B) q33(2x4B) ...
 * So, for RX queue 0, the values are at offset 0x0, for RX queue 1,
 * the values are at offset 0x10, for RX queue 32, the values are at
 * offset 0x8, etc.
 *
 * This ordering above is called the Bit Masked Queue Ordering in
 * PCI.OUT/PCI.IN and it get's it's name from the way the bitmasks
 * reported by the event filters are compacted.
 *
 * XXX This code *assumes* that the credits/packet counts are at
 * offset 0 in CTM of the relevant PCIe island.
 */
__intrinsic static uint32_t
msix_get_rx_queue_cnt(unsigned int pcie_nr, unsigned int queue)
{
    unsigned int addr_hi;
    unsigned int addr_lo;
    __xread uint32_t rdata;
    SIGNAL rsig;

    /* Calculate the offset. */
    queue = NFD_NATQ2BMQ(queue);

    addr_hi = (0x84 | pcie_nr) << 24;
    addr_lo = queue * NFD_OUT_ATOMICS_SZ + NFD_OUT_ATOMICS_SENT;

    __asm mem[atomic_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rsig];

    return rdata;
}


/*
 * Attempt to send an MSI-X for a given queue
 * @pcie_isl:  PCIe Island number
 * @qnum:      Queue number
 * @rx_queue:  Boolean, set if this is for an RX queue, TX queue otherwise
 */
static int
msix_send_q_irq(unsigned int pcie_isl, int qnum, int rx_queue)
{
    unsigned int automask;
    unsigned int entry;
    int vf_num;
    int ret;

    if (rx_queue)
        entry = msix_rx_entries[pcie_isl][qnum];
    else
        entry = msix_tx_entries[pcie_isl][qnum];

    /* Should we automask for this queue? */
    automask = msix_automask[pcie_isl] & (1ull << qnum);

    if (MSIX_Q_IS_PF(qnum)) {
        ret = msix_pf_send(pcie_isl + 4, entry, automask);
    } else {
        vf_num = qnum / NFD_MAX_VF_QUEUES;
        ret = msix_vf_send(pcie_isl + 4, vf_num, entry, automask);
    }

    return ret;
}


void
msix_qmon_loop(unsigned int pcie_isl)
{
    int qnum;
    uint32_t rx_cnt;
    uint64_t pending;

    int ret;


    for (;;) {

        /*
         * Handle RX queues:
         *
         * We check for all active RX queues and if their packet count
         * changed immediately try to send an MSI-X.
         */
        for (qnum = 0; qnum <= MAX_QUEUE_NUM; qnum++) {
            /* Skip if no queues are enabled or a queue is not enabled */
            if (!msix_rx_enabled[pcie_isl])
                break;
            if (!(msix_rx_enabled[pcie_isl] & (1ull << qnum)))
                continue;

            /* Check if queue got new packets and try to send MSI-X if so */
            rx_cnt = msix_get_rx_queue_cnt(pcie_isl + 4, qnum);
            if (rx_cnt != msix_prev_rx_cnt[pcie_isl][qnum]) {
                ret = msix_send_q_irq(pcie_isl, qnum, 1);

                /* If un-successful, mark queue in the pending mask,
                 * else remove it from the pending mask. */
                if (ret)
                    msix_rx_pending[pcie_isl] |= (1ull << qnum);
                else
                    msix_rx_pending[pcie_isl] &= ~(1ull << qnum);

                /* Update the count to the new value */
                msix_prev_rx_cnt[pcie_isl][qnum] = rx_cnt;
            }
        }

        /*
         * Handle Pending interrupts.
         * RX first
         */
        pending = msix_rx_pending[pcie_isl];
        while (pending) {
            qnum = ffs64(pending);
            pending &= ~(1ull << qnum);

            /* If the Queue got disabled in the meantime, skip it */
            if (!(msix_rx_enabled[pcie_isl] & (1ull << qnum)))
                continue;

            /* MAYBE TODO: We could check here if the packet count has
             * increased and update the local state  */

            /* Try to send MSI-X. If successful remove from pending */
            ret = msix_send_q_irq(pcie_isl, qnum, 1);
            if (!ret)
                msix_rx_pending[pcie_isl] &= ~(1ull << qnum);
        }

        /* In the loop above we perform sufficient IO for others to
         * run, but give them another chance here. */
        ctx_swap();
    }
}
