/*
 * Name:    mx_semaphore.c
 *
 * Purpose: MX semaphore functions.
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_semaphore.h"

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

#include <windows.h>

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
		unsigned long initial_value,
		char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	HANDLE *semaphore_handle_ptr;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	size_t name_length;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	semaphore_handle_ptr = (HANDLE *) malloc( sizeof(HANDLE) );

	if ( semaphore_handle_ptr == (HANDLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a HANDLE pointer." );
	}
	
	(*semaphore)->semaphore_ptr = semaphore_handle_ptr;

	if ( name == NULL ) {
		(*semaphore)->name = NULL;
	} else {
		name_length = strlen( name );

		name_length++;

		(*semaphore)->name = (char *)
					malloc( name_length * sizeof(char) );

		mx_strncpy( (*semaphore)->name, name, name_length );
	}

	*semaphore_handle_ptr = CreateSemaphore( NULL,
						initial_value,
						initial_value,
						(*semaphore)->name );

	if ( *semaphore_handle_ptr == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to create a semaphore.  "
			"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	semaphore_handle_ptr = semaphore->semaphore_ptr;

	if ( semaphore_handle_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	status = CloseHandle( *semaphore_handle_ptr );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to close the Win32 handle for semaphore %p. "
			"Win32 error code = %ld, error_message = '%s'",
			semaphore, last_error_code, message_buffer );
	}

	if ( semaphore->name != NULL ) {
		mx_free( semaphore->name );
	}

	mx_free( semaphore_handle_ptr );

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_lock()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_handle_ptr = semaphore->semaphore_ptr;

	if ( semaphore_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = WaitForSingleObject( *semaphore_handle_ptr, INFINITE );

	switch( status ) {
	case WAIT_ABANDONED:
		return MXE_OBJECT_ABANDONED;
		break;
	case WAIT_OBJECT_0:
		return MXE_SUCCESS;
		break;
	case WAIT_TIMEOUT:
		return MXE_TIMED_OUT;
		break;
	case WAIT_FAILED:
		{
			DWORD last_error_code;
			TCHAR message_buffer[100];

			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unexpected error from WaitForSingleObject() "
				"for semaphore %p. "
				"Win32 error code = %ld, error_message = '%s'",
				semaphore, last_error_code, message_buffer );

			return MXE_OPERATING_SYSTEM_ERROR;
		}
		break;
	}

	return MXE_UNKNOWN_ERROR;
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_unlock()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_handle_ptr = semaphore->semaphore_ptr;

	if ( semaphore_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = ReleaseSemaphore( *semaphore_handle_ptr, 1, NULL );

	if ( status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[100];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error from WaitForSingleObject() "
			"for semaphore %p. "
			"Win32 error code = %ld, error_message = '%s'",
			semaphore, last_error_code, message_buffer );

		return MXE_OPERATING_SYSTEM_ERROR;
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_trylock()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_handle_ptr = semaphore->semaphore_ptr;

	if ( semaphore_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = WaitForSingleObject( *semaphore_handle_ptr, 0 );

	switch( status ) {
	case WAIT_ABANDONED:
		return MXE_OBJECT_ABANDONED;
		break;
	case WAIT_OBJECT_0:
		return MXE_SUCCESS;
		break;
	case WAIT_TIMEOUT:
		return MXE_NOT_AVAILABLE;
		break;
	case WAIT_FAILED:
		{
			DWORD last_error_code;
			TCHAR message_buffer[100];

			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unexpected error from WaitForSingleObject() "
				"for semaphore %p. "
				"Win32 error code = %ld, error_message = '%s'",
				semaphore, last_error_code, message_buffer );

			return MXE_OPERATING_SYSTEM_ERROR;
		}
		break;
	}

	return MXE_UNKNOWN_ERROR;
}

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	HANDLE *semaphore_handle_ptr;
	BOOL status;
	LONG old_value;

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( current_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The current_value pointer passed was NULL." );
	}

	semaphore_handle_ptr = semaphore->semaphore_ptr;

	if ( semaphore_handle_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The semaphore handle pointer for semaphore %p is NULL.",
			semaphore );
	}

	status = ReleaseSemaphore( *semaphore_handle_ptr, 0, &old_value );

	if ( status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[100];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error from WaitForSingleObject() "
			"for semaphore %p. "
			"Win32 error code = %ld, error_message = '%s'",
			semaphore, last_error_code, message_buffer );
	}

	*current_value = (unsigned long) old_value;

	return MX_SUCCESSFUL_RESULT;
}

/************************ Posix Pthreads ***********************/

#elif defined(_POSIX_SEMAPHORES)

#include <semaphore.h>

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			unsigned long initial_value )
{
	static const char fname[] = "mx_semaphore_create()";

	sem_t *p_semaphore_ptr;
	int status, saved_errno;
	unsigned long sem_value_max;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	/* Allocate the data structures we need. */

	*semaphore = (MX_SEMAPHORE *) malloc( sizeof(MX_SEMAPHORE) );

	if ( *semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_SEMAPHORE structure." );
	}

	p_semaphore_ptr = (sem_t *) malloc( sizeof(sem_t) );

	if ( p_semaphore_ptr == (sem_t *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a sem_t structure." );
	}

	(*semaphore)->semaphore_ptr = p_semaphore_ptr;

	/* Create the semaphore. */

	status = sem_init( p_semaphore_ptr, 0, (unsigned int) initial_value );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EINVAL:

#if defined(OS_SOLARIS)
			sem_value_max = sysconf(_SC_SEM_VALUE_MAX);
#else
			sem_value_max = SEM_VALUE_MAX;
#endif
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested initial value %lu exceeds the maximum "
			"allowed value of %lu.", initial_value, sem_value_max );
			break;
		case ENOSPC:
			return mx_error( MXE_NOT_AVAILABLE, fname,
	    "A resource required to create the semaphore is not available.");
		    	break;
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"Posix semaphores are not supported on this platform." );
			break;
		case EPERM:
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The current process lacks the appropriated privilege "
			"to create the semaphore." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected sem_init() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	sem_t *p_semaphore_ptr;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	status = sem_destroy( p_semaphore_ptr );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( status ) {
		case EBUSY:
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
				"Detected an attempt to destroy a semaphore "
				"that is locked or referenced by "
				"another thread." );
			break;
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The semaphore passed to sem_destroy() was not "
				"a valid semaphore." );
			break;
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"Posix semaphores are not supported on this platform." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected sem_destroy() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			break;
		}
	}

	mx_free( p_semaphore_ptr );

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	sem_t *p_semaphore_ptr;
	int status, saved_errno;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = sem_wait( p_semaphore_ptr );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSYS:
			return MXE_UNSUPPORTED;
			break;
		case EDEADLK:
			return MXE_MIGHT_CAUSE_DEADLOCK;
			break;
		default:
			return MXE_UNKNOWN_ERROR;
			break;
		}
	}
	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	sem_t *p_semaphore_ptr;
	int status, saved_errno;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = sem_post( p_semaphore_ptr );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case ENOSYS:
			return MXE_UNSUPPORTED;
			break;
		default:
			return MXE_UNKNOWN_ERROR;
			break;
		}
	}
	return MXE_SUCCESS;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	sem_t *p_semaphore_ptr;
	int status, saved_errno;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = sem_trywait( p_semaphore_ptr );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
#if defined(OS_SOLARIS)
		/* Saw this value returned on Solaris 8.  (WML) */
		case 0:
#endif
		case EAGAIN:
			return MXE_NOT_AVAILABLE;
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSYS:
			return MXE_UNSUPPORTED;
			break;
		case EDEADLK:
			return MXE_MIGHT_CAUSE_DEADLOCK;
			break;
		default:
#if 1
			MX_DEBUG(-2,
		("mx_semaphore_trylock(): errno = %d, error message = '%s'.",
					saved_errno, strerror(saved_errno) ));
#endif
			return MXE_UNKNOWN_ERROR;
			break;
		}
	}
	return MXE_SUCCESS;
}

MX_EXPORT mx_status_type
mx_semaphore_get_value( MX_SEMAPHORE *semaphore,
			unsigned long *current_value )
{
	static const char fname[] = "mx_semaphore_get_value()";

	sem_t *p_semaphore_ptr;
	int status, saved_errno, semaphore_value;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}
	if ( current_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The current_value pointer passed was NULL." );
	}

	p_semaphore_ptr = semaphore->semaphore_ptr;

	if ( p_semaphore_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	status = sem_getvalue( p_semaphore_ptr, &semaphore_value );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( status ) {
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected sem_getvalue() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			break;
		}
	}

	*current_value = (unsigned long) semaphore_value;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif 0

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
		unsigned long initial_value )
{
	static const char fname[] = "mx_semaphore_create()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"MX semaphore functions are not supported for this operating system." );
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"MX semaphore functions are not supported for this operating system." );
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	return MXE_UNSUPPORTED;
}

MX_EXPORT long
mx_semaphore_unlock( MX_SEMAPHORE *semaphore )
{
	return MXE_UNSUPPORTED;
}

MX_EXPORT long
mx_semaphore_trylock( MX_SEMAPHORE *semaphore )
{
	return MXE_UNSUPPORTED;
}

#else

#error MX semaphore functions have not yet been defined for this platform.

#endif

