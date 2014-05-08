/*
 * Name:    mx_condition_variable.c
 *
 * Purpose: MX condition variable functions.
 *
 *          These functions are modeled on Posix condition variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_osdef.h"
#include "mx_hrt.h"
#include "mx_mutex.h"
#include "mx_condition_variable.h"

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

/************************ Posix pthreads ***********************/

#elif defined(OS_LINUX)

#include <pthread.h>

MX_EXPORT mx_status_type
mx_condition_variable_create( MX_CONDITION_VARIABLE **cv )
{
	static const char fname[] = "mx_condition_variable_create()";

	pthread_cond_t *pthread_cv = NULL;
	int pthread_status;

	if ( cv == (MX_CONDITION_VARIABLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	*cv = malloc( sizeof(MX_CONDITION_VARIABLE) );

	if ( (*cv) == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
  "Ran out of memory trying to allocate an MX_CONDITION_VARIABLE structure." );
	}

	pthread_cv = malloc( sizeof(pthread_cond_t) );

	if ( pthread_cv == (pthread_cond_t *) NULL ) {
		mx_free( *cv );
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to allocate a pthread_cond_t struct." );
	}

	pthread_status = pthread_cond_init( pthread_cv, NULL );

	if ( pthread_status != 0 ) {
		mx_free( pthread_cv );
		mx_free( *cv );
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to initialize the condition variable at %p failed."
		"  Error code = %d, error message = '%s'",
		pthread_cv, pthread_status, strerror(pthread_status) );
	}

	(*cv)->cv_private = pthread_cv;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_destroy( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_create()";

	pthread_cond_t *pthread_cv;
	int pthread_status;
	mx_status_type mx_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );

		mx_free( cv );
		return mx_status;
	}

	pthread_status = pthread_cond_destroy( pthread_cv );

	if ( pthread_status != 0 ) {
		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to destroy the Posix condition variable "
		"for MX condition variable %p failed.  "
		"Error code = %d, error message = '%s'",
			cv, pthread_status, strerror(pthread_status) );

		mx_free( pthread_cv );
		mx_free( cv );

		return mx_status;
	}

	mx_free( pthread_cv );
	mx_free( cv );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_wait( MX_CONDITION_VARIABLE *cv,
				MX_MUTEX *mutex )
{
	static const char fname[] = "mx_condition_variable_wait()";

	pthread_cond_t *pthread_cv;
	pthread_mutex_t *pthread_mutex;
	int pthread_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}
	if ( mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	pthread_mutex = mutex->mutex_ptr;

	if ( pthread_mutex == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix mutex pointer for MX mutex %p is NULL.", mutex );
	}

	pthread_status = pthread_cond_wait( pthread_cv, pthread_mutex );

	if ( pthread_status != 0 ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to wait for MX condition variable %p using "
		"MX mutex %p failed.  "
		"Posix condition variable = %p, Posix mutex = %p  "
		"Error code = %d, error message = '%s'",
			cv, mutex, pthread_cv, pthread_mutex,
			pthread_status, strerror(pthread_status) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_timed_wait( MX_CONDITION_VARIABLE *cv,
				MX_MUTEX *mutex,
				const struct timespec *ts )
{
	static const char fname[] = "mx_condition_variable_timed_wait()";

	pthread_cond_t *pthread_cv;
	pthread_mutex_t *pthread_mutex;
	int pthread_status;
	double timeout;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}
	if ( mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}
	if ( ts == (const struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The timespec pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	pthread_mutex = mutex->mutex_ptr;

	if ( pthread_mutex == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix mutex pointer for MX mutex %p is NULL.", mutex );
	}

	pthread_status = pthread_cond_timedwait( pthread_cv, pthread_mutex, ts);

	switch( pthread_status ) {
	case 0:
		break;
	case ETIMEDOUT:
		timeout = mx_convert_high_resolution_time_to_seconds( *ts );

		return mx_error( MXE_TIMED_OUT, fname,
		"The wait for MX condition variable %p using MX mutex %p "
		"timed out after %f seconds.", cv, mutex, timeout );
		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to wait for MX condition variable %p using "
		"MX mutex %p failed.  "
		"Posix condition variable = %p, Posix mutex = %p  "
		"Error code = %d, error message = '%s'",
			cv, mutex, pthread_cv, pthread_mutex,
			pthread_status, strerror(pthread_status) );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_signal( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_signal()";

	pthread_cond_t *pthread_cv;
	int pthread_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	pthread_status = pthread_cond_signal( pthread_cv );

	if ( pthread_status != 0 ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to signal MX condition variable %p failed.  "
		"Error code = %d, error message = '%s'",
			cv, pthread_status, strerror(pthread_status) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_broadcast( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_broadcast()";

	pthread_cond_t *pthread_cv;
	int pthread_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	pthread_status = pthread_cond_broadcast( pthread_cv );

	if ( pthread_status != 0 ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to broadcast to MX condition variable %p failed.  "
		"Error code = %d, error message = '%s'",
			cv, pthread_status, strerror(pthread_status) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

#error MX condition variables are not yet implemented on this platform.

#endif

