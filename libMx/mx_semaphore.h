/*
 * Name:    mx_semaphore.h
 *
 * Purpose: Header file for MX semaphore functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SEMAPHORE_H__
#define __MX_SEMAPHORE_H__

typedef struct {
	char *name;

	int semaphore_type;
	void *semaphore_ptr;
} MX_SEMAPHORE;

#define MXT_SEM_INVALID	0
#define MXT_SEM_WIN32	1
#define MXT_SEM_SYSV	2
#define MXT_SEM_POSIX	3

MX_API mx_status_type mx_semaphore_create( MX_SEMAPHORE **semaphore,
					unsigned long initial_value,
					char *name );

MX_API mx_status_type mx_semaphore_destroy( MX_SEMAPHORE *semaphore );

/* The normal mx_error() function uses sprintf() to format the error messages.
 * However, we should not assume that sprintf() is thread safe, so for the
 * functions below, we return raw mx_status codes rather than the full
 * mx_status_type structure.
 */

MX_API long mx_semaphore_lock( MX_SEMAPHORE *semaphore );

MX_API long mx_semaphore_unlock( MX_SEMAPHORE *semaphore );

MX_API long mx_semaphore_trylock( MX_SEMAPHORE *semaphore );

MX_API mx_status_type mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
					unsigned long *current_value );

#endif /* __MX_SEMAPHORE_H__ */

