/*
 * Copyright (C) 2014 Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_cfg.c
 * @brief         An API to manage access to NFD configuration data
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/mem_bulk.h>
#include <nfp/mem_ring.h>

#include <vnic/shared/nfd.h>
#include <vnic/shared/nfd_cfg.h>

#include <ns_vnic_ctrl.h>


__intrinsic void
nfd_cfg_init_cfg_msg(SIGNAL *cfg_sig, struct nfd_cfg_msg *cfg_msg)
{
    __implicit_write(cfg_sig);

    cfg_msg->__raw = 0;
}


__intrinsic void
nfd_cfg_check_cfg_msg(struct nfd_cfg_msg *cfg_msg, SIGNAL *cfg_sig,
                      unsigned int rnum)
{
    mem_ring_addr_t ring_addr;

    ring_addr = (unsigned long long) NFD_CFG_EMEM >> 8;

    /* XXX should this method read the vnic config BAR? */
    if (signal_test(cfg_sig)) {
        int ret;
        __xread struct nfd_cfg_msg cfg_msg_rd;

        __implicit_write(cfg_sig);

        ret = mem_ring_get(rnum, ring_addr, &cfg_msg_rd, sizeof cfg_msg_rd);

        if (ret == 0) {
            *cfg_msg = cfg_msg_rd;
        }
    }
}


__intrinsic void
nfd_cfg_app_complete_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                             __dram void *isl_base)
{
    __xwrite unsigned int result;
    __dram char *addr = (__dram char *) isl_base;

    /* Compute the address of the update field */
    addr += cfg_msg->vnic * NS_VNIC_CFG_BAR_SZ;
    addr += NS_VNIC_CFG_UPDATE;

    if (cfg_msg->error) {
        result = NS_VNIC_CFG_UPDATE_ERR;
    } else {
        /* XXX add NS_VNIC_CFG_UPDATE_SUCCESS value to ns_vnic_ctrl.h */
        result = 0;
    }

    mem_write32(&result, addr, sizeof(result));
}
