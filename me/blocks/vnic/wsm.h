/*
 * Copyright (C) 2013-2016,  Netronome Systems, Inc.  All rights reserved.
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
 * @file          blocks/vnic/wsm.h
 * @brief         Extraction/set utilities for _wrd/_shf/_msk defined fields
 */

#ifndef __WSM_H
#define __WSM_H


#define SM_GET(_var, _fld)                                      \
    (((_var) >> _fld##_shf) & _fld##_msk)

#define SM_CLR(_var, _fld)                                      \
    do {                                                        \
        (_var) = (_var) & ~(_fld##_msk << _fld##_shf);          \
    } while (0)

#define SM_SET_NOCLR(_var, _fld, _val)                          \
    do {                                                        \
        (_var) |= (((_val) & _fld##_msk) << _fld##_shf);        \
    } while (0)

#define SM_SET(_var, _fld, _val)                                \
    do {                                                        \
        (_var) = (_var) & ~(_fld##_msk << _fld##_shf);          \
        (_var) |= (((_val) & _fld##_msk) << _fld##_shf);        \
    } while (0)

#define SM_VAL(_fld, _val)                                      \
    (((_val) & _fld##_msk) << _fld##_shf)

#define WSM_GET(_var, _fld)                                     \
    SM_GET(((uint32_t *)(_var))[_fld##_wrd], _fld)

#define WSM_CLR(_var, _fld)                                     \
    SM_CLR(((uint32_t *)(_var))[_fld##_wrd], _fld)

#define WSM_SET_NOCLR(_var, _fld, _val)                         \
    SM_SET_NOCLR(((uint32_t *)(_var))[_fld##_wrd], _fld, _val)

#define WSM_SET(_var, _fld, _val)                               \
    SM_SET(((uint32_t *)(_var))[_fld##_wrd], _fld, _val)

#endif /* __WSM_H */
