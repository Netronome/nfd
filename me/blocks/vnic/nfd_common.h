/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/nfd_common.h
 * @brief         NFD defines and macros shared between firmware and host
 */
#ifndef __NFD_COMMON_H
#define __NFD_COMMON_H

#include <nfd_user_cfg.h>

#ifndef NFD_MAX_VNIC_QUEUES
#error "NFD_MAX_VNIC_QUEUES is not defined but is required"
#endif

#define NFD_IN_DESC_SIZE        16

#define NFD_OUT_DESC_SIZE       16

#define NFD_NATQ2BMQ(_qid) \
    ((((_qid) << 1) | (((_qid) >> 5) & 1)) & 63)

#define NFD_BMQ2NATQ(_qid) \
    (((_qid) >> 1) | (((_qid) << 5) & 32))

#define NFD_QID2VNIC(_qid) \
    (NFD_BMQ2NATQ(_qid) / NFD_MAX_VNIC_QUEUES)

#define NFD_QID2VQN(_qid) \
    (NFD_BMQ2NATQ(_qid) % NFD_MAX_VNIC_QUEUES)

#define NFD_BUILD_QID(_vnic, _vqn) \
    NFD_NATQ2BMQ((_vnic) * NFD_MAX_VNIC_QUEUES + (_vqn))

#endif /* __NFD_COMMON_H */
