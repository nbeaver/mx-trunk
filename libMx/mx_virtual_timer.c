/*
 * Name:    mx_virtual_timer.c
 *
 * Purpose: Functions for using MX virtual interval timers.
 *
 * Author:  William Lavender
 *
 * Copyright 2006-2008, 2018, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_VIRTUAL_TIMER_DEBUG			FALSE

#define MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_time.h"
#include "mx_hrt.h"
#include "mx_mutex.h"
#include "mx_virtual_timer.h"

#define MXF_VTIMER_RELATIVE_TIME	1
#define MXF_VTIMER_ABSOLUTE_TIME	2

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
	struct mx_master_timer_event_struct *previous_event;
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
 *    The following functions from mx_show_event_list() to
 *    mx_delete_all_vtimer_events() must all be invoked with
 *    with the event_list's mutex already LOCKED!!!
 */

#if ( MX_VIRTUAL_TIMER_DEBUG || MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK )

static void
mx_show_event_list( const char *calling_fname,
		MX_MASTER_TIMER_EVENT_LIST *event_list )
{
	static const char fname[] = "mx_show_event_list()";

	MX_MASTER_TIMER_EVENT *current_event;
	unsigned long i;

	MX_DEBUG(-2,("%s invoked for event_list = %p by %s",
		fname, event_list, calling_fname));

	if ( event_list == (MX_MASTER_TIMER_EVENT_LIST *) NULL ) {
		return;
	}

	current_event = event_list->timer_event_list;

	MX_DEBUG(-2,("Event list %p", event_list));
	MX_DEBUG(-2,("------------------------------"));

	if ( current_event == (MX_MASTER_TIMER_EVENT *) NULL ) {

		MX_DEBUG(-2,("---> Event list is empty <---"));
		return;
	}

	i = 0;

	for (;;) {

		MX_DEBUG(-2,("Event %lu = %p", i, current_event));

		current_event = current_event->next_event;

		if ( current_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
			MX_DEBUG(-2,("---> End of event list <---"));
			return;
		}
		i++;
	}
}

#endif /* MX_VIRTUAL_TIMER_DEBUG */

static mx_status_type
mx_add_vtimer_event( MX_MASTER_TIMER_EVENT_LIST *event_list,
			MX_VIRTUAL_TIMER *vtimer,
			struct timespec timer_interval,
			int interval_type )
{
	static const char fname[] = "mx_add_vtimer_event()";

	MX_MASTER_TIMER_EVENT *new_event;
	MX_MASTER_TIMER_EVENT *previous_event, *current_event, *next_event;
	struct timespec current_time;
	int comparison;

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for event list %p and vtimer %p",
		fname, event_list, vtimer));
#endif

	if ( event_list == (MX_MASTER_TIMER_EVENT_LIST *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MASTER_TIMER_EVENT_LIST pointer passed was NULL." );
	}

	if ( vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIRTUAL_TIMER pointer passed was NULL." );
	}

	if ( (interval_type != MXF_VTIMER_ABSOLUTE_TIME)
	  && (interval_type != MXF_VTIMER_RELATIVE_TIME) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested interval type %d is not one of the allowed "
		"values of MXF_VTIMER_ABSOLUTE_TIME (%d) or "
		"MXF_VTIMER_RELATIVE_TIME (%d).", interval_type,
			MXF_VTIMER_ABSOLUTE_TIME, MXF_VTIMER_RELATIVE_TIME );
	}

	/* Create an event structure. */

	new_event = (MX_MASTER_TIMER_EVENT *)
			malloc( sizeof(MX_MASTER_TIMER_EVENT) );

	if ( new_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
   "Ran out of memory trying to allocate an MX_MASTER_TIMER_EVENT structure." );
   	}

	new_event->vtimer = vtimer;

	/* Compute the absolute time of the event we are adding. */

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: timer_interval = (%lu,%lu), interval_type = %d",
		fname, (unsigned long) timer_interval.tv_sec,
		(unsigned long) timer_interval.tv_nsec, interval_type));
#endif

	switch ( interval_type ) {
	case MXF_VTIMER_ABSOLUTE_TIME:
		new_event->expiration_time = timer_interval;
		break;
	case MXF_VTIMER_RELATIVE_TIME:
		current_time = mx_high_resolution_time();

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: current_time = (%lu,%lu)",
			fname, (unsigned long) current_time.tv_sec,
			(unsigned long) current_time.tv_nsec));
#endif

		new_event->expiration_time = mx_add_timespec_times(
					    current_time, timer_interval );
		break;
	}

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Created new_event = %p, expiration_time = (%lu,%lu)",
		fname, new_event,
		(unsigned long) new_event->expiration_time.tv_sec,
		(unsigned long) new_event->expiration_time.tv_nsec));
#endif

	/* If there are no events in the event list, add new_event as the
	 * only event in the list.
	 */

	current_event = event_list->timer_event_list;

	if ( current_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
		new_event->previous_event = NULL;
		new_event->next_event = NULL;

		event_list->timer_event_list = new_event;

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: event %p is the only event in the list.",
			fname, event_list->timer_event_list));

		mx_show_event_list( fname, event_list );
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* Find the correct place in the event list for this event. */

	for (;;) {

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: Top of for(;;) loop, current_event = %p",
			fname, current_event));
#endif

		comparison = mx_compare_timespec_times(
					new_event->expiration_time,
					current_event->expiration_time );

		/* If the new event is scheduled to occur before the current
		 * event, insert the new event here and then return.
		 */

		if ( comparison < 0 ) {
			if ( current_event == event_list->timer_event_list ) {
				new_event->previous_event = NULL;
				new_event->next_event = current_event;

				current_event->previous_event = new_event;
				event_list->timer_event_list = new_event;

#if MX_VIRTUAL_TIMER_DEBUG
				MX_DEBUG(-2,
			("%s: Adding event %p to the start of the list."
			" event_list->timer_event_list->next_event = %p",
				    fname, event_list->timer_event_list,
				    event_list->timer_event_list->next_event));
#endif
			} else {
				previous_event = current_event->previous_event;

				new_event->previous_event = previous_event;
				new_event->next_event = current_event;

#if MX_VIRTUAL_TIMER_DEBUG
				MX_DEBUG(-2,
			("%s: Adding event %p between events %p and %p",
			fname, new_event, previous_event, current_event));
#endif

				current_event->previous_event = new_event;
				previous_event->next_event = new_event;

#if MX_VIRTUAL_TIMER_DEBUG
				MX_DEBUG(-2,
			("%s: previous_event->next_event = %p, "
			"current_event->previous_event = %p",
				fname, previous_event->next_event,
				current_event->previous_event));
#endif
			}
			return MX_SUCCESSFUL_RESULT;
		}

		/* Proceed on to the next event in the list. */

		next_event = current_event->next_event;

		/* If we find that we are at the end of the list, append
		 * the new event here.
		 */

		if ( next_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
			new_event->next_event = NULL;
			new_event->previous_event = current_event;
			current_event->next_event = new_event;

#if MX_VIRTUAL_TIMER_DEBUG
			MX_DEBUG(-2,
			("%s: Appending event %p at the end of the list.  "
			"current_event->next_event = %p, "
			"new_event->previous_event = %p",
				fname, new_event,
				current_event->next_event,
				new_event->previous_event));

			mx_show_event_list( fname, event_list );
#endif

			return MX_SUCCESSFUL_RESULT;
		}

		/* Otherwise, we must keep going. */

		current_event = next_event;
	}

#if defined(OS_HPUX) && defined(__hppa)
	return MX_SUCCESSFUL_RESULT;
#endif
}

static mx_status_type
mx_get_next_vtimer_event( MX_MASTER_TIMER_EVENT_LIST *event_list,
			MX_VIRTUAL_TIMER *vtimer,
			MX_MASTER_TIMER_EVENT *current_vtimer_event,
			MX_MASTER_TIMER_EVENT **next_vtimer_event )
{
	static const char fname[] = "mx_get_next_vtimer_event()";

	MX_MASTER_TIMER_EVENT *current_event;

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for event list = %p", fname, event_list));
	MX_DEBUG(-2,("%s: vtimer = %p, current_vtimer_event = %p",
		fname, vtimer, current_vtimer_event));
#endif

	if ( vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIRTUAL_TIMER pointer passed was NULL." );
	}

	if ( next_vtimer_event == (MX_MASTER_TIMER_EVENT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The next_vtimer_event pointer passed was NULL." );
	}

	if ( current_vtimer_event != (MX_MASTER_TIMER_EVENT *) NULL ) {
		current_event = current_vtimer_event->next_event;
	} else {
		/* Specify the first event in the event list. */

		if ( event_list == (MX_MASTER_TIMER_EVENT_LIST *) NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The event_list and the current_vtimer_event pointers "
			"passed to this function cannot _both_ be NULL." );
		}

		current_event = event_list->timer_event_list;

		if ( current_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
			/* This event list does not have any events right now.*/

			*next_vtimer_event = NULL;

#if MX_VIRTUAL_TIMER_DEBUG
			MX_DEBUG(-2,
			("%s: No events in this event list.", fname));
#endif

			return MX_SUCCESSFUL_RESULT;
		}
	}

	for (;;) {

		if ( current_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
			/* This event list has no more events in it. */

			*next_vtimer_event = NULL;

#if MX_VIRTUAL_TIMER_DEBUG
			MX_DEBUG(-2,
			("%s: No more events in this event list.", fname));
#endif

			return MX_SUCCESSFUL_RESULT;
		}

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,
		("%s: current_event = %p, current_event->vtimer = %p",
			fname, current_event, current_event->vtimer));
#endif

		if ( current_event->vtimer == vtimer ) {
			/* We have a match, so return this event. */

			*next_vtimer_event = current_event;

#if MX_VIRTUAL_TIMER_DEBUG
			MX_DEBUG(-2,("%s: FOUND ---> next_vtimer_event = %p",
				fname, *next_vtimer_event));
#endif

			return MX_SUCCESSFUL_RESULT;
		}

		current_event = current_event->next_event;
	}

#if defined(OS_HPUX) && defined(__hppa)
	return MX_SUCCESSFUL_RESULT;
#endif
}

static mx_status_type
mx_delete_vtimer_event( MX_MASTER_TIMER_EVENT_LIST *event_list,
			MX_MASTER_TIMER_EVENT *current_event )
{
	static const char fname[] = "mx_delete_vtimer_event()";

	MX_MASTER_TIMER_EVENT *previous_event, *next_event;

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for event list %p and event %p",
		fname, event_list, current_event));
#endif

	if ( current_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MASTER_TIMER_EVENT pointer passed was NULL." );
	}

	previous_event = current_event->previous_event;

	next_event = current_event->next_event;

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: previous_event = %p, next_event = %p",
		fname, previous_event, next_event));
#endif

	if ( current_event == event_list->timer_event_list ) {
		event_list->timer_event_list = next_event;

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: current_event = %p is at the start of the "
		"event list.  Set event list start to next event = %p",
		fname, current_event, event_list->timer_event_list));
#endif
	}

	if ( next_event != (MX_MASTER_TIMER_EVENT *) NULL ) {
		next_event->previous_event = previous_event;

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: Set next_event->previous_event to %p",
			fname, next_event->previous_event));
#endif
	}

	if ( previous_event != (MX_MASTER_TIMER_EVENT *) NULL ) {
		previous_event->next_event = next_event;

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: Set previous_event->next_event to %p",
			fname, previous_event->next_event));
#endif
	}

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: About to free event %p", fname, current_event));
#endif

	mx_free( current_event );

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s: Finished freeing event", fname));

	mx_show_event_list( fname, event_list );
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_delete_all_vtimer_events( MX_MASTER_TIMER_EVENT_LIST *event_list,
				MX_VIRTUAL_TIMER *vtimer )
{
#if MX_VIRTUAL_TIMER_DEBUG
	static const char fname[] = "mx_delete_all_vtimer_events()";
#endif

	MX_MASTER_TIMER_EVENT *current_event, *next_event;
	unsigned long i;
	mx_status_type mx_status;

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("*****************************************************"));
	MX_DEBUG(-2,("%s invoked for event list %p and vtimer %p",
		fname, event_list, vtimer));

	mx_show_event_list( fname, event_list );
#endif

	/* Look for the first event in the linked list for this timer. */

	mx_status = mx_get_next_vtimer_event( event_list, vtimer,
						NULL, &next_event );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( next_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
		/* There are no outstanding events for this timer,
		 * so just return.
		 */

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,
		("%s: No events to delete for this vtimer.", fname));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* Loop through the event list looking for events for this vtimer. */

	i = 0;

	while ( next_event != (MX_MASTER_TIMER_EVENT *) NULL ) {

		current_event = next_event;

		/* Find the next event. */

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: finding next event for event #%lu = %p",
			fname, i, current_event));
#endif

		mx_status = mx_get_next_vtimer_event( event_list, vtimer,
						current_event, &next_event );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Delete the current event from the event list. */

#if MX_VIRTUAL_TIMER_DEBUG
		MX_DEBUG(-2,("%s: deleting event #%lu = %p",
			fname, i, current_event));
#endif

		mx_status = mx_delete_vtimer_event( event_list, current_event );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		i++;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static void
mx_master_timer_callback_function( MX_INTERVAL_TIMER *itimer, void *args )
{
	static const char fname[] = "mx_master_timer_callback_function()";

	MX_MASTER_TIMER_EVENT_LIST *event_list;
	MX_MASTER_TIMER_EVENT *current_event;
	MX_VIRTUAL_TIMER *vtimer;
	struct timespec current_time, expiration_time, new_expiration_time;
	int comparison;
	long lock_status;
	mx_status_type mx_status;

#if MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK
	MX_DEBUG(-2,("vvv------------------------------------------------vvv"));
	MX_DEBUG(-2,("%s invoked for itimer = %p, args = %p",
		fname, itimer, args));
#endif

	event_list = (MX_MASTER_TIMER_EVENT_LIST *) args;

#if MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK
	MX_DEBUG(-2,("%s: num_timer_events = %lu",
		fname, event_list->num_timer_events));
#endif

	/* Attempt to lock the mutex for the timer event list. */

	lock_status = mx_mutex_trylock( event_list->mutex );

	switch( lock_status ) {
	case MXE_SUCCESS:
		/* We have acquired the mutex. */

#if MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK
		MX_DEBUG(-2,("%s: Locked the mutex for timer event list %p",
				fname, event_list));
#endif
		break;
	case MXE_NOT_AVAILABLE:
		/* Somebody else has the mutex.  We will try again later. */

#if MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK
		MX_DEBUG(-2,("%s: timer event list %p mutex is unavailable.",
				fname, event_list ));
#endif
		return;
	default:
		/* Something bad happened, so we abort noisily. */

		(void) mx_error( lock_status, fname,
		"An attempt to lock the mutex for timer event list %p failed.",
			event_list );
		return;
	}

#if MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK
	MX_DEBUG(-2,("%s: Entering event processing loop.", fname));

	mx_show_event_list( fname, event_list );
#endif

	/* Process and delete events in the event list that are before
	 * or at the current time.
	 */

	current_time = mx_high_resolution_time();

	for (;;) {
		current_event = event_list->timer_event_list;

#if MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK
		MX_DEBUG(-2,("%s: current_event = %p", fname, current_event));
#endif

		if ( current_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
			/* We have reached the end of the event list. */

			break;	/* Exit the for(;;) loop. */
		}

		expiration_time = current_event->expiration_time;

		comparison = mx_compare_timespec_times( current_time,
							expiration_time );

#if MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK
		MX_DEBUG(-2,("%s: current_time = (%lu,%lu), "
			"expiration_time = (%lu,%lu), comparison = %d",
			fname, current_time.tv_sec, current_time.tv_nsec,
			expiration_time.tv_sec, expiration_time.tv_nsec,
			comparison));
#endif

		if ( comparison < 0 ) {
			/* If this event is scheduled to happen in the future,
			 * stop processing the event list at this point.
			 */

			break;	/* Exit the for(;;) loop. */
		}

		/* If the virtual timer is a periodic timer, then schedule
		 * the next event.
		 */

		vtimer = current_event->vtimer;

		if ( vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
			(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_VIRTUAL_TIMER pointer for event %p is NULL.",
				current_event );

			break;	/* Exit the for(;;) loop. */
		}

		if ( vtimer->timer_type == MXIT_PERIODIC_TIMER ) {

			/* Compute the new expiration time relative to the
			 * old expiration time.  We do NOT do it relative to
			 * the current time.
			 */

			new_expiration_time = mx_add_timespec_times(
						expiration_time,
						vtimer->timer_period );

			mx_status = mx_add_vtimer_event( event_list,
						vtimer,
						new_expiration_time,
						MXF_VTIMER_ABSOLUTE_TIME );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;	/* Exit the for(;;) loop. */
			}

#if MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK
			MX_DEBUG(-2,
		    ("%s: periodic timer %p, new_expiration_time = (%lu,%lu)",
				fname, vtimer,
				new_expiration_time.tv_sec,
				new_expiration_time.tv_nsec));
#endif
		}
		
		/* Invoke the callback function if there is one. */

		vtimer = current_event->vtimer;

		if ( vtimer != (MX_VIRTUAL_TIMER *) NULL ) {

			if ( vtimer->callback_function != NULL ) {

				(vtimer->callback_function)( vtimer,
							vtimer->callback_args );
			}
		}

		/* We are done with this event, so delete it
		 * from the event list.
		 */

		mx_status = mx_delete_vtimer_event( event_list, current_event );

		if ( mx_status.code != MXE_SUCCESS ) {
			break;	/* Exit the for(;;) loop. */
		}
	}

	/* We are done, so unlock the event list before returning. */

	UNLOCK_EVENT_LIST(event_list);

#if MX_VIRTUAL_TIMER_DEBUG_MASTER_CALLBACK
	MX_DEBUG(-2,("%s: Unlocked the mutex for timer event list %p",
			fname, event_list ));
	MX_DEBUG(-2,("^^^------------------------------------------------^^^"));
#endif

	return;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_virtual_timer_create_master( MX_INTERVAL_TIMER **master_timer,
				double master_timer_period_in_seconds )
{
	static const char fname[] = "mx_virtual_timer_create_master()";

	MX_MASTER_TIMER_EVENT_LIST *event_list;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

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

	MX_DEBUG( 2,("%s invoked.", fname));

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

	MX_DEBUG( 2,("%s invoked.", fname));

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
	(*vtimer)->private_ptr = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_destroy( MX_VIRTUAL_TIMER *vtimer )
{
	static const char fname[] = "mx_virtual_timer_destroy()";

	mx_status_type mx_status;

	if ( vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VIRTUAL_TIMER_POINTER passed was NULL." );
	}

	MX_DEBUG( 2,("%s invoked for vtimer %p.", fname, vtimer));

	mx_status = mx_virtual_timer_stop( vtimer, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_free( vtimer );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_is_busy( MX_VIRTUAL_TIMER *vtimer,
			mx_bool_type *busy )
{
	static const char fname[] = "mx_virtual_timer_is_busy()";

	MX_INTERVAL_TIMER *master_timer;
	MX_MASTER_TIMER_EVENT_LIST *event_list;
	MX_MASTER_TIMER_EVENT *next_event;
	long lock_status;
	mx_status_type mx_status;

	master_timer = NULL;
	event_list = NULL;

	mx_status = mx_virtual_timer_get_pointers( vtimer,
					&master_timer, &event_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for vtimer %p.", fname, vtimer));

	/* Get exclusive access to the master timer event list. */

	lock_status = mx_mutex_lock( event_list->mutex );

	if ( lock_status != MXE_SUCCESS ) {
		/* The mutex is not locked, so just return an error. */

		return mx_error( lock_status, fname,
			"Unable to lock the mutex for the event list of "
			"master timer %p used by virtual timer %p",
			master_timer, vtimer );
	}

	/* See if there are any scheduled events for this timer. */

	mx_status = mx_get_next_vtimer_event( event_list, vtimer,
						NULL, &next_event );

	if ( mx_status.code != MXE_SUCCESS ) {
		UNLOCK_EVENT_LIST( event_list );
		return mx_status;
	}

	/* If a next_event structure is returned, then the timer is still
	 * busy.  Otherwise, it is not busy.
	 */

	if ( next_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
		*busy = FALSE;
	} else {
		*busy = TRUE;
	}

	UNLOCK_EVENT_LIST( event_list );

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

	master_timer = NULL;
	event_list = NULL;

	mx_status = mx_virtual_timer_get_pointers( vtimer,
					&master_timer, &event_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for vtimer %p, timer_period_in_seconds = %g",
		fname, vtimer, timer_period_in_seconds));
#endif

	if ( timer_period_in_seconds > 0.0 ) {

		/* Save the timer period. */

		vtimer->timer_period = mx_convert_seconds_to_timespec_time(
						timer_period_in_seconds );

	} else {
		/* If the timer period in seconds is zero or a negative
		 * number, then we assume that we are going to reuse the
		 * previous timer period.  However, if the virtual timer
		 * has never been started, the internal timer period will
		 * be zero, which is an error that we must check for.
		 */

		if ( ( vtimer->timer_period.tv_sec == 0 )
		  && ( vtimer->timer_period.tv_nsec == 0 ) )
		{
			if ( timer_period_in_seconds == 0.0 ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Starting a new virtual timer with a "
				"timer period of 0 seconds is not supported." );
			} else {
				return mx_error( MXE_INITIALIZATION_ERROR,fname,
				"Restarting virtual timer %p cannot be done "
				"until after the virtual timer has had at "
				"one normal start.", vtimer );
			}
		}

		/* If we get here, then the timer will be restarted with
		 * the preexisting contents of vtimer->timer_period.
		 */
	}

	/* We need to add a timer event to the timer event list managed
	 * by the master timer that this virtual timer uses.
	 */

	/* Get exclusive access to the master timer event list. */

	lock_status = mx_mutex_lock( event_list->mutex );

	if ( lock_status != MXE_SUCCESS ) {
		/* The mutex is not locked, so just return an error. */

		return mx_error( lock_status, fname,
			"Unable to lock the mutex for the event list of "
			"master timer %p used by virtual timer %p",
			master_timer, vtimer );
	}

#if MX_VIRTUAL_TIMER_DEBUG
	if ( timer_period_in_seconds > 0.0 ) {
		MX_DEBUG(-2,
		("%s: Adding %g second (%ld,%ld) timer event for vtimer %p",
			fname, timer_period_in_seconds,
			(long) vtimer->timer_period.tv_sec,
			vtimer->timer_period.tv_nsec,
			vtimer ));
	} else {
		timer_period_in_seconds = mx_convert_timespec_time_to_seconds(
							vtimer->timer_period );

		MX_DEBUG(-2,
	    ("%s: Resubmitting %g second (%ld,%ld) timer event for vtimer %p",
			fname, timer_period_in_seconds,
			(long) vtimer->timer_period.tv_sec,
			vtimer->timer_period.tv_nsec,
			vtimer ));
	}
#endif

	/* Get rid of any old events due to this virtual timer. */

	mx_status = mx_delete_all_vtimer_events( event_list, vtimer );

	if ( mx_status.code != MXE_SUCCESS ) {
		UNLOCK_EVENT_LIST( event_list );
		return mx_status;
	}

	/* Add a list entry for the next virtual timer event. */

	mx_status = mx_add_vtimer_event( event_list,
					vtimer, vtimer->timer_period,
					MXF_VTIMER_RELATIVE_TIME );

	if ( mx_status.code != MXE_SUCCESS ) {
		UNLOCK_EVENT_LIST( event_list );
		return mx_status;
	}

	UNLOCK_EVENT_LIST( event_list );

#if MX_VIRTUAL_TIMER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_stop( MX_VIRTUAL_TIMER *vtimer,
			double *seconds_left )
{
	static const char fname[] = "mx_virtual_timer_stop()";

	MX_MASTER_TIMER_EVENT_LIST *event_list;
	mx_status_type mx_status;

	event_list = NULL;

	mx_status = mx_virtual_timer_get_pointers( vtimer,
					NULL, &event_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for vtimer %p.", fname, vtimer));

	mx_status = mx_delete_all_vtimer_events( event_list, vtimer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_virtual_timer_read( MX_VIRTUAL_TIMER *vtimer,
			double *seconds_till_expiration )
{
	static const char fname[] = "mx_virtual_timer_read()";

	MX_INTERVAL_TIMER *master_timer;
	MX_MASTER_TIMER_EVENT_LIST *event_list;
	MX_MASTER_TIMER_EVENT *next_event;
	struct timespec current_time, expiration_time, time_difference;
	int comparison;
	long lock_status;
	mx_status_type mx_status;

	master_timer = NULL;
	event_list = NULL;

	mx_status = mx_virtual_timer_get_pointers( vtimer,
					&master_timer, &event_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for vtimer %p.", fname, vtimer));

	/* Get exclusive access to the master timer event list. */

	lock_status = mx_mutex_lock( event_list->mutex );

	if ( lock_status != MXE_SUCCESS ) {
		/* The mutex is not locked, so just return an error. */

		return mx_error( lock_status, fname,
			"Unable to lock the mutex for the event list of "
			"master timer %p used by virtual timer %p",
			master_timer, vtimer );
	}

	/* See if there are any scheduled events for this timer. */

	mx_status = mx_get_next_vtimer_event( event_list, vtimer,
						NULL, &next_event );

	if ( mx_status.code != MXE_SUCCESS ) {
		UNLOCK_EVENT_LIST( event_list );
		return mx_status;
	}

	/* If a next_event structure is returned, then the timer is still
	 * busy, so we must extract the number of seconds until the next
	 * expiration time.
	 */

	if ( next_event == (MX_MASTER_TIMER_EVENT *) NULL ) {
		*seconds_till_expiration = 0.0;
	} else {
		current_time = mx_high_resolution_time();

		expiration_time = next_event->expiration_time;

		comparison = mx_compare_timespec_times( current_time,
							expiration_time );

		if ( comparison >= 0 ) {
			*seconds_till_expiration = 0.0;
		} else {
			time_difference = mx_subtract_timespec_times(
					expiration_time, current_time );

			*seconds_till_expiration =
			  mx_convert_timespec_time_to_seconds(time_difference);
		}
	}

	UNLOCK_EVENT_LIST( event_list );

	return MX_SUCCESSFUL_RESULT;
}

