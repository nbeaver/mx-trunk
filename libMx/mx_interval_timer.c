/*
 * Name:    mx_interval_timer.c
 *
 * Purpose: Functions for using MX interval timers.  Interval timers are
 *          used to arrange for a function to be invoked at periodic
 *          intervals.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_unistd.h"
#include "mx_util.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_interval_timer.h"

/******************** Microsoft Windows multimedia timers ********************/

#if defined(OS_WIN32)

#include <windows.h>

typedef struct {
	UINT timer_id;
	TIMECAPS timecaps;
	UINT timer_resolution;
} MX_WIN32_MMTIMER_PRIVATE;

static void CALLBACK
mx_interval_timer_thread_handler( UINT timer_id,
				UINT reserved_msg,
				DWORD user_data,
				DWORD reserved1,
				DWORD reserved2 )
{
	static const char fname[] = "mx_interval_timer_thread_handler()";

	MX_INTERVAL_TIMER *itimer;

	itimer = (MX_INTERVAL_TIMER *) user_data;

	MX_DEBUG(-2,("%s: itimer = %p", fname, itimer));
	MX_DEBUG(-2,("%s: itimer->callback_function = %p",
			fname, itimer->callback_function));

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		(void) mx_interval_timer_stop( itimer, NULL );
	}

	MX_DEBUG(-2,("%s complete.", fname));

	return;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				void *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_WIN32_MMTIMER_PRIVATE *win32_mmtimer_private;
	MMRESULT result;

	MX_DEBUG(-2,("%s invoked for Win32 multimedia timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));

	if ( itimer == (MX_INTERVAL_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	switch( timer_type ) {
	case MXIT_ONE_SHOT_TIMER:
	case MXIT_PERIODIC_TIMER:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Interval timer type %d is unsupported.", timer_type );
		break;
	}

	*itimer = (MX_INTERVAL_TIMER *) malloc( sizeof(MX_INTERVAL_TIMER) );

	if ( (*itimer) == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for an MX_INTERVAL_TIMER structure.");
	}

	MX_DEBUG(-2,("%s: *itimer = %p", fname, *itimer));

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *)
				malloc( sizeof(MX_WIN32_MMTIMER_PRIVATE) );

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_WIN32_MMTIMER_PRIVATE structure." );
	}

	MX_DEBUG(-2,("%s: win32_mmtimer_private = %p",
			fname, win32_mmtimer_private));

	(*itimer)->timer_type = timer_type;
	(*itimer)->timer_period = -1.0;
	(*itimer)->num_overruns = 0;
	(*itimer)->callback_function = callback_function;
	(*itimer)->callback_args = callback_args;
	(*itimer)->private = win32_mmtimer_private;

	win32_mmtimer_private->timer_id = 0;

	result = timeGetDevCaps( &(win32_mmtimer_private->timecaps),
				sizeof( win32_mmtimer_private->timecaps ) );

	switch( result ) {
	case TIMERR_NOERROR:
		break;
	case TIMERR_STRUCT:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"An attempt to call timeGetDevCaps() for Win32 multimedia "
		"timers failed with a TIMERR_STRUCT error code.");
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected status %lu returned by timeGetDevCaps()",
			(unsigned long) result );
		break;
	}

	win32_mmtimer_private->timer_resolution =
		min( max(win32_mmtimer_private->timecaps.wPeriodMin, 0),
			win32_mmtimer_private->timecaps.wPeriodMax );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_WIN32_MMTIMER_PRIVATE *win32_mmtimer_private;
	double seconds_left;
	mx_status_type mx_status;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER_POINTER passed was NULL." );
	}

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *) itimer->private;

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL )
		return MX_SUCCESSFUL_RESULT;

	if ( win32_mmtimer_private->timer_id != 0 ) {
		mx_status = mx_interval_timer_stop( itimer, &seconds_left );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_free( win32_mmtimer_private );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, int *busy )
{
	static const char fname[] = "mx_interval_timer_is_busy()";

	MX_WIN32_MMTIMER_PRIVATE *win32_mmtimer_private;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	if ( busy == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The busy pointer passed was NULL." );
	}

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *) itimer->private;

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WIN32_MMTIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	};

	if ( win32_mmtimer_private->timer_id == 0 ) {
		*busy = FALSE;
	} else {
		*busy = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_start( MX_INTERVAL_TIMER *itimer,
				double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_start()";

	MX_WIN32_MMTIMER_PRIVATE *win32_mmtimer_private;
	UINT event_delay_ms, timer_flags;
	DWORD last_error_code;
	MMRESULT result;
	TCHAR message_buffer[100];

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER_POINTER passed was NULL." );
	}

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *) itimer->private;

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WIN32_MMTIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	};

	if( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		timer_flags = TIME_ONESHOT;
	} else {
		timer_flags = TIME_PERIODIC;
	}

	/* Convert the timer period to milliseconds. */

	event_delay_ms = (UINT) mx_round( 1000.0 * timer_period_in_seconds );

	/* Select the timer resolution. */

	result = timeBeginPeriod( win32_mmtimer_private->timer_resolution );

	switch( result ) {
	case TIMERR_NOERROR:
		break;
	case TIMERR_NOCANDO:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested multimedia timer resolution %u is outside "
		"the allowed range of %u to %u.",
			win32_mmtimer_private->timer_resolution,
			win32_mmtimer_private->timecaps.wPeriodMin,
			win32_mmtimer_private->timecaps.wPeriodMax );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected status %lu returned by timeBeginPeriod()",
			(unsigned long) result );
		break;
	}

	/* Setup the timer callback.
	 * 
	 * FIXME: The second argument of timeSetEvent(), which is the timer
	 *        resolution, is currently set to 0, which requests the highest
	 *        possible resolution.  We should check to see if this has too
	 *        much overhead.
	 */

	win32_mmtimer_private->timer_id = timeSetEvent( event_delay_ms, 0,
					mx_interval_timer_thread_handler,
					(DWORD) itimer,
					timer_flags );

	if ( win32_mmtimer_private->timer_id == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to start a multimedia timer failed.  "
		"Win32 error code = %ld, error message = '%s'",
				last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_stop( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration )
{
	static const char fname[] = "mx_interval_timer_stop()";

	
	MX_WIN32_MMTIMER_PRIVATE *win32_mmtimer_private;
	MMRESULT result;
	UINT timer_id;
	mx_status_type mx_status;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER_POINTER passed was NULL." );
	}

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *) itimer->private;

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WIN32_MMTIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	};

	if ( seconds_till_expiration != NULL ) {
		mx_status = mx_interval_timer_read( itimer,
						seconds_till_expiration );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	timer_id = win32_mmtimer_private->timer_id;

	win32_mmtimer_private->timer_id = 0;

	result = timeKillEvent( timer_id );

	switch( result ) {
	case TIMERR_NOERROR:
		break;
	case MMSYSERR_INVALPARAM:
		return mx_error( MXE_NOT_FOUND, fname,
	"The timer id %u specified for timeKillEvent() does not exist.",
			timer_id );
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected status %lu returned by timeGetDevCaps()",
			(unsigned long) result );
		break;
	}

	/* Reset the time resolution. */

	result = timeEndPeriod( win32_mmtimer_private->timer_resolution );

	switch( result ) {
	case TIMERR_NOERROR:
		break;
	case TIMERR_NOCANDO:
#if 1
		mx_warning( "%s: TIMERR_NOCANDO returned for timeEndPeriod().",
			fname );
#else
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested multimedia timer resolution %lu is outside "
		"the allowed range of %lu to %lu.",
			win32_mmtimer_private->timer_resolution,
			win32_mmtimer_private->timecaps.wPeriodMin,
			win32_mmtimer_private->timecaps.wPeriodMax );
#endif
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected status %lu returned by timeBeginPeriod()",
			(unsigned long) result );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_read( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration )
{
	/* FIXME: I do not know how to do this with Win32 multimedia timers. */

	*seconds_till_expiration = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

/*************************** POSIX realtime timers **************************/

#elif defined(_POSIX_TIMERS)

#include <time.h>
#include <signal.h>
#include <errno.h>

/* Select the notification method for the realtime timers.  The possible
 * values are SIGEV_THREAD or SIGEV_SIGNAL.  SIGEV_SIGNAL has much tighter
 * constraints than SIGEV_THREAD on the functions that can be invoked by
 * the event handler, since SIGEV_SIGNAL only allows functions that are
 * signal safe in the event handler.  You should always select SIGEV_THREAD
 * unless it is not available on the operating system that you are using.
 */

#define MX_SIGEV_TYPE	SIGEV_THREAD

typedef struct {
	timer_t timer_id;
	struct sigevent evp;

#if ( MX_SIGEV_TYPE == SIGEV_SIGNAL )
	int signal_number;
#endif

} MX_POSIX_ITIMER_PRIVATE;

/*--------------------------------------------------------------------------*/

static mx_status_type
mx_interval_timer_get_pointers( MX_INTERVAL_TIMER *itimer,
			MX_POSIX_ITIMER_PRIVATE **itimer_private,
			const char *calling_fname )
{
	static const char fname[] = "mx_interval_timer_get_pointers()";

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed by '%s' is NULL.",
			calling_fname );
	}
	if ( itimer_private == (MX_POSIX_ITIMER_PRIVATE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POSIX_ITIMER_PRIVATE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*itimer_private = (MX_POSIX_ITIMER_PRIVATE *) itimer->private;

	if ( (*itimer_private) == (MX_POSIX_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POSIX_ITIMER_PRIVATE pointer for itimer %p is NULL.",
			itimer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

#if ( MX_SIGEV_TYPE == SIGEV_THREAD )

static void
mx_interval_timer_thread_handler( union sigval sigev_value )
{
	static const char fname[] = "mx_interval_timer_thread_handler()";

	MX_INTERVAL_TIMER *itimer;
	MX_THREAD *thread;
	mx_status_type mx_status;

	itimer = (MX_INTERVAL_TIMER *) sigev_value.sival_ptr;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );

		return;
	}

	MX_DEBUG(-2,("%s: itimer = %p", fname, itimer));
	MX_DEBUG(-2,("%s: itimer->callback_function = %p",
			fname, itimer->callback_function));

	/* Setup an MX_THREAD structure for this thread. */

	mx_status = mx_thread_build_data_structures( &thread );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	/* Invoke the callback function. */

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}

	/* If this is supposed to be a one-shot timer, then
	 * make sure that the timer is turned off.
	 */

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		(void) mx_interval_timer_stop( itimer, NULL );
	}

	/* Get rid of the MX_THREAD structure for this thread. */

	mx_status = mx_thread_free_data_structures( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	/* End the thread by returning to the caller. */

	MX_DEBUG(-2,("%s complete.", fname));

	return;
}

static mx_status_type
mx_interval_timer_create_event_handler( MX_INTERVAL_TIMER *itimer,
				MX_POSIX_ITIMER_PRIVATE *posix_itimer_private )
{
	posix_itimer_private->evp.sigev_value.sival_ptr = itimer;

	posix_itimer_private->evp.sigev_notify = SIGEV_THREAD;
	posix_itimer_private->evp.sigev_notify_function =
					mx_interval_timer_thread_handler;
	posix_itimer_private->evp.sigev_notify_attributes = NULL;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_interval_timer_destroy_event_handler( MX_INTERVAL_TIMER *itimer,
				MX_POSIX_ITIMER_PRIVATE *posix_itimer_private )
{
	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#elif ( MX_SIGEV_TYPE == SIGEV_SIGNAL )

#if !defined(_POSIX_REALTIME_SIGNALS)
#error SIGEV_SIGNAL can only be used if Posix realtime signals are available.  If realtime signals are not available, you must use the setitimer-based timer mechanism.
#endif

#define MX_NUM_TIMER_SIGNALS	( SIGRTMAX - SIGRTMIN + 1 )

static MX_INTERVAL_TIMER **mx_interval_timer_signal_array;
static int mx_interval_timer_num_signals;

static MX_MUTEX *mx_interval_timer_signal_mutex = NULL;

static mx_status_type
mx_interval_timer_initialize_signal_array( void )
{
	static const char fname[] =
			"mx_interval_timer_initialize_signal_array()";

	mx_status_type mx_status;

	mx_interval_timer_num_signals = SIGRTMAX - SIGRTMIN + 1;

	MX_DEBUG(-2,("%s: mx_interval_timer_num_signals = %d",
			fname, mx_interval_timer_num_signals));

	mx_interval_timer_signal_array = (MX_INTERVAL_TIMER **)
	    calloc(mx_interval_timer_num_signals, sizeof(MX_INTERVAL_TIMER *));

	if ( mx_interval_timer_signal_array == (MX_INTERVAL_TIMER **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
  "Unable to allocate memory for a %d element mx_interval_timer_signal_array.",
			mx_interval_timer_num_signals );
	}

	mx_status = mx_mutex_create( &mx_interval_timer_signal_mutex, 0 );

	return mx_status;
}

static mx_status_type
mx_interval_timer_allocate_signal_number( MX_INTERVAL_TIMER *itimer,
				MX_POSIX_ITIMER_PRIVATE *posix_itimer_private )
{
	static const char fname[] =
			"mx_interval_timer_allocate_signal_number()";

	int i, signal_number;
	long status;

	status = mx_mutex_lock( mx_interval_timer_signal_mutex );

	if ( status != MXE_SUCCESS ) {
		return mx_error( status, fname,
			"Unable to lock mx_interval_timer_signal_mutex." );
	}

	for ( i = 0; i < mx_interval_timer_num_signals; i++ ) {
		if ( mx_interval_timer_signal_array[i] == NULL ) {
			break;			/* Exit the for() loop. */
		}
	}

	if ( i >= mx_interval_timer_num_signals ) {
		status = mx_mutex_unlock( mx_interval_timer_signal_mutex );

		if ( status != MXE_SUCCESS ) {
			(void) mx_error( status, fname,
			"Unable to unlock mx_interval_timer_signal_mutex." );
		}

		return mx_error( MXE_NOT_AVAILABLE, fname,
		"All %d of the Posix realtime signals are already in use.",
			mx_interval_timer_num_signals );
	}

	signal_number = i + SIGRTMIN;

	posix_itimer_private->signal_number = signal_number;

	mx_interval_timer_signal_array[i] = itimer;

	status = mx_mutex_unlock( mx_interval_timer_signal_mutex );

	if ( status != MXE_SUCCESS ) {
		return mx_error( status, fname,
		"Unable to unlock mx_interval_timer_signal_mutex." );
	}

	MX_DEBUG(-2,("%s: Allocated signal number %d", fname, signal_number));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_interval_timer_free_signal_number( MX_INTERVAL_TIMER *itimer,
				MX_POSIX_ITIMER_PRIVATE *posix_itimer_private )
{
	static const char fname[] = "mx_interval_timer_free_signal_number()";

	int i, signal_number, was_in_use;
	long status;

	signal_number = posix_itimer_private->signal_number;

	i = signal_number - SIGRTMIN;

	if ( (i < 0) || (i >= mx_interval_timer_num_signals) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The signal number %d in use by interval timer %p is outside "
		"the allowed range of %d to %d.", signal_number, itimer,
						SIGRTMIN, SIGRTMAX );
	}

	status = mx_mutex_lock( mx_interval_timer_signal_mutex );

	if ( status != MXE_SUCCESS ) {
		return mx_error( status, fname,
			"Unable to lock mx_interval_timer_signal_mutex." );
	}

	if ( mx_interval_timer_signal_array[i] == NULL ) {
		was_in_use = FALSE;
	} else {
		was_in_use = TRUE;
	}

	mx_interval_timer_signal_array[i] = NULL;

	status = mx_mutex_unlock( mx_interval_timer_signal_mutex );

	if ( status != MXE_SUCCESS ) {
		return mx_error( status, fname,
		"Unable to unlock mx_interval_timer_signal_mutex." );
	}

	if ( was_in_use == FALSE ) {
		mx_warning("%s: Freeing realtime signal number %d "
		"although it was not in use.", fname, signal_number );
	}

	MX_DEBUG(-2,("%s: Freed signal number %d.", fname, signal_number));

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static void
mx_interval_timer_signal_handler( int signum, siginfo_t *siginfo, void *cruft )
{
	static const char fname[] = "mx_interval_timer_signal_handler()";

	MX_INTERVAL_TIMER *itimer;

	MX_DEBUG(-2,("%s invoked for signal %d", fname, signum));

	itimer = (MX_INTERVAL_TIMER *) siginfo->si_value.sival_ptr;

	MX_DEBUG(-2,("%s: itimer = %p", fname, itimer));
	MX_DEBUG(-2,("%s: itimer->callback_function = %p",
			fname, itimer->callback_function));

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		(void) mx_interval_timer_stop( itimer, NULL );
	}

	MX_DEBUG(-2,("%s complete.", fname));

	return;
}

static mx_status_type
mx_interval_timer_create_event_handler( MX_INTERVAL_TIMER *itimer,
				MX_POSIX_ITIMER_PRIVATE *posix_itimer_private )
{
	static const char fname[] = "mx_interval_timer_create_event_handler()";

	struct sigaction sa;
	int status, saved_errno;
	mx_status_type mx_status;

	if ( mx_interval_timer_signal_mutex == (MX_MUTEX *) NULL ) {
		mx_status = mx_interval_timer_initialize_signal_array();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_interval_timer_allocate_signal_number( itimer,
							posix_itimer_private );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	posix_itimer_private->evp.sigev_value.sival_ptr = itimer;

	posix_itimer_private->evp.sigev_notify = SIGEV_SIGNAL;
	posix_itimer_private->evp.sigev_signo =
					posix_itimer_private->signal_number;

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = mx_interval_timer_signal_handler;

	status = sigaction( posix_itimer_private->signal_number, &sa, NULL );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to setup signal handler with sigaction().  "
			"Errno = %d, error message = '%s'.",
				saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_interval_timer_destroy_event_handler( MX_INTERVAL_TIMER *itimer,
				MX_POSIX_ITIMER_PRIVATE *posix_itimer_private )
{
	mx_status_type mx_status;

	mx_status = mx_interval_timer_free_signal_number( itimer,
							posix_itimer_private );
	return mx_status;
}

#else
#error MX_SIGEV_TYPE must be either SIGEV_THREAD or SIGEV_SIGNAL.  SIGEV_THREAD is by far the better choice.
#endif /* MX_SIGEV_TYPE */

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				void *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	int status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for POSIX realtime timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));

	if ( itimer == (MX_INTERVAL_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	switch( timer_type ) {
	case MXIT_ONE_SHOT_TIMER:
	case MXIT_PERIODIC_TIMER:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Interval timer type %d is unsupported.", timer_type );
		break;
	}

	*itimer = (MX_INTERVAL_TIMER *) malloc( sizeof(MX_INTERVAL_TIMER) );

	if ( (*itimer) == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for an MX_INTERVAL_TIMER structure.");
	}

	MX_DEBUG(-2,("%s: *itimer = %p", fname, *itimer));

	posix_itimer_private = (MX_POSIX_ITIMER_PRIVATE *)
				malloc( sizeof(MX_POSIX_ITIMER_PRIVATE) );

	if ( posix_itimer_private == (MX_POSIX_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_POSIX_ITIMER_PRIVATE structure." );
	}

	(*itimer)->timer_type = timer_type;
	(*itimer)->timer_period = -1.0;
	(*itimer)->num_overruns = 0;
	(*itimer)->callback_function = callback_function;
	(*itimer)->callback_args = callback_args;
	(*itimer)->private = posix_itimer_private;

	/* Configure the event handler for the timer. */

	mx_status = mx_interval_timer_create_event_handler( *itimer,
							posix_itimer_private );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create the Posix timer. */

	status = timer_create( CLOCK_REALTIME, 
			&(posix_itimer_private->evp),
			&(posix_itimer_private->timer_id) );

	if ( status == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		saved_errno = errno;

		switch( saved_errno ) {
		case EAGAIN:
			return mx_error( MXE_TRY_AGAIN, fname,
	"The system is unable to create a Posix timer at this time." );
			break;
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"One or more of the arguments to timer_create() were invalid.");
			break;
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error creating a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
			break;
		}
	}
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	double seconds_left;
	int status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&posix_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_stop( itimer, &seconds_left );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = timer_delete( posix_itimer_private->timer_id );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error deleting a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	mx_status = mx_interval_timer_destroy_event_handler( itimer,
							posix_itimer_private );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_free( posix_itimer_private );

	mx_free( itimer );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, int *busy )
{
	static const char fname[] = "mx_interval_timer_is_busy()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	struct itimerspec value;
	int status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&posix_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The busy pointer passed was NULL." );
	}

	status = timer_gettime( posix_itimer_private->timer_id, &value );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The specified timer does not exist." );
			break;
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error reading from a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	if ( (value.it_value.tv_sec != 0)
	  || (value.it_value.tv_nsec != 0 ) )
	{
		*busy = TRUE;
	} else {
		*busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_start( MX_INTERVAL_TIMER *itimer,
				double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_start()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	struct itimerspec itimer_value;
	int status, saved_errno;
	unsigned long timer_ulong_seconds, timer_ulong_nsec;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&posix_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Convert the timer period to a struct timespec. */

	timer_ulong_seconds = (unsigned long) timer_period_in_seconds;

	timer_ulong_nsec = (unsigned long)
				( 1.0e9 * ( timer_period_in_seconds
					- (double) timer_ulong_seconds ) );

	/* The way we use the timer values depends on whether or not
	 * we configure the timer as a one-shot timer or a periodic
	 * timer.
	 */

	itimer_value.it_value.tv_sec  = timer_ulong_seconds;
	itimer_value.it_value.tv_nsec = timer_ulong_nsec;

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		itimer_value.it_interval.tv_sec  = 0;
		itimer_value.it_interval.tv_nsec = 0;
	} else {
		itimer_value.it_interval.tv_sec  = timer_ulong_seconds;
		itimer_value.it_interval.tv_nsec = timer_ulong_nsec;
	}

	/* Set the timer period. */

	status = timer_settime( posix_itimer_private->timer_id,
					0, &itimer_value, NULL );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The specified timer does not exist." );
			break;
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error writing to a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_stop( MX_INTERVAL_TIMER *itimer, double *seconds_left )
{
	static const char fname[] = "mx_interval_timer_stop()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	struct itimerspec itimer_value;
	int status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&posix_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the time left on the timer, if requested, but do not abort
	 * if the attempt to read the timer value fails.
	 */

	if ( seconds_left != NULL ) {
		(void) mx_interval_timer_read( itimer, seconds_left );
	}

	/* The way we use the timer values depends on whether or not
	 * we configure the timer as a one-shot timer or a periodic
	 * timer.
	 */

	/* If we set all the timer values to 0, the timer is disabled. */

	itimer_value.it_value.tv_sec  = 0;
	itimer_value.it_value.tv_nsec = 0;
	itimer_value.it_interval.tv_sec  = 0;
	itimer_value.it_interval.tv_nsec = 0;

	status = timer_settime( posix_itimer_private->timer_id,
					0, &itimer_value, NULL );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The specified timer does not exist." );
			break;
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error writing to a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_read( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration )
{
	static const char fname[] = "mx_interval_timer_read()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	struct itimerspec value;
	int status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&posix_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( seconds_till_expiration == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The seconds_till_expiration passed was NULL." );
	}

	status = timer_gettime( posix_itimer_private->timer_id, &value );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The specified timer does not exist." );
			break;
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error reading from a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	*seconds_till_expiration = (double) value.it_value.tv_sec;

	*seconds_till_expiration += 1.0e-9 * (double) value.it_value.tv_nsec;

	return MX_SUCCESSFUL_RESULT;
}

/************************ BSD style setitimer() timers ***********************/

#elif defined( OS_DJGPP ) || defined( OS_UNIX )

/* WARNING: BSD setitimer() timers should only be used as a last resort,
 *          since they have some significant limitations in their
 *          functionality.
 *
 *          1.  There can only be one setitimer() based timer in a given
 *              process.
 *          2.  They are usually based on SIGALRM signals.  SIGALRM tends
 *              to be widely used for a variety of purposes in Unix, so it
 *              can be hard to ensure that two different parts of an MX
 *              based program are not fighting for control of SIGALRM.
 *
 *              Note that DJGPP is an exception in that it has a dedicated
 *              signal (SIGTIMR) for private use by setitimer().
 *
 *          If you have any other way of implementing interval timers on a
 *          particular platform, you should use that other method instead.
 */

#include <signal.h>
#include <errno.h>
#include <sys/time.h>

typedef struct {
	int dummy;
} MX_SETITIMER_PRIVATE;

static MX_INTERVAL_TIMER *mx_setitimer_interval_timer = NULL;
static MX_MUTEX *mx_setitimer_mutex = NULL;

#define RELEASE_SETITIMER_INTERVAL_TIMER \
		do {							\
			(void) mx_mutex_lock( mx_setitimer_mutex );	\
			mx_setitimer_interval_timer = NULL;		\
			(void) mx_mutex_unlock( mx_setitimer_mutex );	\
		} while(0)

static void
mx_interval_timer_signal_handler( int signum )
{
	static const char fname[] = "mx_interval_timer_signal_handler()";

	MX_INTERVAL_TIMER *itimer;

	(void) mx_mutex_lock( mx_setitimer_mutex );

	itimer = mx_setitimer_interval_timer;

	(void) mx_mutex_unlock( mx_setitimer_mutex );

	MX_DEBUG(-2,("%s: itimer = %p", fname, itimer));

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		mx_warning(
		    "%s was invoked although no interval timer was started.",
			fname );

		return;
	}

	MX_DEBUG(-2,("%s: itimer->callback_function = %p",
		fname, itimer->callback_function));

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		RELEASE_SETITIMER_INTERVAL_TIMER;
	}

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}

	MX_DEBUG(-2,("%s complete.", fname));
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				void *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_SETITIMER_PRIVATE *setitimer_private;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for BSD setitimer timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));

	if ( mx_setitimer_mutex == NULL ) {
		mx_status = mx_mutex_create( &mx_setitimer_mutex, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( itimer == (MX_INTERVAL_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	switch( timer_type ) {
	case MXIT_ONE_SHOT_TIMER:
	case MXIT_PERIODIC_TIMER:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Interval timer type %d is unsupported.", timer_type );
		break;
	}

	*itimer = (MX_INTERVAL_TIMER *) malloc( sizeof(MX_INTERVAL_TIMER) );

	if ( (*itimer) == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for an MX_INTERVAL_TIMER structure.");
	}

	MX_DEBUG(-2,("%s: *itimer = %p", fname, *itimer));

	setitimer_private = (MX_SETITIMER_PRIVATE *)
				malloc( sizeof(MX_SETITIMER_PRIVATE) );

	if ( setitimer_private == (MX_SETITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_SETITIMER_PRIVATE structure." );
	}

	MX_DEBUG(-2,("%s: setitimer_private = %p", fname, setitimer_private));

	(*itimer)->timer_type = timer_type;
	(*itimer)->timer_period = -1.0;
	(*itimer)->num_overruns = 0;
	(*itimer)->callback_function = callback_function;
	(*itimer)->callback_args = callback_args;
	(*itimer)->private = setitimer_private;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_SETITIMER_PRIVATE *setitimer_private;
	double seconds_left;
	mx_status_type mx_status;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	mx_status = mx_interval_timer_stop( itimer, &seconds_left );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	setitimer_private = (MX_SETITIMER_PRIVATE *) itimer->private;

	if ( setitimer_private != (MX_SETITIMER_PRIVATE *) NULL ) {
		mx_free( setitimer_private );
	}

	mx_free( itimer );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, int *busy )
{
	static const char fname[] = "mx_interval_timer_is_busy()";

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	if ( busy == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The busy pointer passed was NULL." );
	}

	(void) mx_mutex_lock( mx_setitimer_mutex );

	if ( mx_setitimer_interval_timer == itimer ) {
		*busy = TRUE;
	} else {
		*busy = FALSE;
	}

	(void) mx_mutex_unlock( mx_setitimer_mutex );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_start( MX_INTERVAL_TIMER *itimer,
				double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_start()";

	MX_SETITIMER_PRIVATE *setitimer_private;
	struct itimerval itimer_value;
	struct sigaction sa;
	int status, saved_errno;
	unsigned long timer_ulong_seconds, timer_ulong_usec;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	setitimer_private = (MX_SETITIMER_PRIVATE *) itimer->private;

	if ( setitimer_private == (MX_SETITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SETITIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	}

	/* Convert the timer period to a struct timespec. */

	timer_ulong_seconds = (unsigned long) timer_period_in_seconds;

	timer_ulong_usec = (unsigned long)
				( 1.0e6 * ( timer_period_in_seconds
					- (double) timer_ulong_seconds ) );

	/* The way we use the timer values depends on whether or not
	 * we configure the timer as a one-shot timer or a periodic
	 * timer.
	 */

	itimer_value.it_value.tv_sec  = timer_ulong_seconds;
	itimer_value.it_value.tv_usec = timer_ulong_usec;

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		itimer_value.it_interval.tv_sec  = 0;
		itimer_value.it_interval.tv_usec = 0;

		sa.sa_flags = SA_ONESHOT;
	} else {
		itimer_value.it_interval.tv_sec  = timer_ulong_seconds;
		itimer_value.it_interval.tv_usec = timer_ulong_usec;

		sa.sa_flags = 0;
	}

	/* Attempt to restart system calls when possible. */

	sa.sa_flags |= SA_RESTART;

	/* Set the function to be used by the signal handler. */

	sa.sa_handler = mx_interval_timer_signal_handler;

	/* Is an interval timer already running? */

	(void) mx_mutex_lock( mx_setitimer_mutex );

	if ( mx_setitimer_interval_timer != (MX_INTERVAL_TIMER *) NULL ) {
		(void) mx_mutex_unlock( mx_setitimer_mutex );

		return mx_error( MXE_NOT_AVAILABLE, fname,
		"There can only be one setitimer-based interval timer started "
		"at a time and one is already started." );
	}

	mx_setitimer_interval_timer = itimer;

	(void) mx_mutex_unlock( mx_setitimer_mutex );

	/* Set up the signal handler. */

	status = sigaction( SIGALRM, &sa, NULL );

	/* Set the timer period. */

	status = setitimer( ITIMER_REAL, &itimer_value, NULL );

	if ( status != 0 ) {
		saved_errno = errno;

		RELEASE_SETITIMER_INTERVAL_TIMER;

		switch( saved_errno ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The specified timer does not exist." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error writing to ITIMER_REAL.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_stop( MX_INTERVAL_TIMER *itimer, double *seconds_left )
{
	static const char fname[] = "mx_interval_timer_stop()";

	struct itimerval itimer_value;
	int status, saved_errno;

	/* Read the time left on the timer, if requested, but do not abort
	 * if the attempt to read the timer value fails.
	 */

	if ( seconds_left != NULL ) {
		(void) mx_interval_timer_read( itimer, seconds_left );
	}

	/* First, disable the signal handler. */

	signal( SIGALRM, SIG_IGN );

	/* Then, disable the timer by setting all its intervals to 0. */

	itimer_value.it_value.tv_sec  = 0;
	itimer_value.it_value.tv_usec = 0;

	itimer_value.it_interval.tv_sec = 0;
	itimer_value.it_interval.tv_usec = 0;

	status = setitimer( ITIMER_REAL, &itimer_value, NULL );

	/* Mark the timer as available. */

	RELEASE_SETITIMER_INTERVAL_TIMER;

	/* Check to see if there were any errors from the setitimer() call. */

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The specified timer does not exist." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error writing to ITIMER_REAL.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_read( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration )
{
	static const char fname[] = "mx_interval_timer_read()";

	MX_SETITIMER_PRIVATE *setitimer_private;
	struct itimerval value;
	int status, saved_errno;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	if ( seconds_till_expiration == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The seconds_till_expiration passed was NULL." );
	}

	setitimer_private = (MX_SETITIMER_PRIVATE *) itimer->private;

	if ( setitimer_private == (MX_SETITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SETITIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	}

	status = getitimer( ITIMER_REAL, &value );

	if ( status != 0 ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The specified timer does not exist." );
			break;
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error reading from timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	*seconds_till_expiration = (double) value.it_value.tv_sec;

	*seconds_till_expiration += 1.0e-6 * (double) value.it_value.tv_usec;

	return MX_SUCCESSFUL_RESULT;
}

#else

#error MX interval timer functions have not yet been defined for this platform.

#endif
