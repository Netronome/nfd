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
    local_csr_write(local_csr_same_me_signal, val);

    /* This method is only called from one CTX, which might not even
     * participate in the ordering, so we do not issue an
     * __implicit_write on sig.  The signal is possibly unused on this CTX.
     * The user can issue the __implicit_write if required. */
}


__intrinsic void
reorder_done(unsigned int start_ctx, SIGNAL *sig)
{
    unsigned int val;

    ctassert(__is_ct_const(start_ctx));

    /* XXX Might be necessary to avoid testing ctx() each time. */
#ifdef CTX_MODE_4
    /* 0,2,4,6 */
    if (ctx() != 6) {
#else
    /* 0,1,2,3,4,5,6,7 */
    if (ctx() != 7) {
#endif
        __critical_path(); /* Optimise for majority of contexts */

        val = (NFP_MECSR_SAME_ME_SIGNAL_SIG_NO(__signal_number(sig)) |
              NFP_MECSR_SAME_ME_SIGNAL_NEXT_CTX);
    } else {
        val= (NFP_MECSR_SAME_ME_SIGNAL_SIG_NO(__signal_number(sig)) |
              NFP_MECSR_SAME_ME_SIGNAL_CTX(start_ctx));
    }

    local_csr_write(local_csr_same_me_signal, val);

    /* Although another thread receives this particular signal, that thread
     * should return the same signal later, so allowing the next context to go
     * implicitly causes sig to become set in this context. */
    __implicit_write(sig);
}


__intrinsic unsigned int
reorder_get_next_ctx(unsigned int start_ctx)
{
    unsigned int val;

#ifdef CTX_MODE_4
    /* 0,2,4,6 */
    if (ctx() != 6) {
#else
    /* 0,1,2,3,4,5,6,7 */
    if (ctx() != 7) {
#endif
        val = NFP_MECSR_SAME_ME_SIGNAL_NEXT_CTX;
    } else {
        val= NFP_MECSR_SAME_ME_SIGNAL_CTX(start_ctx);
    }

    return val;
}


__intrinsic void
reorder_done_opt(unsigned int *next_ctx, SIGNAL *sig)
{
    unsigned int val;

    val = *next_ctx | NFP_MECSR_SAME_ME_SIGNAL_SIG_NO(__signal_number(sig));
    local_csr_write(local_csr_same_me_signal, val);

    /* Although another thread receives this particular signal, that thread
     * should return the same signal later, so allowing the next context to go
     * implicitly causes sig to become set in this context. */
    __implicit_write(sig);
}


__intrinsic void
reorder_self(SIGNAL *sig)
{
    unsigned int val, ctx;

    ctx = ctx();
    val = (NFP_MECSR_SAME_ME_SIGNAL_SIG_NO(__signal_number(sig)) |
           NFP_MECSR_SAME_ME_SIGNAL_CTX(ctx));

    local_csr_write(local_csr_same_me_signal, val);
    __implicit_write(sig);
}


__intrinsic void
reorder_future_sig(SIGNAL *sig, unsigned int cycles)
{
    unsigned sig_num, tslo;

    sig_num = __signal_number(sig);
    tslo = local_csr_read(local_csr_timestamp_low);
    tslo += cycles >> 4;
    local_csr_write(local_csr_active_ctx_future_count, tslo);
    local_csr_write(local_csr_active_future_count_signal, sig_num);
    __implicit_write(sig);
}


__intrinsic void
reorder_test_swap(SIGNAL *sig)
{
    if (signal_test(sig)) {
        __critical_path();
        return;
    }
    __wait_for_all(sig);
}
