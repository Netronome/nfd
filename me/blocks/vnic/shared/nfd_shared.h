/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_shared.h
 * @brief         NFD shared defines and macros
 */
#ifndef _BLOCKS__VNIC_SHARED_NFD_SHARED_H_
#define _BLOCKS__VNIC_SHARED_NFD_SHARED_H_

#include <nfcc_chipres.h>

#define NFD_PCIE0_EMEM      0
#define NFD_PCIE1_EMEM      0
#define NFD_PCIE2_EMEM      1
#define NFD_PCIE3_EMEM      1

#define NFD_EMEM_IND2(_emem) __LoadTimeConstant("__addr_emem" #_emem)
#define NFD_EMEM_IND1(_emem) NFD_EMEM_IND2(_emem)
#define NFD_EMEM_IND0(_isl) NFD_EMEM_IND1(NFD_PCIE##_isl##_EMEM)
#define NFD_EMEM(_isl) NFD_EMEM_IND0(_isl)

#ifndef NFD_WQ_SZ
#define NFD_WQ_SZ           (16 * 1024)
#endif

#define NFD_NUM_WQS         8

#define NFD_RING_BASE_IND(_isl, _comp)   _comp##_ring_isl##_isl
#define NFD_RING_BASE(_isl, _comp)       NFD_RING_BASE_IND(_isl, _comp)

#define NFD_RING_DECLARE_IND1(_isl, _emem, _comp, _sz)                  \
    __export __emem_n(_emem) __align(_sz)                               \
    unsigned int _comp##_ring_isl##_isl##[_sz / 4]
#define NFD_RING_DECLARE_IND0(_isl, _comp, _sz)                         \
    NFD_RING_DECLARE_IND1(_isl, NFD_PCIE##_isl##_EMEM, _comp, _sz)
#define NFD_RING_DECLARE(_isl, _comp, _sz)                              \
    NFD_RING_DECLARE_IND0(_isl, _comp, _sz)


#define NFD_RING_ALLOC_IND2(_isl, _emem, _name, _num)           \
_alloc_resource(_name emem##_emem##_queues global _num _num)
#define NFD_RING_ALLOC_IND1(_isl, _emem, _name, _num)           \
    NFD_RING_ALLOC_IND2(_isl, _emem, _name, _num)
#define NFD_RING_ALLOC_IND0(_isl, _comp, _num)                  \
    NFD_RING_ALLOC_IND1(_isl, NFD_PCIE##_isl##_EMEM,            \
                        _comp##_num_start_isl##_isl, _num)
#define NFD_RING_ALLOC(_isl, _comp, _num)                       \
    NFD_RING_ALLOC_IND0(_isl, _comp, _num)


#define NFD_RING_CONFIGURE_IND(_isl, _comp)                     \
    MEM_RING_CONFIGURE(_comp##_ring_isl##_isl##, in_ring_num)
#define NFD_RING_CONFIGURE(_isl, _comp) NFD_RING_CONFIGURE_IND(_isl, _comp)

/* XXX can provide an extra _pool parameter here if required */
#define NFD_BLM_Q_ALLOC_IND(_name)                  \
    _alloc_resource(_name BLQ_EMU_RINGS global 1)
#define NFD_BLM_Q_ALLOC(_name) NFD_BLM_Q_ALLOC_IND(_name)


/*
 * Allocate 256B (64 x 4B) of memory at offset 0 in CTM for credits.
 * Forcing this to zero on all PCIe islands makes code to access credits
 * simpler and more efficient throughout the system.
 */
#define NFD_CREDITS_ALLOC_IND(_off)                                    \
     _alloc_mem("nfd_pci_out_credits ctm+" #_off " island 256")
#define NFD_CREDITS_ALLOC(_off) NFD_CREDITS_ALLOC_IND(_off)

#endif /* !_BLOCKS__VNIC_SHARED_NFD_SHARED_H_ */
