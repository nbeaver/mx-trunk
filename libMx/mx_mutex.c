/*
 * Name:    mx_mutex.c
 *
 * Purpose: MX mutex functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2007 Illinois Institute of Technology
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
#include "mx_mutex.h"

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

#include <windows.h>

MX_EXPORT mx_status_type
mx_mutex_create( MX_MUTEX **mutex )
{
	static const char fname[] = "mx_mutex_create()";

	HANDLE *mutex_handle_ptr;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mutex == (MX_MUTEX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	*mutex = (MX_MUTEX *) malloc( sizeof(MX_MUTEX) );

	if ( *mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_MUTEX structure." );
	}

	mutex_handle_ptr = (HANDLE *) malloc( sizeof(HANDLE) );

	if ( mutex_handle_ptr == (HANDLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a HANDLE pointer." );
	}
	
	(*mutex)->mutex_ptr = mutex_handle_ptr;

	*mutex_handle_ptr = CreateMutex( NULL, FALSE, NULL );

	if ( *mutex_handle_ptr == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to create a mutex.  "
			"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mutex_destroy( MX_MUTEX *mutex )
{
	static const char fname[] = "mx_mutex_destroy()";

	HANDLE *mutex_handle_ptr;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	mutex_handle_ptr = mutex->mutex_ptr;

	if ( mutex_handle_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The mutex_ptr field for the MX_MUTEX pointer passed was NULL.");
	}

	status = CloseHandle( *mutex_handle_ptr );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to close the Win32 handle for mutex %p. "
			"Win32 error code = %ld, error_message = '%s'",
			mutex, last_error_code, message_buffer );
	}

	mx_free( mutex_handle_ptr );

	mx_free( mutex );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_mutex_lock( MX_MUTEX *mutex )
{
	static const char fname[] = "mx_mutex_lock()";

	HANDLE *mutex_handle_ptr;
	BOOL status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	mutex_handle_ptr = mutex->mutex_ptr;

	if ( mutex_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = WaitForSingleObject( *mutex_handle_ptr, INFINITE );

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
				"for mutex %p. "
				"Win32 error code = %ld, error_message = '%s'",
				mutex, last_error_code, message_buffer );

			return MXE_OPERATING_SYSTEM_ERROR;
		}
		break;
	}

	return MXE_UNKNOWN_ERROR;
}

MX_EXPORT long
mx_mutex_unlock( MX_MUTEX *mutex )
{
	static const char fname[] = "mx_mutex_unlock()";

	HANDLE *mutex_handle_ptr;
	BOOL status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	mutex_handle_ptr = mutex->mutex_ptr;

	if ( mutex_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = ReleaseMutex( *mutex_handle_ptr );

	if ( status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[100];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error from WaitForSingleObject() "
			"for mutex %p. "
			"Win32 error code = %ld, error_message = '%s'",
			mutex, last_error_code, message_buffer );

		return MXE_OPERATING_SYSTEM_ERROR;
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_mutex_trylock( MX_MUTEX *mutex )
{
	static const char fname[] = "mx_mutex_trylock()";

	HANDLE *mutex_handle_ptr;
	BOOL status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	mutex_handle_ptr = mutex->mutex_ptr;

	if ( mutex_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = WaitForSingleObject( *mutex_handle_ptr, 0 );

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
				"for mutex %p. "
				"Win32 error code = %ld, error_message = '%s'",
				mutex, last_error_code, message_buffer );

			return MXE_OPERATING_SYSTEM_ERROR;
		}
		break;
	}

	return MXE_UNKNOWN_ERROR;
}

/************************ VxWorks ***********************/

#elif defined(OS_VXWORKS)

#include "semLib.h"
#include "intLib.h"

MX_EXPORT mx_status_type
mx_mutex_create( MX_MUTEX **mutex )
{
	static const char fname[] = "mx_mutex_create()";

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mutex == (MX_MUTEX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	/* Allocate the data structures we need. */

	*mutex = (MX_MUTEX *) malloc( sizeof(MX_MUTEX) );

	if ( *mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_MUTEX structure." );
	}

	/* Create the mutex. */

	(*mutex)->mutex_ptr = semMCreate(SEM_Q_FIFO);

	if ( (*mutex)->mutex_ptr == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Insufficient memory exists to initialize the mutex." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mutex_destroy( MX_MUTEX *mutex )
{
	static const char fname[] = "mx_mutex_destroy()";

	SEM_ID semaphore_id;
	STATUS status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	semaphore_id = mutex->mutex_ptr;

	if ( semaphore_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The mutex_ptr field for the MX_MUTEX pointer passed was NULL.");
	}

	status = semDelete( semaphore_id );

	if ( status == ERROR ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The mutex passed to semDelete() was invalid." );
	}

	mx_free( mutex );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_mutex_lock( MX_MUTEX *mutex )
{
	SEM_ID semaphore_id;
	int status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = mutex->mutex_ptr;

	if ( semaphore_id == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = semTake( semaphore_id, WAIT_FOREVER );

	if ( status != OK ) {
		switch( errno ) {
		case S_objLib_OBJ_TIMEOUT:
			return MXE_TIMED_OUT;
			break;
		case S_intLib_NOT_ISR_CALLABLE:
			return MXE_NOT_VALID_FOR_CURRENT_STATE;
			break;
		case S_objLib_OBJ_ID_ERROR:
		case S_objLib_OBJ_UNAVAILABLE:
		default:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		}
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_mutex_unlock( MX_MUTEX *mutex )
{
	SEM_ID semaphore_id;
	int status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = mutex->mutex_ptr;

	if ( semaphore_id == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = semGive( semaphore_id );

	if ( status != OK ) {
		switch( errno ) {
		case S_intLib_NOT_ISR_CALLABLE:
			return MXE_NOT_VALID_FOR_CURRENT_STATE;
			break;
		case S_objLib_OBJ_ID_ERROR:
		case S_semLib_INVALID_OPERATION:
		default:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		}
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_mutex_trylock( MX_MUTEX *mutex )
{
	SEM_ID semaphore_id;
	int status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = mutex->mutex_ptr;

	if ( semaphore_id == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = semTake( semaphore_id, NO_WAIT );

	if ( status != OK ) {
		switch( errno ) {
		case S_objLib_OBJ_TIMEOUT:
			return MXE_NOT_AVAILABLE;
			break;
		case S_intLib_NOT_ISR_CALLABLE:
			return MXE_NOT_VALID_FOR_CURRENT_STATE;
			break;
		case S_objLib_OBJ_ID_ERROR:
		case S_objLib_OBJ_UNAVAILABLE:
		default:
			return MXE_ILLEGAL_ARGUMENT;
			break;
		}
	}

	return MXE_SUCCESS;
}

/************************ RTEMS ***********************/

#elif defined(OS_RTEMS)

#include "rtems.h"

MX_EXPORT mx_status_type
mx_mutex_create( MX_MUTEX **mutex )
{
	static const char fname[] = "mx_mutex_create()";

	rtems_id *semaphore_id;
	rtems_unsigned32 initial_state;
	rtems_status_code rtems_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mutex == (MX_MUTEX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	/* Allocate the data structures we need. */

	*mutex = (MX_MUTEX *) malloc( sizeof(MX_MUTEX) );

	if ( *mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_MUTEX structure." );
	}

	semaphore_id = (rtems_id *) malloc( sizeof(rtems_id) );

	if ( semaphore_id == (rtems_id *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an rtems_id structure." );
	}

	/* Create the mutex using an RTEMS binary semaphore.  Note that
	 * this must _NOT_ be an RTEMS simple binary semaphore, since the
	 * simple binary semaphores block on nested access.  MX requires
	 * mutexes to be recursive.
	 */

	initial_state = 1;

	rtems_status = rtems_semaphore_create(rtems_build_name('M','X','M','U'),
	    initial_state,
	    RTEMS_FIFO | RTEMS_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY \
	    	| RTEMS_NO_PRIORITY_CEILING | RTEMS_LOCAL, 0, semaphore_id );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_INVALID_NAME:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The RTEMS name specified for rtems_semaphore_create() "
			"was invalid." );
		break;
	case RTEMS_INVALID_ADDRESS:
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The rtems_id pointer passed to "
			"rtems_semaphore_create() was NULL." );
		break;
	case RTEMS_TOO_MANY:
		return mx_error( MXE_TRY_AGAIN, fname,
			"Too many RTEMS semaphores or global objects "
			"are in use." );
		break;
	case RTEMS_NOT_DEFINED:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"One or more of the attributes specified for the call "
			"to rtems_semaphore_create() were invalid." );
		break;
	case RTEMS_INVALID_NUMBER:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Invalid starting count %lu specified for the call "
			"to rtems_semaphore_create().",
				(unsigned long) initial_state );
		break;
	case RTEMS_MP_NOT_CONFIGURED:
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
			"Multiprocessing has not been configured for this "
			"copy of RTEMS." );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"An unexpected status code %lu was returned by the "
			"call to rtems_semaphore_create().",
				(unsigned long) rtems_status );
		break;
	}

	(*mutex)->mutex_ptr = semaphore_id;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mutex_destroy( MX_MUTEX *mutex )
{
	static const char fname[] = "mx_mutex_destroy()";

	rtems_id *semaphore_id;
	rtems_status_code rtems_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	semaphore_id = mutex->mutex_ptr;

	if ( semaphore_id == (rtems_id *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The mutex_ptr field for the MX_MUTEX pointer passed was NULL.");
	}

	rtems_status = rtems_semaphore_delete( *semaphore_id );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_INVALID_ID:
		return mx_error( MXE_NOT_FOUND, fname,
			"The semaphore id %#lx supplied to "
			"rtems_semaphore_delete() was not found.",
			(unsigned long) (*semaphore_id) );
		break;
	case RTEMS_ILLEGAL_ON_REMOTE_OBJECT:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Cannot delete remote semaphore id %#lx.",
			(unsigned long) (*semaphore_id) );
		break;
	case RTEMS_RESOURCE_IN_USE:
		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"Semaphore id %#lx is currently in use.",
			(unsigned long) (*semaphore_id) );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"An unexpected status code %lu was returned by the "
			"call to rtems_semaphore_delete().",
				(unsigned long) rtems_status );
		break;
	}

	mx_free( mutex->mutex_ptr );
	mx_free( mutex );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_mutex_lock( MX_MUTEX *mutex )
{
	rtems_id *semaphore_id;
	rtems_status_code rtems_status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = mutex->mutex_ptr;

	if ( semaphore_id == (rtems_id *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	rtems_status = rtems_semaphore_obtain( *semaphore_id,
					RTEMS_WAIT, RTEMS_NO_TIMEOUT );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_UNSATISFIED:
	case RTEMS_TIMEOUT:
		return MXE_OPERATING_SYSTEM_ERROR;
		break;
	case RTEMS_OBJECT_WAS_DELETED:
		return MXE_BAD_HANDLE;
		break;
	case RTEMS_INVALID_ID:
		return MXE_NOT_FOUND;
		break;
	default:
		return MXE_UNKNOWN_ERROR;
		break;
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_mutex_unlock( MX_MUTEX *mutex )
{
	rtems_id *semaphore_id;
	rtems_status_code rtems_status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = mutex->mutex_ptr;

	if ( semaphore_id == (rtems_id *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	rtems_status = rtems_semaphore_release( *semaphore_id );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_INVALID_ID:
		return MXE_NOT_FOUND;
		break;
	case RTEMS_NOT_OWNER_OF_RESOURCE:
		return MXE_PERMISSION_DENIED;
		break;
	default:
		return MXE_UNKNOWN_ERROR;
		break;
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_mutex_trylock( MX_MUTEX *mutex )
{
	rtems_id *semaphore_id;
	rtems_status_code rtems_status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	semaphore_id = mutex->mutex_ptr;

	if ( semaphore_id == (rtems_id *) NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	rtems_status = rtems_semaphore_obtain( *semaphore_id,
					RTEMS_NO_WAIT, 0 );

	switch( rtems_status ) {
	case RTEMS_SUCCESSFUL:
		break;
	case RTEMS_UNSATISFIED:
		return MXE_NOT_AVAILABLE;
		break;
	case RTEMS_TIMEOUT:
		return MXE_OPERATING_SYSTEM_ERROR;
		break;
	case RTEMS_OBJECT_WAS_DELETED:
		return MXE_BAD_HANDLE;
		break;
	case RTEMS_INVALID_ID:
		return MXE_NOT_FOUND;
		break;
	default:
		return MXE_UNKNOWN_ERROR;
		break;
	}

	return MXE_SUCCESS;
}

/************************ Posix Pthreads ***********************/

#elif defined(_POSIX_THREADS) || defined(OS_HPUX) || defined(__OpenBSD__)

#include <pthread.h>

#if defined(OS_LINUX)

/* FIXME: For some future version of Linux, explicitly providing these
 *        definitions will not be necessary.  It might not be necessary
 *        _now_ if you define USE_UNIX98, but I am not currently certain
 *        of all the consequences and side effects of doing that.
 */

extern int pthread_mutexattr_settype( pthread_mutexattr_t *, int );

#define PTHREAD_MUTEX_RECURSIVE		PTHREAD_MUTEX_RECURSIVE_NP

#endif /* OS_LINUX */

MX_EXPORT mx_status_type
mx_mutex_create( MX_MUTEX **mutex )
{
	static const char fname[] = "mx_mutex_create()";

	pthread_mutex_t *p_mutex_ptr;
	pthread_mutexattr_t p_mutex_attr;
	int status, init_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mutex == (MX_MUTEX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	/* Allocate the data structures we need. */

	*mutex = (MX_MUTEX *) malloc( sizeof(MX_MUTEX) );

	if ( *mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_MUTEX structure." );
	}

	p_mutex_ptr = (pthread_mutex_t *) malloc( sizeof(pthread_mutex_t) );

	if ( p_mutex_ptr == (pthread_mutex_t *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a pthread_mutex_t structure." );
	}

	(*mutex)->mutex_ptr = p_mutex_ptr;

	/* Configure a pthread_mutexattr_t object to request
	 * recursive mutexes.
	 */

	status = pthread_mutexattr_init( &p_mutex_attr );

	switch( status ) {
	case 0:
		break;
	case ENOMEM:
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a pthread_mutexattr_t object." );
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected Pthreads error code %d returned "
			"by pthread_mutexattr_init().  Error message = '%s'.",
				status, strerror( status ) );
	}
	
#if defined( OS_ECOS )
	/* FIXME: Need to include our own implementation of recursive
	 * mutexes for platforms that do not support them.
	 */
#else

	status = pthread_mutexattr_settype( &p_mutex_attr,
					PTHREAD_MUTEX_RECURSIVE );

	switch( status ) {
	case 0:
		break;
	case EINVAL:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"One or more of the arguments to pthread_mutexattr_settype() "
		"were invalid." );
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected Pthreads error code %d returned "
			"by pthread_mutexattr_settype().  "
			"Error message = '%s'.",
				status, strerror( status ) );
	}

#endif /* ! OS_ECOS */

	/* Create the mutex. */

	init_status = pthread_mutex_init( p_mutex_ptr, &p_mutex_attr );

	/* We want the mutex attributes object to be destroyed, no matter
	 * what value was returned by pthread_mutex_init().
	 */
	
	status = pthread_mutexattr_destroy( &p_mutex_attr );

	switch( status ) {
	case 0:
		break;
	case EINVAL:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "The argument given to pthread_mutexattr_destroy() was invalid." );
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"Unexpected Pthreads error code %d returned "
			"by pthread_mutexattr_destroy().  "
			"Error message = '%s'.",
				status, strerror( status ) );
	}
	
	/* Now check the status from pthread_mutex_init(). */

	switch( init_status ) {
	case 0:
		return MX_SUCCESSFUL_RESULT;
	case EAGAIN:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The system lacked the necessary resources (other than memory) "
		"to initialize the mutex." );
	case ENOMEM:
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Insufficient memory exists to initialize the mutex." );
	case EPERM:
		return mx_error( MXE_PERMISSION_DENIED, fname,
	   "The caller does not have the privilege to perform the operation.");
	case EBUSY:
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The pthreads library has detected an attempt to re-initialize "
		"an already initialized, but not yet destroyed, mutex." );
	case EINVAL:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The attribute passed to pthread_mutex_init() was invalid." );
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected Pthreads error code %d returned.  "
		"Error message = '%s'.",
			init_status, strerror( init_status ) );
	}
}

MX_EXPORT mx_status_type
mx_mutex_destroy( MX_MUTEX *mutex )
{
	static const char fname[] = "mx_mutex_destroy()";

	pthread_mutex_t *p_mutex_ptr;
	int status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	p_mutex_ptr = mutex->mutex_ptr;

	if ( p_mutex_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The mutex_ptr field for the MX_MUTEX pointer passed was NULL.");
	}

	status = pthread_mutex_destroy( p_mutex_ptr );

	switch( status ) {
	case 0:
		break;
	case EBUSY:
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The pthreads library has detected an attempt to destroy "
		"a mutex that is locked or referenced by another thread." );
	case EINVAL:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The mutex passed to pthread_mutex_destroy() was invalid." );
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected Pthreads error code %d returned.  "
		"Error message = '%s'.",
			status, strerror( status ) );
	}

	mx_free( p_mutex_ptr );

	mx_free( mutex );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_mutex_lock( MX_MUTEX *mutex )
{
	pthread_mutex_t *p_mutex_ptr;
	int status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	p_mutex_ptr = mutex->mutex_ptr;

	if ( p_mutex_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = pthread_mutex_lock( p_mutex_ptr );

	switch( status ) {
	case 0:
		return MXE_SUCCESS;
	case EINVAL:
		return MXE_ILLEGAL_ARGUMENT;
	case EAGAIN:
		return MXE_WOULD_EXCEED_LIMIT;
	case EDEADLK:
		return MXE_NOT_VALID_FOR_CURRENT_STATE;
	default:
		return MXE_UNKNOWN_ERROR;
	}
}

MX_EXPORT long
mx_mutex_unlock( MX_MUTEX *mutex )
{
	pthread_mutex_t *p_mutex_ptr;
	int status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	p_mutex_ptr = mutex->mutex_ptr;

	if ( p_mutex_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = pthread_mutex_unlock( p_mutex_ptr );

	switch( status ) {
	case 0:
		return MXE_SUCCESS;
	case EINVAL:
		return MXE_ILLEGAL_ARGUMENT;
	case EAGAIN:
		return MXE_WOULD_EXCEED_LIMIT;
	case EPERM:
		return MXE_PERMISSION_DENIED;
	default:
		return MXE_UNKNOWN_ERROR;
	}
}

MX_EXPORT long
mx_mutex_trylock( MX_MUTEX *mutex )
{
	pthread_mutex_t *p_mutex_ptr;
	int status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	p_mutex_ptr = mutex->mutex_ptr;

	if ( p_mutex_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = pthread_mutex_trylock( p_mutex_ptr );

	switch( status ) {
	case 0:
		return MXE_SUCCESS;
	case EINVAL:
		return MXE_ILLEGAL_ARGUMENT;
	case EBUSY:
		return MXE_NOT_AVAILABLE;
	case EAGAIN:
		return MXE_WOULD_EXCEED_LIMIT;
	default:
		return MXE_UNKNOWN_ERROR;
	}
}

/********** Use the following stubs when threads are not supported **********/

#elif defined(OS_DJGPP)

MX_EXPORT mx_status_type
mx_mutex_create( MX_MUTEX **mutex )
{
	static const char fname[] = "mx_mutex_create()";

	if ( mutex == (MX_MUTEX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	*mutex = (MX_MUTEX *) malloc( sizeof(MX_MUTEX) );

	if ( *mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_MUTEX structure." );
	}

	(*mutex)->mutex_ptr = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_mutex_destroy( MX_MUTEX *mutex )
{
	if ( mutex != (MX_MUTEX *) NULL ) {
		mx_free(mutex);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mx_mutex_lock( MX_MUTEX *mutex )
{
	return MXE_SUCCESS;
}

MX_EXPORT long
mx_mutex_unlock( MX_MUTEX *mutex )
{
	return MXE_SUCCESS;
}

MX_EXPORT long
mx_mutex_trylock( MX_MUTEX *mutex )
{
	return MXE_SUCCESS;
}

/*-------------------------------------------------------------------------*/

#else

#error MX mutex functions have not yet been defined for this platform.

#endif

