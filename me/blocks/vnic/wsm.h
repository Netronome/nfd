/*
 * Copyright (C) 2013-2016,  Netronome Systems, Inc.  All rights reserved.
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
