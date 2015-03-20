/*
 * Copyright 2015 Netronome, Inc.
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
 * @file   msix.h       
 * @brief  header file for MSI/MSI-X library 
 */

/**
  * Send MSI-X interrupt for a PF, and optionally mask the interrupt
  *
  * Returns 0 on success and non-zero when the entry is masked.
  *
  * @param pcie_nr     PCIe cluster number
  * @param entry_nr    MSI-X table entry number
  * @param mask_en     Boolean, should interrupt be masked after sending.
  * @return            0 on success, else the interrupt was masked.
  */
__intrinsic int msix_pf_send(unsigned int pcie_nr,
                             unsigned int entry_nr, unsigned int mask_en);

/**
  * Send MSI-X interrupt for specified virtual function and optionally mask
  * @param pcie_nr     PCIe cluster number
  * @param vf_nr       Virtual function number (0 to 15)
  * @param entry_nr    MSI-X table entry number
  * @param mask_en     Boolean, should interrupt be masked after sending.
  * @return            0 on success, else the interrupt was masked.
  */
__intrinsic int msix_vf_send(unsigned int pcie_nr, unsigned int vf_nr,
                             unsigned int entry_nr, unsigned int  mask_en);
