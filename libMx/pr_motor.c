/*
 * Name:    pr_motor.c
 *
 * Purpose: Functions used to process MX motor record events.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006-2007, 2010, 2012-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PR_MOTOR_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_motor.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_pipe.h"
#include "mx_virtual_timer.h"
#include "mx_callback.h"

#include "mx_process.h"
#include "pr_handlers.h"

#define FREE_CALLBACK_OBJECTS() \
	do {								\
		motor->server_backlash_in_progress = FALSE;		\
		mx_free( callback_message );				\
		(void) mx_virtual_timer_destroy( oneshot_timer );	\
	} while (0)

/* mxp_motor_backlash_vtimer_callback() is called when the one-shot
 * virtual timer fires.  Its job is to write a pointer to the
 * MX_CALLBACK_MESSAGE structure to the callback pipe.  Virtual timer
 * callbacks are very limited in what they can do, so it is not safe
 * to try to do the whole thing here.
 */

static void
mxp_motor_backlash_vtimer_callback( MX_VIRTUAL_TIMER *vtimer, void *args )
{
	static const char fname[] = "mxp_motor_backlash_vtimer_callback()";

	MX_CALLBACK_MESSAGE *callback_message;
	MX_LIST_HEAD *list_head;

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,
	("%s: *************************************************", fname));
#endif

	callback_message = args;

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: callback_message = %p", fname, callback_message));
	MX_DEBUG(-2,("%s: backlash callback for motor '%s'",
		fname, callback_message->u.backlash.motor_record->name ));
#endif

	list_head = callback_message->list_head;

	if ( list_head->callback_pipe == (MX_PIPE *) NULL ) {
		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"The callback pipe for this process has not been created." );

		return;
	}

	(void) mx_pipe_write( list_head->callback_pipe,
				(char *) &callback_message,
				sizeof(MX_CALLBACK_MESSAGE *) );

	return;
}

/* mx_motor_backlash_callback() gets called after the main thread reads
 * the message written by mxp_motor_backlash_vtimer_callback() to the
 * callback pipe and sees whether or not the motor has completed the
 * backlash move.  If the backlash move is complete, the main move is
 * started and the callback message is deleted.  If the backlash move
 * is _not_ complete, the one-shot virtual timer is started to reschedule
 * the callback for a later time.
 */

MX_EXPORT mx_status_type
mx_motor_backlash_callback( MX_CALLBACK_MESSAGE *callback_message )
{
	static const char fname[] = "mx_motor_backlash_callback()";

	MX_LIST_HEAD *list_head;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	MX_VIRTUAL_TIMER *oneshot_timer;
	double original_position, destination, backlash_distance, delay;
	double current_position, backlash_destination;
	unsigned long motor_status;
	int flags;
	mx_status_type mx_status;

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: callback_message = %p", fname, callback_message));
#endif

	list_head         = callback_message->list_head;

	motor_record      = callback_message->u.backlash.motor_record;
	oneshot_timer     = callback_message->u.backlash.oneshot_timer;
	original_position = callback_message->u.backlash.original_position;
	destination       = callback_message->u.backlash.destination;
	backlash_distance = callback_message->u.backlash.backlash_distance;
	delay             = callback_message->u.backlash.delay;

	backlash_destination = destination + backlash_distance;

	motor = (MX_MOTOR *) motor_record->record_class_struct;

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: oneshot_timer = %p", fname, oneshot_timer));
#endif

	/* Check to see if the motor is still moving. */

	mx_status = mx_motor_get_extended_status( motor_record,
					&current_position, &motor_status );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_CALLBACK_OBJECTS();
		return mx_status;
	}

#if PR_MOTOR_DEBUG
	MX_DEBUG(-20,("%s: motor '%s' position = %g, status = %#lx",
		fname, motor_record->name, current_position, motor_status));
#endif

	/* Has a motor error occurred? */

	if ( motor_status & MXSF_MTR_ERROR_BITMASK ) {

		/* If so, generate an error message
		 * and shut down the callback.
		 */

		mx_status = mx_error( MXE_INTERRUPTED, fname,
			"Backlash callback for motor '%s' terminated due to "
			"a motor status error.  status = %#lx",
				motor_record->name, motor_status );

		FREE_CALLBACK_OBJECTS();
		return mx_status;
	}

	/* Is the motor still moving? */

	if ( motor_status & MXSF_MTR_IS_BUSY ) {

#if PR_MOTOR_DEBUG
		MX_DEBUG(-2,("%s: Rescheduling backlash callback.",fname));
#endif
		/* If the motor is still moving, reschedule the callback. */

		mx_status = mx_virtual_timer_start( oneshot_timer, delay );

#if PR_MOTOR_DEBUG
		MX_DEBUG(-2,("%s: mx_virtual_timer_start() status = %ld",
			fname, mx_status.code));
#endif
		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_CALLBACK_OBJECTS();
		}

		return mx_status;
	}

	/* If 'server_backlash_in_progress' has been set to FALSE, then we
	 * assume that the motor move has been aborted and terminate the
	 * backlash callback.
	 */

	if ( motor->server_backlash_in_progress == FALSE ) {
		FREE_CALLBACK_OBJECTS();
		return MX_SUCCESSFUL_RESULT;
	}

	/* The motor has stopped moving and is ready to start the main move. */

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: backlash_destination = %g, current_position = %g",
		fname, backlash_destination, current_position));
#endif

	/* Start the main move. */

	flags = MXF_MTR_NOWAIT | MXF_MTR_IGNORE_KEYBOARD
					| MXF_MTR_IGNORE_BACKLASH;

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: starting the main move for motor '%s' to %g",
		fname, motor_record->name, destination));
#endif

	mx_status = mx_motor_move_absolute( motor_record, destination, flags );

	/* Terminate the backlash callback. */

	FREE_CALLBACK_OBJECTS();

	return mx_status;
}

/* mxp_motor_move_absolute_handler() serves as a replacement for
 * mx_motor_move_absolute() in the mx_motor_process_function() below.
 *
 * The job of mxp_motor_move_absolute_handler() is to see if a backlash
 * move is required, start the backlash move, and then schedule a virtual
 * timer event to check to see if the backlash has completed.  If no
 * backlash is needed, the main move is started here.
 */

static mx_status_type
mxp_motor_move_absolute_handler( MX_RECORD *record, double destination )
{
	static const char fname[] = "mxp_motor_move_absolute_handler()";

	MX_LIST_HEAD *list_head;
	MX_MOTOR *motor;
	MX_CALLBACK_MESSAGE *callback_message;
	MX_VIRTUAL_TIMER *oneshot_timer;
	double original_position, relative_motion, backlash, delay_in_seconds;
	mx_bool_type do_backlash;
	int flags;
	mx_status_type mx_status;

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: motor = '%s', destination = %g",
		fname, record->name, destination));
#endif
	/* See if callbacks are enabled. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head->callback_pipe == NULL ) {

		/* We are not configured to handle callbacks,
		 * so just start the move.
		 */

		flags = MXF_MTR_NOWAIT | MXF_MTR_IGNORE_KEYBOARD;

		mx_status = mx_motor_move_absolute( record,
						destination, flags );

		return mx_status;
	}

	/* If we get here, then callbacks are enabled. */

	motor = (MX_MOTOR *) record->record_class_struct;

	/* Figure out whether or not we need to perform a backlash correction
	 * by checking the direction of motion.
	 */

	backlash = motor->backlash_correction;

	mx_status = mx_motor_get_position( record, &original_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	relative_motion = destination - original_position;

	/* We perform the backlash correction if 'relative_motion'
	 * and 'backlash' are both non-zero and have the same signs.
	 */

	if ( (relative_motion > 0.0) && (backlash > 0.0) ) {
		do_backlash = TRUE;
	} else
	if ( (relative_motion < 0.0) && (backlash < 0.0) ) {
		do_backlash = TRUE;
	} else {
		do_backlash = FALSE;
	}

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: original_position = %g", fname, original_position));
	MX_DEBUG(-2,("%s: relative_motion = %g", fname, relative_motion));
	MX_DEBUG(-2,("%s: backlash = %g", fname, backlash));
	MX_DEBUG(-2,("%s: do_backlash = %d", fname, do_backlash));
#endif

	if ( do_backlash == FALSE ) {
		/* If backlash is not required, then just start the move. */

		flags = MXF_MTR_NOWAIT | MXF_MTR_IGNORE_KEYBOARD
					| MXF_MTR_IGNORE_BACKLASH;

		mx_status = mx_motor_move_absolute( record,
						destination, flags );

		return mx_status;
	}

	/**** Backlash is required. ****/

	motor->server_backlash_in_progress = TRUE;

	/* Initialize the backlash callback structure. */

	callback_message = malloc( sizeof(MX_CALLBACK_MESSAGE) );

	if ( callback_message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_CALLBACK_MESSAGE "
		"structure for motor '%s'.", record->name );
	}

	delay_in_seconds = 0.5;

	callback_message->callback_type = MXCBT_MOTOR_BACKLASH;
	callback_message->list_head     = list_head;

	callback_message->u.backlash.motor_record      = record;
	callback_message->u.backlash.original_position = original_position;
	callback_message->u.backlash.destination       = destination;
	callback_message->u.backlash.backlash_distance = backlash;
	callback_message->u.backlash.delay             = delay_in_seconds;

	/* Start the move to the backlash position. */

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: move '%s' to backlash position %g",
		fname, record->name, destination + backlash));
#endif

	flags = MXF_MTR_NOWAIT | MXF_MTR_IGNORE_KEYBOARD
			| MXF_MTR_GO_TO_BACKLASH_POSITION;

	mx_status = mx_motor_move_absolute( record, destination, flags );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( callback_message );
		return mx_status;
	}

	/* Start a one-shot interval timer that will arrange for the
	 * backlash callback function to be invoked later.
	 */

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: callback_message = %p", fname, callback_message));
	MX_DEBUG(-2,("%s: creating backlash one-shot timer.", fname));
#endif

	mx_status = mx_virtual_timer_create( &oneshot_timer,
					list_head->master_timer,
					MXIT_ONE_SHOT_TIMER,
					mxp_motor_backlash_vtimer_callback,
					callback_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( callback_message );
		return mx_status;
	}

	callback_message->u.backlash.oneshot_timer = oneshot_timer;

#if PR_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: starting backlash one-shot timer %p for %g seconds.",
		fname, oneshot_timer, delay_in_seconds));
#endif

	mx_status = mx_virtual_timer_start( oneshot_timer, delay_in_seconds );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_CALLBACK_OBJECTS();
	}

	return mx_status;
}

mx_status_type
mx_setup_motor_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_motor_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_MTR_ACCELERATION_DISTANCE:
		case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		case MXLV_MTR_ACCELERATION_TIME:
		case MXLV_MTR_AXIS_ENABLE:
		case MXLV_MTR_BACKLASH_CORRECTION:
		case MXLV_MTR_BASE_SPEED:
		case MXLV_MTR_BUSY:
		case MXLV_MTR_BUSY_START_INTERVAL:
		case MXLV_MTR_CLOSED_LOOP:
		case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		case MXLV_MTR_COMPUTE_REAL_POSITION:
		case MXLV_MTR_CONSTANT_VELOCITY_MOVE:
		case MXLV_MTR_DERIVATIVE_GAIN:
		case MXLV_MTR_DESTINATION:
		case MXLV_MTR_EXTRA_GAIN:
		case MXLV_MTR_FAULT_RESET:
		case MXLV_MTR_GET_EXTENDED_STATUS:
		case MXLV_MTR_GET_STATUS:
		case MXLV_MTR_HOME_SEARCH:
		case MXLV_MTR_IMMEDIATE_ABORT:
		case MXLV_MTR_INTEGRAL_GAIN:
		case MXLV_MTR_INTEGRAL_LIMIT:
		case MXLV_MTR_LAST_START_TIME:
		case MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
		case MXLV_MTR_MAXIMUM_SPEED:
		case MXLV_MTR_NEGATIVE_LIMIT_HIT:
		case MXLV_MTR_POSITION:
		case MXLV_MTR_POSITIVE_LIMIT_HIT:
		case MXLV_MTR_PROPORTIONAL_GAIN:
		case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		case MXLV_MTR_RAW_HOME_COMMAND:
		case MXLV_MTR_RELATIVE_MOVE:
		case MXLV_MTR_RESTORE_SPEED:
		case MXLV_MTR_SAVE_SPEED:
		case MXLV_MTR_SAVE_START_POSITIONS:
		case MXLV_MTR_SET_POSITION:
		case MXLV_MTR_SOFT_ABORT:
		case MXLV_MTR_SPEED:
		case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		case MXLV_MTR_TWEAK_DISTANCE:
		case MXLV_MTR_TWEAK_FORWARD:
		case MXLV_MTR_TWEAK_REVERSE:
		case MXLV_MTR_VALUE_CHANGE_THRESHOLD:
		case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:

			record_field->process_function
					    = mx_motor_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_motor_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_motor_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *motor_function_list;
	double position1, position2, time_for_move;
	mx_status_type mx_status;

	double start_position, end_position;
	double pseudomotor_position, real_position;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	motor = (MX_MOTOR *) record->record_class_struct;

	motor_function_list = (MX_MOTOR_FUNCTION_LIST *)
				record->class_specific_function_list;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_MTR_POSITION:
			mx_status = mx_motor_get_position( record, NULL );
			break;
		case MXLV_MTR_BUSY:
			mx_status = mx_motor_is_busy( record, NULL );

			if ( motor->server_backlash_in_progress ) {
				motor->busy = TRUE;
				motor->status |= MXSF_MTR_IS_BUSY;
			}
#if PR_MOTOR_DEBUG
			MX_DEBUG(-20,
			("%s: B -> busy = %d, status = %#lx, sbip = %d",
				fname, (int) motor->busy, motor->status,
				motor->server_backlash_in_progress));
#endif
			break;
		case MXLV_MTR_BACKLASH_CORRECTION:
			switch( motor->subclass ) {
			case MXC_MTR_STEPPER:
				motor->backlash_correction = motor->scale
				    * motor->raw_backlash_correction.stepper;
				break;
			case MXC_MTR_ANALOG:
				motor->backlash_correction = motor->scale
				    * motor->raw_backlash_correction.analog;
				break;
			}
			break;
		case MXLV_MTR_NEGATIVE_LIMIT_HIT:
			mx_status = mx_motor_negative_limit_hit( record, NULL );
			break;
		case MXLV_MTR_POSITIVE_LIMIT_HIT:
			mx_status = mx_motor_positive_limit_hit( record, NULL );
			break;
		case MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
			mx_status = mx_motor_get_limit_switch_as_home_switch(
								record, NULL );
			break;
		case MXLV_MTR_SPEED:
			mx_status = mx_motor_get_speed( record, NULL );

			break;
		case MXLV_MTR_BASE_SPEED:
			mx_status = mx_motor_get_base_speed( record, NULL );

			break;
		case MXLV_MTR_MAXIMUM_SPEED:
			mx_status = mx_motor_get_maximum_speed( record, NULL );

			break;
		case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
			mx_status = mx_motor_get_synchronous_motion_mode(
							record, NULL );
			break;
		case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
			mx_status = mx_motor_get_raw_acceleration_parameters(
							record, NULL );

			break;
		case MXLV_MTR_ACCELERATION_TIME:
			mx_status = mx_motor_get_acceleration_time(
							record, NULL );
			break;
		case MXLV_MTR_ACCELERATION_DISTANCE:
			mx_status = mx_motor_get_acceleration_distance(
							record, NULL );
			break;
		case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
			start_position = motor->compute_extended_scan_range[0];
			end_position = motor->compute_extended_scan_range[1];

			mx_status = mx_motor_compute_extended_scan_range(record,
				start_position, end_position, NULL, NULL );
			break;
		case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
			real_position = motor->compute_pseudomotor_position[0];

			mx_status =
		mx_motor_compute_pseudomotor_position_from_real_position(
			record, real_position, &pseudomotor_position, TRUE );

			break;
		case MXLV_MTR_COMPUTE_REAL_POSITION:
			pseudomotor_position = motor->compute_real_position[0];

			mx_status =
		mx_motor_compute_real_position_from_pseudomotor_position(
			record, pseudomotor_position, &real_position, TRUE );

			break;
		case MXLV_MTR_GET_STATUS:
			mx_status = mx_motor_get_status( record, NULL );

			if ( motor->server_backlash_in_progress ) {
				motor->busy = TRUE;
				motor->status |= MXSF_MTR_IS_BUSY;
			}
#if PR_MOTOR_DEBUG
			MX_DEBUG(-20,
			("%s: S -> busy = %d, status = %#lx, sbip = %d",
				fname, (int) motor->busy, motor->status,
				motor->server_backlash_in_progress));
#endif
			break;
		case MXLV_MTR_GET_EXTENDED_STATUS:
			mx_status = mx_motor_get_extended_status( record,
								NULL, NULL );

			if ( motor->server_backlash_in_progress ) {
				motor->busy = TRUE;
				motor->status |= MXSF_MTR_IS_BUSY;

				snprintf( motor->extended_status,
					sizeof(motor->extended_status),
					"%.*e %lx",
					record->precision,
					motor->position,
					motor->status );
			}
#if PR_MOTOR_DEBUG
			MX_DEBUG(-20,
		("%s: E -> busy = %d, status = %#lx, sbip = %d, pos = %g",
				fname, (int) motor->busy, motor->status,
				motor->server_backlash_in_progress,
				motor->position));
#endif
			break;
		case MXLV_MTR_LAST_START_TIME:
			motor->last_start_time =
		    mx_convert_clock_ticks_to_seconds( motor->last_start_tick );
			break;
		case MXLV_MTR_PROPORTIONAL_GAIN:
			mx_status = mx_motor_get_gain( record,
					MXLV_MTR_PROPORTIONAL_GAIN, NULL );
			break;
		case MXLV_MTR_INTEGRAL_GAIN:
			mx_status = mx_motor_get_gain( record,
					MXLV_MTR_INTEGRAL_GAIN, NULL );
			break;
		case MXLV_MTR_DERIVATIVE_GAIN:
			mx_status = mx_motor_get_gain( record,
					MXLV_MTR_DERIVATIVE_GAIN, NULL );
			break;
		case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
			mx_status = mx_motor_get_gain( record,
				MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN, NULL );
			break;
		case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
			mx_status = mx_motor_get_gain( record,
				MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN, NULL );
			break;
		case MXLV_MTR_INTEGRAL_LIMIT:
			mx_status = mx_motor_get_gain( record,
					MXLV_MTR_INTEGRAL_LIMIT, NULL );
			break;
		case MXLV_MTR_EXTRA_GAIN:
			mx_status = mx_motor_get_gain( record,
					MXLV_MTR_EXTRA_GAIN, NULL );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_MTR_TWEAK_FORWARD:
		case MXLV_MTR_TWEAK_REVERSE:
			if (record_field->label_value == MXLV_MTR_TWEAK_FORWARD)
			{
				motor->relative_move = motor->tweak_distance;
			} else {
				motor->relative_move = - motor->tweak_distance;
			}

			/* Fall through to the relative move case with
			 * the newly computed relative move.
			 */

		case MXLV_MTR_RELATIVE_MOVE:
			mx_status = mx_motor_get_position( record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			motor->destination = motor->relative_move
						+ motor->position;

			/* Fall through to the destination case with
			 * the newly computed destination.
			 */

		case MXLV_MTR_DESTINATION:
			mx_status = mxp_motor_move_absolute_handler(
						record, motor->destination );
			break;

			/*---*/

		case MXLV_MTR_SET_POSITION:
			mx_status = mx_motor_set_position( record,
						motor->set_position );

			break;
		case MXLV_MTR_BACKLASH_CORRECTION:
			switch( motor->subclass ) {
			case MXC_MTR_STEPPER:
				motor->raw_backlash_correction.stepper = 
					mx_round( mx_divide_safely(
						motor->backlash_correction,
						motor->scale ) );
				break;
			case MXC_MTR_ANALOG:
				motor->raw_backlash_correction.analog =
					mx_divide_safely(
						motor->backlash_correction,
						motor->scale );
				break;
			}
			break;
		case MXLV_MTR_SOFT_ABORT:
			mx_status = mx_motor_soft_abort( record );

			/* Set the soft abort flag back to zero. */

			motor->soft_abort = 0;
			motor->server_backlash_in_progress = FALSE;
			break;
		case MXLV_MTR_IMMEDIATE_ABORT:
			mx_status = mx_motor_immediate_abort( record );

			/* Set the immediate abort flag back to zero. */

			motor->immediate_abort = 0;
			motor->server_backlash_in_progress = FALSE;
			break;
		case MXLV_MTR_CONSTANT_VELOCITY_MOVE:
			mx_status = mx_motor_constant_velocity_move( record,
					motor->constant_velocity_move );
			break;
		case MXLV_MTR_RAW_HOME_COMMAND:
			mx_status = mx_motor_raw_home_command( record,
					motor->raw_home_command );
			break;
		case MXLV_MTR_HOME_SEARCH:
			mx_status = mx_motor_home_search( record,
						motor->home_search, 0 );
			break;
		case MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
			mx_status = mx_motor_set_limit_switch_as_home_switch(
				record, motor->limit_switch_as_home_switch );
			break;
		case MXLV_MTR_SPEED:
			mx_status = mx_motor_set_speed( record, motor->speed );

			break;
		case MXLV_MTR_BASE_SPEED:
			mx_status = mx_motor_set_base_speed( record,
							motor->base_speed );

			break;
		case MXLV_MTR_MAXIMUM_SPEED:
			mx_status = mx_motor_set_base_speed( record,
							motor->maximum_speed );

			break;
		case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
			mx_status = mx_motor_set_raw_acceleration_parameters(
				record, motor->raw_acceleration_parameters );

			break;
		case MXLV_MTR_ACCELERATION_TIME:
			mx_status = mx_motor_set_acceleration_time( record,
						motor->acceleration_time );
			break;
		case MXLV_MTR_BUSY_START_INTERVAL:
			mx_status = mx_motor_set_busy_start_interval( record,
						motor->busy_start_interval );
			break;
		case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
			position1 = motor->speed_choice_parameters[0];
			position2 = motor->speed_choice_parameters[1];
			time_for_move = motor->speed_choice_parameters[2];

			mx_status = mx_motor_set_speed_between_positions(record,
					position1, position2, time_for_move );
			break;
		case MXLV_MTR_SAVE_SPEED:
			mx_status = mx_motor_save_speed( record );
			break;
		case MXLV_MTR_RESTORE_SPEED:
			mx_status = mx_motor_restore_speed( record );
			break;
		case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
			mx_status = mx_motor_set_synchronous_motion_mode( record,
					motor->synchronous_motion_mode );
			break;
		case MXLV_MTR_SAVE_START_POSITIONS:
			mx_status = mx_motor_save_start_positions( record,
					motor->save_start_positions );
			break;
	
		case MXLV_MTR_AXIS_ENABLE:
			mx_status = mx_motor_send_control_command( record,
					MXLV_MTR_AXIS_ENABLE,
					motor->axis_enable );
			break;
		case MXLV_MTR_CLOSED_LOOP:
			mx_status = mx_motor_send_control_command( record,
					MXLV_MTR_CLOSED_LOOP,
					motor->closed_loop );
			break;
		case MXLV_MTR_FAULT_RESET:
			mx_status = mx_motor_send_control_command( record,
					MXLV_MTR_FAULT_RESET,
					motor->fault_reset );
			break;

		case MXLV_MTR_PROPORTIONAL_GAIN:
			mx_status = mx_motor_set_gain( record,
					MXLV_MTR_PROPORTIONAL_GAIN,
					motor->proportional_gain );
			break;
		case MXLV_MTR_INTEGRAL_GAIN:
			mx_status = mx_motor_set_gain( record,
					MXLV_MTR_INTEGRAL_GAIN,
					motor->integral_gain );
			break;
		case MXLV_MTR_DERIVATIVE_GAIN:
			mx_status = mx_motor_set_gain( record,
					MXLV_MTR_DERIVATIVE_GAIN,
					motor->derivative_gain );
			break;
		case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
			mx_status = mx_motor_set_gain( record,
					MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN,
					motor->velocity_feedforward_gain );
			break;
		case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
			mx_status = mx_motor_set_gain( record,
					MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN,
					motor->acceleration_feedforward_gain );
			break;
		case MXLV_MTR_INTEGRAL_LIMIT:
			mx_status = mx_motor_set_gain( record,
					MXLV_MTR_INTEGRAL_LIMIT,
					motor->integral_limit );
			break;
		case MXLV_MTR_EXTRA_GAIN:
			mx_status = mx_motor_set_gain( record,
					MXLV_MTR_EXTRA_GAIN,
					motor->extra_gain );
			break;
		case MXLV_MTR_VALUE_CHANGE_THRESHOLD:
			{
				MX_RECORD_FIELD *position_field;

				mx_status = mx_find_record_field( record,
						"position", &position_field );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				position_field->value_change_threshold
					= motor->value_change_threshold;
			}
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

