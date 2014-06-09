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
__visible volatile __xread unsigned int tx_data_reflect_xread;
__visible volatile SIGNAL tx_gather_reflect_sig;
__visible volatile SIGNAL tx_data_reflect_sig;
__shared __gpr unsigned int gather_dma_seq_compl = 0;
__shared __gpr unsigned int data_dma_seq_compl = 0;


void
init_gather_seq_recv()
{

}

void
gather_seq_recv()
{
    if (signal_test(&tx_gather_reflect_sig)) {
        gather_dma_seq_compl = tx_gather_reflect_xread;
    }

    if (signal_test(&tx_data_reflect_sig)) {
        data_dma_seq_compl = tx_data_reflect_xread;
    }
}
