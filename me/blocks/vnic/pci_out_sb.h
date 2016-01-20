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
 * @file          blocks/vnic/pci_out_sb.h
 * @brief         Implement "stage batch" (SB) task for PCI.OUT
 */

#ifndef __PCI_OUT_SB_H
#define __PCI_OUT_SB_H

/*
 * Work queue descriptor (to Packet DMA MEs):
 *
 * Bit    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
 * -----\ 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * Word  +-+---------------+---------+-------------------+---------------+
 *    0  |E| Requester ID  | Unused  |   Sequence Num    | HostBuf[39:32]|
 *       +-+---------------+---------+-------------------+---------------+
 *    1  |                       Host Buffer [31:0]                      |
 *       +-----------+-+-----------------+---+-+-------------------------+
 *    2  |  CTM ISL  |C|  Packet Number  |SPL|0|     Starting Offset     |
 *       +-+---+-----+-+-----------------+---+-+-------------------------+
 *    3  |N|BLS|           MU Buffer Address [39:11]                     |
 *       +-+---+---------+---------------+-------------------------------+
 *    4  |D| Meta Length |  RX Queue     |           Data Length         |
 *       +-+-------------+---------------+-------------------------------+
 */

// Word 0
#define SB_WQ_ENABLED_bf        0, 31, 31
#define SB_WQ_ENABLED_wrd       0
#define SB_WQ_ENABLED_shf       31
#define SB_WQ_ENABLED_msk       1
#define SB_WQ_RID_bf            0, 30, 23
#define SB_WQ_RID_wrd           0
#define SB_WQ_RID_shf           23
#define SB_WQ_RID_msk           0xFF
#define SB_WQ_SEQ_bf            0, 17, 8
#define SB_WQ_SEQ_wrd           0
#define SB_WQ_SEQ_shf           8
#define SB_WQ_SEQ_msk           0x3FF
#define SB_WQ_HOST_ADDR_HI_bf   0, 7, 0
#define SB_WQ_HOST_ADDR_HI_wrd  0
#define SB_WQ_HOST_ADDR_HI_shf  0
#define SB_WQ_HOST_ADDR_HI_msk  0xFF

// Word 1
#define SB_WQ_HOST_ADDR_LO_bf   1, 31, 0
#define SB_WQ_HOST_ADDR_LO_wrd  1
#define SB_WQ_HOST_ADDR_LO_shf  0
#define SB_WQ_HOST_ADDR_LO_msk  0xFFFFFFFF

// Word 2
#define SB_WQ_CTM_ISL_bf        2, 31, 26
#define SB_WQ_CTM_ISL_wrd       2
#define SB_WQ_CTM_ISL_shf       26
#define SB_WQ_CTM_ISL_msk       0x3F
#define SB_WQ_CTM_ONLY_bf       2, 25, 25
#define SB_WQ_CTM_ONLY_wrd      2
#define SB_WQ_CTM_ONLY_shf      25
#define SB_WQ_CTM_ONLY_msk      0x1
#define SB_WQ_PKT_NUM_bf        2, 24, 16
#define SB_WQ_PKT_NUM_wrd       2
#define SB_WQ_PKT_NUM_shf       16
#define SB_WQ_PKT_NUM_msk       0x1FF
#define SB_WQ_CTM_SPLIT_bf      2, 15, 14
#define SB_WQ_CTM_SPLIT_wrd     2
#define SB_WQ_CTM_SPLIT_shf     14
#define SB_WQ_CTM_SPLIT_msk     0x3
#define SB_WQ_OFFSET_bf         2, 12, 0
#define SB_WQ_OFFSET_wrd        2
#define SB_WQ_OFFSET_shf        0
#define SB_WQ_OFFSET_msk        0x1FFF

// Word 3
#define SB_WQ_NBI_bf            3, 31, 31
#define SB_WQ_NBI_wrd           3
#define SB_WQ_NBI_shf           31
#define SB_WQ_NBI_msk           0x1
#define SB_WQ_BLS_bf            3, 30, 29
#define SB_WQ_BLS_wrd           3
#define SB_WQ_BLS_shf           29
#define SB_WQ_BLS_msk           0x3
#define SB_WQ_MUBUF_bf          3, 28, 0
#define SB_WQ_MUBUF_wrd         3
#define SB_WQ_MUBUF_shf         0
#define SB_WQ_MUBUF_msk         0x1FFFFFFF

// Word 4
#define SB_WQ_DDONE_bf          4, 31, 31
#define SB_WQ_DDONE_wrd         4
#define SB_WQ_DDONE_shf         31
#define SB_WQ_DDONE_msk         0x1
#define SB_WQ_METALEN_bf        4, 30, 24
#define SB_WQ_METALEN_wrd       4
#define SB_WQ_METALEN_shf       24
#define SB_WQ_METALEN_msk       0x7F
#define SB_WQ_QNUM_bf           4, 23, 16
#define SB_WQ_QNUM_wrd          4
#define SB_WQ_QNUM_shf          16
#define SB_WQ_QNUM_msk          0xFF
#define SB_WQ_DATALEN_bf        4, 15, 0
#define SB_WQ_DATALEN_wrd       4
#define SB_WQ_DATALEN_shf       0
#define SB_WQ_DATALEN_msk       0xFFFF

#define SB_WQ_SIZE_LW           5

#define SB_WQ_CREDIT_BATCH      64

#endif /* __PCI_OUT_SB_H */
