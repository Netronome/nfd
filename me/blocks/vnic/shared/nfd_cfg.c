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
#include <nfp/xpb.h>

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
__send_interthread_sig(unsigned int dst_me, unsigned int ctx, unsigned int sig_no)
{
    unsigned int addr;

    /* Generic address computation.
     * Could be expensive if dst_me, or dst_xfer
     * not compile time constants */
    addr = ((dst_me & 0x3F0)<<20 | (dst_me & 15)<<9 | (ctx & 7) << 6 |
            (sig_no & 15)<<2);

    __asm ct[interthread_signal, --, addr, 0, --];
}

__intrinsic void
nfd_cfg_check_cfg_msg(struct nfd_cfg_msg *cfg_msg, SIGNAL *cfg_sig,
                      unsigned int rnum)
{
    /* XXX should this method read the vnic config BAR? */
    if (signal_test(cfg_sig)) {
        int ret;
        __xread struct nfd_cfg_msg cfg_msg_rd;
        mem_ring_addr_t ring_addr;

        __implicit_write(cfg_sig);

        ring_addr = (unsigned long long) NFD_CFG_EMEM >> 8;
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

__intrinsic void
nfd_cfg_svc_complete_cfg_msg(struct nfd_cfg_msg *cfg_msg,
                          __remote SIGNAL *cfg_sig_remote,
                          unsigned int next_me, unsigned int rnum_out,
                          unsigned int rnum_in)
{
    struct nfd_cfg_msg cfg_msg_tmp;
    __xrw struct nfd_cfg_msg cfg_msg_wr;
    __xread struct nfd_cfg_msg cfg_msg_rd;
    mem_ring_addr_t ring_addr = (unsigned long long) NFD_CFG_EMEM >> 8;
    SIGNAL_PAIR put_sig;
    SIGNAL_PAIR get_sig;

    /* Clear the internal state fields and set msg_valid before sending  */
    cfg_msg_tmp.__raw = 0;
    cfg_msg_tmp.msg_valid = 1;
    cfg_msg_tmp.error = cfg_msg->error;
    cfg_msg_tmp.vnic = cfg_msg->vnic;
    cfg_msg_wr.__raw = cfg_msg_tmp.__raw;

    /* Put is guaranteed to succeed by design (the ring larger than
     * the number of possible vNICs). */
    __mem_ring_put(rnum_out, ring_addr, &cfg_msg_wr, sizeof cfg_msg_wr,
                   sizeof cfg_msg_wr, sig_done, &put_sig);
    __mem_ring_get(rnum_in, ring_addr, &cfg_msg_rd, sizeof cfg_msg_rd,
                   sizeof cfg_msg_rd, sig_done, &get_sig);
    wait_for_all_single(&put_sig.even, &put_sig.odd, &get_sig.even);

    /* XXX don't check put return, assume put succeeded. */

    if (!signal_test(&get_sig.odd)) {
        *cfg_msg = cfg_msg_rd;
    }

    __send_interthread_sig(next_me, 0,
                         __signal_number(cfg_sig_remote, next_me));
}

