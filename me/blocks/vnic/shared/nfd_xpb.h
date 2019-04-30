/*
 * Copyright (C) 2018-2019,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/shared/nfd_xpb.h
 * @brief         An API to manage access to NFD configuration data
 */

#ifndef _BLOCKS__SHARED_NFD_XPB_H_
#define _BLOCKS__SHARED_NFD_XPB_H_


/*
 * Defines to use to access FLR hardware CSRs
 */
#define NFP_PCIEX_ISL_BASE                                   0x04000000
#define NFP_PCIEX_ISL_shf                                    24

/* Link power state */
/* PcieComponentCfgXpb.PCIeCntrlrConfig0 */
#define NFP_PCIEX_LINK_POWER_STATE                           0x00100060
#define NFP_PCIEX_LINK_POWER_STATE_msk                       0xf
#define NFP_PCIEX_LINK_POWER_STATE_shf                       8

/* How should the PCIe engine bus master disabled on link? */
/* PcieComponentCfgXpb.PCIeCntrlrConfig1 */
#define NFP_PCIEX_BUS_MSTR_DISABLED_CFG                      0x00100064
#define NFP_PCIEX_BUS_MSTR_HOLD_IF_DISABLED_shf              6
#define NFP_PCIEX_BUS_MSTR_DROP_IF_DISABLED_shf              4

/* PCIe error logging */
/* PcieComponentCfgXpb.PCIeCntrlrConfig2 */
#define NFP_PCIEX_PCIE_ERR                                   0x00100068
#define NFP_PCIEX_PCIE_ERR_msk                               0x21c
#define NFP_PCIEX_PCIE_ERR_CORR(x)                           (1 & ((x) >> 2))
#define NFP_PCIEX_PCIE_ERR_NON_FATAL(x)                      (1 & ((x) >> 3))
#define NFP_PCIEX_PCIE_ERR_FATAL(x)                          (1 & ((x) >> 4))
#define NFP_PCIEX_PCIE_ERR_LOCAL(x)                          (1 & ((x) >> 9))

/* Top level FLR CSR */
/* PcieComponentCfgXpb.PCIeCntrlrConfig3 */
#define NFP_PCIEX_FLR_CSR                                    0x0010006c
#define NFP_PCIEX_FLR_CSR_VF_FLR_DONE_CHANNEL_msk            0x3f
#define NFP_PCIEX_FLR_CSR_VF_FLR_DONE_CHANNEL_shf            16
#define NFP_PCIEX_FLR_CSR_VF_FLR_DONE_shf                    15
#define NFP_PCIEX_FLR_CSR_PF_FLR_DONE_shf                    14
#define NFP_PCIEX_FLR_CSR_PF_FLR_IN_PROGRESS_shf             13

/* Testing VF FLRs in progress */
/* PcieComponentCfgXpb.PCIeVfFlrInProg0/1 */
#define NFP_PCIEX_PCIE_VF_FLR_IN_PROGRESS0                   0x00100080
#define NFP_PCIEX_PCIE_VF_FLR_IN_PROGRESS1                   0x00100084

/* PCIe state change events */
/* PcieComponentCfgXpb.PCIeStateChangeStat */
#define NFP_PCIEX_PCIE_STATE_CHANGE_STAT                     0x001000cc
#define NFP_PCIEX_PCIE_STATE_CHANGE_STAT_msk                 0x7f

/* PCIe interrupt manager status */
/* PcieXpb.PcieIntMgr.Status */
#define NFP_PCIEX_PCIE_INT_MGR_STATUS                        0x00130000
/* XXX define the PCIe interrupts we expect and ACK in regular operation,
 * see table "Interrupt Manager PCIe Status Mapping" in databook
 * FLR interrupts are excluded as they take longer to processes */
#define NFP_PCIEX_PCIE_INT_MGR_STATUS_RECHK_msk              0xc00f4

/* Vendor defined message */
/* PcieComponentCfgXpb.PCIeCompConfig0 */
#define NFP_PCIEX_VENDOR_MSG                                 0x00100000
#define NFP_PCIEX_VENDOR_MSG_VALID_shf                       11

/* Overlay reset remove bits */
/* IslandControl.ClockResetControl */
#define NFP_PCIEX_CLOCK_RESET_CTRL                           0x44045400
#define NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_msk              0xff
#define NFP_PCIEX_CLOCK_RESET_CTRL_RM_RESET_shf              16

#endif /* !_BLOCKS__SHARED_NFD_XPB_H_ */
