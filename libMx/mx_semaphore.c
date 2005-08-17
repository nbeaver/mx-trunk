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

#define MX_SEMAPHORE_DEBUG	FALSE

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

/************************ System V Semaphores ***********************/

#elif defined(OS_MACOSX)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#if defined(_POSIX_THREADS)
#  include <pthread.h>
#endif

#if defined(OS_MACOSX)
#  define SEMVMX	32767
#endif

typedef struct {
	key_t semaphore_key;
	int semaphore_id;

	uid_t creator_process;
#if defined(_POSIX_THREADS)
	pthread_t creator_thread;
#endif
} MX_SYSTEM_V_SEMAPHORE_PRIVATE;

MX_EXPORT mx_status_type
mx_semaphore_create( MX_SEMAPHORE **semaphore,
			unsigned long initial_value,
			char *name )
{
	static const char fname[] = "mx_semaphore_create()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	int create_new_semaphore, num_semaphores, semget_flags;
	int status, saved_errno;
	size_t name_length;
	union semun value_union;

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

	system_v_private = (MX_SYSTEM_V_SEMAPHORE_PRIVATE *)
				malloc( sizeof(MX_SYSTEM_V_SEMAPHORE_PRIVATE) );

	if ( system_v_private == (MX_SYSTEM_V_SEMAPHORE_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate memory for a "
			"MX_SYSTEM_V_SEMAPHORE_PRIVATE structure." );
	}

	(*semaphore)->semaphore_ptr = system_v_private;

	/* Is the initial value of the semaphore greater than the 
	 * maximum allowed value?
	 */

	if ( initial_value > SEMVMX ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested initial value (%lu) for the semaphore "
		"is larger than the maximum allowed value of %lu.",
			initial_value, (unsigned long) SEMVMX );
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

	/* Get the semaphore key. */

	create_new_semaphore = FALSE;

	if ( (*semaphore)->name == NULL ) {

		/* No name was specified, so we create a new
		 * private semaphore.
		 */

		system_v_private->semaphore_key = IPC_PRIVATE;

		create_new_semaphore = TRUE;
	} else {
		/* The specified semaphore name is expected to be a filename
		 * in the file system.  If the named file does not already
		 * exist, then we assume that we are creating a new semaphore.
		 * If it does exist, then we assume that we are connecting
		 * to an existing semaphore.
		 *
		 * FIXME: The necessary logic has not yet been written!
		 */

		/* Construct the semaphore key from the filename.
		 * We always use 1 as the integer identifier.
		 */

		system_v_private->semaphore_key = ftok((*semaphore)->name, 1);

		if ( system_v_private->semaphore_key == (-1) ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The semaphore file '%s' does not exist or cannot "
			"be accessed by this program.",
				(*semaphore)->name );
		}
	}

	if ( create_new_semaphore == FALSE ) {
		num_semaphores = 0;
		semget_flags = 0;
	} else {
		/* Create a new semaphore. */

		num_semaphores = 1;

		semget_flags = IPC_CREAT;

		/* Allow access by anyone. */

		semget_flags |= SEM_R | SEM_A ;
		semget_flags |= (SEM_R>>3) | (SEM_A>>3);
		semget_flags |= (SEM_R>>6) | (SEM_A>>6);
	}

	/* Create or connect to the semaphore. */

	system_v_private->semaphore_id =
		semget( system_v_private->semaphore_key,
				num_semaphores, semget_flags );

	if ( (system_v_private->semaphore_id) == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Cannot connect to semaphore key %lu, file '%s'.  "
		"Errno = %d, error message = '%s'.",
			(unsigned long) system_v_private->semaphore_key,
			(*semaphore)->name,
			saved_errno, strerror(saved_errno) );
	}

	/* If necessary, set the initial value of the semaphore.
	 *
	 * WARNING: Note that the gap in time between semget() and semctl()
	 *          theoretically leaves a window of opportunity for a race
	 *          condition, so this function is not really thread safe.
	 */

	if ( create_new_semaphore ) {

		value_union.val = (int) initial_value;

		status = semctl( system_v_private->semaphore_id, 0,
							SETVAL, value_union );

		if ( status != 0 ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EINVAL:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Either the semaphore key %lu, filename '%s' does not "
			"exist or semaphore number 0 was not found.",
					system_v_private->semaphore_key,
					(*semaphore)->name );
				break;
			case EPERM:
				return mx_error( MXE_PERMISSION_DENIED, fname,
			"The current process lacks the appropriated privilege "
			"to access the semaphore." );
				break;
			case EACCES:
				return mx_error( MXE_TYPE_MISMATCH, fname,
			"The operation and the mode of the semaphore set "
			"do not match." );
			    	break;
			case ERANGE:
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Attempted to set semaphore value (%lu) outside the allowed "
		"range of 0 to %d.", initial_value, SEMVMX );
				break;
			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected semctl() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
				break;
			}
		}

		system_v_private->creator_process = getuid();

#if defined(_POSIX_THREADS)
		system_v_private->creator_thread = pthread_self();
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_semaphore_destroy( MX_SEMAPHORE *semaphore )
{
	static const char fname[] = "mx_semaphore_destroy()";

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	int status, saved_errno, destroy_semaphore;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( semaphore == (MX_SEMAPHORE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEMAPHORE pointer passed was NULL." );
	}

	system_v_private = semaphore->semaphore_ptr;

	if ( system_v_private == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	destroy_semaphore = FALSE;

	if ( semaphore->name == NULL ) {
		destroy_semaphore = TRUE;
	} else {
		if ( system_v_private->creator_process == getuid() ) {
#if defined(_POSIX_THREADS)
		    if ( system_v_private->creator_thread == pthread_self() ){
			destroy_semaphore = TRUE;
		    }
#else
		    destroy_semaphore = TRUE;
#endif
		}
	}

#if MX_SEMAPHORE_DEBUG
	MX_DEBUG(-2,("%s: destroy_semaphore = %d", fname, destroy_semaphore));
#endif

	if ( destroy_semaphore ) {
		status = semctl( system_v_private->semaphore_id, 0, IPC_RMID );

		if ( status != 0 ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EINVAL:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Either the semaphore key %lu, filename '%s' does not "
			"exist or semaphore number 0 was not found.",
					system_v_private->semaphore_key,
					semaphore->name );
				break;
			case EPERM:
				return mx_error( MXE_PERMISSION_DENIED, fname,
			"The current process lacks the appropriated privilege "
			"to access the semaphore." );
				break;
			case EACCES:
				return mx_error( MXE_TYPE_MISMATCH, fname,
			"The operation and the mode of the semaphore set "
			"do not match." );
			    	break;
			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected semctl() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
				break;
			}
		}
	}

	if ( semaphore->name != NULL ) {
		mx_free( semaphore->name );
	}

	mx_free( system_v_private );

	mx_free( semaphore );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_semaphore_lock( MX_SEMAPHORE *semaphore )
{
	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	struct sembuf sembuf_struct;
	int status, saved_errno;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	system_v_private = semaphore->semaphore_ptr;

	if ( system_v_private == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	sembuf_struct.sem_num = 0;
	sembuf_struct.sem_op = -1;
	sembuf_struct.sem_flg = 0;

	status = semop( system_v_private->semaphore_id, &sembuf_struct, 1 );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EACCES:
			return MXE_TYPE_MISMATCH;
			break;
		case EAGAIN:
			return MXE_MIGHT_CAUSE_DEADLOCK;
			break;
		case E2BIG:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		case EFBIG:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EIDRM:
			return MXE_NOT_FOUND;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSPC:
			return MXE_OUT_OF_MEMORY;
			break;
		case ERANGE:
			return MXE_WOULD_EXCEED_LIMIT;
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
	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	struct sembuf sembuf_struct;
	int status, saved_errno;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	system_v_private = semaphore->semaphore_ptr;

	if ( system_v_private == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	sembuf_struct.sem_num = 0;
	sembuf_struct.sem_op  = 1;
	sembuf_struct.sem_flg = 0;

	status = semop( system_v_private->semaphore_id, &sembuf_struct, 1 );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EACCES:
			return MXE_TYPE_MISMATCH;
			break;
		case EAGAIN:
			return MXE_MIGHT_CAUSE_DEADLOCK;
			break;
		case E2BIG:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		case EFBIG:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EIDRM:
			return MXE_NOT_FOUND;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSPC:
			return MXE_OUT_OF_MEMORY;
			break;
		case ERANGE:
			return MXE_WOULD_EXCEED_LIMIT;
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
	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
	struct sembuf sembuf_struct;
	int status, saved_errno;

	if ( semaphore == (MX_SEMAPHORE *) NULL )
		return MXE_NULL_ARGUMENT;

	system_v_private = semaphore->semaphore_ptr;

	if ( system_v_private == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	sembuf_struct.sem_num = 0;
	sembuf_struct.sem_op = -1;
	sembuf_struct.sem_flg = IPC_NOWAIT;

	status = semop( system_v_private->semaphore_id, &sembuf_struct, 1 );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case 0:
			return MXE_SUCCESS;
			break;
		case EINVAL:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EACCES:
			return MXE_TYPE_MISMATCH;
			break;
		case EAGAIN:
			return MXE_NOT_AVAILABLE;
			break;
		case E2BIG:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		case EFBIG:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		case EIDRM:
			return MXE_NOT_FOUND;
			break;
		case EINTR:
			return MXE_INTERRUPTED;
			break;
		case ENOSPC:
			return MXE_OUT_OF_MEMORY;
			break;
		case ERANGE:
			return MXE_WOULD_EXCEED_LIMIT;
			break;
		default:
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

	MX_SYSTEM_V_SEMAPHORE_PRIVATE *system_v_private;
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

	system_v_private = semaphore->semaphore_ptr;

	if ( system_v_private == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The semaphore_ptr field for the MX_SEMAPHORE pointer "
			"passed was NULL.");
	}

	semaphore_value = semctl( system_v_private->semaphore_id, 0, GETVAL );

	if ( semaphore_value < 0 ) {
		saved_errno = errno;

		switch( status ) {
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unexpected semctl() error code %d returned.  "
			"Error message = '%s'.",
				saved_errno, strerror( saved_errno ) );
			break;
		}
	}

	*current_value = (unsigned long) semaphore_value;

	return MX_SUCCESSFUL_RESULT;
}

/************************ Posix Pthreads ***********************/

#elif defined(_POSIX_SEMAPHORES) || defined(OS_IRIX) || defined(OS_MACOSX)

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

		if ( p_semaphore_ptr == (sem_t *) SEM_FAILED ) {
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

