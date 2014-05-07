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

#include "mx_util.h"
#include "mx_osdef.h"
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
		mx_free( posix_cv_struct );
		mx_free( *cv );
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to allocate a pthread_cond_t struct." );
	}

	pthread_status = pthread_cond_init( pthread_cv, NULL );

	if ( pthread_status != 0 ) {
		mx_free( pthread_cv );
		mx_free( posix_cv_struct );
		mx_free( *cv );
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to initialize the condition variable at %p failed."
		"  Error code = %d, error message = '%s'",
		pthread_cv, pthread_status, strerror(pthread_status) );
	}

	posix_cv_struct->pthread_cv = pthread_cv;

	(*cv)->cv_private = posix_cv_struct;

	return MX_SUCCESSFUL_RESULT;
}

#else

#error MX condition variables are not yet implemented on this platform.

#endif
