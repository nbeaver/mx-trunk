/*
 * Name:    mx_interval_timer.c
 *
 * Purpose: Functions for using MX interval timers.  Interval timers are
 *          used to arrange for a function to be invoked at periodic
 *          intervals.
 *
 * Author:  William Lavender
 *
 * WARNING FOR SOLARIS:
 *          For versions of Solaris up to and including Solaris 8, you _must_
 *          define the preprocessor symbol _POSIX_PER_PROCESS_TIMER_SOURCE
 *          when compiling to get correct behavior from MX interval timers.
 *          The man page for timer_create() in Solaris 2.6 includes the
 *          paragraph:
 *
 *              The timer may be created per-LWP or per-process.  Expiration
 *              signals for a per-LWP timer will be sent to the creating LWP.
 *              Expiration signals for a per-process timer will be sent to
 *              the process.  A per-LWP timer will automatically be deleted
 *              when the creating LWP ends.  By default, timers are created
 *              per-LWP.  If the symbol _POSIX_PER_PROCESS_TIMER_SOURCE is
 *              defined or the symbol _POSIX_C_SOURCE is defined to have a
 *              value greater than 199500L before the inclusion of <time.h>,
 *              timers will be created per-process.
 *
 *          The Solaris 2.5 man page has a similar statement, except for the
 *          fact that it mentions _POSIX_PER_PROCESS_TIMERS rather than
 *          _POSIX_PER_PROCESS_TIMER_SOURCE.  Apparently the name was changed
 *          in going to Solaris 2.6 for no obvious reason.
 *
 *          For Solaris 7 and 8, the man page for timer_create no longer
 *          mentions _POSIX_PER_PROCESS_TIMER_SOURCE.  However, you _still_
 *          need to define it to get correct behavior.  This is confirmed
 *          by a posting from Sun employee Roger Faulkner to the Usenet
 *          news group comp.programming.threads on November 24, 2002 where
 *          he says:
 *
 *              By default, in Solaris 8 and previous releases,
 *              timers have per-LWP semantics (th1s violates POSIX semantics).
 *              This has been fixed in Solaris 9.
 *
 *              To get correct per-process semantics, you need to compile \
 *              like this:
 *
 *                 cc -mt -D_POSIX_C_SOURCE=199506 sigtest.c -lrt -lpthread
 *              or:
 *                 cc -mt -D_POSIX_PER_PROCESS_TIMER_SOURCE sigtest.c -lrt \
 *                 -lpthread
 *
 *          For Solaris, MX uses timer_create() based timers with SIGEV_SIGNAL
 *          notification since Solaris does not support SIGEV_THREAD 
 *          notification (boo!).  MX then creates a new thread which then
 *          waits in sigwaitinfo() for the timing signals to arrive.  If
 *          you have the default behavior of thread-specific timers, then
 *          only the thread that created the timer will see the signal when
 *          the timer expires.  However, since MX is waiting in a different
 *          thread in sigwaitinfo() for the signal, the signal never gets
 *          delivered unless you recompile to use per-process timers.
 *
 *          I have not yet had a chance to verify the claim that the problem
 *          is fixed in Solaris 9 and above.  (WML)
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2004-2007, 2010-2014, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_INTERVAL_TIMER_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_interval_timer.h"

#if defined( _POSIX_TIMERS )

#  if defined(OS_TRU64)
#     define HAVE_POSIX_TIMERS	TRUE

#  elif defined(OS_HURD)
#     define HAVE_POSIX_TIMERS	FALSE

#  elif defined(OS_MINIX)
#     define HAVE_POSIX_TIMERS	FALSE

#  elif ( _POSIX_TIMERS < 0 )
#     define HAVE_POSIX_TIMERS	FALSE
#  else
#     define HAVE_POSIX_TIMERS	TRUE
#  endif
#else
#  define HAVE_POSIX_TIMERS	FALSE
#endif

/******************** Microsoft Windows multimedia timers ********************/

#if defined(OS_WIN32)

#include <windows.h>

typedef struct {
	UINT timer_id;
	TIMECAPS timecaps;
	UINT timer_resolution;
} MX_WIN32_MMTIMER_PRIVATE;

/* The prototypes for timeSetEvent() were rather sloppy in
 * older Visual C++ versions, so we have to do the following
 * definition of DWORD_PTR here.
 */

#if ( defined(_MSC_VER ) && (_MSC_VER < 1300) )
#  define DWORD_PTR	DWORD
#endif

static void CALLBACK
mx_interval_timer_thread_handler( UINT timer_id,
				UINT reserved_msg,
				DWORD_PTR user_data,
				DWORD_PTR reserved1,
				DWORD_PTR reserved2 )
{
	static const char fname[] = "mx_interval_timer_thread_handler()";

	MX_INTERVAL_TIMER *itimer;
	MX_THREAD *thread;
	mx_status_type mx_status;

	itimer = (MX_INTERVAL_TIMER *) user_data;

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );

		return;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: itimer = %p", fname, itimer));
	MX_DEBUG(-2,("%s: itimer->callback_function = %p",
			fname, itimer->callback_function));
#endif

	/* Setup an MX_THREAD structure for this thread. */

	mx_status = mx_thread_build_data_structures( &thread );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	mx_status = mx_thread_save_thread_pointer( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	mx_status = mx_win32_thread_get_handle_and_id( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	/* Invoke the callback function. */

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}

	/* if this is supposed to be a one-shot timer, then
	 * make sure that the timer is turned off.
	 */

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		(void) mx_interval_timer_stop( itimer, NULL );
	}

	mx_status = mx_thread_free_data_structures( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	/* End the thread by returning to the caller. */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				MX_INTERVAL_TIMER_CALLBACK *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_WIN32_MMTIMER_PRIVATE *win32_mmtimer_private;
	MMRESULT result;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for Win32 multimedia timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));
#endif

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

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: *itimer = %p", fname, *itimer));
#endif

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *)
				malloc( sizeof(MX_WIN32_MMTIMER_PRIVATE) );

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_WIN32_MMTIMER_PRIVATE structure." );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: win32_mmtimer_private = %p",
			fname, win32_mmtimer_private));
#endif

	(*itimer)->timer_type = timer_type;
	(*itimer)->timer_period = -1.0;
	(*itimer)->num_overruns = 0;
	(*itimer)->callback_function = callback_function;
	(*itimer)->callback_args = callback_args;
	(*itimer)->private_ptr = win32_mmtimer_private;

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
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *)itimer->private_ptr;

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL )
		return MX_SUCCESSFUL_RESULT;

	if ( win32_mmtimer_private->timer_id != 0 ) {
		mx_status = mx_interval_timer_stop( itimer, &seconds_left );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_free( win32_mmtimer_private );

	mx_free( itimer );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, mx_bool_type *busy )
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

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *)itimer->private_ptr;

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WIN32_MMTIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	};

	if ( win32_mmtimer_private->timer_id == 0 ) {
		*busy = FALSE;

		itimer->timer_period = -1;
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
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *)itimer->private_ptr;

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WIN32_MMTIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	};

	itimer->timer_period = timer_period_in_seconds;

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
			(TIMECALLBACK *) mx_interval_timer_thread_handler,
			(DWORD_PTR) itimer,
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
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	win32_mmtimer_private = (MX_WIN32_MMTIMER_PRIVATE *)itimer->private_ptr;

	if ( win32_mmtimer_private == (MX_WIN32_MMTIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WIN32_MMTIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	};

	itimer->timer_period = -1;

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

/********************************** OpenVMS *********************************/

#elif defined(OS_VMS)

#include <lib$routines.h>
#include <starlet.h>
#include <ssdef.h>
#include <libdef.h>

typedef struct {
	uint32_t event_flag;
	uint32_t timer_period[2];
	uint32_t finish_time[2];
	int timer_is_busy;
	int restart_timer;
	MX_THREAD *event_flag_thread;
} MX_VMS_ITIMER_PRIVATE;

/*--------------------------------------------------------------------------*/

static mx_status_type
mx_interval_timer_get_pointers( MX_INTERVAL_TIMER *itimer,
			MX_VMS_ITIMER_PRIVATE **itimer_private,
			const char *calling_fname )
{
	static const char fname[] = "mx_interval_timer_get_pointers()";

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed by '%s' is NULL.",
			calling_fname );
	}
	if ( itimer_private == (MX_VMS_ITIMER_PRIVATE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VMS_ITIMER_PRIVATE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*itimer_private = (MX_VMS_ITIMER_PRIVATE *) itimer->private_ptr;

	if ( (*itimer_private) == (MX_VMS_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VMS_ITIMER_PRIVATE pointer for itimer %p is NULL.",
			itimer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mx_interval_timer_event_flag_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mx_interval_timer_event_flag_thread()";

	MX_INTERVAL_TIMER *itimer;
	MX_VMS_ITIMER_PRIVATE *vms_itimer_private;
	int vms_status;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for thread %p, args = %p.",
		fname, thread, args));
#endif

	itimer = (MX_INTERVAL_TIMER *) args;

	vms_itimer_private = (MX_VMS_ITIMER_PRIVATE *) itimer->private_ptr;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: itimer = %p, vms_itimer_private = %p",
		fname, itimer, vms_itimer_private));
#endif

	while (1) {
		/* Wait for our event flag to be set. */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Waiting for event flag %lu",
		fname, (unsigned long) vms_itimer_private->event_flag ));
#endif

		vms_status = sys$waitfr( vms_itimer_private->event_flag );

		switch( vms_status ) {
		case SS$_NORMAL:
			break;
		case SS$_ILLEFC:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"%d is not a legal event flag number.",
				vms_itimer_private->event_flag );
			break;
		case SS$_UNASEFC:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"This process is not associated with the cluster "
			"containing event flag %d.",
				vms_itimer_private->event_flag );
			break;
		default:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The attempt to wait for VMS event flag %d failed with "
			"a VMS error code of %d.", vms_status );
			break;
		}

		/* If we get here, the event flag has been set.
		 * Clear the event flag to prevent the callback
		 * from being invoked multiple times.
		 */

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: Event flag %lu for timer has been set.  "
			"Invoking sys$clref() to clear it.", fname,
			(unsigned long) vms_itimer_private->event_flag ));
#endif
		vms_status = sys$clref( vms_itimer_private->event_flag );

		switch( vms_status ) {
		case SS$_WASCLR:
		case SS$_WASSET:
			break;
		case SS$_ILLEFC:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"%d is not a legal event flag number.",
				vms_itimer_private->event_flag );
			break;
		case SS$_UNASEFC:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"This process is not associated with the cluster "
			"containing event flag %d.",
				vms_itimer_private->event_flag );
			break;
		default:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The attempt to wait for VMS event flag %d failed with "
			"a VMS error code of %d.", vms_status );
			break;
		}

		/* If we get here, the event flag has been set, which
		 * makes it time to invoke the callback function.
		 */

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: Invoking interval timer callback.", fname ));
#endif
		if ( itimer->callback_function != NULL ) {
			itimer->callback_function( itimer,
						itimer->callback_args );
		}

		/* See if someone has asked us to terminate ourself. */

		mx_status = mx_thread_check_for_stop_request( thread );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* sys$setimr() is intrinsically a one-shot timer function,
		 * so if this is supposed to be a periodic timer, we must
		 * manually restart the timer for the next period.
		 */

		if ( itimer->timer_type == MXIT_PERIODIC_TIMER ) {

#if MX_INTERVAL_TIMER_DEBUG
			MX_DEBUG(-2,("%s: Restarting timer for event flag %lu",
				fname, vms_itimer_private->event_flag ));
#endif

			vms_itimer_private->restart_timer = TRUE;

			mx_status = mx_interval_timer_start( itimer, 0.0 );
		} else {
			vms_itimer_private->restart_timer = FALSE;
			vms_itimer_private->timer_is_busy = FALSE;
		}
	}

	MX_DEBUG(-2,("%s: Should not get here.", fname));
	
	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

/* Store the value of 2^32 as a floating point number. */

static double vms_2_to_the_32nd_power = -1.0;

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				MX_INTERVAL_TIMER_CALLBACK *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_VMS_ITIMER_PRIVATE *vms_itimer_private;
	uint32_t event_flag_number;
	int vms_status;
	mx_status_type mx_status;

	if ( vms_2_to_the_32nd_power < 0.0 ) {
		vms_2_to_the_32nd_power = pow( 2.0, 32.0 );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for VMS timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));
#endif

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

	vms_itimer_private = (MX_VMS_ITIMER_PRIVATE *)
				malloc( sizeof(MX_VMS_ITIMER_PRIVATE) );

	if ( vms_itimer_private == (MX_VMS_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_VMS_ITIMER_PRIVATE structure." );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: *itimer = %p, vms_itimer_private = %p",
		fname, *itimer, vms_itimer_private));
#endif

	(*itimer)->timer_type = timer_type;
	(*itimer)->timer_period = -1.0;
	(*itimer)->num_overruns = 0;
	(*itimer)->callback_function = callback_function;
	(*itimer)->callback_args = callback_args;
	(*itimer)->private_ptr = vms_itimer_private;

	/* Allocate an event flag for this timer. */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Calling lib$get_ef", fname));
#endif

	vms_status = lib$get_ef( &event_flag_number );

	switch( vms_status ) {
	case SS$_NORMAL:
		break;
	case LIB$_INSEF:
		return mx_error( MXE_NOT_AVAILABLE, fname,
			"No more event flags are available for use by a new "
			"MX interval timer." );
		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to allocate an event flag for a new "
		"MX interval timer failed with a VMS error code of %d.",
			vms_status );
		break;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: event_flag_number = %lu",
		fname, event_flag_number));
#endif

	vms_itimer_private->event_flag = event_flag_number;
	vms_itimer_private->timer_is_busy = FALSE;
	vms_itimer_private->restart_timer = FALSE;

	/* Configure the event handler for the timer. */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,
  ("%s: About to invoke mx_thread_create() for args = %p", fname, *itimer ));
#endif
				
	mx_status = mx_thread_create( &(vms_itimer_private->event_flag_thread),
					mx_interval_timer_event_flag_thread,
					*itimer );
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_VMS_ITIMER_PRIVATE *vms_itimer_private;
	double seconds_left;
	int vms_status;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vms_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for event flag %lu",
		fname, vms_itimer_private->event_flag));
#endif

	mx_status = mx_interval_timer_stop( itimer, &seconds_left );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_thread_stop( vms_itimer_private->event_flag_thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: About to free event flag %lu",
		fname, vms_itimer_private->event_flag ));
#endif

#if 0
	/* FIXME: Invoking lib$free_ef() here causes the program to die
	 * with an access violation for some reason.  For now, we have
	 * this section ifdef'ed out.
	 */

	vms_status = lib$free_ef( vms_itimer_private->event_flag );

	switch( vms_status ) {
	case SS$_NORMAL:
		break;
	case LIB$_EF_ALRFRE:
		mx_warning( "The attempt to free VMS event flag %lu failed, "
		"since VMS says that the event flag has already been freed.",
			vms_itimer_private->event_flag );
		break;
	case LIB$_EF_RESSYS:
		mx_warning( "Attempted to free VMS event flag %lu which is "
		"reserved to the operating system.  The allowed event flag "
		"values are in the ranges 1 to 23 and 32 to 63.",
			vms_itimer_private->event_flag );
		break;
	default:
		mx_warning( "The attempt to free VMS event flag %lu failed "
				"with VMS error code %d.",
				vms_itimer_private->event_flag,
				vms_status );
		break;
	}
#endif  /* 0 */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Successfully freed event flag %lu",
		fname, vms_itimer_private->event_flag ));
#endif

	mx_free( vms_itimer_private );

	mx_free( itimer );

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, mx_bool_type *busy )
{
	static const char fname[] = "mx_interval_timer_is_busy()";

	MX_VMS_ITIMER_PRIVATE *vms_itimer_private;
	uint32_t event_flag_cluster;
	int vms_status;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vms_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The busy pointer passed was NULL." );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for event flag %lu",
		fname, vms_itimer_private->event_flag));
#endif

	if ( vms_itimer_private->timer_is_busy ) {
		*busy = TRUE;
	} else {
		*busy = FALSE;

		itimer->timer_period = -1;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: *busy = %d", fname, *busy));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_start( MX_INTERVAL_TIMER *itimer,
				double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_start()";

	MX_VMS_ITIMER_PRIVATE *vms_itimer_private;
	double timer_period_in_nanoseconds;
	uint32_t current_time[2];
	uint32_t new_low_order;
	int vms_status;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vms_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	itimer->timer_period = timer_period_in_seconds;

	vms_itimer_private->timer_is_busy = FALSE;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for event flag %lu",
		fname, vms_itimer_private->event_flag));
	MX_DEBUG(-2,("%s: itimer = %p, vms_itimer_private = %p",
		fname, itimer, vms_itimer_private));
#endif

	/* If this is a timer restart for a periodic timer, then leave
	 * the existing timer period alone.  Otherwise, we must compute
	 * the timer period from the supplied 'timer_period_in_seconds'
	 * argument.
	 */

	if ( vms_itimer_private->restart_timer == FALSE ) {

		/* Convert the timer period to VMS 64-bit format.  Please note
		 * that all VMS computers are little-endian.
		 */

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: Starting timer for %g seconds.",
			fname, timer_period_in_seconds));
#endif

		timer_period_in_nanoseconds = 1.0e9 * timer_period_in_seconds;

		if ( timer_period_in_nanoseconds < vms_2_to_the_32nd_power ) {
			vms_itimer_private->timer_period[1] = 0;
			vms_itimer_private->timer_period[0] =
				mx_round( timer_period_in_nanoseconds );
		} else {
			vms_itimer_private->timer_period[1] =
				mx_round( timer_period_in_nanoseconds
					/ vms_2_to_the_32nd_power );

			vms_itimer_private->timer_period[0] =
				mx_round( fmod(timer_period_in_nanoseconds,
						vms_2_to_the_32nd_power) );
		}

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: timer_period_in_nanoseconds = %g",
			fname, timer_period_in_nanoseconds));
		MX_DEBUG(-2,("%s: timer_period[1] = %lu, timer_period[0] = %lu",
		  fname, (unsigned long) vms_itimer_private->timer_period[1],
		  fname, (unsigned long) vms_itimer_private->timer_period[0]));
#endif
	}

	/* Get the current time. */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Invoking sys$gettim() for event flag %lu",
		fname, vms_itimer_private->event_flag));
#endif

	vms_status = sys$gettim( current_time );

	switch( vms_status ) {
	case SS$_NORMAL:
		break;
	case SS$_ACCVIO:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The memory location to receive the time cannot be written "
		"by the caller." );
		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the current VMS time failed with "
		"a VMS error code of %d.", vms_status );
		break;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: current_time[1] = %lu, current_time[0] = %lu",
		fname, (unsigned long) current_time[1],
		(unsigned long) current_time[0]));
#endif

	/* Compute the finish time. */

	new_low_order = current_time[0] + vms_itimer_private->timer_period[0];

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: (ct[0] = %lu + tp[0] = %lu) --> new_low_order = %lu",
		fname, (unsigned long) current_time[0],
		(unsigned long) vms_itimer_private->timer_period[0],
		(unsigned long) new_low_order));
#endif

	vms_itimer_private->finish_time[0] = new_low_order;
	vms_itimer_private->finish_time[1] = current_time[1]
					+ vms_itimer_private->timer_period[1];

	/* Check for the carry bit. */
	
	if ( new_low_order < current_time[0] ) {
		(vms_itimer_private->finish_time[1])++;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: finish_time[1] = %lu, finish_time[0] = %lu",
		fname, (unsigned long) vms_itimer_private->finish_time[1],
		fname, (unsigned long) vms_itimer_private->finish_time[0]));
#endif

	/* Start the timer.
	 *
	 * We use the event flag number as the request id.
	 */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Invoking sys$setimr() for event flag %lu",
		fname, vms_itimer_private->event_flag));
#endif

	vms_status = sys$setimr( vms_itimer_private->event_flag,
				vms_itimer_private->finish_time, 0,
				vms_itimer_private->event_flag, 0 );

	switch( vms_status ) {
	case SS$_NORMAL:
		break;
	case SS$_ACCVIO:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The expiration time cannot be read by the caller." );
		break;
	case SS$_EXQUOTA:
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The process has exceeded its quota for timer entries "
		"or there is not enough memory to complete the request.");
		break;
	case SS$_ILLEFC:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested event flag %lu is not a legal event flag.",
			vms_itimer_private->event_flag );
		break;
	case SS$_INSFMEM:
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a timer queue entry." );
		break;
	case SS$_UNASEFC:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"This process is not associated with the cluster containing "
		"event flag %lu.", vms_itimer_private->event_flag );
		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to start the timer failed with VMS error code %d.",
			vms_status );
		break;
	}

	vms_itimer_private->timer_is_busy = TRUE;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer successfully started.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_stop( MX_INTERVAL_TIMER *itimer, double *seconds_left )
{
	static const char fname[] = "mx_interval_timer_stop()";

	MX_VMS_ITIMER_PRIVATE *vms_itimer_private;
	int vms_status;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vms_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for event flag %lu",
		fname, vms_itimer_private->event_flag));
#endif

	/* Read the time left on the timer, if requested, but do not abort
	 * if the attempt to read the timer value fails.
	 */

	if ( seconds_left != NULL ) {
		(void) mx_interval_timer_read( itimer, seconds_left );
	}

	itimer->timer_period = -1;

	/* Stop the timer.
	 *
	 * Note that this program uses the event flag number as the
	 * timer request id.
	 */
#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Invoking sys$cantim() for event flag %lu",
		fname, vms_itimer_private->event_flag));
#endif

	vms_status = sys$cantim( vms_itimer_private->event_flag, 0 );

	switch( vms_status ) {
	case SS$_NORMAL:
		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to stop the timer failed with VMS error code %d.",
			vms_status );
		break;
	}

	vms_itimer_private->timer_is_busy = FALSE;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Timer successfully stopped.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_read( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration )
{
	static const char fname[] = "mx_interval_timer_read()";

	MX_VMS_ITIMER_PRIVATE *vms_itimer_private;
	uint32_t current_time[2];
	uint32_t H1, L1, H2, L2;
	double H_result, L_result, nanoseconds_till_expiration;
	int vms_status;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vms_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( seconds_till_expiration == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The seconds_till_expiration passed was NULL." );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for event flag %lu",
		fname, vms_itimer_private->event_flag));
#endif

	if ( vms_2_to_the_32nd_power < 0.0 ) {
		vms_2_to_the_32nd_power = pow( 2.0, 32.0 );
	}

	/* Get the current time. */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Invoking sys$gettim() for event flag %lu",
		fname, vms_itimer_private->event_flag));
#endif

	vms_status = sys$gettim( current_time );

	switch( vms_status ) {
	case SS$_NORMAL:
		break;
	case SS$_ACCVIO:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The memory location to receive the time cannot be written "
		"by the caller." );
		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the current VMS time failed with "
		"a VMS error code of %d.", vms_status );
		break;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: current_time[1] = %lu, current_time[0] = %lu",
		fname, current_time[1], current_time[0]));
#endif

	/* Compute the time until expiration by subtracting the
	 * current time from the finish time.  If the current time,
	 * is after the finish time, then return 0.
	 */

	H1 = vms_itimer_private->finish_time[1];
	L1 = vms_itimer_private->finish_time[0];

	H2 = current_time[1];
	L2 = current_time[0];

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: H1 = %lu, L1 = %lu",
		fname, (unsigned long) H1, (unsigned long) L1));
	MX_DEBUG(-2,("%s: H2 = %lu, L2 = %lu",
		fname, (unsigned long) H2, (unsigned long) L2));
#endif

	/* Check for borrow. */

	if ( L1 < L2 ) {
		H1--;
	}

	if ( H1 < H2 ) {
		nanoseconds_till_expiration = 0.0;
	} else {
		H_result = H1 - H2;

		L_result = L1 - L2;

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: H_result = %g, L_result = %g",
			fname, H_result, L_result));
#endif

		nanoseconds_till_expiration =
			( vms_2_to_the_32nd_power * H_result ) + L_result;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,( "%s: nanoseconds_till_expiration = %g",
		fname, nanoseconds_till_expiration));
#endif

	*seconds_till_expiration = 1.0e-9 * nanoseconds_till_expiration;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,( "%s: *seconds_till_expiration = %g",
		fname, *seconds_till_expiration));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*************************** POSIX realtime timers **************************/

#elif HAVE_POSIX_TIMERS || defined(OS_IRIX)

#include <time.h>
#include <signal.h>
#include <errno.h>

#if defined(OS_BSD)
#include <sys/param.h>	/* Needed for #define __NetBSD_Version__ on NetBSD */
#endif

/* Select the notification method for the realtime timers.  The possible
 * values are SIGEV_THREAD or SIGEV_SIGNAL.  Generally SIGEV_THREAD is a
 * better choice since SIGEV_SIGNAL limits the maximum number of timers
 * to the total number of realtime signals for the process.
 */

#if defined(OS_SOLARIS) || defined(OS_IRIX) || defined(OS_TRU64)

#   define MX_SIGEV_TYPE	SIGEV_SIGNAL
#else
#   define MX_SIGEV_TYPE	SIGEV_THREAD
#endif

typedef struct {
	timer_t timer_id;
	struct sigevent evp;

#if ( MX_SIGEV_TYPE == SIGEV_SIGNAL )
	int signal_number;
	MX_THREAD *thread;
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

	*itimer_private = (MX_POSIX_ITIMER_PRIVATE *) itimer->private_ptr;

	if ( (*itimer_private) == (MX_POSIX_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POSIX_ITIMER_PRIVATE pointer for itimer %p is NULL.",
			itimer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

#if ( MX_SIGEV_TYPE == SIGEV_THREAD )

#if defined(__NetBSD_Version__)
static void mx_interval_timer_thread_handler( union sigval *sigev_value_ptr )
#else
static void mx_interval_timer_thread_handler( union sigval sigev_value )
#endif
{
	static const char fname[] = "mx_interval_timer_thread_handler()";

	MX_INTERVAL_TIMER *itimer;
	MX_THREAD *thread;
	mx_status_type mx_status;

#if defined(__NetBSD_Version__)
	itimer = (MX_INTERVAL_TIMER *) sigev_value_ptr->sival_ptr;
#else
	itimer = (MX_INTERVAL_TIMER *) sigev_value.sival_ptr;
#endif

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );

		return;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: itimer = %p", fname, itimer));
	MX_DEBUG(-2,("%s: itimer->callback_function = %p",
			fname, itimer->callback_function));
#endif

	/* Setup an MX_THREAD structure for this thread. */

	mx_status = mx_thread_build_data_structures( &thread );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	mx_status = mx_thread_save_thread_pointer( thread );

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

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return;
}

static mx_status_type
mx_interval_timer_create_event_handler( MX_INTERVAL_TIMER *itimer,
				MX_POSIX_ITIMER_PRIVATE *posix_itimer_private )
{
	posix_itimer_private->evp.sigev_value.sival_ptr = itimer;

	posix_itimer_private->evp.sigev_notify = SIGEV_THREAD;

	posix_itimer_private->evp.sigev_notify_function =
				(void *) mx_interval_timer_thread_handler;

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

#define MX_POSIX_SIGNAL_BLOCKING_SUPPORTED	TRUE

#include "mx_signal_alloc.h"

/*---*/

static mx_status_type
mx_interval_timer_signal_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mx_interval_timer_signal_thread()";

	MX_INTERVAL_TIMER *itimer;
	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	sigset_t signal_set;
	siginfo_t siginfo;
	int signal_number, status, saved_errno;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	itimer = (MX_INTERVAL_TIMER *) args;

	posix_itimer_private = (MX_POSIX_ITIMER_PRIVATE *) itimer->private_ptr;

	/* Setup the signal mask that the thread uses. */

	status = sigemptyset( &signal_set );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"A call to sigemptyset() failed with errno = %d, "
			"error message = '%s'.",
				saved_errno, strerror(saved_errno) );
	}

	status = sigaddset( &signal_set, posix_itimer_private->signal_number );

	if ( status != 0 ) {
		saved_errno = errno;

		if ( saved_errno == EINVAL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"sigaddset() says that signal %d is an invalid "
			"or unsupported signal number.",
				posix_itimer_private->signal_number );
		} else {
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to add signal %d to the blocked signal set "
			"with sigaddset().  Errno = %d, error message = '%s'.",
				posix_itimer_private->signal_number,
				saved_errno, strerror(saved_errno) );
		}
	}

#if MX_POSIX_SIGNAL_BLOCKING_SUPPORTED

	/* Unblock the signal for this thread. */

	status = pthread_sigmask( SIG_UNBLOCK, &signal_set, NULL );

	if ( status != 0 ) {
		switch( status ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The first argument to pthread_sigmask() was invalid.");
			break;
		default:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to unblock signal %d using pthread_sigmask().  "
			"Errno = %d, error message = '%s'.",
				posix_itimer_private->signal_number,
				status, strerror(status) );
			break;
		}
	}
#endif /* MX_POSIX_SIGNAL_BLOCKING_SUPPORTED */

	/* Wait in an infinite loop for signals to arrive. */

	while (1) {

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: About to invoke sigwaitinfo()", fname));
#endif

		signal_number = sigwaitinfo( &signal_set, &siginfo );

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,
		("%s: Returned from sigwaitinfo(), signal_number = %d",
			fname, signal_number));
#endif

		if ( signal_number < 0 ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case ENOSYS:
				return mx_error( MXE_NOT_AVAILABLE, fname,
			"sigwaitinfo() is not available on this platform.  "
			"This means that MX interval timers will not work on "
			"this platform." );
				break;
			case EINTR:
				return mx_error( MXE_INTERRUPTED, fname,
			"sigwaitinfo() was interrupted by an unblocked, caught "
			"signal for interval timer %p (thread %p).",
					itimer, thread );
				break;
			default:
				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error code returned by sigwaitinfo() for "
			"interval timer %p (thread %p).  "
			"Errno = %d, error message = '%s'.",
					itimer, thread, saved_errno,
					strerror( saved_errno ) );
			}
		}

		/* Invoke the callback function. */

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: Invoking callback function %p",
			fname, itimer->callback_function));
#endif

		if ( itimer->callback_function != NULL ) {
			itimer->callback_function( itimer,
						itimer->callback_args );
		}

		/* If this is supposed to be a one-shot timer, then
		 * make sure that the timer is turned off and then exit.
		 */

		if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
			(void) mx_interval_timer_stop( itimer, NULL );

			/* Get rid of the MX_THREAD structure for this thread.*/

			mx_status = mx_thread_free_data_structures( thread );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if MX_POSIX_SIGNAL_BLOCKING_SUPPORTED

			/* Block the signal again. */

			status = pthread_sigmask(SIG_BLOCK, &signal_set, NULL);

			if ( status != 0 ) {
				switch( status ) {
				case EINVAL:
					return mx_error(
					    MXE_ILLEGAL_ARGUMENT, fname,
			"The first argument to pthread_sigmask() was invalid.");
					break;
				default:
					return mx_error(
					    MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to reblock signal %d using pthread_sigmask().  "
			"Errno = %d, error message = '%s'.",
					posix_itimer_private->signal_number,
						status, strerror(status) );
					break;
				}
			}
#endif /* MX_POSIX_SIGNAL_BLOCKING_SUPPORTED */

			/* End the thread by returning to the caller. */

#if MX_INTERVAL_TIMER_DEBUG
			MX_DEBUG(-2,("%s exiting.", fname));
#endif
	
			return MX_SUCCESSFUL_RESULT;
		}
	}
}

static mx_status_type
mx_interval_timer_create_event_handler( MX_INTERVAL_TIMER *itimer,
				MX_POSIX_ITIMER_PRIVATE *posix_itimer_private )
{
	mx_status_type mx_status;

	if ( mx_signals_are_initialized() == FALSE ) {
		mx_status = mx_signal_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_signal_allocate( MXF_ANY_REALTIME_SIGNAL,
				&(posix_itimer_private->signal_number) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	posix_itimer_private->evp.sigev_value.sival_ptr = itimer;

	posix_itimer_private->evp.sigev_notify = SIGEV_SIGNAL;
	posix_itimer_private->evp.sigev_signo =
					posix_itimer_private->signal_number;

	/* It is not necessary to install a signal handler here, since that
	 * will already have been done for us by mx_signal_initialize().
	 */

	/* Create a thread to service the signals. */

	mx_status = mx_thread_create( &(posix_itimer_private->thread),
					mx_interval_timer_signal_thread,
					itimer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_interval_timer_destroy_event_handler( MX_INTERVAL_TIMER *itimer,
				MX_POSIX_ITIMER_PRIVATE *posix_itimer_private )
{
	mx_status_type mx_status;

	mx_status = mx_signal_free( posix_itimer_private->signal_number );

	return mx_status;
}

#else
#error MX_SIGEV_TYPE must be either SIGEV_THREAD or SIGEV_SIGNAL.  If available, SIGEV_THREAD is usually the better choice.
#endif /* MX_SIGEV_TYPE */

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				MX_INTERVAL_TIMER_CALLBACK *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	int status, saved_errno;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for POSIX realtime timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));
#endif

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
	}

	*itimer = (MX_INTERVAL_TIMER *) malloc( sizeof(MX_INTERVAL_TIMER) );

	if ( (*itimer) == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for an MX_INTERVAL_TIMER structure.");
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: *itimer = %p", fname, *itimer));
#endif

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
	(*itimer)->private_ptr = posix_itimer_private;

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
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"One or more of the arguments to timer_create() were invalid.");
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error creating a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
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

	posix_itimer_private = NULL;

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
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, mx_bool_type *busy )
{
	static const char fname[] = "mx_interval_timer_is_busy()";

	MX_POSIX_ITIMER_PRIVATE *posix_itimer_private;
	struct itimerspec value;
	int status, saved_errno;
	mx_status_type mx_status;

	posix_itimer_private = NULL;

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
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error reading from a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
		}
	}

	if ( (value.it_value.tv_sec != 0)
	  || (value.it_value.tv_nsec != 0 ) )
	{
		*busy = TRUE;
	} else {
		*busy = FALSE;

		itimer->timer_period = -1;
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
	time_t timer_seconds;
	long   timer_nsec;
	mx_status_type mx_status;

	posix_itimer_private = NULL;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&posix_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	itimer->timer_period = timer_period_in_seconds;

	/* Convert the timer period to a struct timespec. */

	timer_seconds = (time_t) timer_period_in_seconds;

	timer_nsec = (long) ( 1.0e9 * ( timer_period_in_seconds
					- (double) timer_seconds ) );

	/* The way we use the timer values depends on whether or not
	 * we configure the timer as a one-shot timer or a periodic
	 * timer.
	 */

	itimer_value.it_value.tv_sec  = timer_seconds;
	itimer_value.it_value.tv_nsec = timer_nsec;

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		itimer_value.it_interval.tv_sec  = 0;
		itimer_value.it_interval.tv_nsec = 0;
	} else {
		itimer_value.it_interval.tv_sec  = timer_seconds;
		itimer_value.it_interval.tv_nsec = timer_nsec;
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
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error writing to a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
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

	posix_itimer_private = NULL;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&posix_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	itimer->timer_period = -1;

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
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error writing to a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
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

	posix_itimer_private = NULL;

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
		case ENOSYS:
			return mx_error( MXE_UNSUPPORTED, fname,
		"This system does not support Posix realtime timers "
		"although the compiler said that it does.  "
		"This should not be able to happen." );
		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected error reading from a Posix realtime timer.  "
		"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
		}
	}

	*seconds_till_expiration = (double) value.it_value.tv_sec;

	*seconds_till_expiration += 1.0e-9 * (double) value.it_value.tv_nsec;

	return MX_SUCCESSFUL_RESULT;
}

/************************ BSD style kqueue() timers ***********************/

#elif defined( __FreeBSD__ ) || defined( __NetBSD__ )

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>

typedef struct {
	MX_THREAD *thread;
	int kq;
	int ident;
	mx_bool_type busy;
} MX_KQUEUE_ITIMER_PRIVATE;

/*---*/

static mx_status_type
mx_interval_timer_get_pointers( MX_INTERVAL_TIMER *itimer,
			MX_KQUEUE_ITIMER_PRIVATE **itimer_private,
			const char *calling_fname )
{
	static const char fname[] = "mx_interval_timer_get_pointers()";

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( itimer_private == (MX_KQUEUE_ITIMER_PRIVATE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KQUEUE_ITIMER_PRIVATE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*itimer_private = (MX_KQUEUE_ITIMER_PRIVATE *) itimer->private_ptr;

	if ( (*itimer_private) == (MX_KQUEUE_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KQUEUE_ITIMER_PRIVATE pointer for itimer %p is NULL.",
			itimer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mx_interval_timer_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mx_interval_timer_thread()";

	MX_INTERVAL_TIMER *itimer;
	MX_KQUEUE_ITIMER_PRIVATE *kqueue_itimer_private;
	struct kevent event;
	int num_events;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for thread %p, args %p.",
		fname, thread, args));
#endif

	itimer = (MX_INTERVAL_TIMER *) args;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&kqueue_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Loop forever. */

	for (;;) {
		/* Wait forever for an event. */

		num_events = kevent( kqueue_itimer_private->kq,
					NULL, 0, &event, 1, NULL );

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: kqueue() returned.  num_events = %d",
			fname, num_events));
#endif

		/* Did we get the event we were expecting? */

		if ( (event.ident != kqueue_itimer_private->ident)
		  || (event.filter != EVFILT_TIMER) )
		{
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unexpected event type seen.  "
			"event.ident = %d, event.filter = %d instead of "
			"the expected values of "
			"event.ident = %d, event.filter = %d (EVFILT_TIMER).",
				(int) event.ident, event.filter,
				kqueue_itimer_private->ident, EVFILT_TIMER );
		} else {
			/* Invoke the callback function. */

			if ( itimer->callback_function != NULL ) {
				itimer->callback_function( itimer,
						itimer->callback_args );
			}
		}

		/* If this was a one-shot timer, mark the timer as not busy. */

		if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
			kqueue_itimer_private->busy = FALSE;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				MX_INTERVAL_TIMER_CALLBACK *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_KQUEUE_ITIMER_PRIVATE *kqueue_itimer_private;
	int saved_errno;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for BSD kqueue timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));
#endif

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

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: *itimer = %p", fname, *itimer));
#endif

	kqueue_itimer_private = (MX_KQUEUE_ITIMER_PRIVATE *)
				malloc( sizeof(MX_KQUEUE_ITIMER_PRIVATE) );

	if ( kqueue_itimer_private == (MX_KQUEUE_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_KQUEUE_ITIMER_PRIVATE structure." );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: kqueue_itimer_private = %p",
			fname, kqueue_itimer_private));
#endif

	(*itimer)->timer_type = timer_type;
	(*itimer)->timer_period = -1.0;
	(*itimer)->num_overruns = 0;
	(*itimer)->callback_function = callback_function;
	(*itimer)->callback_args = callback_args;
	(*itimer)->private_ptr = kqueue_itimer_private;

	/* Create the kqueue specific data structures. */

	kqueue_itimer_private->kq = kqueue();

	if ( kqueue_itimer_private->kq < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to create a BSD kqueue failed with "
		"errno = %d, error = '%s'.",
			saved_errno, strerror(saved_errno) );
	}

	kqueue_itimer_private->ident = 1;

	/* Create a thread to manage the kqueue. */

	mx_status = mx_thread_create( &(kqueue_itimer_private->thread),
					mx_interval_timer_thread,
					*itimer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_KQUEUE_ITIMER_PRIVATE *kqueue_itimer_private;
	double seconds_left;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&kqueue_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_stop( itimer, &seconds_left );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	close( kqueue_itimer_private->kq );

	mx_free( kqueue_itimer_private );

	mx_free( itimer );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, mx_bool_type *busy )
{
	static const char fname[] = "mx_interval_timer_is_busy()";

	MX_KQUEUE_ITIMER_PRIVATE *kqueue_itimer_private;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&kqueue_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy != (mx_bool_type *) NULL ) {
		*busy = kqueue_itimer_private->busy;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_start( MX_INTERVAL_TIMER *itimer,
			double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_start()";

	MX_KQUEUE_ITIMER_PRIVATE *kqueue_itimer_private;
	struct kevent event;
	int num_events, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&kqueue_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	itimer->timer_period = timer_period_in_seconds;

	/* Construct the kevent structure. */

	event.ident = kqueue_itimer_private->ident;

	event.filter = EVFILT_TIMER;

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		event.flags = EV_ADD | EV_ONESHOT;
	} else {
		event.flags = EV_ADD;
	}

	/* EVFILT_TIMER has no fflags. */

	event.fflags = 0;

	/* Don't need udata since the callback_function and callback_args
	 * pointers in the MX_INTERVAL_TIMER structure already contain
	 * everything we need.
	 */

	event.udata = NULL;

	/* Convert the timer period to milliseconds. */

	event.data = mx_round( 1000.0 * timer_period_in_seconds );

	/* Add the kevent to the kqueue. */

	kqueue_itimer_private->busy = TRUE;

	num_events = kevent( kqueue_itimer_private->kq,
				&event, 1, NULL, 0, NULL );

	if ( num_events < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to add a timer event to the event queue failed.  "
		"Errno = %d, error message = '%s'.",
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_stop( MX_INTERVAL_TIMER *itimer, double *seconds_left )
{
	static const char fname[] = "mx_interval_timer_stop()";

	MX_KQUEUE_ITIMER_PRIVATE *kqueue_itimer_private;
	struct kevent event;
	int num_events, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&kqueue_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the delete command for kqueue. */

	event.ident  = kqueue_itimer_private->ident;
	event.filter = EVFILT_TIMER;
	event.flags  = EV_DELETE;
	event.fflags = 0;
	event.data   = 0;
	event.udata  = NULL;

	kqueue_itimer_private->busy = FALSE;

	/* Delete the event from the kqueue. */

	num_events = kevent( kqueue_itimer_private->kq,
				&event, 1, NULL, 0, NULL );

	if ( num_events < 0 ) {
		saved_errno = errno;

		if ( saved_errno == ENOENT ) {
			/* If we get here, the event was already gone
			 * by the time we tried to delete it.
			 */

			return MX_SUCCESSFUL_RESULT;
		}

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to delete a timer event from the event "
		"queue failed.  Errno = %d, error message = '%s'.",
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_read( MX_INTERVAL_TIMER *itimer,
			double *seconds_till_expiration )
{
	static const char fname[] = "mx_interval_timer_read()";

	MX_KQUEUE_ITIMER_PRIVATE *kqueue_itimer_private;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&kqueue_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( seconds_till_expiration != (double *) NULL ) {
		*seconds_till_expiration = 0.0;
	}

	return MX_SUCCESSFUL_RESULT;
}

/************************ Mach-based MacOS X timers ***********************/

#elif defined( OS_MACOSX )

#include <errno.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

typedef struct {
	MX_THREAD *thread;

	pthread_mutex_t mutex;
	pthread_cond_t cond;

	mx_bool_type busy;
	mx_bool_type destroy_timer;

	uint64_t absolute_timer_period;
	uint64_t absolute_expiration_time;

	uint64_t timebase_info_numerator;
	uint64_t timebase_info_denominator;
} MX_MACH_ITIMER_PRIVATE;

/*---*/

static mx_status_type
mx_interval_timer_get_pointers( MX_INTERVAL_TIMER *itimer,
			MX_MACH_ITIMER_PRIVATE **itimer_private,
			const char *calling_fname )
{
	static const char fname[] = "mx_interval_timer_get_pointers()";

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( itimer_private == (MX_MACH_ITIMER_PRIVATE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MACH_ITIMER_PRIVATE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*itimer_private = (MX_MACH_ITIMER_PRIVATE *) itimer->private_ptr;

	if ( (*itimer_private) == (MX_MACH_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MACH_ITIMER_PRIVATE pointer for itimer %p is NULL.",
			itimer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mx_interval_timer_thread_destroy( MX_THREAD *thread,
				MX_INTERVAL_TIMER *itimer,
				MX_MACH_ITIMER_PRIVATE *mach_itimer_private )
{
	static const char fname[] = "mx_interval_timer_thread_destroy()";

	int status, saved_errno;

	MX_DEBUG(-2,("%s invoked for itimer %p", fname, itimer));

	status = pthread_cond_destroy( &(mach_itimer_private->cond) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to destroy a Posix condition variable failed with "
		"errno = %d, error = '%s'.",
			saved_errno, strerror(saved_errno) );
	}

	status = pthread_mutex_destroy( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to destroy a Posix mutex failed with "
		"errno = %d, error = '%s'.",
			saved_errno, strerror(saved_errno) );
	}

	mx_free( mach_itimer_private );

	mx_free( itimer );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

#define UNLOCK_MUTEX \
    do { \
	(void) pthread_mutex_unlock( &(mach_itimer_private->mutex) ); \
    } while (0)

static mx_status_type
mx_interval_timer_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mx_interval_timer_thread()";

	MX_INTERVAL_TIMER *itimer;
	MX_MACH_ITIMER_PRIVATE *mach_itimer_private;
	uint64_t absolute_expiration_time, next_expiration_time;
	int status, saved_errno;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for thread %p, args %p.",
		fname, thread, args));
#endif

	itimer = (MX_INTERVAL_TIMER *) args;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&mach_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Lock the mutex before entering the infinite loop. */

	status = pthread_mutex_lock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to lock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

	/* Loop forever. */

	for (;;) {
		/* At this point in the loop, the mutex should _already_
		 * have been locked either before the start of the loop
		 * or at the bottom of the previous pass through the loop.
		 * The loop is structured this way to avoid extra locking
		 * and unlocking of the mutex.
		 */

		if ( mach_itimer_private->destroy_timer ) {
			UNLOCK_MUTEX;
			return mx_interval_timer_thread_destroy(
				thread, itimer, mach_itimer_private );
		}

		/* Wait forever in a condition variable for busy to be TRUE. */

		while ( mach_itimer_private->busy == FALSE ) {

			status = pthread_cond_wait(
					&(mach_itimer_private->cond),
					&(mach_itimer_private->mutex) );

			if ( status != 0 ) {
				saved_errno = errno;
				UNLOCK_MUTEX;
				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
					"An attempt to wait on the condition "
					"variable for interval timer %p failed "
					"with errno = %d, error = '%s'.",
						itimer, saved_errno,
						strerror(saved_errno) );
			}

			if ( mach_itimer_private->destroy_timer ) {
				UNLOCK_MUTEX;
				return mx_interval_timer_thread_destroy(
					thread, itimer, mach_itimer_private );
			}
		}

		/* If we get here, busy has been set to TRUE by
		 * mx_interval_timer_start().   The first thing
		 * we do is save a copy of the expiration time.
		 */

		absolute_expiration_time
			= mach_itimer_private->absolute_expiration_time;

		/* Next, we unlock the mutex. */

		status = pthread_mutex_unlock( &(mach_itimer_private->mutex) );

		if ( status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"An attempt to unlock the mutex for interval timer %p "
			"failed with errno = %d, error = '%s'.",
				itimer, saved_errno, strerror(saved_errno) );
		}

		/* Since another thread has requested that the timer start,
		 * we wait here until the expiration time for the timer
		 * using the Mach function mach_wait_until().
		 */

		mach_wait_until( absolute_expiration_time );

		/* The expiration time has arrived, so invoke
		 * the callback_function.
		 */

		if ( itimer->callback_function != NULL ) {
			itimer->callback_function( itimer,
						itimer->callback_args );
		}

		/* Relock the mutex so that we can modify the contents
		 * of 'mach_itimer_private'.
		 */

		status = pthread_mutex_lock( &(mach_itimer_private->mutex) );

		if ( status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"An attempt to lock the mutex for interval timer %p "
			"failed with errno = %d, error = '%s'.",
				itimer, saved_errno, strerror(saved_errno) );
		}

		if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {

			/* If this was a one-shot timer, mark the timer
			 * as not busy.
			 */

			mach_itimer_private->busy = FALSE;
		} else {
			/* If this is a periodic timer, we must compute
			 * and save the time of the next expiration.
			 */

			next_expiration_time = absolute_expiration_time
				+ mach_itimer_private->absolute_timer_period;

			mach_itimer_private->absolute_expiration_time
				= next_expiration_time;
		}

		/* Leave the mutex locked since it will be unlocked near
		 * the top of the next pass through the for(;;) loop.
		 */

	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				MX_INTERVAL_TIMER_CALLBACK *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_MACH_ITIMER_PRIVATE *mach_itimer_private;
	struct mach_timebase_info timebase_info;

	int status, saved_errno;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for BSD mach timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));
#endif

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

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: *itimer = %p", fname, *itimer));
#endif

	mach_itimer_private = (MX_MACH_ITIMER_PRIVATE *)
				malloc( sizeof(MX_MACH_ITIMER_PRIVATE) );

	if ( mach_itimer_private == (MX_MACH_ITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_MACH_ITIMER_PRIVATE structure." );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: mach_itimer_private = %p",
			fname, mach_itimer_private));
#endif

	(*itimer)->timer_type = timer_type;
	(*itimer)->timer_period = -1.0;
	(*itimer)->num_overruns = 0;
	(*itimer)->callback_function = callback_function;
	(*itimer)->callback_args = callback_args;
	(*itimer)->private_ptr = mach_itimer_private;

	/* Initialize the MacOS X specific data structures. */

	mach_itimer_private->busy          = FALSE;
	mach_itimer_private->destroy_timer = FALSE;

	(void) mach_timebase_info( &timebase_info );

	mach_itimer_private->timebase_info_numerator
				= (uint64_t) timebase_info.numer;

	mach_itimer_private->timebase_info_denominator
				= (uint64_t) timebase_info.denom;

	status = pthread_mutex_init( &(mach_itimer_private->mutex), NULL );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to create a Posix mutex failed with "
		"errno = %d, error = '%s'.",
			saved_errno, strerror(saved_errno) );
	}

	status = pthread_cond_init( &(mach_itimer_private->cond), NULL );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to create a Posix condition variable failed with "
		"errno = %d, error = '%s'.",
			saved_errno, strerror(saved_errno) );
	}

	/* Create a thread to manage the Mach timer. */

	mx_status = mx_thread_create( &(mach_itimer_private->thread),
					mx_interval_timer_thread,
					*itimer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_MACH_ITIMER_PRIVATE *mach_itimer_private;
	double seconds_left;
	int status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&mach_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_stop( itimer, &seconds_left );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* It is not safe for us to destroy the contents of
	 * 'mach_itimer_private' ourselves since the interval
	 * timer thread is still using them.  Instead, we tell
	 * the interval timer thread to do the destruction
	 * itself.
	 */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: About to request destruction of interval timer %p",
		fname, itimer));
#endif

	status = pthread_mutex_lock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to lock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

	mach_itimer_private->destroy_timer = TRUE;

	status = pthread_mutex_unlock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to unlock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Finished requesting destruction of interval timer %p",
		fname, itimer));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, mx_bool_type *busy )
{
	static const char fname[] = "mx_interval_timer_is_busy()";

	MX_MACH_ITIMER_PRIVATE *mach_itimer_private;
	int status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&mach_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy == (mx_bool_type *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

#if 0 && MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: About to request busy status for interval timer %p",
		fname, itimer));
#endif

	status = pthread_mutex_lock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to lock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

	*busy = mach_itimer_private->busy;

	status = pthread_mutex_unlock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to unlock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

#if 0 && MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,
	("%s: Finished requesting busy status for interval timer %p",
		fname, itimer));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_start( MX_INTERVAL_TIMER *itimer,
			double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_start()";

	MX_MACH_ITIMER_PRIVATE *mach_itimer_private;
	uint64_t timer_period_in_nanoseconds, absolute_timer_period;
	uint64_t absolute_current_time, absolute_expiration_time;
	int status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&mach_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: About to start interval timer %p for %g seconds",
		fname, itimer, timer_period_in_seconds));
#endif

	status = pthread_mutex_lock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to lock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

	/* Compute the absolute timer period and the first expiration time. */

	absolute_current_time = mach_absolute_time();

	itimer->timer_period = timer_period_in_seconds;

	timer_period_in_nanoseconds
		= (uint64_t) ( 1.0e9 * timer_period_in_seconds );

	absolute_timer_period = timer_period_in_nanoseconds
			* mach_itimer_private->timebase_info_denominator
				/ mach_itimer_private->timebase_info_numerator;

	absolute_expiration_time
		= absolute_current_time + absolute_timer_period;

	mach_itimer_private->absolute_timer_period = absolute_timer_period;
	mach_itimer_private->absolute_expiration_time
						= absolute_expiration_time;

	/* Mark the timer as busy. */

	mach_itimer_private->busy = TRUE;

	status = pthread_mutex_unlock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to unlock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

	/* Wake up the interval timer thread. */

	status = pthread_cond_signal( &(mach_itimer_private->cond) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to wake up the timer thread for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Finished starting interval timer %p", fname, itimer));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_stop( MX_INTERVAL_TIMER *itimer, double *seconds_left )
{
	static const char fname[] = "mx_interval_timer_stop()";

	MX_MACH_ITIMER_PRIVATE *mach_itimer_private;
	uint64_t absolute_current_time, absolute_expiration_time;
	uint64_t absolute_time_left, time_left_in_nanoseconds;
	int status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&mach_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: About to stop interval timer %p",
		fname, itimer));
#endif

	status = pthread_mutex_lock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to lock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

	/* Find out how much time was left on the timer. */

	if ( seconds_left != (double *) NULL ) {
		absolute_expiration_time
			= mach_itimer_private->absolute_expiration_time;

		absolute_current_time = mach_absolute_time();

		if ( absolute_current_time >= absolute_expiration_time ) {
			*seconds_left = 0.0;
		} else {
			absolute_time_left
			    = absolute_expiration_time - absolute_current_time;

			time_left_in_nanoseconds = absolute_time_left
			    * mach_itimer_private->timebase_info_numerator
			       / mach_itimer_private->timebase_info_denominator;

			*seconds_left
				= 1.0e-9 * (double) time_left_in_nanoseconds;
		}
	}

	/* Stop the timer by setting busy to FALSE. */

	mach_itimer_private->busy = FALSE;

	status = pthread_mutex_unlock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to unlock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

	/* Wake up the interval timer thread. */

	status = pthread_cond_signal( &(mach_itimer_private->cond) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to wake up the timer thread for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Finished stopping interval timer %p",
		fname, itimer));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_read( MX_INTERVAL_TIMER *itimer,
			double *seconds_till_expiration )
{
	static const char fname[] = "mx_interval_timer_read()";

	MX_MACH_ITIMER_PRIVATE *mach_itimer_private;
	uint64_t absolute_current_time, absolute_expiration_time;
	uint64_t absolute_time_left, time_left_in_nanoseconds;
	int status, saved_errno;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&mach_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( seconds_till_expiration == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The seconds_till_expiration pointer passed was NULL." );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: About to read interval timer %p",
		fname, itimer));
#endif

	status = pthread_mutex_lock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to lock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

	/* Find out how much time was left on the timer. */

	absolute_expiration_time
		= mach_itimer_private->absolute_expiration_time;

	absolute_current_time = mach_absolute_time();

	if ( absolute_current_time >= absolute_expiration_time ) {
		*seconds_till_expiration = 0.0;
	} else {
		absolute_time_left
			    = absolute_expiration_time - absolute_current_time;

		time_left_in_nanoseconds = absolute_time_left
			* mach_itimer_private->timebase_info_numerator
			    / mach_itimer_private->timebase_info_denominator;

		*seconds_till_expiration
			= 1.0e-9 * (double) time_left_in_nanoseconds;
	}

	status = pthread_mutex_unlock( &(mach_itimer_private->mutex) );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to unlock the mutex for interval timer %p "
		"failed with errno = %d, error = '%s'.",
			itimer, saved_errno, strerror(saved_errno) );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Read interval timer %p, seconds_till_expiration = %g",
		fname, itimer, *seconds_till_expiration));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/************************ VxWorks "watchdog" timers ***********************/

#elif defined( OS_VXWORKS )

#include "wdLib.h"
#include "eventLib.h"
#include "sysLib.h"

typedef struct {
	WDOG_ID watchdog_id;
	UINT32 event_id;
	MX_INTERVAL_TIMER *itimer;
	MX_THREAD *thread;
	int callback_task_id;
	mx_bool_type busy;
	unsigned long event_counter;
} MX_VXWORKS_WATCHDOG_PRIVATE;

/*---*/

static FUNCPTR mxp_interval_timer_callback( int parameter );

/*---*/

static mx_status_type
mx_interval_timer_get_pointers( MX_INTERVAL_TIMER *itimer,
			MX_VXWORKS_WATCHDOG_PRIVATE **itimer_private,
			const char *calling_fname )
{
	static const char fname[] = "mx_interval_timer_get_pointers()";

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( itimer_private == (MX_VXWORKS_WATCHDOG_PRIVATE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_VXWORKS_WATCHDOG_PRIVATE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*itimer_private = (MX_VXWORKS_WATCHDOG_PRIVATE *) itimer->private_ptr;

	if ( (*itimer_private) == (MX_VXWORKS_WATCHDOG_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_VXWORKS_WATCHDOG_PRIVATE pointer for itimer %p is NULL.",
			itimer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mx_interval_timer_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mx_interval_timer_thread()";

	MX_INTERVAL_TIMER *itimer;
	MX_VXWORKS_WATCHDOG_PRIVATE *vxworks_wd_private;
	int task_id_self;
	UINT32 receive_event_mask, events_received;
	STATUS vx_status;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for thread %p, args %p.",
		fname, thread, args));
#endif

	itimer = (MX_INTERVAL_TIMER *) args;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vxworks_wd_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	task_id_self = taskIdSelf();

	if ( task_id_self == ERROR ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
	    "The current task id %#x is the value of ERROR.", task_id_self );
	}

	vxworks_wd_private->callback_task_id = taskIdSelf();

	receive_event_mask = vxworks_wd_private->event_id;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: event_id = %#x, receive_event_mask = %#x",
		fname, vxworks_wd_private->event_id, receive_event_mask));
#endif

	/* Loop forever. */

	for (;;) {
		events_received = 0;

		/* Wait forever for an event. */

		vx_status = eventReceive( receive_event_mask,
				EVENTS_WAIT_ANY, WAIT_FOREVER,
				&events_received );

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: eventReceive() returned, "
			"vx_status = %d, events_received = %lx",
			fname, (int) vx_status,
			(unsigned long) events_received));
#endif

		if ( vx_status != OK ) {
			vxworks_wd_private->busy = FALSE;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error %d occurred while waiting for an event "
			"for interval timer %p",
				(int) vx_status, itimer );
		} else {
			/* Invoke the MX interval timer's callback function. */

			if ( itimer->callback_function != NULL ) {
				itimer->callback_function( itimer,
						itimer->callback_args );
			}
		}

#if 0 && MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: MX callback function has returned.", fname));
#endif

		if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
			/* For a one-shot timer, mark the timer as not busy. */

			vxworks_wd_private->busy = FALSE;
		} else
		if ( itimer->timer_type == MXIT_PERIODIC_TIMER ) {

			vxworks_wd_private->busy = TRUE;

			/* Reschedule the timer to run again. */

			vx_status = wdStart( vxworks_wd_private->watchdog_id,
					sysClkRateGet() * itimer->timer_period,
					(FUNCPTR) mxp_interval_timer_callback,
					(int) vxworks_wd_private );

			if ( vx_status != OK ) {
				vxworks_wd_private->busy = FALSE;

				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Error %d occurred while calling wdStart() "
				"for interval timer %p",
					(int) vx_status, itimer );
			}
		} else {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown interval timer type %d requested for "
			"interval timer %p.", itimer->timer_type, itimer );
		}

#if 0 && MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: Periodic callback rescheduled.", fname));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static FUNCPTR mxp_interval_timer_callback( int parameter )
{
	MX_VXWORKS_WATCHDOG_PRIVATE *vxworks_wd_private;
	int callback_task_id;
	STATUS vx_status;

	/* FIXME: Every time you cast an integer back to a pointer,
	 * the universe kills a kitten.
	 */

	vxworks_wd_private = (MX_VXWORKS_WATCHDOG_PRIVATE *) parameter;

	callback_task_id = vxworks_wd_private->callback_task_id;

	if ( callback_task_id == ERROR ) {
		return NULL;
	}

	/* We are in Interrupt context, so we must get out of
	 * here as soon as possible.  Thus, we send a message
	 * to mx_interval_timer_thread() to tell it to do the
	 * real work.
	 */

	vx_status = eventSend( callback_task_id, vxworks_wd_private->event_id );

	/* FIXME: I'm not sure what I am supposed to return here. */

	return NULL;
}

/*---*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				MX_INTERVAL_TIMER_CALLBACK *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_VXWORKS_WATCHDOG_PRIVATE *vxworks_wd_private;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for VxWorks watchdog timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));
#endif

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

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: *itimer = %p", fname, *itimer));
#endif

	vxworks_wd_private = (MX_VXWORKS_WATCHDOG_PRIVATE *)
				malloc( sizeof(MX_VXWORKS_WATCHDOG_PRIVATE) );

	if ( vxworks_wd_private == (MX_VXWORKS_WATCHDOG_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
    "Unable to allocate memory for a MX_VXWORKS_WATCHDOG_PRIVATE structure." );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: vxworks_wd_private = %p",
			fname, vxworks_wd_private));
#endif

	(*itimer)->timer_type = timer_type;
	(*itimer)->timer_period = -1.0;
	(*itimer)->num_overruns = 0;
	(*itimer)->callback_function = callback_function;
	(*itimer)->callback_args = callback_args;
	(*itimer)->private_ptr = vxworks_wd_private;

	/* Create the VxWorks specific data structures. */

	vxworks_wd_private->watchdog_id = wdCreate();

	if ( vxworks_wd_private->watchdog_id == NULL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to create a VxWorks watchdog structure failed "
		"for interval timer %p since we ran out of memory.",
			itimer );
	}

	/* FIXME: In principle, the event id should be unique, but I do not
	 * currently know of a way to make that happen.
	 */

	vxworks_wd_private->event_id = (UINT32) 0x8000;

	/* Initialize the rest of this stuff. */

	vxworks_wd_private->itimer = *itimer;
	vxworks_wd_private->thread = NULL;
	vxworks_wd_private->callback_task_id = ERROR;
	vxworks_wd_private->busy = FALSE;
	vxworks_wd_private->event_counter = 0;

	/* Create a thread to handle timer callbacks. */

	mx_status = mx_thread_create( &(vxworks_wd_private->thread),
					mx_interval_timer_thread,
					*itimer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer )
{
	static const char fname[] = "mx_interval_timer_destroy()";

	MX_VXWORKS_WATCHDOG_PRIVATE *vxworks_wd_private;
	double seconds_left;
	STATUS vx_status;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vxworks_wd_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	vxworks_wd_private->busy = FALSE;

	mx_status = mx_interval_timer_stop( itimer, &seconds_left );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_thread_stop( vxworks_wd_private->thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vx_status = wdDelete( vxworks_wd_private->watchdog_id );

	if ( vx_status != OK ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to deallocate watchdog timer %d failed.",
			(int) vxworks_wd_private->watchdog_id );
	}

	mx_free( vxworks_wd_private );

	mx_free( itimer );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, mx_bool_type *busy )
{
	static const char fname[] = "mx_interval_timer_is_busy()";

	MX_VXWORKS_WATCHDOG_PRIVATE *vxworks_wd_private;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vxworks_wd_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy != (mx_bool_type *) NULL ) {
		*busy = vxworks_wd_private->busy;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: busy = %d", fname, (int) vxworks_wd_private->busy));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_start( MX_INTERVAL_TIMER *itimer,
			double timer_period_in_seconds )
{
	static const char fname[] = "mx_interval_timer_start()";

	MX_VXWORKS_WATCHDOG_PRIVATE *vxworks_wd_private;
	int delay_count_in_ticks;
	STATUS vx_status;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vxworks_wd_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	itimer->timer_period = timer_period_in_seconds;

	/* Start the watchdog timer. */

	delay_count_in_ticks =
		mx_round( sysClkRateGet() * timer_period_in_seconds );

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: starting timer for %f seconds (%d ticks)",
		fname, timer_period_in_seconds, delay_count_in_ticks ));
#endif
	/* FIXME!!!!!!!
	 * Casting a pointer to an int is a planetary extinction event.
	 */

	vx_status = wdStart( vxworks_wd_private->watchdog_id,
				delay_count_in_ticks,
				(FUNCPTR) mxp_interval_timer_callback,
				(int) vxworks_wd_private );

	if ( vx_status != OK ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to start interval timer %p failed.", itimer );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_stop( MX_INTERVAL_TIMER *itimer, double *seconds_left )
{
	static const char fname[] = "mx_interval_timer_stop()";

	MX_VXWORKS_WATCHDOG_PRIVATE *vxworks_wd_private;
	STATUS vx_status;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vxworks_wd_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Stop the timer. */

	vx_status = wdCancel( vxworks_wd_private->watchdog_id );

	vxworks_wd_private->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_interval_timer_read( MX_INTERVAL_TIMER *itimer,
			double *seconds_till_expiration )
{
	static const char fname[] = "mx_interval_timer_read()";

	MX_VXWORKS_WATCHDOG_PRIVATE *vxworks_wd_private;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vxworks_wd_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( seconds_till_expiration != (double *) NULL ) {
		*seconds_till_expiration = 0.0;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: busy = %d", fname, (int) vxworks_wd_private->busy));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/************************ BSD style setitimer() timers ***********************/

#elif ( defined( OS_UNIX ) && ( HAVE_POSIX_TIMERS == FALSE ) ) \
	|| defined( OS_DJGPP ) || defined( OS_MINIX )

/* WARNING: BSD setitimer() timers should only be used as a last resort,
 *          since they have some significant limitations in their
 *          functionality.
 *
 *          1.  There can only be one setitimer() based timer in a given
 *              process.
 *          2.  They are generally based on SIGALRM signals.  SIGALRM tends
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

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: itimer = %p", fname, itimer));
#endif

	if ( itimer == (MX_INTERVAL_TIMER *) NULL ) {
		mx_warning(
		    "%s was invoked although no interval timer was started.",
			fname );

		return;
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: itimer->callback_function = %p",
		fname, itimer->callback_function));
#endif

	if ( itimer->timer_type == MXIT_ONE_SHOT_TIMER ) {
		RELEASE_SETITIMER_INTERVAL_TIMER;
	}

	if ( itimer->callback_function != NULL ) {
		itimer->callback_function( itimer, itimer->callback_args );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				MX_INTERVAL_TIMER_CALLBACK *callback_function,
				void *callback_args )
{
	static const char fname[] = "mx_interval_timer_create()";

	MX_SETITIMER_PRIVATE *setitimer_private;
	mx_status_type mx_status;

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for BSD setitimer timers.", fname));
	MX_DEBUG(-2,("%s: timer_type = %d", fname, timer_type));
	MX_DEBUG(-2,("%s: callback_function = %p", fname, callback_function));
	MX_DEBUG(-2,("%s: callback_args = %p", fname, callback_args));
#endif

	if ( mx_setitimer_mutex == NULL ) {
		mx_status = mx_mutex_create( &mx_setitimer_mutex );

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

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: *itimer = %p", fname, *itimer));
#endif

	setitimer_private = (MX_SETITIMER_PRIVATE *)
				malloc( sizeof(MX_SETITIMER_PRIVATE) );

	if ( setitimer_private == (MX_SETITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_SETITIMER_PRIVATE structure." );
	}

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: setitimer_private = %p", fname, setitimer_private));
#endif

	(*itimer)->timer_type = timer_type;
	(*itimer)->timer_period = -1.0;
	(*itimer)->num_overruns = 0;
	(*itimer)->callback_function = callback_function;
	(*itimer)->callback_args = callback_args;
	(*itimer)->private_ptr = setitimer_private;

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

	setitimer_private = (MX_SETITIMER_PRIVATE *) itimer->private_ptr;

	if ( setitimer_private != (MX_SETITIMER_PRIVATE *) NULL ) {
		mx_free( setitimer_private );
	}

	mx_free( itimer );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, mx_bool_type *busy )
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

		itimer->timer_period = -1;
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

	setitimer_private = (MX_SETITIMER_PRIVATE *) itimer->private_ptr;

	if ( setitimer_private == (MX_SETITIMER_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SETITIMER_PRIVATE pointer for timer %p is NULL.",
			itimer );
	}

	itimer->timer_period = timer_period_in_seconds;

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

#if defined(OS_DJGPP)
		sa.sa_flags = 0;
#else
		sa.sa_flags = SA_RESETHAND;
#endif
	} else {
		itimer_value.it_interval.tv_sec  = timer_ulong_seconds;
		itimer_value.it_interval.tv_usec = timer_ulong_usec;

		sa.sa_flags = 0;
	}

	/* Attempt to restart system calls when possible. */

#if ! defined(OS_DJGPP)
	sa.sa_flags |= SA_RESTART;
#endif

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

	itimer->timer_period = -1;

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

	setitimer_private = (MX_SETITIMER_PRIVATE *) itimer->private_ptr;

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
