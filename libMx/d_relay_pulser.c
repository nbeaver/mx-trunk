/*
 * Name:    d_relay_pulser.c
 *
 * Purpose: MX pulse generator driver for using an MX relay as
 *          a pulse generator using operating system timers to
 *          control the timing of the pulses.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2019, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_RELAY_PULSER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_time.h"
#include "mx_hrt.h"
#include "mx_callback.h"
#include "mx_relay.h"
#include "mx_pulse_generator.h"
#include "d_relay_pulser.h"

/* Initialize the pulse generator driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_relay_pulser_record_function_list = {
	NULL,
	mxd_relay_pulser_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_relay_pulser_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_relay_pulser_pulser_function_list = {
	mxd_relay_pulser_is_busy,
	mxd_relay_pulser_arm,
	mxd_relay_pulser_trigger,
	mxd_relay_pulser_stop,
	NULL,
	mxd_relay_pulser_get_parameter,
	mxd_relay_pulser_set_parameter
};

/* MX relay pulser data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_relay_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_RELAY_PULSER_STANDARD_FIELDS
};

long mxd_relay_pulser_num_record_fields
		= sizeof( mxd_relay_pulser_record_field_defaults )
		  / sizeof( mxd_relay_pulser_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_relay_pulser_rfield_def_ptr
			= &mxd_relay_pulser_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_relay_pulser_get_pointers( MX_PULSE_GENERATOR *pulser,
			MX_RELAY_PULSER **relay_pulser,
			const char *calling_fname )
{
	static const char fname[] = "mxd_relay_pulser_get_pointers()";

	MX_RELAY_PULSER *relay_pulser_ptr;

	if ( pulser == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( pulser->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for timer pointer passed by '%s' is NULL.",
			calling_fname );
	}

	relay_pulser_ptr = (MX_RELAY_PULSER *)
					pulser->record->record_type_struct;

	if ( relay_pulser_ptr == (MX_RELAY_PULSER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RELAY_PULSER pointer for pulse generator "
			"record '%s' passed by '%s' is NULL",
				pulser->record->name, calling_fname );
	}

	if ( relay_pulser != (MX_RELAY_PULSER **) NULL ) {
		*relay_pulser = relay_pulser_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

static mx_status_type
mxd_relay_pulser_update_internal_state( MX_PULSE_GENERATOR *pulser,
					MX_RELAY_PULSER *relay_pulser )
{
	static const char fname[] =
			"mxd_relay_pulser_update_internal_state()";

	unsigned long flags;
	struct timespec current_timespec, transition_timespec;
	struct timespec timespec_until_next_transition;
	struct timespec old_transition_timespec;
	int comparison;
	unsigned long next_relay_value;
	double time_until_next_transition;
	mx_status_type mx_status;

	if ( pulser == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PULSE_GENERATOR pointer passed was NULL." );
	}

	if ( relay_pulser == (MX_RELAY_PULSER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RELAY_PULSER pointer passed was NULL." );
	}

	flags = relay_pulser->relay_pulser_flags;

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked for '%s', busy = %d, "
		"relay_is_high = %d, num_pulses_left = %ld, "
		"count_forever = %d",
			fname, pulser->record->name, pulser->busy,
			(int) relay_pulser->relay_is_high,
			relay_pulser->num_pulses_left,
			relay_pulser->count_forever));
#endif

	/* If we are marked as not busy, then there is nothing for us
	 * to do.
	 */

	if ( pulser->busy == FALSE ) {
		relay_pulser->count_forever = FALSE;   /* Just to be sure */
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we are busy, has the time of the next state transition
	 * arrived yet?
	 */

	current_timespec = mx_high_resolution_time();

	transition_timespec = relay_pulser->next_transition_timespec;

	comparison = mx_compare_timespec_times( current_timespec,
						transition_timespec );

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s: current_time = (%lu,%lu), "
		"next_transition_time = (%lu,%lu), comparison = %d",
		fname, current_timespec.tv_sec, current_timespec.tv_nsec,
		transition_timespec.tv_sec, transition_timespec.tv_nsec,
		comparison ));
#endif

	if ( comparison < 0 ) {

		/* It is not yet time for the next state transition,
		 * so just return.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	/*--------------- It is time for a state transition. --------------- */

	if ( relay_pulser->relay_is_closed ) {
		relay_pulser->relay_is_closed = FALSE;

		next_relay_value = 0;

		time_until_next_transition =
				pulser->pulse_period - pulser->pulse_width;

		if ( relay_pulser->num_pulses_left == 0 ) {
			if ( relay_pulser->count_forever ) {
				pulser->busy = TRUE;
			} else {
				pulser->busy = FALSE;
			}
		}
	} else {
		relay_pulser->relay_is_closed = TRUE;

		next_relay_value = 1;

		time_until_next_transition = pulser->pulse_width;

		if ( relay_pulser->num_pulses_left > 0 ) {
			relay_pulser->num_pulses_left--;
		}
	}

	if ( time_until_next_transition < 0.0 ) {
		time_until_next_transition = 0.0;
	}

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s: pulser '%s', next_relay_value = %ld",
		fname, pulser->record->name, next_relay_value ));
#endif

	mx_status = mx_relay_command( relay_pulser->relay_record,
						next_relay_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If necessary, compute the time of the next transition.
	 *
	 * If the flag bit MXF_RELAY_PULSER_ALLOW_TIME_SKEW is set,
	 * we compute the time of the next transition relative to the
	 * current time.  Otherwise, we compute the time of the next
	 * transition relative to the time when the previous transition
	 * _should_ have occurred in the absence of operating system
	 * delays.
	 */

	if ( pulser->busy ) {
		timespec_until_next_transition =
			mx_convert_seconds_to_timespec_time(
				time_until_next_transition );

		if ( flags & MXF_RELAY_PULSER_ALLOW_TIME_SKEW ) {
			old_transition_timespec = mx_high_resolution_time();
		} else {
			old_transition_timespec =
				relay_pulser->next_transition_timespec;
		}

		relay_pulser->next_transition_timespec =
			mx_add_timespec_times( old_transition_timespec,
					timespec_until_next_transition );

#if MXD_RELAY_PULSER_DEBUG
		MX_DEBUG(-2,("%s: new next_transition_time = (%lu,%lu)",
			fname, relay_pulser->next_transition_timespec.tv_sec,
			relay_pulser->next_transition_timespec.tv_nsec ));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----*/

static mx_status_type
mxd_relay_pulser_callback_function( MX_CALLBACK_MESSAGE *callback_message )
{
	static const char fname[] = "mxd_relay_pulser_callback_function()";

	MX_PULSE_GENERATOR *pulser;
	MX_RELAY_PULSER *relay_pulser;
	mx_status_type mx_status;

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Check the validity of all of the pointers we need before
	 * restarting the callback timer.
	 */

	if ( callback_message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The MX_CALLBACK_MESSAGE pointer passed to this callback "
		"function is NULL." );
	}

	relay_pulser = (MX_RELAY_PULSER *)
				callback_message->u.function.callback_args;	

	if ( relay_pulser == (MX_RELAY_PULSER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The callback argument pointer contained in "
		"callback message %p is NULL.", callback_message );
	}

	if ( relay_pulser->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The record pointer for the MX_RELAY_PULSER "
		"structure is NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *)
			relay_pulser->record->record_class_struct;

	if ( pulser == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PULSE_GENERATOR pointer for pulser '%s' is NULL.",
			relay_pulser->record->name );
	}

	/* Restart the virtual timer to arrange for the next callback. */

	mx_status = mx_virtual_timer_start(
			callback_message->u.function.oneshot_timer,
			callback_message->u.function.callback_interval );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the status of the pulse generator. */

	mx_status = mxd_relay_pulser_update_internal_state( pulser,
							relay_pulser );

	return mx_status;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_relay_pulser_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_relay_pulser_create_record_structures()";

	MX_PULSE_GENERATOR *pulser;
	MX_RELAY_PULSER *relay_pulser;

	/* Allocate memory for the necessary structures. */

	pulser = (MX_PULSE_GENERATOR *) malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PULSE_GENERATOR structure." );
	}

	relay_pulser = (MX_RELAY_PULSER *)
				malloc( sizeof(MX_RELAY_PULSER) );

	if ( relay_pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RELAY_PULSER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = pulser;
	record->record_type_struct = relay_pulser;
	record->class_specific_function_list
			= &mxd_relay_pulser_pulser_function_list;

	pulser->record = record;
	relay_pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_relay_pulser_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_relay_pulser_open()";

	MX_PULSE_GENERATOR *pulser;
	MX_RELAY_PULSER *relay_pulser = NULL;
	MX_LIST_HEAD *list_head;
	MX_VIRTUAL_TIMER *oneshot_timer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_relay_pulser_get_pointers( pulser,
						&relay_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure that the high resolution timers are initialized. */

	(void) mx_high_resolution_time();

	/* Unconditionally set the relay to closed, so that we are
	 * sure about what state the output is in.
	 */

	mx_status = mx_relay_command( relay_pulser->relay_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	relay_pulser->relay_is_closed = TRUE;
	relay_pulser->next_transition_timespec = mx_high_resolution_time();
	relay_pulser->num_pulses_left = 0;
	relay_pulser->count_forever = FALSE;
	pulser->busy = FALSE;

	/* See if callbacks are enabled. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head->callback_pipe == NULL ) {
		relay_pulser->use_callback = FALSE;
	} else {
		relay_pulser->use_callback = TRUE;
	}

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s: Pulser '%s', use_callback = %d",
		fname, record->name, (int) relay_pulser->use_callback));
#endif

	/* If we are not using callbacks, then we are done here. */

	if ( relay_pulser->use_callback == FALSE )
		return MX_SUCCESSFUL_RESULT;

	/* Create a callback message structure.
	 *
	 * The same callback message structure will be used over and over
	 * again to send messages to the server's main event loop.  It may
	 * not be safe to call malloc() in the context (such as a signal
	 * handler) where the message is put into the main even loop's pipe,
	 * so we do that here where we know that it is safe.
	 */

	relay_pulser->callback_message =
				malloc( sizeof(MX_CALLBACK_MESSAGE) );

	if ( relay_pulser->callback_message == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_CALLBACK_MESSAGE "
		"structure for pulser '%s'.", record->name );
	}

	relay_pulser->callback_message->callback_type = MXCBT_FUNCTION;
	relay_pulser->callback_message->list_head = list_head;

	relay_pulser->callback_message->u.function.callback_function =
					mxd_relay_pulser_callback_function;

	relay_pulser->callback_message->u.function.callback_args =
							relay_pulser;

	/* Specify the callback interval in seconds. */

	relay_pulser->callback_message->u.function.callback_interval = 0.1;

	/* Create a one-shot interval timer that will arrange for the
	 * pulse generator's callback function to be called later.
	 * 
	 * We use a one-shot timer here to avoid filling the callback pipe.
	 */

	mx_status = mx_virtual_timer_create(
				&oneshot_timer,
				list_head->master_timer,
				MXIT_ONE_SHOT_TIMER,
				mx_callback_standard_vtimer_handler,
				relay_pulser->callback_message );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	relay_pulser->callback_message->u.function.oneshot_timer =
								oneshot_timer;

	/* Start the callback virtual timer. */

	mx_status = mx_virtual_timer_start( oneshot_timer,
	    relay_pulser->callback_message->u.function.callback_interval );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s: Callback timer started for pulser '%s'.",
			fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_relay_pulser_is_busy( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_relay_pulser_is_busy()";

	MX_RELAY_PULSER *relay_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_relay_pulser_get_pointers( pulser,
						&relay_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If callbacks are in use, the callback function will manage the
	 * busy state for us.
	 */

	if ( relay_pulser->use_callback == FALSE ) {

		/* If we are not using callbacks and the pulser is marked
		 * as not busy, then we leave it as not busy.
		 */

		if ( pulser->busy != FALSE ) {

			/* The pulser is busy and not using callbacks, so we
			 * must directly update the internal state of the
			 * pulse generator.
			 */

			mx_status = mxd_relay_pulser_update_internal_state(
						pulser, relay_pulser );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s: pulser '%s', busy = %d",
		fname, pulser->record->name, (int) pulser->busy));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_relay_pulser_arm( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_relay_pulser_arm()";

	MX_RELAY_PULSER *relay_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_relay_pulser_get_pointers( pulser,
						&relay_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s: Pulse generator '%s' starting, "
		"pulse_width = %f, pulse_period = %f, num_pulses = %ld",
			fname, pulser->record->name,
			pulser->pulse_width,
			pulser->pulse_period,
			pulser->num_pulses));
#endif
	/* Initialize the internal state of the pulse generator. */

	pulser->busy = TRUE;
	relay_pulser->relay_is_closed          = TRUE;
	relay_pulser->next_transition_timespec = mx_high_resolution_time();
	relay_pulser->num_pulses_left          = pulser->num_pulses;

	if ( pulser->num_pulses == MXF_PGN_FOREVER ) {
		relay_pulser->count_forever = TRUE;
	} else {
		relay_pulser->count_forever = FALSE;
	}

	/* Tell the pulse generator to start the first pulse. */

	mx_status = mxd_relay_pulser_update_internal_state( pulser,
							relay_pulser );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_relay_pulser_trigger( MX_PULSE_GENERATOR *pulser )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_relay_pulser_stop( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_relay_pulser_stop()";

	MX_RELAY_PULSER *relay_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_relay_pulser_get_pointers( pulser,
						&relay_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s: Stopping pulse generator '%s'.",
		fname, pulser->record->name ));
#endif

	/* Begin by closing the relay. */

	mx_status = mx_relay_command( relay_pulser->relay_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the internal state of the pulse generator. */

	pulser->busy = FALSE;
	relay_pulser->relay_is_closed          = TRUE;
	relay_pulser->next_transition_timespec = mx_high_resolution_time();
	relay_pulser->num_pulses_left          = 0;
	relay_pulser->count_forever            = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_relay_pulser_get_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_relay_pulser_get_parameter()";

	MX_RELAY_PULSER *relay_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_relay_pulser_get_pointers( pulser,
						&relay_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		break;

	case MXLV_PGN_PULSE_WIDTH:
		break;

	case MXLV_PGN_PULSE_DELAY:
		break;

	case MXLV_PGN_FUNCTION_MODE:
		switch( pulser->function_mode ) {
		case MXF_PGN_PULSE:
			break;
		case MXF_PGN_SQUARE_WAVE:
			pulser->pulse_width = 0.5 * pulser->pulse_period;
			break;
		default:
			mx_status = mx_error(
			    MXE_HARDWARE_CONFIGURATION_ERROR, fname,
			"Illegal function mode %ld configured for pulser '%s'.",
			    pulser->function_mode, pulser->record->name );

			pulser->function_mode = -1;
			break;
		}
		break;

	case MXLV_PGN_TRIGGER_MODE:
		pulser->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		break;

	default:
		mx_status =
		    mx_pulse_generator_default_get_parameter_handler( pulser );
	}

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_relay_pulser_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_relay_pulser_set_parameter()";

	MX_RELAY_PULSER *relay_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_relay_pulser_get_pointers( pulser,
						&relay_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		break;

	case MXLV_PGN_PULSE_WIDTH:
		break;

	case MXLV_PGN_PULSE_DELAY:
		break;

	case MXLV_PGN_FUNCTION_MODE:
		switch( pulser->function_mode ) {
		case MXF_PGN_PULSE:
		case MXF_PGN_SQUARE_WAVE:
			break;
		default:
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "Illegal function mode %ld requested for pulser '%s'.  "
		    "The allowed modes are 'pulse' (1) or 'square wave' (2).",
				pulser->function_mode, pulser->record->name );
		}
		break;

	case MXLV_PGN_TRIGGER_MODE:
		pulser->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		break;

	default:
		mx_status =
		    mx_pulse_generator_default_set_parameter_handler( pulser );
	}

#if MXD_RELAY_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

