/*
 * Copyright (C) 2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/utils/_c/ordering.c
 * @brief         NFP next neighbour ring interface
 */

#include <assert.h>
#include <nfp.h>

#include <nfp/me.h>

#include <nfp6000/nfp_me.h>

#include <vnic/utils/ordering.h>

__intrinsic void
reorder_start(unsigned int start_ctx, SIGNAL *sig)
{
    unsigned int val;

    ctassert(__is_ct_const(start_ctx));

    val= (NFP_MECSR_SAME_ME_SIGNAL_SIG_NO(__signal_number(sig)) |
          NFP_MECSR_SAME_ME_SIGNAL_CTX(start_ctx));
    local_csr_write(NFP_MECSR_SAME_ME_SIGNAL, val);
}

__intrinsic void
reorder_done(unsigned int start_ctx, SIGNAL *sig)
{
    unsigned int val;

    ctassert(__is_ct_const(start_ctx));

    /* XXX Might be necessary to avoid testing ctx() each time. */
    if (ctx() != 7) {
        __critical_path(); /* Optimise for majority of contexts */

        val = (NFP_MECSR_SAME_ME_SIGNAL_SIG_NO(__signal_number(sig)) |
              NFP_MECSR_SAME_ME_SIGNAL_NEXT_CTX);
    } else {
        val= (NFP_MECSR_SAME_ME_SIGNAL_SIG_NO(__signal_number(sig)) |
              NFP_MECSR_SAME_ME_SIGNAL_CTX(start_ctx));
    }

    local_csr_write(NFP_MECSR_SAME_ME_SIGNAL, val);
}

__intrinsic void
reorder_self(SIGNAL *sig)
{
    unsigned int val, ctx;

    ctx = ctx();
    val = (NFP_MECSR_SAME_ME_SIGNAL_SIG_NO(__signal_number(sig)) |
           NFP_MECSR_SAME_ME_SIGNAL_CTX(ctx));

    local_csr_write(NFP_MECSR_SAME_ME_SIGNAL, val);
}


__intrinsic void
reorder_test_swap(SIGNAL *sig)
{
    if (!signal_test(sig)) {
        __wait_for_all(sig);
    }
    __critical_path();
}
