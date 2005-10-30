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
 * Copyright 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_INTERVAL_TIMER_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_osdef.h"
#include "mx_unistd.h"
#include "mx_util.h"
#include "mx_types.h"
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

/********************************** OpenVMS *********************************/

#elif defined(OS_VMS)

#include <lib$routines.h>
#include <starlet.h>
#include <ssdef.h>
#include <libdef.h>

typedef struct {
	mx_uint32_type event_flag;
	mx_uint32_type timer_period[2];
	mx_uint32_type finish_time[2];
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

	*itimer_private = (MX_VMS_ITIMER_PRIVATE *) itimer->private;

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

	vms_itimer_private = (MX_VMS_ITIMER_PRIVATE *) itimer->private;

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

		/* If we get here, the event flag has been set, which
		 * makes it time to invoke the callback function.
		 */

#if MX_INTERVAL_TIMER_DEBUG
		MX_DEBUG(-2,
		("%s: Event flag %lu for timer is set.  Invoking callback.",
		    fname, (unsigned long) vms_itimer_private->event_flag ));
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
	mx_uint32_type event_flag_number;
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
	(*itimer)->private = vms_itimer_private;

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

	/* We use the convention that the timer is running if the
	 * event flag is not set.  Therefore, we must now set the
	 * event flag since the timer is not running yet.
	 */

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Calling sys$setef() for event flag %lu.",
		fname, event_flag_number));
#endif

	vms_status = sys$setef( event_flag_number );

	switch( vms_status ) {
	case SS$_WASCLR:
	case SS$_WASSET:
		break;
	case SS$_ILLEFC:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"%d is not a legal event flag number.",
			event_flag_number );
		break;
	case SS$_UNASEFC:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"This process is not associated with the cluster "
		"containing event flag %d.", event_flag_number );
		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to set VMS event flag %d failed with "
		"a VMS error code of %d.", vms_status );
		break;
	}

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

	mx_status = mx_interval_timer_stop( itimer, &seconds_left );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_thread_stop( vms_itimer_private->event_flag_thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	mx_free( vms_itimer_private );

	mx_free( itimer );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, int *busy )
{
	static const char fname[] = "mx_interval_timer_is_busy()";

	MX_VMS_ITIMER_PRIVATE *vms_itimer_private;
	mx_uint32_type event_flag_cluster;
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

#if MX_INTERVAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Calling sys$readef() for event flag %lu.",
		fname, vms_itimer_private->event_flag));
#endif

	vms_status = sys$readef( vms_itimer_private->event_flag,
					&event_flag_cluster );

	/* If the event flag is set, the timer is not running. */

	switch( vms_status ) {
	case SS$_WASCLR:
		*busy = TRUE;
		break;
	case SS$_WASSET:
		*busy = FALSE;
		break;
	case SS$_ACCVIO:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The specified event flag cluster pointer %p cannot be "
		"written to by the caller.", &event_flag_cluster );
		break;
	case SS$_ILLEFC:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"%lu is not a legal event flag number.",
			vms_itimer_private->event_flag );
		break;
	case SS$_UNASEFC:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"This process is not associated with the cluster containing "
		"the specified event flag %lu.",
			vms_itimer_private->event_flag );
		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to read VMS event flag %lu failed with "
		"VMS status code %d.", vms_itimer_private->event_flag,
			vms_status );
		break;
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
	mx_uint32_type current_time[2];
	mx_uint32_type new_low_order;
	int vms_status;
	mx_status_type mx_status;

	mx_status = mx_interval_timer_get_pointers( itimer,
					&vms_itimer_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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
	mx_uint32_type current_time[2];
	mx_uint32_type H1, L1, H2, L2;
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

#elif defined(_POSIX_TIMERS) || defined(OS_IRIX) || defined(OS_VXWORKS)

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

#if defined(OS_SOLARIS) || defined(OS_IRIX) || defined(OS_VXWORKS)
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

#if (!defined(_POSIX_REALTIME_SIGNALS)) || ( _POSIX_REALTIME_SIGNALS < 0 )

#  if !defined(OS_VXWORKS)
#    error SIGEV_SIGNAL can only be used if Posix realtime signals are available.  If realtime signals are not available, you must use the setitimer-based timer mechanism.
#  endif

#endif

#include "mx_signal.h"

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

	posix_itimer_private = (MX_POSIX_ITIMER_PRIVATE *) itimer->private;

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

#if !defined(OS_VXWORKS)

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
#endif

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

#if !defined(OS_VXWORKS)

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
#endif

			/* End the thread by returning to the caller. */

#if MX_INTERVAL_TIMER_DEBUG
			MX_DEBUG(-2,("%s exiting.", fname));
#endif
	
			return MX_SUCCESSFUL_RESULT;
		}
	}

	MX_DEBUG(-2,("%s: Should not get here.", fname));
	
	return MX_SUCCESSFUL_RESULT;
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

#elif defined( OS_MACOSX ) || defined( OS_BSD ) || defined( OS_DJGPP )

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
