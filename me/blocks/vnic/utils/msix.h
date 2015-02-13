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
  * Returns MSI mask status for given interrupt vector number and virtual function
  * @param pcie_nr     - PCIe cluster number
  * @param vf_nr       - virtual function number (0 to 15)
  * @param vec_nr      - interrupt vector number
  * @return            - the status of the interrupt vector
  *                      0 - interrupt not masked
  *                      1 - interrupt masked
  */
unsigned int msi_vf_status(unsigned int pcie_nr, unsigned int vf_nr, unsigned int vec_nr); 

/**
  * Send MSI interrupt for specified virtual function
  * @param pcie_nr     - PCIe cluster number
  * @param vf_nr       - virtual function number (0 to 15)
  * @param vec_nr      - interrupt vector number
  * @param mask_en     - specifies if the interrupt should be masked
  *                      masked after sending. 
  *                      0 - do not mask after sending
  *                      1 - mask after sending
  */
void msi_vf_send(unsigned int pcie_nr, unsigned int vf_nr,  unsigned int vec_nr, unsigned int  mask_en);

/**
  * Returns MSI-X mask status for given interrupt vector number
  * @param pcie_nr     - PCIe cluster number
  * @param vec_nr      - interrupt vector number
  * @return            - the status of the interrupt vector
  *                      0 - interrupt not masked
  *                      1 - interrupt masked
  */
unsigned int  msix_status(unsigned int  pcie_nr, unsigned int  vec_nr); 

/**
  * Send MSI-X interrupt 
  * @param pcie_nr     - PCIe cluster number
  * @param vec_nr      - interrupt vector number
  * @param mask_en     - specifies if the interrupt should be masked
  *                      masked after sending. 
  *                      0 - do not mask after sending
  *                      1 - mask after sending
  */
void msix_send(unsigned int pcie_nr, unsigned int vec_nr, unsigned int mask_en);

