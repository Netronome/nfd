/*
 * Copyright (C) 2014,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/utils/_c/qcntl.c
 * @brief         Configure and access the queue controller peripheral
 */
#include <assert.h>
#include <nfp.h>

#include <nfp6000/nfp_pcie.h>
#include <nfp6000/nfp_qc.h>

#include <vnic/utils/qcntl.h>

/* XXX This method could possibly be generalised and moved to libnfp.h */
__intrinsic unsigned int
__qc_read(unsigned char pcie_isl, unsigned char queue, enum qc_ptr_type ptr,
          sync_t sync, SIGNAL *sig)
{
    __gpr unsigned int addr_hi = pcie_isl << 30;
    unsigned int queue_base_addr;
    unsigned int offset;
    __xread unsigned int value;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);
    ctassert(ptr == QC_WPTR || ptr == QC_RPTR);

    if (ptr == QC_WPTR) {
        offset = NFP_QC_STS_HI;
    } else {
        offset = NFP_QC_STS_LO;
    }

    queue_base_addr = NFP_PCIE_QUEUE(queue) | offset;

    __intrinsic_begin();
    if (sync == sig_done) {
        __asm pcie[read_pci, value, addr_hi, <<8, queue_base_addr, 1],  \
            sig_done[*sig];
    } else {
        __asm pcie[read_pci, value, addr_hi, <<8, queue_base_addr, 1],  \
            ctx_swap[*sig];
    }
    __intrinsic_end();

    return value;
}

/* XXX This method could possibly be generalised and moved to libnfp.h */
__intrinsic void
__qc_write(unsigned char pcie_isl, unsigned char queue,
           __xwrite unsigned int *value, unsigned int offset, sync_t sync,
           SIGNAL *sig)
{
    __gpr unsigned int addr_hi = pcie_isl << 30;
    unsigned int queue_base_addr;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    queue_base_addr = NFP_PCIE_QUEUE(queue) | offset;

    __intrinsic_begin();
    if (sync == sig_done) {
        __asm pcie[write_pci, *value, addr_hi, <<8, queue_base_addr, 1], \
            sig_done[*sig];
    } else {
        __asm pcie[write_pci, *value, addr_hi, <<8, queue_base_addr, 1], \
            ctx_swap[*sig];
    }
    __intrinsic_end();
}


__intrinsic void
__qc_init_queue(unsigned char pcie_isl, unsigned char queue,
                struct qc_queue_config *cfg, sync_t sync, SIGNAL *s1,
                SIGNAL *s2)
{
    __gpr    unsigned int queue_base_addr;
    __gpr    struct nfp_qc_sts_hi config_hi_tmp;
    __xwrite struct nfp_qc_sts_hi config_hi;
    __gpr    struct nfp_qc_sts_lo config_lo_tmp;
    __xwrite struct nfp_qc_sts_lo config_lo;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    /* Initialise variables */
    config_hi_tmp.__raw = 0;
    config_lo_tmp.__raw = 0;

    /* Setup low config first, including ptr */
    config_lo_tmp.rptr_enable = 1;
    config_lo_tmp.event_data     = cfg->event_data;
    config_lo_tmp.event_type     = cfg->event_type;
    config_lo_tmp.readptr        = cfg->ptr;
    config_lo = config_lo_tmp;

    /* Setup hi config, setting empty flag */
    config_hi_tmp.watermark  = cfg->watermark;
    config_hi_tmp.size       = cfg->size;
    config_hi_tmp.writeptr   = cfg->ptr;
    config_hi_tmp.empty      = 1;
    config_hi = config_hi_tmp;

    /* Write settings to the queue
     * config_lo must be written before config_hi if ECC is enabled. */
    __qc_write(pcie_isl, queue, &config_lo.__raw, NFP_QC_STS_LO, sig_done, s1);
    __qc_write(pcie_isl, queue, &config_hi.__raw, NFP_QC_STS_HI, sig_done, s2);

    if (sync == ctx_swap) {
        __wait_for_all(s1, s2);
    }
}

__intrinsic void
qc_init_queue(unsigned char pcie_isl, unsigned char queue,
              struct qc_queue_config *cfg)
{
    SIGNAL s1, s2;

    __qc_init_queue(pcie_isl, queue, cfg, ctx_swap, &s1, &s2);
}


__intrinsic void
__qc_ping_queue(unsigned char pcie_isl, unsigned char queue,
                unsigned int event_data, enum pcie_qc_event event_type,
                sync_t sync, SIGNAL *sig)
{
    __gpr    struct nfp_qc_sts_lo config_lo_tmp;
    __xwrite struct nfp_qc_sts_lo config_lo;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    /* Initialise variables */
    config_lo_tmp.__raw = 0;
    config_lo_tmp.event_data = event_data;
    config_lo_tmp.event_type = event_type;
    config_lo = config_lo_tmp;

    /* Write data to NFP_QC_STS_LO to initiate ping */
    __qc_write(pcie_isl, queue, &config_lo.__raw, NFP_QC_STS_LO, sync, sig);
}

__intrinsic void
qc_ping_queue(unsigned char pcie_isl, unsigned char queue,
              unsigned int event_data, enum pcie_qc_event event_type)
{
    SIGNAL sig;

    __qc_ping_queue(pcie_isl, queue, event_data, event_type, ctx_swap, &sig);
}


__intrinsic void
__qc_add_to_ptr(unsigned char pcie_isl, unsigned char queue,
                enum qc_ptr_type ptr, unsigned int value, sync_t sync,
                SIGNAL *sig)
{
    __xwrite unsigned int data;
    struct nfp_qc_add_rptr val_str;
    unsigned int ptr_offset;

    ctassert(ptr == QC_WPTR || ptr == QC_RPTR);
    try_ctassert(value < (1<<18)); /* RPTR and WPTR are max 18bits */

    if (ptr == QC_WPTR) {
        ptr_offset = NFP_QC_ADD_WPTR;
    } else {
        ptr_offset = NFP_QC_ADD_RPTR;
    }

    /* NB: nfp_qc_add_rptr and nfp_qc_add_wptr place
     * the count field in the same bits */
    val_str.__raw = 0;
    val_str.val = value;
    data = val_str.__raw;

    /* Write data to specified address to initiate add */
    __qc_write(pcie_isl, queue, &data, ptr_offset, sync, sig);
}

__intrinsic void
qc_add_to_ptr(unsigned char pcie_isl, unsigned char queue,
              enum qc_ptr_type ptr, unsigned int value)
{
    SIGNAL sig;

    __qc_add_to_ptr(pcie_isl, queue, ptr, value, ctx_swap, &sig);
}
