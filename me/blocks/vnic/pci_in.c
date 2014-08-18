/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/pci_in.c
 * @brief         Interface to PCI.IN
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>
#include <nfp/mem_ring.h>
#include <std/reg_utils.h>

#include <vnic/pci_in.h>
#include <vnic/shared/nfd_shared.h>
#include <vnic/shared/qc.h>


__intrinsic void
__nfd_pkt_recv(unsigned int pcie_isl, unsigned int workq,
               __xread struct nfd_pci_in_pkt_desc *pci_in_meta,
               sync_t sync, SIGNAL *sig)
{
    mem_ring_addr_t raddr;
    unsigned int rnum;

    switch (pcie_isl) {
    case 0:
        raddr = (unsigned long long) NFD_EMEM(0) >> 8;
        rnum = NFD_RING_ALLOC(0, pci_in, NFD_NUM_WQS);
        break;
    case 1:
        raddr = (unsigned long long) NFD_EMEM(1) >> 8;
        rnum = NFD_RING_ALLOC(1, pci_in, NFD_NUM_WQS);
        break;
    case 2:
        raddr = (unsigned long long) NFD_EMEM(2) >> 8;
        rnum = NFD_RING_ALLOC(2, pci_in, NFD_NUM_WQS);
        break;
    case 3:
        raddr = (unsigned long long) NFD_EMEM(3) >> 8;
        rnum = NFD_RING_ALLOC(3, pci_in, NFD_NUM_WQS);
        break;
    default:
        halt();
    }

    rnum |= workq;

    __mem_workq_add_thread(rnum, raddr, pci_in_meta, sizeof(*pci_in_meta),
                           sizeof(*pci_in_meta), sync, sig);
}


void
pci_in_map_queue(unsigned int *vnic, unsigned int *queue,
                 unsigned int nfd_queue)
{
    unsigned int natural_queue;

    natural_queue = map_bitmask_to_natural(nfd_queue);
    *queue = natural_queue & (MAX_VNIC_QUEUES - 1);
    *vnic = natural_queue / MAX_VNIC_QUEUES;
}


__intrinsic unsigned int
pci_in_pkt_len(__xread struct nfd_pci_in_pkt_desc *pci_in_meta)
{
    return pci_in_meta->data_len - pci_in_meta->offset;
}


