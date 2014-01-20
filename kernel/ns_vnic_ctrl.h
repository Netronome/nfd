/*
 * Copyright (C) 2014 Netronome Systems, Inc. All rights reserved.
 *
 * This software may be redistributed under either of two provisions:
 *
 * 1. The GNU General Public License version 2 (see
 *    http://www.gnu.org/licenses/old-licenses/gpl-2.0.html or
 *    COPYING.txt file) when it is used for Linux or other
 *    compatible free software as defined by GNU at
 *    http://www.gnu.org/licenses/license-list.html.
 *
 * 2. Or under a non-free commercial license executed directly with
 *    Netronome. The direct Netronome license does not apply when the
 *    software is used as part of the Linux kernel.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * vim:shiftwidth=8:noexpandtab
 *
 * @file kernel/ns_vnic_ctrl.h
 *
 * Netronome vNIC driver: Control BAR layout
 */
#ifndef _NS_VNIC_CTRL_H_
#define _NS_VNIC_CTRL_H_


/**
 * Queue configuration CSRs:
 * - @NS_VNIC_QCFG_MAX_TXQS and @NS_VNIC_QCFG_MAX_RXQS specify the
 *   maximum number of queues support by the vNIC instance.  These
 *   CSRs are read-only.  Note, this information can also be derived
 *   from the BAR size.
 * - @NS_VNIC_QCFG_ACTIVE_TXQS and @NS_VNIC_QCFG_ACTIVE_RXQS indicate
 *   how many queues are used by the driver.  The queues used are
 *   0-(Active - 1). These CSRs are read-write.  The driver may only
 *   change these values when the device is down.
 * - @NS_VNIC_QCFG_TXQ_LEN and @NS_VNIC_QCFG_RXQ_LEN allow the device
 *   driver to configure the size of the TX and RX queues.  These CSRs
 *   are read-write.  The driver may only change these values when the
 *   device is down.
 */
#define NS_VNIC_QCFG_BASE		0x00c0
#define NS_VNIC_QCFG_MAX_TXQS		(NS_VNIC_QCFG_BASE)
#define NS_VNIC_QCFG_MAX_RXQS		(NS_VNIC_QCFG_BASE + 0x04)
#define NS_VNIC_QCFG_ACTIVE_TXQS	(NS_VNIC_QCFG_BASE + 0x08)
#define NS_VNIC_QCFG_ACTIVE_RXQS	(NS_VNIC_QCFG_BASE + 0x0c)
#define NS_VNIC_QCFG_TXQ_LEN		(NS_VNIC_QCFG_BASE + 0x10)
#define NS_VNIC_QCFG_RXQ_LEN		(NS_VNIC_QCFG_BASE + 0x14)


/**
 * DMA address of TX queues on the host
 * 64 in total. These are 32bit DMA addresses, i.e., the queue
 * structures must be located in the low 4GB of host memory.  The CSRs
 * are read-write.  The driver may only change these values when the
 * device is down.
 */
#define NS_VNIC_TXQ_BASE		0x0100
#define NS_VNIC_TXQ_ADDR(_x)		(NS_VNIC_TXQ_BASE + ((_x) * 0x4))


/**
 * DMA address of TX queues on the host
 * 64 in total. These are 32bit DMA addresses, i.e., the queue
 * structures must be located in the low 4GB of host memory.  The CSRs
 * are read-write.  The driver may only change these values when the
 * device is down.
 */
#define NS_VNIC_RXQ_BASE		0x0200
#define NS_VNIC_RXQ_ADDR(_x)		(NS_VNIC_RXQ_BASE + ((_x) * 0x4))

#endif /* _NS_VNIC_CTRL_H_ */
/*
 * Local variables:
 * c-file-style: "Linux"
 * indent-tabs-mode: t
 * End:
 */
