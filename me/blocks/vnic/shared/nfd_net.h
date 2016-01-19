/*
 * Copyright (C) 2014-2016,  Netronome Systems, Inc.  All rights reserved.
 *
 * @file          blocks/vnic/shared/nfd_net.h
 * @brief         Shared packet related defines
 */
#ifndef _BLOCKS__VNIC_SHARED_NFD_NET_H_
#define _BLOCKS__VNIC_SHARED_NFD_NET_H_

/* XXX consolidate with ns_vnic_cntl.h? */

/*
 * TX descriptor format
 */

/* Flags in the host TX descriptor */
#define PCIE_DESC_TX_CSUM           (1 << 7)
#define PCIE_DESC_TX_IP4_CSUM       (1 << 6)
#define PCIE_DESC_TX_TCP_CSUM       (1 << 5)
#define PCIE_DESC_TX_UDP_CSUM       (1 << 4)
#define PCIE_DESC_TX_VLAN           (1 << 3)
#define PCIE_DESC_TX_LSO            (1 << 2)
#define PCIE_DESC_TX_ENCAP_NONE     (0)
#define PCIE_DESC_TX_ENCAP_VXLAN    (1 << 1)
#define PCIE_DESC_TX_ENCAP_GRE      (1 << 0)

/* XXX add defines for TX descriptor? */


/*
 * RX and freelist descriptor format
 */
/* Flags in the RX descriptor */
#define PCIE_DESC_RX_RSS            (1 << 15)
#define PCIE_DESC_RX_I_IP4_CSUM     (1 << 14)
#define PCIE_DESC_RX_I_IP4_CSUM_OK  (1 << 13)
#define PCIE_DESC_RX_I_TCP_CSUM     (1 << 12)
#define PCIE_DESC_RX_I_TCP_CSUM_OK  (1 << 11)
#define PCIE_DESC_RX_I_UDP_CSUM     (1 << 10)
#define PCIE_DESC_RX_I_UDP_CSUM_OK  (1 <<  9)
#define PCIE_DESC_RX_SPARE          (1 <<  8)
#define PCIE_DESC_RX_EOP            (1 <<  7)
#define PCIE_DESC_RX_IP4_CSUM       (1 <<  6)
#define PCIE_DESC_RX_IP4_CSUM_OK    (1 <<  5)
#define PCIE_DESC_RX_TCP_CSUM       (1 <<  4)
#define PCIE_DESC_RX_TCP_CSUM_OK    (1 <<  3)
#define PCIE_DESC_RX_UDP_CSUM       (1 <<  2)
#define PCIE_DESC_RX_UDP_CSUM_OK    (1 <<  1)
#define PCIE_DESC_RX_VLAN           (1 <<  0)

/* XXX add defines for RX descriptor? */


#endif /* !_BLOCKS__VNIC_SHARED_NFD_NET_H_ */
