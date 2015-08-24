/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_cfg.h
 * @brief         An API to manage access to NFD configuration data
 */
#ifndef _BLOCKS__SHARED_NFD_CFG_H_
#define _BLOCKS__SHARED_NFD_CFG_H_

#include <nfp/mem_ring.h>
#include <nfp_net_ctrl.h>

#include <vnic/shared/nfcc_chipres.h>
#include <vnic/shared/nfd.h>

#include "nfd_user_cfg.h"


/* /\* XXX Magic number currently */
/*  * Set to official version number before release *\/ */
/* #define NFD_CFG_VERSION 0x1248 */

#ifndef NFD_CFG_CLASS
/* The user hasn't specified a firmware class, so we set default class */
#define NFD_CFG_CLASS NFD_CFG_CLASS_DEFAULT
#endif

#if NFD_CFG_CLASS == NFD_CFG_CLASS_DEFAULT
/* Firmwares with default class must have class version 0.
 * The behaviour of default class firmwares is fully specified by
 * the NFD major and minor version numbers. */
#ifndef NFD_CFG_CLASS_VERSION
#define NFD_CFG_CLASS_VERSION   0
#elif NFD_CFG_CLASS_VERSION != 0
#error "NFD_CFG_CLASS_VERSION must be zero for default class firmwares"
#endif
#endif


#define NFD_CFG_VERSION                         \
    (NFD_CFG_CLASS_VER(NFD_CFG_CLASS_VERSION) | \
     NFD_CFG_CLASS_TYPE(NFD_CFG_CLASS) |        \
     NFD_CFG_MAJOR_VERSION(NFD_CFG_MAJOR) |     \
     NFD_CFG_MINOR_VERSION(NFD_CFG_MINOR))


#ifndef NFD_CFG_MAX_MTU
#define NFD_CFG_MAX_MTU         1500
#endif

#ifndef NFD_CFG_RING_EMEM
#error "NFD_CFG_RING_EMEM must be defined by the user"
#endif


/* Configuration mechanism memory and ring defines */
#define NFD_CFG_TOTAL_RINGS     32
#define NFD_CFG_NUM_RINGS       8


/* It is convenient to have a value like NFD_MAX_VFS for the PF,
 * so we set one up based on whether PF queues are defined or not. */
#if (NFD_MAX_PF_QUEUES != 0)
#define NFD_MAX_PFS 1
#else
#define NFD_MAX_PFS 0
#endif


#define NFD_CFG_RINGS_RES_IND(_emem)                                    \
    _emem##_queues_DECL                                                 \
    ASM(.alloc_resource nfd_cfg_ring_nums _emem##_queues global \
        NFD_CFG_TOTAL_RINGS NFD_CFG_TOTAL_RINGS)
#define NFD_CFG_RINGS_RES(_emem) NFD_CFG_RINGS_RES_IND(_emem)

NFD_CFG_RINGS_RES(NFD_CFG_RING_EMEM);


#define NFD_CFG_RINGS_DECL_IND(_isl)                                    \
    ASM(.declare_resource nfd_cfg_ring_nums##_isl global NFD_CFG_NUM_RINGS \
        nfd_cfg_ring_nums)                                              \
    ASM(.alloc_resource nfd_cfg_ring_num##_isl##0 nfd_cfg_ring_nums##_isl \
        global 1)                                                       \
    ASM(.alloc_resource nfd_cfg_ring_num##_isl##1 nfd_cfg_ring_nums##_isl \
        global 1)                                                       \
    ASM(.alloc_resource nfd_cfg_ring_num##_isl##2 nfd_cfg_ring_nums##_isl \
        global 1)                                                       \
    ASM(.alloc_resource nfd_cfg_ring_num##_isl##3 nfd_cfg_ring_nums##_isl \
        global 1)                                                       \
    ASM(.alloc_resource nfd_cfg_ring_num##_isl##4 nfd_cfg_ring_nums##_isl \
        global 1)                                                       \
    ASM(.alloc_resource nfd_cfg_ring_num##_isl##5 nfd_cfg_ring_nums##_isl \
        global 1)

#define NFD_CFG_RINGS_DECL(_isl) NFD_CFG_RINGS_DECL_IND(_isl)


#if _nfp_has_island("pcie0")
NFD_CFG_RINGS_DECL(0);
#endif

#if _nfp_has_island("pcie1")
NFD_CFG_RINGS_DECL(1);
#endif

#if _nfp_has_island("pcie2")
NFD_CFG_RINGS_DECL(2);
#endif

#if _nfp_has_island("pcie3")
NFD_CFG_RINGS_DECL(3);
#endif

#define NFD_CFG_EMEM_IND1(_emem) __LoadTimeConstant("__addr_" #_emem)
#define NFD_CFG_EMEM_IND0(_emem) NFD_CFG_EMEM_IND1(_emem)
#define NFD_CFG_EMEM NFD_CFG_EMEM_IND0(NFD_CFG_RING_EMEM)


#define NFD_CFG_RING_NUM_IND(_isl, _num)        \
    _link_sym(nfd_cfg_ring_num##_isl##_num)
#define NFD_CFG_RING_NUM(_isl, _num) NFD_CFG_RING_NUM_IND(_isl, _num)

#define NFD_CFG_BASE_IND(_x) nfd_cfg_base##_x
#define NFD_CFG_BASE(_x) NFD_CFG_BASE_IND(_x)


/* XXX test for chip revision correctly */
/* Due to THB-350, BARs must be 2M aligned on A0 */
#if 1
#define NFD_CFG_BASE_DECLARE(_isl)                                   \
    ASM(.alloc_mem NFD_CFG_BASE(_isl) NFD_EMEM(_isl) global          \
        ((NFD_MAX_VFS + NFD_MAX_PFS) * NFP_NET_CFG_BAR_SZ) SZ_2M)
#else
#define NFD_CFG_BASE_DECLARE(_isl)                                   \
    ASM(.alloc_mem NFD_CFG_BASE(_isl) NFD_EMEM(_isl) global          \
        ((NFD_MAX_VFS + NFD_MAX_PFS) * NFP_NET_CFG_BAR_SZ)           \
        (NFD_MAX_VFS * NFP_NET_CFG_BAR_SZ * (1 +  NFD_MAX_PFS)))
#endif


#define NFD_CFG_BASE_LINK_IND1(_sym)                 \
    ((__emem char *) _link_sym(_sym))
#define NFD_CFG_BASE_LINK_IND0(_isl)                \
    NFD_CFG_BASE_LINK_IND1(NFD_CFG_BASE(_isl))
#define NFD_CFG_BASE_LINK(_isl) NFD_CFG_BASE_LINK_IND0(_isl)


/* XXX we can't use "|" here due to NFCC error, check and possibly JIRA. */
#define NFD_CFG_BAR(_base, _vnic)               \
    ((_base) + (_vnic * NFP_NET_CFG_BAR_SZ))

#define NFD_CFG_BAR_ISL(_isl, _vnic)            \
    NFD_CFG_BAR(NFD_CFG_BASE_LINK(_isl), _vnic)


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
 */
__intrinsic void nfd_cfg_check_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                                       SIGNAL *cfg_sig,
                                       unsigned int rnum);

/**
 * Convenience macro to check cfg_msg from the app ME master without rnum
 * @param _msg              message struct to fill
 * @param _sig              signal to check for messages
 * @param _pci              PCIe island
 */
#ifdef USE_SVC_ME
#define nfd_cfg_master_chk_cfg_msg(_msg, _sig, _pci)                    \
    nfd_cfg_check_cfg_msg((_msg), (_sig), NFD_CFG_RING_NUM(_pci, 5))
#else
#define nfd_cfg_master_chk_cfg_msg(_msg, _sig, _pci)                    \
    nfd_cfg_check_cfg_msg((_msg), (_sig), NFD_CFG_RING_NUM(_pci, 4))
#endif

/**
 * Notify the host that a cfg_msg has been processed
 * @param cfg_msg       message listing the queue that has been configured
 */
__intrinsic void nfd_cfg_app_complete_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                                              __dram void *isl_base);

/**
 * Pass this message to the next stage, and check for a new message
 * @param cfg_msg           completed message, refilled with new message
 * @param cfg_sig_remote    signal to set for next recipient
 * @param next_me           ME to signal
 * @param rnum_out          output ring number
 * @param rbase_out         output ring address
 * @param rnum_in           input ring number
 * @param rbase_in          input ring address
 */
__intrinsic void nfd_cfg_svc_complete_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                          __remote SIGNAL *cfg_sig_remote,
                          unsigned int next_me, unsigned int rnum_out,
                          unsigned int rnum_in);

#endif /* !_BLOCKS__SHARED_NFD_CFG_H_ */
