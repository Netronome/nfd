/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/nfd_common.h
 * @brief         NFD defines and macros shared between firmware and host
 */
#ifndef __NFD_COMMON_H
#define __NFD_COMMON_H

#include <nfd_user_cfg.h>

#ifndef NFD_MAX_VF_QUEUES
#error "NFD_MAX_VF_QUEUES is not defined but is required"
#endif

#ifndef NFD_MAX_PF_QUEUES
#error "NFD_MAX_PF_QUEUES is not defined but is required"
#endif

#ifndef NFD_MAX_VFS
#error "NFD_MAX_VFS is not defined but is required"
#endif


#define NFD_IN_DESC_SIZE        16

#define NFD_OUT_DESC_SIZE       16

#define NFD_NATQ2BMQ(_qid) \
    ((((_qid) << 1) | (((_qid) >> 5) & 1)) & 63)

#define NFD_BMQ2NATQ(_qid) \
    (((_qid) >> 1) | (((_qid) << 5) & 32))


/* Config queues are a special case where the vNIC
 * can be computed easily. The queue is known by definition. */
#define NFD_CFGQ2VNIC(_cfg)                     \
    ((_cfg) / NFD_MAX_VF_QUEUES)


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


/* If we know the vNIC, we can compute the queue cheaply */
#define NFD_NATQ2VQN(_nat, _vnic)               \
    ((_nat) - (_vnic * NFD_MAX_VF_QUEUES))


/* With no special knowledge about the natural queue,
 * we need to test whether it is a PF queue or a VF queue,
 * and handle each case differently. */
#define NFD_EXTRACT_NATQ(_vnic, _vqn, _nat)             \
do {                                                    \
    if ((_nat) < (NFD_MAX_VF_QUEUES * NFD_MAX_VFS)) {   \
        (_vnic) = NFD_NATQ2VF(_nat);                    \
        (_vqn) = NFD_NATQ2VFQ(_nat);                    \
    } else {                                            \
        (_vnic) = NFD_NATQ2PF(_nat);                    \
        (_vqn) = NFD_NATQ2PFQ(_nat);                    \
    }                                                   \
} while(0)

#define NFD_EXTRACT_QID(_vnic, _vqn, _qid)              \
    NFD_EXTRACT_NATQ(_vnic, _vqn, NFD_BMQ2NATQ(_qid))


/* Building queue numbers does not have corner cases */
#define NFD_BUILD_NATQ(_vnic, _vqn) \
    ((_vnic) * NFD_MAX_VF_QUEUES + (_vqn))

#define NFD_BUILD_QID(_vnic, _vqn) \
    NFD_NATQ2BMQ(NFD_BUILD_NATQ(_vnic, _vqn))

#endif /* __NFD_COMMON_H */
