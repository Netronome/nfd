/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/vnic_cfg.h
 * @brief         An API to manage access to NFD configuration data
 */
#ifndef _BLOCKS__SHARED_VNIC_CFG_H_
#define _BLOCKS__SHARED_VNIC_CFG_H_

#include <nfp/mem_ring.h>
#include <ns_vnic_ctrl.h>

/* XXX Magic number currently
 * Set to official version number before release */
#define VNIC_CFG_VERSION 0x1248

#ifndef VNIC_CFG_CAP
/* XXX Set some "random" bits in the capabilities field
 * for testing purposes */
#define VNIC_CFG_CAP                                            \
    (NS_VNIC_CFG_CTRL_ENABLE | NS_VNIC_CFG_CTRL_PROMISC |       \
     NS_VNIC_CFG_CTRL_RXCSUM | NS_VNIC_CFG_CTRL_TXCSUM |        \
     NS_VNIC_CFG_CTRL_RXVLAN | NS_VNIC_CFG_CTRL_TXVLAN)
#endif

#ifndef VNIC_CFG_MAX_MTU
#define VNIC_CFG_MAX_MTU        1500
#endif

/* XXX allocate using generic resource management */
#define REQUESTER_ID_BASE       (6<<6)

/* Minimum size configuration rings are fine */
#define VNIC_CFG_RING_SZ        512

/* XXX allocated this via generic resource allocation */
#define VNIC_CFG_RING_NUM_START 256

#define VNIC_CFG_QUEUE          1
#define VNIC_CFG_EVENT_DATA     (2<<4)
#define VNIC_CFG_EVENT_FILTER   13

#define VNIC_CFG_DECLARE(_sig, _next_sig) \
    __visible SIGNAL _sig;                \
    __remote SIGNAL _next_sig;


#define VNIC_CFG_NEXT_ME_IND1(_me_str) __nfp_idstr2meid(#_me_str)
#define VNIC_CFG_NEXT_ME_IND0(_isl, _me)        \
    VNIC_CFG_NEXT_ME_IND1(pcie##_isl##.me##_me)

#ifndef VNIC_CFG_NEXT_ME
#define VNIC_CFG_NEXT_ME(_isl, _me) VNIC_CFG_NEXT_ME_IND0(_isl, _me)
#endif

#define VNIC_CFG_RING_ADDR_IND(_isl, _num) vnic_cfg_pcie##_isl##_ring##_num
#define VNIC_CFG_RING_ADDR(_isl, _num) VNIC_CFG_RING_ADDR_IND(_isl, _num)

#define VNIC_CFG_RING_NUM_IND(_isl, _num) \
    (VNIC_CFG_RING_NUM_START + 4 * _isl + _num)
#define VNIC_CFG_RING_NUM(_isl, _num) VNIC_CFG_RING_NUM_IND(_isl, _num)

MEM_RING_DECLARE(VNIC_CFG_RING_ADDR(PCIE_ISL, 0), VNIC_CFG_RING_SZ);
MEM_RING_DECLARE(VNIC_CFG_RING_ADDR(PCIE_ISL, 1), VNIC_CFG_RING_SZ);
MEM_RING_DECLARE(VNIC_CFG_RING_ADDR(PCIE_ISL, 2), VNIC_CFG_RING_SZ);

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
struct vnic_cfg_msg {
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

enum vnic_cfg_component {
    VNIC_CFG_PCI_IN,
    VNIC_CFG_PCI_OUT,
    VNIC_CFG_APP_MASTER
};

__intrinsic void vnic_cfg_setup_pf();

void nfd_ctrl_write_max_qs(unsigned int vnic);

void vnic_cfg_setup();

__intrinsic void vnic_cfg_init_cfg_msg(SIGNAL *cfg_sig,
                                       struct vnic_cfg_msg *cfg_msg);

void vnic_cfg_check_cfg_ap();

int vnic_cfg_next_vnic();

__intrinsic void vnic_cfg_start_cfg_msg(struct vnic_cfg_msg *cfg_msg,
                                        __remote SIGNAL *cfg_sig_remote,
                                        unsigned int next_me, unsigned int rnum,
                                        __dram void *rbase);

__intrinsic void vnic_cfg_app_check_cfg_msg(SIGNAL *cfg_sig, unsigned int rnum,
                                        __dram void *rbase);

__intrinsic void vnic_cfg_check_cfg_msg(struct vnic_cfg_msg *cfg_msg,
                                        SIGNAL *cfg_sig,
                                        unsigned int rnum,
                                        __dram void *rbase);

__intrinsic void vnic_cfg_complete_cfg_msg(struct vnic_cfg_msg *cfg_msg,
                                           __remote SIGNAL *cfg_sig_remote,
                                           unsigned int next_me,
                                           unsigned int rnum_out,
                                           __dram void *rbase_out,
                                           unsigned int rnum_in,
                                           __dram void *rbase_in);

/**
 * Read configuration message from BAR and interpret fields
 * @param cfg_msg       holds state related to the configuration request
 * @param comp          which component to specialise code for
 *
 * This method performs basic consistency checks on the configuration BAR
 * information, and determines whether the message affects the current
 * component.  It sets up internal data such as caching ring enables, and
 * also reads the first ring address and ring sizes (if necessary).  This
 * prepares the internal state for the 'vnic_cfg_proc_msg' method.
 */
__intrinsic void vnic_cfg_parse_msg(struct vnic_cfg_msg *cfg_msg,
                                    enum vnic_cfg_component comp);

/**
 * Extract BAR information
 * @param cfg_msg       holds state related to the configuration request
 * @param queue         which queue to process next
 * @param ring_sz       size of the ring
 * @param ring_base     base address for the ring
 * @param comp          which component to specialise code for
 *
 * Test 'cfg_msg' and internal state to determine whether any queue
 * configuration must be changed in this configuration cycle.  If not, this is
 * indicated through flags in 'cfg_msg', otherwise at least queue will be valid.
 * If the queue must be "up'ed", 'ring_sz' and 'ring_base' will also be valid.
 */
__intrinsic void vnic_cfg_proc_msg(struct vnic_cfg_msg *cfg_msg,
                                   unsigned char *queue, unsigned char *ring_sz,
                                   unsigned int *ring_base,
                                   enum vnic_cfg_component comp);

#endif /* !_BLOCKS__SHARED_VNIC_CFG_H_ */
