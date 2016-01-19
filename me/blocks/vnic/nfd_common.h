/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/nfd_common.h
 * @brief         NFD defines and macros shared between firmware and host
 */
#ifndef __NFD_COMMON_H
#define __NFD_COMMON_H

#include <nfd_user_cfg.h>


/* NFD version number define guards */
#ifdef NFD_CFG_MAJOR
#error "NFD application code may not set NFD_CFG_MAJOR version number"
#endif

#ifdef NFD_CFG_MINOR
#error "NFD application code may not set NFD_CFG_MINOR version number"
#endif

/* NFD version numbers.  Increment major version number for each incompatible
 * ABI change.  Increment minor version number for each compatible ABI change
 * (e.g. a new feature flag).  Reset minor version number to zero for each
 * major version number change. */
#define NFD_CFG_MAJOR               3
#define NFD_CFG_MINOR               0
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


/* NFD_MAX_VFS is used to determine PF vNIC number, so must
 * always be consistent with VFs in use.  The ambiguous case
 * where N VFs with 0 queues are requested is illegal. */
#if ((NFD_MAX_VF_QUEUES == 0) && (NFD_MAX_VFS != 0))
#error "NFD_MAX_VFS must be zero if NFD_MAX_VF_QUEUES equals zero"
#endif


#ifdef NFD_CFG_CLASS
#ifndef NFD_CFG_CLASS_VERSION
#error "NFD_CFG_CLASS_VERSION must be provided if NFD_CFG_CLASS is specified"
#endif
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
 * == Bitmask queue number (BMQ) ==
 * NFD stores bitmasks of which queues need to be processed.  All data used
 * by PCI.IN and PCI.OUT is stored in the bitmask format.  A layer of
 * abstraction is used between the natural queue number and its internal
 * bitmask queue numbering, to allow NFD to convert from QC queue numbers
 * to an offset in a bitmask as efficiently as possible.
 *
 * == vNIC:queue pair numbering ==
 * Ultimately, NFD exposes up to N vNICs with up to Q queues to the user.
 * These vNIC queues need to be mapped into the NATQ space.  Currently,
 * the VF queues are enumerated first, from VF 0 to VF N-1, and the PF
 * afterwards.  Thus NATQ = vNIC * NFD_MAX_VF_QUEUES + queue.
 * We need to handle the case where there are no VFs or where there is no
 * PF as well.
 */


/*
 * Conversions between NATQ and BMQ representations
 */
#define NFD_NATQ2BMQ(_qid) _qid

#define NFD_BMQ2NATQ(_qid) _qid


/*
 * Conversions between vNIC:queue to NATQ
 */

/* Config queues are a special case where the vNIC
 * can be computed easily. The queue is known by definition. */
#if NFD_MAX_VF_QUEUES != 0
#define NFD_CFGQ2VNIC(_cfg)                     \
    ((_cfg) / NFD_MAX_VF_QUEUES)
#else
/* We must have the PF */
#define NFD_CFGQ2VNIC(_cfg) NFD_MAX_VFS
#endif

/* If a natural queue is known to be related to a VF,
 * the mapping can be computed cheaply. */
#define NFD_NATQ2VF(_nat)                       \
    ((_nat) / NFD_MAX_VF_QUEUES)

#define NFD_NATQ2VFQ(_nat)                      \
    ((_nat) % NFD_MAX_VF_QUEUES)


/* If a natural queue is known to be related to a PF,
 * the mapping can be computed cheaply. */
#define NFD_NATQ2PF(_nat)                       \
    (NFD_MAX_VFS)

#define NFD_NATQ2PFQ(_nat)                          \
    ((_nat) - (NFD_MAX_VF_QUEUES * NFD_MAX_VFS))


/* If we know the vNIC, we can compute the queue cheaply. */
#define NFD_NATQ2VQN(_nat, _vnic)               \
    ((_nat) - (_vnic * NFD_MAX_VF_QUEUES))


#ifndef __NFP_LANG_ASM

/* Provide NFD_EXTRACT_NATQ and NFD_NATQ2VNIC that select from
 * the above PF and VF macros as appropriate, if the configuration
 * allows compile time selection. */
#if (((NFD_MAX_VFS != 0) && (NFD_MAX_VF_QUEUES != 0)) &&     \
     (NFD_MAX_PF_QUEUES != 0))
    /* With no special knowledge about the natural queue, */
    /* we need to test whether it is a PF queue or a VF */
    /* queue, and handle each case differently. */
#define NFD_EXTRACT_NATQ(_vnic, _vqn, _nat)                  \
do {                                                         \
    if ((_nat) < (NFD_MAX_VF_QUEUES * NFD_MAX_VFS)) {        \
         (_vnic) = NFD_NATQ2VF(_nat);                        \
         (_vqn) = NFD_NATQ2VFQ(_nat);                        \
    } else {                                                 \
         (_vnic) = NFD_NATQ2PF(_nat);                        \
         (_vqn) = NFD_NATQ2PFQ(_nat);                        \
    }                                                        \
} while(0)

#define NFD_NATQ2VNIC(_vnic, _nat)                \
do {                                              \
    if (_nat < (NFD_MAX_VF_QUEUES * NFD_MAX_VFS)) \
        _vnic = NFD_NATQ2VF(_nat);                \
    else                                          \
        _vnic = NFD_NATQ2PF(_nat);                \
} while(0)

#elif (NFD_MAX_PF_QUEUES != 0)
    /* We have PF queues only */
#define NFD_EXTRACT_NATQ(_vnic, _vqn, _nat)       \
do {                                              \
    (_vnic) = NFD_NATQ2PF(_nat);                  \
    (_vqn) = NFD_NATQ2PFQ(_nat);                  \
} while(0)

#define NFD_NATQ2VNIC(_vnic, _nat)                \
do {                                              \
        _vnic = NFD_NATQ2PF(_nat);                \
} while(0)

#elif ((NFD_MAX_VFS != 0) && (NFD_MAX_VF_QUEUES != 0))
    /* We have VF queues only */
#define NFD_EXTRACT_NATQ(_vnic, _vqn, _nat)       \
do {                                              \
    (_vnic) = NFD_NATQ2VF(_nat);                  \
    (_vqn) = NFD_NATQ2VFQ(_nat);                  \
} while(0)                                        \

#define NFD_NATQ2VNIC(_vnic, _nat)                \
do {                                              \
        _vnic = NFD_NATQ2VF(_nat);                \
} while(0)

#else
#error "PF and VF options imply that no queues are in use!"
#endif /* NFD_EXTRACT_NATQ defines */

#endif /* __NFP_LANG_ASM */


/*
 * Convert between NATQ and vNIC:queue represenations,
 * using the natural queue number as an intermediate stage.
 */

#define NFD_EXTRACT_QID(_vnic, _vqn, _qid)              \
    NFD_EXTRACT_NATQ(_vnic, _vqn, NFD_BMQ2NATQ(_qid))


/* Building queue numbers does not have corner cases */
#define NFD_BUILD_NATQ(_vnic, _vqn) \
    ((_vnic) * NFD_MAX_VF_QUEUES + (_vqn))

#define NFD_BUILD_QID(_vnic, _vqn) \
    NFD_NATQ2BMQ(NFD_BUILD_NATQ(_vnic, _vqn))


/*
 * Convert between the queue controller queues and natural queues.
 * Each natural queue has a block of 4 QC queues allocated to it.  Different
 * types of queues are at different offsets within the block.  Converting
 * to a QC queue requires the queue offset of the queue type.  Converting
 * to a natural queue does not, as a >> operator eliminates the low bits.
 * The defines to specify the types are as follows: NFD_OUT_FL_QUEUE,
 * NFD_IN_TX_QUEUE, and NFD_CFG_QUEUE.
 */
#define NFD_NATQ2QC(_nat_q, _type)             \
    ((_nat_q << 2) | _type)

#define NFD_QC2NATQ(_qc_q)                     \
    (_qc_q >> 2)


#endif /* __NFD_COMMON_H */
