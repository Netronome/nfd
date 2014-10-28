/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_cfg.h
 * @brief         An API to manage access to NFD configuration data
 */
#ifndef _BLOCKS__SHARED_NFD_CFG_H_
#define _BLOCKS__SHARED_NFD_CFG_H_

#include <nfp/mem_ring.h>
#include <ns_vnic_ctrl.h>

#include <vnic/shared/nfcc_chipres.h>

/* XXX Magic number currently
 * Set to official version number before release */
#define NFD_CFG_VERSION 0x1248

#ifndef NFD_CFG_CAP
/* XXX Set some "random" bits in the capabilities field
 * for testing purposes */
#define NFD_CFG_CAP                                             \
    (NS_VNIC_CFG_CTRL_ENABLE | NS_VNIC_CFG_CTRL_PROMISC |       \
     NS_VNIC_CFG_CTRL_RXCSUM | NS_VNIC_CFG_CTRL_TXCSUM |        \
     NS_VNIC_CFG_CTRL_RXVLAN | NS_VNIC_CFG_CTRL_TXVLAN)
#endif

#ifndef NFD_CFG_MAX_MTU
#define NFD_CFG_MAX_MTU         1500
#endif


/* Minimum size configuration rings are fine */
#define NFD_CFG_TOTAL_RINGS     16
#define NFD_CFG_NUM_RINGS       4
#define NFD_CFG_RING_EMEM       2


#define NFD_CFG_EMEM_IND1(_emem) __LoadTimeConstant("__addr_emem" #_emem)
#define NFD_CFG_EMEM_IND0(_emem) NFD_CFG_EMEM_IND1(_emem)
#define NFD_CFG_EMEM NFD_CFG_EMEM_IND0(NFD_CFG_RING_EMEM)

#define NFD_CFG_RING_ALLOC_IND1(_emem, _name, _num)             \
_alloc_resource(_name emem##_emem##_queues global _num _num)
#define NFD_CFG_RING_ALLOC_IND0(_emem, _name, _num)             \
    NFD_CFG_RING_ALLOC_IND1(_emem, _name, _num)
#define NFD_CFG_RING_ALLOC                                        \
    NFD_CFG_RING_ALLOC_IND0(NFD_CFG_RING_EMEM, nfd_cfg_num_start, \
                            NFD_CFG_TOTAL_RINGS)

#define NFD_CFG_RING_NUM_IND(_isl, _num)                    \
    (NFD_CFG_RING_ALLOC | _isl * NFD_CFG_NUM_RINGS | _num)
#define NFD_CFG_RING_NUM(_isl, _num) NFD_CFG_RING_NUM_IND(_isl, _num)

#define NFD_CFG_BASE_IND(_x) nfd_cfg_base##_x
#define NFD_CFG_BASE(_x) NFD_CFG_BASE_IND(_x)

/* XXX remove EMU specification */
#define NFD_CFG_BASE_DECLARE(_isl)                                  \
    __export __emem_n(2) __align(NS_VNIC_CFG_BAR_SZ * MAX_VNICS)    \
        char NFD_CFG_BASE(_isl)[MAX_VNICS][NS_VNIC_CFG_BAR_SZ];


/* XXX TEMP */
enum nfd_cfg_component {
    NFD_CFG_PCI_IN,
    NFD_CFG_PCI_OUT
};


/**
 * @param msg_valid     message contains valid information
 * @param error         an error has been detected
 * @param interested    this component must process this message
 * @param up_bit        the current queue is up
 * @param spare         spare bits
 * @param queue         queue number to process
 * @param vnic          vnic to process
 *
 * This structure is passed between NFD components and used internally
 * to carry configuration messages.  'vnic', 'msg_valid', and 'error' are
 * passed between components (other fields must be zeroed).  'msg_valid' is
 * used to determine whether to process a configuration message, and must be
 * set when passing the structure to the next stage.  Unsetting this bit signals
 * that processing in this stage is complete.  'up_bit' and 'queue' are only
 * valid if 'msg_valid' and 'interested' are set, and 'error' is not set.
 */
struct nfd_cfg_msg {
    union {
        struct {
            unsigned int msg_valid:1;
            unsigned int error:1;
            unsigned int interested:1;
            unsigned int up_bit:1;
            unsigned int spare:12;
            unsigned int queue:8;
            unsigned int vnic:8;
        };
        unsigned int __raw;
    };
};


/**
 * Perform shared configuration on "cfg_msg" and "cfg_sig".
 * @param cfg_sig       signal set to indicate that a message is ready
 * @param cfg_msg       current configuration message and state
 */
__intrinsic void nfd_cfg_init_cfg_msg(SIGNAL *cfg_sig,
                                      struct nfd_cfg_msg *cfg_msg);


/**
 * Check for a cfg_msg  on a NFD ME
 * @param cfg_msg           message struct to fill
 * @param cfg_sig           signal to check for messages
 * @param rnum              ring number to fetch messages from
 * @param rbase             base address of the ring to use
 */
__intrinsic void nfd_cfg_check_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                                       SIGNAL *cfg_sig,
                                       unsigned int rnum);


/**
 * Notify the host that a cfg_msg has been processed
 * @param cfg_msg       message listing the queue that has been configured
 */
__intrinsic void nfd_cfg_app_complete_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                                              __dram void *isl_base);

#endif /* !_BLOCKS__SHARED_NFD_CFG_H_ */
