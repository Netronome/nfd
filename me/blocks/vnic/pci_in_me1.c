/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in_cfg.h
 * @brief         Code to deploy on PCIe ME3 in NFD with 2 MEs per direction
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>

#include <vnic/pci_in/gather_seq_recv.h>
#include <vnic/pci_in/issue_dma.h>
#include <vnic/shared/vnic_cfg.h>

 /*
  * Temporary header includes
  */
#include <vnic/pci_in/pci_in_internal.h>    /* TEMP */
#include <vnic/shared/qc.h>                 /* TEMP */
#include <vnic/utils/nn_ring.h>             /* TEMP */
#include <vnic/utils/qcntl.h>               /* TEMP */

/*
 * Temporary variables TEMP
 */
extern __shared __gpr unsigned int gather_dma_seq_compl;
MEM_JOURNAL_DECLARE(work_batches, 1024);
__shared __gpr mem_ring_addr_t work_batches_addr;
__shared __gpr unsigned int tmp_dma_seq_done = 0;

static __forceinline void
tmp_advance_tx_r()
{
    /* Check "DMA" completed and we can read the batch
     * If so, the NN ring MUST have a batch descriptor for us */
    if (gather_dma_seq_compl != tmp_dma_seq_done) {
        struct batch_desc batch;
        unsigned char qc_queue;

        if (nn_ring_empty()) {
            halt();          /* A serious error has occurred */
        }

        /* Read the batch */
        batch.__raw = nn_ring_get();

        /* Journal the batch */
        mem_ring_journal_fast(0, work_batches_addr, batch.__raw); /* no swap */

        /* Increment tmp_dma_seq_done BEFORE swapping */
        tmp_dma_seq_done++;

        /* Map batch.queue to a QC queue and increment the TX_R pointer
         * for that queue by batch.num */
        qc_queue = map_bitmask_to_natural(batch.queue) << 1;
        qc_add_to_ptr(PCIE_ISL, qc_queue, QC_RPTR, batch.num); /* Swaps! */
    }
}


int
main(void)
{
    /* Perform per ME initialisation  */
    if (ctx() == 0) {
        /* init_buffer_precache(); */

        init_gather_seq_recv();

        nn_ring_init_receive(0, 0);             /* TEMP */
        work_batches_addr = MEM_JOURNAL_CONFIGURE(work_batches, 0); /* TEMP */

        issue_dma_setup_shared();

    } else {
        /* init_tx_desc_cls_fetch(); */

        /* init_dma_patch_issue();  */
    }

    /* Kickoff ordering stages! */

    /*
     * Work loop
     */
    if (ctx() == 0) {
        /* CTX0 main loop */
        for (;;) {
            gather_seq_recv();

            /* Yield thread */
            ctx_swap();
        }
    } else {
        /* Worker main loop */
        for (;;) {
            tmp_advance_tx_r(); /* TEMP */

            /* Yield thread */
            ctx_swap();
        }
    }
}
