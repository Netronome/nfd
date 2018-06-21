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
 * @file          blocks/vnic/nfd_common.h
 * @brief         NFD defines and macros shared between firmware and host
 */
#ifndef __NFD_COMMON_H
#define __NFD_COMMON_H

#include <nfd_user_cfg.h>
#include <vnic/shared/nfd_ctrl.h>


/* NFD version number define guards */
#ifdef NFD_CFG_MAJOR_PF
#error "NFD application code may not set NFD_CFG_MAJOR_PF version number"
#endif

#ifdef NFD_CFG_MINOR_PF
#error "NFD application code may not set NFD_CFG_MINOR_PF version number"
#endif

#ifdef NFD_CFG_MAJOR_CTRL
#error "NFD application code may not set NFD_CFG_MAJOR_CTRL version number"
#endif

#ifdef NFD_CFG_MINOR_CTRL
#error "NFD application code may not set NFD_CFG_MINOR_CTRL version number"
#endif

/* NFD version numbers.  Increment major version number for each incompatible
 * ABI change.  Increment minor version number for each compatible ABI change
 * (e.g. a new feature flag).  Reset minor version number to zero for each
 * major version number change. */
#if (defined(NFD_NET_APP_ID_FORCE_CHK) || defined(NFD_USE_CTRL))
/* Firmwares using CTRL vNICs need an app_id and must use ABI 5
 * Firmwares that advertise an app_id but don't use CTRL vNICs
 * may choose whether to advertise ABI 5 */
    #define NFD_CFG_MAJOR_PF            5
    #define NFD_CFG_MAJOR_CTRL          5
    #define NFD_CFG_MAJOR_VF_MAX        5
#else
    #define NFD_CFG_MAJOR_PF            4
    #define NFD_CFG_MAJOR_CTRL          4
    #define NFD_CFG_MAJOR_VF_MAX        4
#endif
#define NFD_CFG_MAJOR_VF_MIN        3

#define NFD_CFG_MINOR_PF            5
#define NFD_CFG_MINOR_CTRL          5
#define NFD_CFG_MINOR_VF_MAX        5
#define NFD_CFG_MINOR_VF_MIN        0

#define NFD_CFG_CLASS_DEFAULT       0

/* Version number helper defines */
#define NFD_CFG_CLASS_VER_msk       0xff
#define NFD_CFG_CLASS_VER_shf       24
#define NFD_CFG_CLASS_VER(x)        (((x) & 0xff) << 24)
#define NFD_CFG_CLASS_VER_of(x)     (((x) >> 24) & 0xff)
#define NFD_CFG_CLASS_TYPE_msk      0xff
#define NFD_CFG_CLASS_TYPE_shf      16
#define NFD_CFG_CLASS_TYPE(x)       (((x) & 0xff) << 16)
#define NFD_CFG_CLASS_TYPE_of(x)    (((x) >> 16) & 0xff)
#define NFD_CFG_MAJOR_VERSION_msk   0xff
#define NFD_CFG_MAJOR_VERSION_shf   8
#define NFD_CFG_MAJOR_VERSION(x)    (((x) & 0xff) << 8)
#define NFD_CFG_MAJOR_VERSION_of(x) (((x) >> 8) & 0xff)
#define NFD_CFG_MINOR_VERSION_msk   0xff
#define NFD_CFG_MINOR_VERSION_shf   0
#define NFD_CFG_MINOR_VERSION(x)    (((x) & 0xff) << 0)
#define NFD_CFG_MINOR_VERSION_of(x) (((x) >> 0) & 0xff)


/* User define consistency checks */
#ifndef NFD_MAX_VF_QUEUES
#error "NFD_MAX_VF_QUEUES is not defined but is required"
#endif

#ifndef NFD_MAX_PF_QUEUES
#error "NFD_MAX_PF_QUEUES is not defined but is required"
#endif

#ifndef NFD_MAX_VFS
#error "NFD_MAX_VFS is not defined but is required"
#endif

#ifndef NFD_MAX_PFS
#error "NFD_MAX_PFS is not defined but is required"
#endif

#ifdef NFD_CFG_MAJOR_VF
#if (NFD_CFG_MAJOR_VF > NFD_CFG_MAJOR_VF_MAX)
#error "NFD_CFG_MAJOR_VF out of supported range"
#endif

#if (NFD_CFG_MAJOR_VF < NFD_CFG_MAJOR_VF_MIN)
#error "NFD_CFG_MAJOR_VF out of supported range"
#endif
#else
#define NFD_CFG_MAJOR_VF NFD_CFG_MAJOR_VF_MAX
#endif

#ifdef NFD_CFG_MINOR_VF
#if (NFD_CFG_MINOR_VF > NFD_CFG_MINOR_VF_MAX)
#error "NFD_CFG_MINOR_VF out of supported range"
#endif

#if (NFD_CFG_MINOR_VF < NFD_CFG_MINOR_VF_MIN)
#error "NFD_CFG_MINOR_VF out of supported range"
#endif
#else
#define NFD_CFG_MINOR_VF NFD_CFG_MINOR_VF_MAX
#endif


/* vNICs must have queues, so if the "MAX_QUEUES" parameter is zero,
 * require the related MAX_VFS/PFS parameter to be zero as well. */
#if ((NFD_MAX_VF_QUEUES == 0) && (NFD_MAX_VFS != 0))
#error "NFD_MAX_VFS must be zero if NFD_MAX_VF_QUEUES equals zero"
#endif

#if (NFD_MAX_PF_QUEUES == 0) && (NFD_MAX_PFS != 0)
#error "NFD_MAX_PFS must be zero if NFD_MAX_PF_QUEUES equals zero"
#endif

/* The CTRL vNIC can only be used if PF vNICs are also in use */
#if ((NFD_MAX_PFS == 0) && defined(NFD_USE_CTRL))
#error "PF vNICs are required if the CTRL vNIC is used (NFD_USE_CTRL is set)"
#endif

/* The CTRL vNIC can only be used for FWs advertising an app_id */
#if (defined(NFD_USE_CTRL) && !defined(NFD_NET_APP_ID))
#error "An APP ID must be advertised if the CTRL vNIC is used"
#endif

#ifdef NFD_CFG_CLASS
#ifndef NFD_CFG_CLASS_VERSION
#error "NFD_CFG_CLASS_VERSION must be provided if NFD_CFG_CLASS is specified"
#endif
#endif

/* Check if BPF config is complete if enabled */
#ifdef NFD_BPF_CAPABLE
#if !defined(NFD_BPF_START_OFF) || !defined(NFD_BPF_DONE_OFF)
#error "NFD_BPF_CAPABLE defined but NFD_BPF_START_OFF and NFD_BPF_DONE_OFF not set"
#endif
#endif


#define NFD_VNIC_TYPE_VF    0
#define NFD_VNIC_TYPE_PF    1
#define NFD_VNIC_TYPE_CTRL  2

#define NFD_TOTAL_VFQS      (NFD_MAX_VFS * NFD_MAX_VF_QUEUES)
#define NFD_TOTAL_CTRLQS    (NFD_MAX_CTRL * NFD_MAX_CTRL_QUEUES)
#ifdef NFD_USE_OVERSUBSCRIPTION
    #define NFD_OVERSUBSCRIBED (NFD_TOTAL_VFQS + NFD_TOTAL_CTRLQS + \
                                NFD_MAX_PFS * NFD_MAX_PF_QUEUES - 64)
    #if (NFD_OVERSUBSCRIBED < 0)
        #undef NFD_OVERSUBSCRIBED
        #define NFD_OVERSUBSCRIBED 0
    #endif

    #if ((NFD_OVERSUBSCRIBED != 0) &&               \
         (NFD_OVERSUBSCRIBED >= NFD_MAX_PF_QUEUES))
        #error "Insufficient queues on PF for requested oversubscription"
    #endif

    #define NFD_LAST_PF_MAX_QUEUES (NFD_MAX_PF_QUEUES - NFD_OVERSUBSCRIBED)
    #define NFD_TOTAL_PFQS                                  \
        (NFD_MAX_PFS * NFD_MAX_PF_QUEUES - NFD_OVERSUBSCRIBED)

#else
    #define NFD_TOTAL_PFQS      (NFD_MAX_PFS * NFD_MAX_PF_QUEUES)
#endif

#if (NFD_TOTAL_VFQS + NFD_TOTAL_CTRLQS + NFD_TOTAL_PFQS > 64)
#error "Total number of NFD queues per island cannot exceed 64"
#endif


#ifdef NFD_USE_CTRL
/* XXX The control vNIC is placed after the VF vNICs and before the PF vNICs */
#define NFD_CTRL_VNIC       NFD_MAX_VFS
#define NFD_CTRL_QUEUE      NFD_TOTAL_VFQS
#endif

#define NFD_FIRST_PF        (NFD_MAX_VFS + NFD_MAX_CTRL)
#if (NFD_MAX_PFS > 0)
#define NFD_LAST_PF         (NFD_FIRST_PF + NFD_MAX_PFS -1)
#else
#define NFD_LAST_PF         NFD_FIRST_PF
#endif
#define NFD_FIRST_PF_QUEUE  (NFD_TOTAL_VFQS + NFD_TOTAL_CTRLQS)


/*
 * VFs are assigned VIDs and queues first, followed by the CTRL vNIC
 * if used and finally the PF vNICs.
 */
#if (NFD_MAX_VFS > 0)
    #if (NFD_MAX_CTRL + NFD_MAX_PFS)
        #define NFD_VID_IS_VF(_x) ((_x) < NFD_MAX_VFS)
        #define NFD_Q_IS_VF(_x) ((_x) < NFD_TOTAL_VFQS)
    #else
        #define NFD_VID_IS_VF(_x) 1
        #define NFD_Q_IS_VF(_x) 1
    #endif
#else
    #define NFD_VID_IS_VF(_x) 0
    #define NFD_Q_IS_VF(_x) 0
#endif

#if (NFD_MAX_PFS > 0)
    #if (NFD_MAX_VFS + NFD_MAX_CTRL)
        #define NFD_VID_IS_PF(_x) ((_x) >= NFD_FIRST_PF)
        #define NFD_Q_IS_PF(_x) ((_x) >= NFD_FIRST_PF_QUEUE)
    #else
        #define NFD_VID_IS_PF(_x) 1
        #define NFD_Q_IS_PF(_x) 1
    #endif
#else
    #define NFD_VID_IS_PF(_x) 0
    #define NFD_Q_IS_PF(_x) 0
#endif

#if (NFD_MAX_CTRL > 0)
    #if (NFD_MAX_CTRL_QUEUES != 1 || NFD_MAX_CTRL != 1)
        #error "Unsupported CTRL vNIC values"
    #endif

    #if (NFD_MAX_VFS + NFD_MAX_PFS)
        #define NFD_VID_IS_CTRL(_x) ((_x) == NFD_CTRL_VNIC)
        #define NFD_Q_IS_CTRL(_x) ((_x) == NFD_CTRL_QUEUE)
    #else
        #define NFD_VID_IS_CTRL(_x) 1
        #define NFD_Q_IS_CTRL(_x) 1
    #endif
#else
    #define NFD_VID_IS_CTRL(_x) 0
    #define NFD_Q_IS_CTRL(_x) 0
#endif


#define NFD_IN_DESC_SIZE        16

#define NFD_OUT_DESC_SIZE       16


/* NFD uses queue numbers in various formats, and provides
 * macros to convert from one format to another, depending
 * on what is available and what is needed.  The formats are
 * as follows:
 *
 * == Natural queue number (NATQ) ==
 * These are in the range [0 .. 63] and are the most basic format.
 *
 * == QC queue number ==
 * QC queues must be double spaced, so the QC queue number is the
 * natural queue number times 2.  There is also a constant offset
 * added depending on whether we need to access the PCI.OUT queues
 * or the PCI.IN queues ("NFD_OUT_Q_START" and "NFD_IN_Q_START"
 * respectively).  Only PCI.IN and PCI.OUT access the QC queues
 * from the firmware, so this representation is never used outside
 * the PCIe island.
 *
 * == Bitmask queue number (BMQ / QID) ==
 * NFD stores bitmasks of which queues need to be processed.  All data used
 * by PCI.IN and PCI.OUT is stored in the bitmask format.  A layer of
 * abstraction is used between the natural queue number and its internal
 * bitmask queue numbering, to allow NFD to convert from QC queue numbers
 * to an offset in a bitmask as efficiently as possible.  The bitmask queue
 * number is also referred to as the QID.
 *
 * == type:vNIC:queue pair numbering ==
 * Ultimately, NFD exposes N of three different types of vNICs with up to Q
 * queues to the user, where N and Q differ between types.
 * These vNIC queues need to be mapped into the NATQ space.  Currently,
 * the VF queues are enumerated first, from VF 0 to VF N-1, then the CTRL,
 * and finally the PFs.
 *
 * == VID:queue pair numbering ==
 * All vNICs are given a unique ID independent of their type, the VID.  This
 * VID is used for example to determine the vNIC's CFG BAR within the memory
 * symbol.
 */


/*
 * Conversions between NATQ and BMQ representations
 */
#define NFD_NATQ2BMQ(_qid) (_qid)

#define NFD_BMQ2NATQ(_qid) (_qid)


/*
 * Conversions between type:vNIC:queue to NATQ
 */

#if (NFD_MAX_VF_QUEUES != 0)
#define NFD_NATQ2VF(_nat)                       \
    ((_nat) / NFD_MAX_VF_QUEUES)
#define NFD_NATQ2VFQ(_nat)                      \
    ((_nat) % NFD_MAX_VF_QUEUES)
#define NFD_VF2NATQ(_vf, _q)                    \
    ((_vf) * NFD_MAX_VF_QUEUES + (_q))
#else
#define NFD_NATQ2VF(_nat) 0
#define NFD_NATQ2VFQ(_nat) 0
#define NFD_VF2NATQ(_vf, _q) 0
#endif

#if (NFD_MAX_PF_QUEUES != 0)
#define NFD_NATQ2PF(_nat)                               \
    (((_nat) - NFD_FIRST_PF_QUEUE) / NFD_MAX_PF_QUEUES)
#define NFD_NATQ2PFQ(_nat)                              \
    (((_nat) - NFD_FIRST_PF_QUEUE) % NFD_MAX_PF_QUEUES)
#define NFD_PF2NATQ(_pf, _q)                            \
    ((_pf) * NFD_MAX_PF_QUEUES + (_q) + NFD_FIRST_PF_QUEUE)
#else
#define NFD_NATQ2PF(_nat) 0
#define NFD_NATQ2PFQ(_nat) 0
#define NFD_PF2NATQ(_pf, _q) 0
#endif

#ifdef NFD_USE_CTRL
    #if (NFD_MAX_CTRL_QUEUES != 1 || NFD_MAX_CTRL != 1)
        #error "Unsupported CTRL vNIC values"
    #endif

    #define NFD_NATQ2CTRL(_nat) 0
    #define NFD_NATQ2CTRLQ(_nat) 0
    #define NFD_CTRL2NATQ(_ctrl, _q) NFD_CTRL_QUEUE
#else
    #define NFD_NATQ2CTRL(_nat) 0
    #define NFD_NATQ2CTRLQ(_nat) 0
    #define NFD_CTRL2NATQ(_ctrl, _q) 0
#endif


#ifndef __NFP_LANG_ASM

/* Provide NFD_EXTRACT_NATQ and NFD_NATQ2VNIC that select from
 * the above PF and VF macros as appropriate, if the configuration
 * allows compile time selection. */
#define NFD_EXTRACT_NATQ(_type, _vnic, _vqn, _nat)           \
do {                                                         \
    if (NFD_Q_IS_PF(_nat)) {                                 \
        (_type) = NFD_VNIC_TYPE_PF;                          \
        (_vnic) = NFD_NATQ2PF(_nat);                         \
        (_vqn) = NFD_NATQ2PFQ(_nat);                         \
    } else if (NFD_Q_IS_VF(_nat)) {                          \
        (_type) = NFD_VNIC_TYPE_VF;                          \
        (_vnic) = NFD_NATQ2VF(_nat);                         \
        (_vqn) = NFD_NATQ2VFQ(_nat);                         \
    } else {                                                 \
        (_type) = NFD_VNIC_TYPE_CTRL;                        \
        (_vnic) = NFD_NATQ2CTRL(_nat);                       \
        (_vqn) = NFD_NATQ2CTRLQ(_nat);                       \
    }                                                        \
} while(0)

#endif /* __NFP_LANG_ASM */


#define NFD_BUILD_NATQ(_type, _vnic, _vqn)                      \
    (((_type) == NFD_VNIC_TYPE_PF) ? NFD_PF2NATQ(_vnic, _vqn) : \
     ((_type) == NFD_VNIC_TYPE_VF) ? NFD_VF2NATQ(_vnic, _vqn) : \
     NFD_CTRL2NATQ(_vnic, _vqn))


/*
 * Conversions between type:vNIC and vid
 */

#define NFD_VID2PF(_vid) ((_vid) - NFD_FIRST_PF)
#define NFD_PF2VID(_pf) ((_pf) + NFD_FIRST_PF)

#define NFD_VID2VF(_vid) (_vid)
#define NFD_VF2VID(_vf) (_vf)

#ifdef NFD_USE_CTRL
#define NFD_VID2CTRL(_vid) 0
#define NFD_CTRL2VID(_ctrl) NFD_CTRL_VNIC
#else
#define NFD_VID2CTRL(_vid) 0
#define NFD_CTRL2VID(_ctrl) 0
#endif


#ifndef __NFP_LANG_ASM

#define NFD_VID2VNIC(_type, _vnic, _vid)            \
do {                                                \
    if (NFD_VID_IS_PF(_vid)) {                      \
        (_type) = NFD_VNIC_TYPE_PF;                 \
        (_vnic) = NFD_VID2PF(_vid);                 \
    } else if (NFD_VID_IS_VF(_vid)) {               \
        (_type) = NFD_VNIC_TYPE_VF;                 \
        (_vnic) = NFD_VID2VF(_vid);                 \
    } else {                                        \
        (_type) = NFD_VNIC_TYPE_CTRL;               \
        (_vnic) = NFD_VID2CTRL(_vid);               \
    }                                               \
} while (0)

#endif /* __NFP_LANG_ASM */

#define NFD_VNIC2VID(_type, _vnic)                                  \
    (((_type) == NFD_VNIC_TYPE_PF) ? NFD_PF2VID(_vnic) :            \
     ((_type) == NFD_VNIC_TYPE_VF) ? NFD_VF2VID(_vnic) :            \
     NFD_CTRL2VID(_vnic))


/*
 * Conversions between vid:queue to NATQ
 */

/* Config queues are a special case where the VID
 * can be computed easily. The queue is known by definition. */
#define NFD_CFGQ2VID(_cfg)                                              \
    (NFD_Q_IS_PF(_cfg) ? NFD_PF2VID(NFD_NATQ2PF(_cfg)) :                \
     (NFD_Q_IS_VF(_cfg)) ? NFD_VF2VID(NFD_NATQ2VF(_cfg)) :              \
     NFD_CTRL2VID(NFD_NATQ2CTRL(_cfg)))


#ifndef __NFP_LANG_ASM

#define NFD_NATQ2VID(_vid, _vqn, _nat)                   \
do {                                                     \
    if (NFD_Q_IS_PF(_nat)) {                             \
        (_vid) = NFD_PF2VID(NFD_NATQ2PF(_nat));          \
        (_vqn) = NFD_NATQ2PFQ(_nat);                     \
    } else if (NFD_Q_IS_VF(_nat)) {                      \
        (_vid) = NFD_VF2VID(NFD_NATQ2VF(_nat));          \
        (_vqn) = NFD_NATQ2VFQ(_nat);                     \
    } else {                                             \
        (_vid) = NFD_CTRL2VID(NFD_NATQ2CTRL(_nat));      \
        (_vqn) = NFD_NATQ2CTRLQ(_nat);                   \
    }                                                    \
} while(0)

#endif /* __NFP_LANG_ASM */

#define NFD_VID2NATQ(_vid, _vqn)                                        \
   (NFD_VID_IS_PF(_vid) ? NFD_PF2NATQ(NFD_VID2PF(_vid), _vqn) :         \
    NFD_VID_IS_VF(_vid) ? NFD_VF2NATQ(NFD_VID2VF(_vid), _vqn) :         \
    NFD_CTRL2NATQ(NFD_VID2CTRL(_vid), _vqn))


/*
 * Convert between QID and type:vNIC:queue represenations,
 * using the natural queue number as an intermediate stage.
 */
#ifndef __NFP_LANG_ASM
#define NFD_EXTRACT_QID(_type, _vnic, _vqn, _qid)        \
    NFD_EXTRACT_NATQ(_type, _vnic, _vqn, NFD_BMQ2NATQ(_qid))
#endif /* __NFP_LANG_ASM */

#define NFD_BUILD_QID(_type, _vnic, _vqn)        \
    NFD_NATQ2BMQ(NFD_BUILD_NATQ(_type, _vnic, _vqn))


/*
 * Convert between QID and vid:queue represenations,
 * using the natural queue number as an intermediate stage.
 */

#ifndef __NFP_LANG_ASM
#define NFD_QID2VID(_vid, _vqn, _qid)               \
    NFD_NATQ2VID(_vid, _vqn, NFD_BMQ2NATQ(_qid))
#endif /* __NFP_LANG_ASM */

#define NFD_VID2QID(_vid, _vqn)                 \
    NFD_NATQ2BMQ(NFD_VID2NATQ(_vid, _vqn))


/*
 * Convert between the queue controller queues and natural queues.
 * Each natural queue has a block of 4 QC queues allocated to it.  Different
 * types of queues are at different offsets within the block.  Converting
 * to a QC queue requires the queue offset of the queue type.  Converting
 * to a natural queue does not, as a >> operator eliminates the low bits.
 * The defines to specify the types are as follows: NFD_OUT_FL_QUEUE,
 * NFD_IN_TX_QUEUE, and NFD_CFG_QUEUE.
 */
#define NFD_NATQ2QC(_nat_q, _qc_type)           \
    (((_nat_q) << 2) | (_qc_type))

#define NFD_QC2NATQ(_qc_q)                      \
    ((_qc_q) >> 2)


/*
 * Find the number of queues supported by a vNIC
 */
#ifdef NFD_USE_OVERSUBSCRIPTION
#define NFD_VNIC_MAXQS(_type, _vnic)                            \
    (((_type) == NFD_VNIC_TYPE_VF) ? NFD_MAX_VF_QUEUES :        \
     ((_type) == NFD_VNIC_TYPE_CTRL) ? NFD_MAX_CTRL_QUEUES :    \
     ((_vnic) != (NFD_MAX_PFS -1)) ? NFD_MAX_PF_QUEUES :        \
     NFD_LAST_PF_MAX_QUEUES)

#define NFD_VID_MAXQS(_vid)                         \
    (NFD_VID_IS_VF(_vid) ? NFD_MAX_VF_QUEUES :      \
     NFD_VID_IS_CTRL(_vid) ? NFD_MAX_CTRL_QUEUES :  \
     ((_vid) != NFD_LAST_PF) ? NFD_MAX_PF_QUEUES :  \
     NFD_LAST_PF_MAX_QUEUES)

#else
#define NFD_VNIC_MAXQS(_type, _vnic)                        \
    (((_type) == NFD_VNIC_TYPE_PF) ? NFD_MAX_PF_QUEUES :    \
     ((_type) == NFD_VNIC_TYPE_VF) ? NFD_MAX_VF_QUEUES :    \
     NFD_MAX_CTRL_QUEUES)

#define NFD_VID_MAXQS(_vid)                         \
    (NFD_VID_IS_PF(_vid) ? NFD_MAX_PF_QUEUES :      \
     NFD_VID_IS_VF(_vid) ? NFD_MAX_VF_QUEUES :      \
     NFD_MAX_CTRL_QUEUES)

#endif

#endif /* __NFD_COMMON_H */
