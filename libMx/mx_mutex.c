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

#if defined(_POSIX_THREADS)

#include <pthread.h>

MX_EXPORT mx_status_type
mx_mutex_create( MX_MUTEX **mutex, int type )
{
	static const char fname[] = "mx_mutex_create()";

	pthread_mutex_t p_mutex;
	int status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mutex == (MX_MUTEX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	status = pthread_mutex_init( &p_mutex, NULL );

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
		return MX_SUCCESSFUL_RESULT;
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
		return MXE_OPERATING_SYSTEM_ERROR;
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
		return MXE_OPERATING_SYSTEM_ERROR;
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
		return MXE_OPERATING_SYSTEM_ERROR;
		break;
	}
}

/*-------------------------------------------------------------------------*/

#else

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

#endif

