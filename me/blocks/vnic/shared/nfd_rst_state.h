/*
 * Copyright (C) 2018,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/shared/nfd_rst_state.h
 * @brief         An API to manage PCIe reset state in an ME
 */
#ifndef _BLOCKS__SHARED_NFD_RST_STATE_H_
#define _BLOCKS__SHARED_NFD_RST_STATE_H_


/*
 * Use a shared GPR for state.  Bits 0..3 represent PCIe isl 0..3,
 * set for reset and clear for up.
 */
__shared __gpr unsigned int nfd_rst_state = 0;


/*
 * Helper macros for testing island state
 */
#define NFD_RST_STATE_TEST_UP(_isl)             \
    (~nfd_rst_state & (1 << _isl))

#define NFD_RST_STATE_TEST_RST(_isl)            \
    (nfd_rst_state & (1 << _isl))


/**
 * Set the PCIe island reset state to up
 * @param isl           PCIe island to modify (0..3)
 */
__intrinsic void nfd_rst_state_set_up(unsigned int isl)
{
    nfd_rst_state &= ~(1 << isl);
};


/**
 * Set the PCIe island reset state to reset
 * @param isl           PCIe island to modify (0..3)
 */
__intrinsic void nfd_rst_state_set_rst(unsigned int isl)
{
    nfd_rst_state |= (1 << isl);
};


#endif /* !_BLOCKS__SHARED_NFD_RST_STATE_H_ */
