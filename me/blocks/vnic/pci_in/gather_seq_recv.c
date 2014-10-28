/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/gather_seq_recv.c
 * @brief         Receive gather_dma_seq_compl and expose it to local ME
 */

#include <assert.h>
#include <nfp.h>

#include <vnic/shared/nfd_internal.h>


__visible volatile __xread unsigned int nfd_in_gather_compl_refl_in;
__visible volatile __xread unsigned int nfd_in_data_compl_refl_in;
__visible volatile __xread unsigned int nfd_in_data_served_refl_in;
__visible volatile SIGNAL nfd_in_gather_compl_refl_sig;
__visible volatile SIGNAL nfd_in_data_compl_refl_sig;
__visible volatile SIGNAL nfd_in_data_served_refl_sig;
__shared __gpr unsigned int gather_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_served = 0;


static struct nfd_in_me1_state state = {0, 0, 0};


/**
 * Check for sequence number reflects from "distr_seqn" and copy to shared
 * registers.  Recompute data_dma_seq_safe if necessary.
 */
void
gather_seq_recv()
{
    if (signal_test(&nfd_in_gather_compl_refl_sig)) {
        gather_dma_seq_compl = nfd_in_gather_compl_refl_in;
    }

    if (signal_test(&nfd_in_data_compl_refl_sig)) {
        data_dma_seq_compl = nfd_in_data_compl_refl_in;

        state.recompute_seq_safe = 1;
    }

    if (signal_test(&nfd_in_data_served_refl_sig)) {
        data_dma_seq_served = nfd_in_data_served_refl_in;

        state.recompute_seq_safe = 1;
    }

    if (state.recompute_seq_safe == 1) {
        precache_bufs_compute_seq_safe();
        state.recompute_seq_safe = 0;
    }
}
