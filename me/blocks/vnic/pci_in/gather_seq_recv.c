/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/gather_seq_recv.c
 * @brief         Receive gather_dma_seq_compl and expose it to local ME
 */

#include <assert.h>
#include <nfp.h>

#include "gather_seq_recv.h"

#include <vnic/pci_in/precache_bufs.h>


struct seq_recv_state {
    unsigned int recompute_seq_safe:1;
    unsigned int spare:31;
};

__visible volatile __xread unsigned int tx_gather_compl_reflect_xread;
__visible volatile __xread unsigned int tx_data_compl_reflect_xread;
__visible volatile __xread unsigned int tx_data_served_reflect_xread;
__visible volatile SIGNAL tx_gather_compl_reflect_sig;
__visible volatile SIGNAL tx_data_compl_reflect_sig;
__visible volatile SIGNAL tx_data_served_reflect_sig;
__shared __gpr unsigned int gather_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_served = 0;


static struct seq_recv_state state = {0, 0};


void
gather_seq_recv()
{
    if (signal_test(&tx_gather_compl_reflect_sig)) {
        gather_dma_seq_compl = tx_gather_compl_reflect_xread;
    }

    if (signal_test(&tx_data_compl_reflect_sig)) {
        data_dma_seq_compl = tx_data_compl_reflect_xread;

        state.recompute_seq_safe = 1;
    }

    if (signal_test(&tx_data_served_reflect_sig)) {
        data_dma_seq_served = tx_data_served_reflect_xread;

        state.recompute_seq_safe = 1;
    }

    if (state.recompute_seq_safe == 1) {
        precache_bufs_compute_seq_safe();
        state.recompute_seq_safe = 0;
    }
}
