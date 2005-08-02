/*
 * Name:    mx_mutex.c
 *
 * Purpose: MX mutex functions.
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
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_mutex.h"

#if defined(OS_WIN32)

#include <windows.h>

MX_EXPORT mx_status_type
mx_mutex_create( MX_MUTEX **mutex, int type )
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

	status = CloseHandle( mutex_handle_ptr );

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
	HANDLE *mutex_handle_ptr;
	DWORD last_error_code;
	BOOL status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	mutex_handle_ptr = mutex->mutex_ptr;

	if ( mutex_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = WaitForSingleObject( mutex_handle_ptr, INFINITE );

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
		last_error_code = GetLastError();

		return MXE_OPERATING_SYSTEM_ERROR;
		break;
	}

	return MXE_UNKNOWN_ERROR;
}

MX_EXPORT long
mx_mutex_unlock( MX_MUTEX *mutex )
{
	HANDLE *mutex_handle_ptr;
	DWORD last_error_code;
	BOOL status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	mutex_handle_ptr = mutex->mutex_ptr;

	if ( mutex_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = ReleaseMutex( mutex_handle_ptr );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		return MXE_OPERATING_SYSTEM_ERROR;
	}

	return MXE_SUCCESS;
}

MX_EXPORT long
mx_mutex_trylock( MX_MUTEX *mutex )
{
	HANDLE *mutex_handle_ptr;
	DWORD last_error_code;
	BOOL status;

	if ( mutex == (MX_MUTEX *) NULL )
		return MXE_NULL_ARGUMENT;

	mutex_handle_ptr = mutex->mutex_ptr;

	if ( mutex_handle_ptr == NULL )
		return MXE_CORRUPT_DATA_STRUCTURE;

	status = WaitForSingleObject( mutex_handle_ptr, 0 );

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
		last_error_code = GetLastError();

		return MXE_OPERATING_SYSTEM_ERROR;
		break;
	}

	return MXE_UNKNOWN_ERROR;
}

#elif defined(_POSIX_THREADS)

#include <pthread.h>

MX_EXPORT mx_status_type
mx_mutex_create( MX_MUTEX **mutex, int type )
{
	static const char fname[] = "mx_mutex_create()";

	pthread_mutex_t *p_mutex_ptr;
	int status;

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

	p_mutex_ptr = (pthread_mutex_t *) malloc( sizeof(pthread_mutex_t) );

	if ( p_mutex_ptr == (pthread_mutex_t *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a pthread_mutex_t structure." );
	}

	(*mutex)->mutex_ptr = p_mutex_ptr;

	status = pthread_mutex_init( p_mutex_ptr, NULL );

	switch( status ) {
	case 0:
		return MX_SUCCESSFUL_RESULT;
		break;
	case EAGAIN:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The system lacked the necessary resources (other than memory) "
		"to initialize the mutex." );
		break;
	case ENOMEM:
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Insufficient memory exists to initialize the mutex." );
		break;
	case EPERM:
		return mx_error( MXE_PERMISSION_DENIED, fname,
	   "The caller does not have the privilege to perform the operation.");
		break;
	case EBUSY:
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The pthreads library has detected an attempt to re-initialize "
		"an already initialized, but not yet destroyed, mutex." );
		break;
	case EINVAL:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The attribute passed to pthread_mutex_init() was invalid." );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected Pthreads error code %d returned.  "
		"Error message = '%s'.",
			status, strerror( status ) );
		break;
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
		break;
	case EINVAL:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The mutex passed to pthread_mutex_destroy() was invalid." );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected Pthreads error code %d returned.  "
		"Error message = '%s'.",
			status, strerror( status ) );
		break;
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
		break;
	case EINVAL:
		return MXE_ILLEGAL_ARGUMENT;
		break;
	case EAGAIN:
		return MXE_WOULD_EXCEED_LIMIT;
		break;
	case EDEADLK:
		return MXE_NOT_VALID_FOR_CURRENT_STATE;
		break;
	default:
		return MXE_UNKNOWN_ERROR;
		break;
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
		break;
	case EINVAL:
		return MXE_ILLEGAL_ARGUMENT;
		break;
	case EAGAIN:
		return MXE_WOULD_EXCEED_LIMIT;
		break;
	case EPERM:
		return MXE_PERMISSION_DENIED;
		break;
	default:
		return MXE_UNKNOWN_ERROR;
		break;
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
		break;
	case EINVAL:
		return MXE_ILLEGAL_ARGUMENT;
		break;
	case EBUSY:
		return MXE_NOT_AVAILABLE;
		break;
	case EAGAIN:
		return MXE_WOULD_EXCEED_LIMIT;
		break;
	default:
		return MXE_UNKNOWN_ERROR;
		break;
	}
}

/*-------------------------------------------------------------------------*/

#elif 0

MX_EXPORT mx_status_type
mx_mutex_create( MX_MUTEX **mutex, int type )
{
	static const char fname[] = "mx_mutex_create()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"MX mutex functions are not supported for this operating system." );
}

MX_EXPORT mx_status_type
mx_mutex_destroy( MX_MUTEX *mutex )
{
	static const char fname[] = "mx_mutex_destroy()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"MX mutex functions are not supported for this operating system." );
}

MX_EXPORT long
mx_mutex_lock( MX_MUTEX *mutex )
{
	return MXE_UNSUPPORTED;
}

MX_EXPORT long
mx_mutex_unlock( MX_MUTEX *mutex )
{
	return MXE_UNSUPPORTED;
}

MX_EXPORT long
mx_mutex_trylock( MX_MUTEX *mutex )
{
	return MXE_UNSUPPORTED;
}

#else

#error MX mutex functions have not yet been defined for this platform.

#endif

