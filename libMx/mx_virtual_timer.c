/*
 * Name:    mx_virtual_timer.c
 *
 * Purpose: Functions for using MX virtual interval timers.
 *
 * Author:  William Lavender
 *
 * WARNING: NOT YET FINISHED! (as of March 27, 2006)
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_VIRTUAL_TIMER_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_hrt.h"
#include "mx_mutex.h"
#include "mx_virtual_timer.h"

#define UNLOCK_EVENT_LIST(x) \
		do {							\
			long temp_status;				\
			temp_status = mx_mutex_unlock( (x)->mutex );	\
			if ( temp_status != MXE_SUCCESS ) {		\
				(void) mx_error( temp_status, fname,	\
    "An attempt to unlock the mutex for timer event list %p failed.",	\
					(x) );				\
			}						\
		} while (0)

/* The following are typedefs intended for the private usage of this file. */

struct mx_master_timer_event_struct {
	MX_VIRTUAL_TIMER *vtimer;
	struct timespec expiration_time;
	struct mx_master_timer_event_struct *next_event;
};

typedef struct mx_master_timer_event_struct MX_MASTER_TIMER_EVENT;

typedef struct {
	MX_MUTEX *mutex;
	unsigned long num_timer_events;
	MX_MASTER_TIMER_EVENT *timer_event_list;
} MX_MASTER_TIMER_EVENT_LIST;

/*--------------------------------------------------------------------------*/

static mx_status_type
mx_virtual_timer_get_pointers( MX_VIRTUAL_TIMER *vtimer,
				MX_INTERVAL_TIMER **master_timer,
				MX_MASTER_TIMER_EVENT_LIST **event_list,
				const char *calling_fname )

{
	static const char fname[] = "mx_virtual_timer_get_pointers()";

	MX_INTERVAL_TIMER *master_timer_ptr;

	if ( vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIRTUAL_TIMER_POINTER passed by '%s' was NULL.",
			calling_fname );
	}

	master_timer_ptr = vtimer->master_timer;

	if ( master_timer_ptr == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The master_timer pointer for virtual timer %p is NULL.",
			vtimer );
	}

	if ( master_timer != (MX_INTERVAL_TIMER **) NULL ) {
		*master_timer = master_timer_ptr;
	}

	if ( event_list != (MX_MASTER_TIMER_EVENT_LIST **) NULL ) {

		*event_list = master_timer_ptr->callback_args;

		if ( (*event_list) == (MX_MASTER_TIMER_EVENT_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The MX_MASTER_TIMER_EVENT_LIST pointer for "
				"master interval timer %p used by "
				"virtual timer %p is NULL.",
				master_timer_ptr, vtimer );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

/* WARNING, WARNING, WARNING:
 *
 *    The following functions mx_master_timer_event_list_add_vtimer_event()
 *    and mx_master_timer_event_list_delete_vtimer_events() must be called
 *    with the event list mutex already LOCKED!!!!!!
 */

static mx_status_type
mx_master_timer_event_list_add_vtimer_event(
					MX_MASTER_TIMER_EVENT_LIST *event_list,
					MX_VIRTUAL_TIMER *vtimer )
{
	static const char fname[] =
		"mx_master_timer_event_list_add_vtimer_event()";

	MX_DEBUG(-2,("%s invoked for event list %p and vtimer %p",
		fname, event_list, vtimer));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_master_timer_event_list_delete_vtimer_events(
					MX_MASTER_TIMER_EVENT_LIST *event_list,
					MX_VIRTUAL_TIMER *vtimer )
{
	static const char fname[] =
		"mx_master_timer_event_list_delete_vtimer_events()";

	MX_DEBUG(-2,("%s invoked for event list %p and vtimer %p",
		fname, event_list, vtimer));

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static void
mx_master_timer_callback_function( MX_INTERVAL_TIMER *itimer, void *args )
{
	static const char fname[] = "mx_master_timer_callback_function()";

	MX_MASTER_TIMER_EVENT_LIST *event_list;
	long lock_status;

	MX_DEBUG(-2,("%s: itimer = %p", fname, itimer));
	MX_DEBUG(-2,("%s: args = %p", fname, args));

	event_list = (MX_MASTER_TIMER_EVENT_LIST *) args;

	MX_DEBUG(-2,("%s: num_timer_events = %lu",
		fname, event_list->num_timer_events));

	/* Attempt to lock the mutex for the timer event list. */

	lock_status = mx_mutex_trylock( event_list->mutex );

	switch( lock_status ) {
	case MXE_SUCCESS:
		/* We have acquired the mutex. */

		MX_DEBUG(-2,("%s: Locked the mutex for timer event list %p",
				fname, event_list));
		break;
	case MXE_NOT_AVAILABLE:
		/* Somebody else has the mutex.  We will try again later. */

		MX_DEBUG(-2,("%s: timer event list %p mutex is unavailable.",
				fname, event_list ));
		return;
	default:
		/* Something bad happened, so we abort noisily. */

		(void) mx_error( lock_status, fname,
		"An attempt to lock the mutex for timer event list %p failed.",
			event_list );
		return;
	}

	/* FIXME: Insert event list processing here. */

	/* We are done, so unlock the event list before returning. */

	UNLOCK_EVENT_LIST(event_list);

	MX_DEBUG(-2,("%s: Unlocked the mutex for timer event list %p",
			fname, event_list ));
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_virtual_timer_create_master( MX_INTERVAL_TIMER **master_timer,
				double master_timer_period_in_seconds )
{
	static const char fname[] = "mx_virtual_timer_create_master()";

	MX_MASTER_TIMER_EVENT_LIST *event_list;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( master_timer == (MX_INTERVAL_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	/* Create a list of future virtual timer events that are to be
	 * processed by the real interval timer.
	 */

	event_list = (MX_MASTER_TIMER_EVENT_LIST *)
				malloc( sizeof(MX_MASTER_TIMER_EVENT_LIST) );

	if ( event_list == (MX_MASTER_TIMER_EVENT_LIST *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"MX_MASTER_TIMER_EVENT_LIST structure." );
	}

	/* Initialize the list of future virtual timer events to be 0 and
	 * create a mutex to manage access to the virtual timer event list.
	 */

	event_list->num_timer_events = 0;
	event_list->timer_event_list = NULL;

	mx_status = mx_mutex_create( &(event_list->mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The virtual timers all depend on a single real interval timer
	 * that periodically looks at the outstanding list of virtual timer
	 * expiration times to see if any of the virtual timers have expired.
	 */

	mx_status = mx_interval_timer_create( master_timer,
					MXIT_PERIODIC_TIMER,
					mx_master_timer_callback_function,
					event_list );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_start( *master_timer,
					master_timer_period_in_seconds );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_virtual_timer_destroy_master( MX_INTERVAL_TIMER *master_timer )
{
	static const char fname[] = "mx_virtual_timer_destroy_master()";

	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( master_timer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	mx_status = mx_interval_timer_stop( master_timer, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_interval_timer_destroy( master_timer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_virtual_timer_create( MX_VIRTUAL_TIMER **vtimer,
			MX_INTERVAL_TIMER *master_timer,
			int timer_type,
			MX_VIRTUAL_TIMER_CALLBACK *callback_function,
			void *callback_args )
{
	static const char fname[] = "mx_virtual_timer_create()";

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( vtimer == (MX_VIRTUAL_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIRTUAL_TIMER pointer passed was NULL." );
	}

	if ( master_timer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The master_timer pointer passed was NULL." );
	}

	*vtimer = (MX_VIRTUAL_TIMER *) malloc( sizeof(MX_VIRTUAL_TIMER) );

	if ( *vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate an MX_VIRTUAL_TIMER structure." );
	}

	(*vtimer)->master_timer = master_timer;
	(*vtimer)->timer_type = timer_type;
	(*vtimer)->timer_period.tv_sec = 0;
	(*vtimer)->timer_period.tv_nsec = 0;
	(*vtimer)->num_overruns = 0;
	(*vtimer)->callback_function = callback_function;
	(*vtimer)->callback_args = callback_args;
	(*vtimer)->private = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_destroy( MX_VIRTUAL_TIMER *vtimer )
{
	static const char fname[] = "mx_virtual_timer_destroy()";

	if ( vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIRTUAL_TIMER_POINTER passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for vtimer %p.", fname, vtimer));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_is_busy( MX_VIRTUAL_TIMER *vtimer,
			mx_bool_type *busy )
{
	static const char fname[] = "mx_virtual_timer_is_busy()";

	if ( vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIRTUAL_TIMER_POINTER passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for vtimer %p.", fname, vtimer));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_start( MX_VIRTUAL_TIMER *vtimer,
			double timer_period_in_seconds )
{
	static const char fname[] = "mx_virtual_timer_start()";

	MX_INTERVAL_TIMER *master_timer;
	MX_MASTER_TIMER_EVENT_LIST *event_list;
	long lock_status;
	mx_status_type mx_status;

	mx_status = mx_virtual_timer_get_pointers( vtimer,
					&master_timer, &event_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for vtimer %p.", fname, vtimer));

	/* We need to add a timer event to the timer event list managed
	 * by the master timer that this virtual timer uses.
	 */

	/* Save the timer period. */

	vtimer->timer_period = mx_convert_seconds_to_high_resolution_time(
					timer_period_in_seconds );

	/* Get exclusive access to the master timer event list. */

	lock_status = mx_mutex_lock( event_list->mutex );

	if ( lock_status != MXE_SUCCESS ) {
		/* The mutex is not locked, so just return an error. */

		return mx_error( lock_status, fname,
			"Unable to lock the mutex for the event list of "
			"master timer %p used by virtual timer %p",
			master_timer, vtimer );
	}

	MX_DEBUG(-2,("%s: Adding %g second (%ld,%ld) timer event for vtimer %p",
		fname, timer_period_in_seconds,
		(long) vtimer->timer_period.tv_sec,
		vtimer->timer_period.tv_nsec,
		vtimer ));

	/* Get rid of any old events due to this virtual timer. */

	mx_status = mx_master_timer_event_list_delete_vtimer_events(
				event_list, vtimer );

	if ( mx_status.code != MXE_SUCCESS ) {
		UNLOCK_EVENT_LIST( event_list );
		return mx_status;
	}

	/* Add a list entry for the next virtual timer event. */

	mx_status = mx_master_timer_event_list_add_vtimer_event(
				event_list, vtimer );

	if ( mx_status.code != MXE_SUCCESS ) {
		UNLOCK_EVENT_LIST( event_list );
		return mx_status;
	}

	UNLOCK_EVENT_LIST( event_list );

	MX_DEBUG(-2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_stop( MX_VIRTUAL_TIMER *vtimer,
			double *seconds_left )
{
	static const char fname[] = "mx_virtual_timer_stop()";

	if ( vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIRTUAL_TIMER_POINTER passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for vtimer %p.", fname, vtimer));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_read( MX_VIRTUAL_TIMER *vtimer,
			double *seconds_till_expiration )
{
	static const char fname[] = "mx_virtual_timer_read()";

	MX_DEBUG(-2,("%s invoked for vtimer %p.", fname, vtimer));

	return MX_SUCCESSFUL_RESULT;
}

