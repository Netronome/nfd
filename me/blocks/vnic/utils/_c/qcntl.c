/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/utils/_c/qcntl.c
 * @brief         Configure and access the queue controller peripheral
 */
#include <assert.h>
#include <nfp.h>

#include <nfp6000/nfp_pcie.h>
#include <nfp6000/nfp_qc.h>

#include <vnic/utils/qcntl.h>

/* XXX This method could possibly be generalised and moved to libnfp.h */
__intrinsic void
__qc_read(unsigned char pcie_isl, unsigned int queue, enum qc_ptr_type ptr,
          __xread unsigned int *value, sync_t sync, SIGNAL *sig)
{
    __gpr unsigned int addr_hi = pcie_isl << 30;
    unsigned int queue_base_addr;
    unsigned int offset;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);
    ctassert(ptr == QC_WPTR || ptr == QC_RPTR);

    if (ptr == QC_WPTR) {
        offset = NFP_QC_STS_HI;
    } else {
        offset = NFP_QC_STS_LO;
    }

    queue_base_addr = NFP_PCIE_QUEUE(queue) | offset;

    if (sync == sig_done) {
        __asm pcie[read_pci, *value, addr_hi, <<8, queue_base_addr, 1], \
            sig_done[*sig];
    } else {
        __asm pcie[read_pci, *value, addr_hi, <<8, queue_base_addr, 1], \
            ctx_swap[*sig];
    }
}

__intrinsic unsigned int
qc_read(unsigned char pcie_isl, unsigned int queue, enum qc_ptr_type ptr)
{
    __xread unsigned int xfer;
    SIGNAL sig;

    __qc_read(pcie_isl, queue, ptr, &xfer, ctx_swap, &sig);
    return xfer;
}


/* XXX This method could possibly be generalised and moved to libnfp.h */
__intrinsic void
__qc_write(unsigned char pcie_isl, unsigned int queue,
           __xwrite unsigned int *value, unsigned int offset,
           sync_t sync, SIGNAL *sig)
{
    __gpr unsigned int addr_hi = pcie_isl << 30;
    unsigned int queue_base_addr;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    queue_base_addr = NFP_PCIE_QUEUE(queue) | offset;

    if (sync == sig_done) {
        __asm pcie[write_pci, *value, addr_hi, <<8, queue_base_addr, 1], \
            sig_done[*sig];
    } else {
        __asm pcie[write_pci, *value, addr_hi, <<8, queue_base_addr, 1], \
            ctx_swap[*sig];
    }
}


__intrinsic void
qc_init_queue(unsigned char pcie_isl, unsigned int queue,
              struct qc_queue_config *cfg)
{
    __gpr    unsigned int queue_base_addr;
    __gpr    struct nfp_qc_sts_hi config_hi_tmp;
    __xwrite struct nfp_qc_sts_hi config_hi;
    __gpr    struct nfp_qc_sts_lo config_lo_tmp;
    __xwrite struct nfp_qc_sts_lo config_lo;
    SIGNAL s1, s2;

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
    __qc_write(pcie_isl, queue, &config_lo.__raw, NFP_QC_STS_LO, sig_done, &s1);
    __qc_write(pcie_isl, queue, &config_hi.__raw, NFP_QC_STS_HI, sig_done, &s2);

    __wait_for_all(&s1, &s2);
}


__intrinsic void
__qc_ping_queue(unsigned char pcie_isl, unsigned int queue,
                unsigned int event_data, enum pcie_qc_event event_type,
                __xwrite unsigned int *xfer, sync_t sync, SIGNAL *sig)
{
    __gpr    struct nfp_qc_sts_lo config_lo_tmp;

    ctassert(__is_ct_const(sync));
    ctassert(sync == sig_done || sync == ctx_swap);

    /* Initialise variables */
    config_lo_tmp.__raw = 0;
    config_lo_tmp.event_data = event_data;
    config_lo_tmp.event_type = event_type;
    *xfer = config_lo_tmp.__raw;

    /* Write data to NFP_QC_STS_LO to initiate ping */
    __qc_write(pcie_isl, queue, xfer, NFP_QC_STS_LO, sync, sig);
}

__intrinsic void
qc_ping_queue(unsigned char pcie_isl, unsigned int queue,
              unsigned int event_data, enum pcie_qc_event event_type)
{
    __xwrite unsigned int xfer;
    SIGNAL sig;

    __qc_ping_queue(pcie_isl, queue, event_data, event_type, &xfer,
                    ctx_swap, &sig);
}


__intrinsic void
__qc_add_to_ptr(unsigned char pcie_isl, unsigned int queue,
                enum qc_ptr_type ptr, unsigned int value,
                __xread unsigned int *xfer, sync_t sync, SIGNAL *sig)
{
    __gpr unsigned int addr_hi = pcie_isl << 30;
    struct nfp_qc_add_rptr val_str;
    unsigned int ptr_offset;

    ctassert(ptr == QC_WPTR || ptr == QC_RPTR);
    try_ctassert(value < 64); /* We are using the address bits to pass value */

    if (ptr == QC_WPTR) {
        ptr_offset = NFP_QC_ADD_WPTR;
    } else {
        ptr_offset = NFP_QC_ADD_RPTR;
    }

    /* Code the value to add into the ptr_offset
     * The value is taken from bits [9:4], and bit 10 must be set to
     * indicate that we are using this access mode. */
    ptr_offset |= (value << 4);
    ptr_offset |= (1 << 10);

    ptr_offset |= NFP_PCIE_QUEUE(queue);

    if (sync == sig_done) {
        __asm pcie[read_pci, *xfer, addr_hi, <<8, ptr_offset, 1], \
            sig_done[*sig];
    } else {
        __asm pcie[read_pci, *xfer, addr_hi, <<8, ptr_offset, 1], \
            ctx_swap[*sig];
    }
}

__intrinsic void
qc_add_to_ptr(unsigned char pcie_isl, unsigned int queue,
              enum qc_ptr_type ptr, unsigned int value)
{
    __xread unsigned int xfer;
    SIGNAL sig;

    __qc_add_to_ptr(pcie_isl, queue, ptr, value, &xfer, ctx_swap, &sig);
    __implicit_read(&xfer);
}

