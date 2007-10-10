/*
 * Name:    mx_mutex.h
 *
 * Purpose: Header file for MX mutex functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MUTEX_H__
#define __MX_MUTEX_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *mutex_ptr;
} MX_MUTEX;

MX_API mx_status_type mx_mutex_create( MX_MUTEX **mutex );

MX_API mx_status_type mx_mutex_destroy( MX_MUTEX *mutex );

/* The normal mx_error() function uses sprintf() to format the error messages.
 * However, we should not assume that sprintf() is thread safe, so for the
 * functions below, we return raw mx_status codes rather than the full
 * mx_status_type structure.
 */

MX_API long mx_mutex_lock( MX_MUTEX *mutex );

MX_API long mx_mutex_unlock( MX_MUTEX *mutex );

MX_API long mx_mutex_trylock( MX_MUTEX *mutex );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MUTEX_H__ */

