/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/gather_seq_recv.c
 * @brief         Receive gather_dma_seq_compl and expose it to local ME
 */

#include <assert.h>
#include <nfp.h>

#include "gather_seq_recv.h"

__visible volatile __xread unsigned int tx_gather_reflect_xread;
__visible volatile SIGNAL tx_gather_reflect_sig;
__shared __gpr unsigned int gather_dma_seq_compl;



void
init_gather_seq_recv()
{
    /* XXX assign value in declaration if there is nothing else to do here */
    gather_dma_seq_compl = 0;
}

void
gather_seq_recv()
{
    if (signal_test(&tx_gather_reflect_sig)) {
        gather_dma_seq_compl = tx_gather_reflect_xread;
    }
}
