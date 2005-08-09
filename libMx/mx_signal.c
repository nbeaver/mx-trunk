/*
 * Name:    mx_signal.c
 *
 * Purpose: Functions for managing the use of signals.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "mx_osdef.h"
#include "mx_unistd.h"
#include "mx_util.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_signal.h"

#if defined( _POSIX_REALTIME_SIGNALS )

#define MX_NUM_SIGNALS	SIGRTMAX

static int *mx_signal_array = NULL;
static int mx_num_signals;

static MX_MUTEX *mx_signal_mutex = NULL;

MX_EXPORT mx_status_type
mx_signal_initialize( void )
{
	static const char fname[] = "mx_signal_initialize()";

	mx_status_type mx_status;

	mx_num_signals = MX_NUM_SIGNALS;

	MX_DEBUG(-2,("%s: mx_num_signals = %d", fname, mx_num_signals));

	mx_signal_array = (int *) calloc(mx_num_signals, sizeof(int));

	if ( mx_signal_array == (int *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
  "Unable to allocate memory for a %d element mx_signal_array.",
			mx_num_signals );
	}

	mx_status = mx_mutex_create( &mx_signal_mutex, 0 );

	return mx_status;
}

MX_EXPORT int
mx_signals_are_initialized( void )
{
	if ( mx_signal_mutex != NULL ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#define MX_SIGNAL_MUTEX_UNLOCK \
		do {							\
			int mt_status;					\
									\
			mt_status = mx_mutex_unlock( mx_signal_mutex );	\
									\
			if ( status != MXE_SUCCESS ) {			\
				(void) mx_error( status, fname,		\
				"Unable to unlock mx_signal_mutex." );	\
			}						\
		} while(0)

MX_EXPORT mx_status_type
mx_signal_allocate( int requested_signal_number,
			int *allocated_signal_number )
{
	static const char fname[] = "mx_signal_allocate()";

	int i;
	long status;
	mx_status_type mx_status;

	if ( mx_signals_are_initialized() == FALSE ) {
		mx_status = mx_signal_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	status = mx_mutex_lock( mx_signal_mutex );

	if ( status != MXE_SUCCESS ) {
		return mx_error( status, fname,
			"Unable to lock mx_signal_mutex." );
	}

	if ( requested_signal_number != MXF_ANY_REALTIME_SIGNAL ) {

		i = requested_signal_number - 1;

		if ( mx_signal_array[i] != FALSE ) {
			MX_SIGNAL_MUTEX_UNLOCK;
			return mx_error( MXE_NOT_AVAILABLE, fname,
				"Signal %d is already in use.",
				requested_signal_number );
		}

		*allocated_signal_number = requested_signal_number;
	} else {
		/* Search for an available realtime signal. */

		for ( i = (SIGRTMIN - 1); i < SIGRTMAX; i++ ) {
			if ( mx_signal_array[i] == FALSE ) {
				break;		/* Exit the for() loop. */
			}
		}

		if ( i >= SIGRTMAX ) {
			MX_SIGNAL_MUTEX_UNLOCK;
			return mx_error( MXE_NOT_AVAILABLE, fname,
		"All of the Posix realtime signals are already in use." );
		}

		*allocated_signal_number = i + 1;
	}

	mx_signal_array[i] = TRUE;

	MX_SIGNAL_MUTEX_UNLOCK;

	MX_DEBUG(-2,("%s: Allocated signal number %d",
			fname, *allocated_signal_number));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_signal_free( int signal_number )
{
	static const char fname[] = "mx_signal_free()";

	int i, was_in_use;
	long status;

	if ( mx_signals_are_initialized() == FALSE ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Attempted to free signal %d when signals had not yet "
		"been initialized.", signal_number );
	}

	i = signal_number - 1;

	if ( (i < 0) || (i >= mx_num_signals) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested signal number %d is outside "
		"the allowed range of %d to %d.", signal_number,
						1, MX_NUM_SIGNALS );
	}

	status = mx_mutex_lock( mx_signal_mutex );

	if ( status != MXE_SUCCESS ) {
		return mx_error( status, fname,
			"Unable to lock mx_signal_mutex." );
	}

	if ( mx_signal_array[i] == FALSE ) {
		was_in_use = FALSE;
	} else {
		was_in_use = TRUE;
	}

	mx_signal_array[i] = FALSE;

	MX_SIGNAL_MUTEX_UNLOCK;

	if ( was_in_use == FALSE ) {
		mx_warning("%s: Freeing realtime signal number %d "
		"although it was not in use.", fname, signal_number );
	}

	MX_DEBUG(-2,("%s: Freed signal number %d.", fname, signal_number));

	return MX_SUCCESSFUL_RESULT;
}

#endif /* _POSIX_REALTIME_SIGNALS */

