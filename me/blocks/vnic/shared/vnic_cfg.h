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
     NS_VNIC_CFG_CTRL_LSO)
#endif

#ifndef VNIC_CFG_MAX_MTU
#define VNIC_CFG_MAX_MTU        1518
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

struct vnic_cfg_msg {
    union {
        struct {
            unsigned int msg_valid:1;
            unsigned int error:1;
            unsigned int spare:22;
            unsigned int vnic:8;
        };
        unsigned int __raw;
    };
};

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

#endif /* !_BLOCKS__SHARED_VNIC_CFG_H_ */
