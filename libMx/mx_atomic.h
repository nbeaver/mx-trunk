/*
 * Name:    mx_atomic.h
 *
 * Purpose: Atomic operation functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_ATOMIC_H__
#define __MX_ATOMIC_H__

#include "mx_stdint.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API void mx_atomic_initialize( void );

/*---*/

MX_API int32_t mx_atomic_add32( int32_t *, int32_t );

MX_API int32_t mx_atomic_decrement32( int32_t * );

MX_API int32_t mx_atomic_increment32( int32_t * );

MX_API int32_t mx_atomic_read32( int32_t * );

MX_API void mx_atomic_write32( int32_t *, int32_t );

#ifdef __cplusplus
}
#endif

#endif /* __MX_ATOMIC_H__ */
