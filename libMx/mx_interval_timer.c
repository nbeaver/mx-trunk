/*
 * Name:    mx_interval_timer.c
 *
 * Purpose: Functions for using MX interval timers.  The interval timers
 *          are modeled after the Posix timer_create(), timer_settime(), etc.
 *          routines.  However, the Posix timers are not available on all
 *          supported MX platforms, so we have to provide a wrapper.
 *
 * Author:  William Lavender
 *
 * WARNING: This code has not been tested and is not really finished.
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define USE_EPICS_TIMERS	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_interval_timer.h"

/*************************** EPICS timers **************************/

#if USE_EPICS_TIMERS

#include "epicsTimer.h"
#include "epicsThread.h"

typedef struct {
	epicsTimerQueueId timer_queue;
	epicsTimerId timer;
} MX_EPICS_ITIMER_PRIVATE;

static void
mx_epics_timer_handler( void *arg )
{
	static const char fname[] = "mx_epics_timer_handler()";

	MX_INTERVAL_TIMER *itimer;
	MX_EPICS_ITIMER_PRIVATE *epics_itimer_private;

	if ( arg == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The EPICS timer handler was invoked with a NULL argument.  "
		"This should not happen." );

		return;
	}

	itimer = (MX_INTERVAL_TIMER *) arg;

	epics_itimer_private = (MX_EPICS_ITIMER_PRIVATE *) itimer->private;

	if ( epics_itimer_private == (MX_EPICS_ITIMER_PRIVATE *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_ITIMER_PRIVATE pointer for the EPICS timer "
		"handler is NULL.  This should not happen." );

		return;
	}

	/* If this is a periodic timer, schedule the next invocation. */

	if ( itimer->timer_type == MXIT_PERIODIC_TIMER ) {
		epicsTimerStartDelay( epics_itimer_private->timer,
					itimer->timer_period );
	}

	/* Invoke the callback function. */

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}

	return;
}

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_EPICS_ITIMER_PRIVATE *epics_itimer_private;

	MX_DEBUG(-2,("%s invoked for EPICS timers.", fname));

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	epics_itimer_private = (MX_EPICS_ITIMER_PRIVATE *)
				malloc( sizeof(MX_EPICS_ITIMER_PRIVATE) );

	if ( epics_itimer_private == (MX_EPICS_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_EPICS_ITIMER_PRIVATE structure." );
	}

	itimer->private = epics_itimer_private;

	epics_itimer_private->timer_queue =
		epicsTimerQueueAllocate( 1, epicsThreadPriorityScanHigh );

	if ( epics_itimer_private->timer_queue == 0 ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to create an EPICS timer queue failed." );
	}

	epics_itimer_private->timer =
		epicsTimerQueueCreateTimer( epics_itimer_private->timer_queue,
					mx_epics_timer_handler, itimer );

	if ( epics_itimer_private->timer == 0 ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to create an EPICS timer failed." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_EPICS_ITIMER_PRIVATE *epics_itimer_private;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	epics_itimer_private = (MX_EPICS_ITIMER_PRIVATE *) itimer->private;

	if ( epics_itimer_private != (MX_EPICS_ITIMER_PRIVATE *) NULL ) {
		epicsTimerQueueDestroyTimer( epics_itimer_private->timer_queue,
						epics_itimer_private->timer );

		epicsTimerQueueRelease( epics_itimer_private->timer_queue );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_get_time( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration )
{
	/* FIXME: I do not know how to do this with EPICS. */

	*seconds_till_expiration = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_set_time( MX_INTERVAL_TIMER *itimer,
				double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_set_time()";

	MX_EPICS_ITIMER_PRIVATE *epics_itimer_private;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	epics_itimer_private = (MX_EPICS_ITIMER_PRIVATE *) itimer->private;

	if ( epics_itimer_private == (MX_EPICS_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_ITIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	}

	epicsTimerStartDelay( epics_itimer_private->timer,
					itimer->timer_period );

	return MX_SUCCESSFUL_RESULT;
}

/******************** Microsoft Windows multimedia timers ********************/

#elif defined( OS_WIN32 )

#include <windows.h>

typedef struct {
	UINT timer_id;
} MX_WIN32_MMTIMER_PRIVATE;

static void CALLBACK
mx_interval_timer_thread_handler( UINT timer_id,
				UINT reserved_msg,
				DWORD user_data,
				DWORD reserved1,
				DWORD reserved2 )
{
	MX_INTERVAL_TIMER *itimer;

	itimer = (MX_INTERVAL_TIMER *) user_data;

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_WIN32_MMTIMER_PRIVATE *win32_mmtimer_private;

	MX_DEBUG(-2,("%s invoked for Win32 multimedia timers.", fname));

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *)
				malloc( sizeof(MX_WIN32_MMTIMER_PRIVATE) );

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_WIN32_MMTIMER_PRIVATE structure." );
	}

	itimer->private = win32_mmtimer_private;

	win32_mmtimer_private->timer_id = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_WIN32_MMTIMER_PRIVATE *win32_mmtimer_private;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER_POINTER passed was NULL." );
	}

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *) itimer->private;

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL )
		return MX_SUCCESSFUL_RESULT;

	mx_free( win32_mmtimer_private );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_get_time( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration )
{
	/* FIXME: I do not know how to do this with Win32 multimedia timers. */

	*seconds_till_expiration = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_set_time( MX_INTERVAL_TIMER *itimer,
				double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_set_time()";

	MX_WIN32_MMTIMER_PRIVATE *win32_mmtimer_private;
	UINT event_delay_ms, timer_flags;
	DWORD last_error_code;
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

/*************************** POSIX realtime timers **************************/

#elif ( ( _XOPEN_REALTIME != (-1) ) \
		&& _POSIX_TIMERS && _POSIX_REALTIME_SIGNALS )

#include <time.h>
#include <signal.h>
#include <errno.h>

#define MX_SIGEV_TYPE	SIGEV_SIGNAL  /* May be SIGEV_SIGNAL or SIGEV_THREAD */

#define MX_ITIMER_SIGNAL_NUMBER		(SIGRTMIN + 2)

typedef struct {
	timer_t timer_id;
	struct sigevent evp;
} MX_POSIX_ITIMER_PRIVATE;

#if ( MX_SIGEV_TYPE == SIGEV_SIGNAL )

static void
mx_interval_timer_signal_handler( int signum, siginfo_t *siginfo, void *cruft )
{
	MX_INTERVAL_TIMER *itimer;

	itimer = (MX_INTERVAL_TIMER *) siginfo->si_value.sival_ptr;

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}
}

#elif ( MX_SIGEV_TYPE == SIGEV_THREAD )

static void
mx_interval_timer_thread_handler( union sigval sigev_value )
{
	MX_INTERVAL_TIMER *itimer;

	itimer = (MX_INTERVAL_TIMER *) sigev_value.sival_ptr;

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}
}

#endif /* MX_SIGEV_TYPE */

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_create()";

#if ( MX_SIGEV_TYPE == SIGEV_SIGNAL )
	struct sigaction sa;
#endif
	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	int status, saved_errno;

	MX_DEBUG(-2,("%s invoked for POSIX realtime timers.", fname));

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	posix_itimer_private = (MX_POSIX_ITIMER_PRIVATE *)
				malloc( sizeof(MX_POSIX_ITIMER_PRIVATE) );

	if ( posix_itimer_private == (MX_POSIX_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_POSIX_ITIMER_PRIVATE structure." );
	}

	itimer->private = posix_itimer_private;

	/* Create the Posix timer. */

	posix_itimer_private->evp.sigev_value.sival_ptr = itimer;

# if ( MX_SIGEV_TYPE == SIGEV_SIGNAL )
	/* If necessary, set up the signal handler. */

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = mx_interval_timer_signal_handler;

	status = sigaction( MX_ITIMER_SIGNAL_NUMBER, &sa, NULL );

	posix_itimer_private->evp.sigev_notify = SIGEV_SIGNAL;
	posix_itimer_private->evp.sigev_signo = MX_ITIMER_SIGNAL_NUMBER;

#elif ( MX_SIGEV_TYPE == SIGEV_THREAD )

	posix_itimer_private->evp.sigev_notify = SIGEV_THREAD;
	posix_itimer_private->evp.sigev_notify_function =
					mx_interval_timer_thread_handler;
	posix_itimer_private->evp.sigev_notify_attributes = NULL;

#endif /* MX_SIGEV_TYPE */

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
		"The system did not recognize the clock CLOCK_REALTIME.  "
		"This should not be able to happen." );
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
	int status, saved_errno;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	posix_itimer_private = (MX_POSIX_ITIMER_PRIVATE *) itimer->private;

	if ( posix_itimer_private != (MX_POSIX_ITIMER_PRIVATE *) NULL ) {
		status = timer_delete( posix_itimer_private->timer_id );

		if ( status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error deleting a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
		}

		mx_free( posix_itimer_private );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_get_time( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration )
{
	static const char fname[] = "mx_interval_timer_get_time()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	struct itimerspec value;
	int status, saved_errno;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	if ( seconds_till_expiration == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The seconds_till_expiration passed was NULL." );
	}

	posix_itimer_private = (MX_POSIX_ITIMER_PRIVATE *) itimer->private;

	if ( posix_itimer_private == (MX_POSIX_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POSIX_ITIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
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

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_set_time( MX_INTERVAL_TIMER *itimer,
				double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_set_time()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	struct itimerspec itimer_value;
	int status, saved_errno;
	unsigned long timer_ulong_seconds, timer_ulong_nsec;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	posix_itimer_private = (MX_POSIX_ITIMER_PRIVATE *) itimer->private;

	if ( posix_itimer_private == (MX_POSIX_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POSIX_ITIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	}

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

/************************ BSD style setitimer() timers ***********************/

#elif defined( OS_UNIX )

#include <sys/time.h>

typedef struct {
} MX_SETITIMER_PRIVATE;

static void
mx_interval_timer_signal_handler( int signum, siginfo_t *siginfo, void *cruft )
{
	MX_INTERVAL_TIMER *itimer;

	itimer = (MX_INTERVAL_TIMER *) siginfo->si_value.sival_ptr;

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_create()";

	struct sigaction sa;
	MX_SETITIMER_PRIVATE *setitimer_private;
	int status, saved_errno;

	MX_DEBUG(-2,("%s invoked for BSD setitimer timers.", fname));

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	setitimer_private = (MX_SETITIMER_PRIVATE *)
				malloc( sizeof(MX_SETITIMER_PRIVATE) );

	if ( setitimer_private == (MX_SETITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_SETITIMER_PRIVATE structure." );
	}

	itimer->private = setitimer_private;

	/* Set up the signal handler. */

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = mx_interval_timer_signal_handler;

	status = sigaction( SIGALRM, &sa, NULL );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_SETITIMER_PRIVATE *setitimer_private;
	int status, saved_errno;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	setitimer_private = (MX_SETITIMER_PRIVATE *) itimer->private;

	if ( setitimer_private != (MX_SETITIMER_PRIVATE *) NULL ) {
		mx_free( setitimer_private );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_get_time( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration )
{
	static const char fname[] = "mx_interval_timer_get_time()";

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

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_set_time( MX_INTERVAL_TIMER *itimer,
				double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_set_time()";

	MX_SETITIMER_PRIVATE *setitimer_private;
	struct itimerval itimer_value;
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
	} else {
		itimer_value.it_interval.tv_sec  = timer_ulong_seconds;
		itimer_value.it_interval.tv_usec = timer_ulong_usec;
	}

	/* Set the timer period. */

	status = setitimer( ITIMER_REAL, &itimer_value, NULL );

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

#else

#if 0
#error MX interval timer functions have not yet been defined for this platform.
#endif

#endif
