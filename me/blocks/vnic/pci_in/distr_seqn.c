/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/distr_seqn.c
 * @brief         Code to compute and distribute DMA sequence numbers
 */

#include <assert.h>
#include <nfp.h>

#include <std/event.h>

#include <nfp6000/nfp_cls.h>

#include <vnic/pci_in/distr_seqn.h>

#include <vnic/pci_in_cfg.h>

/*
 * Temporary header includes
 */
#include <nfp/me.h>             /* TEMP */
#include <nfp6000/nfp_me.h>     /* TEMP */

static volatile __xread unsigned int tx_gather_event_xfer;
static volatile __xread unsigned int tx_data_event_xfer;
static volatile SIGNAL tx_gather_event_sig;
static volatile SIGNAL tx_data_event_sig;

static volatile __xwrite unsigned int tx_gather_reflect_xwrite;

/*
 * Sequence numbers visible to other code on this ME
 * NB: avoid name clashes
 */
__shared __gpr unsigned int gather_dma_seq_compl;
__shared __gpr unsigned int data_dma_seq_compl;

__remote volatile __xread unsigned int tx_gather_reflect_xread;
__remote volatile SIGNAL tx_gather_reflect_sig;


/* XXX Move to some sort of CT reflect library */
__intrinsic void
reflect_data(unsigned int dst_me, unsigned int dst_xfer,
             unsigned int sig_no, volatile __xwrite void *src_xfer,
             size_t size)
{
    #define OV_SIG_NUM 13

    unsigned int addr;
    unsigned int count = (size >> 2);
    struct nfp_mecsr_cmd_indirect_ref_0 indirect;

    /* ctassert(__is_write_reg(src_xfer)); */ /* TEMP, avoid volatile warnings */
    ctassert(__is_ct_const(size));

    /* Generic address computation.
     * Could be expensive if dst_me, or dst_xfer
     * not compile time constants */
    addr = ((dst_me & 0xFF0)<<20 | ((dst_me & 15)<<10 | (dst_xfer & 15)<<2));

    indirect.__raw = 0;
    indirect.signal_num = sig_no;
    local_csr_write(NFP_MECSR_CMD_INDIRECT_REF_0, indirect.__raw);

    /* Currently just support reflect_write_sig_remote */
    __intrinsic_begin();
    __asm {
        alu[--, --, b, 1, <<OV_SIG_NUM];
        ct[reflect_write_sig_remote, *src_xfer, addr, 0, \
           __ct_const_val(count)], indirect_ref;
    };
    __intrinsic_end();
}

void
init_distr_seqn()
{
    /* XXX this code would probably be much more efficient if ctx and meid
     * are known at compile time. */
    unsigned int ctx = ctx();
    unsigned int meid = __MEID;
    struct nfp_em_filter_status filter_status;
    __cls struct event_cls_filter *tx_gather_event_filter;
    __cls struct event_cls_filter *tx_data_event_filter;

    /* Strict match type, loose match provider (TEMP), match all seqn */
    unsigned int event_mask = NFP_EVENT_MATCH(0xFC, 0, 0xF);
    unsigned int event_match;
    unsigned int pcie_provider = NFP_EVENT_PROVIDER_NUM(
        meid>>4, NFP_EVENT_PROVIDER_INDEX_CLS); /* TEMP _PCIE);*/

    gather_dma_seq_compl = 0;
    data_dma_seq_compl = 0;

    /*
     * Set filter status.
     * We can throttle autopush rate by setting non-zero values here,
     * but resetting the autopush inherently throttles the rate as well.
     */
    filter_status.__raw = 0;

    /*
     * Setup gather filter
     */
    tx_gather_event_filter = event_cls_filter_handle(TX_GATHER_EVENT_FILTER);
    event_match = NFP_EVENT_MATCH(pcie_provider, 0, TX_GATHER_EVENT_TYPE);
    event_cls_filter_setup(tx_gather_event_filter,
                           NFP_EM_FILTER_MASK_TYPE_LASTEV,
                           event_match, event_mask, filter_status);
    event_cls_autopush_signal_setup(TX_GATHER_EVENT_FILTER, meid, ctx,
                                    __signal_number(&tx_gather_event_sig),
                                    __xfer_reg_number(&tx_gather_event_xfer));
    event_cls_autopush_filter_reset(
        TX_GATHER_EVENT_FILTER,
        NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
        TX_GATHER_EVENT_FILTER);

    /*
     * Setup data filter
     */
    tx_data_event_filter = event_cls_filter_handle(TX_DATA_EVENT_FILTER);
    event_match = NFP_EVENT_MATCH(pcie_provider, 0, TX_DATA_EVENT_TYPE);
    event_cls_filter_setup(tx_data_event_filter,
                           NFP_EM_FILTER_MASK_TYPE_LASTEV,
                           event_match, event_mask, filter_status);
    event_cls_autopush_signal_setup(TX_DATA_EVENT_FILTER, meid, ctx,
                                    __signal_number(&tx_data_event_sig),
                                    __xfer_reg_number(&tx_data_event_xfer));
    event_cls_autopush_filter_reset(
        TX_DATA_EVENT_FILTER,
        NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
        TX_DATA_EVENT_FILTER);
}

void
distr_seqn()
{
    unsigned int seqn_inc;
    struct nfp_event_match event;

    /*
     * Check for an event, updating sequence number,
     * resetting the autopush, and mirroring to remote ME if necessary
     */

    /* Gather */
    if (signal_test(&tx_gather_event_sig)) {
        /* Compute increase and update gather_dma_seq_compl */
        event.__raw = tx_gather_event_xfer;
        seqn_inc = (event.source - gather_dma_seq_compl) & 0xFFF;
        gather_dma_seq_compl += seqn_inc;

        /* Mirror to remote ME TODO! */
        tx_gather_reflect_xwrite = gather_dma_seq_compl;
        reflect_data(TX_DATA_DMA_ME,
                     __xfer_reg_number(&tx_gather_reflect_xread,
                                       TX_DATA_DMA_ME),
                     __signal_number(&tx_gather_reflect_sig, TX_DATA_DMA_ME),
                     &tx_gather_reflect_xwrite,
                     sizeof tx_gather_reflect_xwrite);

        /* Reset autopush */
        event_cls_autopush_filter_reset(
            TX_GATHER_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            TX_GATHER_EVENT_FILTER);
    }

    /* Data */
    if (signal_test(&tx_data_event_sig)) {
        /* Compute increase and update gather_dma_seq_compl */
        event.__raw = tx_data_event_xfer;
        seqn_inc = (event.source - data_dma_seq_compl) & 0xFFF;
        data_dma_seq_compl += seqn_inc;

        /* Mirror to remote ME TODO! */

        /* Reset autopush */
        event_cls_autopush_filter_reset(
            TX_DATA_EVENT_FILTER,
            NFP_CLS_AUTOPUSH_STATUS_MONITOR_ONE_SHOT_ACK,
            TX_DATA_EVENT_FILTER);
    }
}

