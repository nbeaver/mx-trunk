/*
 * Name:    mx_semaphore.h
 *
 * Purpose: Header file for MX semaphore functions.
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

#ifndef __MX_SEMAPHORE_H__
#define __MX_SEMAPHORE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char *name;

	int semaphore_type;
	void *private_ptr;
} MX_SEMAPHORE;

#define MXT_SEM_INVALID	0
#define MXT_SEM_NONE	1
#define MXT_SEM_WIN32	2
#define MXT_SEM_SYSV	3
#define MXT_SEM_POSIX	4
#define MXT_SEM_VMS	5

/* Note: If 'initial_value' is less than zero, then we are attempting
 *       to attach to an existing semaphore, rather than create a new
 *       semaphore.
 */

#define mx_semaphore_open(s,n)		mx_semaphore_create((s), -1, (n))

MX_API mx_status_type mx_semaphore_create( MX_SEMAPHORE **semaphore,
						long initial_value,
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

#ifdef __cplusplus
}
#endif

#endif /* __MX_SEMAPHORE_H__ */

