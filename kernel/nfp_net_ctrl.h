/*
 * Copyright 2014-2015 Netronome, Inc.
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
 * vim:shiftwidth=8:noexpandtab
 *
 * Netronome network device driver: Control BAR layout
 */
#ifndef _NFP_NET_CTRL_H_
#define _NFP_NET_CTRL_H_

/**
 * Configuration BAR size.
 *
 * The configuration BAR is 8K in size, but on the NFP6000, due to
 * THB-350, 32k needs to be reserved.
 */
#ifdef __NFP_IS_6000
#define NFP_NET_CFG_BAR_SZ              (32 * 1024)
#else
#define NFP_NET_CFG_BAR_SZ              (8 * 1024)
#endif

/**
 * Offset in Freelist buffer where packet starts on RX
 */
#define NFP_NET_RX_OFFSET               32

/**
 * Hash type pre-pended when a RSS hash was computed
 */
#define NFP_NET_RSS_NONE                0
#define NFP_NET_RSS_IPV4                1
#define NFP_NET_RSS_IPV6                2
#define NFP_NET_RSS_IPV6_EX             3
#define NFP_NET_RSS_IPV4_TCP            4
#define NFP_NET_RSS_IPV6_TCP            5
#define NFP_NET_RSS_IPV6_EX_TCP         6
#define NFP_NET_RSS_IPV4_UDP            7
#define NFP_NET_RSS_IPV6_UDP            8
#define NFP_NET_RSS_IPV6_EX_UDP         9

/**
 * @NFP_NET_TXR_MAX:         Maximum number of TX rings
 * @NFP_NET_TXR_MASK:        Mask for TX rings
 * @NFP_NET_RXR_MAX:         Maximum number of RX rings
 * @NFP_NET_RXR_MASK:        Mask for RX rings
 */
#define NFP_NET_TXR_MAX                 64
#define NFP_NET_TXR_MASK                (NFP_NET_TXR_MAX - 1)
#define NFP_NET_RXR_MAX                 64
#define NFP_NET_RXR_MASK                (NFP_NET_RXR_MAX - 1)

/**
 * Read/Write config words
 * @NFP_NET_CFG_CTRL:        Global control
 * @NFP_NET_CFG_UPDATE:      Indicate which fields are updated
 * @NFP_NET_CFG_TXRS_ENABLE: Bitmask of enabled TX rings
 * @NFP_NET_CFG_RXRS_ENABLE: Bitmask of enabled RX rings
 * @NFP_NET_CFG_MTU:         Set MTU size
 * @NFP_NET_CFG_FLBUFSZ:     Set freelist buffer size (must be larger than MTU)
 * @NFP_NET_CFG_EXN:         MSI-X table entry for exceptions
 * @NFP_NET_CFG_LSC:         MSI-X table entry for link state changes
 * @NFP_NET_CFG_MACADDR:     MAC address
 *
 * TODO:
 * - define Error details in UPDATE
 */
#define NFP_NET_CFG_CTRL                0x0000
#define   NFP_NET_CFG_CTRL_ENABLE         (0x1 <<  0) /* Global enable */
#define   NFP_NET_CFG_CTRL_PROMISC        (0x1 <<  1) /* Enable Promisc mode */
#define   NFP_NET_CFG_CTRL_L2BC           (0x1 <<  2) /* Allow L2 Broadcast */
#define   NFP_NET_CFG_CTRL_L2MC           (0x1 <<  3) /* Allow L2 Multicast */
#define   NFP_NET_CFG_CTRL_RXCSUM         (0x1 <<  4) /* Enable RX Checksum */
#define   NFP_NET_CFG_CTRL_TXCSUM         (0x1 <<  5) /* Enable TX Checksum */
#define   NFP_NET_CFG_CTRL_RXVLAN         (0x1 <<  6) /* Enable VLAN strip */
#define   NFP_NET_CFG_CTRL_TXVLAN         (0x1 <<  7) /* Enable VLAN insert */
#define   NFP_NET_CFG_CTRL_SCATTER        (0x1 <<  8) /* Scatter DMA */
#define   NFP_NET_CFG_CTRL_GATHER         (0x1 <<  9) /* Gather DMA */
#define   NFP_NET_CFG_CTRL_LSO            (0x1 << 10) /* LSO/TSO */
#define   NFP_NET_CFG_CTRL_RINGCFG        (0x1 << 16) /* Ring runtime changes */
#define   NFP_NET_CFG_CTRL_RSS            (0x1 << 17) /* RSS */
#define   NFP_NET_CFG_CTRL_IRQMOD         (0x1 << 18) /* Interrupt moderation */
#define   NFP_NET_CFG_CTRL_RINGPRIO       (0x1 << 19) /* Ring priorities */
#define   NFP_NET_CFG_CTRL_MSIXAUTO       (0x1 << 20) /* MSI-X auto-masking */
#define   NFP_NET_CFG_CTRL_TXRWB          (0x1 << 21) /* Write-back of TX ring*/
#define   NFP_NET_CFG_CTRL_L2SWITCH       (0x1 << 22) /* L2 Switch */
#define   NFP_NET_CFG_CTRL_L2SWITCH_LOCAL (0x1 << 23) /* Switch to local */
#define NFP_NET_CFG_UPDATE              0x0004
#define   NFP_NET_CFG_UPDATE_GEN          (0x1 <<  0) /* General update */
#define   NFP_NET_CFG_UPDATE_RING         (0x1 <<  1) /* Ring config change */
#define   NFP_NET_CFG_UPDATE_RSS          (0x1 <<  2) /* RSS config change */
#define   NFP_NET_CFG_UPDATE_TXRPRIO      (0x1 <<  3) /* TX Ring prio change */
#define   NFP_NET_CFG_UPDATE_RXRPRIO      (0x1 <<  4) /* RX Ring prio change */
#define   NFP_NET_CFG_UPDATE_MSIX         (0x1 <<  5) /* MSI-X change */
#define   NFP_NET_CFG_UPDATE_L2SWITCH     (0x1 <<  6) /* Switch changes */
#define   NFP_NET_CFG_UPDATE_ERR          (0x1 << 31) /* A error occurred */
#define NFP_NET_CFG_TXRS_ENABLE         0x0008
#define NFP_NET_CFG_RXRS_ENABLE         0x0010
#define NFP_NET_CFG_MTU                 0x0018
#define NFP_NET_CFG_FLBUFSZ             0x001c
#define NFP_NET_CFG_EXN                 0x001f
#define NFP_NET_CFG_LSC                 0x0020
#define NFP_NET_CFG_MACADDR             0x0024

/**
 * Read-only words:
 * @NFP_NET_CFG_VERSION:     Firmware version number
 * @NFP_NET_CFG_STS:         Status
 * @NFP_NET_CFG_CAP:         Capabilities (same bits as @NFP_NET_CFG_CTRL)
 * @NFP_NET_MAX_TXRINGS:     Maximum number of TX rings
 * @NFP_NET_MAX_RXRINGS:     Maximum number of RX rings
 * @NFP_NET_MAX_MTU:         Maximum support MTU
 * @NFP_NET_CFG_START_TXQ:   Start Queue Control Queue to use for TX (PF only)
 * @NFP_NET_CFG_START_RXQ:   Start Queue Control Queue to use for RX (PF only)
 *
 * TODO:
 * - define more STS bits
 */
#define NFP_NET_CFG_VERSION             0x0030
#define NFP_NET_CFG_STS                 0x0034
#define   NFP_NET_CFG_STS_LINK            (0x1 << 0) /* Link up or down */
#define NFP_NET_CFG_CAP                 0x0038
#define NFP_NET_CFG_MAX_TXRINGS         0x003c
#define NFP_NET_CFG_MAX_RXRINGS         0x0040
#define NFP_NET_CFG_MAX_MTU             0x0044
/* Next two words are being used by VFs for solving THB350 issue */
#define NFP_NET_CFG_START_TXQ           0x0048
#define NFP_NET_CFG_START_RXQ           0x004c

/*
 * YDS-155 workaround for the NFP-3200
 * @NFP_NET_CFG_SPARE_ADDR:  Host DMA address for ME code to use as it likes
 */
#define NFP_NET_CFG_SPARE_ADDR          0x0060

/**
 * RSS configuration (only when NFP_NET_CFG_CTRL_RSS is enabled)
 * @NFP_NET_CFG_RSS_CFG:     RSS configuration word
 * @NFP_NET_CFG_RSS_KEY:     RSS "secret" key
 * @NFP_NET_CFG_RSS_ITBL:    RSS indirection table
 */
#define NFP_NET_CFG_RSS_BASE            0x0100
#define NFP_NET_CFG_RSS_CTRL            NFP_NET_CFG_RSS_BASE
#define   NFP_NET_CFG_RSS_MASK            (0x7f)
#define   NFP_NET_CFG_RSS_MASK_of(_x)     ((_x) & 0x7f)
#define   NFP_NET_CFG_RSS_IPV4            (1 <<  8) /* RSS for IPv4 */
#define   NFP_NET_CFG_RSS_IPV6            (1 <<  9) /* RSS for IPv6 */
#define   NFP_NET_CFG_RSS_IPV4_TCP        (1 << 10) /* RSS for IPv4/TCP */
#define   NFP_NET_CFG_RSS_IPV4_UDP        (1 << 11) /* RSS for IPv4/UDP */
#define   NFP_NET_CFG_RSS_IPV6_TCP        (1 << 12) /* RSS for IPv6/TCP */
#define   NFP_NET_CFG_RSS_IPV6_UDP        (1 << 13) /* RSS for IPv6/UDP */
#define   NFP_NET_CFG_RSS_TOEPLITZ        (1 << 24) /* Use Toeplitz hash */
#define NFP_NET_CFG_RSS_KEY             (NFP_NET_CFG_RSS_BASE + 0x4)
#define NFP_NET_CFG_RSS_KEY_SZ          0x28
#define NFP_NET_CFG_RSS_ITBL            (NFP_NET_CFG_RSS_BASE + 0x4 + \
					 NFP_NET_CFG_RSS_KEY_SZ)
#define NFP_NET_CFG_RSS_ITBL_SZ         0x80

/**
 * TX ring configuration
 * @NFP_NET_CFG_TXR_BASE:    Base offset for TX ring configuration
 * @NFP_NET_CFG_TXR_ADDR:    Offset of host address for TX ring _x
 * @NFP_NET_CFG_TXR_WB_ADDR: Offset of host addr for TX ring _x ptr write back
 * @NFP_NET_CFG_TXR_SZ:      Offset to configure size for ring _x
 * @NFP_NET_CFG_TXR_VEC:     Offset to set MSI-X table entry for ring _x
 * @NFP_NET_CFG_TXR_PRIO:    Offset to set per TX ring priorities for ring _x.
 */
#define NFP_NET_CFG_TXR_BASE            0x0200
#define NFP_NET_CFG_TXR_ADDR(_x)        (NFP_NET_CFG_TXR_BASE + ((_x) * 0x8))
#define NFP_NET_CFG_TXR_WB_ADDR(_x)     (NFP_NET_CFG_TXR_BASE + 0x200 + \
					 ((_x) * 0x8))
#define NFP_NET_CFG_TXR_SZ(_x)          (NFP_NET_CFG_TXR_BASE + 0x400 + (_x))
#define NFP_NET_CFG_TXR_VEC(_x)         (NFP_NET_CFG_TXR_BASE + 0x440 + (_x))
#define NFP_NET_CFG_TXR_PRIO(_x)        (NFP_NET_CFG_TXR_BASE + 0x480 + (_x))

/**
 * RX ring configuration
 * @NFP_NET_CFG_RXR_BASE:    Base offset for RX ring configuration
 * @NFP_NET_CFG_RXR_ADDR:    Offset of host address for RX ring _x
 * @NFP_NET_CFG_RXR_SZ:      Offset to configure size for ring _x
 * @NFP_NET_CFG_RXR_VEC:     Offset to set MSI-X table entry for ring _x
 * @NFP_NET_CFG_TXR_PRIO:    Offset to set per RX ring priorities for ring _x.
 */
#define NFP_NET_CFG_RXR_BASE            0x0800
#define NFP_NET_CFG_RXR_ADDR(_x)        (NFP_NET_CFG_RXR_BASE + ((_x) * 0x8))
#define NFP_NET_CFG_RXR_SZ(_x)          (NFP_NET_CFG_RXR_BASE + 0x200 + (_x))
#define NFP_NET_CFG_RXR_VEC(_x)         (NFP_NET_CFG_RXR_BASE + 0x240 + (_x))
#define NFP_NET_CFG_RXR_PRIO(_x)        (NFP_NET_CFG_RXR_BASE + 0x280 + (_x))

/**
 * General device stats (all counters are 64bit)
 */
#define NFP_NET_CFG_STATS_BASE          0x0b00

#define NFP_NET_CFG_STATS_RX_DISCARDS   (NFP_NET_CFG_STATS_BASE + 0x00)
#define NFP_NET_CFG_STATS_RX_ERRORS     (NFP_NET_CFG_STATS_BASE + 0x08)
#define NFP_NET_CFG_STATS_RX_OCTETS     (NFP_NET_CFG_STATS_BASE + 0x10)
#define NFP_NET_CFG_STATS_RX_UC_OCTETS  (NFP_NET_CFG_STATS_BASE + 0x18)
#define NFP_NET_CFG_STATS_RX_MC_OCTETS  (NFP_NET_CFG_STATS_BASE + 0x20)
#define NFP_NET_CFG_STATS_RX_BC_OCTETS  (NFP_NET_CFG_STATS_BASE + 0x28)
#define NFP_NET_CFG_STATS_RX_FRAMES     (NFP_NET_CFG_STATS_BASE + 0x30)
#define NFP_NET_CFG_STATS_RX_MC_FRAMES  (NFP_NET_CFG_STATS_BASE + 0x38)
#define NFP_NET_CFG_STATS_RX_BC_FRAMES  (NFP_NET_CFG_STATS_BASE + 0x40)

#define NFP_NET_CFG_STATS_TX_DISCARDS   (NFP_NET_CFG_STATS_BASE + 0x48)
#define NFP_NET_CFG_STATS_TX_ERRORS     (NFP_NET_CFG_STATS_BASE + 0x50)
#define NFP_NET_CFG_STATS_TX_OCTETS     (NFP_NET_CFG_STATS_BASE + 0x58)
#define NFP_NET_CFG_STATS_TX_UC_OCTETS  (NFP_NET_CFG_STATS_BASE + 0x60)
#define NFP_NET_CFG_STATS_TX_MC_OCTETS  (NFP_NET_CFG_STATS_BASE + 0x68)
#define NFP_NET_CFG_STATS_TX_BC_OCTETS  (NFP_NET_CFG_STATS_BASE + 0x70)
#define NFP_NET_CFG_STATS_TX_FRAMES     (NFP_NET_CFG_STATS_BASE + 0x78)
#define NFP_NET_CFG_STATS_TX_MC_FRAMES  (NFP_NET_CFG_STATS_BASE + 0x80)
#define NFP_NET_CFG_STATS_TX_BC_FRAMES  (NFP_NET_CFG_STATS_BASE + 0x88)

/**
 * Per ring stats (optional)
 * @NFP_NET_CFG_TXR_STATS:   TX ring statistics (Packet and Byte count)
 * @NFP_NET_CFG_RXR_STATS:   RX ring statistics (Packet and Byte count)
 */
#define NFP_NET_CFG_TXR_STATS_BASE      0x1000
#define NFP_NET_CFG_TXR_STATS(_x)       (NFP_NET_CFG_TXR_STATS_BASE + \
					 ((_x) * 0x10))
#define NFP_NET_CFG_RXR_STATS_BASE      0x1400
#define NFP_NET_CFG_RXR_STATS(_x)       (NFP_NET_CFG_RXR_STATS_BASE + \
					 ((_x) * 0x10))

#endif /* _NFP_NET_CTRL_H_ */
/*
 * Local variables:
 * c-file-style: "Linux"
 * indent-tabs-mode: t
 * End:
 */
