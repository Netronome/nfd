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
#define _AUTO_MASK 1   /* should come from config */

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

__shared __lmem uint8_t msix_rx_entries[MAX_NUM_PCI_ISLS][MAX_QUEUE_NUM + 1];
__shared __lmem uint8_t msix_tx_entries[MAX_NUM_PCI_ISLS][MAX_QUEUE_NUM + 1];
__shared __lmem uint32_t msix_prev_rx_cnt[MAX_NUM_PCI_ISLS][MAX_QUEUE_NUM + 1];
__shared __lmem uint32_t msix_prev_tx_cnt[MAX_NUM_PCI_ISLS][MAX_QUEUE_NUM + 1];

#ifdef NFD_SVC_MSIX_DEBUG
__export __cls volatile uint64_t nfd_msix_rx_enabled[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_rx_qmask[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_rx_new[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_tx_enabled[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_tx_qmask[MAX_NUM_PCI_ISLS];
__export __cls volatile uint64_t nfd_msix_tx_new[MAX_NUM_PCI_ISLS];
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

    /* Update the interrupt vector data, ie the MSI-X table entry
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

#ifdef NFD_SVC_MSIX_DEBUG
    if (rx_rings) {
        nfd_msix_rx_qmask[pcie_isl] = vf_queue_mask;
        nfd_msix_rx_new[pcie_isl] = queues;
        nfd_msix_rx_enabled[pcie_isl] = msix_rx_enabled[pcie_isl];
    } else {
        nfd_msix_tx_qmask[pcie_isl] = vf_queue_mask;
        nfd_msix_tx_new[pcie_isl] = queues;
        nfd_msix_tx_enabled[pcie_isl] = msix_rx_enabled[pcie_isl];
    }
#endif
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

void
msix_qmon_loop(unsigned int pcie_isl)
{
    unsigned int int_is_masked;
    int qnum, vf_num;

    uint32_t rx_cnt;

    int ret;

    for (;;) {
        for (qnum = 0; qnum <= MAX_QUEUE_NUM; qnum++) {

            /* Skip if queue is not active */
            if (!(msix_rx_enabled[pcie_isl] & (1ull << qnum)))
                continue;

            /* Check if new packets got received on queue and mark in
             * pending if */
            rx_cnt = msix_get_rx_queue_cnt(pcie_isl + 4, qnum);

            if (rx_cnt != msix_prev_rx_cnt[pcie_isl][qnum]) {
                msix_rx_pending[pcie_isl] |= (1ull << qnum);
                msix_prev_rx_cnt[pcie_isl][qnum] = rx_cnt;
            }

            /* If queue has no pending interrupts, go to next */
            if (!(msix_rx_pending[pcie_isl] & (1ull << qnum)))
                continue;

            /* Attempt to send an interrupt */
            if (MSIX_Q_IS_PF(qnum)) {
                ret = msix_pf_send(pcie_isl + 4,
                                   msix_rx_entries[pcie_isl][qnum], _AUTO_MASK);
            } else {
                vf_num = qnum / NFD_MAX_VF_QUEUES;

                ret = msix_vf_send(pcie_isl + 4, vf_num,
                                   msix_rx_entries[pcie_isl][qnum], _AUTO_MASK);
            }

            /* If successful, remove from the pending list */
            if (!ret)
                msix_rx_pending[pcie_isl] &= ~(1ull << qnum);
        }

        ctx_swap();
    }
}
