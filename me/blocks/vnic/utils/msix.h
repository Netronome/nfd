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
 * @file          
 * @brief         
 */

/**
  * Returns the pending status of given MSI interrupt for specified
  * virtual function
  * @param pcie_nr    - the PCIe cluster number
  * @param vf_fn_nr   - identifies the virtual function number
  * @param intrpt_nr  - identifies the interrupt
  * @return 1 if specified MSI interrupt is pending
  *         0 if specified MSI interrupt is not pending
  */
__gpr uint32_t msi_vf_status(__gpr uint32_t pcie_nr, __gpr uint32_t vf_fn_nr, __gpr uint32_t intrpt_nr); 

/**
  * Masks generation of MSI interrupts for a given virtual function 
  * and interrupt
  * @param pcie_nr   - the PCIe cluster number
  * @param vf_fn_nr  - the virtual function number
  *                    (0 to 15)
  *@param intrpt_nr - the interrupt number
  */
void msi_vf_mask(__gpr int32_t pcie_nr, __gpr uint32_t vf_fn_nr, __gpr uint32_t intrpt_nr);

/**
  * Asserts MSI interrupt for specified virtual function
  * and interrupt number
  * 
  * NOTE Even numbered virtual functions do not work on NFP6K
  * 
  * @param pcie_nr    - the PCIe cluster number
  * @param vf_fn_nr   - identifies the virtual function number
  *                     (0 to 15)
  * @param intrpt_nr  - identifies the interrupt
  * @param mask_intrpt - specifies if the interrupt should be optionally
  *                      masked after assertion
  */
void msi_vf_send(__gpr uint32_t pcie_nr, __gpr uint32_t intrpt_nr, __gpr uint32_t vf_fn_nr, __gpr uint32_t mask_intrpt);

/**
  * Retrieves MSI-X mask status for given interrupt number
  * @param pcie_nr    the PCIe cluster number
  * @param intrpt_nr  identifies the interrupt 
  * @param ret_status provides the status of the interrupt vector
  *        0 - interrupt not masked
  *        1 - interrupt masked
  */
__gpr uint32_t msix_status(__gpr uint32_t pcie_nr, __gpr uint32_t intrpt_nr); 

/**
  * Sets the per-vector mask bit in the MSI-X vector table located
  * in PCIe SRAM
  * @param pcie_nr   the PCIe cluster number
  * @param intrpt_nr identifies the interrupt number
  */
void msix_mask(__gpr int32_t pcie_nr, __gpr uint32_t intrpt_nr);

// DEBUG
void msix_un_mask(__gpr int32_t pcie_nr, __gpr uint32_t intrpt_nr);

/**
  * Asserts MSI-X interrupt 
  * @param pcie_nr   the PCIe cluster number
  * @param intrpt_nr identifies the interrupt
  * @param mask_intrpt - specifies if the interrupt should be optionally
  *                      masked after assertion
  */
void msix_send(__gpr uint32_t pcie_nr, __gpr uint32_t intrpt_nr, __gpr uint32_t mask_intrpt);

