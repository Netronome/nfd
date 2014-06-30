/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in/precache_bufs.h
 * @brief         Fill the local (LM) cache of MU buffers
 */
#ifndef _BLOCKS__VNIC_PCI_IN_PRECACHE_BUFS_H_
#define _BLOCKS__VNIC_PCI_IN_PRECACHE_BUFS_H_

/**
 * Initialise the precache_bufs block
 */
extern void precache_bufs_setup();

/**
 * If there is space in the local cache and no request outstanding, request
 * a batch of TX_BUF_RECACHE_WM buffers from the specified BLM queue.  If
 * there is a request outstanding check whether it returned and was filled.
 * Copy the buffers into the cache if the request succeeded. */
extern void precache_bufs();

/**
 * Get a buffer from the cache, updating the cache pointer.
 */
__intrinsic unsigned int precache_bufs_use();

/**
 * Check the number of buffers available in the cache.
 */
extern __inline unsigned int precache_bufs_avail();

/**
 * Compute a safe sequence for the issue_dma block based on the number of
 * buffers in the cache, the number of batches in "tx_issued_ring" and the
 * number of DMA batches in flight.
 *
 * Note that DMAs are only tracked per batch, with each batch assumed to be
 * filled.  This makes the DMA batches count slightly pessimistic if a few
 * batches are not full.
 */
extern __inline void precache_bufs_compute_seq_safe();

#endif /* !_BLOCKS__VNIC_PCI_IN_PRECACHE_BUFS_H_ */
