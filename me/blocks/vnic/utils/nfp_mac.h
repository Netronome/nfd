/*
 * Copyright (C) 2015,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          nfp6000/nfp_mac.h
 * @brief         NFP6000 MAC CSR definitions
 */

#ifndef _NFP6000__NFP_MAC_H_
#define _NFP6000__NFP_MAC_H_


/* XPB BUS offset of the MAC island */
#define NFP_MAC_XPB_OFF(_isl)  ((_isl << 24) | 0x8400000)



/* MAC CSRs */
/* MacXPB: <base>.MacCsr */
#define NFP_MAC_GLBL_MAP_MAC_CSR                           0x0000
/* InterruptManagerMap: <base>.MacIntrMng */
#define NFP_MAC_GLBL_MAP_MAC_INTR_MNG                      0x10000
/* MacInterlaken: <base>.MacInterlaken0... */
#define NFP_MAC_GLBL_MAP_MAC_ILK(x)                        (0x20000 + ((x) * 0x10000))
/* MacEthernet: <base>.MacEthernet0... */
#define NFP_MAC_GLBL_MAP_MAC_ETH(x)                        (0x40000 + ((x) * 0x20000))
/* MacMemCtrl: <base>.MacCoresCsrMemCtrl0... */
#define NFP_MAC_GLBL_MAP_MAC_CORES_CSR_MEM_CTRL(x)         (0x80000 + ((x) * 0x10000))
/* ECCControlMany: <base>.MacCdsEccMon0... */
#define NFP_MAC_GLBL_MAP_MAC_CDS_ECC_MON(x)                (0xa0000 + ((x) * 0x10000))
/* IslandOverlayExtMap: <base>.MacOvlExt */
#define NFP_MAC_GLBL_MAP_MAC_OVL_EXT                       0x300000


/*
 * Register: MacBlkReset
 *   [31:24]   Rfu2
 *   [23]      MacHy1StatRst
 *   [22]      MacHy0StatRst
 *   [21]      MacTxRstMpb
 *   [20]      MacRxRstMpb
 *   [19]      MacTxRstCore
 *   [18]      MacRxRstCore
 *   [17]      MacFcX2RstLk1
 *   [16]      MacFcX2RstLk0
 *   [15]      MacRxRstLk1
 *   [14]      MacRxRstLk0
 *   [13]      MacTxRstLk1
 *   [12]      MacTxRstLk0
 *   [11]      MacRstLk1
 *   [10]      MacRstLk0
 *   [9]       MacX2ClkEnLk1
 *   [8]       MacX2ClkEnLk0
 *   [7]       MacCoreClkEnLk1
 *   [6]       MacCoreClkEnLk0
 *   [5]       MacCoreClkEnHy1
 *   [4]       MacCoreClkEnHy0
 *   [3]       Rfu
 *   [2]       MacSerDesRst
 *   [1]       MacSReset
 *   [0]       MacHReset
 *
 * Name(s):
 * <base>.MacBlkReset
 */
#define NFP_MAC_CSR_MAC_BLOCK_RST                          0x0000
#define   NFP_MAC_CSR_MAC_BLOCK_RST_RFU2(x)                  (((x) & 0xff) << 24)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_RFU2_of(x)               (((x) >> 24) & 0xff)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_RFU2_bf                0, 31, 24
#define     NFP_MAC_CSR_MAC_BLOCK_RST_RFU2_msk               (0xff)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_RFU2_shf               (24)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_HY1_STAT_RST         (1 << 23)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_HY1_STAT_RST_bf    0, 23, 23
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_HY1_STAT_RST_msk   (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_HY1_STAT_RST_bit   (23)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_HY0_STAT_RST         (1 << 22)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_HY0_STAT_RST_bf    0, 22, 22
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_HY0_STAT_RST_msk   (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_HY0_STAT_RST_bit   (22)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TX_RST_MPB           (1 << 21)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TX_RST_MPB_bf      0, 21, 21
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TX_RST_MPB_msk     (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TX_RST_MPB_bit     (21)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RX_RST_MPB           (1 << 20)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RX_RST_MPB_bf      0, 20, 20
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RX_RST_MPB_msk     (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RX_RST_MPB_bit     (20)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TX_RST_CORE          (1 << 19)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TX_RST_CORE_bf     0, 19, 19
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TX_RST_CORE_msk    (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TX_RST_CORE_bit    (19)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RX_RST_CORE          (1 << 18)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RX_RST_CORE_bf     0, 18, 18
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RX_RST_CORE_msk    (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RX_RST_CORE_bit    (18)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_FCX2RST_LK1          (1 << 17)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_FCX2RST_LK1_bf     0, 17, 17
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_FCX2RST_LK1_msk    (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_FCX2RST_LK1_bit    (17)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_FCX2RST_LK0          (1 << 16)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_FCX2RST_LK0_bf     0, 16, 16
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_FCX2RST_LK0_msk    (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_FCX2RST_LK0_bit    (16)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RXRST_LK1            (1 << 15)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RXRST_LK1_bf       0, 15, 15
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RXRST_LK1_msk      (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RXRST_LK1_bit      (15)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RXRST_LK0            (1 << 14)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RXRST_LK0_bf       0, 14, 14
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RXRST_LK0_msk      (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RXRST_LK0_bit      (14)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TXRST_LK1            (1 << 13)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TXRST_LK1_bf       0, 13, 13
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TXRST_LK1_msk      (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TXRST_LK1_bit      (13)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TXRST_LK0            (1 << 12)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TXRST_LK0_bf       0, 12, 12
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TXRST_LK0_msk      (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_TXRST_LK0_bit      (12)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RST_LK1              (1 << 11)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RST_LK1_bf         0, 11, 11
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RST_LK1_msk        (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RST_LK1_bit        (11)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RST_LK0              (1 << 10)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RST_LK0_bf         0, 10, 10
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RST_LK0_msk        (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_RST_LK0_bit        (10)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_X2CLKEN_LK1          (1 << 9)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_X2CLKEN_LK1_bf     0, 9, 9
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_X2CLKEN_LK1_msk    (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_X2CLKEN_LK1_bit    (9)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_X2CLKEN_LK0          (1 << 8)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_X2CLKEN_LK0_bf     0, 8, 8
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_X2CLKEN_LK0_msk    (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_X2CLKEN_LK0_bit    (8)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_LK1        (1 << 7)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_LK1_bf   0, 7, 7
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_LK1_msk  (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_LK1_bit  (7)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_LK0        (1 << 6)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_LK0_bf   0, 6, 6
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_LK0_msk  (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_LK0_bit  (6)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_HY1        (1 << 5)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_HY1_bf   0, 5, 5
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_HY1_msk  (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_HY1_bit  (5)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_HY0        (1 << 4)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_HY0_bf   0, 4, 4
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_HY0_msk  (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_CORECLKEN_HY0_bit  (4)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_RFU                      (1 << 3)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_RFU_bf                 0, 3, 3
#define     NFP_MAC_CSR_MAC_BLOCK_RST_RFU_msk                (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_RFU_bit                (3)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_SERDES_RST           (1 << 2)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_SERDES_RST_bf      0, 2, 2
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_SERDES_RST_msk     (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_SERDES_RST_bit     (2)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_S_RST                (1 << 1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_S_RST_bf           0, 1, 1
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_S_RST_msk          (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_S_RST_bit          (1)
#define   NFP_MAC_CSR_MAC_BLOCK_RST_MAC_H_RST                (1 << 0)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_H_RST_bf           0, 0, 0
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_H_RST_msk          (0x1)
#define     NFP_MAC_CSR_MAC_BLOCK_RST_MAC_H_RST_bit          (0)


/*
 * Register: MacHydBlkReset
 *   [31:20]   MacHydRxSerDesIfRst
 *   [19:16]   Rfu
 *   [15:4]    MacHydTxSerDesIfRst
 *   [3]       MacHydRxFFRst
 *   [2]       MacHydTxFFRst
 *   [1]       MacHydRegRst
 *   [0]       MacHydRefRst
 *
 * Name(s):
 * <base>.MacHyd0BlkReset <base>.MacHyd1BlkReset
 */
#define NFP_MAC_CSR_MAC_HYD0_BLOCK_RST                     0x0004
#define NFP_MAC_CSR_MAC_HYD1BLOCK_RST                      0x0008
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_RX_SERDES_RST(x) (((x) & 0xfff) << 20)
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_RX_SERDES_RST_of(x) (((x) >> 20) & 0xfff)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_RX_SERDES_RST_bf 0, 31, 20
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_RX_SERDES_RST_msk (0xfff)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_RX_SERDES_RST_shf (20)
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_RFU(x)              (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_RFU_of(x)           (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_RFU_bf            0, 19, 16
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_RFU_msk           (0xf)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_RFU_shf           (16)
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_TX_SERDES_RST(x) (((x) & 0xfff) << 4)
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_TX_SERDES_RST_of(x) (((x) >> 4) & 0xfff)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_TX_SERDES_RST_bf 0, 15, 4
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_TX_SERDES_RST_msk (0xfff)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_TX_SERDES_RST_shf (4)
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_RX_FF_RST   (1 << 3)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_RX_FF_RST_bf 0, 3, 3
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_RX_FF_RST_msk (0x1)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_RX_FF_RST_bit (3)
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_TX_FF_RST   (1 << 2)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_TX_FF_RST_bf 0, 2, 2
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_TX_FF_RST_msk (0x1)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_TX_FF_RST_bit (2)
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_REG_RST     (1 << 1)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_REG_RST_bf 0, 1, 1
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_REG_RST_msk (0x1)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_REG_RST_bit (1)
#define   NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_REF_RST     (1 << 0)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_REF_RST_bf 0, 0, 0
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_REF_RST_msk (0x1)
#define     NFP_MAC_CSR_MAC_HYD0_BLOCK_RST_MAC_HYD_REF_RST_bit (0)


/*
 * Register: MacMuxCtrl
 *   [31:25]   Rfu
 *   [24]      LASelect
 *   [23:0]    MacSerDesIntlknSel
 *
 * Name(s):
 * <base>.MacMuxCtrl
 */
#define NFP_MAC_CSR_MAC_MUX_CTRL                           0x000c
#define   NFP_MAC_CSR_MAC_MUX_CTRL_RFU(x)                    (((x) & 0x7f) << 25)
#define   NFP_MAC_CSR_MAC_MUX_CTRL_RFU_of(x)                 (((x) >> 25) & 0x7f)
#define     NFP_MAC_CSR_MAC_MUX_CTRL_RFU_bf                  0, 31, 25
#define     NFP_MAC_CSR_MAC_MUX_CTRL_RFU_msk                 (0x7f)
#define     NFP_MAC_CSR_MAC_MUX_CTRL_RFU_shf                 (25)
#define   NFP_MAC_CSR_MAC_MUX_CTRL_MAC_ILA_SEL               (1 << 24)
#define     NFP_MAC_CSR_MAC_MUX_CTRL_MAC_ILA_SEL_Interlaken MAC Select (0)
#define     NFP_MAC_CSR_MAC_MUX_CTRL_MAC_ILA_SEL_Interlaken ILA Select (0x1000000)
#define     NFP_MAC_CSR_MAC_MUX_CTRL_MAC_ILA_SEL_bf          0, 24, 24
#define     NFP_MAC_CSR_MAC_MUX_CTRL_MAC_ILA_SEL_msk         (0x1)
#define     NFP_MAC_CSR_MAC_MUX_CTRL_MAC_ILA_SEL_bit         (24)
#define   NFP_MAC_CSR_MAC_MUX_CTRL_MAC_INLK_SEL(x)           (((x) & 0xffffff) << 0)
#define   NFP_MAC_CSR_MAC_MUX_CTRL_MAC_INLK_SEL_of(x)        (((x) >> 0) & 0xffffff)
#define     NFP_MAC_CSR_MAC_MUX_CTRL_MAC_INLK_SEL_bf         0, 23, 0
#define     NFP_MAC_CSR_MAC_MUX_CTRL_MAC_INLK_SEL_msk        (0xffffff)
#define     NFP_MAC_CSR_MAC_MUX_CTRL_MAC_INLK_SEL_shf        (0)


/*
 * Register: MacSerDesEn
 *   [31:24]   Rfu
 *   [23:0]    SerDesEnable
 *
 * Name(s):
 * <base>.MacSerDesEn
 */
#define NFP_MAC_CSR_MAC_SERDES_EN                          0x0010
#define   NFP_MAC_CSR_MAC_SERDES_EN_RFU(x)                   (((x) & 0xff) << 24)
#define   NFP_MAC_CSR_MAC_SERDES_EN_RFU_of(x)                (((x) >> 24) & 0xff)
#define     NFP_MAC_CSR_MAC_SERDES_EN_RFU_bf                 0, 31, 24
#define     NFP_MAC_CSR_MAC_SERDES_EN_RFU_msk                (0xff)
#define     NFP_MAC_CSR_MAC_SERDES_EN_RFU_shf                (24)
#define   NFP_MAC_CSR_MAC_SERDES_EN_SERDES_ENABLE(x)         (((x) & 0xffffff) << 0)
#define   NFP_MAC_CSR_MAC_SERDES_EN_SERDES_ENABLE_of(x)      (((x) >> 0) & 0xffffff)
#define     NFP_MAC_CSR_MAC_SERDES_EN_SERDES_ENABLE_bf       0, 23, 0
#define     NFP_MAC_CSR_MAC_SERDES_EN_SERDES_ENABLE_msk      (0xffffff)
#define     NFP_MAC_CSR_MAC_SERDES_EN_SERDES_ENABLE_shf      (0)


/*
 * Register: MacSysSupCtrl
 *   [31:24]   MacSysSupCtrlC
 *   [23:20]   MacMpbFreeBufFifoLowWm
 *   [19]      MacIgLnkLstFreezeOnErrN
 *   [18]      MacEgLnkLstFreezeOnErrN
 *   [17]      DwrrArbiterDisable
 *   [16]      DwrrWeightWrEnable
 *   [15]      MacIlkLiveIntSel
 *   [14]      Lk1IgDqSegmentedEn
 *   [13]      Lk0IgDqSegmentedEn
 *   [12]      Lk1LinklistEn
 *   [11]      Lk0LinklistEn
 *   [10]      Hy1LinklistEn
 *   [9]       Hy0LinklistEn
 *   [8]       SplitMemIG
 *   [7]       ExtraEthHistMode
 *   [6:4]     MacSysSupCtrlA
 *   [3]       TimeStampFrc
 *   [2]       TimeStampSet
 *   [1]       TimeStampRst
 *   [0]       TimeStampEn
 *
 * Name(s):
 * <base>.MacSysSupCtrl
 */
#define NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL                   0x0014
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLC(x) (((x) & 0xff) << 24)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLC_of(x) (((x) >> 24) & 0xff)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLC_bf 0, 31, 24
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLC_msk (0xff)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLC_shf (24)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_MPB_FREE_BUF_FIFO_LOW_WM(x) (((x) & 0xf) << 20)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_MPB_FREE_BUF_FIFO_LOW_WM_of(x) (((x) >> 20) & 0xf)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_MPB_FREE_BUF_FIFO_LOW_WM_bf 0, 23, 20
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_MPB_FREE_BUF_FIFO_LOW_WM_msk (0xf)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_MPB_FREE_BUF_FIFO_LOW_WM_shf (20)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_IG_LNK_LST_FREEZE_ON_ERR_N (1 << 19)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_IG_LNK_LST_FREEZE_ON_ERR_N_bf 0, 19, 19
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_IG_LNK_LST_FREEZE_ON_ERR_N_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_IG_LNK_LST_FREEZE_ON_ERR_N_bit (19)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_EG_LNK_LST_FREEZE_ON_ERR_N (1 << 18)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_EG_LNK_LST_FREEZE_ON_ERR_N_bf 0, 18, 18
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_EG_LNK_LST_FREEZE_ON_ERR_N_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_EG_LNK_LST_FREEZE_ON_ERR_N_bit (18)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_DWRR_ARBITER_DISABLE (1 << 17)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_DWRR_ARBITER_DISABLE_bf 0, 17, 17
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_DWRR_ARBITER_DISABLE_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_DWRR_ARBITER_DISABLE_bit (17)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_DWRR_WEIGHT_WR_ENABLE (1 << 16)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_DWRR_WEIGHT_WR_ENABLE_bf 0, 16, 16
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_DWRR_WEIGHT_WR_ENABLE_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_DWRR_WEIGHT_WR_ENABLE_bit (16)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_ILK_LIVE_INT_SEL (1 << 15)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_ILK_LIVE_INT_SEL_bf 0, 15, 15
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_ILK_LIVE_INT_SEL_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_ILK_LIVE_INT_SEL_bit (15)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK1_IG_DQ_SEGEMENTED_EN (1 << 14)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK1_IG_DQ_SEGEMENTED_EN_bf 0, 14, 14
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK1_IG_DQ_SEGEMENTED_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK1_IG_DQ_SEGEMENTED_EN_bit (14)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK0_IG_DQ_SEGEMENTED_EN (1 << 13)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK0_IG_DQ_SEGEMENTED_EN_bf 0, 13, 13
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK0_IG_DQ_SEGEMENTED_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK0_IG_DQ_SEGEMENTED_EN_bit (13)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK1_LINKLIST_EN   (1 << 12)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK1_LINKLIST_EN_bf 0, 12, 12
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK1_LINKLIST_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK1_LINKLIST_EN_bit (12)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK0_LINKLIST_EN   (1 << 11)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK0_LINKLIST_EN_bf 0, 11, 11
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK0_LINKLIST_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_LK0_LINKLIST_EN_bit (11)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_HY1_LINKLIST_EN   (1 << 10)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_HY1_LINKLIST_EN_bf 0, 10, 10
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_HY1_LINKLIST_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_HY1_LINKLIST_EN_bit (10)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_HY0_LINKLIST_EN   (1 << 9)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_HY0_LINKLIST_EN_bf 0, 9, 9
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_HY0_LINKLIST_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_HY0_LINKLIST_EN_bit (9)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_SPLIT_MEM_IG      (1 << 8)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_SPLIT_MEM_IG_bf 0, 8, 8
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_SPLIT_MEM_IG_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_SPLIT_MEM_IG_bit (8)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_EXTRA_ETH_HIST_MODE (1 << 7)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_EXTRA_ETH_HIST_MODE_bf 0, 7, 7
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_EXTRA_ETH_HIST_MODE_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_EXTRA_ETH_HIST_MODE_bit (7)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLA(x) (((x) & 7) << 4)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLA_of(x) (((x) >> 4) & 7)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLA_bf 0, 6, 4
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLA_msk (0x7)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_MAC_SYS_SUPPORT_CTRLA_shf (4)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_FRC     (1 << 3)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_FRC_bf 0, 3, 3
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_FRC_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_FRC_bit (3)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_SET     (1 << 2)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_SET_bf 0, 2, 2
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_SET_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_SET_bit (2)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_RST     (1 << 1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_RST_bf 0, 1, 1
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_RST_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_RST_bit (1)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_EN      (1 << 0)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_EN_bf 0, 0, 0
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_CTRL_TIMESTAMP_EN_bit (0)


/*
 * Register: MacSysSupStat
 *   [31:0]    MacSysSupStat
 *
 * Name(s):
 * <base>.MacSysSupStat
 */
#define NFP_MAC_CSR_MAC_SYS_SUPPORT_STAT                   0x0018
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_STAT_MAC_SYS_SUPPORT_STAT(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_SYS_SUPPORT_STAT_MAC_SYS_SUPPORT_STAT_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_STAT_MAC_SYS_SUPPORT_STAT_bf 0, 31, 0
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_STAT_MAC_SYS_SUPPORT_STAT_msk (0xffffffff)
#define     NFP_MAC_CSR_MAC_SYS_SUPPORT_STAT_MAC_SYS_SUPPORT_STAT_shf (0)


/*
 * Register: MacTimeStampNsec
 *   [31:0]    MacTimeStampNsec
 *
 * Name(s):
 * <base>.MacTimeStampNsec
 */
#define NFP_MAC_CSR_MAC_TS_NSEC                            0x001c
#define   NFP_MAC_CSR_MAC_TS_NSEC_MAC_TS_NSEC(x)             (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_TS_NSEC_MAC_TS_NSEC_of(x)          (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_TS_NSEC_MAC_TS_NSEC_bf           0, 31, 0
#define     NFP_MAC_CSR_MAC_TS_NSEC_MAC_TS_NSEC_msk          (0xffffffff)
#define     NFP_MAC_CSR_MAC_TS_NSEC_MAC_TS_NSEC_shf          (0)


/*
 * Register: MacTimeStampSec
 *   [31:0]    MacTimeStampSec
 *
 * Name(s):
 * <base>.MacTimeStampSec
 */
#define NFP_MAC_CSR_MAC_TS_SEC                             0x0020
#define   NFP_MAC_CSR_MAC_TS_SEC_MAC_TS_SEC(x)               (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_TS_SEC_MAC_TS_SEC_of(x)            (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_TS_SEC_MAC_TS_SEC_bf             0, 31, 0
#define     NFP_MAC_CSR_MAC_TS_SEC_MAC_TS_SEC_msk            (0xffffffff)
#define     NFP_MAC_CSR_MAC_TS_SEC_MAC_TS_SEC_shf            (0)


/*
 * Register: MacTimeStampIncr
 *   [19:16]   IncrNsec
 *   [15:0]    IncrFraction
 *
 * Name(s):
 * <base>.MacTimeStampIncr
 */
#define NFP_MAC_CSR_MAC_TS_INCR                            0x0024
#define   NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_NSEC(x)        (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_NSEC_of(x)     (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_NSEC_bf      0, 19, 16
#define     NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_NSEC_msk     (0xf)
#define     NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_NSEC_shf     (16)
#define   NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_FRAC(x)        (((x) & 0xffff) << 0)
#define   NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_FRAC_of(x)     (((x) >> 0) & 0xffff)
#define     NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_FRAC_bf      0, 15, 0
#define     NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_FRAC_msk     (0xffff)
#define     NFP_MAC_CSR_MAC_TS_INCR_MAC_TS_INCR_FRAC_shf     (0)


/*
 * Register: MacTimeStampSetNsec
 *   [31:0]    MacTimeStampSetNsec
 *
 * Name(s):
 * <base>.MacTimeStampSetNsec
 */
#define NFP_MAC_CSR_MAC_TS_SET_NSEC                        0x0028
#define   NFP_MAC_CSR_MAC_TS_SET_NSEC_MAC_TS_SET_NSEC(x)     (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_TS_SET_NSEC_MAC_TS_SET_NSEC_of(x)  (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_TS_SET_NSEC_MAC_TS_SET_NSEC_bf   0, 31, 0
#define     NFP_MAC_CSR_MAC_TS_SET_NSEC_MAC_TS_SET_NSEC_msk  (0xffffffff)
#define     NFP_MAC_CSR_MAC_TS_SET_NSEC_MAC_TS_SET_NSEC_shf  (0)


/*
 * Register: MacTimeStampSetSec
 *   [31:0]    MacTimeStampSetSec
 *
 * Name(s):
 * <base>.MacTimeStampSetSec
 */
#define NFP_MAC_CSR_MAC_TS_SET_SEC                         0x002c
#define   NFP_MAC_CSR_MAC_TS_SET_SEC_MAC_TS_SEC(x)           (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_TS_SET_SEC_MAC_TS_SEC_of(x)        (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_TS_SET_SEC_MAC_TS_SEC_bf         0, 31, 0
#define     NFP_MAC_CSR_MAC_TS_SET_SEC_MAC_TS_SEC_msk        (0xffffffff)
#define     NFP_MAC_CSR_MAC_TS_SET_SEC_MAC_TS_SEC_shf        (0)


/*
 * Register: MacTdmCycleWord3100
 *   [31:28]   MacTdmPortSlot7
 *   [27:24]   MacTdmPortSlot6
 *   [23:20]   MacTdmPortSlot5
 *   [19:16]   MacTdmPortSlot4
 *   [15:12]   MacTdmPortSlot3
 *   [11:8]    MacTdmPortSlot2
 *   [7:4]     MacTdmPortSlot1
 *   [3:0]     MacTdmPortSlot0
 *
 * Name(s):
 * <base>.MacTdm0CycleWord3100 <base>.MacTdm1CycleWord3100
 */
#define NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100               0x0030
#define NFP_MAC_CSR_MAC_TDM1_CYCLE_WORD_3100               0x0038
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT7(x) (((x) & 0xf) << 28)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT7_of(x) (((x) >> 28) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT7_bf 0, 31, 28
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT7_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT7_shf (28)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT6(x) (((x) & 0xf) << 24)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT6_of(x) (((x) >> 24) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT6_bf 0, 27, 24
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT6_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT6_shf (24)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT5(x) (((x) & 0xf) << 20)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT5_of(x) (((x) >> 20) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT5_bf 0, 23, 20
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT5_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT5_shf (20)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT4(x) (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT4_of(x) (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT4_bf 0, 19, 16
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT4_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT4_shf (16)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT3(x) (((x) & 0xf) << 12)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT3_of(x) (((x) >> 12) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT3_bf 0, 15, 12
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT3_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT3_shf (12)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT2(x) (((x) & 0xf) << 8)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT2_of(x) (((x) >> 8) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT2_bf 0, 11, 8
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT2_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT2_shf (8)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT1(x) (((x) & 0xf) << 4)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT1_of(x) (((x) >> 4) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT1_bf 0, 7, 4
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT1_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT1_shf (4)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT0(x) (((x) & 0xf) << 0)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT0_of(x) (((x) >> 0) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT0_bf 0, 3, 0
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT0_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_3100_MAC_TDM_PORT_SLOT0_shf (0)


/*
 * Register: MacTdmCycleWord4732
 *   [15:12]   MacTdmPortSlot11
 *   [11:8]    MacTdmPortSlot10
 *   [7:4]     MacTdmPortSlot9
 *   [3:0]     MacTdmPortSlot8
 *
 * Name(s):
 * <base>.MacTdm0CycleWord4732 <base>.MacTdm1CycleWord4732
 */
#define NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732               0x0034
#define NFP_MAC_CSR_MAC_TDM1_CYCLE_WORD_4732               0x003c
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT11(x) (((x) & 0xf) << 12)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT11_of(x) (((x) >> 12) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT11_bf 0, 15, 12
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT11_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT11_shf (12)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT10(x) (((x) & 0xf) << 8)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT10_of(x) (((x) >> 8) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT10_bf 0, 11, 8
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT10_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT10_shf (8)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT9(x) (((x) & 0xf) << 4)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT9_of(x) (((x) >> 4) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT9_bf 0, 7, 4
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT9_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT9_shf (4)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT8(x) (((x) & 0xf) << 0)
#define   NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT8_of(x) (((x) >> 0) & 0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT8_bf 0, 3, 0
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT8_msk (0xf)
#define     NFP_MAC_CSR_MAC_TDM0_CYCLE_WORD_4732_MAC_TDM_PORT_SLOT8_shf (0)


/*
 * Register: MacTdmMode0900
 *   [29:27]   MacTdmModePort9
 *   [26:24]   MacTdmModePort8
 *   [23:21]   MacTdmModePort7
 *   [20:18]   MacTdmModePort6
 *   [17:15]   MacTdmModePort5
 *   [14:12]   MacTdmModePort4
 *   [11:9]    MacTdmModePort3
 *   [8:6]     MacTdmModePort2
 *   [5:3]     MacTdmModePort1
 *   [2:0]     MacTdmModePort0
 *
 * Name(s):
 * <base>.MacTdm0Mode0900 <base>.MacTdm1Mode0900
 */
#define NFP_MAC_CSR_MAC_TDM0_MODE_0900                     0x0040
#define NFP_MAC_CSR_MAC_TDM1_MODE_0900                     0x0048
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9(x) (((x) & 7) << 27)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9_of(x) (((x) >> 27) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9_bf 0, 29, 27
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_9_shf (27)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8(x) (((x) & 7) << 24)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8_of(x) (((x) >> 24) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8_bf 0, 26, 24
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_8_shf (24)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7(x) (((x) & 7) << 21)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7_of(x) (((x) >> 21) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7_bf 0, 23, 21
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_7_shf (21)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6(x) (((x) & 7) << 18)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6_of(x) (((x) >> 18) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6_bf 0, 20, 18
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_6_shf (18)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5(x) (((x) & 7) << 15)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5_of(x) (((x) >> 15) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5_bf 0, 17, 15
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_5_shf (15)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4(x) (((x) & 7) << 12)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4_of(x) (((x) >> 12) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4_bf 0, 14, 12
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_4_shf (12)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3(x) (((x) & 7) << 9)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3_of(x) (((x) >> 9) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3_bf 0, 11, 9
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_3_shf (9)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2(x) (((x) & 7) << 6)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2_of(x) (((x) >> 6) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2_bf 0, 8, 6
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_2_shf (6)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1(x) (((x) & 7) << 3)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1_of(x) (((x) >> 3) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1_bf 0, 5, 3
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_1_shf (3)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0(x) (((x) & 7) << 0)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0_of(x) (((x) >> 0) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0_bf 0, 2, 0
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_0900_MAC_TDM_MODE_PORT_0_shf (0)


/*
 * Register: MacTdmMode1110Crc
 *   [27:16]   MacEgressPortCrcEn
 *   [5:3]     MacTdmModePort1
 *   [2:0]     MacTdmModePort0
 *
 * Name(s):
 * <base>.MacTdm0Mode1110CrcEn <base>.MacTdm1Mode1110CrcEn
 */
#define NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN              0x0044
#define NFP_MAC_CSR_MAC_TDM1_MODE_1110_CRC_EN              0x004c
#define   NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_EGRESS_PORT_CRC_EN(x) (((x) & 0xfff) << 16)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_EGRESS_PORT_CRC_EN_of(x) (((x) >> 16) & 0xfff)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_EGRESS_PORT_CRC_EN_bf 0, 27, 16
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_EGRESS_PORT_CRC_EN_msk (0xfff)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_EGRESS_PORT_CRC_EN_shf (16)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1(x) (((x) & 7) << 3)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1_of(x) (((x) >> 3) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1_bf 0, 5, 3
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_1_shf (3)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0(x) (((x) & 7) << 0)
#define   NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0_of(x) (((x) >> 0) & 7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0_BW (0)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0_BW (1)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0_BW (2)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0_BW (3)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0_BW (4)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0_bf 0, 2, 0
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0_msk (0x7)
#define     NFP_MAC_CSR_MAC_TDM0_MODE_1110_CRC_EN_MAC_TDM_MODE_PORT_0_shf (0)


/*
 * Register: MacPortChanAssign
 *   [29:26]   PortNumOfChannels2
 *   [25:20]   PortBaseChan2
 *   [19:16]   PortNumOfChannels1
 *   [15:10]   PortBaseChan1
 *   [9:6]     PortNumOfChannels0
 *   [5:0]     PortBaseChan0
 *
 * Name(s):
 * <base>.MacPort2to0ChanAssign <base>.MacPort5to3ChanAssign
 * <base>.MacPort8to6ChanAssign <base>.MacPort11to9ChanAssign
 * <base>.MacPort14to12ChanAssign <base>.MacPort17to15ChanAssign
 * <base>.MacPort20to18ChanAssign <base>.MacPort23to21ChanAssign
 * <base>.MacEgPort2to0ChanAssign <base>.MacEgPort5to3ChanAssign
 * <base>.MacEgPort8to6ChanAssign <base>.MacEgPort11to9ChanAssign
 * <base>.MacEgPort14to12ChanAssign <base>.MacEgPort17to15ChanAssign
 * <base>.MacEgPort20to18ChanAssign <base>.MacEgPort23to21ChanAssign
 */
#define NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN            0x0050
#define NFP_MAC_CSR_MAC_PORT_5_TO_3_CHAN_ASSIGN            0x0054
#define NFP_MAC_CSR_MAC_PORT_8_TO_6_CHAN_ASSIGN            0x0058
#define NFP_MAC_CSR_MAC_PORT_11_TO_9_CHAN_ASSIGN           0x005c
#define NFP_MAC_CSR_MAC_PORT_14_TO_12_CHAN_ASSIGN          0x0060
#define NFP_MAC_CSR_MAC_PORT_17_TO_15_CHAN_ASSIGN          0x0064
#define NFP_MAC_CSR_MAC_PORT_20_TO_18_CHAN_ASSIGN          0x0068
#define NFP_MAC_CSR_MAC_PORT_23_TO_21_CHAN_ASSIGN          0x006c
#define NFP_MAC_CSR_MAC_EG_PORT_2_TO_0_CHAN_ASSIGN         0x0240
#define NFP_MAC_CSR_MAC_EG_PORT_5_TO_3_CHAN_ASSIGN         0x0244
#define NFP_MAC_CSR_MAC_EG_PORT_8_TO_6_CHAN_ASSIGN         0x0248
#define NFP_MAC_CSR_MAC_EG_PORT_11_TO_9_CHAN_ASSIGN        0x024c
#define NFP_MAC_CSR_MAC_EG_PORT_14_TO_12_CHAN_ASSIGN       0x0250
#define NFP_MAC_CSR_MAC_EG_PORT_17_TO_15_CHAN_ASSIGN       0x0254
#define NFP_MAC_CSR_MAC_EG_PORT_20_TO_18_CHAN_ASSIGN       0x0258
#define NFP_MAC_CSR_MAC_EG_PORT_23_TO_21_CHAN_ASSIGN       0x025c
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS2(x) (((x) & 0xf) << 26)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS2_of(x) (((x) >> 26) & 0xf)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS2_bf 0, 29, 26
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS2_msk (0xf)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS2_shf (26)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN2(x) (((x) & 0x3f) << 20)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN2_of(x) (((x) >> 20) & 0x3f)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN2_bf 0, 25, 20
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN2_msk (0x3f)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN2_shf (20)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS1(x) (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS1_of(x) (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS1_bf 0, 19, 16
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS1_msk (0xf)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS1_shf (16)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN1(x) (((x) & 0x3f) << 10)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN1_of(x) (((x) >> 10) & 0x3f)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN1_bf 0, 15, 10
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN1_msk (0x3f)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN1_shf (10)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS0(x) (((x) & 0xf) << 6)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS0_of(x) (((x) >> 6) & 0xf)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS0_bf 0, 9, 6
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS0_msk (0xf)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_NUM_OF_CHANS0_shf (6)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN0(x) (((x) & 0x3f) << 0)
#define   NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN0_of(x) (((x) >> 0) & 0x3f)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN0_bf 0, 5, 0
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN0_msk (0x3f)
#define     NFP_MAC_CSR_MAC_PORT_2_TO_0_CHAN_ASSIGN_PORT_BASE_CHAN0_shf (0)


/*
 * Register: MacPrePendCtl1
 *   [31:28]   EGSkipOctetsPort3
 *   [27:24]   IGSkipOctetsPort3
 *   [23:20]   EGSkipOctetsPort2
 *   [19:16]   IGSkipOctetsPort2
 *   [15:12]   EGSkipOctetsPort1
 *   [11:8]    IGSkipOctetsPort1
 *   [7:4]     EGSkipOctetsPort0
 *   [3:0]     IGSkipOctetsPort0
 *
 * Name(s):
 * <base>.MacPrePendCtl03to00 <base>.MacPrePendCtl15to12
 */
#define NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00               0x0070
#define NFP_MAC_CSR_MAC_PREPEND_CTL_15_TO_12               0x007c
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_3(x) (((x) & 0xf) << 28)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_3_of(x) (((x) >> 28) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_3_bf 0, 31, 28
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_3_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_3_shf (28)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_3(x) (((x) & 0xf) << 24)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_3_of(x) (((x) >> 24) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_3_bf 0, 27, 24
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_3_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_3_shf (24)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_2(x) (((x) & 0xf) << 20)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_2_of(x) (((x) >> 20) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_2_bf 0, 23, 20
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_2_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_2_shf (20)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_2(x) (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_2_of(x) (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_2_bf 0, 19, 16
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_2_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_2_shf (16)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_1(x) (((x) & 0xf) << 12)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_1_of(x) (((x) >> 12) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_1_bf 0, 15, 12
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_1_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_1_shf (12)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_1(x) (((x) & 0xf) << 8)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_1_of(x) (((x) >> 8) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_1_bf 0, 11, 8
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_1_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_1_shf (8)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_0(x) (((x) & 0xf) << 4)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_0_of(x) (((x) >> 4) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_0_bf 0, 7, 4
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_0_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_EG_SKIP_OCTETS_PORT_0_shf (4)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_0(x) (((x) & 0xf) << 0)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_0_of(x) (((x) >> 0) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_0_bf 0, 3, 0
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_0_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_03_TO_00_IG_SKIP_OCTETS_PORT_0_shf (0)


/*
 * Register: MacPrePendCtl2
 *   [31:28]   EGSkipOctetsPort7
 *   [27:24]   IGSkipOctetsPort7
 *   [23:20]   EGSkipOctetsPort6
 *   [19:16]   IGSkipOctetsPort6
 *   [15:12]   EGSkipOctetsPort5
 *   [11:8]    IGSkipOctetsPort5
 *   [7:4]     EGSkipOctetsPort4
 *   [3:0]     IGSkipOctetsPort4
 *
 * Name(s):
 * <base>.MacPrePendCtl07to04 <base>.MacPrePendCtl19to16
 */
#define NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04               0x0074
#define NFP_MAC_CSR_MAC_PREPEND_CTL_19_TO_16               0x0080
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_7(x) (((x) & 0xf) << 28)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_7_of(x) (((x) >> 28) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_7_bf 0, 31, 28
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_7_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_7_shf (28)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_7(x) (((x) & 0xf) << 24)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_7_of(x) (((x) >> 24) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_7_bf 0, 27, 24
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_7_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_7_shf (24)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_6(x) (((x) & 0xf) << 20)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_6_of(x) (((x) >> 20) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_6_bf 0, 23, 20
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_6_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_6_shf (20)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_6(x) (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_6_of(x) (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_6_bf 0, 19, 16
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_6_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_6_shf (16)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_5(x) (((x) & 0xf) << 12)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_5_of(x) (((x) >> 12) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_5_bf 0, 15, 12
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_5_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_5_shf (12)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_5(x) (((x) & 0xf) << 8)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_5_of(x) (((x) >> 8) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_5_bf 0, 11, 8
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_5_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_5_shf (8)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_4(x) (((x) & 0xf) << 4)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_4_of(x) (((x) >> 4) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_4_bf 0, 7, 4
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_4_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_EG_SKIP_OCTETS_PORT_4_shf (4)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_4(x) (((x) & 0xf) << 0)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_4_of(x) (((x) >> 0) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_4_bf 0, 3, 0
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_4_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_07_TO_04_IG_SKIP_OCTETS_PORT_4_shf (0)


/*
 * Register: MacPrePendCtl3
 *   [31:28]   EGSkipOctetsPort11
 *   [27:24]   IGSkipOctetsPort11
 *   [23:20]   EGSkipOctetsPort10
 *   [19:16]   IGSkipOctetsPort10
 *   [15:12]   EGSkipOctetsPort9
 *   [11:8]    IGSkipOctetsPort9
 *   [7:4]     EGSkipOctetsPort8
 *   [3:0]     IGSkipOctetsPort8
 *
 * Name(s):
 * <base>.MacPrePendCtl11to08 <base>.MacPrePendCtl23to20
 */
#define NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08               0x0078
#define NFP_MAC_CSR_MAC_PREPEND_CTL_23_TO_20               0x0084
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_11(x) (((x) & 0xf) << 28)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_11_of(x) (((x) >> 28) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_11_bf 0, 31, 28
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_11_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_11_shf (28)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_11(x) (((x) & 0xf) << 24)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_11_of(x) (((x) >> 24) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_11_bf 0, 27, 24
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_11_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_11_shf (24)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_10(x) (((x) & 0xf) << 20)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_10_of(x) (((x) >> 20) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_10_bf 0, 23, 20
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_10_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_10_shf (20)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_10(x) (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_10_of(x) (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_10_bf 0, 19, 16
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_10_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_10_shf (16)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_9(x) (((x) & 0xf) << 12)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_9_of(x) (((x) >> 12) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_9_bf 0, 15, 12
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_9_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_9_shf (12)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_9(x) (((x) & 0xf) << 8)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_9_of(x) (((x) >> 8) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_9_bf 0, 11, 8
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_9_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_9_shf (8)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_8(x) (((x) & 0xf) << 4)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_8_of(x) (((x) >> 4) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_8_bf 0, 7, 4
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_8_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_EG_SKIP_OCTETS_PORT_8_shf (4)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_8(x) (((x) & 0xf) << 0)
#define   NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_8_of(x) (((x) >> 0) & 0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_8_bf 0, 3, 0
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_8_msk (0xf)
#define     NFP_MAC_CSR_MAC_PREPEND_CTL_11_TO_08_IG_SKIP_OCTETS_PORT_8_shf (0)


/*
 * Register: MacPrePendDsaCtl1
 *   [31:30]   DsaTagModePort15
 *   [29:28]   DsaTagModePort14
 *   [27:26]   DsaTagModePort13
 *   [25:24]   DsaTagModePort12
 *   [23:22]   DsaTagModePort11
 *   [21:20]   DsaTagModePort10
 *   [19:18]   DsaTagModePort9
 *   [17:16]   DsaTagModePort8
 *   [15:14]   DsaTagModePort7
 *   [13:12]   DsaTagModePort6
 *   [11:10]   DsaTagModePort5
 *   [9:8]     DsaTagModePort4
 *   [7:6]     DsaTagModePort3
 *   [5:4]     DsaTagModePort2
 *   [3:2]     DsaTagModePort1
 *   [1:0]     DsaTagModePort0
 *
 * Name(s):
 * <base>.MacPrePendDsaCtl15to00 <base>.MacEgPrePendDsaCtl15to00
 */
#define NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00                   0x0088
#define NFP_MAC_CSR_MAC_EG_DSA_CTL_15_TO_00                0x01cc
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_15(x) (((x) & 3) << 30)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_15_of(x) (((x) >> 30) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_15_bf 0, 31, 30
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_15_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_15_shf (30)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_14(x) (((x) & 3) << 28)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_14_of(x) (((x) >> 28) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_14_bf 0, 29, 28
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_14_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_14_shf (28)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_13(x) (((x) & 3) << 26)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_13_of(x) (((x) >> 26) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_13_bf 0, 27, 26
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_13_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_13_shf (26)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_12(x) (((x) & 3) << 24)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_12_of(x) (((x) >> 24) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_12_bf 0, 25, 24
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_12_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_12_shf (24)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_11(x) (((x) & 3) << 22)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_11_of(x) (((x) >> 22) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_11_bf 0, 23, 22
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_11_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_11_shf (22)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_10(x) (((x) & 3) << 20)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_10_of(x) (((x) >> 20) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_10_bf 0, 21, 20
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_10_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_10_shf (20)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_9(x) (((x) & 3) << 18)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_9_of(x) (((x) >> 18) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_9_bf 0, 19, 18
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_9_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_9_shf (18)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_8(x) (((x) & 3) << 16)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_8_of(x) (((x) >> 16) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_8_bf 0, 17, 16
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_8_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_8_shf (16)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_7(x) (((x) & 3) << 14)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_7_of(x) (((x) >> 14) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_7_bf 0, 15, 14
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_7_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_7_shf (14)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_6(x) (((x) & 3) << 12)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_6_of(x) (((x) >> 12) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_6_bf 0, 13, 12
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_6_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_6_shf (12)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_5(x) (((x) & 3) << 10)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_5_of(x) (((x) >> 10) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_5_bf 0, 11, 10
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_5_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_5_shf (10)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_4(x) (((x) & 3) << 8)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_4_of(x) (((x) >> 8) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_4_bf 0, 9, 8
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_4_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_4_shf (8)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_3(x) (((x) & 3) << 6)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_3_of(x) (((x) >> 6) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_3_bf 0, 7, 6
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_3_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_3_shf (6)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_2(x) (((x) & 3) << 4)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_2_of(x) (((x) >> 4) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_2_bf 0, 5, 4
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_2_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_2_shf (4)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_1(x) (((x) & 3) << 2)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_1_of(x) (((x) >> 2) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_1_bf 0, 3, 2
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_1_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_1_shf (2)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_0(x) (((x) & 3) << 0)
#define   NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_0_of(x) (((x) >> 0) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_0_Mode (0)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_0_Mode (1)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_0_Mode (2)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_0_Mode (3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_0_bf 0, 1, 0
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_0_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_15_TO_00_DSA_TAG_MODE_PORT_0_shf (0)


/*
 * Register: MacPrePendDsaCtl2
 *   [31:20]   Rfu
 *   [19:18]   DsaTagModeLkCore1
 *   [17:16]   DsaTagModeLkCore0
 *   [15:14]   DsaTagModePort23
 *   [13:12]   DsaTagModePort22
 *   [11:10]   DsaTagModePort21
 *   [9:8]     DsaTagModePort20
 *   [7:6]     DsaTagModePort19
 *   [5:4]     DsaTagModePort18
 *   [3:2]     DsaTagModePort17
 *   [1:0]     DsaTagModePort16
 *
 * Name(s):
 * <base>.MacPrePendDsaCtlLkand23to16 <base>.MacEgPrePendDsaCtlLkand23to16
 */
#define NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16                   0x008c
#define NFP_MAC_CSR_MAC_EG_DSA_CTL_23_TO_16                0x01d0
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_RFU(x)            (((x) & 0xfff) << 20)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_RFU_of(x)         (((x) >> 20) & 0xfff)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_RFU_bf          0, 31, 20
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_RFU_msk         (0xfff)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_RFU_shf         (20)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_1(x) (((x) & 3) << 18)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_1_of(x) (((x) >> 18) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_1_bf 0, 19, 18
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_1_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_1_shf (18)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_0(x) (((x) & 3) << 16)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_0_of(x) (((x) >> 16) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_0_bf 0, 17, 16
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_0_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_LK_CORE_0_shf (16)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_23(x) (((x) & 3) << 14)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_23_of(x) (((x) >> 14) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_23_bf 0, 15, 14
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_23_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_23_shf (14)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_22(x) (((x) & 3) << 12)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_22_of(x) (((x) >> 12) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_22_bf 0, 13, 12
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_22_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_22_shf (12)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_21(x) (((x) & 3) << 10)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_21_of(x) (((x) >> 10) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_21_bf 0, 11, 10
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_21_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_21_shf (10)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_20(x) (((x) & 3) << 8)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_20_of(x) (((x) >> 8) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_20_bf 0, 9, 8
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_20_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_20_shf (8)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_19(x) (((x) & 3) << 6)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_19_of(x) (((x) >> 6) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_19_bf 0, 7, 6
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_19_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_19_shf (6)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_18(x) (((x) & 3) << 4)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_18_of(x) (((x) >> 4) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_18_bf 0, 5, 4
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_18_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_18_shf (4)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_17(x) (((x) & 3) << 2)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_17_of(x) (((x) >> 2) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_17_bf 0, 3, 2
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_17_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_17_shf (2)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_16(x) (((x) & 3) << 0)
#define   NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_16_of(x) (((x) >> 0) & 3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_16_Mode (0)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_16_Mode (1)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_16_Mode (2)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_16_Mode (3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_16_bf 0, 1, 0
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_16_msk (0x3)
#define     NFP_MAC_CSR_MAC_DSA_CTL_23_TO_16_DSA_TAG_MODE_PORT_16_shf (0)


/*
 * Register: MacInterlakenCtl1
 *   [31:29]   LkBurstMaxCore1
 *   [28:22]   LkNumChannelsUpper64
 *   [21:16]   LkBaseChannelUpper64
 *   [15:13]   LkBurstMaxCore0
 *   [12:6]    LkNumChannelsLower64
 *   [5:0]     LkBaseChannelLower64
 *
 * Name(s):
 * <base>.MacInterlakenCtl1
 */
#define NFP_MAC_CSR_INTERLAKEN_CTL_1                       0x0090
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1(x) (((x) & 7) << 29)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_of(x) (((x) >> 29) & 7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_BMx (0)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_BMx (1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_BMx (2)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_BMx (3)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_BMx (4)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_BMx (5)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_BMx (6)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_BMx (7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_bf 0, 31, 29
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_msk (0x7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_1_shf (29)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_UPPER_64(x) (((x) & 0x7f) << 22)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_UPPER_64_of(x) (((x) >> 22) & 0x7f)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_UPPER_64_bf 0, 28, 22
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_UPPER_64_msk (0x7f)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_UPPER_64_shf (22)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_UPPER_64(x) (((x) & 0x3f) << 16)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_UPPER_64_of(x) (((x) >> 16) & 0x3f)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_UPPER_64_bf 0, 21, 16
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_UPPER_64_msk (0x3f)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_UPPER_64_shf (16)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0(x) (((x) & 7) << 13)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_of(x) (((x) >> 13) & 7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_BMx (0)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_BMx (1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_BMx (2)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_BMx (3)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_BMx (4)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_BMx (5)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_BMx (6)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_BMx (7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_bf 0, 15, 13
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_msk (0x7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BURSTMAX_CORE_0_shf (13)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_LOWER_64(x) (((x) & 0x7f) << 6)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_LOWER_64_of(x) (((x) >> 6) & 0x7f)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_LOWER_64_bf 0, 12, 6
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_LOWER_64_msk (0x7f)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_NUM_CHANNELS_LOWER_64_shf (6)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_LOWER_64(x) (((x) & 0x3f) << 0)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_LOWER_64_of(x) (((x) >> 0) & 0x3f)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_LOWER_64_bf 0, 5, 0
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_LOWER_64_msk (0x3f)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_1_LK_BASE_CHANNEL_LOWER_64_shf (0)


/*
 * Register: MacInterlakenCtl2
 *   [31:23]   Rfu2
 *   [22]      LkNbiChanSwapEn1
 *   [21]      IgOobFcSelCore1
 *   [20]      EgOobFcSelCore1
 *   [19:17]   EgTdmModeLkCore1
 *   [16]      EgAtomicLkCore1
 *   [15:7]    Rfu
 *   [6]       LkNbiChanSwapEn0
 *   [5]       IgOobFcSelCore0
 *   [4]       EgOobFcSelCore0
 *   [3:1]     EgTdmModeLkCore0
 *   [0]       EgAtomicLkCore0
 *
 * Name(s):
 * <base>.MacInterlakenCtl2
 */
#define NFP_MAC_CSR_INTERLAKEN_CTL_2                       0x0094
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU2(x)               (((x) & 0x1ff) << 23)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU2_of(x)            (((x) >> 23) & 0x1ff)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU2_bf             0, 31, 23
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU2_msk            (0x1ff)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU2_shf            (23)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_LK_NBI_CHAN_SWAP_EN_1 (1 << 22)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_LK_NBI_CHAN_SWAP_EN_1_bf 0, 22, 22
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_LK_NBI_CHAN_SWAP_EN_1_msk (0x1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_LK_NBI_CHAN_SWAP_EN_1_bit (22)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_IG_OOB_FC_SEL_CORE_1  (1 << 21)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_IG_OOB_FC_SEL_CORE_1_bf 0, 21, 21
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_IG_OOB_FC_SEL_CORE_1_msk (0x1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_IG_OOB_FC_SEL_CORE_1_bit (21)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_OOB_FC_SEL_CORE_1  (1 << 20)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_OOB_FC_SEL_CORE_1_bf 0, 20, 20
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_OOB_FC_SEL_CORE_1_msk (0x1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_OOB_FC_SEL_CORE_1_bit (20)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_1(x) (((x) & 7) << 17)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_1_of(x) (((x) >> 17) & 7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_1_bf 0, 19, 17
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_1_msk (0x7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_1_shf (17)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_ATOMIC_LK_CORE_1   (1 << 16)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_ATOMIC_LK_CORE_1_bf 0, 16, 16
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_ATOMIC_LK_CORE_1_msk (0x1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_ATOMIC_LK_CORE_1_bit (16)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU(x)                (((x) & 0x1ff) << 7)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU_of(x)             (((x) >> 7) & 0x1ff)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU_bf              0, 15, 7
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU_msk             (0x1ff)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_RFU_shf             (7)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_LK_NBI_CHAN_SWAP_EN_0 (1 << 6)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_LK_NBI_CHAN_SWAP_EN_0_bf 0, 6, 6
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_LK_NBI_CHAN_SWAP_EN_0_msk (0x1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_LK_NBI_CHAN_SWAP_EN_0_bit (6)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_IG_OOB_FC_SEL_CORE_0  (1 << 5)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_IG_OOB_FC_SEL_CORE_0_bf 0, 5, 5
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_IG_OOB_FC_SEL_CORE_0_msk (0x1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_IG_OOB_FC_SEL_CORE_0_bit (5)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_OOB_FC_SEL_CORE_0  (1 << 4)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_OOB_FC_SEL_CORE_0_bf 0, 4, 4
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_OOB_FC_SEL_CORE_0_msk (0x1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_OOB_FC_SEL_CORE_0_bit (4)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_0(x) (((x) & 7) << 1)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_0_of(x) (((x) >> 1) & 7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_0_bf 0, 3, 1
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_0_msk (0x7)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_TDM_MODE_LK_CORE_0_shf (1)
#define   NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_ATOMIC_LK_CORE_0   (1 << 0)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_ATOMIC_LK_CORE_0_bf 0, 0, 0
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_ATOMIC_LK_CORE_0_msk (0x1)
#define     NFP_MAC_CSR_INTERLAKEN_CTL_2_EG_ATOMIC_LK_CORE_0_bit (0)


/*
 * Register: EgBufferCreditPoolCount
 *   [31]      EgBufferLinklistReady
 *   [29:16]   EgBufferCreditCount1
 *   [13:0]    EgBufferCreditCount
 *
 * Name(s):
 * <base>.EgBufferCreditPoolCount
 */
#define NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT            0x0098
#define   NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_LINKLIST_READY (1 << 31)
#define     NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_LINKLIST_READY_bf 0, 31, 31
#define     NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_LINKLIST_READY_msk (0x1)
#define     NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_LINKLIST_READY_bit (31)
#define   NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT1(x) (((x) & 0x3fff) << 16)
#define   NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT1_of(x) (((x) >> 16) & 0x3fff)
#define     NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT1_bf 0, 29, 16
#define     NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT1_msk (0x3fff)
#define     NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT1_shf (16)
#define   NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT(x) (((x) & 0x3fff) << 0)
#define   NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT_of(x) (((x) >> 0) & 0x3fff)
#define     NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT_bf 0, 13, 0
#define     NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT_msk (0x3fff)
#define     NFP_MAC_CSR_EG_BUFFER_CREDIT_POOL_COUNT_EG_BUFFER_CREDIT_COUNT_shf (0)


/*
 * Register: TxMpbCreditInit
 *   [31:20]   Rfu
 *   [19:16]   TxMpbCreditDataInit
 *   [15:12]   Rfu2
 *   [11:6]    TxMpbCreditMaxPktInit
 *   [5:0]     TxMpbCreditPktInit
 *
 * Name(s):
 * <base>.TxMpbCreditInit
 */
#define NFP_MAC_CSR_TX_MPB_CREDIT_INIT                     0x009c
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU(x)              (((x) & 0xfff) << 20)
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU_of(x)           (((x) >> 20) & 0xfff)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU_bf            0, 31, 20
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU_msk           (0xfff)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU_shf           (20)
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_DATA_INIT(x) (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_DATA_INIT_of(x) (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_DATA_INIT_bf 0, 19, 16
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_DATA_INIT_msk (0xf)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_DATA_INIT_shf (16)
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU2(x)             (((x) & 0xf) << 12)
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU2_of(x)          (((x) >> 12) & 0xf)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU2_bf           0, 15, 12
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU2_msk          (0xf)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_RFU2_shf          (12)
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_MAX_PKT_INIT(x) (((x) & 0x3f) << 6)
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_MAX_PKT_INIT_of(x) (((x) >> 6) & 0x3f)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_MAX_PKT_INIT_bf 0, 11, 6
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_MAX_PKT_INIT_msk (0x3f)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_MAX_PKT_INIT_shf (6)
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_PKT_INIT(x) (((x) & 0x3f) << 0)
#define   NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_PKT_INIT_of(x) (((x) >> 0) & 0x3f)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_PKT_INIT_bf 0, 5, 0
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_PKT_INIT_msk (0x3f)
#define     NFP_MAC_CSR_TX_MPB_CREDIT_INIT_TX_MPB_CREDIT_PKT_INIT_shf (0)


/*
 * Register: IgBufferCreditPoolCount
 *   [31]      IgBufferLinklistReady
 *   [29:16]   IgBufferCreditCount1
 *   [13:0]    IgBufferCreditCount
 *
 * Name(s):
 * <base>.IgBufferCreditPoolCount
 */
#define NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT            0x00a0
#define   NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_LINKLIST_READY (1 << 31)
#define     NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_LINKLIST_READY_bf 0, 31, 31
#define     NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_LINKLIST_READY_msk (0x1)
#define     NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_LINKLIST_READY_bit (31)
#define   NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT1(x) (((x) & 0x3fff) << 16)
#define   NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT1_of(x) (((x) >> 16) & 0x3fff)
#define     NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT1_bf 0, 29, 16
#define     NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT1_msk (0x3fff)
#define     NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT1_shf (16)
#define   NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT(x) (((x) & 0x3fff) << 0)
#define   NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT_of(x) (((x) >> 0) & 0x3fff)
#define     NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT_bf 0, 13, 0
#define     NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT_msk (0x3fff)
#define     NFP_MAC_CSR_IG_BUFFER_CREDIT_POOL_COUNT_IG_BUFFER_CREDIT_COUNT_shf (0)


/*
 * Register: RxMpbCreditInit
 *   [31:30]   Rfu2
 *   [29:16]   RxMpbCreditDataInit
 *   [15:14]   Rfu
 *   [13:0]    RxMpbCreditBufInit
 *
 * Name(s):
 * <base>.RxMpbCreditInit
 */
#define NFP_MAC_CSR_RX_MPB_CREDIT_INIT                     0x00a4
#define   NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU2(x)             (((x) & 3) << 30)
#define   NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU2_of(x)          (((x) >> 30) & 3)
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU2_bf           0, 31, 30
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU2_msk          (0x3)
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU2_shf          (30)
#define   NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_DATA_INIT(x) (((x) & 0x3fff) << 16)
#define   NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_DATA_INIT_of(x) (((x) >> 16) & 0x3fff)
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_DATA_INIT_bf 0, 29, 16
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_DATA_INIT_msk (0x3fff)
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_DATA_INIT_shf (16)
#define   NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU(x)              (((x) & 3) << 14)
#define   NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU_of(x)           (((x) >> 14) & 3)
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU_bf            0, 15, 14
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU_msk           (0x3)
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RFU_shf           (14)
#define   NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_BUF_INIT(x) (((x) & 0x3fff) << 0)
#define   NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_BUF_INIT_of(x) (((x) >> 0) & 0x3fff)
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_BUF_INIT_bf 0, 13, 0
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_BUF_INIT_msk (0x3fff)
#define     NFP_MAC_CSR_RX_MPB_CREDIT_INIT_RX_MPB_CREDIT_BUF_INIT_shf (0)


/*
 * Register: TDMRateCreditInit
 *   [31:24]   TDM100GECreditInit
 *   [23:16]   TDM40GECreditInit
 *   [15:8]    TDM10GECreditInit
 *   [7:0]     TDM1GECreditInit
 *
 * Name(s):
 * <base>.MacTdmRateCreditInit
 */
#define NFP_MAC_CSR_TDM_RATE_CREDIT_INIT                   0x00a8
#define   NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_100GE_CREDIT_INIT(x) (((x) & 0xff) << 24)
#define   NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_100GE_CREDIT_INIT_of(x) (((x) >> 24) & 0xff)
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_100GE_CREDIT_INIT_bf 0, 31, 24
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_100GE_CREDIT_INIT_msk (0xff)
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_100GE_CREDIT_INIT_shf (24)
#define   NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_40GE_CREDIT_INIT(x) (((x) & 0xff) << 16)
#define   NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_40GE_CREDIT_INIT_of(x) (((x) >> 16) & 0xff)
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_40GE_CREDIT_INIT_bf 0, 23, 16
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_40GE_CREDIT_INIT_msk (0xff)
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_40GE_CREDIT_INIT_shf (16)
#define   NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_10GE_CREDIT_INIT(x) (((x) & 0xff) << 8)
#define   NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_10GE_CREDIT_INIT_of(x) (((x) >> 8) & 0xff)
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_10GE_CREDIT_INIT_bf 0, 15, 8
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_10GE_CREDIT_INIT_msk (0xff)
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_10GE_CREDIT_INIT_shf (8)
#define   NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_1GE_CREDIT_INIT(x) (((x) & 0xff) << 0)
#define   NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_1GE_CREDIT_INIT_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_1GE_CREDIT_INIT_bf 0, 7, 0
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_1GE_CREDIT_INIT_msk (0xff)
#define     NFP_MAC_CSR_TDM_RATE_CREDIT_INIT_TDM_1GE_CREDIT_INIT_shf (0)


/*
 * Register: MacInterruptErrStatus0
 *   [31:0]    MacInterruptErrStatus1
 *
 * Name(s):
 * <base>.MacInterruptErrStatus0
 */
#define NFP_MAC_CSR_MAC_INTR_ERR_STATUS_0                  0x00ac
#define   NFP_MAC_CSR_MAC_INTR_ERR_STATUS_0_MAC_INTERRUPT_ERR_STATUS_1(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_INTR_ERR_STATUS_0_MAC_INTERRUPT_ERR_STATUS_1_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_0_MAC_INTERRUPT_ERR_STATUS_1_bf 0, 31, 0
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_0_MAC_INTERRUPT_ERR_STATUS_1_msk (0xffffffff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_0_MAC_INTERRUPT_ERR_STATUS_1_shf (0)


/*
 * Register: MacInterruptErrStatus1
 *   [31:24]   Rfu
 *   [23:12]   MacLinTrainingInthy0
 *   [11:0]    MacLinTrainingInthy1
 *
 * Name(s):
 * <base>.MacInterruptErrStatus1
 */
#define NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1                  0x00b0
#define   NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_RFU(x)           (((x) & 0xff) << 24)
#define   NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_RFU_of(x)        (((x) >> 24) & 0xff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_RFU_bf         0, 31, 24
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_RFU_msk        (0xff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_RFU_shf        (24)
#define   NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY0(x) (((x) & 0xfff) << 12)
#define   NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY0_of(x) (((x) >> 12) & 0xfff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY0_bf 0, 23, 12
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY0_msk (0xfff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY0_shf (12)
#define   NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY1(x) (((x) & 0xfff) << 0)
#define   NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY1_of(x) (((x) >> 0) & 0xfff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY1_bf 0, 11, 0
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY1_msk (0xfff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_STATUS_1_MAC_LINKT_TRAINING_INT_HY1_shf (0)


/*
 * Register: MacInterruptErrEn0
 *   [31:0]    MacInterruptErrEn0
 *
 * Name(s):
 * <base>.MacInterruptErrEn0
 */
#define NFP_MAC_CSR_MAC_INTR_ERR_EN_0                      0x00b4
#define   NFP_MAC_CSR_MAC_INTR_ERR_EN_0_MAC_INTERRUPT_ERR_EN_0(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_INTR_ERR_EN_0_MAC_INTERRUPT_ERR_EN_0_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_EN_0_MAC_INTERRUPT_ERR_EN_0_bf 0, 31, 0
#define     NFP_MAC_CSR_MAC_INTR_ERR_EN_0_MAC_INTERRUPT_ERR_EN_0_msk (0xffffffff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_EN_0_MAC_INTERRUPT_ERR_EN_0_shf (0)


/*
 * Register: MacInterruptErrEn1
 *   [31:0]    MacInterruptErrEn1
 *
 * Name(s):
 * <base>.MacInterruptErrEn1
 */
#define NFP_MAC_CSR_MAC_INTR_ERR_EN_1                      0x00b8
#define   NFP_MAC_CSR_MAC_INTR_ERR_EN_1_MAC_INTERRUPT_ERR_EN_1(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_INTR_ERR_EN_1_MAC_INTERRUPT_ERR_EN_1_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_EN_1_MAC_INTERRUPT_ERR_EN_1_bf 0, 31, 0
#define     NFP_MAC_CSR_MAC_INTR_ERR_EN_1_MAC_INTERRUPT_ERR_EN_1_msk (0xffffffff)
#define     NFP_MAC_CSR_MAC_INTR_ERR_EN_1_MAC_INTERRUPT_ERR_EN_1_shf (0)


/*
 * Register: MacLiveStatus0
 *   [31:0]    MacLiveStatus0
 *
 * Name(s):
 * <base>.MacLiveStatus0
 */
#define NFP_MAC_CSR_MAC_LIVE_STATUS_0                      0x00bc
#define   NFP_MAC_CSR_MAC_LIVE_STATUS_0_MAC_LIVE_STATUS_0(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_LIVE_STATUS_0_MAC_LIVE_STATUS_0_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_LIVE_STATUS_0_MAC_LIVE_STATUS_0_bf 0, 31, 0
#define     NFP_MAC_CSR_MAC_LIVE_STATUS_0_MAC_LIVE_STATUS_0_msk (0xffffffff)
#define     NFP_MAC_CSR_MAC_LIVE_STATUS_0_MAC_LIVE_STATUS_0_shf (0)


/*
 * Register: MacLiveStatus1
 *   [31:0]    MacLiveStatus1
 *
 * Name(s):
 * <base>.MacLiveStatus1
 */
#define NFP_MAC_CSR_MAC_LIVE_STATUS_1                      0x00c0
#define   NFP_MAC_CSR_MAC_LIVE_STATUS_1_MAC_LIVE_STATUS_1(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_MAC_LIVE_STATUS_1_MAC_LIVE_STATUS_1_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_MAC_LIVE_STATUS_1_MAC_LIVE_STATUS_1_bf 0, 31, 0
#define     NFP_MAC_CSR_MAC_LIVE_STATUS_1_MAC_LIVE_STATUS_1_msk (0xffffffff)
#define     NFP_MAC_CSR_MAC_LIVE_STATUS_1_MAC_LIVE_STATUS_1_shf (0)


/*
 * Register: MacChanRdAddr
 *   [31:23]   Rfu1
 *   [22:16]   IgChanRdAddr
 *   [15:7]    Rfu0
 *   [6:0]     EgChanRdAddr
 *
 * Name(s):
 * <base>.MacChanRdAddr
 */
#define NFP_MAC_CSR_MAC_CHAN_RD_ADDR                       0x00c4
#define   NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU1(x)               (((x) & 0x1ff) << 23)
#define   NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU1_of(x)            (((x) >> 23) & 0x1ff)
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU1_bf             0, 31, 23
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU1_msk            (0x1ff)
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU1_shf            (23)
#define   NFP_MAC_CSR_MAC_CHAN_RD_ADDR_IG_CHAN_RD_ADDR(x)    (((x) & 0x7f) << 16)
#define   NFP_MAC_CSR_MAC_CHAN_RD_ADDR_IG_CHAN_RD_ADDR_of(x) (((x) >> 16) & 0x7f)
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_IG_CHAN_RD_ADDR_bf  0, 22, 16
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_IG_CHAN_RD_ADDR_msk (0x7f)
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_IG_CHAN_RD_ADDR_shf (16)
#define   NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU0(x)               (((x) & 0x1ff) << 7)
#define   NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU0_of(x)            (((x) >> 7) & 0x1ff)
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU0_bf             0, 15, 7
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU0_msk            (0x1ff)
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_RFU0_shf            (7)
#define   NFP_MAC_CSR_MAC_CHAN_RD_ADDR_EG_CHAN_RD_ADDR(x)    (((x) & 0x7f) << 0)
#define   NFP_MAC_CSR_MAC_CHAN_RD_ADDR_EG_CHAN_RD_ADDR_of(x) (((x) >> 0) & 0x7f)
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_EG_CHAN_RD_ADDR_bf  0, 6, 0
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_EG_CHAN_RD_ADDR_msk (0x7f)
#define     NFP_MAC_CSR_MAC_CHAN_RD_ADDR_EG_CHAN_RD_ADDR_shf (0)


/*
 * Register: MacChanBufCount
 *   [31:27]   Rfu1
 *   [26:16]   IgChanRdBufCnt
 *   [15:11]   Rfu0
 *   [10:0]    EgChanRdBufCnt
 *
 * Name(s):
 * <base>.MacChanBufCount
 */
#define NFP_MAC_CSR_MAC_CHAN_BUF_COUNT                     0x00c8
#define   NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU1(x)             (((x) & 0x1f) << 27)
#define   NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU1_of(x)          (((x) >> 27) & 0x1f)
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU1_bf           0, 31, 27
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU1_msk          (0x1f)
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU1_shf          (27)
#define   NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_IG_CHAN_RD_BUF_CNT(x) (((x) & 0x7ff) << 16)
#define   NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_IG_CHAN_RD_BUF_CNT_of(x) (((x) >> 16) & 0x7ff)
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_IG_CHAN_RD_BUF_CNT_bf 0, 26, 16
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_IG_CHAN_RD_BUF_CNT_msk (0x7ff)
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_IG_CHAN_RD_BUF_CNT_shf (16)
#define   NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU0(x)             (((x) & 0x1f) << 11)
#define   NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU0_of(x)          (((x) >> 11) & 0x1f)
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU0_bf           0, 15, 11
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU0_msk          (0x1f)
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_RFU0_shf          (11)
#define   NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_EG_CHAN_RD_BUF_CNT(x) (((x) & 0x7ff) << 0)
#define   NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_EG_CHAN_RD_BUF_CNT_of(x) (((x) >> 0) & 0x7ff)
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_EG_CHAN_RD_BUF_CNT_bf 0, 10, 0
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_EG_CHAN_RD_BUF_CNT_msk (0x7ff)
#define     NFP_MAC_CSR_MAC_CHAN_BUF_COUNT_EG_CHAN_RD_BUF_CNT_shf (0)


/*
 * Register: PauseWaterMark
 *   [31:28]   PWMResv1
 *   [27:16]   PauseWaterMark1
 *   [15:12]   PWMResv0
 *   [11:0]    PauseWaterMark0
 *
 * Name(s):
 * <base>.PauseWaterMark0...
 */
#define NFP_MAC_CSR_PAUSE_WATERMARK(x)                     (0x00cc + ((x) * 0x4))
#define   NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV1(x)           (((x) & 0xf) << 28)
#define   NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV1_of(x)        (((x) >> 28) & 0xf)
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV1_bf         0, 31, 28
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV1_msk        (0xf)
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV1_shf        (28)
#define   NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK1(x)    (((x) & 0xfff) << 16)
#define   NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK1_of(x) (((x) >> 16) & 0xfff)
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK1_bf  0, 27, 16
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK1_msk (0xfff)
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK1_shf (16)
#define   NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV0(x)           (((x) & 0xf) << 12)
#define   NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV0_of(x)        (((x) >> 12) & 0xf)
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV0_bf         0, 15, 12
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV0_msk        (0xf)
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PWM_RESV0_shf        (12)
#define   NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK0(x)    (((x) & 0xfff) << 0)
#define   NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK0_of(x) (((x) >> 0) & 0xfff)
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK0_bf  0, 11, 0
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK0_msk (0xfff)
#define     NFP_MAC_CSR_PAUSE_WATERMARK_PAUSE_WATERMARK0_shf (0)


/*
 * Register: BufferCounterRw
 *   [31:24]   CounterAddr
 *   [23:22]   Rfu2
 *   [21]      CounterRdBusy
 *   [20:0]    Rfu
 *
 * Name(s):
 * <base>.IgChanUsedBufferCreditsRw
 */
#define NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW         0x01d4
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_COUNTER_ADDR(x) (((x) & 0xff) << 24)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_COUNTER_ADDR_of(x) (((x) >> 24) & 0xff)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_COUNTER_ADDR_bf 0, 31, 24
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_COUNTER_ADDR_msk (0xff)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_COUNTER_ADDR_shf (24)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU2(x) (((x) & 3) << 22)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU2_of(x) (((x) >> 22) & 3)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU2_bf 0, 23, 22
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU2_msk (0x3)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU2_shf (22)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_COUNTER_RD_BUSY (1 << 21)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_COUNTER_RD_BUSY_bf 0, 21, 21
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_COUNTER_RD_BUSY_msk (0x1)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_COUNTER_RD_BUSY_bit (21)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU(x)  (((x) & 0x1fffff) << 0)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU_of(x) (((x) >> 0) & 0x1fffff)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU_bf 0, 20, 0
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU_msk (0x1fffff)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RW_RFU_shf (0)


/*
 * Register: BufferCounterRdData
 *   [31:24]   CounterAddr
 *   [21]      BufferCounterRdDataValid
 *   [15:0]    CounterRdData
 *
 * Name(s):
 * <base>.IgChanUsedBufferCreditsRdData
 */
#define NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA    0x01d8
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_ADDR(x) (((x) & 0xff) << 24)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_ADDR_of(x) (((x) >> 24) & 0xff)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_ADDR_bf 0, 31, 24
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_ADDR_msk (0xff)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_ADDR_shf (24)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_BUFFER_COUNTER_RD_DATA_VALID (1 << 21)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_BUFFER_COUNTER_RD_DATA_VALID_bf 0, 21, 21
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_BUFFER_COUNTER_RD_DATA_VALID_msk (0x1)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_BUFFER_COUNTER_RD_DATA_VALID_bit (21)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_RD_DATA(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_RD_DATA_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_RD_DATA_bf 0, 15, 0
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_RD_DATA_msk (0xffff)
#define     NFP_MAC_CSR_IG_CHAN_USED_BUFFER_CREDITS_RD_DATA_COUNTER_RD_DATA_shf (0)


/*
 * Register: IgPrependEn
 *   [31:26]   Rfu
 *   [25:24]   PrependLk
 *   [23:22]   PrependEn11
 *   [21:20]   PrependEn10
 *   [19:18]   PrependEn9
 *   [17:16]   PrependEn8
 *   [15:14]   PrependEn7
 *   [13:12]   PrependEn6
 *   [11:10]   PrependEn5
 *   [9:8]     PrependEn4
 *   [7:6]     PrependEn3
 *   [5:4]     PrependEn2
 *   [3:2]     PrependEn1
 *   [1:0]     PrependEn0
 *
 * Name(s):
 * <base>.IgPortPrependEn0 <base>.IgPortPrependEn1
 */
#define NFP_MAC_CSR_IG_PORT_PREPEND_EN0                    0x01dc
#define NFP_MAC_CSR_IG_PORT_PREPEND_EN1                    0x01e0
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_RFU(x)             (((x) & 0x3f) << 26)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_RFU_of(x)          (((x) >> 26) & 0x3f)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_RFU_bf           0, 31, 26
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_RFU_msk          (0x3f)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_RFU_shf          (26)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_LK(x)      (((x) & 3) << 24)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_LK_of(x)   (((x) >> 24) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_LK_bf    0, 25, 24
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_LK_msk   (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_LK_shf   (24)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN11(x)    (((x) & 3) << 22)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN11_of(x) (((x) >> 22) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN11_bf  0, 23, 22
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN11_msk (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN11_shf (22)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN10(x)    (((x) & 3) << 20)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN10_of(x) (((x) >> 20) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN10_bf  0, 21, 20
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN10_msk (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN10_shf (20)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN9(x)     (((x) & 3) << 18)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN9_of(x)  (((x) >> 18) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN9_bf   0, 19, 18
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN9_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN9_shf  (18)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN8(x)     (((x) & 3) << 16)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN8_of(x)  (((x) >> 16) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN8_bf   0, 17, 16
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN8_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN8_shf  (16)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN7(x)     (((x) & 3) << 14)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN7_of(x)  (((x) >> 14) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN7_bf   0, 15, 14
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN7_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN7_shf  (14)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN6(x)     (((x) & 3) << 12)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN6_of(x)  (((x) >> 12) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN6_bf   0, 13, 12
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN6_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN6_shf  (12)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN5(x)     (((x) & 3) << 10)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN5_of(x)  (((x) >> 10) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN5_bf   0, 11, 10
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN5_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN5_shf  (10)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN4(x)     (((x) & 3) << 8)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN4_of(x)  (((x) >> 8) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN4_bf   0, 9, 8
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN4_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN4_shf  (8)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN3(x)     (((x) & 3) << 6)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN3_of(x)  (((x) >> 6) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN3_bf   0, 7, 6
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN3_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN3_shf  (6)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN2(x)     (((x) & 3) << 4)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN2_of(x)  (((x) >> 4) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN2_bf   0, 5, 4
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN2_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN2_shf  (4)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN1(x)     (((x) & 3) << 2)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN1_of(x)  (((x) >> 2) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN1_bf   0, 3, 2
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN1_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN1_shf  (2)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN0(x)     (((x) & 3) << 0)
#define   NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN0_of(x)  (((x) >> 0) & 3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN0_No Prepend (0)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN0_Prepend CHK (1)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN0_Prepend TS (2)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN0_Prepend TS/CHK (3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN0_bf   0, 1, 0
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN0_msk  (0x3)
#define     NFP_MAC_CSR_IG_PORT_PREPEND_EN0_PREPEND_EN0_shf  (0)


/*
 * Register: VlanMatchReg
 *   [31:16]   VlanMask
 *   [15:0]    VlanMatch
 *
 * Name(s):
 * <base>.EgVlanMatchReg0 <base>.IgVlanMatchReg0
 */
#define NFP_MAC_CSR_EG_VLAN_MATCH_REG0                     0x01e8
#define NFP_MAC_CSR_IG_VLAN_MATCH_REG0                     0x01f0
#define   NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MASK(x)        (((x) & 0xffff) << 16)
#define   NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MASK_of(x)     (((x) >> 16) & 0xffff)
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MASK_bf      0, 31, 16
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MASK_msk     (0xffff)
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MASK_shf     (16)
#define   NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MATCH(x)       (((x) & 0xffff) << 0)
#define   NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MATCH_of(x)    (((x) >> 0) & 0xffff)
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MATCH_bf     0, 15, 0
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MATCH_msk    (0xffff)
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG0_VLAN_MATCH_shf    (0)


/*
 * Register: VlanMatchReg1
 *   [31:16]   VlanMask1
 *   [15:0]    VlanMatch1
 *
 * Name(s):
 * <base>.EgVlanMatchReg1 <base>.IgVlanMatchReg1
 */
#define NFP_MAC_CSR_EG_VLAN_MATCH_REG1                     0x01ec
#define NFP_MAC_CSR_IG_VLAN_MATCH_REG1                     0x01f4
#define   NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MASK1(x)       (((x) & 0xffff) << 16)
#define   NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MASK1_of(x)    (((x) >> 16) & 0xffff)
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MASK1_bf     0, 31, 16
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MASK1_msk    (0xffff)
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MASK1_shf    (16)
#define   NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MATCH1(x)      (((x) & 0xffff) << 0)
#define   NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MATCH1_of(x)   (((x) >> 0) & 0xffff)
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MATCH1_bf    0, 15, 0
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MATCH1_msk   (0xffff)
#define     NFP_MAC_CSR_EG_VLAN_MATCH_REG1_VLAN_MATCH1_shf   (0)


/*
 * Register: EgCmdPrependEn
 *   [31:0]    EgCmdPrependEn
 *
 * Name(s):
 * <base>.EgCmdPrependEn0Lo <base>.EgCmdPrependEn0Hi <base>.EgCmdPrependEn1Lo
 * <base>.EgCmdPrependEn1Hi
 */
#define NFP_MAC_CSR_EG_CMD_PREPEND_EN0_LO                  0x0200
#define NFP_MAC_CSR_EG_CMD_PREPEND_EN0_HI                  0x0204
#define NFP_MAC_CSR_EG_CMD_PREPEND_EN1_LO                  0x0208
#define NFP_MAC_CSR_EG_CMD_PREPEND_EN1_HI                  0x020c
#define   NFP_MAC_CSR_EG_CMD_PREPEND_EN0_LO_EG_CMD_PREPEND_EN(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_EG_CMD_PREPEND_EN0_LO_EG_CMD_PREPEND_EN_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_EG_CMD_PREPEND_EN0_LO_EG_CMD_PREPEND_EN_bf 0, 31, 0
#define     NFP_MAC_CSR_EG_CMD_PREPEND_EN0_LO_EG_CMD_PREPEND_EN_msk (0xffffffff)
#define     NFP_MAC_CSR_EG_CMD_PREPEND_EN0_LO_EG_CMD_PREPEND_EN_shf (0)


/*
 * Register: MacEgIlkChanAssign
 *   [31:29]   Rfu1
 *   [28:22]   LkNumChannelsUpper64
 *   [21:16]   LkBaseChannelUpper64
 *   [15:13]   Rfu0
 *   [12:6]    LkNumChannelsLower64
 *   [5:0]     LkBaseChannelLower64
 *
 * Name(s):
 * <base>.MacEgIlkChanAssign
 */
#define NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN                 0x0260
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU1(x)         (((x) & 7) << 29)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU1_of(x)      (((x) >> 29) & 7)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU1_bf       0, 31, 29
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU1_msk      (0x7)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU1_shf      (29)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_UPPER_64(x) (((x) & 0x7f) << 22)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_UPPER_64_of(x) (((x) >> 22) & 0x7f)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_UPPER_64_bf 0, 28, 22
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_UPPER_64_msk (0x7f)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_UPPER_64_shf (22)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_UPPER_64(x) (((x) & 0x3f) << 16)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_UPPER_64_of(x) (((x) >> 16) & 0x3f)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_UPPER_64_bf 0, 21, 16
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_UPPER_64_msk (0x3f)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_UPPER_64_shf (16)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU0(x)         (((x) & 7) << 13)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU0_of(x)      (((x) >> 13) & 7)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU0_bf       0, 15, 13
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU0_msk      (0x7)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_RFU0_shf      (13)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_LOWER_64(x) (((x) & 0x7f) << 6)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_LOWER_64_of(x) (((x) >> 6) & 0x7f)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_LOWER_64_bf 0, 12, 6
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_LOWER_64_msk (0x7f)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_NUM_CHANNELS_LOWER_64_shf (6)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_LOWER_64(x) (((x) & 0x3f) << 0)
#define   NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_LOWER_64_of(x) (((x) >> 0) & 0x3f)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_LOWER_64_bf 0, 5, 0
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_LOWER_64_msk (0x3f)
#define     NFP_MAC_CSR_MAC_EG_ILK_CHAN_ASSIGN_LK_BASE_CHANNEL_LOWER_64_shf (0)


/*
 * Register: MacEgPortRR
 *   [23:0]    EgPortRR
 *
 * Name(s):
 * <base>.MacEgPortRR
 */
#define NFP_MAC_CSR_MAC_EG_PORT_RR                         0x0264
#define   NFP_MAC_CSR_MAC_EG_PORT_RR_EG_PORT_RR(x)           (((x) & 0xffffff) << 0)
#define   NFP_MAC_CSR_MAC_EG_PORT_RR_EG_PORT_RR_of(x)        (((x) >> 0) & 0xffffff)
#define     NFP_MAC_CSR_MAC_EG_PORT_RR_EG_PORT_RR_bf         0, 23, 0
#define     NFP_MAC_CSR_MAC_EG_PORT_RR_EG_PORT_RR_msk        (0xffffff)
#define     NFP_MAC_CSR_MAC_EG_PORT_RR_EG_PORT_RR_shf        (0)


/*
 * Register: MacOobFcTmCntl
 *   [31:22]   Oob1023To512RFU
 *   [21:18]   Oob1023To512Mod32M1
 *   [17]      Oob1023To512MsgEn
 *   [16]      Oob1023To512En
 *   [15:6]    Oob511To0RFU
 *   [5:2]     Oob511To0Mod32M1
 *   [1]       Oob511To0MsgEn
 *   [0]       Oob511To0En
 *
 * Name(s):
 * <base>.MacOobFcTmCntl
 */
#define NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL                     0x0268
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_RFU(x) (((x) & 0x3ff) << 22)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_RFU_of(x) (((x) >> 22) & 0x3ff)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_RFU_bf 0, 31, 22
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_RFU_msk (0x3ff)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_RFU_shf (22)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_MOD32_M1(x) (((x) & 0xf) << 18)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_MOD32_M1_of(x) (((x) >> 18) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_MOD32_M1_bf 0, 21, 18
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_MOD32_M1_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_MOD32_M1_shf (18)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_MSG_EN (1 << 17)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_MSG_EN_bf 0, 17, 17
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_MSG_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_MSG_EN_bit (17)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_EN  (1 << 16)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_EN_bf 0, 16, 16
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_1023_TO_512_EN_bit (16)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_RFU(x) (((x) & 0x3ff) << 6)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_RFU_of(x) (((x) >> 6) & 0x3ff)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_RFU_bf 0, 15, 6
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_RFU_msk (0x3ff)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_RFU_shf (6)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_MOD32_M1(x) (((x) & 0xf) << 2)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_MOD32_M1_of(x) (((x) >> 2) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_MOD32_M1_bf 0, 5, 2
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_MOD32_M1_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_MOD32_M1_shf (2)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_MSG_EN (1 << 1)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_MSG_EN_bf 0, 1, 1
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_MSG_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_MSG_EN_bit (1)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_EN     (1 << 0)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_EN_bf 0, 0, 0
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_EN_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_CNTL_OOB_511_TO_0_EN_bit (0)


/*
 * Register: MacOobFcTmReMap
 *   [31:28]   TmFcAddr7Sel
 *   [27:24]   TmFcAddr6Sel
 *   [23:20]   TmFcAddr5Sel
 *   [19:16]   TmFcAddr4Sel
 *   [15:12]   TmFcAddr3Sel
 *   [11:8]    TmFcAddr2Sel
 *   [7:4]     TmFcAddr1Sel
 *   [3:0]     TmFcAddr0Sel
 *
 * Name(s):
 * <base>.MacOobFcTmReMap
 */
#define NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP                    0x026c
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR7_SEL(x) (((x) & 0xf) << 28)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR7_SEL_of(x) (((x) >> 28) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR7_SEL_bf 0, 31, 28
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR7_SEL_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR7_SEL_shf (28)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR6_SEL(x) (((x) & 0xf) << 24)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR6_SEL_of(x) (((x) >> 24) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR6_SEL_bf 0, 27, 24
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR6_SEL_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR6_SEL_shf (24)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR5_SEL(x) (((x) & 0xf) << 20)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR5_SEL_of(x) (((x) >> 20) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR5_SEL_bf 0, 23, 20
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR5_SEL_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR5_SEL_shf (20)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR4_SEL(x) (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR4_SEL_of(x) (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR4_SEL_bf 0, 19, 16
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR4_SEL_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR4_SEL_shf (16)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR3_SEL(x) (((x) & 0xf) << 12)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR3_SEL_of(x) (((x) >> 12) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR3_SEL_bf 0, 15, 12
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR3_SEL_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR3_SEL_shf (12)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR2_SEL(x) (((x) & 0xf) << 8)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR2_SEL_of(x) (((x) >> 8) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR2_SEL_bf 0, 11, 8
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR2_SEL_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR2_SEL_shf (8)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR1_SEL(x) (((x) & 0xf) << 4)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR1_SEL_of(x) (((x) >> 4) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR1_SEL_bf 0, 7, 4
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR1_SEL_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR1_SEL_shf (4)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR0_SEL(x) (((x) & 0xf) << 0)
#define   NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR0_SEL_of(x) (((x) >> 0) & 0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR0_SEL_bf 0, 3, 0
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR0_SEL_msk (0xf)
#define     NFP_MAC_CSR_MAC_OOB_FC_TM_REMAP_TM_FC_ADDR0_SEL_shf (0)


/*
 * Register: MacHeadDropCounters
 *   [31:16]   MacHeadDropCounter1
 *   [15:0]    MacHeadDropCounter0
 *
 * Name(s):
 * <base>.MacHy0EthIgPktHeadDropCntrPair0...
 * <base>.MacHy1EthIgPktHeadDropCntrPair0... <base>.MacIlkIgPktHeadDropCntrPair
 */
#define NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR_(x) (0x0280 + ((x) * 0x4))
#define NFP_MAC_CSR_MAC_HY1_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR_(x) (0x02a0 + ((x) * 0x4))
#define NFP_MAC_CSR_MAC_ILK_IG_PKT_HEAD_DROP_CNTR_PAIR     0x02b8
#define   NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER1(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER1_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER1_bf 0, 31, 16
#define     NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER1_msk (0xffff)
#define     NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER1_shf (16)
#define   NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER0(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER0_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER0_bf 0, 15, 0
#define     NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER0_msk (0xffff)
#define     NFP_MAC_CSR_MAC_HY0_ETH_IG_PKT_HEAD_DROP_CNTR_PAIR__MAC_HEAD_DROP_COUNTER0_shf (0)


/*
 * Register: MacEthFifoIfErr
 *   [24]      RemLocFaultSticky
 *   [23:12]   EthTxIfOvr
 *   [11:0]    EthTxIfUnf
 *
 * Name(s):
 * <base>.MacEthFifoIfErr0 <base>.MacEthFifoIfErr1
 */
#define NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0                  0x0400
#define NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_1                  0x0404
#define   NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_REM_LOC_FAULT_STICKY (1 << 24)
#define     NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_REM_LOC_FAULT_STICKY_bf 0, 24, 24
#define     NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_REM_LOC_FAULT_STICKY_msk (0x1)
#define     NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_REM_LOC_FAULT_STICKY_bit (24)
#define   NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_OVR(x) (((x) & 0xfff) << 12)
#define   NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_OVR_of(x) (((x) >> 12) & 0xfff)
#define     NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_OVR_bf 0, 23, 12
#define     NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_OVR_msk (0xfff)
#define     NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_OVR_shf (12)
#define   NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_UNF(x) (((x) & 0xfff) << 0)
#define   NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_UNF_of(x) (((x) >> 0) & 0xfff)
#define     NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_UNF_bf 0, 11, 0
#define     NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_UNF_msk (0xfff)
#define     NFP_MAC_CSR_MAC_ETH_FIFO_IF_ERR_0_ETH_TX_IF_UNF_shf (0)


/*
 * Register: MacEthAnStatus
 *   [23:12]   EthAnInt
 *   [11:0]    EthAnDone
 *
 * Name(s):
 * <base>.MacEthAnStatus0 <base>.MacEthAnStatus1
 */
#define NFP_MAC_CSR_MAC_ETH_AN_STATUS_0                    0x0408
#define NFP_MAC_CSR_MAC_ETH_AN_STATUS_1                    0x040c
#define   NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_INT(x)      (((x) & 0xfff) << 12)
#define   NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_INT_of(x)   (((x) >> 12) & 0xfff)
#define     NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_INT_bf    0, 23, 12
#define     NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_INT_msk   (0xfff)
#define     NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_INT_shf   (12)
#define   NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_DONE(x)     (((x) & 0xfff) << 0)
#define   NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_DONE_of(x)  (((x) >> 0) & 0xfff)
#define     NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_DONE_bf   0, 11, 0
#define     NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_DONE_msk  (0xfff)
#define     NFP_MAC_CSR_MAC_ETH_AN_STATUS_0_ETH_AN_DONE_shf  (0)


/*
 * Register: MacOobFcIlkStatus
 *   [19]      MacOobFcCrcErr1
 *   [18]      MacOobFcFrmErr1
 *   [17]      MacOobFcCrcErr0
 *   [16]      MacOobFcFrmErr0
 *   [7]       IlkInt2ndRx1
 *   [6]       IlkInt2ndTx1
 *   [5]       IlkIntRx1
 *   [4]       IlkIntTx1
 *   [3]       IlkInt2ndRx0
 *   [2]       IlkInt2ndTx0
 *   [1]       IlkIntRx0
 *   [0]       IlkIntTx0
 *
 * Name(s):
 * <base>.MacOobFcIlkStatus
 */
#define NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS                  0x0410
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_CRC_ERR_1 (1 << 19)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_CRC_ERR_1_bf 0, 19, 19
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_CRC_ERR_1_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_CRC_ERR_1_bit (19)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_FRM_ERR_1 (1 << 18)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_FRM_ERR_1_bf 0, 18, 18
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_FRM_ERR_1_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_FRM_ERR_1_bit (18)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_CRC_ERR_0 (1 << 17)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_CRC_ERR_0_bf 0, 17, 17
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_CRC_ERR_0_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_CRC_ERR_0_bit (17)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_FRM_ERR_0 (1 << 16)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_FRM_ERR_0_bf 0, 16, 16
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_FRM_ERR_0_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_MAC_OOB_FC_FRM_ERR_0_bit (16)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_RX_1 (1 << 7)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_RX_1_bf 0, 7, 7
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_RX_1_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_RX_1_bit (7)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_TX_1 (1 << 6)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_TX_1_bf 0, 6, 6
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_TX_1_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_TX_1_bit (6)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_RX_1     (1 << 5)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_RX_1_bf 0, 5, 5
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_RX_1_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_RX_1_bit (5)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_TX_1     (1 << 4)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_TX_1_bf 0, 4, 4
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_TX_1_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_TX_1_bit (4)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_RX_0 (1 << 3)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_RX_0_bf 0, 3, 3
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_RX_0_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_RX_0_bit (3)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_TX_0 (1 << 2)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_TX_0_bf 0, 2, 2
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_TX_0_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_2ND_TX_0_bit (2)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_RX_0     (1 << 1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_RX_0_bf 0, 1, 1
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_RX_0_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_RX_0_bit (1)
#define   NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_TX_0     (1 << 0)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_TX_0_bf 0, 0, 0
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_TX_0_msk (0x1)
#define     NFP_MAC_CSR_MAC_OOB_FC_ILK_STATUS_ILK_INT_TX_0_bit (0)


/*
 * Register: MacStatsHalfFull
 *   [24]      TxStatHalfFullVld
 *   [23:16]   TxStatAddr
 *   [8]       RxStatHalfFullVld
 *   [7:0]     RxStatAddr
 *
 * Name(s):
 * <base>.MacStatsHalfFullPort0011 <base>.MacStatsHalfFullPort1223
 * <base>.MacStatsHalfFullChan0063 <base>.MacStatsHalfFullChan63127
 */
#define NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11         0x0420
#define NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_12_23         0x0424
#define NFP_MAC_CSR_MAC_STATS_HALF_FULL_CHAN_00_63         0x0428
#define NFP_MAC_CSR_MAC_STATS_HALF_FULL_CHAN_63_127        0x042c
#define   NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_TX_STAT_HALF_FULL_VLD (1 << 24)
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_TX_STAT_HALF_FULL_VLD_bf 0, 24, 24
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_TX_STAT_HALF_FULL_VLD_msk (0x1)
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_TX_STAT_HALF_FULL_VLD_bit (24)
#define   NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_TX_STAT_ADDR(x) (((x) & 0xff) << 16)
#define   NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_TX_STAT_ADDR_of(x) (((x) >> 16) & 0xff)
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_TX_STAT_ADDR_bf 0, 23, 16
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_TX_STAT_ADDR_msk (0xff)
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_TX_STAT_ADDR_shf (16)
#define   NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_RX_STAT_HALF_FULL_VLD (1 << 8)
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_RX_STAT_HALF_FULL_VLD_bf 0, 8, 8
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_RX_STAT_HALF_FULL_VLD_msk (0x1)
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_RX_STAT_HALF_FULL_VLD_bit (8)
#define   NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_RX_STAT_ADDR(x) (((x) & 0xff) << 0)
#define   NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_RX_STAT_ADDR_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_RX_STAT_ADDR_bf 0, 7, 0
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_RX_STAT_ADDR_msk (0xff)
#define     NFP_MAC_CSR_MAC_STATS_HALF_FULL_PORT_00_11_RX_STAT_ADDR_shf (0)


/*
 * Register: MacPcpReMap
 *   [31:30]   PcpReMapRFU
 *   [29:24]   UntaggedChan
 *   [23:21]   PcpReMap7
 *   [20:18]   PcpReMap6
 *   [17:15]   PcpReMap5
 *   [14:12]   PcpReMap4
 *   [11:9]    PcpReMap3
 *   [8:6]     PcpReMap2
 *   [5:3]     PcpReMap1
 *   [2:0]     PcpReMap0
 *
 * Name(s):
 * <base>.MacPcpReMap0...
 */
#define NFP_MAC_CSR_MAC_PCP_REMAP(x)                       (0x0680 + ((x) * 0x4))
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_RE_MAP_RFU(x)        (((x) & 3) << 30)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_RE_MAP_RFU_of(x)     (((x) >> 30) & 3)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_RE_MAP_RFU_bf      0, 31, 30
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_RE_MAP_RFU_msk     (0x3)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_RE_MAP_RFU_shf     (30)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_MAC_UNTAGD_ABS(x)        (((x) & 0x3f) << 24)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_MAC_UNTAGD_ABS_of(x)     (((x) >> 24) & 0x3f)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_MAC_UNTAGD_ABS_bf      0, 29, 24
#define     NFP_MAC_CSR_MAC_PCP_REMAP_MAC_UNTAGD_ABS_msk     (0x3f)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_MAC_UNTAGD_ABS_shf     (24)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP7(x)            (((x) & 7) << 21)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP7_of(x)         (((x) >> 21) & 7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP7_bf          0, 23, 21
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP7_msk         (0x7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP7_shf         (21)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP6(x)            (((x) & 7) << 18)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP6_of(x)         (((x) >> 18) & 7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP6_bf          0, 20, 18
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP6_msk         (0x7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP6_shf         (18)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP5(x)            (((x) & 7) << 15)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP5_of(x)         (((x) >> 15) & 7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP5_bf          0, 17, 15
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP5_msk         (0x7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP5_shf         (15)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP4(x)            (((x) & 7) << 12)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP4_of(x)         (((x) >> 12) & 7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP4_bf          0, 14, 12
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP4_msk         (0x7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP4_shf         (12)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP3(x)            (((x) & 7) << 9)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP3_of(x)         (((x) >> 9) & 7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP3_bf          0, 11, 9
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP3_msk         (0x7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP3_shf         (9)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP2(x)            (((x) & 7) << 6)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP2_of(x)         (((x) >> 6) & 7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP2_bf          0, 8, 6
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP2_msk         (0x7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP2_shf         (6)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP1(x)            (((x) & 7) << 3)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP1_of(x)         (((x) >> 3) & 7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP1_bf          0, 5, 3
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP1_msk         (0x7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP1_shf         (3)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP0(x)            (((x) & 7) << 0)
#define   NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP0_of(x)         (((x) >> 0) & 7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP0_bf          0, 2, 0
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP0_msk         (0x7)
#define     NFP_MAC_CSR_MAC_PCP_REMAP_PCP_REMAP0_shf         (0)


/*
 * Register: MacPortHwm
 *   [31:27]   PortDropDelta1
 *   [26:16]   PortHwm1
 *   [15:11]   PortDropDelta0
 *   [10:0]    PortHwm0
 *
 * Name(s):
 * <base>.MacPortHwm0...
 */
#define NFP_MAC_CSR_MAC_PORT_HWM(x)                        (0x0700 + ((x) * 0x4))
#define   NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA1(x)        (((x) & 0x1f) << 27)
#define   NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA1_of(x)     (((x) >> 27) & 0x1f)
#define     NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA1_bf      0, 31, 27
#define     NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA1_msk     (0x1f)
#define     NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA1_shf     (27)
#define   NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM1(x)              (((x) & 0x7ff) << 16)
#define   NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM1_of(x)           (((x) >> 16) & 0x7ff)
#define     NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM1_bf            0, 26, 16
#define     NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM1_msk           (0x7ff)
#define     NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM1_shf           (16)
#define   NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA0(x)        (((x) & 0x1f) << 11)
#define   NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA0_of(x)     (((x) >> 11) & 0x1f)
#define     NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA0_bf      0, 15, 11
#define     NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA0_msk     (0x1f)
#define     NFP_MAC_CSR_MAC_PORT_HWM_POR_DROP_DELTA0_shf     (11)
#define   NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM0(x)              (((x) & 0x7ff) << 0)
#define   NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM0_of(x)           (((x) >> 0) & 0x7ff)
#define     NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM0_bf            0, 10, 0
#define     NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM0_msk           (0x7ff)
#define     NFP_MAC_CSR_MAC_PORT_HWM_PORT_HWM0_shf           (0)


/*
 * Register: MacPortHwm1
 *   [31:27]   PortDropDelta1
 *   [26:16]   PortHwm1
 *   [15:11]   PortDropDelta0
 *   [10:0]    PortHwm0
 *
 * Name(s):
 * <base>.MacPortHwmLk1Lk0
 */
#define NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0                   0x0730
#define   NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA1(x) (((x) & 0x1f) << 27)
#define   NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA1_of(x) (((x) >> 27) & 0x1f)
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA1_bf 0, 31, 27
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA1_msk (0x1f)
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA1_shf (27)
#define   NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM1(x)      (((x) & 0x7ff) << 16)
#define   NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM1_of(x)   (((x) >> 16) & 0x7ff)
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM1_bf    0, 26, 16
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM1_msk   (0x7ff)
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM1_shf   (16)
#define   NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA0(x) (((x) & 0x1f) << 11)
#define   NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA0_of(x) (((x) >> 11) & 0x1f)
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA0_bf 0, 15, 11
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA0_msk (0x1f)
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_POR_DROP_DELTA0_shf (11)
#define   NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM0(x)      (((x) & 0x7ff) << 0)
#define   NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM0_of(x)   (((x) >> 0) & 0x7ff)
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM0_bf    0, 10, 0
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM0_msk   (0x7ff)
#define     NFP_MAC_CSR_MAC_PORT_HWM_LK1_LK0_PORT_HWM0_shf   (0)


/*
 * Register: LLMemRdData
 *   [29:18]   LLRdOffsetAddr
 *   [17]      LLRdDataValid
 *   [15:0]    LLRdData
 *
 * Name(s):
 * <base>.EgLnkLstRdData <base>.IgLnkLstRdData
 */
#define NFP_MAC_CSR_EG_LNKLST_RDDATA                       0x07b0
#define NFP_MAC_CSR_IG_LNKLST_RDDATA                       0x07b4
#define   NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_OFFSET_ADDR(x)  (((x) & 0xfff) << 18)
#define   NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_OFFSET_ADDR_of(x) (((x) >> 18) & 0xfff)
#define     NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_OFFSET_ADDR_bf 0, 29, 18
#define     NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_OFFSET_ADDR_msk (0xfff)
#define     NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_OFFSET_ADDR_shf (18)
#define   NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_DATA_VALID      (1 << 17)
#define     NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_DATA_VALID_bf 0, 17, 17
#define     NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_DATA_VALID_msk (0x1)
#define     NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_DATA_VALID_bit (17)
#define   NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_DATA(x)         (((x) & 0xffff) << 0)
#define   NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_DATA_of(x)      (((x) >> 0) & 0xffff)
#define     NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_DATA_bf       0, 15, 0
#define     NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_DATA_msk      (0xffff)
#define     NFP_MAC_CSR_EG_LNKLST_RDDATA_LL_RD_DATA_shf      (0)


/*
 * Register: LLMemRdWr
 *   [31:30]   Rfu
 *   [29:18]   LLOffsetAddr
 *   [17]      LLRdBusy
 *   [16]      LLWrBusy
 *   [15:0]    LLWrData
 *
 * Name(s):
 * <base>.EgLnkLstRdWr <base>.IgLnkLstRdWr
 */
#define NFP_MAC_CSR_EG_LNKLST_RDWR                         0x07b8
#define NFP_MAC_CSR_IG_LNKLST_RDWR                         0x07bc
#define   NFP_MAC_CSR_EG_LNKLST_RDWR_RFU(x)                  (((x) & 3) << 30)
#define   NFP_MAC_CSR_EG_LNKLST_RDWR_RFU_of(x)               (((x) >> 30) & 3)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_RFU_bf                0, 31, 30
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_RFU_msk               (0x3)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_RFU_shf               (30)
#define   NFP_MAC_CSR_EG_LNKLST_RDWR_LL_OFFSET_ADDR(x)       (((x) & 0xfff) << 18)
#define   NFP_MAC_CSR_EG_LNKLST_RDWR_LL_OFFSET_ADDR_of(x)    (((x) >> 18) & 0xfff)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_OFFSET_ADDR_bf     0, 29, 18
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_OFFSET_ADDR_msk    (0xfff)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_OFFSET_ADDR_shf    (18)
#define   NFP_MAC_CSR_EG_LNKLST_RDWR_LL_RD_BUSY              (1 << 17)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_RD_BUSY_bf         0, 17, 17
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_RD_BUSY_msk        (0x1)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_RD_BUSY_bit        (17)
#define   NFP_MAC_CSR_EG_LNKLST_RDWR_LL_WR_BUSY              (1 << 16)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_WR_BUSY_bf         0, 16, 16
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_WR_BUSY_msk        (0x1)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_WR_BUSY_bit        (16)
#define   NFP_MAC_CSR_EG_LNKLST_RDWR_LL_WR_DATA(x)           (((x) & 0xffff) << 0)
#define   NFP_MAC_CSR_EG_LNKLST_RDWR_LL_WR_DATA_of(x)        (((x) >> 0) & 0xffff)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_WR_DATA_bf         0, 15, 0
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_WR_DATA_msk        (0xffff)
#define     NFP_MAC_CSR_EG_LNKLST_RDWR_LL_WR_DATA_shf        (0)


/*
 * Register: SerDes4RdWr
 *   [31:29]   SerDesPageAddr
 *   [28]      SerDesPcsPmaSel
 *   [27:16]   SerDesOffsetAddr
 *   [15:10]   Rfu
 *   [9]       SerDesRdBusy
 *   [8]       SerDesWrBusy
 *   [7:0]     SerDesWrData
 *
 * Name(s):
 * <base>.SerDes4RdWr03To00 <base>.SerDes4RdWr07To04 <base>.SerDes4RdWr11To08
 * <base>.SerDes4RdWr15To12 <base>.SerDes4RdWr19To16 <base>.SerDes4RdWr23To20
 */
#define NFP_MAC_CSR_SERDES4_RDWR_03_00                     0x07c0
#define NFP_MAC_CSR_SERDES4_RDWR_07_04                     0x07c4
#define NFP_MAC_CSR_SERDES4_RDWR_11_08                     0x07c8
#define NFP_MAC_CSR_SERDES4_RDWR_15_12                     0x07cc
#define NFP_MAC_CSR_SERDES4_RDWR_19_16                     0x07d0
#define NFP_MAC_CSR_SERDES4_RDWR_23_20                     0x07d4
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR(x) (((x) & 7) << 29)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_of(x) (((x) >> 29) & 7)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_Lane 0 Select (0)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_Lane 1 Select (1)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_Lane 2 Select (2)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_Lane 3 Select (3)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_Common Select (4)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_All lane Select (7)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_bf 0, 31, 29
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_msk (0x7)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PAGE_ADDR_shf (29)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PCS_PMA_SEL (1 << 28)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PCS_PMA_SEL_bf 0, 28, 28
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PCS_PMA_SEL_msk (0x1)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_PCS_PMA_SEL_bit (28)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_OFFSET_ADDR(x) (((x) & 0xfff) << 16)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_OFFSET_ADDR_of(x) (((x) >> 16) & 0xfff)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_OFFSET_ADDR_bf 0, 27, 16
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_OFFSET_ADDR_msk (0xfff)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_OFFSET_ADDR_shf (16)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_RFU(x)              (((x) & 0x3f) << 10)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_RFU_of(x)           (((x) >> 10) & 0x3f)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_RFU_bf            0, 15, 10
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_RFU_msk           (0x3f)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_RFU_shf           (10)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_RD_BUSY  (1 << 9)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_RD_BUSY_bf 0, 9, 9
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_RD_BUSY_msk (0x1)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_RD_BUSY_bit (9)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_WR_BUSY  (1 << 8)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_WR_BUSY_bf 0, 8, 8
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_WR_BUSY_msk (0x1)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_WR_BUSY_bit (8)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_WR_DATA(x) (((x) & 0xff) << 0)
#define   NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_WR_DATA_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_WR_DATA_bf 0, 7, 0
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_WR_DATA_msk (0xff)
#define     NFP_MAC_CSR_SERDES4_RDWR_03_00_MAC_SERDES_WR_DATA_shf (0)


/*
 * Register: TdmMemRdWr
 *   [31:30]   TdmMemRdWrRFU1
 *   [29:24]   TdmMemRdWrAddr
 *   [23:22]   TdmMemRdWrRFU0
 *   [21]      TdmMemRdBusy
 *   [20]      TdmMemWrBusy
 *   [19:16]   TdmMemUnused
 *   [15]      TdmPortArbEnable
 *   [14:0]    TdmPortWeightWrData
 *
 * Name(s):
 * <base>.IgDqTdmMemoryRW
 */
#define NFP_MAC_CSR_TDM_MEM_RDWR                           0x07d8
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU1(x)     (((x) & 3) << 30)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU1_of(x)  (((x) >> 30) & 3)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU1_bf   0, 31, 30
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU1_msk  (0x3)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU1_shf  (30)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_ADDR(x)     (((x) & 0x3f) << 24)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_ADDR_of(x)  (((x) >> 24) & 0x3f)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_ADDR_bf   0, 29, 24
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_ADDR_msk  (0x3f)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_ADDR_shf  (24)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU0(x)     (((x) & 3) << 22)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU0_of(x)  (((x) >> 22) & 3)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU0_bf   0, 23, 22
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU0_msk  (0x3)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_WR_RFU0_shf  (22)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_BUSY           (1 << 21)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_BUSY_bf      0, 21, 21
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_BUSY_msk     (0x1)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_RD_BUSY_bit     (21)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_WR_BUSY           (1 << 20)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_WR_BUSY_bf      0, 20, 20
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_WR_BUSY_msk     (0x1)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_WR_BUSY_bit     (20)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_UNUSED(x)         (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_UNUSED_of(x)      (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_UNUSED_bf       0, 19, 16
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_UNUSED_msk      (0xf)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_MEM_UNUSED_shf      (16)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_PORT_ARB_ENABLE       (1 << 15)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_PORT_ARB_ENABLE_bf  0, 15, 15
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_PORT_ARB_ENABLE_msk (0x1)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_PORT_ARB_ENABLE_bit (15)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_PORT_WEIGHTWR_DATA(x) (((x) & 0x7fff) << 0)
#define   NFP_MAC_CSR_TDM_MEM_RDWR_TDM_PORT_WEIGHTWR_DATA_of(x) (((x) >> 0) & 0x7fff)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_PORT_WEIGHTWR_DATA_bf 0, 14, 0
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_PORT_WEIGHTWR_DATA_msk (0x7fff)
#define     NFP_MAC_CSR_TDM_MEM_RDWR_TDM_PORT_WEIGHTWR_DATA_shf (0)


/*
 * Register: SerDes4RdData
 *   [31:29]   SerDesRdPageAddr
 *   [28:16]   SerDesRdOffsetAddr
 *   [9]       SerDesRdDataValid
 *   [7:0]     SerDesRdData
 *
 * Name(s):
 * <base>.SerDes4RdData03To00 <base>.SerDes4RdData07To04
 * <base>.SerDes4RdData11To08 <base>.SerDes4RdData15To12
 * <base>.SerDes4RdData19To16 <base>.SerDes4RdData23To20
 */
#define NFP_MAC_CSR_SERDES4_RDDATA_03_00                   0x07e0
#define NFP_MAC_CSR_SERDES4_RDDATA_07_04                   0x07e4
#define NFP_MAC_CSR_SERDES4_RDDATA_11_08                   0x07e8
#define NFP_MAC_CSR_SERDES4_RDDATA_15_12                   0x07ec
#define NFP_MAC_CSR_SERDES4_RDDATA_19_16                   0x07f0
#define NFP_MAC_CSR_SERDES4_RDDATA_23_20                   0x07f4
#define   NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR(x) (((x) & 7) << 29)
#define   NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_of(x) (((x) >> 29) & 7)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_Lane 0 Select (0)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_Lane 1 Select (1)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_Lane 2 Select (2)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_Lane 3 Select (3)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_Common Select (4)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_All lane Select (7)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_bf 0, 31, 29
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_msk (0x7)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_PAGE_ADDR_shf (29)
#define   NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_OFFSET_ADDR(x) (((x) & 0x1fff) << 16)
#define   NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_OFFSET_ADDR_of(x) (((x) >> 16) & 0x1fff)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_OFFSET_ADDR_bf 0, 28, 16
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_OFFSET_ADDR_msk (0x1fff)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_OFFSET_ADDR_shf (16)
#define   NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_DATA_VALID (1 << 9)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_DATA_VALID_bf 0, 9, 9
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_DATA_VALID_msk (0x1)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_DATA_VALID_bit (9)
#define   NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_DATA(x) (((x) & 0xff) << 0)
#define   NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_DATA_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_DATA_bf 0, 7, 0
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_DATA_msk (0xff)
#define     NFP_MAC_CSR_SERDES4_RDDATA_03_00_MAC_SERDES_RD_DATA_shf (0)


/*
 * Register: TdmMemRdData
 *   [29:24]   TdmMemRdAddr
 *   [21]      TdmMemRdDataValid
 *   [19:16]   TdmPortUnusedRdRet
 *   [15]      TdmPortArbEnable
 *   [14:0]    TdmPortWeightRdData
 *
 * Name(s):
 * <base>.IgDqTdmMemoryRdData
 */
#define NFP_MAC_CSR_TDM_MEM_RDDATA                         0x07f8
#define   NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_MEM_RD_ADDR(x)      (((x) & 0x3f) << 24)
#define   NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_MEM_RD_ADDR_of(x)   (((x) >> 24) & 0x3f)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_MEM_RD_ADDR_bf    0, 29, 24
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_MEM_RD_ADDR_msk   (0x3f)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_MEM_RD_ADDR_shf   (24)
#define   NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_MEM_RD_DATA_VALID   (1 << 21)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_MEM_RD_DATA_VALID_bf 0, 21, 21
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_MEM_RD_DATA_VALID_msk (0x1)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_MEM_RD_DATA_VALID_bit (21)
#define   NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_UNUSED_RD_RET(x) (((x) & 0xf) << 16)
#define   NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_UNUSED_RD_RET_of(x) (((x) >> 16) & 0xf)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_UNUSED_RD_RET_bf 0, 19, 16
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_UNUSED_RD_RET_msk (0xf)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_UNUSED_RD_RET_shf (16)
#define   NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_ARB_ENABLE     (1 << 15)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_ARB_ENABLE_bf 0, 15, 15
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_ARB_ENABLE_msk (0x1)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_ARB_ENABLE_bit (15)
#define   NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_WEIGHTRD_DATA(x) (((x) & 0x7fff) << 0)
#define   NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_WEIGHTRD_DATA_of(x) (((x) >> 0) & 0x7fff)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_WEIGHTRD_DATA_bf 0, 14, 0
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_WEIGHTRD_DATA_msk (0x7fff)
#define     NFP_MAC_CSR_TDM_MEM_RDDATA_TDM_PORT_WEIGHTRD_DATA_shf (0)


/*
 * Register: SerDesPdLn
 *   [23:0]    SerDesLanePowerDown
 *
 * Name(s):
 * <base>.SerDesPdRx <base>.SerDesPdTx
 */
#define NFP_MAC_CSR_SERDES_PD_RX                           0x0800
#define NFP_MAC_CSR_SERDES_PD_TX                           0x0804
#define   NFP_MAC_CSR_SERDES_PD_RX_SERDES_LANE_POWER_DOWN(x) (((x) & 0xffffff) << 0)
#define   NFP_MAC_CSR_SERDES_PD_RX_SERDES_LANE_POWER_DOWN_of(x) (((x) >> 0) & 0xffffff)
#define     NFP_MAC_CSR_SERDES_PD_RX_SERDES_LANE_POWER_DOWN_bf 0, 23, 0
#define     NFP_MAC_CSR_SERDES_PD_RX_SERDES_LANE_POWER_DOWN_msk (0xffffff)
#define     NFP_MAC_CSR_SERDES_PD_RX_SERDES_LANE_POWER_DOWN_shf (0)


/*
 * Register: SerDesPdSy
 *   [5:0]     SerDesSynthPowerDown
 *
 * Name(s):
 * <base>.SerDesPdSy
 */
#define NFP_MAC_CSR_SERDES_PD_SY                           0x0808
#define   NFP_MAC_CSR_SERDES_PD_SY_SERDES_SYNTH_POWER_DOWN(x) (((x) & 0x3f) << 0)
#define   NFP_MAC_CSR_SERDES_PD_SY_SERDES_SYNTH_POWER_DOWN_of(x) (((x) >> 0) & 0x3f)
#define     NFP_MAC_CSR_SERDES_PD_SY_SERDES_SYNTH_POWER_DOWN_bf 0, 5, 0
#define     NFP_MAC_CSR_SERDES_PD_SY_SERDES_SYNTH_POWER_DOWN_msk (0x3f)
#define     NFP_MAC_CSR_SERDES_PD_SY_SERDES_SYNTH_POWER_DOWN_shf (0)


/*
 * Register: SerDesCkMuxSel
 *   [31:30]   SerDesCkMuxSel_RFU
 *   [29]      SerDesCkMuxSelGang2320
 *   [28]      SerDesCkMuxSelL23
 *   [27]      SerDesCkMuxSelL22
 *   [26]      SerDesCkMuxSelL21
 *   [25]      SerDesCkMuxSelL20
 *   [24]      SerDesCkMuxSelGang1916
 *   [23]      SerDesCkMuxSelL19
 *   [22]      SerDesCkMuxSelL18
 *   [21]      SerDesCkMuxSelL17
 *   [20]      SerDesCkMuxSelL16
 *   [19]      SerDesCkMuxSelGang1512
 *   [18]      SerDesCkMuxSelL15
 *   [17]      SerDesCkMuxSelL14
 *   [16]      SerDesCkMuxSelL13
 *   [15]      SerDesCkMuxSelL12
 *   [14]      SerDesCkMuxSelGang1108
 *   [13]      SerDesCkMuxSelL11
 *   [12]      SerDesCkMuxSelL10
 *   [11]      SerDesCkMuxSelL09
 *   [10]      SerDesCkMuxSelL08
 *   [9]       SerDesCkMuxSelGang0704
 *   [8]       SerDesCkMuxSelL07
 *   [7]       SerDesCkMuxSelL06
 *   [6]       SerDesCkMuxSelL05
 *   [5]       SerDesCkMuxSelL04
 *   [4]       SerDesCkMuxSelGang0300
 *   [3]       SerDesCkMuxSelL03
 *   [2]       SerDesCkMuxSelL02
 *   [1]       SerDesCkMuxSelL01
 *   [0]       SerDesCkMuxSelL00
 *
 * Name(s):
 * <base>.SerDesCkMuxSel
 */
#define NFP_MAC_CSR_SERDES_CK_MUX_SEL                      0x080c
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_RFU(x) (((x) & 3) << 30)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_RFU_of(x) (((x) >> 30) & 3)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_RFU_bf 0, 31, 30
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_RFU_msk (0x3)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_RFU_shf (30)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_2320 (1 << 29)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_2320_bf 0, 29, 29
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_2320_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_2320_bit (29)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_23 (1 << 28)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_23_bf 0, 28, 28
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_23_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_23_bit (28)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_22 (1 << 27)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_22_bf 0, 27, 27
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_22_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_22_bit (27)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_21 (1 << 26)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_21_bf 0, 26, 26
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_21_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_21_bit (26)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_20 (1 << 25)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_20_bf 0, 25, 25
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_20_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_20_bit (25)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1916 (1 << 24)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1916_bf 0, 24, 24
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1916_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1916_bit (24)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_19 (1 << 23)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_19_bf 0, 23, 23
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_19_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_19_bit (23)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_18 (1 << 22)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_18_bf 0, 22, 22
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_18_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_18_bit (22)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_17 (1 << 21)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_17_bf 0, 21, 21
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_17_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_17_bit (21)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_16 (1 << 20)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_16_bf 0, 20, 20
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_16_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_16_bit (20)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1512 (1 << 19)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1512_bf 0, 19, 19
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1512_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1512_bit (19)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_15 (1 << 18)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_15_bf 0, 18, 18
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_15_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_15_bit (18)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_14 (1 << 17)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_14_bf 0, 17, 17
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_14_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_14_bit (17)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_13 (1 << 16)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_13_bf 0, 16, 16
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_13_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_13_bit (16)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_12 (1 << 15)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_12_bf 0, 15, 15
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_12_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_12_bit (15)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1108 (1 << 14)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1108_bf 0, 14, 14
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1108_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_1108_bit (14)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_11 (1 << 13)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_11_bf 0, 13, 13
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_11_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_11_bit (13)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_10 (1 << 12)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_10_bf 0, 12, 12
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_10_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_10_bit (12)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_09 (1 << 11)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_09_bf 0, 11, 11
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_09_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_09_bit (11)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_08 (1 << 10)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_08_bf 0, 10, 10
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_08_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_08_bit (10)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_0704 (1 << 9)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_0704_bf 0, 9, 9
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_0704_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_0704_bit (9)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_07 (1 << 8)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_07_bf 0, 8, 8
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_07_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_07_bit (8)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_06 (1 << 7)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_06_bf 0, 7, 7
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_06_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_06_bit (7)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_05 (1 << 6)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_05_bf 0, 6, 6
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_05_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_05_bit (6)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_04 (1 << 5)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_04_bf 0, 5, 5
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_04_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_04_bit (5)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_0300 (1 << 4)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_0300_bf 0, 4, 4
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_0300_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_GANG_0300_bit (4)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_03 (1 << 3)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_03_bf 0, 3, 3
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_03_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_03_bit (3)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_02 (1 << 2)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_02_bf 0, 2, 2
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_02_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_02_bit (2)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_01 (1 << 1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_01_bf 0, 1, 1
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_01_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_01_bit (1)
#define   NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_00 (1 << 0)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_00_bf 0, 0, 0
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_00_msk (0x1)
#define     NFP_MAC_CSR_SERDES_CK_MUX_SEL_SERDES_CK_MUX_SEL_00_bit (0)


/*
 * Register: SerDesSigDetect
 *   [23:0]    SerDesLaneSigDetect
 *
 * Name(s):
 * <base>.SerDesSigDetect
 */
#define NFP_MAC_CSR_SERDES_SIG_DETECT                      0x0810
#define   NFP_MAC_CSR_SERDES_SIG_DETECT_SERDES_LANE_SIGNAL_DETECT(x) (((x) & 0xffffff) << 0)
#define   NFP_MAC_CSR_SERDES_SIG_DETECT_SERDES_LANE_SIGNAL_DETECT_of(x) (((x) >> 0) & 0xffffff)
#define     NFP_MAC_CSR_SERDES_SIG_DETECT_SERDES_LANE_SIGNAL_DETECT_bf 0, 23, 0
#define     NFP_MAC_CSR_SERDES_SIG_DETECT_SERDES_LANE_SIGNAL_DETECT_msk (0xffffff)
#define     NFP_MAC_CSR_SERDES_SIG_DETECT_SERDES_LANE_SIGNAL_DETECT_shf (0)


/*
 * Register: SerDesSigDetectOvr
 *   [23:0]    SerDesLaneSigDetectOvr
 *
 * Name(s):
 * <base>.SerDesSigDetectOvr
 */
#define NFP_MAC_CSR_SERDES_SIG_DETECT_OVR                  0x0814
#define   NFP_MAC_CSR_SERDES_SIG_DETECT_OVR_SERDES_LANE_SIGNAL_DETECT_OVR(x) (((x) & 0xffffff) << 0)
#define   NFP_MAC_CSR_SERDES_SIG_DETECT_OVR_SERDES_LANE_SIGNAL_DETECT_OVR_of(x) (((x) >> 0) & 0xffffff)
#define     NFP_MAC_CSR_SERDES_SIG_DETECT_OVR_SERDES_LANE_SIGNAL_DETECT_OVR_bf 0, 23, 0
#define     NFP_MAC_CSR_SERDES_SIG_DETECT_OVR_SERDES_LANE_SIGNAL_DETECT_OVR_msk (0xffffff)
#define     NFP_MAC_CSR_SERDES_SIG_DETECT_OVR_SERDES_LANE_SIGNAL_DETECT_OVR_shf (0)


/*
 * Register: SerDesActDetect
 *   [23:0]    SerDesPortActDetect
 *
 * Name(s):
 * <base>.SerDesEthRxActDetect <base>.SerDesEthTxActDetect
 */
#define NFP_MAC_CSR_SERDES_ETH_RX_ACT_DETECT               0x0818
#define NFP_MAC_CSR_SERDES_ETH_TX_ACT_DETECT               0x081c
#define   NFP_MAC_CSR_SERDES_ETH_RX_ACT_DETECT_SERDES_PORT_ACTIVITY_DETECT(x) (((x) & 0xffffff) << 0)
#define   NFP_MAC_CSR_SERDES_ETH_RX_ACT_DETECT_SERDES_PORT_ACTIVITY_DETECT_of(x) (((x) >> 0) & 0xffffff)
#define     NFP_MAC_CSR_SERDES_ETH_RX_ACT_DETECT_SERDES_PORT_ACTIVITY_DETECT_bf 0, 23, 0
#define     NFP_MAC_CSR_SERDES_ETH_RX_ACT_DETECT_SERDES_PORT_ACTIVITY_DETECT_msk (0xffffff)
#define     NFP_MAC_CSR_SERDES_ETH_RX_ACT_DETECT_SERDES_PORT_ACTIVITY_DETECT_shf (0)


/*
 * Register: SerDesLinkUp
 *   [23:0]    SerDesLInkUp
 *
 * Name(s):
 * <base>.SerDesLinkUp
 */
#define NFP_MAC_CSR_SERDES_LINK_UP                         0x0820
#define   NFP_MAC_CSR_SERDES_LINK_UP_SERDES_LINK_UP(x)       (((x) & 0xffffff) << 0)
#define   NFP_MAC_CSR_SERDES_LINK_UP_SERDES_LINK_UP_of(x)    (((x) >> 0) & 0xffffff)
#define     NFP_MAC_CSR_SERDES_LINK_UP_SERDES_LINK_UP_bf     0, 23, 0
#define     NFP_MAC_CSR_SERDES_LINK_UP_SERDES_LINK_UP_msk    (0xffffff)
#define     NFP_MAC_CSR_SERDES_LINK_UP_SERDES_LINK_UP_shf    (0)


/*
 * Register: ParityErrInject
 *   [5]       InjectIgParErrDescMem
 *   [4]       InjectIgParErrTsmpMem
 *   [3]       InjectIgParErrRsltMem
 *   [2]       InjectEgParErrDescMem
 *   [1]       InjectEgParErrRslt1Mem
 *   [0]       InjectEgParErrRslt0Mem
 *
 * Name(s):
 * <base>.ParityErrInject
 */
#define NFP_MAC_CSR_PARITY_ERR_INJECT                      0x0824
#define   NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_DESC_MEM (1 << 5)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_DESC_MEM_bf 0, 5, 5
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_DESC_MEM_msk (0x1)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_DESC_MEM_bit (5)
#define   NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_TSMP_MEM (1 << 4)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_TSMP_MEM_bf 0, 4, 4
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_TSMP_MEM_msk (0x1)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_TSMP_MEM_bit (4)
#define   NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_RSLT_MEM (1 << 3)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_RSLT_MEM_bf 0, 3, 3
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_RSLT_MEM_msk (0x1)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_IG_PAR_ERR_RSLT_MEM_bit (3)
#define   NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_DESC_MEM (1 << 2)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_DESC_MEM_bf 0, 2, 2
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_DESC_MEM_msk (0x1)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_DESC_MEM_bit (2)
#define   NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_RSLT1_MEM (1 << 1)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_RSLT1_MEM_bf 0, 1, 1
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_RSLT1_MEM_msk (0x1)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_RSLT1_MEM_bit (1)
#define   NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_RSLT0_MEM (1 << 0)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_RSLT0_MEM_bf 0, 0, 0
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_RSLT0_MEM_msk (0x1)
#define     NFP_MAC_CSR_PARITY_ERR_INJECT_INJECT_EG_PAR_ERR_RSLT0_MEM_bit (0)


/*
 * Register: IgParityErrStatus
 *   [14:12]   IgParErrType
 *   [10:0]    IgParErrAddr
 *
 * Name(s):
 * <base>.IgParityErrStatus
 */
#define NFP_MAC_CSR_IG_PARITY_ERR_STATUS                   0x0840
#define   NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_TYPE(x) (((x) & 7) << 12)
#define   NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_TYPE_of(x) (((x) >> 12) & 7)
#define     NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_TYPE_bf 0, 14, 12
#define     NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_TYPE_msk (0x7)
#define     NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_TYPE_shf (12)
#define   NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_ADDR(x) (((x) & 0x7ff) << 0)
#define   NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_ADDR_of(x) (((x) >> 0) & 0x7ff)
#define     NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_ADDR_bf 0, 10, 0
#define     NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_ADDR_msk (0x7ff)
#define     NFP_MAC_CSR_IG_PARITY_ERR_STATUS_IG_PAR_ERR_ADDR_shf (0)


/*
 * Register: EgParityErrStatus
 *   [30:28]   EgParErrType1
 *   [25:16]   EgParErrAddr1
 *   [14:12]   EgParErrType0
 *   [9:0]     EgParErrAddr0
 *
 * Name(s):
 * <base>.EgParityErrStatus
 */
#define NFP_MAC_CSR_EG_PARITY_ERR_STATUS                   0x0844
#define   NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE1(x) (((x) & 7) << 28)
#define   NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE1_of(x) (((x) >> 28) & 7)
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE1_bf 0, 30, 28
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE1_msk (0x7)
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE1_shf (28)
#define   NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR1(x) (((x) & 0x3ff) << 16)
#define   NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR1_of(x) (((x) >> 16) & 0x3ff)
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR1_bf 0, 25, 16
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR1_msk (0x3ff)
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR1_shf (16)
#define   NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE0(x) (((x) & 7) << 12)
#define   NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE0_of(x) (((x) >> 12) & 7)
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE0_bf 0, 14, 12
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE0_msk (0x7)
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_TYPE0_shf (12)
#define   NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR0(x) (((x) & 0x3ff) << 0)
#define   NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR0_of(x) (((x) >> 0) & 0x3ff)
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR0_bf 0, 9, 0
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR0_msk (0x3ff)
#define     NFP_MAC_CSR_EG_PARITY_ERR_STATUS_EG_PAR_ERR_ADDR0_shf (0)


/*
 * Register: MemErrDropCounts
 *   [31:24]   IgMemErrDrop1
 *   [23:16]   IgMemErrDrop0
 *   [15:8]    EgMemErrDrop1
 *   [7:0]     EgMemErrDrop0
 *
 * Name(s):
 * <base>.MemErrDropCounts
 */
#define NFP_MAC_CSR_MEM_ERR_DROP_COUNTS                    0x0848
#define   NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_1(x) (((x) & 0xff) << 24)
#define   NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_1_of(x) (((x) >> 24) & 0xff)
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_1_bf 0, 31, 24
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_1_msk (0xff)
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_1_shf (24)
#define   NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_0(x) (((x) & 0xff) << 16)
#define   NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_0_of(x) (((x) >> 16) & 0xff)
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_0_bf 0, 23, 16
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_0_msk (0xff)
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_IG_MEM_ERR_DROP_0_shf (16)
#define   NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_1(x) (((x) & 0xff) << 8)
#define   NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_1_of(x) (((x) >> 8) & 0xff)
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_1_bf 0, 15, 8
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_1_msk (0xff)
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_1_shf (8)
#define   NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_0(x) (((x) & 0xff) << 0)
#define   NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_0_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_0_bf 0, 7, 0
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_0_msk (0xff)
#define     NFP_MAC_CSR_MEM_ERR_DROP_COUNTS_EG_MEM_ERR_DROP_0_shf (0)


/*
 * Register: AssertConfig0
 *   [31:0]    AssertConfigCsr0
 *
 * Name(s):
 * <base>.AssertConfig0
 */
#define NFP_MAC_CSR_ASSERT_CONFIG_CSR0                     0x084c
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR0_ASSERT_CONFIG_CSR0(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR0_ASSERT_CONFIG_CSR0_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR0_ASSERT_CONFIG_CSR0_bf 0, 31, 0
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR0_ASSERT_CONFIG_CSR0_msk (0xffffffff)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR0_ASSERT_CONFIG_CSR0_shf (0)


/*
 * Register: AssertConfig1
 *   [31:18]   AssertConfigCsr1Disable
 *   [17]      AssertConfigCsr1EnaFsm1
 *   [16]      AssertConfigCsr1EnaFsm0
 *   [15:8]    AssertConfigCsr1FsmCfg1
 *   [7:0]     AssertConfigCsr1FsmCfg0
 *
 * Name(s):
 * <base>.AssertConfig1
 */
#define NFP_MAC_CSR_ASSERT_CONFIG_CSR1                     0x0850
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_DISABLE(x) (((x) & 0x3fff) << 18)
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_DISABLE_of(x) (((x) >> 18) & 0x3fff)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_DISABLE_bf 0, 31, 18
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_DISABLE_msk (0x3fff)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_DISABLE_shf (18)
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_ENA_FSM1 (1 << 17)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_ENA_FSM1_bf 0, 17, 17
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_ENA_FSM1_msk (0x1)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_ENA_FSM1_bit (17)
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_ENA_FSM0 (1 << 16)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_ENA_FSM0_bf 0, 16, 16
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_ENA_FSM0_msk (0x1)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_ENA_FSM0_bit (16)
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG1(x) (((x) & 0xff) << 8)
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG1_of(x) (((x) >> 8) & 0xff)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG1_bf 0, 15, 8
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG1_msk (0xff)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG1_shf (8)
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG0(x) (((x) & 0xff) << 0)
#define   NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG0_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG0_bf 0, 7, 0
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG0_msk (0xff)
#define     NFP_MAC_CSR_ASSERT_CONFIG_CSR1_ASSERT_CONFIG_CSR1_FSM_CFG0_shf (0)


//pyexec<dump_map(InterruptManagerMap(altname="MAC_INTR_MNG"), docs=True)>
/* MacEthSegment: <base>.MacEthSeg0... */
#define NFP_MAC_ETH_MAC_SEG(x)                             (0x0000 + ((x) * 0x400))
/* MacEthGlobal: <base>.MacEthGlobal */
#define NFP_MAC_ETH_MAC_ETH_GLOBAL                         0x3000
/* MacEthMdioCtl: <base>.MacEthMdioCtl */
#define NFP_MAC_ETH_MAC_ETH_MDIO_CTL                       0x3400
/* MacEthVlanTpidCfg: <base>.MacEthVlanTpidCfg */
#define NFP_MAC_ETH_MAC_ETH_VLAN_TPID_CFG                  0x3800
/* MacEthChPcs: <base>.MacEthChPcsSeg0... */
#define NFP_MAC_ETH_MAC_ETH_CHN_PCS_SEG(x)                 (0x4000 + ((x) * 0x400))
/* MacEthAutoNeg: <base>.MacEthAutoNeg */
#define NFP_MAC_ETH_MAC_ETH_AUTO_NEG                       0x7000
/* MacEthFecLT: <base>.MacEthFecLT */
#define NFP_MAC_ETH_MAC_ETH_FEC_LT                         0x7400
/* MacEthPrbs: <base>.MacEthPrbs */
#define NFP_MAC_ETH_MAC_ETH_PRBS                           0x7800


/*
 * Register: EthRevision
 *   [31:16]   EthCustVer
 *   [15:8]    EthCoreVer
 *   [7:0]     EthCoreRev
 *
 * Name(s):
 * <base>.EthRevision
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_REVISON                    0x0000
#define   NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CUST_VER(x)    (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CUST_VER_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CUST_VER_bf  0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CUST_VER_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CUST_VER_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_VER(x)    (((x) & 0xff) << 8)
#define   NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_VER_of(x) (((x) >> 8) & 0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_VER_bf  0, 15, 8
#define     NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_VER_msk (0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_VER_shf (8)
#define   NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_REV(x)    (((x) & 0xff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_REV_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_REV_bf  0, 7, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_REV_msk (0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_REVISON_ETH_CORE_REV_shf (0)


/*
 * Register: EthScratch
 *   [31:0]    EthScratch
 *
 * Name(s):
 * <base>.EthScratch
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_SCRATCH                    0x0004
#define   NFP_MAC_ETH_MAC_SEG_ETH_SCRATCH_ETH_SCRATCH(x)     (((x) & 0xffffffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SCRATCH_ETH_SCRATCH_of(x)  (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SCRATCH_ETH_SCRATCH_bf   0, 31, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_SCRATCH_ETH_SCRATCH_msk  (0xffffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SCRATCH_ETH_SCRATCH_shf  (0)


/*
 * Register: EthCmdConfig
 *   [29]      EthForceSendRf
 *   [28]      EthForceSendLf
 *   [27]      EthDisableFltHdl
 *   [22]      EthTxFlush
 *   [21]      EthRxSfdAny
 *   [20]      EthPausePfcComp
 *   [19]      EthPfcMode
 *   [18]      EthRsColCntExt
 *   [17]      EthNoLgthCheck
 *   [16]      EthSendIdle
 *   [15]      EthPhyTxEna
 *   [14]      EthRxErrDiscard
 *   [13]      EthCmdFrameEna
 *   [12]      EthSwReset
 *   [11]      EthTxPadEn
 *   [10]      EthLoopBackEn
 *   [9]       EthTxAddrIns
 *   [8]       EthPauseIgnore
 *   [7]       EthPauseFwd
 *   [6]       EthCrcFwd
 *   [5]       EthPadEn
 *   [4]       EthPromisEn
 *   [3]       EthWanMode
 *   [1]       EthRxEna
 *   [0]       EthTxEna
 *
 * Name(s):
 * <base>.EthCmdConfig
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG                 0x0008
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_FORCE_SEND_RF (1 << 29)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_FORCE_SEND_RF_bf 0, 29, 29
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_FORCE_SEND_RF_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_FORCE_SEND_RF_bit (29)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_FORCE_SEND_LF (1 << 28)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_FORCE_SEND_LF_bf 0, 28, 28
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_FORCE_SEND_LF_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_FORCE_SEND_LF_bit (28)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_DISABLE_FLT_HDL (1 << 27)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_DISABLE_FLT_HDL_bf 0, 27, 27
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_DISABLE_FLT_HDL_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_DISABLE_FLT_HDL_bit (27)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_FLUSH    (1 << 22)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_FLUSH_bf 0, 22, 22
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_FLUSH_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_FLUSH_bit (22)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_SFD_ANY  (1 << 21)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_SFD_ANY_bf 0, 21, 21
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_SFD_ANY_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_SFD_ANY_bit (21)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_PFC_COMP (1 << 20)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_PFC_COMP_bf 0, 20, 20
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_PFC_COMP_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_PFC_COMP_bit (20)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PFC_MODE    (1 << 19)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PFC_MODE_bf 0, 19, 19
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PFC_MODE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PFC_MODE_bit (19)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RS_COL_CNT_EXT (1 << 18)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RS_COL_CNT_EXT_bf 0, 18, 18
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RS_COL_CNT_EXT_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RS_COL_CNT_EXT_bit (18)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_NO_LENGTH_CHECK (1 << 17)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_NO_LENGTH_CHECK_bf 0, 17, 17
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_NO_LENGTH_CHECK_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_NO_LENGTH_CHECK_bit (17)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_SEND_IDLE   (1 << 16)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_SEND_IDLE_bf 0, 16, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_SEND_IDLE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_SEND_IDLE_bit (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PHY_TX_ENABLE (1 << 15)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PHY_TX_ENABLE_bf 0, 15, 15
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PHY_TX_ENABLE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PHY_TX_ENABLE_bit (15)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_ERR_DISCARD (1 << 14)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_ERR_DISCARD_bf 0, 14, 14
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_ERR_DISCARD_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_ERR_DISCARD_bit (14)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_CMD_FRAME_ENA (1 << 13)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_CMD_FRAME_ENA_bf 0, 13, 13
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_CMD_FRAME_ENA_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_CMD_FRAME_ENA_bit (13)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_SW_RESET    (1 << 12)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_SW_RESET_bf 0, 12, 12
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_SW_RESET_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_SW_RESET_bit (12)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_PAD_EN   (1 << 11)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_PAD_EN_bf 0, 11, 11
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_PAD_EN_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_PAD_EN_bit (11)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_LOOPBACK_EN (1 << 10)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_LOOPBACK_EN_bf 0, 10, 10
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_LOOPBACK_EN_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_LOOPBACK_EN_bit (10)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_ADDR_INSERT (1 << 9)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_ADDR_INSERT_bf 0, 9, 9
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_ADDR_INSERT_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_ADDR_INSERT_bit (9)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_IGNORE (1 << 8)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_IGNORE_bf 0, 8, 8
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_IGNORE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_IGNORE_bit (8)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_FWD   (1 << 7)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_FWD_bf 0, 7, 7
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_FWD_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAUSE_FWD_bit (7)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_CRC_FWD     (1 << 6)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_CRC_FWD_bf 0, 6, 6
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_CRC_FWD_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_CRC_FWD_bit (6)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAD_EN      (1 << 5)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAD_EN_bf 0, 5, 5
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAD_EN_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PAD_EN_bit (5)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PROMISCUOUS_EN (1 << 4)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PROMISCUOUS_EN_bf 0, 4, 4
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PROMISCUOUS_EN_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_PROMISCUOUS_EN_bit (4)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_WAN_MODE    (1 << 3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_WAN_MODE_bf 0, 3, 3
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_WAN_MODE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_WAN_MODE_bit (3)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_ENA      (1 << 1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_ENA_bf 0, 1, 1
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_ENA_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_RX_ENA_bit (1)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_ENA      (1 << 0)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_ENA_bf 0, 0, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_ENA_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CMD_CONFIG_ETH_TX_ENA_bit (0)


/*
 * Register: EthMacAddr0
 *   [31:0]    EthMacAddr0
 *
 * Name(s):
 * <base>.EthMacAddr0
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_0                 0x000c
#define   NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_0_ETH_MAC_ADDR_0(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_0_ETH_MAC_ADDR_0_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_0_ETH_MAC_ADDR_0_bf 0, 31, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_0_ETH_MAC_ADDR_0_msk (0xffffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_0_ETH_MAC_ADDR_0_shf (0)


/*
 * Register: EthMacAddr1
 *   [15:0]    EthMacAddr1
 *
 * Name(s):
 * <base>.EthMacAddr1
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_1                 0x0010
#define   NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_1_ETH_MAC_ADDR_1(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_1_ETH_MAC_ADDR_1_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_1_ETH_MAC_ADDR_1_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_1_ETH_MAC_ADDR_1_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_MAC_ADDR_1_ETH_MAC_ADDR_1_shf (0)


/*
 * Register: EthFrmLength
 *   [15:0]    EthFrmLength
 *
 * Name(s):
 * <base>.EthFrmLength
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_FRAME_LENGTH               0x0014
#define   NFP_MAC_ETH_MAC_SEG_ETH_FRAME_LENGTH_ETH_FRAME_LENGTH(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_FRAME_LENGTH_ETH_FRAME_LENGTH_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_FRAME_LENGTH_ETH_FRAME_LENGTH_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_FRAME_LENGTH_ETH_FRAME_LENGTH_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_FRAME_LENGTH_ETH_FRAME_LENGTH_shf (0)


/*
 * Register: EthRxFifoSections
 *   [31:16]   EthRxSectionEmptyWm
 *   [15:0]    EthRxSectionAvailWm
 *
 * Name(s):
 * <base>.EthRxFifoSections
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS           0x001c
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_EMPTY_WM(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_EMPTY_WM_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_EMPTY_WM_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_EMPTY_WM_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_EMPTY_WM_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_AVAIL_WM(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_AVAIL_WM_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_AVAIL_WM_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_AVAIL_WM_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_SECTIONS_ETH_RX_SECTION_AVAIL_WM_shf (0)


/*
 * Register: EthTxFifoSections
 *   [31:16]   EthTxSectionEmptyWm
 *   [15:0]    EthTxSectionAvailWm
 *
 * Name(s):
 * <base>.EthTxFifoSections
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS           0x0020
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_EMPTY_WM(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_EMPTY_WM_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_EMPTY_WM_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_EMPTY_WM_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_EMPTY_WM_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_AVAIL_WM(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_AVAIL_WM_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_AVAIL_WM_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_AVAIL_WM_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_SECTIONS_ETH_TX_SECTION_AVAIL_WM_shf (0)


/*
 * Register: EthRxFifoAlmostFE
 *   [31:16]   EthRxFifoAlmostFullWm
 *   [15:0]    EthRxFifoAlmostEmptyWm
 *
 * Name(s):
 * <base>.EthRxFifoAlmostFE
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY  0x0024
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_FULL_WM(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_FULL_WM_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_FULL_WM_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_FULL_WM_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_FULL_WM_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_EMPTY_WM(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_EMPTY_WM_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_EMPTY_WM_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_EMPTY_WM_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_FIFO_ALMOST_FULL_EMPTY_ETH_RX_FIFO_ALMOST_EMPTY_WM_shf (0)


/*
 * Register: EthTxFifoAlmostFE
 *   [31:16]   EthTxFifoAlmostFullWm
 *   [15:0]    EthTxFifoAlmostEmptyWm
 *
 * Name(s):
 * <base>.EthTxFifoAlmostFE
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY  0x0028
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_FULL_WM(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_FULL_WM_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_FULL_WM_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_FULL_WM_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_FULL_WM_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_EMPTY_WM(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_EMPTY_WM_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_EMPTY_WM_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_EMPTY_WM_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_FIFO_ALMOST_FULL_EMPTY_ETH_TX_FIFO_ALMOST_EMPTY_WM_shf (0)


/*
 * Register: EthHashTableLoad
 *   [8]       EthHashTableMcEn
 *   [5:0]     EthHashTableAddr
 *
 * Name(s):
 * <base>.EthHashTableLoad
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD            0x002c
#define   NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD_ETH_HASH_TABLE_MC_EN (1 << 8)
#define     NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD_ETH_HASH_TABLE_MC_EN_bf 0, 8, 8
#define     NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD_ETH_HASH_TABLE_MC_EN_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD_ETH_HASH_TABLE_MC_EN_bit (8)
#define   NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD_ETH_HASH_TABLE_ADDR(x) (((x) & 0x3f) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD_ETH_HASH_TABLE_ADDR_of(x) (((x) >> 0) & 0x3f)
#define     NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD_ETH_HASH_TABLE_ADDR_bf 0, 5, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD_ETH_HASH_TABLE_ADDR_msk (0x3f)
#define     NFP_MAC_ETH_MAC_SEG_ETH_HASH_TABLE_LOAD_ETH_HASH_TABLE_ADDR_shf (0)


/*
 * Register: EthStatus
 *   [3]       EthTsAvail
 *   [2]       EthPhyLos
 *   [1]       EthRxRemFault
 *   [0]       EthRxLocFault
 *
 * Name(s):
 * <base>.EthStatus
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_STATUS                     0x0040
#define   NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_TS_AVAILABLE    (1 << 3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_TS_AVAILABLE_bf 0, 3, 3
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_TS_AVAILABLE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_TS_AVAILABLE_bit (3)
#define   NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_PHY_LOS         (1 << 2)
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_PHY_LOS_bf    0, 2, 2
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_PHY_LOS_msk   (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_PHY_LOS_bit   (2)
#define   NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_RX_REMOTE_FAULT (1 << 1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_RX_REMOTE_FAULT_bf 0, 1, 1
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_RX_REMOTE_FAULT_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_RX_REMOTE_FAULT_bit (1)
#define   NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_RX_LOCAL_FAULT  (1 << 0)
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_RX_LOCAL_FAULT_bf 0, 0, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_RX_LOCAL_FAULT_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_STATUS_ETH_RX_LOCAL_FAULT_bit (0)


/*
 * Register: EthTxIpgLength
 *   [6:0]     EthTxIpgLength
 *
 * Name(s):
 * <base>.EthTxIpgLength
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_TX_IPG_LENGTH              0x0044
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_IPG_LENGTH_ETH_TX_IPG_LENGTH(x) (((x) & 0x7f) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_IPG_LENGTH_ETH_TX_IPG_LENGTH_of(x) (((x) >> 0) & 0x7f)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_IPG_LENGTH_ETH_TX_IPG_LENGTH_bf 0, 6, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_IPG_LENGTH_ETH_TX_IPG_LENGTH_msk (0x7f)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_IPG_LENGTH_ETH_TX_IPG_LENGTH_shf (0)


/*
 * Register: EthCreditTrigger
 *   [0]       EthCreditTrigger
 *
 * Name(s):
 * <base>.EthCreditTrigger
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_TRIGGER             0x0048
#define   NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_TRIGGER_ETH_CREDIT_TRIGGER (1 << 0)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_TRIGGER_ETH_CREDIT_TRIGGER_bf 0, 0, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_TRIGGER_ETH_CREDIT_TRIGGER_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_TRIGGER_ETH_CREDIT_TRIGGER_bit (0)


/*
 * Register: EthInitCredit
 *   [7:0]     EthInitCredit
 *
 * Name(s):
 * <base>.EthInitCredit
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_INIT_CREDIT                0x004c
#define   NFP_MAC_ETH_MAC_SEG_ETH_INIT_CREDIT_ETH_INIT_CREDIT(x) (((x) & 0xff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_INIT_CREDIT_ETH_INIT_CREDIT_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_INIT_CREDIT_ETH_INIT_CREDIT_bf 0, 7, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_INIT_CREDIT_ETH_INIT_CREDIT_msk (0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_INIT_CREDIT_ETH_INIT_CREDIT_shf (0)


/*
 * Register: EthCreditReg
 *   [7:0]     EthCreditReg
 *
 * Name(s):
 * <base>.EthCreditReg
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_REG                 0x0050
#define   NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_REG_ETH_CREDIT_REG(x) (((x) & 0xff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_REG_ETH_CREDIT_REG_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_REG_ETH_CREDIT_REG_bf 0, 7, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_REG_ETH_CREDIT_REG_msk (0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_CREDIT_REG_ETH_CREDIT_REG_shf (0)


/*
 * Register: EthPauseQuantaCL01
 *   [31:16]   EthPauseQuantaCL1
 *   [15:0]    EthPauseQuantaCL0
 *
 * Name(s):
 * <base>.EthPauseQuantaCL01
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01          0x0054
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL1(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL1_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL1_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL1_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL1_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL0(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL0_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL0_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL0_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL01_ETH_PAUSE_QUANTA_CL0_shf (0)


/*
 * Register: EthPauseQuantaCL23
 *   [31:16]   EthPauseQuantaCL3
 *   [15:0]    EthPauseQuantaCL2
 *
 * Name(s):
 * <base>.EthPauseQuantaCL23
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23          0x0058
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL3(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL3_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL3_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL3_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL3_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL2(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL2_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL2_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL2_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL23_ETH_PAUSE_QUANTA_CL2_shf (0)


/*
 * Register: EthPauseQuantaCL45
 *   [31:16]   EthPauseQuantaCL5
 *   [15:0]    EthPauseQuantaCL4
 *
 * Name(s):
 * <base>.EthPauseQuantaCL45
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45          0x005c
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL5(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL5_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL5_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL5_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL5_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL4(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL4_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL4_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL4_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL45_ETH_PAUSE_QUANTA_CL4_shf (0)


/*
 * Register: EthPauseQuantaCL67
 *   [31:16]   EthPauseQuantaCL7
 *   [15:0]    EthPauseQuantaCL6
 *
 * Name(s):
 * <base>.EthPauseQuantaCL67
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67          0x0060
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL7(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL7_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL7_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL7_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL7_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL6(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL6_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL6_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL6_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PAUSE_QUANTA_CL67_ETH_PAUSE_QUANTA_CL6_shf (0)


/*
 * Register: EthQuantaThreshCL01
 *   [31:16]   EthQuantaThreshCL1
 *   [15:0]    EthQuantaThreshCL0
 *
 * Name(s):
 * <base>.EthQuantaThreshCL01
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01         0x0064
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL1(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL1_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL1_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL1_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL1_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL0(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL0_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL0_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL0_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL01_ETH_QUANTA_THRESH_CL0_shf (0)


/*
 * Register: EthQuantaThreshCL23
 *   [31:16]   EthQuantaThreshCL3
 *   [15:0]    EthQuantaThreshCL2
 *
 * Name(s):
 * <base>.EthQuantaThreshCL23
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23         0x0068
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL3(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL3_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL3_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL3_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL3_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL2(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL2_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL2_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL2_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL23_ETH_QUANTA_THRESH_CL2_shf (0)


/*
 * Register: EthQuantaThreshCL45
 *   [31:16]   EthQuantaThreshCL5
 *   [15:0]    EthQuantaThreshCL4
 *
 * Name(s):
 * <base>.EthQuantaThreshCL45
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45         0x006c
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL5(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL5_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL5_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL5_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL5_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL4(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL4_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL4_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL4_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL45_ETH_QUANTA_THRESH_CL4_shf (0)


/*
 * Register: EthQuantaThreshCL67
 *   [31:16]   EthQuantaThreshCL7
 *   [15:0]    EthQuantaThreshCL6
 *
 * Name(s):
 * <base>.EthQuantaThreshCL67
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67         0x0070
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL7(x) (((x) & 0xffff) << 16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL7_of(x) (((x) >> 16) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL7_bf 0, 31, 16
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL7_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL7_shf (16)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL6(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL6_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL6_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL6_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_QUANTA_THRESH_CL67_ETH_QUANTA_THRESH_CL6_shf (0)


/*
 * Register: EthRxPauseStatus
 *   [7:0]     EthRxPauseStatus
 *
 * Name(s):
 * <base>.EthRxPauseStatus
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_RX_PAUSE_STATUS            0x0074
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_PAUSE_STATUS_ETH_RX_PAUSE_STATUS(x) (((x) & 0xff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_RX_PAUSE_STATUS_ETH_RX_PAUSE_STATUS_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_PAUSE_STATUS_ETH_RX_PAUSE_STATUS_bf 0, 7, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_PAUSE_STATUS_ETH_RX_PAUSE_STATUS_msk (0xff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_RX_PAUSE_STATUS_ETH_RX_PAUSE_STATUS_shf (0)


/*
 * Register: EthTimestamp
 *   [31:0]    EthTimestamp
 *
 * Name(s):
 * <base>.EthTimestamp
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_TIMESTAMP                  0x007c
#define   NFP_MAC_ETH_MAC_SEG_ETH_TIMESTAMP_ETH_TIMESTAMP(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TIMESTAMP_ETH_TIMESTAMP_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TIMESTAMP_ETH_TIMESTAMP_bf 0, 31, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_TIMESTAMP_ETH_TIMESTAMP_msk (0xffffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TIMESTAMP_ETH_TIMESTAMP_shf (0)


/*
 * Register: EthTxPreamble0
 *   [31:0]    EthTxPreamble0
 *
 * Name(s):
 * <base>.EthTxPreamble0
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_0              0x0280
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_0_ETH_TX_PREAMBLE_0(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_0_ETH_TX_PREAMBLE_0_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_0_ETH_TX_PREAMBLE_0_bf 0, 31, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_0_ETH_TX_PREAMBLE_0_msk (0xffffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_0_ETH_TX_PREAMBLE_0_shf (0)


/*
 * Register: EthTxPreamble1
 *   [23:0]    EthTxPreamble1
 *
 * Name(s):
 * <base>.EthTxPreamble1
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_1              0x0284
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_1_ETH_TX_PREAMBLE_1(x) (((x) & 0xffffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_1_ETH_TX_PREAMBLE_1_of(x) (((x) >> 0) & 0xffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_1_ETH_TX_PREAMBLE_1_bf 0, 23, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_1_ETH_TX_PREAMBLE_1_msk (0xffffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_TX_PREAMBLE_1_ETH_TX_PREAMBLE_1_shf (0)


/*
 * Register: EthSgmiiPcsCtl
 *   [15]      EthPcsReset
 *   [14]      EthPhyLoopBack
 *   [13]      EthSgmiiSpeedSel0
 *   [12]      EthAutoNegEnable
 *   [11]      EthPowerDown
 *   [10]      EthIsolate
 *   [9]       EthRestartAutoNeg
 *   [8]       EthDuplexMode
 *   [7]       EthCollisionTest
 *   [6]       EthSgmiiSpeedSel1
 *
 * Name(s):
 * <base>.EthSgmiiPcsCtl
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL              0x0300
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_PCS_RESET (1 << 15)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_PCS_RESET_bf 0, 15, 15
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_PCS_RESET_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_PCS_RESET_bit (15)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_PHY_LOOPBACK (1 << 14)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_PHY_LOOPBACK_bf 0, 14, 14
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_PHY_LOOPBACK_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_PHY_LOOPBACK_bit (14)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_SGMII_SPEED_SEL_0 (1 << 13)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_SGMII_SPEED_SEL_0_bf 0, 13, 13
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_SGMII_SPEED_SEL_0_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_SGMII_SPEED_SEL_0_bit (13)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_AUTONEG_ENABLE (1 << 12)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_AUTONEG_ENABLE_bf 0, 12, 12
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_AUTONEG_ENABLE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_AUTONEG_ENABLE_bit (12)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_POWER_DOWN (1 << 11)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_POWER_DOWN_bf 0, 11, 11
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_POWER_DOWN_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_POWER_DOWN_bit (11)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_ISOLATE  (1 << 10)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_ISOLATE_bf 0, 10, 10
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_ISOLATE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_ISOLATE_bit (10)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_RESTART_AUTONEG (1 << 9)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_RESTART_AUTONEG_bf 0, 9, 9
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_RESTART_AUTONEG_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_RESTART_AUTONEG_bit (9)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_DUPLEX_MODE (1 << 8)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_DUPLEX_MODE_bf 0, 8, 8
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_DUPLEX_MODE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_DUPLEX_MODE_bit (8)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_COLLISION_TEST (1 << 7)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_COLLISION_TEST_bf 0, 7, 7
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_COLLISION_TEST_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_COLLISION_TEST_bit (7)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_SGMII_SPEED_SEL_1 (1 << 6)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_SGMII_SPEED_SEL_1_bf 0, 6, 6
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_SGMII_SPEED_SEL_1_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_CTL_ETH_SGMII_SPEED_SEL_1_bit (6)


/*
 * Register: EthSgmiiPcsStatus
 *   [15]      Eth100T4
 *   [14:13]   Eth100XHalfDuplex
 *   [12:11]   Eth10MbHalfDuplex
 *   [10:9]    Eth100T2HalfDuplex
 *   [8]       EthExtendedStatus
 *   [5]       EthAutoNegComplete
 *   [4]       EthRemoteFault
 *   [3]       EthAutoNegAbility
 *   [2]       EthLinkStatus
 *   [1]       EthJabberDetect
 *   [0]       EthExtCapable
 *
 * Name(s):
 * <base>.EthSgmiiPcsStatus
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS           0x0304
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASET4 (1 << 15)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASET4_bf 0, 15, 15
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASET4_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASET4_bit (15)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASEX_HALF_DUPLEX(x) (((x) & 3) << 13)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASEX_HALF_DUPLEX_of(x) (((x) >> 13) & 3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASEX_HALF_DUPLEX_bf 0, 14, 13
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASEX_HALF_DUPLEX_msk (0x3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASEX_HALF_DUPLEX_shf (13)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_10M_HALF_DUPLEX(x) (((x) & 3) << 11)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_10M_HALF_DUPLEX_of(x) (((x) >> 11) & 3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_10M_HALF_DUPLEX_bf 0, 12, 11
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_10M_HALF_DUPLEX_msk (0x3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_10M_HALF_DUPLEX_shf (11)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASET2_HALF_DUPLEX(x) (((x) & 3) << 9)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASET2_HALF_DUPLEX_of(x) (((x) >> 9) & 3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASET2_HALF_DUPLEX_bf 0, 10, 9
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASET2_HALF_DUPLEX_msk (0x3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_100BASET2_HALF_DUPLEX_shf (9)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_EXTENDED_STATUS (1 << 8)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_EXTENDED_STATUS_bf 0, 8, 8
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_EXTENDED_STATUS_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_EXTENDED_STATUS_bit (8)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_AUTONEG_COMPLETE (1 << 5)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_AUTONEG_COMPLETE_bf 0, 5, 5
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_AUTONEG_COMPLETE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_AUTONEG_COMPLETE_bit (5)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_REMOTE_FAULT (1 << 4)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_REMOTE_FAULT_bf 0, 4, 4
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_REMOTE_FAULT_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_REMOTE_FAULT_bit (4)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_AUTONEG_ABILITY (1 << 3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_AUTONEG_ABILITY_bf 0, 3, 3
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_AUTONEG_ABILITY_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_AUTONEG_ABILITY_bit (3)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_LINK_STATUS (1 << 2)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_LINK_STATUS_bf 0, 2, 2
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_LINK_STATUS_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_LINK_STATUS_bit (2)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_JABBER_DETECT (1 << 1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_JABBER_DETECT_bf 0, 1, 1
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_JABBER_DETECT_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_JABBER_DETECT_bit (1)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_EXT_CAPABLE (1 << 0)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_EXT_CAPABLE_bf 0, 0, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_EXT_CAPABLE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PCS_STATUS_ETH_EXT_CAPABLE_bit (0)


/*
 * Register: EthSgmiiPhyIdent0
 *   [15:0]    EthPhyIdentifier0
 *
 * Name(s):
 * <base>.EthSgmiiPhyIdent0
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT0           0x0308
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT0_ETH_PHY_IDENTIFIER0(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT0_ETH_PHY_IDENTIFIER0_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT0_ETH_PHY_IDENTIFIER0_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT0_ETH_PHY_IDENTIFIER0_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT0_ETH_PHY_IDENTIFIER0_shf (0)


/*
 * Register: EthSgmiiPhyIdent1
 *   [15:0]    EthPhyIdentifier1
 *
 * Name(s):
 * <base>.EthSgmiiPhyIdent1
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT1           0x030c
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT1_ETH_PHY_IDENTIFIER1(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT1_ETH_PHY_IDENTIFIER1_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT1_ETH_PHY_IDENTIFIER1_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT1_ETH_PHY_IDENTIFIER1_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_SGMII_PHY_IDENT1_ETH_PHY_IDENTIFIER1_shf (0)


/*
 * Register: EthSgmiiDevAbility
 *   [15]      EthNextPageCapable
 *   [14]      EthACK
 *   [13:12]   EthRemoteFault
 *   [8]       EthPS2
 *   [7]       EthPS1
 *   [6]       EthHalfDuplex
 *   [5]       EthFullDuplex
 *
 * Name(s):
 * <base>.EthSgmiiDevAbility
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY                0x0310
#define   NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_NP         (1 << 15)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_NP_bf    0, 15, 15
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_NP_msk   (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_NP_bit   (15)
#define   NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_ACK        (1 << 14)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_ACK_bf   0, 14, 14
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_ACK_msk  (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_ACK_bit  (14)
#define   NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_REMOTE_FAULT(x) (((x) & 3) << 12)
#define   NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_REMOTE_FAULT_of(x) (((x) >> 12) & 3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_REMOTE_FAULT_no error (0)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_REMOTE_FAULT_link failure (1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_REMOTE_FAULT_offline (2)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_REMOTE_FAULT_autonegotiation error (3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_REMOTE_FAULT_bf 0, 13, 12
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_REMOTE_FAULT_msk (0x3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_REMOTE_FAULT_shf (12)
#define   NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_PAUSE_SUPPORT2 (1 << 8)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_PAUSE_SUPPORT2_bf 0, 8, 8
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_PAUSE_SUPPORT2_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_PAUSE_SUPPORT2_bit (8)
#define   NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_PAUSE_SUPPORT1 (1 << 7)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_PAUSE_SUPPORT1_bf 0, 7, 7
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_PAUSE_SUPPORT1_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_PAUSE_SUPPORT1_bit (7)
#define   NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_HALF_DUPLEX (1 << 6)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_HALF_DUPLEX_bf 0, 6, 6
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_HALF_DUPLEX_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_HALF_DUPLEX_bit (6)
#define   NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_FULL_DUPLEX (1 << 5)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_FULL_DUPLEX_bf 0, 5, 5
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_FULL_DUPLEX_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_DEV_ABILITY_ETH_FULL_DUPLEX_bit (5)


/*
 * Register: EthSgmiiPartnerAbility
 *   [15]      EthCopperLinkStatus
 *   [14]      EthACK
 *   [12]      EthCopperDuplexStatus
 *   [11:10]   EthCopperSpeed
 *
 * Name(s):
 * <base>.EthSgmiiPartnerAbility
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY            0x0314
#define   NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_LINK_STATUS (1 << 15)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_LINK_STATUS_bf 0, 15, 15
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_LINK_STATUS_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_LINK_STATUS_bit (15)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_ACK    (1 << 14)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_ACK_bf 0, 14, 14
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_ACK_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_ACK_bit (14)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_DUPLEX_STATUS (1 << 12)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_DUPLEX_STATUS_bf 0, 12, 12
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_DUPLEX_STATUS_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_DUPLEX_STATUS_bit (12)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_SPEED(x) (((x) & 3) << 10)
#define   NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_SPEED_of(x) (((x) >> 10) & 3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_SPEED_Cu Speed (0)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_SPEED_Cu Speed (1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_SPEED_Cu Speed (2)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_SPEED_bf 0, 11, 10
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_SPEED_msk (0x3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_ABILITY_ETH_COPPER_SPEED_shf (10)


/*
 * Register: EthSgmiiAnExpansion
 *   [1]       EthLatchedHiPageRcvd
 *   [0]       EthRealTimePageRcvd
 *
 * Name(s):
 * <base>.EthSgmiiAnExpansion
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_AN_EXPANSION               0x0318
#define   NFP_MAC_ETH_MAC_SEG_ETH_AN_EXPANSION_ETH_LATCHED_PAGE_RCVD (1 << 1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_AN_EXPANSION_ETH_LATCHED_PAGE_RCVD_bf 0, 1, 1
#define     NFP_MAC_ETH_MAC_SEG_ETH_AN_EXPANSION_ETH_LATCHED_PAGE_RCVD_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_AN_EXPANSION_ETH_LATCHED_PAGE_RCVD_bit (1)
#define   NFP_MAC_ETH_MAC_SEG_ETH_AN_EXPANSION_ETH_REAL_TIME_PAGE_RCVD (1 << 0)
#define     NFP_MAC_ETH_MAC_SEG_ETH_AN_EXPANSION_ETH_REAL_TIME_PAGE_RCVD_bf 0, 0, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_AN_EXPANSION_ETH_REAL_TIME_PAGE_RCVD_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_AN_EXPANSION_ETH_REAL_TIME_PAGE_RCVD_bit (0)


/*
 * Register: EthSgmiiNoSupport
 *
 * Name(s):
 * <base>.EthSgmiiDeviceNextPage <base>.EthSgmiiPartnerNextPage
 * <base>.EthSgmiiExtendedStatus
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_DEVICE_NEXT_PAGE           0x031c
#define NFP_MAC_ETH_MAC_SEG_ETH_PARTNER_NEXT_PAGE          0x0320
#define NFP_MAC_ETH_MAC_SEG_ETH_EXTENDED_STATUS            0x033c


/*
 * Register: EthSgmiiLinkTimerLo
 *   [15:0]    EthLinkTimerLo
 *
 * Name(s):
 * <base>.EthSgmiiLinkTimerLo
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_LO              0x0348
#define   NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_LO_ETH_LINK_TIMER_LO(x) (((x) & 0xffff) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_LO_ETH_LINK_TIMER_LO_of(x) (((x) >> 0) & 0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_LO_ETH_LINK_TIMER_LO_bf 0, 15, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_LO_ETH_LINK_TIMER_LO_msk (0xffff)
#define     NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_LO_ETH_LINK_TIMER_LO_shf (0)


/*
 * Register: EthSgmiiLinkTimerHi
 *   [4:0]     EthLinkTimerHi
 *
 * Name(s):
 * <base>.EthSgmiiLinkTimerHi
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_HI              0x034c
#define   NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_HI_ETH_LINK_TIMER_HI(x) (((x) & 0x1f) << 0)
#define   NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_HI_ETH_LINK_TIMER_HI_of(x) (((x) >> 0) & 0x1f)
#define     NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_HI_ETH_LINK_TIMER_HI_bf 0, 4, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_HI_ETH_LINK_TIMER_HI_msk (0x1f)
#define     NFP_MAC_ETH_MAC_SEG_ETH_LINK_TIMER_HI_ETH_LINK_TIMER_HI_shf (0)


/*
 * Register: EthSgmiiIfMode
 *   [5]       EthSgmiiPcsEnable
 *   [4]       EthSgmiiHDuplex
 *   [3:2]     EthSgmiiSpeed
 *   [1]       EthUseSgmiiAn
 *   [0]       EthSgmiiEna
 *
 * Name(s):
 * <base>.EthSgmiiIfMode
 */
#define NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE                    0x0350
#define   NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_PCS_ENABLE (1 << 5)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_PCS_ENABLE_bf 0, 5, 5
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_PCS_ENABLE_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_PCS_ENABLE_bit (5)
#define   NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_HDUPLEX  (1 << 4)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_HDUPLEX_bf 0, 4, 4
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_HDUPLEX_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_HDUPLEX_bit (4)
#define   NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_SPEED(x) (((x) & 3) << 2)
#define   NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_SPEED_of(x) (((x) >> 2) & 3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_SPEED_SGMII Speed (0)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_SPEED_SGMII Speed (1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_SPEED_SGMII Speed (2)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_SPEED_bf 0, 3, 2
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_SPEED_msk (0x3)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_SPEED_shf (2)
#define   NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_USE_SGMII_AN   (1 << 1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_USE_SGMII_AN_bf 0, 1, 1
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_USE_SGMII_AN_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_USE_SGMII_AN_bit (1)
#define   NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_ENA      (1 << 0)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_ENA_bf 0, 0, 0
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_ENA_msk (0x1)
#define     NFP_MAC_ETH_MAC_SEG_ETH_IF_MODE_ETH_SGMII_ENA_bit (0)



/* EthPortStatsHyd: <base>.MacEthPortStatsHyd0... */
#define NFP_MAC_STATS_MAP_MAC_ETH_PORT_STATS_HY(x)         (0x0000 + ((x) * 0x1000))
/* ChannelStatsSeg: <base>.MacChannelStats0... */
#define NFP_MAC_STATS_MAP_MAC_CHNL_STATS(x)                (0x10000 + ((x) * 0x100))

/* EthPortStatsSeg: <base>.MacEthPortStatsSeg0... */
#define NFP_MAC_ETH_PORT_STATS_HY_MAC_ETH_PORT_STATS_SEG(x) (0x0000 + ((x) * 0x100))


/*
 * Register: StatCounter32
 *   [31:0]    StatCounter32
 *
 * Name(s):
 * <base>.RxPIfInOctetsLo <base>.RxFrameTooLongErrors
 * <base>.RxInRangeLengthErrors <base>.RxVlanReceivedOK <base>.RxPIfInErrors
 * <base>.RxPIfInBroadCastPkts <base>.RxPStatsDropEvents
 * <base>.RxAlignmentErrors <base>.RxPauseMacCtlFramesReceived
 * <base>.RxFramesReceivedOK <base>.RxFrameCheckSequenceErrors
 * <base>.RxPIfInUniCastPkts <base>.RxPIfInMultiCastPkts <base>.RxPStatsPkts
 * <base>.RxPStatsUndersizePkts <base>.RxPStatsPkts64octets
 * <base>.RxPStatsPkts65to127octets <base>.RxPStatsPkts512to1023octets
 * <base>.RxPStatsPkts1024to1518octets <base>.RxPStatsJabbers
 * <base>.RxPStatsFragments <base>.RxCBFCPauseFramesReceived2
 * <base>.RxCBFCPauseFramesReceived3 <base>.RxPStatsPkts128to255octets
 * <base>.RxPStatsPkts256to511octets <base>.RxPStatsPkts1519toMaxoctets
 * <base>.RxPStatsOversizePkts <base>.RxCBFCPauseFramesReceived0
 * <base>.RxCBFCPauseFramesReceived1 <base>.RxCBFCPauseFramesReceived4
 * <base>.RxCBFCPauseFramesReceived5 <base>.RxCBFCPauseFramesReceived6
 * <base>.RxCBFCPauseFramesReceived7 <base>.RxMacCtlFramesReceived
 * <base>.TxPIfOutOctetsLo <base>.TxVlanTransmittedOK <base>.TxPIfOutErrors
 * <base>.TxPIfOutBroadCastPkts <base>.TxPStatsPkts64octets
 * <base>.TxPStatsPkts256to511octets <base>.TxPStatsPkts512to1023octets
 * <base>.TxPauseMacCtlFramesTransmitted <base>.TxFramesTransmittedOK
 * <base>.TxPIfOutUniCastPkts <base>.TxPIfOutMultiCastPkts
 * <base>.TxPStatsPkts65to127octets <base>.TxPStatsPkts128to255octets
 * <base>.TxPStatsPkts1024to1518octets <base>.TxPStatsPkts1518toMAXoctets
 * <base>.TxCBFCPauseFramesTransmitted0 <base>.TxCBFCPauseFramesTransmitted1
 * <base>.TxCBFCPauseFramesTransmitted4 <base>.TxCBFCPauseFramesTransmitted5
 * <base>.TxCBFCPauseFramesTransmitted2 <base>.TxCBFCPauseFramesTransmitted3
 * <base>.TxCBFCPauseFramesTransmitted6 <base>.TxCBFCPauseFramesTransmitted7
 */
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_LO     0x0000
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_FRAME_TOO_LONG_ERRORS 0x0008
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_IN_RANGE_LENGTH_ERRORS 0x000c
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_VLAN_RECEIVED_OK     0x0010
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_ERRORS        0x0014
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_BROADCAST_PKTS 0x0018
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_DROP_EVENTS   0x001c
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_ALIGNMENT_ERRORS     0x0020
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PAUSE_MAC_CTL_FRAMES_RECEIVED 0x0024
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_FRAMES_RECEIVED_OK   0x0028
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_FRAME_CHECK_SEQUENCE_ERRORS 0x002c
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_UNICAST_PKTS  0x0030
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_MULTICAST_PKTS 0x0034
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_PKTS          0x0038
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_UNDERSIZE_PKTS 0x003c
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_PKTS_64_OCTETS 0x0040
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_PKTS_65_127_OCTETS 0x0044
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_PKTS_512_1023_OCTETS 0x0048
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_PKTS_1024_1518_OCTETS 0x004c
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_JABBERS       0x0050
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_FRAGMENTS     0x0054
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_CBFC_PAUSE_FRAMES_RECEIVED_2 0x0058
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_CBFC_PAUSE_FRAMES_RECEIVED_3 0x005c
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_PKTS_128_255_OCTETS 0x0060
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_PKTS_256_511_OCTETS 0x0064
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_PKTS_1519_MAX_OCTETS 0x0068
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PSTATS_OVERSIZE_PKTS 0x006c
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_CBFC_PAUSE_FRAMES_RECEIVED_0 0x0070
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_CBFC_PAUSE_FRAMES_RECEIVED_1 0x0074
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_CBFC_PAUSE_FRAMES_RECEIVED_4 0x0078
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_CBFC_PAUSE_FRAMES_RECEIVED_5 0x007c
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_CBFC_PAUSE_FRAMES_RECEIVED_6 0x0080
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_CBFC_PAUSE_FRAMES_RECEIVED_7 0x0084
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_MAC_CTL_FRAMES_RECEIVED 0x0088
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PIF_OCTETS_LO        0x00a0
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_VLAN_TRANSMITTED_OK  0x00a8
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PIF_ERRORS           0x00ac
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PIF_BROADCAST_PKTS   0x00b0
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PSTATS_PKTS_64_OCTETS 0x00b4
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PSTATS_PKTS_256_511_OCTETS 0x00b8
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PSTATS_PKTS_512_1023_OCTETS 0x00bc
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PAUSE_MAC_CTL_FRAMES_TRANSMITTED 0x00c0
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_FRAMED_TRANSMITTED_OK 0x00c4
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PIF_UNICAST_PKTS     0x00c8
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PIF_MULTICAST_PKTS   0x00cc
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PSTATS_PKTS_65_127_OCTETS 0x00d0
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PSTATS_PKTS_128_255_OCTETS 0x00d4
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PSTATS_PKTS_1024_1518_OCTETS 0x00d8
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PSTATS_PKTS_1519_MAX_OCTETS 0x00dc
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_CBFC_PAUSE_FRAMES_TRANSMITTED_0 0x00e0
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_CBFC_PAUSE_FRAMES_TRANSMITTED_1 0x00e4
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_CBFC_PAUSE_FRAMES_TRANSMITTED_4 0x00e8
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_CBFC_PAUSE_FRAMES_TRANSMITTED_5 0x00ec
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_CBFC_PAUSE_FRAMES_TRANSMITTED_2 0x00f0
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_CBFC_PAUSE_FRAMES_TRANSMITTED_3 0x00f4
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_CBFC_PAUSE_FRAMES_TRANSMITTED_6 0x00f8
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_CBFC_PAUSE_FRAMES_TRANSMITTED_7 0x00fc
#define   NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_LO_STAT_COUNTER_32(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_LO_STAT_COUNTER_32_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_LO_STAT_COUNTER_32_bf 0, 31, 0
#define     NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_LO_STAT_COUNTER_32_msk (0xffffffff)
#define     NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_LO_STAT_COUNTER_32_shf (0)


/*
 * Register: StatCounter8
 *   [7:0]     StatCounter8
 *
 * Name(s):
 * <base>.RxPIfInOctetsHi <base>.TxPIfOutOctetsHi
 */
#define NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_HI     0x0004
#define NFP_MAC_ETH_PORT_STATS_SEG_TX_PIF_OCTETS_HI        0x00a4
#define   NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_HI_STAT_COUNTER_8(x) (((x) & 0xff) << 0)
#define   NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_HI_STAT_COUNTER_8_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_HI_STAT_COUNTER_8_bf 0, 7, 0
#define     NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_HI_STAT_COUNTER_8_msk (0xff)
#define     NFP_MAC_ETH_PORT_STATS_SEG_RX_PIF_IN_OCTETS_HI_STAT_COUNTER_8_shf (0)



/*
 * Register: StatCounter32
 *   [31:0]    StatCounter32
 *
 * Name(s):
 * <base>.RxCIfInOctetsLo <base>.RxCStatsOctetsLo <base>.RxCIfInErrors
 * <base>.RxCIfInUniCastPkts <base>.RxCIfInMultiCastPkts
 * <base>.RxCIfInBroadCastPkts <base>.RxCStatsPkts <base>.RxCStatsPkts64octets
 * <base>.RxCStatsPkts65to127octets <base>.RxCStatsPkts128to255octets
 * <base>.RxCStatsPkts256to511octets <base>.RxCStatsPkts512to1023octets
 * <base>.RxCStatsPkts1024to1518octets <base>.RxCStatsPkts1519toMaxoctets
 * <base>.RxChanFramesReceivedOK <base>.RxChanVlanReceivedOK
 * <base>.TxCIfOutOctetsLo <base>.TxCIfOutErrors <base>.TxCIfOutUniCastPkts
 * <base>.TxChanFramesTransmittedOK <base>.TxChanVlanTransmittedOK
 * <base>.TxCIfOutMultiCastPkts <base>.TxCIfOutBroadCastPkts
 */
#define NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_LO             0x0000
#define NFP_MAC_CHNL_STATS_RX_PSTATS_OCTETS_LO             0x0008
#define NFP_MAC_CHNL_STATS_RX_CIF_IN_ERRORS                0x0010
#define NFP_MAC_CHNL_STATS_RX_CIF_IN_UNICAST_PKTS          0x0014
#define NFP_MAC_CHNL_STATS_RX_CIF_IN_MULTICAST_PKTS        0x0018
#define NFP_MAC_CHNL_STATS_RX_CIF_IN_BROADCAST_PKTS        0x001c
#define NFP_MAC_CHNL_STATS_RX_PSTATS_PKTS                  0x0020
#define NFP_MAC_CHNL_STATS_RX_PSTATS_PKTS_64_OCTETS        0x0024
#define NFP_MAC_CHNL_STATS_RX_PSTATS_PKTS_65_127_OCTETS    0x0028
#define NFP_MAC_CHNL_STATS_RX_PSTATS_PKTS_128_255_OCTETS   0x002c
#define NFP_MAC_CHNL_STATS_RX_PSTATS_PKTS_256_511_OCTETS   0x0030
#define NFP_MAC_CHNL_STATS_RX_PSTATS_PKTS_512_1023_OCTETS  0x0034
#define NFP_MAC_CHNL_STATS_RX_PSTATS_PKTS_1024_1518_OCTETS 0x0038
#define NFP_MAC_CHNL_STATS_RX_PSTATS_PKTS_1519_MAX_OCTETS  0x003c
#define NFP_MAC_CHNL_STATS_RX_CHAN_FRAMES_RECEIVED_OK      0x0040
#define NFP_MAC_CHNL_STATS_RX_CHAN_VLAN_RECEIVED_OK        0x0044
#define NFP_MAC_CHNL_STATS_TX_CIF_OCTETS_LO                0x0060
#define NFP_MAC_CHNL_STATS_TX_CIF_ERRORS                   0x0068
#define NFP_MAC_CHNL_STATS_TX_CIF_UNICAST_PKTS             0x006c
#define NFP_MAC_CHNL_STATS_TX_CHAN_FRAMED_TRANSMITTED_OK   0x0070
#define NFP_MAC_CHNL_STATS_TX_CHAN_VLAN_TRANSMITTED_OK     0x0074
#define NFP_MAC_CHNL_STATS_TX_CIF_MULTICAST_PKTS           0x0078
#define NFP_MAC_CHNL_STATS_TX_CIF_BROADCAST_PKTS           0x007c
#define   NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_LO_STAT_COUNTER_32(x) (((x) & 0xffffffff) << 0)
#define   NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_LO_STAT_COUNTER_32_of(x) (((x) >> 0) & 0xffffffff)
#define     NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_LO_STAT_COUNTER_32_bf 0, 31, 0
#define     NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_LO_STAT_COUNTER_32_msk (0xffffffff)
#define     NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_LO_STAT_COUNTER_32_shf (0)


/*
 * Register: StatCounter8
 *   [7:0]     StatCounter8
 *
 * Name(s):
 * <base>.RxCIfInOctetsHi <base>.RxCStatsOctetsHi <base>.TxCIfOutOctetsHi
 */
#define NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_HI             0x0004
#define NFP_MAC_CHNL_STATS_RX_PSTATS_OCTETS_HI             0x000c
#define NFP_MAC_CHNL_STATS_TX_CIF_OCTETS_HI                0x0064
#define   NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_HI_STAT_COUNTER_8(x) (((x) & 0xff) << 0)
#define   NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_HI_STAT_COUNTER_8_of(x) (((x) >> 0) & 0xff)
#define     NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_HI_STAT_COUNTER_8_bf 0, 7, 0
#define     NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_HI_STAT_COUNTER_8_msk (0xff)
#define     NFP_MAC_CHNL_STATS_RX_CIF_IN_OCTETS_HI_STAT_COUNTER_8_shf (0)



/* MAC register structures */
#if defined(__NFP_LANG_MICROC)


struct nfp_mac_csr_mac_block_rst {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu2:8;
            unsigned int mac_hy1_stat_rst:1;
            unsigned int mac_hy0_stat_rst:1;
            unsigned int mac_tx_rst_mpb:1;
            unsigned int mac_rx_rst_mpb:1;
            unsigned int mac_tx_rst_core:1;
            unsigned int mac_rx_rst_core:1;
            unsigned int mac_fcx2rst_lk1:1;
            unsigned int mac_fcx2rst_lk0:1;
            unsigned int mac_rxrst_lk1:1;
            unsigned int mac_rxrst_lk0:1;
            unsigned int mac_txrst_lk1:1;
            unsigned int mac_txrst_lk0:1;
            unsigned int mac_rst_lk1:1;
            unsigned int mac_rst_lk0:1;
            unsigned int mac_x2clken_lk1:1;
            unsigned int mac_x2clken_lk0:1;
            unsigned int mac_coreclken_lk1:1;
            unsigned int mac_coreclken_lk0:1;
            unsigned int mac_coreclken_hy1:1;
            unsigned int mac_coreclken_hy0:1;
            unsigned int rfu:1;
            unsigned int mac_serdes_rst:1;
            unsigned int mac_s_rst:1;
            unsigned int mac_h_rst:1;
#           else
            unsigned int mac_h_rst:1;
            unsigned int mac_s_rst:1;
            unsigned int mac_serdes_rst:1;
            unsigned int rfu:1;
            unsigned int mac_coreclken_hy0:1;
            unsigned int mac_coreclken_hy1:1;
            unsigned int mac_coreclken_lk0:1;
            unsigned int mac_coreclken_lk1:1;
            unsigned int mac_x2clken_lk0:1;
            unsigned int mac_x2clken_lk1:1;
            unsigned int mac_rst_lk0:1;
            unsigned int mac_rst_lk1:1;
            unsigned int mac_txrst_lk0:1;
            unsigned int mac_txrst_lk1:1;
            unsigned int mac_rxrst_lk0:1;
            unsigned int mac_rxrst_lk1:1;
            unsigned int mac_fcx2rst_lk0:1;
            unsigned int mac_fcx2rst_lk1:1;
            unsigned int mac_rx_rst_core:1;
            unsigned int mac_tx_rst_core:1;
            unsigned int mac_rx_rst_mpb:1;
            unsigned int mac_tx_rst_mpb:1;
            unsigned int mac_hy0_stat_rst:1;
            unsigned int mac_hy1_stat_rst:1;
            unsigned int rfu2:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_hyd0_block_rst {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_hyd_rx_serdes_rst:12;
            unsigned int rfu:4;
            unsigned int mac_hyd_tx_serdes_rst:12;
            unsigned int mac_hyd_rx_ff_rst:1;
            unsigned int mac_hyd_tx_ff_rst:1;
            unsigned int mac_hyd_reg_rst:1;
            unsigned int mac_hyd_ref_rst:1;
#           else
            unsigned int mac_hyd_ref_rst:1;
            unsigned int mac_hyd_reg_rst:1;
            unsigned int mac_hyd_tx_ff_rst:1;
            unsigned int mac_hyd_rx_ff_rst:1;
            unsigned int mac_hyd_tx_serdes_rst:12;
            unsigned int rfu:4;
            unsigned int mac_hyd_rx_serdes_rst:12;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_mux_ctrl {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu:7;
            unsigned int mac_ila_sel:1;
            unsigned int mac_inlk_sel:24;
#           else
            unsigned int mac_inlk_sel:24;
            unsigned int mac_ila_sel:1;
            unsigned int rfu:7;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_serdes_en {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu:8;
            unsigned int serdes_enable:24;
#           else
            unsigned int serdes_enable:24;
            unsigned int rfu:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_sys_support_ctrl {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_sys_support_ctrlc:8;
            unsigned int mac_mpb_free_buf_fifo_low_wm:4;
            unsigned int mac_ig_lnk_lst_freeze_on_err_n:1;
            unsigned int mac_eg_lnk_lst_freeze_on_err_n:1;
            unsigned int dwrr_arbiter_disable:1;
            unsigned int dwrr_weight_wr_enable:1;
            unsigned int mac_ilk_live_int_sel:1;
            unsigned int lk1_ig_dq_segemented_en:1;
            unsigned int lk0_ig_dq_segemented_en:1;
            unsigned int lk1_linklist_en:1;
            unsigned int lk0_linklist_en:1;
            unsigned int hy1_linklist_en:1;
            unsigned int hy0_linklist_en:1;
            unsigned int split_mem_ig:1;
            unsigned int extra_eth_hist_mode:1;
            unsigned int mac_sys_support_ctrla:3;
            unsigned int timestamp_frc:1;
            unsigned int timestamp_set:1;
            unsigned int timestamp_rst:1;
            unsigned int timestamp_en:1;
#           else
            unsigned int timestamp_en:1;
            unsigned int timestamp_rst:1;
            unsigned int timestamp_set:1;
            unsigned int timestamp_frc:1;
            unsigned int mac_sys_support_ctrla:3;
            unsigned int extra_eth_hist_mode:1;
            unsigned int split_mem_ig:1;
            unsigned int hy0_linklist_en:1;
            unsigned int hy1_linklist_en:1;
            unsigned int lk0_linklist_en:1;
            unsigned int lk1_linklist_en:1;
            unsigned int lk0_ig_dq_segemented_en:1;
            unsigned int lk1_ig_dq_segemented_en:1;
            unsigned int mac_ilk_live_int_sel:1;
            unsigned int dwrr_weight_wr_enable:1;
            unsigned int dwrr_arbiter_disable:1;
            unsigned int mac_eg_lnk_lst_freeze_on_err_n:1;
            unsigned int mac_ig_lnk_lst_freeze_on_err_n:1;
            unsigned int mac_mpb_free_buf_fifo_low_wm:4;
            unsigned int mac_sys_support_ctrlc:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_sys_support_stat {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_sys_support_stat:32;
#           else
            unsigned int mac_sys_support_stat:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_ts_nsec {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_ts_nsec:32;
#           else
            unsigned int mac_ts_nsec:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_ts_sec {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_ts_sec:32;
#           else
            unsigned int mac_ts_sec:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_ts_incr {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_20:12;
            unsigned int mac_ts_incr_nsec:4;
            unsigned int mac_ts_incr_frac:16;
#           else
            unsigned int mac_ts_incr_frac:16;
            unsigned int mac_ts_incr_nsec:4;
            unsigned int __reserved_20:12;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_ts_set_nsec {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_ts_set_nsec:32;
#           else
            unsigned int mac_ts_set_nsec:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_ts_set_sec {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_ts_sec:32;
#           else
            unsigned int mac_ts_sec:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_tdm0_cycle_word_3100 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_tdm_port_slot7:4;
            unsigned int mac_tdm_port_slot6:4;
            unsigned int mac_tdm_port_slot5:4;
            unsigned int mac_tdm_port_slot4:4;
            unsigned int mac_tdm_port_slot3:4;
            unsigned int mac_tdm_port_slot2:4;
            unsigned int mac_tdm_port_slot1:4;
            unsigned int mac_tdm_port_slot0:4;
#           else
            unsigned int mac_tdm_port_slot0:4;
            unsigned int mac_tdm_port_slot1:4;
            unsigned int mac_tdm_port_slot2:4;
            unsigned int mac_tdm_port_slot3:4;
            unsigned int mac_tdm_port_slot4:4;
            unsigned int mac_tdm_port_slot5:4;
            unsigned int mac_tdm_port_slot6:4;
            unsigned int mac_tdm_port_slot7:4;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_tdm0_cycle_word_4732 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int mac_tdm_port_slot11:4;
            unsigned int mac_tdm_port_slot10:4;
            unsigned int mac_tdm_port_slot9:4;
            unsigned int mac_tdm_port_slot8:4;
#           else
            unsigned int mac_tdm_port_slot8:4;
            unsigned int mac_tdm_port_slot9:4;
            unsigned int mac_tdm_port_slot10:4;
            unsigned int mac_tdm_port_slot11:4;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_tdm0_mode_0900 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_30:2;
            unsigned int mac_tdm_mode_port_9:3;
            unsigned int mac_tdm_mode_port_8:3;
            unsigned int mac_tdm_mode_port_7:3;
            unsigned int mac_tdm_mode_port_6:3;
            unsigned int mac_tdm_mode_port_5:3;
            unsigned int mac_tdm_mode_port_4:3;
            unsigned int mac_tdm_mode_port_3:3;
            unsigned int mac_tdm_mode_port_2:3;
            unsigned int mac_tdm_mode_port_1:3;
            unsigned int mac_tdm_mode_port_0:3;
#           else
            unsigned int mac_tdm_mode_port_0:3;
            unsigned int mac_tdm_mode_port_1:3;
            unsigned int mac_tdm_mode_port_2:3;
            unsigned int mac_tdm_mode_port_3:3;
            unsigned int mac_tdm_mode_port_4:3;
            unsigned int mac_tdm_mode_port_5:3;
            unsigned int mac_tdm_mode_port_6:3;
            unsigned int mac_tdm_mode_port_7:3;
            unsigned int mac_tdm_mode_port_8:3;
            unsigned int mac_tdm_mode_port_9:3;
            unsigned int __reserved_30:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_tdm0_mode_1110_crc_en {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_28:4;
            unsigned int mac_egress_port_crc_en:12;
            unsigned int __reserved_6:10;
            unsigned int mac_tdm_mode_port_1:3;
            unsigned int mac_tdm_mode_port_0:3;
#           else
            unsigned int mac_tdm_mode_port_0:3;
            unsigned int mac_tdm_mode_port_1:3;
            unsigned int __reserved_6:10;
            unsigned int mac_egress_port_crc_en:12;
            unsigned int __reserved_28:4;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_port_2_to_0_chan_assign {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_30:2;
            unsigned int port_num_of_chans2:4;
            unsigned int port_base_chan2:6;
            unsigned int port_num_of_chans1:4;
            unsigned int port_base_chan1:6;
            unsigned int port_num_of_chans0:4;
            unsigned int port_base_chan0:6;
#           else
            unsigned int port_base_chan0:6;
            unsigned int port_num_of_chans0:4;
            unsigned int port_base_chan1:6;
            unsigned int port_num_of_chans1:4;
            unsigned int port_base_chan2:6;
            unsigned int port_num_of_chans2:4;
            unsigned int __reserved_30:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_prepend_ctl_03_to_00 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eg_skip_octets_port_3:4;
            unsigned int ig_skip_octets_port_3:4;
            unsigned int eg_skip_octets_port_2:4;
            unsigned int ig_skip_octets_port_2:4;
            unsigned int eg_skip_octets_port_1:4;
            unsigned int ig_skip_octets_port_1:4;
            unsigned int eg_skip_octets_port_0:4;
            unsigned int ig_skip_octets_port_0:4;
#           else
            unsigned int ig_skip_octets_port_0:4;
            unsigned int eg_skip_octets_port_0:4;
            unsigned int ig_skip_octets_port_1:4;
            unsigned int eg_skip_octets_port_1:4;
            unsigned int ig_skip_octets_port_2:4;
            unsigned int eg_skip_octets_port_2:4;
            unsigned int ig_skip_octets_port_3:4;
            unsigned int eg_skip_octets_port_3:4;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_prepend_ctl_07_to_04 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eg_skip_octets_port_7:4;
            unsigned int ig_skip_octets_port_7:4;
            unsigned int eg_skip_octets_port_6:4;
            unsigned int ig_skip_octets_port_6:4;
            unsigned int eg_skip_octets_port_5:4;
            unsigned int ig_skip_octets_port_5:4;
            unsigned int eg_skip_octets_port_4:4;
            unsigned int ig_skip_octets_port_4:4;
#           else
            unsigned int ig_skip_octets_port_4:4;
            unsigned int eg_skip_octets_port_4:4;
            unsigned int ig_skip_octets_port_5:4;
            unsigned int eg_skip_octets_port_5:4;
            unsigned int ig_skip_octets_port_6:4;
            unsigned int eg_skip_octets_port_6:4;
            unsigned int ig_skip_octets_port_7:4;
            unsigned int eg_skip_octets_port_7:4;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_prepend_ctl_11_to_08 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eg_skip_octets_port_11:4;
            unsigned int ig_skip_octets_port_11:4;
            unsigned int eg_skip_octets_port_10:4;
            unsigned int ig_skip_octets_port_10:4;
            unsigned int eg_skip_octets_port_9:4;
            unsigned int ig_skip_octets_port_9:4;
            unsigned int eg_skip_octets_port_8:4;
            unsigned int ig_skip_octets_port_8:4;
#           else
            unsigned int ig_skip_octets_port_8:4;
            unsigned int eg_skip_octets_port_8:4;
            unsigned int ig_skip_octets_port_9:4;
            unsigned int eg_skip_octets_port_9:4;
            unsigned int ig_skip_octets_port_10:4;
            unsigned int eg_skip_octets_port_10:4;
            unsigned int ig_skip_octets_port_11:4;
            unsigned int eg_skip_octets_port_11:4;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_dsa_ctl_15_to_00 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int dsa_tag_mode_port_15:2;
            unsigned int dsa_tag_mode_port_14:2;
            unsigned int dsa_tag_mode_port_13:2;
            unsigned int dsa_tag_mode_port_12:2;
            unsigned int dsa_tag_mode_port_11:2;
            unsigned int dsa_tag_mode_port_10:2;
            unsigned int dsa_tag_mode_port_9:2;
            unsigned int dsa_tag_mode_port_8:2;
            unsigned int dsa_tag_mode_port_7:2;
            unsigned int dsa_tag_mode_port_6:2;
            unsigned int dsa_tag_mode_port_5:2;
            unsigned int dsa_tag_mode_port_4:2;
            unsigned int dsa_tag_mode_port_3:2;
            unsigned int dsa_tag_mode_port_2:2;
            unsigned int dsa_tag_mode_port_1:2;
            unsigned int dsa_tag_mode_port_0:2;
#           else
            unsigned int dsa_tag_mode_port_0:2;
            unsigned int dsa_tag_mode_port_1:2;
            unsigned int dsa_tag_mode_port_2:2;
            unsigned int dsa_tag_mode_port_3:2;
            unsigned int dsa_tag_mode_port_4:2;
            unsigned int dsa_tag_mode_port_5:2;
            unsigned int dsa_tag_mode_port_6:2;
            unsigned int dsa_tag_mode_port_7:2;
            unsigned int dsa_tag_mode_port_8:2;
            unsigned int dsa_tag_mode_port_9:2;
            unsigned int dsa_tag_mode_port_10:2;
            unsigned int dsa_tag_mode_port_11:2;
            unsigned int dsa_tag_mode_port_12:2;
            unsigned int dsa_tag_mode_port_13:2;
            unsigned int dsa_tag_mode_port_14:2;
            unsigned int dsa_tag_mode_port_15:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_dsa_ctl_23_to_16 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu:12;
            unsigned int dsa_tag_mode_lk_core_1:2;
            unsigned int dsa_tag_mode_lk_core_0:2;
            unsigned int dsa_tag_mode_port_23:2;
            unsigned int dsa_tag_mode_port_22:2;
            unsigned int dsa_tag_mode_port_21:2;
            unsigned int dsa_tag_mode_port_20:2;
            unsigned int dsa_tag_mode_port_19:2;
            unsigned int dsa_tag_mode_port_18:2;
            unsigned int dsa_tag_mode_port_17:2;
            unsigned int dsa_tag_mode_port_16:2;
#           else
            unsigned int dsa_tag_mode_port_16:2;
            unsigned int dsa_tag_mode_port_17:2;
            unsigned int dsa_tag_mode_port_18:2;
            unsigned int dsa_tag_mode_port_19:2;
            unsigned int dsa_tag_mode_port_20:2;
            unsigned int dsa_tag_mode_port_21:2;
            unsigned int dsa_tag_mode_port_22:2;
            unsigned int dsa_tag_mode_port_23:2;
            unsigned int dsa_tag_mode_lk_core_0:2;
            unsigned int dsa_tag_mode_lk_core_1:2;
            unsigned int rfu:12;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_interlaken_ctl_1 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int lk_burstmax_core_1:3;
            unsigned int lk_num_channels_upper_64:7;
            unsigned int lk_base_channel_upper_64:6;
            unsigned int lk_burstmax_core_0:3;
            unsigned int lk_num_channels_lower_64:7;
            unsigned int lk_base_channel_lower_64:6;
#           else
            unsigned int lk_base_channel_lower_64:6;
            unsigned int lk_num_channels_lower_64:7;
            unsigned int lk_burstmax_core_0:3;
            unsigned int lk_base_channel_upper_64:6;
            unsigned int lk_num_channels_upper_64:7;
            unsigned int lk_burstmax_core_1:3;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_interlaken_ctl_2 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu2:9;
            unsigned int lk_nbi_chan_swap_en_1:1;
            unsigned int ig_oob_fc_sel_core_1:1;
            unsigned int eg_oob_fc_sel_core_1:1;
            unsigned int eg_tdm_mode_lk_core_1:3;
            unsigned int eg_atomic_lk_core_1:1;
            unsigned int rfu:9;
            unsigned int lk_nbi_chan_swap_en_0:1;
            unsigned int ig_oob_fc_sel_core_0:1;
            unsigned int eg_oob_fc_sel_core_0:1;
            unsigned int eg_tdm_mode_lk_core_0:3;
            unsigned int eg_atomic_lk_core_0:1;
#           else
            unsigned int eg_atomic_lk_core_0:1;
            unsigned int eg_tdm_mode_lk_core_0:3;
            unsigned int eg_oob_fc_sel_core_0:1;
            unsigned int ig_oob_fc_sel_core_0:1;
            unsigned int lk_nbi_chan_swap_en_0:1;
            unsigned int rfu:9;
            unsigned int eg_atomic_lk_core_1:1;
            unsigned int eg_tdm_mode_lk_core_1:3;
            unsigned int eg_oob_fc_sel_core_1:1;
            unsigned int ig_oob_fc_sel_core_1:1;
            unsigned int lk_nbi_chan_swap_en_1:1;
            unsigned int rfu2:9;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_eg_buffer_credit_pool_count {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eg_buffer_linklist_ready:1;
            unsigned int __reserved_30:1;
            unsigned int eg_buffer_credit_count1:14;
            unsigned int __reserved_14:2;
            unsigned int eg_buffer_credit_count:14;
#           else
            unsigned int eg_buffer_credit_count:14;
            unsigned int __reserved_14:2;
            unsigned int eg_buffer_credit_count1:14;
            unsigned int __reserved_30:1;
            unsigned int eg_buffer_linklist_ready:1;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_tx_mpb_credit_init {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu:12;
            unsigned int tx_mpb_credit_data_init:4;
            unsigned int rfu2:4;
            unsigned int tx_mpb_credit_max_pkt_init:6;
            unsigned int tx_mpb_credit_pkt_init:6;
#           else
            unsigned int tx_mpb_credit_pkt_init:6;
            unsigned int tx_mpb_credit_max_pkt_init:6;
            unsigned int rfu2:4;
            unsigned int tx_mpb_credit_data_init:4;
            unsigned int rfu:12;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_ig_buffer_credit_pool_count {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int ig_buffer_linklist_ready:1;
            unsigned int __reserved_30:1;
            unsigned int ig_buffer_credit_count1:14;
            unsigned int __reserved_14:2;
            unsigned int ig_buffer_credit_count:14;
#           else
            unsigned int ig_buffer_credit_count:14;
            unsigned int __reserved_14:2;
            unsigned int ig_buffer_credit_count1:14;
            unsigned int __reserved_30:1;
            unsigned int ig_buffer_linklist_ready:1;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_rx_mpb_credit_init {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu2:2;
            unsigned int rx_mpb_credit_data_init:14;
            unsigned int rfu:2;
            unsigned int rx_mpb_credit_buf_init:14;
#           else
            unsigned int rx_mpb_credit_buf_init:14;
            unsigned int rfu:2;
            unsigned int rx_mpb_credit_data_init:14;
            unsigned int rfu2:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_tdm_rate_credit_init {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int tdm_100ge_credit_init:8;
            unsigned int tdm_40ge_credit_init:8;
            unsigned int tdm_10ge_credit_init:8;
            unsigned int tdm_1ge_credit_init:8;
#           else
            unsigned int tdm_1ge_credit_init:8;
            unsigned int tdm_10ge_credit_init:8;
            unsigned int tdm_40ge_credit_init:8;
            unsigned int tdm_100ge_credit_init:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_intr_err_status_0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_interrupt_err_status_1:32;
#           else
            unsigned int mac_interrupt_err_status_1:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_intr_err_status_1 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu:8;
            unsigned int mac_linkt_training_int_hy0:12;
            unsigned int mac_linkt_training_int_hy1:12;
#           else
            unsigned int mac_linkt_training_int_hy1:12;
            unsigned int mac_linkt_training_int_hy0:12;
            unsigned int rfu:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_intr_err_en_0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_interrupt_err_en_0:32;
#           else
            unsigned int mac_interrupt_err_en_0:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_intr_err_en_1 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_interrupt_err_en_1:32;
#           else
            unsigned int mac_interrupt_err_en_1:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_live_status_0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_live_status_0:32;
#           else
            unsigned int mac_live_status_0:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_live_status_1 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_live_status_1:32;
#           else
            unsigned int mac_live_status_1:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_chan_rd_addr {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu1:9;
            unsigned int ig_chan_rd_addr:7;
            unsigned int rfu0:9;
            unsigned int eg_chan_rd_addr:7;
#           else
            unsigned int eg_chan_rd_addr:7;
            unsigned int rfu0:9;
            unsigned int ig_chan_rd_addr:7;
            unsigned int rfu1:9;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_chan_buf_count {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu1:5;
            unsigned int ig_chan_rd_buf_cnt:11;
            unsigned int rfu0:5;
            unsigned int eg_chan_rd_buf_cnt:11;
#           else
            unsigned int eg_chan_rd_buf_cnt:11;
            unsigned int rfu0:5;
            unsigned int ig_chan_rd_buf_cnt:11;
            unsigned int rfu1:5;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_pause_watermark {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int pwm_resv1:4;
            unsigned int pause_watermark1:12;
            unsigned int pwm_resv0:4;
            unsigned int pause_watermark0:12;
#           else
            unsigned int pause_watermark0:12;
            unsigned int pwm_resv0:4;
            unsigned int pause_watermark1:12;
            unsigned int pwm_resv1:4;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_ig_chan_used_buffer_credits_rw {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int counter_addr:8;
            unsigned int rfu2:2;
            unsigned int counter_rd_busy:1;
            unsigned int rfu:21;
#           else
            unsigned int rfu:21;
            unsigned int counter_rd_busy:1;
            unsigned int rfu2:2;
            unsigned int counter_addr:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_ig_chan_used_buffer_credits_rd_data {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int counter_addr:8;
            unsigned int __reserved_22:2;
            unsigned int buffer_counter_rd_data_valid:1;
            unsigned int __reserved_16:5;
            unsigned int counter_rd_data:16;
#           else
            unsigned int counter_rd_data:16;
            unsigned int __reserved_16:5;
            unsigned int buffer_counter_rd_data_valid:1;
            unsigned int __reserved_22:2;
            unsigned int counter_addr:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_ig_port_prepend_en0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu:6;
            unsigned int prepend_lk:2;
            unsigned int prepend_en11:2;
            unsigned int prepend_en10:2;
            unsigned int prepend_en9:2;
            unsigned int prepend_en8:2;
            unsigned int prepend_en7:2;
            unsigned int prepend_en6:2;
            unsigned int prepend_en5:2;
            unsigned int prepend_en4:2;
            unsigned int prepend_en3:2;
            unsigned int prepend_en2:2;
            unsigned int prepend_en1:2;
            unsigned int prepend_en0:2;
#           else
            unsigned int prepend_en0:2;
            unsigned int prepend_en1:2;
            unsigned int prepend_en2:2;
            unsigned int prepend_en3:2;
            unsigned int prepend_en4:2;
            unsigned int prepend_en5:2;
            unsigned int prepend_en6:2;
            unsigned int prepend_en7:2;
            unsigned int prepend_en8:2;
            unsigned int prepend_en9:2;
            unsigned int prepend_en10:2;
            unsigned int prepend_en11:2;
            unsigned int prepend_lk:2;
            unsigned int rfu:6;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_eg_vlan_match_reg0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int vlan_mask:16;
            unsigned int vlan_match:16;
#           else
            unsigned int vlan_match:16;
            unsigned int vlan_mask:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_eg_vlan_match_reg1 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int vlan_mask1:16;
            unsigned int vlan_match1:16;
#           else
            unsigned int vlan_match1:16;
            unsigned int vlan_mask1:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_eg_cmd_prepend_en0_lo {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eg_cmd_prepend_en:32;
#           else
            unsigned int eg_cmd_prepend_en:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_eg_ilk_chan_assign {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu1:3;
            unsigned int lk_num_channels_upper_64:7;
            unsigned int lk_base_channel_upper_64:6;
            unsigned int rfu0:3;
            unsigned int lk_num_channels_lower_64:7;
            unsigned int lk_base_channel_lower_64:6;
#           else
            unsigned int lk_base_channel_lower_64:6;
            unsigned int lk_num_channels_lower_64:7;
            unsigned int rfu0:3;
            unsigned int lk_base_channel_upper_64:6;
            unsigned int lk_num_channels_upper_64:7;
            unsigned int rfu1:3;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_eg_port_rr {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_24:8;
            unsigned int eg_port_rr:24;
#           else
            unsigned int eg_port_rr:24;
            unsigned int __reserved_24:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_oob_fc_tm_cntl {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int oob_1023_to_512_rfu:10;
            unsigned int oob_1023_to_512_mod32_m1:4;
            unsigned int oob_1023_to_512_msg_en:1;
            unsigned int oob_1023_to_512_en:1;
            unsigned int oob_511_to_0_rfu:10;
            unsigned int oob_511_to_0_mod32_m1:4;
            unsigned int oob_511_to_0_msg_en:1;
            unsigned int oob_511_to_0_en:1;
#           else
            unsigned int oob_511_to_0_en:1;
            unsigned int oob_511_to_0_msg_en:1;
            unsigned int oob_511_to_0_mod32_m1:4;
            unsigned int oob_511_to_0_rfu:10;
            unsigned int oob_1023_to_512_en:1;
            unsigned int oob_1023_to_512_msg_en:1;
            unsigned int oob_1023_to_512_mod32_m1:4;
            unsigned int oob_1023_to_512_rfu:10;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_oob_fc_tm_remap {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int tm_fc_addr7_sel:4;
            unsigned int tm_fc_addr6_sel:4;
            unsigned int tm_fc_addr5_sel:4;
            unsigned int tm_fc_addr4_sel:4;
            unsigned int tm_fc_addr3_sel:4;
            unsigned int tm_fc_addr2_sel:4;
            unsigned int tm_fc_addr1_sel:4;
            unsigned int tm_fc_addr0_sel:4;
#           else
            unsigned int tm_fc_addr0_sel:4;
            unsigned int tm_fc_addr1_sel:4;
            unsigned int tm_fc_addr2_sel:4;
            unsigned int tm_fc_addr3_sel:4;
            unsigned int tm_fc_addr4_sel:4;
            unsigned int tm_fc_addr5_sel:4;
            unsigned int tm_fc_addr6_sel:4;
            unsigned int tm_fc_addr7_sel:4;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_hy0_eth_ig_pkt_head_drop_cntr_pair_ {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_head_drop_counter1:16;
            unsigned int mac_head_drop_counter0:16;
#           else
            unsigned int mac_head_drop_counter0:16;
            unsigned int mac_head_drop_counter1:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_eth_fifo_if_err_0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_25:7;
            unsigned int rem_loc_fault_sticky:1;
            unsigned int eth_tx_if_ovr:12;
            unsigned int eth_tx_if_unf:12;
#           else
            unsigned int eth_tx_if_unf:12;
            unsigned int eth_tx_if_ovr:12;
            unsigned int rem_loc_fault_sticky:1;
            unsigned int __reserved_25:7;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_eth_an_status_0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_24:8;
            unsigned int eth_an_int:12;
            unsigned int eth_an_done:12;
#           else
            unsigned int eth_an_done:12;
            unsigned int eth_an_int:12;
            unsigned int __reserved_24:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_oob_fc_ilk_status {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_20:12;
            unsigned int mac_oob_fc_crc_err_1:1;
            unsigned int mac_oob_fc_frm_err_1:1;
            unsigned int mac_oob_fc_crc_err_0:1;
            unsigned int mac_oob_fc_frm_err_0:1;
            unsigned int __reserved_8:8;
            unsigned int ilk_int_2nd_rx_1:1;
            unsigned int ilk_int_2nd_tx_1:1;
            unsigned int ilk_int_rx_1:1;
            unsigned int ilk_int_tx_1:1;
            unsigned int ilk_int_2nd_rx_0:1;
            unsigned int ilk_int_2nd_tx_0:1;
            unsigned int ilk_int_rx_0:1;
            unsigned int ilk_int_tx_0:1;
#           else
            unsigned int ilk_int_tx_0:1;
            unsigned int ilk_int_rx_0:1;
            unsigned int ilk_int_2nd_tx_0:1;
            unsigned int ilk_int_2nd_rx_0:1;
            unsigned int ilk_int_tx_1:1;
            unsigned int ilk_int_rx_1:1;
            unsigned int ilk_int_2nd_tx_1:1;
            unsigned int ilk_int_2nd_rx_1:1;
            unsigned int __reserved_8:8;
            unsigned int mac_oob_fc_frm_err_0:1;
            unsigned int mac_oob_fc_crc_err_0:1;
            unsigned int mac_oob_fc_frm_err_1:1;
            unsigned int mac_oob_fc_crc_err_1:1;
            unsigned int __reserved_20:12;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_stats_half_full_port_00_11 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_25:7;
            unsigned int tx_stat_half_full_vld:1;
            unsigned int tx_stat_addr:8;
            unsigned int __reserved_9:7;
            unsigned int rx_stat_half_full_vld:1;
            unsigned int rx_stat_addr:8;
#           else
            unsigned int rx_stat_addr:8;
            unsigned int rx_stat_half_full_vld:1;
            unsigned int __reserved_9:7;
            unsigned int tx_stat_addr:8;
            unsigned int tx_stat_half_full_vld:1;
            unsigned int __reserved_25:7;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_pcp_remap {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int pcp_re_map_rfu:2;
            unsigned int mac_untagd_abs:6;
            unsigned int pcp_remap7:3;
            unsigned int pcp_remap6:3;
            unsigned int pcp_remap5:3;
            unsigned int pcp_remap4:3;
            unsigned int pcp_remap3:3;
            unsigned int pcp_remap2:3;
            unsigned int pcp_remap1:3;
            unsigned int pcp_remap0:3;
#           else
            unsigned int pcp_remap0:3;
            unsigned int pcp_remap1:3;
            unsigned int pcp_remap2:3;
            unsigned int pcp_remap3:3;
            unsigned int pcp_remap4:3;
            unsigned int pcp_remap5:3;
            unsigned int pcp_remap6:3;
            unsigned int pcp_remap7:3;
            unsigned int mac_untagd_abs:6;
            unsigned int pcp_re_map_rfu:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_port_hwm {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int por_drop_delta1:5;
            unsigned int port_hwm1:11;
            unsigned int por_drop_delta0:5;
            unsigned int port_hwm0:11;
#           else
            unsigned int port_hwm0:11;
            unsigned int por_drop_delta0:5;
            unsigned int port_hwm1:11;
            unsigned int por_drop_delta1:5;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mac_port_hwm_lk1_lk0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int por_drop_delta1:5;
            unsigned int port_hwm1:11;
            unsigned int por_drop_delta0:5;
            unsigned int port_hwm0:11;
#           else
            unsigned int port_hwm0:11;
            unsigned int por_drop_delta0:5;
            unsigned int port_hwm1:11;
            unsigned int por_drop_delta1:5;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_eg_lnklst_rddata {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_30:2;
            unsigned int ll_rd_offset_addr:12;
            unsigned int ll_rd_data_valid:1;
            unsigned int __reserved_16:1;
            unsigned int ll_rd_data:16;
#           else
            unsigned int ll_rd_data:16;
            unsigned int __reserved_16:1;
            unsigned int ll_rd_data_valid:1;
            unsigned int ll_rd_offset_addr:12;
            unsigned int __reserved_30:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_eg_lnklst_rdwr {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int rfu:2;
            unsigned int ll_offset_addr:12;
            unsigned int ll_rd_busy:1;
            unsigned int ll_wr_busy:1;
            unsigned int ll_wr_data:16;
#           else
            unsigned int ll_wr_data:16;
            unsigned int ll_wr_busy:1;
            unsigned int ll_rd_busy:1;
            unsigned int ll_offset_addr:12;
            unsigned int rfu:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_serdes4_rdwr_03_00 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_serdes_page_addr:3;
            unsigned int mac_serdes_pcs_pma_sel:1;
            unsigned int mac_serdes_offset_addr:12;
            unsigned int rfu:6;
            unsigned int mac_serdes_rd_busy:1;
            unsigned int mac_serdes_wr_busy:1;
            unsigned int mac_serdes_wr_data:8;
#           else
            unsigned int mac_serdes_wr_data:8;
            unsigned int mac_serdes_wr_busy:1;
            unsigned int mac_serdes_rd_busy:1;
            unsigned int rfu:6;
            unsigned int mac_serdes_offset_addr:12;
            unsigned int mac_serdes_pcs_pma_sel:1;
            unsigned int mac_serdes_page_addr:3;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_tdm_mem_rdwr {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int tdm_mem_rd_wr_rfu1:2;
            unsigned int tdm_mem_rd_wr_addr:6;
            unsigned int tdm_mem_rd_wr_rfu0:2;
            unsigned int tdm_mem_rd_busy:1;
            unsigned int tdm_mem_wr_busy:1;
            unsigned int tdm_mem_unused:4;
            unsigned int tdm_port_arb_enable:1;
            unsigned int tdm_port_weightwr_data:15;
#           else
            unsigned int tdm_port_weightwr_data:15;
            unsigned int tdm_port_arb_enable:1;
            unsigned int tdm_mem_unused:4;
            unsigned int tdm_mem_wr_busy:1;
            unsigned int tdm_mem_rd_busy:1;
            unsigned int tdm_mem_rd_wr_rfu0:2;
            unsigned int tdm_mem_rd_wr_addr:6;
            unsigned int tdm_mem_rd_wr_rfu1:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_serdes4_rddata_03_00 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int mac_serdes_rd_page_addr:3;
            unsigned int mac_serdes_rd_offset_addr:13;
            unsigned int __reserved_10:6;
            unsigned int mac_serdes_rd_data_valid:1;
            unsigned int __reserved_8:1;
            unsigned int mac_serdes_rd_data:8;
#           else
            unsigned int mac_serdes_rd_data:8;
            unsigned int __reserved_8:1;
            unsigned int mac_serdes_rd_data_valid:1;
            unsigned int __reserved_10:6;
            unsigned int mac_serdes_rd_offset_addr:13;
            unsigned int mac_serdes_rd_page_addr:3;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_tdm_mem_rddata {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_30:2;
            unsigned int tdm_mem_rd_addr:6;
            unsigned int __reserved_22:2;
            unsigned int tdm_mem_rd_data_valid:1;
            unsigned int __reserved_20:1;
            unsigned int tdm_port_unused_rd_ret:4;
            unsigned int tdm_port_arb_enable:1;
            unsigned int tdm_port_weightrd_data:15;
#           else
            unsigned int tdm_port_weightrd_data:15;
            unsigned int tdm_port_arb_enable:1;
            unsigned int tdm_port_unused_rd_ret:4;
            unsigned int __reserved_20:1;
            unsigned int tdm_mem_rd_data_valid:1;
            unsigned int __reserved_22:2;
            unsigned int tdm_mem_rd_addr:6;
            unsigned int __reserved_30:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_serdes_pd_rx {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_24:8;
            unsigned int serdes_lane_power_down:24;
#           else
            unsigned int serdes_lane_power_down:24;
            unsigned int __reserved_24:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_serdes_pd_sy {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_6:26;
            unsigned int serdes_synth_power_down:6;
#           else
            unsigned int serdes_synth_power_down:6;
            unsigned int __reserved_6:26;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_serdes_ck_mux_sel {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int serdes_ck_mux_sel_rfu:2;
            unsigned int serdes_ck_mux_sel_gang_2320:1;
            unsigned int serdes_ck_mux_sel_23:1;
            unsigned int serdes_ck_mux_sel_22:1;
            unsigned int serdes_ck_mux_sel_21:1;
            unsigned int serdes_ck_mux_sel_20:1;
            unsigned int serdes_ck_mux_sel_gang_1916:1;
            unsigned int serdes_ck_mux_sel_19:1;
            unsigned int serdes_ck_mux_sel_18:1;
            unsigned int serdes_ck_mux_sel_17:1;
            unsigned int serdes_ck_mux_sel_16:1;
            unsigned int serdes_ck_mux_sel_gang_1512:1;
            unsigned int serdes_ck_mux_sel_15:1;
            unsigned int serdes_ck_mux_sel_14:1;
            unsigned int serdes_ck_mux_sel_13:1;
            unsigned int serdes_ck_mux_sel_12:1;
            unsigned int serdes_ck_mux_sel_gang_1108:1;
            unsigned int serdes_ck_mux_sel_11:1;
            unsigned int serdes_ck_mux_sel_10:1;
            unsigned int serdes_ck_mux_sel_09:1;
            unsigned int serdes_ck_mux_sel_08:1;
            unsigned int serdes_ck_mux_sel_gang_0704:1;
            unsigned int serdes_ck_mux_sel_07:1;
            unsigned int serdes_ck_mux_sel_06:1;
            unsigned int serdes_ck_mux_sel_05:1;
            unsigned int serdes_ck_mux_sel_04:1;
            unsigned int serdes_ck_mux_sel_gang_0300:1;
            unsigned int serdes_ck_mux_sel_03:1;
            unsigned int serdes_ck_mux_sel_02:1;
            unsigned int serdes_ck_mux_sel_01:1;
            unsigned int serdes_ck_mux_sel_00:1;
#           else
            unsigned int serdes_ck_mux_sel_00:1;
            unsigned int serdes_ck_mux_sel_01:1;
            unsigned int serdes_ck_mux_sel_02:1;
            unsigned int serdes_ck_mux_sel_03:1;
            unsigned int serdes_ck_mux_sel_gang_0300:1;
            unsigned int serdes_ck_mux_sel_04:1;
            unsigned int serdes_ck_mux_sel_05:1;
            unsigned int serdes_ck_mux_sel_06:1;
            unsigned int serdes_ck_mux_sel_07:1;
            unsigned int serdes_ck_mux_sel_gang_0704:1;
            unsigned int serdes_ck_mux_sel_08:1;
            unsigned int serdes_ck_mux_sel_09:1;
            unsigned int serdes_ck_mux_sel_10:1;
            unsigned int serdes_ck_mux_sel_11:1;
            unsigned int serdes_ck_mux_sel_gang_1108:1;
            unsigned int serdes_ck_mux_sel_12:1;
            unsigned int serdes_ck_mux_sel_13:1;
            unsigned int serdes_ck_mux_sel_14:1;
            unsigned int serdes_ck_mux_sel_15:1;
            unsigned int serdes_ck_mux_sel_gang_1512:1;
            unsigned int serdes_ck_mux_sel_16:1;
            unsigned int serdes_ck_mux_sel_17:1;
            unsigned int serdes_ck_mux_sel_18:1;
            unsigned int serdes_ck_mux_sel_19:1;
            unsigned int serdes_ck_mux_sel_gang_1916:1;
            unsigned int serdes_ck_mux_sel_20:1;
            unsigned int serdes_ck_mux_sel_21:1;
            unsigned int serdes_ck_mux_sel_22:1;
            unsigned int serdes_ck_mux_sel_23:1;
            unsigned int serdes_ck_mux_sel_gang_2320:1;
            unsigned int serdes_ck_mux_sel_rfu:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_serdes_sig_detect {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_24:8;
            unsigned int serdes_lane_signal_detect:24;
#           else
            unsigned int serdes_lane_signal_detect:24;
            unsigned int __reserved_24:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_serdes_sig_detect_ovr {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_24:8;
            unsigned int serdes_lane_signal_detect_ovr:24;
#           else
            unsigned int serdes_lane_signal_detect_ovr:24;
            unsigned int __reserved_24:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_serdes_eth_rx_act_detect {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_24:8;
            unsigned int serdes_port_activity_detect:24;
#           else
            unsigned int serdes_port_activity_detect:24;
            unsigned int __reserved_24:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_serdes_link_up {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_24:8;
            unsigned int serdes_link_up:24;
#           else
            unsigned int serdes_link_up:24;
            unsigned int __reserved_24:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_parity_err_inject {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_6:26;
            unsigned int inject_ig_par_err_desc_mem:1;
            unsigned int inject_ig_par_err_tsmp_mem:1;
            unsigned int inject_ig_par_err_rslt_mem:1;
            unsigned int inject_eg_par_err_desc_mem:1;
            unsigned int inject_eg_par_err_rslt1_mem:1;
            unsigned int inject_eg_par_err_rslt0_mem:1;
#           else
            unsigned int inject_eg_par_err_rslt0_mem:1;
            unsigned int inject_eg_par_err_rslt1_mem:1;
            unsigned int inject_eg_par_err_desc_mem:1;
            unsigned int inject_ig_par_err_rslt_mem:1;
            unsigned int inject_ig_par_err_tsmp_mem:1;
            unsigned int inject_ig_par_err_desc_mem:1;
            unsigned int __reserved_6:26;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_ig_parity_err_status {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_15:17;
            unsigned int ig_par_err_type:3;
            unsigned int __reserved_11:1;
            unsigned int ig_par_err_addr:11;
#           else
            unsigned int ig_par_err_addr:11;
            unsigned int __reserved_11:1;
            unsigned int ig_par_err_type:3;
            unsigned int __reserved_15:17;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_eg_parity_err_status {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_31:1;
            unsigned int eg_par_err_type1:3;
            unsigned int __reserved_26:2;
            unsigned int eg_par_err_addr1:10;
            unsigned int __reserved_15:1;
            unsigned int eg_par_err_type0:3;
            unsigned int __reserved_10:2;
            unsigned int eg_par_err_addr0:10;
#           else
            unsigned int eg_par_err_addr0:10;
            unsigned int __reserved_10:2;
            unsigned int eg_par_err_type0:3;
            unsigned int __reserved_15:1;
            unsigned int eg_par_err_addr1:10;
            unsigned int __reserved_26:2;
            unsigned int eg_par_err_type1:3;
            unsigned int __reserved_31:1;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_mem_err_drop_counts {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int ig_mem_err_drop_1:8;
            unsigned int ig_mem_err_drop_0:8;
            unsigned int eg_mem_err_drop_1:8;
            unsigned int eg_mem_err_drop_0:8;
#           else
            unsigned int eg_mem_err_drop_0:8;
            unsigned int eg_mem_err_drop_1:8;
            unsigned int ig_mem_err_drop_0:8;
            unsigned int ig_mem_err_drop_1:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_assert_config_csr0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int assert_config_csr0:32;
#           else
            unsigned int assert_config_csr0:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_csr_assert_config_csr1 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int assert_config_csr1_disable:14;
            unsigned int assert_config_csr1_ena_fsm1:1;
            unsigned int assert_config_csr1_ena_fsm0:1;
            unsigned int assert_config_csr1_fsm_cfg1:8;
            unsigned int assert_config_csr1_fsm_cfg0:8;
#           else
            unsigned int assert_config_csr1_fsm_cfg0:8;
            unsigned int assert_config_csr1_fsm_cfg1:8;
            unsigned int assert_config_csr1_ena_fsm0:1;
            unsigned int assert_config_csr1_ena_fsm1:1;
            unsigned int assert_config_csr1_disable:14;
#           endif
        };
        unsigned int __raw;
    };
};


//pyexec<dump_map_structs(InterruptManagerMap(altname="MAC_INTR_MNG"))>

struct nfp_mac_eth_mac_seg_eth_revison {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_cust_ver:16;
            unsigned int eth_core_ver:8;
            unsigned int eth_core_rev:8;
#           else
            unsigned int eth_core_rev:8;
            unsigned int eth_core_ver:8;
            unsigned int eth_cust_ver:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_scratch {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_scratch:32;
#           else
            unsigned int eth_scratch:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_cmd_config {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_30:2;
            unsigned int eth_force_send_rf:1;
            unsigned int eth_force_send_lf:1;
            unsigned int eth_disable_flt_hdl:1;
            unsigned int __reserved_23:4;
            unsigned int eth_tx_flush:1;
            unsigned int eth_rx_sfd_any:1;
            unsigned int eth_pause_pfc_comp:1;
            unsigned int eth_pfc_mode:1;
            unsigned int eth_rs_col_cnt_ext:1;
            unsigned int eth_no_length_check:1;
            unsigned int eth_send_idle:1;
            unsigned int eth_phy_tx_enable:1;
            unsigned int eth_rx_err_discard:1;
            unsigned int eth_cmd_frame_ena:1;
            unsigned int eth_sw_reset:1;
            unsigned int eth_tx_pad_en:1;
            unsigned int eth_loopback_en:1;
            unsigned int eth_tx_addr_insert:1;
            unsigned int eth_pause_ignore:1;
            unsigned int eth_pause_fwd:1;
            unsigned int eth_crc_fwd:1;
            unsigned int eth_pad_en:1;
            unsigned int eth_promiscuous_en:1;
            unsigned int eth_wan_mode:1;
            unsigned int __reserved_2:1;
            unsigned int eth_rx_ena:1;
            unsigned int eth_tx_ena:1;
#           else
            unsigned int eth_tx_ena:1;
            unsigned int eth_rx_ena:1;
            unsigned int __reserved_2:1;
            unsigned int eth_wan_mode:1;
            unsigned int eth_promiscuous_en:1;
            unsigned int eth_pad_en:1;
            unsigned int eth_crc_fwd:1;
            unsigned int eth_pause_fwd:1;
            unsigned int eth_pause_ignore:1;
            unsigned int eth_tx_addr_insert:1;
            unsigned int eth_loopback_en:1;
            unsigned int eth_tx_pad_en:1;
            unsigned int eth_sw_reset:1;
            unsigned int eth_cmd_frame_ena:1;
            unsigned int eth_rx_err_discard:1;
            unsigned int eth_phy_tx_enable:1;
            unsigned int eth_send_idle:1;
            unsigned int eth_no_length_check:1;
            unsigned int eth_rs_col_cnt_ext:1;
            unsigned int eth_pfc_mode:1;
            unsigned int eth_pause_pfc_comp:1;
            unsigned int eth_rx_sfd_any:1;
            unsigned int eth_tx_flush:1;
            unsigned int __reserved_23:4;
            unsigned int eth_disable_flt_hdl:1;
            unsigned int eth_force_send_lf:1;
            unsigned int eth_force_send_rf:1;
            unsigned int __reserved_30:2;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_mac_addr_0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_mac_addr_0:32;
#           else
            unsigned int eth_mac_addr_0:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_mac_addr_1 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int eth_mac_addr_1:16;
#           else
            unsigned int eth_mac_addr_1:16;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_frame_length {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int eth_frame_length:16;
#           else
            unsigned int eth_frame_length:16;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_rx_fifo_sections {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_rx_section_empty_wm:16;
            unsigned int eth_rx_section_avail_wm:16;
#           else
            unsigned int eth_rx_section_avail_wm:16;
            unsigned int eth_rx_section_empty_wm:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_tx_fifo_sections {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_tx_section_empty_wm:16;
            unsigned int eth_tx_section_avail_wm:16;
#           else
            unsigned int eth_tx_section_avail_wm:16;
            unsigned int eth_tx_section_empty_wm:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_rx_fifo_almost_full_empty {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_rx_fifo_almost_full_wm:16;
            unsigned int eth_rx_fifo_almost_empty_wm:16;
#           else
            unsigned int eth_rx_fifo_almost_empty_wm:16;
            unsigned int eth_rx_fifo_almost_full_wm:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_tx_fifo_almost_full_empty {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_tx_fifo_almost_full_wm:16;
            unsigned int eth_tx_fifo_almost_empty_wm:16;
#           else
            unsigned int eth_tx_fifo_almost_empty_wm:16;
            unsigned int eth_tx_fifo_almost_full_wm:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_hash_table_load {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_9:23;
            unsigned int eth_hash_table_mc_en:1;
            unsigned int __reserved_6:2;
            unsigned int eth_hash_table_addr:6;
#           else
            unsigned int eth_hash_table_addr:6;
            unsigned int __reserved_6:2;
            unsigned int eth_hash_table_mc_en:1;
            unsigned int __reserved_9:23;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_status {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_4:28;
            unsigned int eth_ts_available:1;
            unsigned int eth_phy_los:1;
            unsigned int eth_rx_remote_fault:1;
            unsigned int eth_rx_local_fault:1;
#           else
            unsigned int eth_rx_local_fault:1;
            unsigned int eth_rx_remote_fault:1;
            unsigned int eth_phy_los:1;
            unsigned int eth_ts_available:1;
            unsigned int __reserved_4:28;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_tx_ipg_length {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_7:25;
            unsigned int eth_tx_ipg_length:7;
#           else
            unsigned int eth_tx_ipg_length:7;
            unsigned int __reserved_7:25;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_credit_trigger {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_1:31;
            unsigned int eth_credit_trigger:1;
#           else
            unsigned int eth_credit_trigger:1;
            unsigned int __reserved_1:31;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_init_credit {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_8:24;
            unsigned int eth_init_credit:8;
#           else
            unsigned int eth_init_credit:8;
            unsigned int __reserved_8:24;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_credit_reg {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_8:24;
            unsigned int eth_credit_reg:8;
#           else
            unsigned int eth_credit_reg:8;
            unsigned int __reserved_8:24;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_pause_quanta_cl01 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_pause_quanta_cl1:16;
            unsigned int eth_pause_quanta_cl0:16;
#           else
            unsigned int eth_pause_quanta_cl0:16;
            unsigned int eth_pause_quanta_cl1:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_pause_quanta_cl23 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_pause_quanta_cl3:16;
            unsigned int eth_pause_quanta_cl2:16;
#           else
            unsigned int eth_pause_quanta_cl2:16;
            unsigned int eth_pause_quanta_cl3:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_pause_quanta_cl45 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_pause_quanta_cl5:16;
            unsigned int eth_pause_quanta_cl4:16;
#           else
            unsigned int eth_pause_quanta_cl4:16;
            unsigned int eth_pause_quanta_cl5:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_pause_quanta_cl67 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_pause_quanta_cl7:16;
            unsigned int eth_pause_quanta_cl6:16;
#           else
            unsigned int eth_pause_quanta_cl6:16;
            unsigned int eth_pause_quanta_cl7:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_quanta_thresh_cl01 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_quanta_thresh_cl1:16;
            unsigned int eth_quanta_thresh_cl0:16;
#           else
            unsigned int eth_quanta_thresh_cl0:16;
            unsigned int eth_quanta_thresh_cl1:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_quanta_thresh_cl23 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_quanta_thresh_cl3:16;
            unsigned int eth_quanta_thresh_cl2:16;
#           else
            unsigned int eth_quanta_thresh_cl2:16;
            unsigned int eth_quanta_thresh_cl3:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_quanta_thresh_cl45 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_quanta_thresh_cl5:16;
            unsigned int eth_quanta_thresh_cl4:16;
#           else
            unsigned int eth_quanta_thresh_cl4:16;
            unsigned int eth_quanta_thresh_cl5:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_quanta_thresh_cl67 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_quanta_thresh_cl7:16;
            unsigned int eth_quanta_thresh_cl6:16;
#           else
            unsigned int eth_quanta_thresh_cl6:16;
            unsigned int eth_quanta_thresh_cl7:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_rx_pause_status {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_8:24;
            unsigned int eth_rx_pause_status:8;
#           else
            unsigned int eth_rx_pause_status:8;
            unsigned int __reserved_8:24;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_timestamp {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_timestamp:32;
#           else
            unsigned int eth_timestamp:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_tx_preamble_0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int eth_tx_preamble_0:32;
#           else
            unsigned int eth_tx_preamble_0:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_tx_preamble_1 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_24:8;
            unsigned int eth_tx_preamble_1:24;
#           else
            unsigned int eth_tx_preamble_1:24;
            unsigned int __reserved_24:8;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_sgmii_pcs_ctl {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int eth_pcs_reset:1;
            unsigned int eth_phy_loopback:1;
            unsigned int eth_sgmii_speed_sel_0:1;
            unsigned int eth_autoneg_enable:1;
            unsigned int eth_power_down:1;
            unsigned int eth_isolate:1;
            unsigned int eth_restart_autoneg:1;
            unsigned int eth_duplex_mode:1;
            unsigned int eth_collision_test:1;
            unsigned int eth_sgmii_speed_sel_1:1;
            unsigned int __reserved_0:6;
#           else
            unsigned int __reserved_0:6;
            unsigned int eth_sgmii_speed_sel_1:1;
            unsigned int eth_collision_test:1;
            unsigned int eth_duplex_mode:1;
            unsigned int eth_restart_autoneg:1;
            unsigned int eth_isolate:1;
            unsigned int eth_power_down:1;
            unsigned int eth_autoneg_enable:1;
            unsigned int eth_sgmii_speed_sel_0:1;
            unsigned int eth_phy_loopback:1;
            unsigned int eth_pcs_reset:1;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_sgmii_pcs_status {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int eth_100baset4:1;
            unsigned int eth_100basex_half_duplex:2;
            unsigned int eth_10m_half_duplex:2;
            unsigned int eth_100baset2_half_duplex:2;
            unsigned int eth_extended_status:1;
            unsigned int __reserved_6:2;
            unsigned int eth_autoneg_complete:1;
            unsigned int eth_remote_fault:1;
            unsigned int eth_autoneg_ability:1;
            unsigned int eth_link_status:1;
            unsigned int eth_jabber_detect:1;
            unsigned int eth_ext_capable:1;
#           else
            unsigned int eth_ext_capable:1;
            unsigned int eth_jabber_detect:1;
            unsigned int eth_link_status:1;
            unsigned int eth_autoneg_ability:1;
            unsigned int eth_remote_fault:1;
            unsigned int eth_autoneg_complete:1;
            unsigned int __reserved_6:2;
            unsigned int eth_extended_status:1;
            unsigned int eth_100baset2_half_duplex:2;
            unsigned int eth_10m_half_duplex:2;
            unsigned int eth_100basex_half_duplex:2;
            unsigned int eth_100baset4:1;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_sgmii_phy_ident0 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int eth_phy_identifier0:16;
#           else
            unsigned int eth_phy_identifier0:16;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_sgmii_phy_ident1 {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int eth_phy_identifier1:16;
#           else
            unsigned int eth_phy_identifier1:16;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_dev_ability {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int eth_np:1;
            unsigned int eth_ack:1;
            unsigned int eth_remote_fault:2;
            unsigned int __reserved_9:3;
            unsigned int eth_pause_support2:1;
            unsigned int eth_pause_support1:1;
            unsigned int eth_half_duplex:1;
            unsigned int eth_full_duplex:1;
            unsigned int __reserved_0:5;
#           else
            unsigned int __reserved_0:5;
            unsigned int eth_full_duplex:1;
            unsigned int eth_half_duplex:1;
            unsigned int eth_pause_support1:1;
            unsigned int eth_pause_support2:1;
            unsigned int __reserved_9:3;
            unsigned int eth_remote_fault:2;
            unsigned int eth_ack:1;
            unsigned int eth_np:1;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_partner_ability {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int eth_copper_link_status:1;
            unsigned int eth_ack:1;
            unsigned int __reserved_13:1;
            unsigned int eth_copper_duplex_status:1;
            unsigned int eth_copper_speed:2;
            unsigned int __reserved_0:10;
#           else
            unsigned int __reserved_0:10;
            unsigned int eth_copper_speed:2;
            unsigned int eth_copper_duplex_status:1;
            unsigned int __reserved_13:1;
            unsigned int eth_ack:1;
            unsigned int eth_copper_link_status:1;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_an_expansion {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_2:30;
            unsigned int eth_latched_page_rcvd:1;
            unsigned int eth_real_time_page_rcvd:1;
#           else
            unsigned int eth_real_time_page_rcvd:1;
            unsigned int eth_latched_page_rcvd:1;
            unsigned int __reserved_2:30;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_device_next_page {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_0:32;
#           else
            unsigned int __reserved_0:32;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_link_timer_lo {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_16:16;
            unsigned int eth_link_timer_lo:16;
#           else
            unsigned int eth_link_timer_lo:16;
            unsigned int __reserved_16:16;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_link_timer_hi {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_5:27;
            unsigned int eth_link_timer_hi:5;
#           else
            unsigned int eth_link_timer_hi:5;
            unsigned int __reserved_5:27;
#           endif
        };
        unsigned int __raw;
    };
};

struct nfp_mac_eth_mac_seg_eth_if_mode {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_6:26;
            unsigned int eth_sgmii_pcs_enable:1;
            unsigned int eth_sgmii_hduplex:1;
            unsigned int eth_sgmii_speed:2;
            unsigned int eth_use_sgmii_an:1;
            unsigned int eth_sgmii_ena:1;
#           else
            unsigned int eth_sgmii_ena:1;
            unsigned int eth_use_sgmii_an:1;
            unsigned int eth_sgmii_speed:2;
            unsigned int eth_sgmii_hduplex:1;
            unsigned int eth_sgmii_pcs_enable:1;
            unsigned int __reserved_6:26;
#           endif
        };
        unsigned int __raw;
    };
};



/* EthPortStatsHyd */
/* ChannelStatsSeg */

/* EthPortStatsSeg */


/*
 * Register: StatCounter32
 *   [31:0]    StatCounter32
 */
struct nfp_mac_eth_port_stats_seg_rx_pif_in_octets_lo {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int stat_counter_32:32;
#           else
            unsigned int stat_counter_32:32;
#           endif
        };
        unsigned int __raw;
    };
};


/*
 * Register: StatCounter8
 *   [7:0]     StatCounter8
 */
struct nfp_mac_eth_port_stats_seg_rx_pif_in_octets_hi {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_8:24;
            unsigned int stat_counter_8:8;
#           else
            unsigned int stat_counter_8:8;
            unsigned int __reserved_8:24;
#           endif
        };
        unsigned int __raw;
    };
};



/*
 * Register: StatCounter32
 *   [31:0]    StatCounter32
 */
struct nfp_mac_chnl_stats_rx_cif_in_octets_lo {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int stat_counter_32:32;
#           else
            unsigned int stat_counter_32:32;
#           endif
        };
        unsigned int __raw;
    };
};


/*
 * Register: StatCounter8
 *   [7:0]     StatCounter8
 */
struct nfp_mac_chnl_stats_rx_cif_in_octets_hi {
    union {
        struct {
#           ifdef BIGENDIAN
            unsigned int __reserved_8:24;
            unsigned int stat_counter_8:8;
#           else
            unsigned int stat_counter_8:8;
            unsigned int __reserved_8:24;
#           endif
        };
        unsigned int __raw;
    };
};



#endif /* __NFP_LANG_MICROC */

#endif /* !_NFP6000__NFP_MAC_H_ */
