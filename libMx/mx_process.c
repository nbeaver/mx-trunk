/*
 * Name:    mx_process.c
 *
 * Purpose: MX event processing functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PROCESS_DEBUG		FALSE

#define PROCESS_DEBUG_TIMING	FALSE

#define PROCESS_DEBUG_QUEUEING	FALSE

#include <stdio.h>
#include <stdlib.h>

#ifndef OS_WIN32
#include <sys/times.h>
#endif

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_clock.h"
#include "mx_hrt_debug.h"

#include "mx_process.h"
#include "pr_handlers.h"

typedef struct {
	long mx_superclass;
	long mx_class;
	mx_status_type (*setup_fn) ( MX_RECORD * );
} MX_PROCESS_FUNCTION_SETUP;

static MX_PROCESS_FUNCTION_SETUP mx_process_function_setup_array[] = {

{ MXR_DEVICE, MXC_MOTOR, mx_setup_motor_process_functions },
{ MXR_DEVICE, MXC_SCALER, mx_setup_scaler_process_functions },
{ MXR_DEVICE, MXC_TIMER, mx_setup_timer_process_functions },
{ MXR_DEVICE, MXC_AMPLIFIER, mx_setup_amplifier_process_functions },
{ MXR_DEVICE, MXC_RELAY, mx_setup_relay_process_functions },
{ MXR_DEVICE, MXC_MULTICHANNEL_ANALYZER, mx_setup_mca_process_functions },
{ MXR_DEVICE, MXC_MULTICHANNEL_ENCODER, mx_setup_mce_process_functions },
{ MXR_DEVICE, MXC_MULTICHANNEL_SCALER, mx_setup_mcs_process_functions },
{ MXR_DEVICE, MXC_ANALOG_INPUT, mx_setup_analog_input_process_functions },
{ MXR_DEVICE, MXC_ANALOG_OUTPUT, mx_setup_analog_output_process_functions },
{ MXR_DEVICE, MXC_DIGITAL_INPUT, mx_setup_digital_input_process_functions },
{ MXR_DEVICE, MXC_DIGITAL_OUTPUT, mx_setup_digital_output_process_functions},
{ MXR_DEVICE, MXC_PULSE_GENERATOR, mx_setup_pulser_process_functions},
{ MXR_DEVICE, MXC_AUTOSCALE, mx_setup_autoscale_process_functions },
{ MXR_DEVICE, MXC_SINGLE_CHANNEL_ANALYZER, mx_setup_sca_process_functions },
{ MXR_DEVICE, MXC_CCD, mx_setup_ccd_process_functions },
{ MXR_DEVICE, MXC_SAMPLE_CHANGER, mx_setup_sample_changer_process_functions},
{ MXR_DEVICE, MXC_PAN_TILT_ZOOM, mx_setup_ptz_process_functions},
{ MXR_INTERFACE, MXI_RS232, mx_setup_rs232_process_functions },
{ MXR_INTERFACE, MXI_GPIB, mx_setup_gpib_process_functions },
{ MXR_VARIABLE, MXC_ANY, mx_setup_variable_process_functions },
{ MXR_LIST_HEAD, MXC_ANY, mx_setup_list_head_process_functions },
{ MXR_SCAN, MXC_ANY, NULL },
{ MXR_SERVER, MXC_ANY, NULL },
{ MXR_SPECIAL, MXC_ANY, NULL },
};

static int mx_num_process_function_setup_elements =
			sizeof( mx_process_function_setup_array )
			/ sizeof( mx_process_function_setup_array[0] );

MX_EXPORT mx_status_type
mx_initialize_database_processing( MX_RECORD *mx_record_list )
{
	MX_RECORD *current_record, *list_head_record;
	mx_status_type mx_status;

	list_head_record = mx_record_list->list_head;

	current_record = list_head_record;

	do {
		mx_status = mx_initialize_record_processing( current_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		current_record = current_record->next_record;

	} while ( current_record != list_head_record );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_initialize_record_processing( MX_RECORD *record )
{
	static const char fname[] = "mx_initialize_record_processing()";

	MX_RECORD_FUNCTION_LIST *flist;
	mx_status_type ( *process_function_setup ) ( MX_RECORD * );
	mx_status_type ( *special_processing_setup ) ( MX_RECORD * );
	long this_superclass, this_class;
	unsigned long processing_flags;
	int i;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s: invoked for record '%s'", fname, record->name));

	process_function_setup = NULL;

	/* Do we use the default processing function for this record? */

	processing_flags = record->record_processing_flags;

	/* Has the record processing already been set up?  If so, then
	 * just return.
	 */

	if ( processing_flags & MXF_PROC_PROCESSING_IS_INITIALIZED ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* We need to set up record processing. */

	if ( processing_flags & MXF_PROC_BYPASS_DEFAULT_PROCESSING ) {

		/* No, we do not use the default processing function,
		 * so we do nothing here.
		 */

	} else {

		/* First, setup the record processing that is used for all
		 * MX drivers.
		 */

		mx_status = mx_setup_record_process_functions( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Look for a default processing function for this class
		 * of MX drivers.
		 */

		for (i = 0; i < mx_num_process_function_setup_elements; i++ )
		{
		    this_superclass = 
			    mx_process_function_setup_array[i].mx_superclass;

		    this_class = mx_process_function_setup_array[i].mx_class;

		    if ( ( this_class == record->mx_class )
		      && ( this_superclass == record->mx_superclass ) )
		    {
			process_function_setup =
				mx_process_function_setup_array[i].setup_fn;
		    } else
		    if ( ( this_class == MXC_ANY )
		      && ( this_superclass == record->mx_superclass ) )
		    {
			process_function_setup =
				mx_process_function_setup_array[i].setup_fn;
		    }
		}

		/* If a default processing function is available, invoke it. */

		if ( process_function_setup != NULL ) {
		    mx_status = (*process_function_setup)( record );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}
	}

	/* If a special processing function is available, invoke it. */

	flist = (MX_RECORD_FUNCTION_LIST *) record->record_function_list;

	special_processing_setup = flist->special_processing_setup;

	if ( special_processing_setup != NULL ) {
		mx_status = (*special_processing_setup)( record );

		if ( mx_status.code != MXE_SUCCESS )
		    return mx_status;
	}

	record->record_processing_flags |= MXF_PROC_PROCESSING_IS_INITIALIZED;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_process_record_field( MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			int direction )
{
	static const char fname[] = "mx_process_record_field()";

	mx_status_type (*process_fn) ( void *, void *, int );
	mx_status_type mx_status;

#if PROCESS_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD_FIELD pointer passed was NULL." );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	/* If there is one, call the process function to read data
	 * from the hardware.
	 */

	process_fn = record_field->process_function;

#if PROCESS_DEBUG
	if ( direction == MX_PROCESS_GET ) {
		MX_DEBUG(-1,("%s: '%s.%s' MX_PROCESS_GET (%ld)",
			fname, record->name, record_field->name,
			record_field->label_value ));
	} else
	if ( direction == MX_PROCESS_PUT ) {
		MX_DEBUG(-1,("%s: '%s.%s' MX_PROCESS_PUT (%ld)",
			fname, record->name, record_field->name,
			record_field->label_value ));
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal direction flag (%d) passed.  The only "
		"allowed values are MX_PROCESS_GET (%d) and "
		"MX_PROCESS_PUT (%d).", direction,
			MX_PROCESS_GET, MX_PROCESS_PUT );
	}

	MX_DEBUG(-1,("%s: process_fn = %p", fname, process_fn));
#endif /* PROCESS_DEBUG */

	if ( process_fn != NULL ) {

		/* Only invoke the process function for non-negative
		 * values of record_field->label_value.  A field
		 * with a negative label value is just a placeholder
		 * for some value, so it does not need any further
		 * record processing.
		 */

#if PROCESS_DEBUG_TIMING && 0
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
"#1 before record processing for '%s.%s'", record->name, record_field->name );
#endif

		if ( record_field->label_value < 0 ) {
			return  MX_SUCCESSFUL_RESULT;
		} else {
			/* Invoke the record processing function. */

			mx_status = ( *process_fn )
				    ( record, record_field, direction );

			MX_DEBUG( 1,(
				"%s: process_fn returned mx_status.code = %ld",
					fname, mx_status.code ));
		}

#if PROCESS_DEBUG_TIMING && 0
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
"#2 after record processing for '%s.%s'", record->name, record_field->name );
#endif

		/* If the process function succeeded, invoke
		 * the value changed callback.
		 */

		if ( mx_status.code == MXE_SUCCESS ) {
			mx_status = mx_invoke_callback_list(
					MXSF_VALUE_CHANGED_CALLBACK,
					record_field );
		}
	}

#if PROCESS_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
"- end of processing for '%s.%s'", record->name, record_field->name );
#endif

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_see_if_event_must_be_queued( MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			int *event_must_be_queued )
{
	static const char fname[] = "mx_see_if_event_must_be_queued()";

	MX_EVENT_TIME_MANAGER *event_time_manager;
	MX_CLOCK_TICK current_clock_tick, next_allowed_event_time;
	int event_interval_is_zero, clock_tick_comparison;

	if ( event_must_be_queued == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The event_must_be_queued pointer is NULL." );
	}

	/* If either the record or record_field pointers are NULL,
	 * there is no place to store the queued event, so just
	 * return FALSE for event_must_be_queued.
	 */

	if ( record == (MX_RECORD *) NULL ) {
		*event_must_be_queued = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		*event_must_be_queued = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Does this record have an event time manager?  If not, then
	 * all events must be serviced immediately.
	 */

	event_time_manager = record->event_time_manager;

	if ( event_time_manager == (MX_EVENT_TIME_MANAGER *) NULL ) {

		*event_must_be_queued = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Does this record have a minimum event interval that is non-zero?
	 * If not, then the event must be serviced immediately.
	 */

	event_interval_is_zero = mx_clock_tick_is_zero(
					event_time_manager->event_interval );

	if ( event_interval_is_zero ) {

		*event_must_be_queued = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

#if PROCESS_DEBUG_QUEUEING

	MX_DEBUG(-2,("%s: checking next allowed event time for '%s', '%s'",
		fname, record->name, record_field->name));
#endif

	/* Have we reached the next allowed event time for this record? */

	current_clock_tick = mx_current_clock_tick();

	next_allowed_event_time = event_time_manager->next_allowed_event_time;

#if PROCESS_DEBUG_QUEUEING
	MX_DEBUG(-2,("%s: current_clock_tick = (%lu,%lu)",
		fname, current_clock_tick.high_order,
		current_clock_tick.low_order ));

	MX_DEBUG(-2,("%s: next_allowed_event_time = (%lu,%lu)",
		fname, next_allowed_event_time.high_order,
		next_allowed_event_time.low_order ));
#endif

	clock_tick_comparison = mx_compare_clock_ticks( current_clock_tick,
						      next_allowed_event_time );

#if PROCESS_DEBUG_QUEUEING
	MX_DEBUG(-2,("%s: clock_tick_comparison = %d",
		fname, clock_tick_comparison));
#endif

	if ( clock_tick_comparison >= 0 ) {

		/* If we have reached the next allowed event time, then
		 * we do not need to queue an event.  Instead we can
		 * service it immediately.
		 */

		*event_must_be_queued = FALSE;
	} else {
		/* Otherwise, we must queue an event.  This should be the
		 * only occasion in which we need to queue an event.
		 */

		*event_must_be_queued = TRUE;
	}

#if PROCESS_DEBUG_QUEUEING
	MX_DEBUG(-2,("%s: *event_must_be_queued = %d",
		fname, *event_must_be_queued));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_add_queued_event( MX_SOCKET_HANDLER *socket_handler,
		MX_RECORD *record,
		MX_RECORD_FIELD *record_field,
		void *event_data )
{
	static const char fname[] = "mx_add_queued_event()";

	MX_QUEUED_EVENT *event, *current_event;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

#if PROCESS_DEBUG_QUEUEING
	if ( record_field != (MX_RECORD_FIELD *) NULL ) {
		MX_DEBUG(-2,("%s: queueing event for '%s', '%s'",
			fname, record->name, record_field->name));
	} else {
		MX_DEBUG(-2,("%s: queueing event for '%s'",
			fname, record->name));
	}
#endif

	/* Create the event structure. */

	event = (MX_QUEUED_EVENT *) malloc( sizeof(MX_QUEUED_EVENT) );

	if ( event == (MX_QUEUED_EVENT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating an MX_QUEUED_EVENT for record '%s'",
			record->name );
	}

	event->socket_handler = socket_handler;
	event->record = record;
	event->record_field = record_field;
	event->event_data = event_data;
	event->next_event = NULL;

	/* Add the event to the queue. */

	current_event = record->event_queue;

	if ( current_event == NULL ) {
		record->event_queue = event;
	} else {
		while ( current_event->next_event != NULL ) {
			current_event = current_event->next_event;
		}
		current_event->next_event = event;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_process_queued_event( MX_RECORD *record,
			MX_CLOCK_TICK current_clock_tick )
{
	static const char fname[] = "mx_process_queued_event()";

	MX_QUEUED_EVENT *queued_event;
	MX_EVENT_HANDLER *event_handler;
	MX_EVENT_TIME_MANAGER *event_time_manager;
	mx_status_type ( *process_queued_event_fn ) (MX_QUEUED_EVENT *);
	int process_event, comparison;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	MX_DEBUG(1,(
	"%s invoked for record '%s', current_clock_time = (%lu,%lu)",
		fname, record->name,
		current_clock_tick.high_order,
		current_clock_tick.low_order ));

	queued_event = (MX_QUEUED_EVENT *) record->event_queue;

	if ( queued_event == NULL ) {

		/* There are no events to dequeue. */

		return MX_SUCCESSFUL_RESULT;
	}

	/* Is it time to process the next event? */

	event_time_manager = record->event_time_manager;

	if ( event_time_manager == (MX_EVENT_TIME_MANAGER *) NULL ) {
		process_event = TRUE;
	} else {
		if ( (event_time_manager->event_interval.low_order == 0)
		  && (event_time_manager->event_interval.high_order == 0) ) {

			process_event = TRUE;
		} else {
			comparison = mx_compare_clock_ticks( current_clock_tick,
				event_time_manager->next_allowed_event_time );

			if ( comparison >= 0 ) {
				process_event = TRUE;
			} else {
				process_event = FALSE;
			}
		}
	}

	if ( process_event == TRUE ) {
		/* Remove this event from the queue. */

		record->event_queue = queued_event->next_event;

		/* Get a pointer to the event type specific
		 * process_queued_event function.
		 */

		event_handler = queued_event->socket_handler->event_handler;

		process_queued_event_fn = event_handler->process_queued_event;

		if ( process_queued_event_fn == NULL ) {
			mx_free( queued_event );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Event handler type %ld does not have a process_queued_event function.",
				event_handler->type );
		}

		/* Invoke the event type specific processing function. */

		(void) ( *process_queued_event_fn ) ( queued_event );

		/* If an event time manager is in use, compute the time
		 * of the next allowed event.
		 */

		if ( record->event_time_manager != NULL ) {

#if PROCESS_DEBUG_QUEUEING
			MX_DEBUG(-2,
			("%s: updating next_allowed_event_time for '%s'",
			 	fname, record->name));
#endif

			mx_status = mx_update_next_allowed_event_time(
					queued_event->record,
					queued_event->record_field );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Dispose of the event now that we are done with it. */

		mx_free( queued_event );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_update_next_allowed_event_time( MX_RECORD *record,
				MX_RECORD_FIELD *record_field )
{

#if PROCESS_DEBUG_QUEUEING
	static const char fname[] = "mx_update_next_allowed_event_time()";
#endif

	MX_EVENT_TIME_MANAGER *event_time_manager;
	MX_CLOCK_TICK event_interval, current_clock_tick;

	/* If there is no record or no event time manager, then there is
	 * nothing we can do here.
	 */

	if ( record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	event_time_manager = record->event_time_manager;

	if ( event_time_manager == (MX_EVENT_TIME_MANAGER *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If there is a record field, but it has the 
	 * MXFF_NO_NEXT_EVENT_TIME_UPDATE flag set, then
	 * we do not update next_allowed_event_time.
	 */

	if ( record_field != (MX_RECORD_FIELD *) NULL ) {
		if ( record_field->flags & MXFF_NO_NEXT_EVENT_TIME_UPDATE )
			return MX_SUCCESSFUL_RESULT;
	}

	/* If the event interval is (0,0) then we do nothing here. */

	event_interval = event_time_manager->event_interval;

	if ( (event_interval.low_order == 0)
	  && (event_interval.high_order == 0) )
	{
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we update the next_allowed_event_time
	 * structure with the current time.
	 */

	current_clock_tick = mx_current_clock_tick();

	event_time_manager->next_allowed_event_time =
				    mx_add_clock_ticks( current_clock_tick,
							event_interval );
#if PROCESS_DEBUG_QUEUEING
	MX_DEBUG(-2, ("%s: next_allowed_event_time = (%lu,%lu)", fname,
			event_time_manager->next_allowed_event_time.high_order,
			event_time_manager->next_allowed_event_time.low_order));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_add_callback( long callback_type, MX_RECORD_FIELD *record_field,
	    mx_status_type( *callback_function ) (MX_RECORD_FIELD *, void *),
	    void *callback_argument )
{
	static const char fname[] = "mx_add_callback()";

	MX_CALLBACK_LIST *server_callback_list;
	MX_CALLBACK **list_ptr, **new_callback_ptr;
	MX_CALLBACK *current_callback;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD_FIELD pointer passed was NULL." );
	}

	if ( record_field->server_callback_list != NULL ) {
		server_callback_list = record_field->server_callback_list;
	} else {
		server_callback_list = (MX_CALLBACK_LIST *)
				malloc( sizeof(MX_CALLBACK_LIST) );

		if ( server_callback_list == (MX_CALLBACK_LIST *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Attempt to allocate server_callback_list for field '%s' failed.",
				record_field->name );
		}

		server_callback_list->value_changed_callback_list = NULL;

		record_field->server_callback_list = server_callback_list;
	}

	/* Get the address of the pointer to the particular callback
	 * list we will be using.
	 */

	switch( callback_type ) {
	case MXSF_VALUE_CHANGED_CALLBACK:
		list_ptr = &(server_callback_list->value_changed_callback_list);
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The server callback type %ld is unrecognized.",
			callback_type );
	}

	/* We will be malloc'ing a new MX_CALLBACK structure in a moment,
	 * but first we must determine the location at which the pointer
	 * returned by malloc() is to be stored.
	 */

	if ( *list_ptr == (MX_CALLBACK *)NULL ) {
		new_callback_ptr = list_ptr;
	} else {
		/* Search until we find the end of the list. */

		current_callback = *list_ptr;

		while ( current_callback->next_callback != NULL ) {
			current_callback = current_callback->next_callback;
		}

		new_callback_ptr = &(current_callback->next_callback);
	}

	/* Now allocate the memory and store a pointer to it. */

	*new_callback_ptr = (MX_CALLBACK *) malloc(sizeof(MX_CALLBACK));

	if ( *new_callback_ptr == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Attempt to allocate MX_CALLBACK structure for field '%s' failed.",
			record_field->name );
	}

	current_callback = *new_callback_ptr;

	/* Finally, we initialize the new callback structure. */

	current_callback->callback_function = callback_function;

	current_callback->callback_argument = callback_argument;

	current_callback->callback_enabled = TRUE;

	current_callback->next_callback = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_callback( long callback_type, MX_RECORD_FIELD *record_field,
	    mx_status_type( *callback_function ) (MX_RECORD_FIELD *, void *),
	    void *callback_argument )
{
	static const char fname[] = "mx_delete_callback()";

	MX_CALLBACK_LIST *server_callback_list;
	MX_CALLBACK **list_ptr;
	MX_CALLBACK *current_callback, *next_callback;
	MX_CALLBACK *next_plus_one_callback;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD_FIELD pointer passed was NULL." );
	}

	server_callback_list = (MX_CALLBACK_LIST *)
				(record_field->server_callback_list);

	if ( server_callback_list == (MX_CALLBACK_LIST *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No callbacks are currently defined for field '%s'",
			record_field->name );
	}
	switch( callback_type ) {
	case MXSF_VALUE_CHANGED_CALLBACK:
		list_ptr = &(server_callback_list->value_changed_callback_list);
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The server callback type %ld is unrecognized.",
			callback_type );
	}

	current_callback = NULL;
	next_callback = *list_ptr;

	while ( next_callback != NULL ) {

		/* Check the next callback in the list to see if it is
		 * the one we want to delete.
		 */

		if (next_callback->callback_function == callback_function){
			if ( (callback_argument == NULL)
			  || (callback_argument
					== next_callback->callback_argument) )
			{
				/* We've found the callback to delete,
				 * so eliminate it from the list.
				 *
				 * If next_plus_one_callback turns out to
				 * be NULL, that's OK, so we don't need
				 * to test for it.
				 */

				next_plus_one_callback
					= next_callback->next_callback;

				if ( current_callback == NULL ) {

					/* This is the first callback in
					 * the list.
					 */
					
					*list_ptr = next_plus_one_callback;
				} else {
					current_callback->next_callback
						= next_plus_one_callback;
				}

				/* next_callback has been removed from
				 * the list, so we can now free it.
				 */

				next_callback->callback_function = NULL;
				next_callback->callback_argument = NULL;
				next_callback->callback_enabled = FALSE;
				next_callback->next_callback = NULL;

				mx_free(next_callback);

				/* We're done, so return to our caller. */

				return MX_SUCCESSFUL_RESULT;
			}
		}
		current_callback = next_callback;
		next_callback = current_callback->next_callback;
	}

	/* If we get here, we didn't find the callback. */

	return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Attempted to delete a non-existant callback function %p for field '%s'",
		callback_function, record_field->name );
}

MX_EXPORT mx_status_type
mx_find_callback( long callback_type, MX_RECORD_FIELD *record_field,
	    mx_status_type( *callback_function ) (MX_RECORD_FIELD *, void *),
	    void *callback_argument, MX_CALLBACK **callback_found )
{
	static const char fname[] = "mx_find_callback()";

	MX_CALLBACK_LIST *server_callback_list;
	MX_CALLBACK *current_callback;

	if ( callback_found == (MX_CALLBACK **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Pointer to MX_CALLBACK pointer passed was NULL." );
	}

	*callback_found = NULL;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD_FIELD pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s invoked for record field '%s'",
			fname, record_field->name));

	server_callback_list = (MX_CALLBACK_LIST *)
				(record_field->server_callback_list);

	if ( server_callback_list == (MX_CALLBACK_LIST *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No callbacks are currently defined for field '%s'",
			record_field->name );
	}
	switch( callback_type ) {
	case MXSF_VALUE_CHANGED_CALLBACK:
		current_callback
			= server_callback_list->value_changed_callback_list;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The server callback type %ld is unrecognized.",
			callback_type );
	}

	while ( current_callback != NULL ) {

		/* Check the callback to see if it is the one we want. */

		if (current_callback->callback_function == callback_function){
			if ( (callback_argument == NULL)
			  || (callback_argument
				== current_callback->callback_argument) )
			{
				/* We've found the callback so return it. */

				*callback_found = current_callback;

				return MX_SUCCESSFUL_RESULT;
			}
		}
		current_callback = current_callback->next_callback;
	}

	/* If we get here, we didn't find the callback. */

	return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Did not find callback function %p for field '%s'",
		callback_function, record_field->name );
}

MX_EXPORT mx_status_type
mx_invoke_callback_list( long callback_type, MX_RECORD_FIELD *record_field )
{
	static const char fname[] = "mx_invoke_callback_list()";

	MX_CALLBACK_LIST *server_callback_list;
	MX_CALLBACK *current_callback;
	int callback_enabled;
	mx_status_type ( *callback_fn ) ( MX_RECORD_FIELD *, void * );

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD_FIELD pointer passed was NULL." );
	}

	MX_DEBUG(1,("%s: $$$ invoked.", fname));

	server_callback_list = (MX_CALLBACK_LIST *)
				(record_field->server_callback_list);

	if ( server_callback_list == (MX_CALLBACK_LIST *) NULL ) {

		/* No callbacks are defined, so just return success. */

		return MX_SUCCESSFUL_RESULT;
	}

	switch( callback_type ) {
	case MXSF_VALUE_CHANGED_CALLBACK:
		current_callback
			= server_callback_list->value_changed_callback_list;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The server callback type %ld is unrecognized.",
			callback_type );
	}

	/* Invoke all the callbacks.  Do not abort if some of the
	 * callbacks fail.
	 */

	while( current_callback != NULL ) {

		callback_fn = current_callback->callback_function;

		callback_enabled = current_callback->callback_enabled;

		if ( callback_enabled == FALSE ) {

			MX_DEBUG(1,(
			"%s: Callback %p for field '%s' is disabled.",
				fname, current_callback, record_field->name));

		} else if ( callback_fn != NULL ) {

			MX_DEBUG(1,(
			"%s: Invoking callback %p for field '%s'.",
				fname, current_callback, record_field->name));

			(void) ( *callback_fn ) ( record_field,
					current_callback->callback_argument );
		}
		current_callback = current_callback->next_callback;
	}

	MX_DEBUG(1,("%s: $$$ succeeded.", fname));

	return MX_SUCCESSFUL_RESULT;
}

