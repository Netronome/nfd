/*
 * Copyright (C) 2015,  Netronome Systems, Inc.  All rights reserved.
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
 * @file   msix_qmon.c
 * @brief  Monitor RX/TX queues and generate MSI-X on changes.
 */
#ifndef _BLOCKS__VNIC_SVC_MSIX_QMON_C_
#define _BLOCKS__VNIC_SVC_MSIX_QMON_C_

#include <assert.h>
#include <vnic/shared/nfcc_chipres.h>
#include <nfp.h>
#include <stdint.h>
#include <types.h>

#include <nfp/cls.h>
#include <nfp/me.h>
#include <nfp/mem_bulk.h>
#include <nfp/mem_atomic.h>

#include <std/reg_utils.h>

#include <nfp6000/nfp_me.h>

#include <vnic/shared/nfd_cfg.h>
#include <vnic/pci_out.h>
#include <vnic/shared/nfd_internal.h>
#include <vnic/utils/qcntl.h>

#include <nfp_net_ctrl.h>

#include "nfd_common.h"

#include "msix.c"

/*
 * TODO:
 * - interrupt moderation
 * - any other operation when link comes down?
 * - *maybe* read counters in bulk
 */


/*
 * This file implements the core of the logic for generating MSI-X for
 * packet transmit and receive.  At the core a single context per PCIe
 * Island is monitoring all of the active RX and TX queues for that
 * Island.  If changes to a queue are noticed, a MSI-X is generated.
 * This core logic also handles interrupt masking and the like and the
 * main entry point is implemented in @msix_qmon_loop().
 *
 * Since functions (PFs and VFs) as well as individual rings in these
 * functions can be configured dynamically at run-time, this file also
 * contains the logic for handling re-configurations.
 *
 * A general comment on terminology.  Through-out (unless for
 * identifiers from other files) we use the term "ring" when referring
 * to a queue or ring inside a VF/PF and the term "queue" when
 * referring to one of the 64 RX/TX queues available in total.  The
 * core MSI-X logic is only dealing with "queues" and configuration
 * logic translates from "rings" to queues.
 */


#define MAX_QUEUE_NUM (NFD_MAX_VFS * NFD_MAX_VF_QUEUES + NFD_MAX_PF_QUEUES - 1)
#define MAX_NUM_PCI_ISLS 4


/*
 * Create masks for PF and VF
 */
#define MSIX_RINGS_MASK(num_rings)  ((num_rings) == 64 ? 0xffffffffffffffff : \
                                     (1ull << (num_rings)) - 1)
#define MSIX_PF_RINGS_MASK          MSIX_RINGS_MASK(NFD_MAX_PF_QUEUES)
#define MSIX_VF_RINGS_MASK          MSIX_RINGS_MASK(NFD_MAX_VF_QUEUES)

/*
 * Functions to perform little endian reads and write from the MU.
 * The host writes byte arrays in little endian format.  Rather than
 * performing the conversion on the host, we just access them with LE
 * commands.
 * XXX Remove once we have equivalent functions in flowenv
 */
__intrinsic void
alt_mem_read32_le(__xread void *data, __mem void *addr)
{
    unsigned int addr_hi, addr_lo;
    SIGNAL sig;

    addr_hi = ((unsigned long long int)addr >> 8) & 0xff000000;
    addr_lo = (unsigned long long int)addr & 0xffffffff;

    __asm mem[read32_le, *data, addr_hi, <<8, addr_lo, 1], ctx_swap[sig];
}

__intrinsic void
alt_mem_write8_le(__xwrite void *data, __mem void *addr)
{
    unsigned int addr_hi, addr_lo;
    SIGNAL sig;

    addr_hi = ((unsigned long long int)addr >> 8) & 0xff000000;
    addr_lo = (unsigned long long int)addr & 0xffffffff;

    __asm mem[write8_le, *data, addr_hi, <<8, addr_lo, 1], ctx_swap[sig];
}

/*
 * Some functions below use 64bit shift left (e.g. 1ull << qnum), for
 * which the compiler calls generates code relying on the compiler
 * runtime function _shl_64().  Unfortunately, this functions does not
 * seem to get inlined and then causes a major headache when it comes
 * to register liveranges.  shl64() is a copy the runtime
 * implementation, which is marked as intrinsic so will get inlined.
 *
 * TODO: Re-retest once this code is properly integrated with the
 * compiler runtime.
 */
__intrinsic static long long
shl64(long long x, unsigned int y)
{
    long long result;
    int thirtytwo = 32;
    unsigned int y_cp;
    int y1;

    /* truncate shift count to 6 bits
     * copy into a local variable in the process
     * in case y is a compile time constant */
    y_cp = y & 63;

    if (y_cp >= thirtytwo)
        __asm {
            alu         [result+4, y_cp, AND, 0]
            alu_shf     [result, --, B, x+4, <<indirect]
        }
    else if (y_cp != 0)
        __asm {
            alu         [y1, thirtytwo, -, y_cp]
            alu         [--, y1, OR, 0]
            dbl_shf     [result, x, x+4, >>indirect]
            alu         [--, y_cp, OR, 0]
            alu_shf     [result+4, --, B, x+4, <<indirect]
        }
    else
        result = x;

    return result;
}

/*
 * Configuration changes:
 *
 * When new rings get enabled (e.g. PFs or VFs are spun up or down)
 * context 0 in the service ME gets notified via suitable
 * configuration messages.  The MSI-X queue monitoring contexts then
 * needs to pick up the changes (like which queues are enabled, which
 * MSI-X entries to use etc).
 *
 * This process is split between context 0 and the MSI-X queue
 * monitoring contexts.  Context 0 is handling the conversion from
 * rings to queues and then writes the new state (plus additional
 * information to CLS, and signals the appropriate MSI-X queue
 * monitoring context.  The qmon context then simply copies the state
 * from CLS into its local state (some of which is held in local
 * registers for efficient access.  One the qmon context has updated
 * its locla state it signals back to context 0, which handles the
 * remainder of the configuration chain.
 *
 * Splitting the re-config handling relives pressure on GPRs and
 * allows the MSI-X queue monitoring contexts to maintain state in
 * registers which can't be shared (such as Next Neighbour
 * Registers). It also aids debugging as the state is externally
 * visible in CLS.
 *
 * @msix_qmon_reconfig() contains the code executed by context 0,
 * while @msix_local_reconfig() is the code executed by the MSI-X
 * queue monitoring context.
 *
 * Shared state (in CLS):
 * @msix_cls_rx_enabled      Bitmask of which RX queues are enabled
 * @msix_cls_tx_enabled      Bitmask of which TX queues are enabled
 * @msix_cls_rx_new_enabled  Bitmask of which new RX queues are being enabled
 * @msix_cls_tx_new_enabled  Bitmask of which new TX queues are being enabled
 * @msix_cls_automask        Bitmask of which queues should automask
 *
 * @msix_cls_rx_entries      Mapping of RX queue to MSI-X table entry
 * @msix_cls_tx_entries      Mapping of TX queue to MSI-X table entry
 */
__shared __cls uint64_t msix_cls_rx_enabled[MAX_NUM_PCI_ISLS];
__shared __cls uint64_t msix_cls_tx_enabled[MAX_NUM_PCI_ISLS];
__shared __cls uint64_t msix_cls_rx_new_enabled[MAX_NUM_PCI_ISLS];
__shared __cls uint64_t msix_cls_tx_new_enabled[MAX_NUM_PCI_ISLS];
__shared __cls uint64_t msix_cls_automask[MAX_NUM_PCI_ISLS];

__shared __cls uint8_t msix_cls_rx_entries[MAX_NUM_PCI_ISLS][NFP_NET_RXR_MAX];
__shared __cls uint8_t msix_cls_tx_entries[MAX_NUM_PCI_ISLS][NFP_NET_TXR_MAX];

/*
 * Initialise the state (executed by context 0)
 *
 * Global variables are initialised to zero so init only the ones
 * which are not 0. This could/should be done via array init.
 */
void
msix_qmon_init(unsigned int pcie_isl)
{
    int i;
    __cls uint8_t *r = msix_cls_rx_entries[pcie_isl];
    __cls uint8_t *t = msix_cls_tx_entries[pcie_isl];
    __xwrite int tmp = 0xffffffff;

    for (i = 0; i < 64; i += 4) {
        cls_write(&tmp, r + i, sizeof(tmp));
        cls_write(&tmp, t + i, sizeof(tmp));
    }
}

/*
 * Reconfigure RX and TX rings (executed by context 0)
 *
 * @pcie_isl        PCIe Island this function is handling
 * @vnic            vNIC inside the island this function is handling
 * @cfg_bar         Points to the control bar for the vnic
 * @rx_rings        Boolean, if set, handle RX rings, else RX rings
 * @vf_rings        Bitmask of enabled tings for the VF/vNIC.
 *
 * This function updates the internal state (used by other MEs) for
 * handling MSI-X generation for RX and TX rings. The logic is
 * identical for RX and TX rings, only different data structures are
 * updated.  This is a bit ugly, but the other option would be
 * significant code duplication, which isn't pretty either.
 *
 * Note: Some of the code generates contains CLS reads/writes.
 */
__intrinsic static void
msix_reconfig_rings(unsigned int pcie_isl, unsigned int vnic,
                    __mem char *cfg_bar, int rx_rings, uint64_t vf_rings)
{
    unsigned int qnum;
    uint64_t rings;
    unsigned int ring;
    unsigned int entry;
    __xread unsigned int entry_r, tmp_r;
    __xwrite unsigned int tmp_w;
    __cls uint8_t *cls_addr;
    __mem char *entry_addr;

    uint64_t queues;
    uint64_t new_queues_en;
    uint64_t vf_queue_mask;

    /* Update the interrupt vector data, i.e. the MSI-X table entry
     * number, for all active rings. */
    rings = vf_rings;
    while (rings) {
        ring = ffs64(rings);
        rings &= rings - 1;

        /* Convert ring number to a queue number */
        qnum = ring + vnic * NFD_MAX_VF_QUEUES;

        /* Get MSI-X entry number and stash it into local memory */
        if (rx_rings) {
            entry_addr = cfg_bar + NFP_NET_CFG_RXR_VEC(ring);
            cls_addr = msix_cls_rx_entries[pcie_isl];
        } else {
            entry_addr = cfg_bar + NFP_NET_CFG_TXR_VEC(ring);
            cls_addr = msix_cls_tx_entries[pcie_isl];
        }
        alt_mem_read32_le(&entry_r, entry_addr);
        entry = entry_r & 0xff;
        /* Write to CLS. We do this in BE format so it's easy to pick up */
        cls_addr += qnum;
        cls_read(&tmp_r, cls_addr, sizeof(tmp_r));
        tmp_w = (tmp_r & 0x00ffffff) | (entry << 24);
        cls_write(&tmp_w, cls_addr, sizeof(tmp_w));

        /* Make sure the ICR is set. The driver is supposed to unmask
         * once it is done with the initialisation. */
        tmp_w = NFP_NET_CFG_ICR_RXTX;
        alt_mem_write8_le(&tmp_w, cfg_bar + NFP_NET_CFG_ICR(entry));
    }

    /* Convert VF ring bitmask into Queue mask */
    queues = shl64(vf_rings, vnic * NFD_MAX_VF_QUEUES);

    /* Work out which queues have been newly enabled and make sure
     * they don't have pending bits set. */
    if (rx_rings) {
        new_queues_en = (msix_cls_rx_enabled[pcie_isl] | queues) ^
            msix_cls_rx_enabled[pcie_isl];
        msix_cls_rx_new_enabled[pcie_isl] = new_queues_en;
    } else {
        new_queues_en = (msix_cls_tx_enabled[pcie_isl] | queues) ^
            msix_cls_tx_enabled[pcie_isl];
        msix_cls_tx_new_enabled[pcie_isl] = new_queues_en;
    }

    /* Update the enabled bit mask with queues for this VF. */
    if (vnic == NFD_MAX_VFS)
        vf_queue_mask = shl64(MSIX_PF_RINGS_MASK, vnic * NFD_MAX_VF_QUEUES);
    else
        vf_queue_mask = shl64(MSIX_VF_RINGS_MASK, vnic * NFD_MAX_VF_QUEUES);

    if (rx_rings) {
        msix_cls_rx_enabled[pcie_isl] &= ~vf_queue_mask;
        msix_cls_rx_enabled[pcie_isl] |= queues;
    } else {
        msix_cls_tx_enabled[pcie_isl] &= ~vf_queue_mask;
        msix_cls_tx_enabled[pcie_isl] |= queues;
    }
}

/*
 * Handle reconfiguration changes of RX queues (executed by context 0)
 *
 * @pcie_isl        PCIe Island this function is handling
 * @vnic            vNIC inside the island this function is handling
 * @cfg_bar         points to the control bar for the vnic
 * @cfg_bar_data[]  contains the first 6 words of the control bar.
 *
 * This function is called from context 0 of the service ME on
 * configuration.  The MSI-X queue monitoring code runs on different
 * contexts and this function updates the shared CLS data structures
 * before signalling the MSI-X contexts.
 *
 * Note: Some of the code contains implicit CLS reads/writes.
 */
__intrinsic void
msix_qmon_reconfig(unsigned int pcie_isl, unsigned int vnic,
                   __mem char *cfg_bar, __xread unsigned int cfg_bar_data[6])
{
    unsigned int control, update;

    uint64_t vf_tx_rings_new;
    uint64_t vf_rx_rings_new;
    uint64_t queues;
    SIGNAL ack_sig;

    __assign_relative_register(&ack_sig, SVC_RECONFIG_SIG_NUM);

    control = cfg_bar_data[NFP_NET_CFG_CTRL >> 2];
    update = cfg_bar_data[NFP_NET_CFG_UPDATE >> 2];

    /* If no MSI-X updates, return */
    if (!(update & NFP_NET_CFG_UPDATE_MSIX))
        return;

    /* Check if we are up and rings have changed */
    if ((control & NFP_NET_CFG_CTRL_ENABLE) &&
        (update & NFP_NET_CFG_UPDATE_RING)) {
        vf_tx_rings_new = cfg_bar_data[NFP_NET_CFG_TXRS_ENABLE >> 2];
        vf_rx_rings_new = cfg_bar_data[NFP_NET_CFG_RXRS_ENABLE >> 2];

    } else if ((update & NFP_NET_CFG_UPDATE_GEN) &&
               (!(control & NFP_NET_CFG_CTRL_ENABLE))) {
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
    queues = shl64(vf_rx_rings_new, vnic * NFD_MAX_VF_QUEUES);
    if (control & NFP_NET_CFG_CTRL_MSIXAUTO)
        msix_cls_automask[pcie_isl] |= queues;
    else
        msix_cls_automask[pcie_isl] &= ~queues;

    /* Reconfigure the RX/TX ring state */
    msix_reconfig_rings(pcie_isl, vnic, cfg_bar, 1, vf_rx_rings_new);
    msix_reconfig_rings(pcie_isl, vnic, cfg_bar, 0, vf_rx_rings_new);

    signal_ctx(pcie_isl + 1, SVC_RECONFIG_SIG_NUM);
    __implicit_write(&ack_sig);
    wait_for_all(&ack_sig);
}


/*
 * Code beyond this point is executed by the MSI-X queue monitoring contexts
 */


/*
 * Per PCIe Island state:
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
 * We keep the first group state variables in Next Neighbour
 * Registers.  The second group is kept in local memory.  The local
 * memory variables are marked as shared and are arrays of arrays,
 * because otherwise something like uint32_t
 * msix_prev_rx_cnt[NFP_NET_RXR_MAX] would get allocated for *every*
 * context.
 *
 * XXX Global __nnr variables explicitly initialised to zero due to THSDK-2070
 */
__nnr static uint64_t msix_rx_enabled = 0;
__nnr static uint64_t msix_tx_enabled = 0;
__nnr static uint64_t msix_rx_pending = 0;
__nnr static uint64_t msix_tx_pending = 0;
__nnr static uint64_t msix_automask = 0;

__shared __lmem uint8_t msix_rx_entries[MAX_NUM_PCI_ISLS][NFP_NET_RXR_MAX];
__shared __lmem uint8_t msix_tx_entries[MAX_NUM_PCI_ISLS][NFP_NET_TXR_MAX];
__shared __lmem uint32_t msix_prev_rx_cnt[MAX_NUM_PCI_ISLS][NFP_NET_RXR_MAX];
__shared __lmem uint32_t msix_prev_tx_cnt[MAX_NUM_PCI_ISLS][NFP_NET_TXR_MAX];

/*
 * Local reconfig
 *
 * Copy state from CLS into local state.  For newly enabled queues
 * reset some of the internal state.
 *
 * Note: Some of the code contains implicit CLS reads/writes.
 */
__intrinsic static void
msix_local_reconfig(const unsigned int pcie_isl)
{
    uint64_t new_enabled;
    __xread uint64_t tmp64;
    int qnum;
    __xread uint32_t entries[NFP_NET_RXR_MAX / 4];

    /*
     * Handle newly enabled queues (RX first, then TX)
     * - remove any pending bits.
     * - zero the count to keep track if new packets have been RXed/TXed
     *
     * Note: We use 'tmp64 &= tmp64 - 1' instead of 'tmp64 &= ~(1ull
     * << qnum)' to zero the first bit set because it removes
     * dependency on the _shl_64() intrinsic.
     */
    cls_read(&tmp64, &msix_cls_rx_new_enabled[pcie_isl], sizeof(tmp64));
    new_enabled = tmp64;
    msix_rx_pending &= ~new_enabled;
    while (new_enabled) {
        qnum = ffs64(new_enabled);
        new_enabled &= new_enabled - 1;
        msix_prev_rx_cnt[pcie_isl][qnum] = 0;
    }

    cls_read(&tmp64, &msix_cls_tx_new_enabled[pcie_isl], sizeof(tmp64));
    new_enabled = tmp64;
    msix_tx_pending &= ~new_enabled;
    while (new_enabled) {
        qnum = ffs64(new_enabled);
        new_enabled &= new_enabled - 1;
        msix_prev_tx_cnt[pcie_isl][qnum] = 0;
    }

    /* Update automask */
    cls_read(&tmp64, &msix_cls_automask[pcie_isl], sizeof(tmp64));
    msix_automask = tmp64;

    /* Copy entries */
    cls_read(&entries, msix_cls_rx_entries[pcie_isl], sizeof(entries));
    reg_cp(&msix_rx_entries[pcie_isl], entries, sizeof(entries));
    cls_read(&entries, msix_cls_tx_entries[pcie_isl], sizeof(entries));
    reg_cp(&msix_tx_entries[pcie_isl], entries, sizeof(entries));

    /* Copy enable bits */
    msix_rx_enabled = msix_cls_rx_enabled[pcie_isl];
    msix_tx_enabled = msix_cls_tx_enabled[pcie_isl];

    /* We are done. Signal context zero */
    signal_ctx(0, SVC_RECONFIG_SIG_NUM);
}


/*
 * Core MSI-X logic for generating interrupts for RX/TX queues
 */


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
msix_get_rx_queue_cnt(const unsigned int pcie_isl, unsigned int queue)
{
    unsigned int addr_hi;
    unsigned int addr_lo;
    __xread uint32_t rdata;
    SIGNAL rsig;

    /* Calculate the offset. */
    queue = NFD_NATQ2BMQ(queue);

    addr_hi = (0x84 | (pcie_isl + 4)) << 24;
    addr_lo = queue * NFD_OUT_ATOMICS_SZ + NFD_OUT_ATOMICS_SENT;

    __asm mem[atomic_read, rdata, addr_hi, <<8, addr_lo, 1], ctx_swap[rsig];

    return rdata;
}


/*
 * Attempt to send an MSI-X for a given queue
 * @pcie_isl:  PCIe Island number
 * @qnum:      Queue number
 * @rx_queue:  Boolean, set if this is for an RX queue, TX queue otherwise
 *
 * Returns 0 on success.  Otherwise the interrupt is masked in some way.
 *
 * If MSI-X auto-masking is enabled for the function, just go with
 * that.  If MSI-X auto-masking is disabled, check the ICR for the
 * entry in the function configuration BAR.  If not set, set it and
 * attempt to generate a MSI-X.  If the ICR is already set, then the
 * entry is already "masked".
 */
__intrinsic static int
msix_send_q_irq(const unsigned int pcie_isl, int qnum, int rx_queue)
{
    unsigned int automask;
    unsigned int entry;
    int fn;
    uint64_t cfg_bar;
    __xread uint32_t mask_r;
    __xwrite uint32_t mask_w;

    int ret;

    if (rx_queue)
        entry = msix_rx_entries[pcie_isl][qnum];
    else
        entry = msix_tx_entries[pcie_isl][qnum];

    /* Should we automask this queue? */
    automask = msix_automask & shl64(1ull, qnum);

    /* Get the function (aka vnic) */
    NFD_NATQ2VNIC(fn, qnum);

    /* If we don't use auto-masking, check (and update) the ICR */
    if (!automask) {
        cfg_bar = NFD_CFG_BAR(svc_cfg_bars[pcie_isl], fn);
        cfg_bar += NFP_NET_CFG_ICR(entry);
        alt_mem_read32_le(&mask_r, (__mem void *)cfg_bar);
        if (mask_r & 0x000000ff) {
            ret = 1;
            goto out;
        }
        mask_w = NFP_NET_CFG_ICR_RXTX;
        alt_mem_write8_le(&mask_w, (__mem void *)cfg_bar);
    }

    if (fn >= NFD_MAX_VFS)
        ret = msix_pf_send(pcie_isl + 4, entry, automask);
    else
        ret = msix_vf_send(pcie_isl + 4, fn, entry, automask);

out:
    return ret;
}


/*
 * The main monitoring loop.
 */
__forceinline void
msix_qmon_loop(const unsigned int pcie_isl)
{
    int qnum;
    uint32_t count;
    uint64_t qmask;
    uint64_t enabled, pending;
    int ret;

    SIGNAL reconfig_sig;

    __assign_relative_register(&reconfig_sig, SVC_RECONFIG_SIG_NUM);

    for (;;) {

        if (signal_test(&reconfig_sig))
            msix_local_reconfig(pcie_isl);

        /*
         * Check enabled RX and TX queues.
         * Read their respective counts and if they changed try to
         * send an interrupt (RX) or mark them as pending (TX).
         */
        enabled = msix_rx_enabled;
        while (enabled) {
            qnum = ffs64(enabled);
            qmask = shl64(1ull, qnum);
            enabled &= ~qmask;

            /* Check if queue got new packets and try to send MSI-X if so */
            count = msix_get_rx_queue_cnt(pcie_isl, qnum);
            if (count != msix_prev_rx_cnt[pcie_isl][qnum]) {
                ret = msix_send_q_irq(pcie_isl, qnum, 1);

                /* If un-successful, mark queue in the pending mask */
                if (ret)
                    msix_rx_pending |= qmask;
                else
                    msix_rx_pending &= ~qmask;

                /* Update the count to the new value */
                msix_prev_rx_cnt[pcie_isl][qnum] = count;
            }
        }

        enabled = msix_tx_enabled;
        while (enabled) {
            qnum = ffs64(enabled);
            qmask = shl64(1ull, qnum);
            enabled &= ~qmask;

            /* XXX assumes/hardcodes that TX queues are first...no macro */
            count = qc_read(pcie_isl, qnum << 1, QC_RPTR);
            count = NFP_QC_STS_LO_READPTR_of(count);
            if (count != msix_prev_tx_cnt[pcie_isl][qnum]) {
                msix_tx_pending |= qmask;
                msix_prev_tx_cnt[pcie_isl][qnum] = count;
            }
        }

        /*
         * Handle pending Interrupts. RX first, then TX.  If a RX
         * queue and a TX queue share the same MSI-X entry (very
         * likely) then remove them from the respective other pending
         * mask.  Also update the respective RX/TX count in case new
         * packets were received/trasnmitted since we last checked.
         */
        pending = msix_rx_pending;
        while (pending) {
            qnum = ffs64(pending);
            qmask = shl64(1ull, qnum);
            pending &= ~qmask;

            /* Update RX queue count in case it changed. */
            count = msix_get_rx_queue_cnt(pcie_isl, qnum);
            msix_prev_rx_cnt[pcie_isl][qnum] = count;

            /* Try to send MSI-X. If successful remove from pending */
            ret = msix_send_q_irq(pcie_isl, qnum, 1);
            if (!ret) {
                msix_rx_pending &= ~qmask;
                if (msix_rx_entries[pcie_isl][qnum] ==
                    msix_tx_entries[pcie_isl][qnum])
                    msix_tx_pending &= ~qmask;
            }
        }

        while (pending) {
            qnum = ffs64(pending);
            qmask = shl64(1ull, qnum);
            pending &= ~qmask;

            /* Update TX queue count in case it changed. */
            count = qc_read(pcie_isl, qnum << 1, QC_RPTR);
            count = NFP_QC_STS_LO_READPTR_of(count);
            msix_prev_tx_cnt[pcie_isl][qnum] = count;

            /* Try to send MSI-X. If successful remove from pending */
            ret = msix_send_q_irq(pcie_isl, qnum, 0);
            if (!ret) {
                msix_tx_pending &= ~qmask;
                if (msix_tx_entries[pcie_isl][qnum] ==
                    msix_rx_entries[pcie_isl][qnum])
                    msix_rx_pending &= ~qmask;
            }
        }

        /* In the loop above we perform sufficient IO for others to
         * run, but give them another chance here. */
        ctx_swap();
    }
}

#endif /* !_BLOCKS__VNIC_SVC_MSIX_QMON_C_ */
