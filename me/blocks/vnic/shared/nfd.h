/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd.h
 * @brief         NFD shared defines and macros
 */
#ifndef _BLOCKS__VNIC_SHARED_NFD_H_
#define _BLOCKS__VNIC_SHARED_NFD_H_

#include <vnic/shared/nfcc_chipres.h>

/* User configuration defines */
#define NFD_PCIE0_EMEM      0
#define NFD_PCIE1_EMEM      0
#define NFD_PCIE2_EMEM      1
#define NFD_PCIE3_EMEM      1

/* Set defines */
#define NFD_MAX_ISL     4   /* Maximum number of PCIe islands NFD may support */


/* Helper macros */
#define NFD_EMEM_IND2(_emem) __LoadTimeConstant("__addr_emem" #_emem)
#define NFD_EMEM_IND1(_emem) NFD_EMEM_IND2(_emem)
#define NFD_EMEM_IND0(_isl) NFD_EMEM_IND1(NFD_PCIE##_isl##_EMEM)
#define NFD_EMEM(_isl) NFD_EMEM_IND0(_isl)

#define NFD_RING_ALLOC_IND2(_emem, _name, _num)                 \
_alloc_resource(_name emem##_emem##_queues global _num _num)
#define NFD_RING_ALLOC_IND1(_emem, _name, _num)                 \
    NFD_RING_ALLOC_IND2(_emem, _name, _num)
#define NFD_RING_ALLOC_IND0(_isl, _comp, _num)                  \
    NFD_RING_ALLOC_IND1(NFD_PCIE##_isl##_EMEM,                  \
                        _comp##_num_start_isl##_isl, _num)
#define NFD_RING_ALLOC(_isl, _comp, _num)                       \
    NFD_RING_ALLOC_IND0(_isl, _comp, _num)


#define NFD_RING_INIT(_isl, _comp, _num)                                \
do {                                                                    \
    _comp##_ring_info[_isl].addr_hi = ((unsigned long long) NFD_EMEM(_isl) \
                                       >> 32);                          \
    _comp##_ring_info[_isl].sp0 = 0;                                    \
    _comp##_ring_info[_isl].rnum = NFD_RING_ALLOC(_isl, _comp, _num); \
} while(0)



/* Shared structures */
/**
 * Compact storage of NFD ring addresses and ring numbers
 */
struct nfd_ring_info {
    union {
        struct {
            unsigned int addr_hi:8;     /**< EMU access bits */
            unsigned int sp0:14;        /**< Spare */
            unsigned int rnum:10;       /**< Ring number */
        };
        unsigned int __raw;
    };
};


#endif /* !_BLOCKS__VNIC_SHARED_NFD_H_ */
