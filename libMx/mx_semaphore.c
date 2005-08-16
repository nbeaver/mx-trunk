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

	/* Allocate space for a name if one was specified. */

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

#elif defined(_POSIX_SEMAPHORES) || defined(OS_IRIX)

#include <fcntl.h>
#include <semaphore.h>

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			unsigned long initial_value,
			char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	sem_t *p_semaphore_ptr;
	int status, saved_errno;
	unsigned long sem_value_max;
	size_t name_length;

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

	/* Is the initial value of the semaphore greater than the 
	 * maximum allowed value?
	 */
#if defined(OS_SOLARIS) || defined(OS_IRIX)
	sem_value_max = sysconf(_SC_SEM_VALUE_MAX);
#else
	sem_value_max = SEM_VALUE_MAX;
#endif

	if ( initial_value > sem_value_max ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested initial value (%lu) for the semaphore "
		"is larger than the maximum allowed value of %lu.",
			initial_value, sem_value_max );
	}

	/* Allocate space for a name if one was specified. */

	if ( name == NULL ) {
		(*semaphore)->name = NULL;
	} else {
		name_length = strlen( name );

		name_length++;

		(*semaphore)->name = (char *)
					malloc( name_length * sizeof(char) );

		mx_strncpy( (*semaphore)->name, name, name_length );
	}

	/* Create the semaphore. */

	if ( (*semaphore)->name == NULL ) {

		/* Create an unnamed semaphore. */

		status = sem_init( p_semaphore_ptr, 0,
					(unsigned int) initial_value );

		if ( status != 0 ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EINVAL:

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

	} else {
		/* Create a named semaphore. */

		p_semaphore_ptr = sem_open( (*semaphore)->name,
					O_CREAT,
					0644,
					(unsigned int) initial_value );

		if ( p_semaphore_ptr == SEM_FAILED ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EACCES:
				return mx_error( MXE_PERMISSION_DENIED, fname,
				"Could not connect to semaphore '%s'.",
					(*semaphore)->name );
				break;
			case EEXIST:
				return mx_error( MXE_PERMISSION_DENIED, fname,
				"Semaphore '%s' already exists, but you "
				"requested exclusive access.",
					(*semaphore)->name );
				break;
			case EINTR:
				return mx_error( MXE_INTERRUPTED, fname,
			    "sem_open() for semaphore '%s' was interrupted.",
			    		(*semaphore)->name );
				break;
			case EINVAL:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Either sem_open() is not supported for "
				"semaphore '%s' or the requested initial "
				"value %lu was larger than the maximum (%lu).",
					(*semaphore)->name,
					initial_value,
					sem_value_max );
				break;
			case EMFILE:
				return mx_error( MXE_NOT_AVAILABLE, fname,
				"Ran out of semaphore or file descriptors "
				"for this process." );
				break;
			case ENAMETOOLONG:
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Semaphore name '%s' is longer than the "
				"maximum allowed length for this system.",
					(*semaphore)->name );
				break;
			case ENFILE:
				return mx_error( MXE_NOT_AVAILABLE, fname,
				"Too many semaphores are currently open "
				"on this computer." );
				break;
			case ENOENT:
				return mx_error( MXE_NOT_FOUND, fname,
				"Semaphore '%s' was not found and the call "
				"to sem_open() did not specify O_CREAT.",
					(*semaphore)->name );
				break;
			case ENOSPC:
				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"There is insufficient space for the creation "
				"of the new named semaphore." );
				break;
			case ENOSYS:
				return mx_error( MXE_UNSUPPORTED, fname,
				"The function sem_open() is not supported "
				"on this platform." );
				break;
			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected sem_open() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
				break;
			}
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

	if ( semaphore->name == NULL ) {
		/* Destroy an unnamed semaphore. */

		status = sem_destroy( p_semaphore_ptr );
	} else {
		/* Close a named semaphore. */

		status = sem_close( p_semaphore_ptr );
	}

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
				"The semaphore passed was not "
				"a valid semaphore." );
			break;
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"Posix semaphores are not supported on this platform." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			break;
		}
	}

	if ( semaphore->name != NULL ) {
		mx_free( semaphore->name );
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

