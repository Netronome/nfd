/*
 * Copyright (C) 2014-2018,  Netronome Systems, Inc.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file          blocks/vnic/shared/nfd_cfg.h
 * @brief         An API to manage access to NFD configuration data
 */
#ifndef _BLOCKS__SHARED_NFD_CFG_H_
#define _BLOCKS__SHARED_NFD_CFG_H_

#include <nfp_chipres.h>

#include <nfp_net_ctrl.h>

#include <nfd_user_cfg.h>

#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_ctrl.h>


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


#ifndef NFD_CFG_MAX_MTU
#define NFD_CFG_MAX_MTU         1500
#endif

#ifndef NFD_CFG_RING_EMEM
#error "NFD_CFG_RING_EMEM must be defined by the user"
#endif


/* Configuration mechanism memory and ring defines */
#define NFD_CFG_TOTAL_RINGS     32
#define NFD_CFG_NUM_RINGS       8


/* Config message field declarations */
#define NFD_CFG_MSG_VALID_bf            0, 31, 31
#define NFD_CFG_MSG_VALID_wrd           0
#define NFD_CFG_MSG_VALID_shf           31
#define NFD_CFG_MSG_VALID_msk           0x1
#define NFD_CFG_MSG_VALID_bit           31
#define NFD_CFG_MSG_ERR_bf              0, 30, 30
#define NFD_CFG_MSG_ERR_wrd             0
#define NFD_CFG_MSG_ERR_shf             30
#define NFD_CFG_MSG_ERR_msk             0x1
#define NFD_CFG_MSG_ERR_bit             30
#define NFD_CFG_MSG_INTERESTED_bf       0, 29, 29
#define NFD_CFG_MSG_INTERESTED_wrd      0
#define NFD_CFG_MSG_INTERESTED_shf      29
#define NFD_CFG_MSG_INTERESTED_msk      0x1
#define NFD_CFG_MSG_INTERESTED_bit      29
#define NFD_CFG_MSG_UP_bf               0, 28, 28
#define NFD_CFG_MSG_UP_wrd              0
#define NFD_CFG_MSG_UP_shf              28
#define NFD_CFG_MSG_UP_msk              0x1
#define NFD_CFG_MSG_UP_bit              28
#define NFD_CFG_MSG_QUEUE_bf            0, 15, 8
#define NFD_CFG_MSG_QUEUE_wrd           0
#define NFD_CFG_MSG_QUEUE_shf           8
#define NFD_CFG_MSG_QUEUE_msk           0xFF
#define NFD_CFG_MSG_VID_bf              0, 7, 0
#define NFD_CFG_MSG_VID_wrd             0
#define NFD_CFG_MSG_VID_shf             0
#define NFD_CFG_MSG_VID_msk             0xFF


/*
 * Defines that set the structure of "nfd_flr_seen".  These are also
 * reused for FLR state in nfd_cfg_internal.c.
 */
#define NFD_FLR_VF_LO_ind   0
#define NFD_FLR_VF_HI_ind   1
#define NFD_FLR_PF_ind      2
#define NFD_FLR_PF_shf      0

#define NFD_FLR_PEND_BUSY_shf 31

/*
 * Defines used to signal PCI.IN ME0 on completion of FLR processing
 */
#define NFD_CFG_FLR_AP_SIG_NO       7
#define NFD_CFG_FLR_AP_CTX_NO       1
#define NFD_CFG_ISL_MASTER_BASE     0x45

/*
 * Defines to use to access FLR hardware CSRs
 */
#define NFP_PCIEX_ISL_BASE                                   0x04000000
#define NFP_PCIEX_ISL_shf                                    24
/* XXX temp defines that loosely match the BSP pcie_monitor_api.h */
#define NFP_PCIEX_COMPCFG_CNTRLR3                            0x0010006c
#define NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_CHANNEL_msk    0x3f
#define NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_CHANNEL_shf    16
#define NFP_PCIEX_COMPCFG_CNTRLR3_VF_FLR_DONE_shf            15
#define NFP_PCIEX_COMPCFG_CNTRLR3_FLR_DONE_shf               14
#define NFP_PCIEX_COMPCFG_CNTRLR3_FLR_IN_PROGRESS_shf        13
#define NFP_PCIEX_COMPCFG_PCIE_VF_FLR_IN_PROGRESS0           0x00100080
#define NFP_PCIEX_COMPCFG_PCIE_VF_FLR_IN_PROGRESS1           0x00100084
#define NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT             0x001000cc
#define NFP_PCIEX_COMPCFG_PCIE_STATE_CHANGE_STAT_msk         0x3f
#define NFP_PCIEX_PCIE_INT_MGR_STATUS                        0x00130000
#define NFP_PCIEX_PCIE_INT_MGR_STATUS_FLR_msk                0x300
#define NFP_PCIEX_COMPCFG_CFG0                               0x00100000
#define NFP_PCIEX_COMPCFG_CFG0_MSG_VALID_shf                 11


#if defined(__NFP_LANG_MICROC)

#ifndef _link_sym
#define _link_sym(x) __link_sym(#x)
#endif


#define NFD_CFG_VERSION(_type)                      \
    (NFD_CFG_CLASS_VER(NFD_CFG_CLASS_VERSION) |     \
     NFD_CFG_CLASS_TYPE(NFD_CFG_CLASS) |            \
     NFD_CFG_MAJOR_VERSION(NFD_CFG_MAJOR_##_type) |  \
     NFD_CFG_MINOR_VERSION(NFD_CFG_MINOR_##_type))


/* Configuration mechanism memory and ring defines */
#define NFD_CFG_RINGS_RES_IND(_emem)                                    \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_nums _emem##_queues   \
                     global NFD_CFG_TOTAL_RINGS NFD_CFG_TOTAL_RINGS)
#define NFD_CFG_RINGS_RES(_emem) NFD_CFG_RINGS_RES_IND(_emem)

NFD_CFG_RINGS_RES(NFD_CFG_RING_EMEM);


#define NFD_CFG_RINGS_DECL_IND(_isl)                                    \
    _NFP_CHIPRES_ASM(.declare_resource nfd_cfg_ring_nums##_isl          \
                     global NFD_CFG_NUM_RINGS nfd_cfg_ring_nums)        \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_num##_isl##0          \
                     nfd_cfg_ring_nums##_isl global 1)                  \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_num##_isl##1          \
                     nfd_cfg_ring_nums##_isl global 1)                  \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_num##_isl##2          \
                     nfd_cfg_ring_nums##_isl global 1)                  \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_num##_isl##3          \
                     nfd_cfg_ring_nums##_isl global 1)                  \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_num##_isl##4          \
                     nfd_cfg_ring_nums##_isl global 1)                  \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_cfg_ring_num##_isl##5          \
                     nfd_cfg_ring_nums##_isl global 1)

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


/*
 * The "nfd_flr_seen" atomic is used to track which FLRs have been noticed by
 * by PCI.IN ME0 but have not been completed by the service ME.  It prevents
 * us reissuing an FLR message if something else causes us to check the
 * hardware pending mask.
 *
 * "nfd_flr_seen" is structured as an "array" of 16B entries per PCIe island.
 * Each entry consists of two 32bit bitmasks "VF_LO" and "VF_HI", and an
 * extra 32bit mask with just one bit used for the PF (selected by
 * NFD_FLR_PF_shf).  There is one byte of padding per PCIe island too.
 *
 * Users outside of this file would generally call APIs from this file, so
 * they should not depend on this structure.
 */
#define NFD_FLR_DECLARE                                                 \
    _NFP_CHIPRES_ASM(.alloc_mem nfd_flr_atomic NFD_CFG_RING_EMEM        \
                     global 64 64)                                      \
    _NFP_CHIPRES_ASM(.declare_resource nfd_flr_atomic_mem               \
                     global 64 nfd_flr_atomic)                          \
    _NFP_CHIPRES_ASM(.alloc_resource nfd_flr_seen nfd_flr_atomic_mem    \
                     global 64 64)

#define NFD_FLR_LINK_IND(_isl)                                  \
    ((__mem40 char *) _link_sym(nfd_flr_seen) + ((_isl) * 16))
#define NFD_FLR_LINK(_isl) NFD_FLR_LINK_IND(_isl)


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
#define NFD_CFG_BASE_DECLARE(_isl)                                      \
    _NFP_CHIPRES_ASM(.alloc_mem NFD_CFG_BASE(_isl) NFD_EMEM(_isl) global \
                     ((NFD_MAX_VFS + NFD_MAX_PFS + NFD_MAX_CTRL) * \
                      NFP_NET_CFG_BAR_SZ) SZ_2M)
#else
/* TODO This logic seems broken, remove it entirely */
#define NFD_CFG_BASE_DECLARE(_isl)                                      \
    _NFP_CHIPRES_ASM(.alloc_mem NFD_CFG_BASE(_isl) NFD_EMEM(_isl) global \
                     ((NFD_MAX_VFS + NFD_MAX_PFS) * NFP_NET_CFG_BAR_SZ) \
                     (NFD_MAX_VFS * NFP_NET_CFG_BAR_SZ * (1 +  NFD_MAX_PFS)))
#endif


#define NFD_CFG_BASE_LINK_IND1(_sym)            \
    ((__emem char *) _link_sym(_sym))
#define NFD_CFG_BASE_LINK_IND0(_isl)            \
    NFD_CFG_BASE_LINK_IND1(NFD_CFG_BASE(_isl))
#define NFD_CFG_BASE_LINK(_isl) NFD_CFG_BASE_LINK_IND0(_isl)


/* XXX we can't use "|" here due to NFCC error, check and possibly JIRA. */
#define NFD_CFG_BAR(_base, _vid)                \
    ((_base) + ((_vid) * NFP_NET_CFG_BAR_SZ))

#define NFD_CFG_BAR_ISL(_isl, _vid )            \
    NFD_CFG_BAR(NFD_CFG_BASE_LINK(_isl), (_vid))



/**
 * @param isl           PCIe island, in the range 0..3
 * @param vid           vNIC ID
 *
 * This function is not intended for use on the data plane as it is
 * expensive to extract the BAR pointer.  Supplying an isl not in the
 * range 0..3 or for a PCIe island not in use with NFD is illegal.
 * It is caught with a halt().
 */
__intrinsic __emem char *nfd_cfg_bar_base(unsigned int isl, unsigned int vid) {
    __emem char *bar_base;
    switch (isl) {
    case 0:
        #ifdef NFD_PCIE0_EMEM
            bar_base = NFD_CFG_BAR_ISL(0, vid);
        #else
            goto err;
        #endif
        break;
    case 1:
        #ifdef NFD_PCIE1_EMEM
            bar_base = NFD_CFG_BAR_ISL(1, vid);
        #else
            goto err;
        #endif
        break;
    case 2:
        #ifdef NFD_PCIE2_EMEM
            bar_base = NFD_CFG_BAR_ISL(2, vid);
        #else
            goto err;
        #endif
        break;
    case 3:
        #ifdef NFD_PCIE3_EMEM
            bar_base = NFD_CFG_BAR_ISL(3, vid);
        #else
            goto err;
        #endif
        break;
    default:
        goto err;
        break;
    };

    return bar_base;

err:
    /* XXX we are halting on this error because it indicates a serious error
     * the return won't actually execute, and 0 is as good as any other
     * value.  0x809 == NFD_CFG_BAR_BASE_ISL_INVALID */
    local_csr_write(local_csr_mailbox_0, 0x809);
    local_csr_write(local_csr_mailbox_1, isl);
    halt();
    return 0;
};


#if (defined(NFD_USE_TLV_VF) || defined(NFD_USE_TLV_CTRL) || \
     defined(NFD_USE_TLV_PF))

#ifndef NFD_USE_TLV
#define NFD_USE_TLV
#endif

#ifndef NFD_CFG_TLV_BLOCK_SZ
#error "NFD_CFG_TLV_BLOCK_SZ must be defined if NFD_USE_TLV is set"
#endif

#if ((NFD_CFG_TLV_BLOCK_SZ % 4) != 0)
#error "NFD_CFG_TLV_BLOCK_SZ must be a 4B multiple"
#endif

#ifndef NFD_CFG_TLV_BLOCK_OFF
#error "NFD_CFG_TLV_BLOCK_OFF must be defined if NFD_USE_TLV is set"
#endif

#if ((NFD_CFG_TLV_BLOCK_OFF % 4) != 0)
#error "NFD_CFG_TLV_BLOCK_OFF must be a 4B multiple"
#endif

#define NFD_TLV_BASE_IND(_x) nfd_cfg_tlv_template##_x
#define NFD_TLV_BASE(_x) NFD_TLV_BASE_IND(_x)

#define NFD_TLV_BASE_DECLARE(_isl) \
    _NFP_CHIPRES_ASM(.alloc_mem NFD_TLV_BASE(_isl) emem global	\
    ((NFD_MAX_VFS + NFD_MAX_CTRL + NFD_MAX_PFS) * NFD_CFG_TLV_BLOCK_SZ) 4)

#define NFD_TLV_BASE_LINK_IND1(_sym)            \
    ((__emem char *) _link_sym(_sym))
#define NFD_TLV_BASE_LINK_IND0(_isl)            \
    NFD_TLV_BASE_LINK_IND1(NFD_TLV_BASE(_isl))
#define NFD_TLV_BASE_LINK(_isl) NFD_TLV_BASE_LINK_IND0(_isl)

#define NFD_TLV_VID_BASE(_isl, _vid)                            \
    (NFD_TLV_BASE_LINK(_isl) + (_vid) * NFD_CFG_TLV_BLOCK_SZ)


/**
 * @param isl           PCIe island, in the range 0..3
 * @param vid           vNIC ID
 *
 * This function is not intended for use on the data plane as it is
 * expensive to extract the BAR pointer.  Supplying an isl not in the
 * range 0..3 or for a PCIe island not in use with NFD is illegal.
 * It is caught with a halt().
 */
__intrinsic __emem char *nfd_cfg_tlv_tml_base(unsigned int isl,
                                              unsigned int vid) {
    __emem char *tlv_base;
    switch (isl) {
    case 0:
        #ifdef NFD_PCIE0_EMEM
            tlv_base = NFD_TLV_VID_BASE(0, vid);
        #else
            goto err;
        #endif
        break;
    case 1:
        #ifdef NFD_PCIE1_EMEM
            tlv_base = NFD_TLV_VID_BASE(1, vid);
        #else
            goto err;
        #endif
        break;
    case 2:
        #ifdef NFD_PCIE2_EMEM
            tlv_base = NFD_TLV_VID_BASE(2, vid);
        #else
            goto err;
        #endif
        break;
    case 3:
        #ifdef NFD_PCIE3_EMEM
            tlv_base = NFD_TLV_VID_BASE(3, vid);
        #else
            goto err;
        #endif
        break;
    default:
        goto err;
        break;
    };

    return tlv_base;

err:
    /* XXX we are halting on this error because it indicates a serious error
     * the return won't actually execute, and 0 is as good as any other
     * value.  0x809 == NFD_CFG_BAR_BASE_ISL_INVALID */
    local_csr_write(local_csr_mailbox_0, 0x809);
    local_csr_write(local_csr_mailbox_1, isl);
    halt();
    return 0;
};
#endif /* NFD_USE_TLV */


/**
 * @param msg_valid     message contains valid information
 * @param error         an error has been detected
 * @param interested    this component must process this message
 * @param up_bit        the current queue is up
 * @param spare         spare bits
 * @param queue         queue number to process
 * @param vid           vNIC ID to process
 *
 * This structure is passed between NFD components and used internally
 * to carry configuration messages.  'vid', 'msg_valid', and 'error' are
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
            unsigned int vid:8;
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
#define nfd_cfg_master_chk_cfg_msg(_msg, _sig, _pci)                    \
    nfd_cfg_check_cfg_msg((_msg), (_sig), NFD_CFG_RING_NUM(_pci, 5))


/**
 * Notify the host that a cfg_msg has been processed
 * @param pcie_isl      PCIe island that the message relates to
 * @param cfg_msg       message listing the queue that has been configured
 * @param isl_base      start address of the CFG BAR for this PCIe island
 * @param cfg_sig       signal to trigger a check for new messages
 *
 * As this function completes, it sets "cfg_sig" to ensure that
 * "nfd_cfg_check_cfg_msg()" checks for any outstanding configuration
 * messages.
 */
__intrinsic void nfd_cfg_app_complete_cfg_msg(unsigned int pcie_isl,
                                              struct nfd_cfg_msg *cfg_msg,
                                              __dram void *isl_base,
                                              SIGNAL *cfg_sig);

/**
 * Pass this message to the next stage, and flag to check for new msg
 * @param cfg_msg           completed message
 * @param cfg_sig           signal for this ME to check for new message
 * @param cfg_sig_remote    signal to set for next recipient
 * @param next_me           ME to signal
 * @param rnum_out          output ring number
 */
__intrinsic void nfd_cfg_complete_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                                          SIGNAL *cfg_sig,
                                          __remote SIGNAL *cfg_sig_remote,
                                          unsigned int next_me,
                                          unsigned int rnum_out);


#endif /* __NFP_LANG_MICROC */

#endif /* !_BLOCKS__SHARED_NFD_CFG_H_ */
