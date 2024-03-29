/*
 * Name:    mx_motor.c
 *
 * Purpose: MX function library of generic motor operations.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_MOTOR_DEBUG_VCTEST				FALSE

#define MX_MOTOR_DEBUG_MOVE_ARRAY_TIMING		FALSE

#define MX_MOTOR_DEBUG_INTERNAL_MOVE_ARRAY_TIMING	FALSE

#define MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING	FALSE

#define MX_MOTOR_DEBUG_WAIT_ARRAY_TIMING		FALSE

#define MX_MOTOR_DEBUG_HOME_SEARCH			FALSE

#define MX_MOTOR_DEBUG_SET_ESTIMATED_MOVE_POSITIONS	FALSE

#define MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_key.h"
#include "mx_hrt_debug.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_callback.h"
#include "mx_motor.h"

/*=======================================================================*/

/* This function is used by a motor's finish_record_initialization
 * driver entry point to initialize several fields based on the 
 * values in the record description.
 */

MX_EXPORT mx_status_type
mx_motor_finish_record_initialization( MX_RECORD *motor_record )
{
	static const char fname[] = "mx_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *flist = NULL;
	MX_RECORD_FIELD *extended_status_field = NULL;
	MX_RECORD_FIELD *position_field = NULL;
	MX_RECORD_FIELD *status_field = NULL;
	MX_RECORD_FIELD *estimated_move_positions_field = NULL;
	MX_RECORD_FIELD *estimated_move_durations_field = NULL;
	mx_status_type mx_status;

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}
	motor = (MX_MOTOR *) (motor_record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			motor_record->name );
	}

	motor->triggered_move_destination = 0.0;
	motor->trigger_move = FALSE;
	motor->trigger_mode = MXF_DEV_NO_TRIGGER;

	motor->window_is_available = FALSE;
	motor->use_window = FALSE;
	motor->window[0] = 0.0;
	motor->window[1] = 0.0;

	/* Initialize motor_flags here to make sure that all bits
	 * are set to 0.
	 */

	motor->motor_flags = 0;

	/* Initialize scaled quantities from the raw quantities. */

	switch( motor->subclass ) {
	case MXC_MTR_STEPPER:
		motor->position = motor->offset + motor->scale
				* (double) motor->raw_position.stepper;

		motor->backlash_correction = motor->scale
			* (double) motor->raw_backlash_correction.stepper;
		
		motor->negative_limit = motor->offset + motor->scale
				* (double) motor->raw_negative_limit.stepper;

		motor->positive_limit = motor->offset + motor->scale
				* (double) motor->raw_positive_limit.stepper;

		break;
	case MXC_MTR_ANALOG:
		motor->position = motor->offset + motor->scale
					* motor->raw_position.analog;

		motor->backlash_correction = motor->scale
				* motor->raw_backlash_correction.analog;
		
		motor->negative_limit = motor->offset + motor->scale
					* motor->raw_negative_limit.analog;

		motor->positive_limit = motor->offset + motor->scale
					* motor->raw_positive_limit.analog;

		break;
	}

	/* See if the driver can change the motor speed. */

	flist = (MX_MOTOR_FUNCTION_LIST *)
			motor_record->class_specific_function_list;

	if ( flist == (MX_MOTOR_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR_FUNCTION_LIST pointer for record '%s' is NULL.",
			motor_record->name );
	}

	if ( ( flist->get_parameter == NULL )
	  || ( flist->set_parameter == NULL ) )
	{
		motor->motor_flags |= MXF_MTR_CANNOT_CHANGE_SPEED;
		motor->motor_flags |= MXF_MTR_CANNOT_QUICK_SCAN;
	}

	/* Set some plausible initial values for parameters. */

	motor->destination     = motor->position;
	motor->old_destination = motor->position;
	motor->set_position    = motor->position;

	motor->predicted_position = motor->destination;
	motor->following_error    = 0;

	motor->use_internal_position_offset = FALSE;
	motor->internal_position_offset     = 0.0;

	motor->raw_saved_speed = motor->raw_speed;

	motor->busy = FALSE;
	motor->negative_limit_hit = FALSE;
	motor->positive_limit_hit = FALSE;

	motor->synchronous_motion_mode = FALSE;

	motor->latched_status = 0;

	motor->real_motor_record = NULL;

	motor->busy_start_interval_enabled = FALSE;
	motor->busy_start_interval = -1;

	motor->backlash_move_in_progress = FALSE;
	motor->server_backlash_in_progress = FALSE;

	motor->home_search_in_progress = FALSE;

	/* If 'quick_scan_backlash_correction' is changed to be settable
	 * in the database file, the following will have to be removed.
	 */

	motor->quick_scan_backlash_correction = 0.0;

	/*---*/

	motor->home_search_type = MXHS_MTR_RAW_HOME_COMMAND;

	/*---*/

	mx_status = mx_find_record_field( motor_record, "extended_status",
						&extended_status_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->extended_status_field_number =
			extended_status_field->field_number;

	/*---*/

	mx_status = mx_find_record_field( motor_record, "position",
						&position_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->position_field_number = position_field->field_number;

	/*---*/

	mx_status = mx_find_record_field( motor_record, "status",
						&status_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status_field_number = status_field->field_number;

	/* Initialize the estimated_move fields. */

	motor->estimated_move_array_size = 0;
	motor->num_estimated_move_positions = 0;
	motor->estimated_move_positions = NULL;
	motor->estimated_move_durations = NULL;
	motor->total_estimated_move_duration = -1.0;
	motor->bypass_estimated_move_calc = FALSE;

	motor->must_recalculate_estimated_move_duration = FALSE;

	mx_status = mx_find_record_field( motor_record,
					"estimated_move_positions",
					&estimated_move_positions_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	estimated_move_positions_field->dimension[0] = 0;
	estimated_move_positions_field->data_element_size[0] = sizeof(double);

	mx_status = mx_find_record_field( motor_record,
					"estimated_move_durations",
					&estimated_move_durations_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	estimated_move_durations_field->dimension[0] = 0;
	estimated_move_durations_field->data_element_size[0] = sizeof(double);

	/*---*/

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_motor_...
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_motor_get_pointers( MX_RECORD *motor_record,
			MX_MOTOR **motor,
			MX_MOTOR_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_motor_get_pointers()";

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The motor_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( motor_record->mx_class != MXC_MOTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not a motor.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			motor_record->name, calling_fname,
			motor_record->mx_superclass,
			motor_record->mx_class,
			motor_record->mx_type );
	}

	if ( motor != (MX_MOTOR **) NULL ) {
		*motor = (MX_MOTOR *) (motor_record->record_class_struct);

		if ( *motor == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' passed by '%s' is NULL",
				motor_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_MOTOR_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_MOTOR_FUNCTION_LIST *)
				(motor_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_MOTOR_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_MOTOR_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				motor_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_motor_is_busy( MX_RECORD *motor_record, mx_bool_type *busy )
{
	static const char fname[] = "mx_motor_is_busy()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor_record->name));

	/* Try to get the busy status is the following order:
	 *
	 * 1.  Invoke the motor_is_busy driver function if available.
	 * 2.  Invoke the get_status driver function if available.
	 * 3.  Invoke the get_extended_status driver function if available.
	 * 4.  Return a not busy status and generate a warning message.
	 */

	if ( fl_ptr->motor_is_busy != NULL ) {
		fptr = fl_ptr->motor_is_busy;

		status = ( *fptr ) ( motor );

		if ( status.code == MXE_SUCCESS ) {
			if ( motor->busy ) {
				motor->status |= MXSF_MTR_IS_BUSY;
			} else {
				motor->status &= ( ~MXSF_MTR_IS_BUSY );
			}
		}
	} else {
		if ( fl_ptr->get_status != NULL ) {
			fptr = fl_ptr->get_status;
		} else
		if ( fl_ptr->get_extended_status != NULL ) {
			fptr = fl_ptr->get_extended_status;
		} else {
			fptr = NULL;

			motor->busy = FALSE;

			status = MX_SUCCESSFUL_RESULT;
		}

		if ( fptr != NULL ) {
			status = ( *fptr ) ( motor );

			if ( status.code == MXE_SUCCESS ) {
				if ( motor->status & MXSF_MTR_IS_BUSY ) {
					motor->busy = TRUE;
				} else {
					motor->busy = FALSE;
				}
			}
		}
	}

	if ( motor->busy_start_interval_enabled ) {
		mx_bool_type busy_start_set;

		status = mx_motor_check_busy_start_interval( motor_record,
							&busy_start_set );

		if ( status.code != MXE_SUCCESS )
			return status;

		if ( busy_start_set ) {
			motor->status |= MXSF_MTR_IS_BUSY;
			motor->busy = TRUE;
		}
	}

	if ( motor->backlash_move_in_progress ) {
		if ( motor->busy == FALSE ) {
			motor->backlash_move_in_progress = FALSE;
		}
	}

	if ( busy != NULL ) {
		*busy = motor->busy;
	}

	return status;
}

/* 'mx_motor_stop' and 'mx_motor_abort' are friendlier names for
 * 'mx_motor_soft_abort' and 'mx_motor_immediate_abort'.
 */

MX_EXPORT mx_status_type
mx_motor_stop( MX_RECORD *motor_record )
{
	mx_status_type mx_status;

	mx_status = mx_motor_soft_abort( motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_abort( MX_RECORD *motor_record )
{
	mx_status_type mx_status;

	mx_status = mx_motor_immediate_abort( motor_record );

	return mx_status;
}

/*------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_soft_abort( MX_RECORD *motor_record )
{
	static const char fname[] = "mx_motor_soft_abort()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Look first for a soft_abort driver function.  If this is not found,
	 * look for an immediate_abort driver function.  If this is not found
	 * either, then ignore the request, but generate a warning message.
	 */

	if ( fl_ptr->soft_abort != NULL ) {
		fptr = fl_ptr->soft_abort;
	} else
	if ( fl_ptr->immediate_abort != NULL ) {
		fptr = fl_ptr->immediate_abort;

		mx_warning(
	"%s: The '%s' driver for motor '%s' does not define a soft_abort "
	"driver function.  We will use immediate_abort instead.",
			fname, mx_get_driver_name( motor_record ),
			motor_record->name );
	} else {
		mx_warning(
	"%s: The '%s' driver for motor '%s' does not define either "
	"a soft_abort or an immediate_abort driver function.  "
	"The abort request will be ignored since the driver provides no way "
	"to do the abort.",
			fname, mx_get_driver_name( motor_record ),
			motor_record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	motor->last_start_tick.high_order = 0;
	motor->last_start_tick.low_order  = 0;

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_immediate_abort( MX_RECORD *motor_record )
{
	static const char fname[] = "mx_motor_immediate_abort()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR *);
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Look first for an immediate_abort driver function.  If this is not
	 * found, look for a soft_abort driver function.  If this is not found
	 * either, then ignore the request, but generate a warning message.
	 */

	if ( fl_ptr->immediate_abort != NULL ) {
		fptr = fl_ptr->immediate_abort;
	} else
	if ( fl_ptr->soft_abort != NULL ) {
		fptr = fl_ptr->soft_abort;

		mx_warning(
	"%s: The '%s' driver for motor '%s' does not define an immediate_abort "
	"driver function.  We will use soft_abort instead.",
			fname, mx_get_driver_name( motor_record ),
			motor_record->name );
	} else {
		mx_warning(
	"%s: The '%s' driver for motor '%s' does not define either "
	"an immediate_abort or a soft_abort driver function.  "
	"The abort request will be ignored since the driver provides no way "
	"to do the abort.",
			fname, mx_get_driver_name( motor_record ),
			motor_record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	motor->last_start_tick.high_order = 0;
	motor->last_start_tick.low_order  = 0;

	status = ( *fptr ) ( motor );

	return status;
}

/* -------------------------------------------- */

static mx_status_type
mx_motor_set_backlash_flags( long num_motors,
				MX_RECORD **motor_record_array,
				mx_bool_type flag_value )
{
	static const char fname[] = "mx_motor_set_backlash_flags()";

	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	long i;

	if ( motor_record_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The motor_record_array pointer passed was NULL." );
	}

	for ( i = 0; i < num_motors; i++ ) {
		motor_record = motor_record_array[i];

		if ( motor_record == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"motor_record_array[%ld] is NULL.", i );
		}

		MX_DEBUG( 2,("%s: motor[%ld] = '%s', flag_value = %d",
			fname, i, motor_record->name, (int) flag_value));

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR for motor_record_array[%ld] is NULL.", i );
		}

		motor->backlash_move_in_progress = flag_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --- Move by engineering units functions. --- */

MX_EXPORT mx_status_type
mx_motor_move_relative_with_report(MX_RECORD *motor_record,
			double relative_position,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags)
{
	double current_position, new_position;
	mx_status_type status;

	status = mx_motor_get_position( motor_record, &current_position );

	if ( status.code != MXE_SUCCESS )
		return status;

	new_position = current_position + relative_position;

	status = mx_motor_move_absolute_with_report( motor_record,
				new_position, move_report_fn, flags );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_move_absolute_with_report(MX_RECORD *motor_record,
			double destination,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags)
{
	static const char fname[] = "mx_motor_move_absolute_with_report()";

	MX_MOTOR *motor;
	double raw_destination;
	long raw_motor_steps;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, NULL, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	MX_DEBUG( 2,("%s: moving '%s' to %g",
			fname, motor_record->name, destination ));

	if ( motor->use_internal_position_offset ) {
		destination -= motor->internal_position_offset;
	}

	raw_destination = mx_divide_safely( destination - motor->offset,
						motor->scale );

	switch ( motor->subclass ) {
	case MXC_MTR_STEPPER:
		raw_motor_steps = mx_round( raw_destination );

		status = mx_motor_move_absolute_steps_with_report(motor_record,
			raw_motor_steps, move_report_fn, flags );

		break;

	case MXC_MTR_ANALOG:
		status = mx_motor_move_absolute_analog_with_report(motor_record,
			raw_destination, move_report_fn, flags );

		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown motor subclass %ld.", motor->subclass );
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_array_move_absolute_with_report( long num_motors,
			MX_RECORD **motor_record_array,
			double *motor_position,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags )
{
	static const char fname[] =
			"mx_motor_array_move_absolute_with_report()";

	MX_MOTOR *motor;
	unsigned long modified_flags;
	long i;
	int do_backlash, individual_backlash_needed, result_flag;
	double backlash, backlash_position;
	double present_position, relative_motion;
	mx_status_type status;

#if MX_MOTOR_DEBUG_MOVE_ARRAY_TIMING
	MX_HRT_TIMING total_move_measurement;
	MX_HRT_TIMING check_limits_measurement;
	MX_HRT_TIMING check_for_backlash_measurement;
	MX_HRT_TIMING do_backlash_measurement;
	MX_HRT_TIMING main_move_measurement;

	MX_HRT_START( total_move_measurement );
	MX_HRT_START( check_limits_measurement );
#endif

	MX_DEBUG( 2,("%s: *** flags = %#lx ***", fname, flags));

	/* See if any of the requested moves will exceed a software limit. */

	MX_DEBUG( 2,("%s: Checking software limits.",fname));

	for ( i = 0; i < num_motors; i++ ) {
		status = mx_is_motor_position_between_software_limits(
				motor_record_array[i], motor_position[i],
				&result_flag, TRUE);

		if ( status.code != MXE_SUCCESS )
			return status;
	}

#if MX_MOTOR_DEBUG_MOVE_ARRAY_TIMING
	MX_HRT_END( check_limits_measurement );
	MX_HRT_RESULTS( check_limits_measurement, fname, "check limits" );
#endif

	/* Move to the backlash_position. */

	MX_DEBUG( 2,("%s: Checking for backlash correction.", fname));

	if ( (flags & MXF_MTR_IGNORE_BACKLASH) == 0 ) {


#if MX_MOTOR_DEBUG_MOVE_ARRAY_TIMING
		MX_HRT_START( check_for_backlash_measurement );
#endif
		MX_DEBUG( 2,("%s: Backlash will be done if needed.", fname));

		/* We must also check to see if the backlash correction
		 * will exceed a limit.
		 */

		do_backlash = FALSE;

		for ( i = 0; i < num_motors; i++ ) {

			motor = (MX_MOTOR *)
				motor_record_array[i]->record_class_struct;

			backlash = motor->backlash_correction;

			status = mx_motor_get_position( motor_record_array[i],
							&present_position );
			if ( status.code != MXE_SUCCESS )
				return status;

			relative_motion = motor_position[i] - present_position;

			/* A given motor needs to do a backlash correction
			 * if relative_motion and backlash have the same signs.
			 */

			individual_backlash_needed = FALSE;

			if (( relative_motion > 0.0 ) && ( backlash > 0.0 )) {
				individual_backlash_needed = TRUE;
			} else
			if (( relative_motion < 0.0 ) && ( backlash < 0.0 )) {
				individual_backlash_needed = TRUE;
			}

			if ( individual_backlash_needed ) {

				do_backlash = TRUE;

				backlash_position
					= motor_position[i] + backlash;

				status =
				  mx_is_motor_position_between_software_limits(
					motor_record_array[i],
					backlash_position,
					&result_flag, TRUE);

				if ( status.code != MXE_SUCCESS )
					return status;
			}
		}

#if MX_MOTOR_DEBUG_MOVE_ARRAY_TIMING
		MX_HRT_END( check_for_backlash_measurement );
		MX_HRT_RESULTS( check_for_backlash_measurement, fname,
							"check for backlash" );

		MX_HRT_START( do_backlash_measurement );
#endif

		MX_DEBUG( 2,("%s: do_backlash = %d", fname, do_backlash));

		/* Do the backlash correction. */

		if ( do_backlash ) {
			modified_flags = flags
					   | MXF_MTR_GO_TO_BACKLASH_POSITION;

			modified_flags &= ( ~ MXF_MTR_NOWAIT );

			MX_DEBUG( 2,
			("%s: Backlash correction for move segment.", fname));

			mx_motor_set_backlash_flags( num_motors,
					motor_record_array, TRUE );

			status = mx_motor_array_internal_move_with_report(
					num_motors,
					motor_record_array,
					motor_position,
					move_report_fn,
					modified_flags );

			mx_motor_set_backlash_flags( num_motors,
					motor_record_array, FALSE );

			if ( status.code != MXE_SUCCESS )
				return status;
		}

#if MX_MOTOR_DEBUG_MOVE_ARRAY_TIMING
		MX_HRT_END( do_backlash_measurement );
		MX_HRT_RESULTS( do_backlash_measurement, fname, "do backlash" );
#endif
	}

#if MX_MOTOR_DEBUG_MOVE_ARRAY_TIMING
	MX_HRT_START( main_move_measurement );
#endif

	/* Now move to the final position. */

	modified_flags = flags | MXF_MTR_IGNORE_BACKLASH;

	MX_DEBUG( 2,("%s: Main move for move segment.", fname));

	status = mx_motor_array_internal_move_with_report(
					num_motors,
					motor_record_array,
					motor_position,
					move_report_fn,
					modified_flags );
#if MX_MOTOR_DEBUG_MOVE_ARRAY_TIMING
	MX_HRT_END( main_move_measurement );
	MX_HRT_END( total_move_measurement );

	MX_HRT_RESULTS( main_move_measurement, fname, "main move" );
	MX_HRT_RESULTS( total_move_measurement, fname, "TOTAL move" );
#endif
	return status;
}

MX_EXPORT mx_status_type
mx_motor_array_internal_move_with_report( long num_motors,
			MX_RECORD **motor_record_array,
			double *motor_position,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags )
{
	static const char fname[]
		= "mx_motor_array_internal_move_with_report()";

	MX_RECORD *first_motor_record;
	MX_MOTOR_FUNCTION_LIST *flist;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	mx_status_type (*simultaneous_start_fn)(
				long, MX_RECORD **, double *, unsigned long );
	mx_status_type status;
	unsigned long move_report_flag, modified_flags, wait_flag;
	int i, j;

#if MX_MOTOR_DEBUG_INTERNAL_MOVE_ARRAY_TIMING
	MX_HRT_TIMING total_internal_move_measurement, wait_measurement;
	MX_HRT_TIMING simultaneous_start_measurement, start_move_measurement;
	MX_HRT_TIMING display_move_measurement;

	MX_HRT_START( total_internal_move_measurement );
#endif

	MX_DEBUG( 2,
		("%s invoked.  flags = %#lx, move_report_fn = %p",
		fname, flags, move_report_fn));

	move_report_flag = flags & MXF_MTR_SHOW_MOVE;

	modified_flags = flags & ( ~ MXF_MTR_SHOW_MOVE );

	modified_flags |= MXF_MTR_NOWAIT;

	if ( flags & MXF_MTR_SIMULTANEOUS_START ) {

#if MX_MOTOR_DEBUG_INTERNAL_MOVE_ARRAY_TIMING
		MX_HRT_START( simultaneous_start_measurement );
#endif

		if ( num_motors <= 0 ) {
			/* If there are no motors, then return without
			 * doing anything.
			 */

			return MX_SUCCESSFUL_RESULT;
		}

		first_motor_record = motor_record_array[0];

		/* Get the MX_MOTOR_FUNCTION_LIST for the first motor. */

		flist = (MX_MOTOR_FUNCTION_LIST *)
			first_motor_record->class_specific_function_list;

		if ( flist == (MX_MOTOR_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR_FUNCTION_LIST pointer for motor '%s' is NULL.",
				first_motor_record->name );
		}

		/* Find the simultaneous_start driver function for the
		 * first motor.
		 */

		simultaneous_start_fn = flist->simultaneous_start;

		if ( simultaneous_start_fn == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for motor '%s' does not support simultaneous starts.",
				mx_get_driver_name( first_motor_record ),
				first_motor_record->name );
		}

		/* If we get this far, save the old destinations for 
		 * each motor.
		 */

		for ( i = 0; i < num_motors; i++ ) {
			motor_record = motor_record_array[i];

			if ( motor_record == (MX_RECORD *) NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
					"Motor record %d for the requested "
					"simultaneous start is NULL.", i );
			}

			motor = (MX_MOTOR *) motor_record->record_class_struct;

			if ( motor == (MX_MOTOR *) NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
					"The MX_MOTOR pointer for motor "
					"record '%s' is NULL.",
						motor_record->name );
			}

			motor->old_destination = motor->destination;

			motor->latched_status = 0;
		}

		/* Execute the simultaneous start. */

		status = (*simultaneous_start_fn)( num_motors,
						motor_record_array,
						motor_position,
						modified_flags );

		if ( status.code != MXE_SUCCESS )
			return status;

#if MX_MOTOR_DEBUG_INTERNAL_MOVE_ARRAY_TIMING
		MX_HRT_END( simultaneous_start_measurement );

		MX_HRT_RESULTS( simultaneous_start_measurement, fname,
				"simultaineous start" );
#endif

	} else {
		/* We were not asked for a simultaneous start, so start
		 * the motors in sequence.
		 */

		for ( i = 0; i < num_motors; i++ ) {

#if MX_MOTOR_DEBUG_INTERNAL_MOVE_ARRAY_TIMING
			MX_HRT_START( start_move_measurement );
#endif

			MX_DEBUG( 2,("%s: moving %s to %g",
				fname, motor_record_array[i]->name,
				motor_position[i]));

			status = mx_motor_move_absolute_with_report(
				motor_record_array[i], motor_position[i],
				NULL, modified_flags );

			if ( status.code != MXE_SUCCESS ) {
				for ( j = 0; j < i-1; j++ ) {
					(void) mx_motor_soft_abort(
						motor_record_array[j] );
				}
				return status;
			}

#if MX_MOTOR_DEBUG_INTERNAL_MOVE_ARRAY_TIMING
			MX_HRT_END( start_move_measurement );

			MX_HRT_RESULTS( start_move_measurement, fname,
							"start move" );
#endif
		}
	}

#if MX_MOTOR_DEBUG_INTERNAL_MOVE_ARRAY_TIMING
	MX_HRT_START( display_move_measurement );
#endif

	MX_DEBUG( 2,("%s: About to display motor moves.", fname));

	/* Display the motor moves if requested. */

	if ( move_report_flag ) {
		status = ( *move_report_fn ) ( flags,
					num_motors, motor_record_array );
	}

#if MX_MOTOR_DEBUG_INTERNAL_MOVE_ARRAY_TIMING
	MX_HRT_END( display_move_measurement );
	MX_HRT_RESULTS( display_move_measurement, fname, "display move" );

	MX_HRT_START( wait_measurement );
#endif

	/* Wait for the motor move to complete if requested. */

	wait_flag = ! ( flags & MXF_MTR_NOWAIT );

	if ( wait_flag ) {
		MX_DEBUG( 2,("%s: wait flag is set.",fname));

		status = mx_wait_for_motor_array_stop( num_motors,
						motor_record_array, flags );

		if ( status.code != MXE_SUCCESS )
			return status;
	}

#if MX_MOTOR_DEBUG_INTERNAL_MOVE_ARRAY_TIMING
	MX_HRT_END( wait_measurement );
	MX_HRT_END( total_internal_move_measurement );

	MX_HRT_RESULTS( wait_measurement, fname, "wait" );
	MX_HRT_RESULTS( total_internal_move_measurement, fname,
						"TOTAL internal move" );
#endif

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_wait_for_motor_stop( MX_RECORD *motor_record, unsigned long flags )
{
	static const char fname[] = "mx_wait_for_motor_stop()";

	int interrupt;
	unsigned long motor_status, busy, error_bitmask;
	unsigned long hardware_limit_bitmask, software_limit_bitmask;
	unsigned long hardware_limit_hit, software_limit_hit;
	unsigned long ignore_keyboard, ignore_limit_switches;
	unsigned long ignore_pause, show_move;
	MX_CLOCK_TICK show_tick_interval, current_tick, next_show_tick;
	int comparison;
	double position;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor_record->name ));

	memset( &next_show_tick, 0, sizeof(next_show_tick) );

	hardware_limit_bitmask =
		MXSF_MTR_POSITIVE_LIMIT_HIT | MXSF_MTR_NEGATIVE_LIMIT_HIT;

	software_limit_bitmask =
	    MXSF_MTR_SOFT_POSITIVE_LIMIT_HIT | MXSF_MTR_SOFT_NEGATIVE_LIMIT_HIT;

	error_bitmask = MXSF_MTR_ERROR_BITMASK;

	ignore_keyboard = flags & MXF_MTR_IGNORE_KEYBOARD;

	ignore_limit_switches = flags & MXF_MTR_IGNORE_LIMIT_SWITCHES;

	ignore_pause = flags & MXF_MTR_IGNORE_PAUSE;

	show_move = flags & MXF_MTR_SHOW_MOVE;

	if ( ignore_limit_switches ) {
		error_bitmask &=
	   ~(hardware_limit_bitmask | software_limit_bitmask | MXSF_MTR_ERROR);
	}

	if ( show_move ) {
		show_tick_interval = mx_convert_seconds_to_clock_ticks( 1.0 );

		current_tick = mx_current_clock_tick();

		next_show_tick = mx_add_clock_ticks( current_tick,
							show_tick_interval );
	}

	if ( flags & MXF_MTR_IGNORE_ERRORS ) {
		error_bitmask = 0;
	}

#if MX_MOTOR_DEBUG_HOME_SEARCH
	MX_DEBUG(-2,("%s: error_bitmask = %#lx",
		fname, error_bitmask));
#endif

	for(;;) {
		if ( show_move ) {
			current_tick = mx_current_clock_tick();

			comparison = mx_compare_clock_ticks( current_tick,
							next_show_tick );

			if ( comparison >= 0 ) {
				mx_status = mx_motor_get_position(
						motor_record, &position );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				mx_info( "  %g", position );

				next_show_tick = mx_add_clock_ticks(
							current_tick,
							show_tick_interval );
			}
		}

		mx_status = mx_motor_get_status( motor_record, &motor_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ignore_limit_switches == FALSE ) {

			/* Has the motor hit a limit? */

			hardware_limit_hit = 
					motor_status & hardware_limit_bitmask;

			if ( hardware_limit_hit ) {
				(void) mx_motor_soft_abort( motor_record );

				return mx_error( MXE_INTERRUPTED, fname,
			    "Hardware limit hit.  Move aborted for motor '%s'.",
					motor_record->name );
			}

			software_limit_hit =
					motor_status & software_limit_bitmask;

			if ( software_limit_hit ) {
				(void) mx_motor_soft_abort( motor_record );

				return mx_error( MXE_INTERRUPTED, fname,
			    "Software limit hit.  Move aborted for motor '%s'.",
					motor_record->name );
			}
		}

		if ( motor_status & error_bitmask ) {
#if MX_MOTOR_DEBUG_HOME_SEARCH
			MX_DEBUG(-2,
			("%s: motor_status = %#lx, error_bitmask = %#lx",
				fname, motor_status, error_bitmask));
#endif
			if ( motor_status & MXSF_MTR_FOLLOWING_ERROR ) {
				mx_warning( "Following error for motor '%s'.",
					motor_record->name );
			}
			if ( motor_status & MXSF_MTR_DRIVE_FAULT ) {
				mx_warning( "Drive fault for motor '%s'.",
					motor_record->name );
			}
			if ( motor_status & MXSF_MTR_AXIS_DISABLED ) {
				mx_warning( "Motor '%s' axis is disabled.",
					motor_record->name );
			}
			if ( motor_status & MXSF_MTR_OPEN_LOOP ) {
				mx_warning( "Motor '%s' is open loop.",
					motor_record->name );
			}
			return mx_error( MXE_INTERRUPTED, fname,
				"Move aborted for motor '%s' due to errors.  "
				"MX motor status = %#lx", 
					motor_record->name, motor_status );
		}

		/* Has the motor stopped? */

		busy = motor_status & MXSF_MTR_IS_BUSY;

		if ( busy == 0 ) {
			/* The motor has stopped.  Exit the while() loop. */

			break;
		}

		if ( ignore_keyboard == FALSE ) {

			/* Did someone hit a key? */

			interrupt = mx_user_requested_interrupt_or_pause();

			switch( interrupt ) {
			case MXF_USER_INT_NONE:
				/* No interrupt ocurred. */

				break;

			case MXF_USER_INT_ABORT:
				(void) mx_motor_soft_abort( motor_record );

				return mx_error( MXE_INTERRUPTED, fname,
		    "Move to backlash position aborted due to user request.");

				break;

			case MXF_USER_INT_PAUSE:
				if ( ignore_pause == FALSE ) {
					return mx_error(
					MXE_PAUSE_REQUESTED, fname,
					"Pause requested by user.");
				}
				break;

			case MXF_USER_INT_ERROR:
				return mx_error( MXE_FUNCTION_FAILED, fname,
				    "An error occurred while attempting to "
				    "check for a user requested interrupt." );
				break;

			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
					"Unexpected value %d returned "
					"by mx_user_requested_interrupt()",
						    interrupt );
				break;
			}
		}

		mx_msleep(10);
	}

	MX_DEBUG( 2,("%s complete for motor '%s'.", fname, motor_record->name));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_wait_for_motor_array_stop( long num_motor_records,
			MX_RECORD **motor_record_array,
			unsigned long flags )
{
	static const char fname[] = "mx_wait_for_motor_array_stop()";

	int i, j, interrupt;
	int motor_is_moving, any_error_occurred;
	unsigned long motor_status, error_bitmask;
	unsigned long hardware_limit_bitmask, software_limit_bitmask;
	unsigned long ignore_keyboard, ignore_limit_switches;
	unsigned long ignore_pause;
	mx_status_type mx_status;

#if MX_MOTOR_DEBUG_WAIT_ARRAY_TIMING
	MX_HRT_TIMING total_wait_measurement, get_status_measurement;
	MX_HRT_TIMING check_keyboard_measurement, msleep_measurement;

	MX_HRT_START( total_wait_measurement );
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	ignore_keyboard = flags & MXF_MTR_IGNORE_KEYBOARD;

	ignore_limit_switches = flags & MXF_MTR_IGNORE_LIMIT_SWITCHES;

	ignore_pause = flags & MXF_MTR_IGNORE_PAUSE;

	hardware_limit_bitmask =
		MXSF_MTR_POSITIVE_LIMIT_HIT | MXSF_MTR_NEGATIVE_LIMIT_HIT;

	software_limit_bitmask =
	    MXSF_MTR_SOFT_POSITIVE_LIMIT_HIT | MXSF_MTR_SOFT_NEGATIVE_LIMIT_HIT;

	error_bitmask = MXSF_MTR_ERROR_BITMASK;

	if ( ignore_limit_switches ) {
		error_bitmask &=
			( ~ (hardware_limit_bitmask | software_limit_bitmask) );
	}
	if ( flags & MXF_MTR_IGNORE_ERRORS ) {
		error_bitmask = 0;
	}

	motor_is_moving = TRUE;
	any_error_occurred = FALSE;

	for(;;) {
		motor_is_moving = FALSE;

		for ( i = 0; i < num_motor_records; i++ ) {

#if MX_MOTOR_DEBUG_WAIT_ARRAY_TIMING
			MX_HRT_START( get_status_measurement );
#endif

			/* What is the status of this motor? */

			mx_status = mx_motor_get_status(
					motor_record_array[i], &motor_status );

			if ( mx_status.code != MXE_SUCCESS ) {
				for ( j = 0; j < num_motor_records; j++ ) {
					(void) mx_motor_soft_abort(
						motor_record_array[j]);
				}
				return mx_status;
			}

			if ( motor_status & MXSF_MTR_IS_BUSY )
				motor_is_moving = TRUE;

			if ( ignore_limit_switches == FALSE ) {
				if ( motor_status & hardware_limit_bitmask ) {
					mx_warning(
					"Hardware limit hit for motor '%s'.",
						motor_record_array[i]->name );
				}
				if ( motor_status & software_limit_bitmask ) {
					mx_warning(
					"Software limit hit for motor '%s'.",
						motor_record_array[i]->name );
				}
			}
			if ( motor_status & error_bitmask ) {
				mx_warning( "Error occurred for motor '%s'.  "
					"MX motor status = %#lx",
					motor_record_array[i]->name,
					motor_status );

				any_error_occurred = TRUE;
			}
			if ( motor_status & MXSF_MTR_FOLLOWING_ERROR ) {
				mx_warning( "Following error for motor '%s'.",
					motor_record_array[i]->name );
			}
			if ( motor_status & MXSF_MTR_DRIVE_FAULT ) {
				mx_warning( "Drive fault for motor '%s'.",
					motor_record_array[i]->name );
			}
			if ( motor_status & MXSF_MTR_AXIS_DISABLED ) {
				mx_warning( "Motor '%s' axis is disabled.",
					motor_record_array[i]->name );
			}
			if ( motor_status & MXSF_MTR_OPEN_LOOP ) {
				mx_warning( "Motor '%s' is open loop.",
					motor_record_array[i]->name );
			}

#if MX_MOTOR_DEBUG_WAIT_ARRAY_TIMING
			MX_HRT_END( get_status_measurement );
			MX_HRT_RESULTS( get_status_measurement, fname,
							"get move status" );

			MX_HRT_START( check_keyboard_measurement );
#endif
			if ( ignore_keyboard == FALSE ) {
				/* Did someone hit a key? */

				interrupt =
					mx_user_requested_interrupt_or_pause();

				switch( interrupt ) {
				case MXF_USER_INT_NONE:
					/* No interrupt occurred. */

					break;

				case MXF_USER_INT_ABORT:

					for (j=0; j < num_motor_records; j++){
						(void) mx_motor_soft_abort(
							motor_record_array[j]);
					}
					return mx_error(
						MXE_INTERRUPTED, fname,
				    "Motor moves aborted due to user request.");

				case MXF_USER_INT_PAUSE:
					if ( ignore_pause == FALSE ) {
						return mx_error(
							MXE_PAUSE_REQUESTED,
							fname,
						"Pause requested by user." );
					}
					break;

				case MXF_USER_INT_ERROR:
					return mx_error(
						MXE_FUNCTION_FAILED, fname,
				    "An error occurred while attempting to "
				    "check for a user requested interrupt." );

				default:
					return mx_error(
						MXE_FUNCTION_FAILED,
						fname,
				    "Unexpected value %d returned "
				    "by mx_user_requested_interrupt()",
						interrupt );
				}
			}

#if MX_MOTOR_DEBUG_WAIT_ARRAY_TIMING
			MX_HRT_END( check_keyboard_measurement );
			MX_HRT_RESULTS( check_keyboard_measurement, fname,
							"check keyboard" );
#endif
		}

#if MX_MOTOR_DEBUG_WAIT_ARRAY_TIMING
		MX_HRT_START( msleep_measurement );
#endif

		if ( any_error_occurred ) {
			for ( j = 0; j < num_motor_records; j++ ) {
				(void) mx_motor_soft_abort(
						motor_record_array[j]);
			}
			return mx_error( MXE_INTERRUPTED, fname,
			"Motor moves aborted due to errors." );
		}

		if ( motor_is_moving == FALSE )
			break;			/* Exit the for loop. */

		mx_msleep(10);

#if MX_MOTOR_DEBUG_WAIT_ARRAY_TIMING
		MX_HRT_END( msleep_measurement );
		MX_HRT_RESULTS( msleep_measurement, fname, "mx_msleep(10)" );
#endif
	}

#if MX_MOTOR_DEBUG_WAIT_ARRAY_TIMING
	MX_HRT_END( total_wait_measurement );
	MX_HRT_RESULTS( total_wait_measurement, fname, "TOTAL wait" );
#endif

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/* mx_motor_internal_move_absolute() bypasses all of the backlash and limit
 * logic.  You should not invoke it directly unless you are prepared to handle
 * limits and backlash yourself.
 */

MX_EXPORT mx_status_type
mx_motor_internal_move_absolute( MX_RECORD *motor_record, double destination )
{
	static const char fname[] = "mx_motor_internal_move_absolute()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	double raw_destination;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	motor->latched_status = 0;

	fptr = fl_ptr->move_absolute;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The '%s' driver for motor '%s' does not define a move_absolute "
	"driver function.  This is almost certain to be a programming bug "
	"since it means that this record can't actually be used as a motor.",
			mx_get_driver_name( motor_record ),
			motor_record->name );
	}

	motor->old_destination = motor->destination;

	motor->destination = destination;

	if ( motor->use_internal_position_offset ) {
		destination -= motor->internal_position_offset;
	}

	raw_destination = mx_divide_safely( destination - motor->offset,
						motor->scale );

	switch( motor->subclass ) {
	case MXC_MTR_STEPPER:
		motor->raw_destination.stepper = mx_round( raw_destination );
		break;

	case MXC_MTR_ANALOG:
		motor->raw_destination.analog = raw_destination;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized motor subclass %ld for motor '%s'.",
			motor->subclass, motor_record->name );
	}

	motor->last_start_tick = mx_current_clock_tick();

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_get_position( MX_RECORD *motor_record, double *position )
{
	static const char fname[] = "mx_motor_get_position()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	double raw_position;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_position;

	if ( fptr == NULL ) {
		/* This driver does not have a get_position function.  See
		 * if there is a get_extended_status function instead.
		 */

		fptr = fl_ptr->get_extended_status;

		if ( fptr == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for motor '%s' does not define either a get_position "
	"or a get_extended_status driver function.  This is likely to be a "
	"bug in that driver.",
				mx_get_driver_name( motor_record ),
				motor_record->name );
		}
	}

	status = ( *fptr ) ( motor );

	switch( motor->subclass ) {
	case MXC_MTR_STEPPER:
		raw_position = (double) ( motor->raw_position.stepper );
		break;
	case MXC_MTR_ANALOG:
		raw_position = motor->raw_position.analog;
		break;
	default:
		raw_position = 0.0;
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized motor subclass %ld for motor '%s'.",
			motor->subclass, motor_record->name );
	}

	/* Update position variables. */

	motor->position = motor->offset + (motor->scale) * raw_position;

	if ( motor->use_internal_position_offset ) {
		motor->position += motor->internal_position_offset;
	}

	if ( position != NULL ) {
		*position = motor->position;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_position( MX_RECORD *motor_record, double set_position )
{
	static const char fname[] = "mx_motor_set_position()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	double raw_set_position, new_set_position;
	double old_position, difference;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_position;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The '%s' driver for motor '%s' does not provide a way to "
		"redefine the position of this motor.",
			mx_get_driver_name( motor_record ),
			motor_record->name );
	}

	if ( motor->use_internal_position_offset ) {
		status = mx_motor_get_position( motor_record, &old_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		difference = set_position - old_position;

		motor->internal_position_offset += difference;

		new_set_position = set_position
				- motor->internal_position_offset;
	} else {
		new_set_position = set_position;
	}

	raw_set_position = mx_divide_safely( new_set_position - motor->offset,
							motor->scale );

	switch( motor->subclass ) {
	case MXC_MTR_STEPPER:
		motor->raw_set_position.stepper = mx_round( raw_set_position );
		break;
	case MXC_MTR_ANALOG:
		motor->raw_set_position.analog = raw_set_position;
		break;
	}

	/* Update the stored set_position value. */

	motor->set_position = motor->offset + motor->scale * raw_set_position;

	/* Invoke the driver function if we are directly changing
	 * the position in the motor controller.
	 */

	if ( motor->use_internal_position_offset == FALSE) {
		status = ( *fptr ) ( motor );
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_positive_limit_hit( MX_RECORD *motor_record, mx_bool_type *limit_hit )
{
	static const char fname[] = "mx_motor_positive_limit_hit()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record, &motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor_record->name));

	/* Try getting the limit status trying the following:
	 *
	 * 1.  Invoke the positive_limit_hit driver function if available.
	 * 2.  Otherwise, invoke the get_status driver function if available.
	 * 3.  Otherwise, invoke the get_extended_status driver function
	 *     if available.
	 * 4.  Otherwise, assume the limit has not been hit and return.
	 */

	if ( fl_ptr->positive_limit_hit != NULL ) {
		fptr = fl_ptr->positive_limit_hit;

		status = ( *fptr ) ( motor );

		if ( status.code == MXE_SUCCESS ) {
			if ( motor->positive_limit_hit ) {
				motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;

				motor->status |= MXSF_MTR_ERROR;
			} else {
				motor->status
					&= ( ~MXSF_MTR_POSITIVE_LIMIT_HIT );
			}
		}
	} else {
		if ( fl_ptr->get_status != NULL ) {
			fptr = fl_ptr->get_status;
		} else
		if ( fl_ptr->get_extended_status != NULL ) {
			fptr = fl_ptr->get_extended_status;
		} else {
			fptr = NULL;

			motor->positive_limit_hit = FALSE;

			status = MX_SUCCESSFUL_RESULT;
		}

		if ( fptr != NULL ) {
			status = ( *fptr ) ( motor );

			if ( status.code == MXE_SUCCESS ) {
				if ( motor->status
					& MXSF_MTR_POSITIVE_LIMIT_HIT )
				{
					motor->positive_limit_hit = TRUE;
				} else {
					motor->positive_limit_hit = FALSE;
				}
			}
		}
	}

	if ( limit_hit != NULL ) {
		*limit_hit = motor->positive_limit_hit;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_negative_limit_hit( MX_RECORD *motor_record, mx_bool_type *limit_hit )
{
	static const char fname[] = "mx_motor_negative_limit_hit()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record, &motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor_record->name));

	/* Try getting the limit status trying the following:
	 *
	 * 1.  Invoke the negative_limit_hit driver function if available.
	 * 2.  Otherwise, invoke the get_status driver function if available.
	 * 3.  Otherwise, invoke the get_extended_status driver function
	 *     if available.
	 * 4.  Otherwise, assume the limit has not been hit and return.
	 */

	if ( fl_ptr->negative_limit_hit != NULL ) {
		fptr = fl_ptr->negative_limit_hit;

		status = ( *fptr ) ( motor );

		if ( status.code == MXE_SUCCESS ) {
			if ( motor->negative_limit_hit ) {
				motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;

				motor->status |= MXSF_MTR_ERROR;
			} else {
				motor->status
					&= ( ~MXSF_MTR_NEGATIVE_LIMIT_HIT );
			}
		}
	} else {
		if ( fl_ptr->get_status != NULL ) {
			fptr = fl_ptr->get_status;
		} else
		if ( fl_ptr->get_extended_status != NULL ) {
			fptr = fl_ptr->get_extended_status;
		} else {
			fptr = NULL;

			motor->negative_limit_hit = FALSE;

			status = MX_SUCCESSFUL_RESULT;
		}

		if ( fptr != NULL ) {
			status = ( *fptr ) ( motor );

			if ( status.code == MXE_SUCCESS ) {
				if ( motor->status
					& MXSF_MTR_NEGATIVE_LIMIT_HIT )
				{
					motor->negative_limit_hit = TRUE;
				} else {
					motor->negative_limit_hit = FALSE;
				}
			}
		}
	}

	if ( limit_hit != NULL ) {
		*limit_hit = motor->negative_limit_hit;
	}

	return status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_raw_home_command( MX_RECORD *motor_record, long direction )
{
	static const char fname[] = "mx_motor_raw_home_command()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	motor->latched_status = 0;

	motor->home_search_in_progress = TRUE;

	fptr = fl_ptr->raw_home_command;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for motor '%s' does not support home searches.",
			mx_get_driver_name( motor_record ),
			motor_record->name );
	}

	if ( motor->scale >= 0.0 ) {
		motor->raw_home_command = direction;
	} else {
		motor->raw_home_command = -direction;
	}

	status = ( *fptr ) ( motor );

	return status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxp_motor_push_motor_off_limit( MX_RECORD *motor_record,
				long direction,
				unsigned long flags )
{
	static const char fname[] = "mxp_motor_push_motor_off_limit()";

	unsigned long motor_status, limit_bitmask;
	unsigned long i, max_attempts, sleep_milliseconds;
	mx_status_type mx_status;

	/* Move off of the limit. */

	mx_status = mx_motor_constant_velocity_move( motor_record, direction );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	max_attempts = 10;
	sleep_milliseconds = 1000;

	limit_bitmask =
		MXSF_MTR_POSITIVE_LIMIT_HIT | MXSF_MTR_NEGATIVE_LIMIT_HIT;

	for ( i = 0; i < max_attempts; i++ ) {
		mx_msleep(sleep_milliseconds);

		mx_status = mx_motor_get_status( motor_record, &motor_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (motor_status & limit_bitmask) == 0 ) {
			break;
		}
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %f seconds for motor '%s' "
		"to move off the limit switch.",
			0.001 * max_attempts * sleep_milliseconds,
			motor_record->name );
	}

	/* Stop the motor and wait for it to stop. */

	mx_status = mx_motor_soft_abort( motor_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_wait_for_motor_stop( motor_record,
					flags | MXF_MTR_IGNORE_LIMIT_SWITCHES );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxp_motor_home_using_limit_switch( MX_RECORD *motor_record,
				MX_MOTOR *motor,
				long direction,
				unsigned long flags,
				unsigned long home_search_type )
{
	static const char fname[] = "mxp_motor_home_using_limit_switch()";

	unsigned long motor_status;
	mx_bool_type unexpected_interrupt;
	mx_bool_type use_two_moves, use_limit_switch_as_home_switch;
	mx_bool_type use_same_limit_direction;
	mx_status_type mx_status;

	switch( home_search_type ) {
	case MXHS_MTR_SAME_DIR_LIMIT_SWITCH_THEN_RAW_HOME_COMMAND:
		use_same_limit_direction = TRUE;
		use_two_moves = TRUE;
		use_limit_switch_as_home_switch = FALSE;
		break;
	case MXHS_MTR_OPPOSITE_DIR_LIMIT_SWITCH_THEN_RAW_HOME_COMMAND:
		use_same_limit_direction = FALSE;
		use_two_moves = TRUE;
		use_limit_switch_as_home_switch = FALSE;
	case MXHS_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
		use_same_limit_direction = TRUE;
		use_two_moves = FALSE;
		use_limit_switch_as_home_switch = TRUE;
		break;
	case MXHS_MTR_SAME_DIR_LIMIT_SWITCH_THEN_LIMIT_SWITCH_AS_HOME_SWITCH:
		use_same_limit_direction = TRUE;
		use_two_moves = TRUE;
		use_limit_switch_as_home_switch = TRUE;
		break;
       case MXHS_MTR_OPPOSITE_DIR_LIMIT_SWITCH_THEN_LIMIT_SWITCH_AS_HOME_SWITCH:
		use_same_limit_direction = FALSE;
		use_two_moves = TRUE;
		use_limit_switch_as_home_switch = TRUE;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal home search type %ld was specified for motor '%s'.",
			home_search_type, motor_record->name );
		break;
	}

	if ( use_two_moves ) {
		if ( use_same_limit_direction ) {
			mx_status = mx_motor_constant_velocity_move(
						motor_record, direction );
		} else {
			mx_status = mx_motor_constant_velocity_move(
						motor_record, -direction );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(1000);

		mx_status = mx_wait_for_motor_stop( motor_record, flags );

#if MX_MOTOR_DEBUG_HOME_SEARCH
		MX_DEBUG(-2,("%s: MX wait status = %ld",
			fname, mx_status.code));
#endif

		switch( mx_status.code ) {
		case MXE_SUCCESS:
#if MX_MOTOR_DEBUG_HOME_SEARCH
			MX_DEBUG(-2,("%s: MXE_SUCCESS", fname));
#endif

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
				"Initial motor move of '%s' completed without "
				"hitting a limit switch.", motor_record->name );
			break;
		case MXE_INTERRUPTED:
			/* Are we at the expected limit switch or did something
			 * else interrupt us?
			 */
#if MX_MOTOR_DEBUG_HOME_SEARCH
			MX_DEBUG(-2,("%s: MXE_INTERRUPTED", fname));
#endif

			unexpected_interrupt = TRUE;

			mx_status = mx_motor_get_status( motor_record,
							&motor_status );
		
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if MX_MOTOR_DEBUG_HOME_SEARCH
			MX_DEBUG(-2,("%s: direction = %ld, motor status = %#lx",
				fname, direction, motor_status ));
#endif

			if ( motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
				if ( direction >= 0 ) {
					if ( use_same_limit_direction ) {
						unexpected_interrupt = FALSE;
					}
				} else {
					if ( use_same_limit_direction == FALSE )
					{
						unexpected_interrupt = FALSE;
					}
				}
			}

			if ( motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
				if ( direction < 0 ) {
					if ( use_same_limit_direction ) {
						unexpected_interrupt = FALSE;
					}
				} else {
					if ( use_same_limit_direction == FALSE )
					{
						unexpected_interrupt = FALSE;
					}
				}
			}

#if MX_MOTOR_DEBUG_HOME_SEARCH
			MX_DEBUG(-2,("%s: unexpected_interrupt = %d",
				fname, (int) unexpected_interrupt));
#endif

			if ( unexpected_interrupt ) {
				return mx_status;
			}
			break;
		default:
#if MX_MOTOR_DEBUG_HOME_SEARCH
			MX_DEBUG(-2,
			("%s: FOO - returning due to motor status %ld",
				fname, mx_status.code));
#endif
			return mx_status;
			break;
		}

#if MX_MOTOR_DEBUG_HOME_SEARCH
		MX_DEBUG(-2,("%s: MARKER 1", fname));
#endif

		if ( use_same_limit_direction ) {

#if MX_MOTOR_DEBUG_HOME_SEARCH
			MX_DEBUG(-2,("%s: pushing off limit.", fname));
#endif
			mx_status = mxp_motor_push_motor_off_limit(
					motor_record, -direction, flags );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

#if MX_MOTOR_DEBUG_HOME_SEARCH
		MX_DEBUG(-2,("%s: MARKER 2", fname));
#endif

	if ( use_limit_switch_as_home_switch ) {
#if MX_MOTOR_DEBUG_HOME_SEARCH
		MX_DEBUG(-2,("%s: Turning limit into home switch.",fname));
#endif
		mx_status = mx_motor_set_limit_switch_as_home_switch(
						motor_record, direction );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MX_MOTOR_DEBUG_HOME_SEARCH
	MX_DEBUG(-2,("%s: Now running the 'real' home command.",fname));
#endif

	mx_status = mx_motor_raw_home_command( motor_record, direction );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_wait_for_motor_stop( motor_record,
				flags | MXF_MTR_IGNORE_LIMIT_SWITCHES );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( use_limit_switch_as_home_switch ) {
#if MX_MOTOR_DEBUG_HOME_SEARCH
		MX_DEBUG(-2,("%s: Reverting to real home switch.",fname));
#endif
		(void) mx_motor_set_limit_switch_as_home_switch(
						motor_record, 0 );
	}

	/* We have reached the end of the home search procedure,
	 * so record that as a latched status bit.
	 */

	motor->latched_status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxp_motor_home_halfway_between_limit_switches( MX_RECORD *motor_record,
						MX_MOTOR *motor,
						long direction,
						unsigned long flags )
{
	static const char fname[] =
		"mxp_motor_home_halfway_between_limit_switches()";

	long limit_switch_search_direction;
	unsigned long motor_status;
	double position_at_limit_switch;
	double first_limit_location, second_limit_location;
	double average_position;
	mx_bool_type unexpected_interrupt;
	mx_status_type mx_status;

	first_limit_location = second_limit_location = 0.0;

	/* We find both limit switches by tripping both of them.
	 * We start by going to the -direction limit_switch.
	 */

	limit_switch_search_direction = -direction;

	while (TRUE) {
		mx_status = mx_motor_constant_velocity_move( motor_record,
						limit_switch_search_direction );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_wait_for_motor_stop( motor_record, flags );

#if 0
		MX_DEBUG(-2,("%s: MX wait status = %ld",
			fname, mx_status.code));
#endif

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
				"Initial motor move of '%s' completed without "
				"hitting a limit switch.", motor_record->name );
			break;
		case MXE_INTERRUPTED:
			/* Are we at the expected limit switch or did something
			 * else interrupt us?
			 */

			unexpected_interrupt = TRUE;

			mx_status = mx_motor_get_status( motor_record,
							&motor_status );
		
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if 0
			MX_DEBUG(-2,("%s: direction = %ld, motor status = %#lx",
				fname, direction, motor_status ));
#endif

			if ( limit_switch_search_direction >= 0 ) {
				if ( motor_status
					& MXSF_MTR_POSITIVE_LIMIT_HIT )
				{
					unexpected_interrupt = FALSE;
				}
			} else {
				if ( motor_status
					& MXSF_MTR_NEGATIVE_LIMIT_HIT )
				{
					unexpected_interrupt = FALSE;
				}
			}

#if 0
			MX_DEBUG(-2,("%s: unexpected_interrupt = %d",
				fname, (int) unexpected_interrupt));
#endif

			if ( unexpected_interrupt ) {
				return mx_status;
			}
			break;
		default:
			return mx_status;
			break;
		}

		mx_status = mx_motor_get_position( motor_record,
						&position_at_limit_switch );

#if 0
		MX_DEBUG(-2,("%s: position_at_limit_switch = %f",
			fname, position_at_limit_switch));
#endif

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( limit_switch_search_direction != direction ) {
			first_limit_location = position_at_limit_switch;

			/* Push the motor away from the limit switch. */

			mx_status = mxp_motor_push_motor_off_limit(
					motor_record, direction, flags );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Now we can now go back and find the other
			 * limit switch.
			 */

			limit_switch_search_direction = direction;
		} else {
			second_limit_location = position_at_limit_switch;

			/* We have now found both limit switches, so
			 * break out of this loop.
			 */

			break;	/* Exit the while() loop. */
		}
	}

	/* Push the motor away from the limit switch. */

	mx_status = mxp_motor_push_motor_off_limit( motor_record,
						-direction, flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Move to a position halfway between the limits. */

	average_position =
		0.5 * ( first_limit_location + second_limit_location );

	mx_status = mx_motor_move_absolute( motor_record, average_position, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_wait_for_motor_stop( motor_record, flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we successfully get to the position halfway between
	 * the limit switches, then redefine this location as 0.
	 */

	mx_status = mx_motor_set_position( motor_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We have reached the end of the home search procedure,
	 * so record that as a latched status bit.
	 */

	motor->latched_status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;

	/* We are done, so return. */

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_home_search( MX_RECORD *motor_record,
			long direction,
			unsigned long flags )
{
	static const char fname[] = "mx_motor_home_search()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *flist;
	mx_status_type ( *special_fn ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record,
					&motor, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->latched_status = 0;

	motor->home_search_in_progress = TRUE;

	/* The action we take depends on the value of 
	 * the 'home_search_type' field.
	 */

	switch( motor->home_search_type ) {

	case MXHS_MTR_HOME_SEARCH_NOT_SUPPORTED:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Home searches are not supported for motor '%s'.",
			motor_record->name );
		break;

	case MXHS_MTR_RAW_HOME_COMMAND:
		mx_status = mx_motor_raw_home_command( motor_record,
							direction );
		break;

	case MXHS_MTR_SAME_DIR_LIMIT_SWITCH_THEN_RAW_HOME_COMMAND:
	case MXHS_MTR_OPPOSITE_DIR_LIMIT_SWITCH_THEN_RAW_HOME_COMMAND:
	case MXHS_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
	case MXHS_MTR_SAME_DIR_LIMIT_SWITCH_THEN_LIMIT_SWITCH_AS_HOME_SWITCH:
       case MXHS_MTR_OPPOSITE_DIR_LIMIT_SWITCH_THEN_LIMIT_SWITCH_AS_HOME_SWITCH:
		mx_status = mxp_motor_home_using_limit_switch(
						motor_record, motor,
						direction, flags,
						motor->home_search_type );
		break;

	case MXHS_MTR_HALFWAY_BETWEEN_LIMIT_SWITCHES:
		mx_status = mxp_motor_home_halfway_between_limit_switches(
						motor_record, motor,
						direction, flags );
		break;

	case MXHS_MTR_SPECIAL:
		special_fn = flist->special_home_search;

		if ( special_fn == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"A special home search was requested for motor '%s', "
			"but the motor driver '%s' used by that motor "
			"does not implement a special home search algorithm.",
				motor_record->name,
				mx_get_driver_name( motor_record ) );
		}

		mx_status = (*special_fn)( motor );
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Home search type %lu is not implemented for motor '%s'.",
			motor->home_search_type,
			motor_record->name );
		break;
	}
	
	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_get_limit_switch_as_home_switch( MX_RECORD *motor_record,
				long *limit_switch_as_home_switch )
{
	static const char fname[] =
		"mx_motor_get_limit_switch_as_home_switch()";

	MX_MOTOR *motor;
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_parameter( motor_record,
				MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_switch_as_home_switch != (long *) NULL ) {
		*limit_switch_as_home_switch =
			motor->limit_switch_as_home_switch;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_set_limit_switch_as_home_switch( MX_RECORD *motor_record,
				long limit_switch_as_home_switch )
{
	static const char fname[] =
		"mx_motor_set_limit_switch_as_home_switch()";

	MX_MOTOR *motor;
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->limit_switch_as_home_switch = limit_switch_as_home_switch;

	mx_status = mx_motor_set_parameter( motor_record,
				MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_zero_position_value( MX_RECORD *motor_record )
{
	mx_status_type status;

	status = mx_motor_set_position( motor_record, 0L );

	return status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_constant_velocity_move( MX_RECORD *motor_record, long direction )
{
	static const char fname[] = "mx_motor_constant_velocity_move()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	motor->latched_status = 0;

	fptr = fl_ptr->constant_velocity_move;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The '%s' driver for motor '%s' does not support constant velocity moves.",
			mx_get_driver_name( motor_record ),
			motor_record->name );
	}

	if ( motor->scale >= 0.0 ) {
		motor->constant_velocity_move = direction;
	} else {
		motor->constant_velocity_move = -direction;
	}

	status = ( *fptr ) ( motor );

	return status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_get_traditional_status( MX_MOTOR *motor )
{
	static const char fname[] = "mx_motor_get_traditional_status()";

	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	unsigned long new_motor_status;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type mx_status;

	/* We normally only get called if neither the get_status nor 
	 * get_extended_status driver functions are defined.  The fallback
	 * is to invoke the older busy and limit functions directly.
	 * We build up the motor status longword as we go.
	 */

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed is NULL." );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MOTOR pointer passed is NULL." );
	}

	fl_ptr = (MX_MOTOR_FUNCTION_LIST *)
			motor->record->class_specific_function_list;

	if ( fl_ptr == (MX_MOTOR_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MOTOR_FUNCTION_LIST pointer for record '%s' is NULL.",
			motor->record->name );
	}

	new_motor_status = 0;

	/* Get the motor busy status. */

	if ( fl_ptr->motor_is_busy == NULL ) {
		motor->busy = FALSE;
	} else {
		fptr = fl_ptr->motor_is_busy;

		mx_status = ( *fptr ) ( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( motor->busy ) {
			new_motor_status |= MXSF_MTR_IS_BUSY;
		}
	}

	/* Get the positive limit hit status. */

	if ( fl_ptr->positive_limit_hit == NULL ) {
		motor->positive_limit_hit = FALSE;
	} else {
		fptr = fl_ptr->positive_limit_hit;

		mx_status = ( *fptr ) ( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( motor->positive_limit_hit ) {
			new_motor_status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		}
	}

	/* Get the negative limit hit status. */

	if ( fl_ptr->negative_limit_hit == NULL ) {
		motor->negative_limit_hit = FALSE;
	} else {
		fptr = fl_ptr->negative_limit_hit;

		mx_status = ( *fptr ) ( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( motor->negative_limit_hit ) {
			new_motor_status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		}
	}

	motor->status = new_motor_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_motor_set_traditional_status( MX_MOTOR *motor )
{
	if ( motor->status & MXSF_MTR_IS_BUSY ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}
	if ( motor->status & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}
	if ( motor->status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}
	return;
}
		
/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_get_status( MX_RECORD *motor_record,
			unsigned long *motor_status )
{
	static const char fname[] = "mx_motor_get_status()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record,
						&motor, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor_record->name));

	/* Use the following strategy to get the motor status:
	 *
	 * 1.  Invoke the get_status driver function if available.
	 * 2.  Otherwise, invoke the get_extended_status driver function
	 *     if available.
	 * 3.  Otherwise, use the traditional motor_is_busy,
	 *     positive_limit_hit, and negative_limit_hit functions
	 *     directly.
	 * 4.  If this fails, set the status flags to something safe
	 *     and return.
	 */

	if ( fl_ptr->get_status != NULL ) {
		fptr = fl_ptr->get_status;
	} else
	if ( fl_ptr->get_extended_status != NULL ) {
		fptr = fl_ptr->get_extended_status;
	} else {
		fptr = NULL;
	}

	/* If one of the status functions is defined, invoke it
	 * and then return to the caller.
	 */

	if ( fptr == NULL ) {
		mx_status = mx_motor_get_traditional_status( motor );
	} else {
		mx_status = ( *fptr ) ( motor );

		(void) mx_motor_set_traditional_status( motor );
	}

	if ( motor->busy_start_interval_enabled ) {
		mx_bool_type busy_start_set;

		mx_status = mx_motor_check_busy_start_interval( motor_record,
							&busy_start_set );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy_start_set ) {
			motor->status |= MXSF_MTR_IS_BUSY;
			motor->busy = TRUE;
		}
	}

	/* Add latched status bits, if any. */

	if ( motor->latched_status ) {
		motor->status |= motor->latched_status;
	}

	/* If a home search was in progress, see if it is over. */

	if ( motor->home_search_in_progress ) {
		if ( ( motor->status & MXSF_MTR_IS_BUSY ) == 0 ) {
			motor->home_search_in_progress = FALSE;
		}
	}

	/* If any error are set, turn the error bit on. */

	if ( motor->status & MXSF_MTR_ERROR_BITMASK ) {
		motor->status |= MXSF_MTR_ERROR;
	}

	if ( motor->backlash_move_in_progress ) {
		if ( ( motor->status & MXSF_MTR_IS_BUSY ) == 0 ) {
			motor->backlash_move_in_progress = FALSE;
		}
	}

	if ( motor_status != NULL ) {
		*motor_status = motor->status;
	}

	return mx_status;
}

/*-----------------------------------------------------------------------*/

#define MXP_MOTOR_EXTENDED_STATUS_FORMAT   "%.*e %lx"

MX_EXPORT mx_status_type
mx_motor_get_extended_status( MX_RECORD *motor_record,
			double *motor_position,
			unsigned long *motor_status )
{
	static const char fname[] = "mx_motor_get_extended_status()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	int max_precision;
	double raw_position;
	mx_status_type ( *get_extended_status_fn ) ( MX_MOTOR * );
	mx_status_type ( *get_status_fn ) ( MX_MOTOR * );
	mx_status_type ( *get_position_fn ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record,
						&motor, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor_record->name));

	/* Use the following strategy to get the extended motor status:
	 *
	 * 1.  Invoke the get_extended_status driver function if available.
	 * 2.  Otherwise, invoke the get_position driver function to get
	 *     the motor position part of the extended status.
	 * 3.  In this case, invoke the get_status driver function if available.
	 * 4.  Otherwise, use the traditional methods to get the status bits.
	 */

	/* If the get_extended_status function exists, invoke it. */

	get_extended_status_fn = fl_ptr->get_extended_status;

	if ( get_extended_status_fn != NULL ) {
		mx_status = ( *get_extended_status_fn ) ( motor );
	} else {
		/*
		 * Get the motor status bits.  Abort if we do not succeed
		 * at this.
		 */

		get_status_fn = fl_ptr->get_status;

		if ( get_status_fn == NULL ) {
			mx_status = mx_motor_get_traditional_status( motor );
		} else {
			mx_status = ( *get_status_fn ) ( motor );

			(void) mx_motor_set_traditional_status( motor );
		}

		if ( mx_status.code != MXE_SUCCESS ) {
			if ( motor->backlash_move_in_progress ) {
				if (( motor->status & MXSF_MTR_IS_BUSY ) == 0) {
					motor->backlash_move_in_progress = FALSE;
				}
			}
			return mx_status;
		}

		/* Get the motor position. */

		get_position_fn = fl_ptr->get_position;

		if ( get_position_fn == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for motor '%s' does not define a get_position "
	"driver function.  This is likely to be a bug in that driver.",
				mx_get_driver_name( motor_record ),
				motor_record->name );
		}

		mx_status = ( *get_position_fn ) ( motor );
	}

	if ( motor->busy_start_interval_enabled ) {
		mx_bool_type busy_start_set;

		mx_status = mx_motor_check_busy_start_interval( motor_record,
							&busy_start_set );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy_start_set ) {
			motor->status |= MXSF_MTR_IS_BUSY;
			motor->busy = TRUE;
		}
	}

	/* Add latched status bits, if any. */

	if ( motor->latched_status ) {
		motor->status |= motor->latched_status;
	}

	/* If a home search was in progress, see if it is over. */

	if ( motor->home_search_in_progress ) {
		if ( ( motor->status & MXSF_MTR_IS_BUSY ) == 0 ) {
			motor->home_search_in_progress = FALSE;
		}
	}

	/*---*/

	if ( motor->backlash_move_in_progress ) {
		if ( ( motor->status & MXSF_MTR_IS_BUSY ) == 0 ) {
			motor->backlash_move_in_progress = FALSE;
		}
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If any error are set, turn the error bit on. */

	if ( motor->status & MXSF_MTR_ERROR_BITMASK ) {
		motor->status |= MXSF_MTR_ERROR;
	}

	/* Convert the raw position into user units. */

	if ( motor->subclass == MXC_MTR_STEPPER ) {
		raw_position = (double) motor->raw_position.stepper;
	} else {
		raw_position = motor->raw_position.analog;
	}

	motor->position = motor->offset + (motor->scale) * raw_position;

	if ( motor->use_internal_position_offset ) {
		motor->position += motor->internal_position_offset;
	}

	/* Save a string version of the extended status in the MX_MOTOR
	 * structure.
	 */

	max_precision = MXU_EXTENDED_STATUS_STRING_LENGTH - 20;

	if ( motor_record->precision > max_precision ) {
		motor_record->precision = max_precision;

	} else if ( motor_record->precision < 0 ) {
		motor_record->precision = 0;
	}

	snprintf( motor->extended_status, sizeof(motor->extended_status),
		MXP_MOTOR_EXTENDED_STATUS_FORMAT,
		motor_record->precision,
		motor->position, motor->status );

	/* Return status values to the caller if desired. */

	if ( motor_position != NULL ) {
		*motor_position = motor->position;
	}
	if ( motor_status != NULL ) {
		*motor_status = motor->status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_is_idle( MX_RECORD *motor_record,
		mx_bool_type *motor_is_idle )
{
	static const char fname[] = "mx_motor_is_idle()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	unsigned long motor_status, idle_status_test;
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record,
						&motor, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for motor '%s'.", fname, motor_record->name));

	mx_status = mx_motor_get_status( motor_record, &motor_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	idle_status_test = motor_status & ( ! MXSF_MTR_IDLE_BITMASK );

	if ( idle_status_test == 0 ) {
		*motor_is_idle = TRUE;
	} else {
		*motor_is_idle = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* Setting a speed limit to -1 means ignore that limit, while setting it
 * to any other negative value means the motor speed may not be changed.
 */

static mx_status_type
mx_motor_check_speed_limits( MX_MOTOR *motor,
				long speed_type,
				double proposed_speed )
{
	static const char fname[] = "mx_motor_check_speed_limits()";

	char speed_type_name[20];
	double proposed_raw_speed;
	long test_value;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed was NULL." );
	}

	/* Make sure the engineering limit values match the raw limits. */

	motor->maximum_speed_limit = motor->raw_maximum_speed_limit
					* fabs( motor->scale );

	motor->minimum_speed_limit = motor->raw_minimum_speed_limit
					* fabs( motor->scale );

	proposed_raw_speed = mx_divide_safely( proposed_speed,
						fabs(motor->scale) );

	switch( speed_type ) {
	case MXLV_MTR_SPEED:
		strlcpy( speed_type_name, "speed", sizeof(speed_type_name) );
		break;

	case MXLV_MTR_BASE_SPEED:
		strlcpy(speed_type_name, "base speed", sizeof(speed_type_name));
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported speed type %ld was passed for motor '%s'",
			speed_type, motor->record->name );
	}

	/* Check for negative and zero speeds. */

	if ( proposed_speed < 0.0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The %s of %g %s/sec requested for motor '%s' is illegal.  "
		"Motor speeds must not be negative numbers.",
			speed_type_name, proposed_speed, motor->units,
			motor->record->name );
	}

	/* Check the upper limit. */

	if ( motor->raw_maximum_speed_limit < 0.0 ) {

		test_value = mx_round( motor->raw_maximum_speed_limit );

		/* -1 means ignore the limit. */

		if ( test_value != -1 ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The %s of motor '%s' is not allowed to be changed.",
				speed_type_name, motor->record->name );
		}
	} else {
		if ( proposed_raw_speed > motor->raw_maximum_speed_limit ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The %s of %g %s/sec requested for '%s' is above the "
			"maximum allowed speed of %g %s/sec.",
				speed_type_name, proposed_speed, motor->units,
				motor->record->name,
				motor->maximum_speed_limit, motor->units );
		}
	}

	/* Check the lower limit. */

	if ( motor->raw_minimum_speed_limit < 0.0 ) {

		test_value = mx_round( motor->raw_minimum_speed_limit );

		/* -1 means ignore the limit. */

		if ( test_value != -1 ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The %s of motor '%s' is not allowed to be changed.",
				speed_type_name, motor->record->name );
		}
	} else {
		if ( proposed_raw_speed < motor->raw_minimum_speed_limit ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The %s of %g %s/sec requested for '%s' is below the "
			"minimum allowed speed of %g %s/sec.",
				speed_type_name, proposed_speed, motor->units,
				motor->record->name,
				motor->minimum_speed_limit, motor->units );
		}
	}

	/* Perform speed type specific checks. */

	switch( speed_type ) {
	case MXLV_MTR_SPEED:
		if ( proposed_raw_speed < motor->raw_base_speed ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The %s of %g %s/sec requested for '%s' is below the "
		"current base speed of %g %s/sec.  This is not allowed.",
				speed_type_name, proposed_speed, motor->units,
				motor->record->name,
				motor->scale * motor->raw_base_speed,
				motor->units );
		}
		break;

	case MXLV_MTR_BASE_SPEED:
		if ( proposed_raw_speed > motor->raw_speed ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The %s of %g %s/sec requested for '%s' is above the "
		"current speed of %g %s/sec.  This is not allowed.",
				speed_type_name, proposed_speed, motor->units,
				motor->record->name,
				motor->scale * motor->raw_speed,
				motor->units );
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported speed type %ld was passed for motor '%s'",
			speed_type, motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_get_parameter( MX_RECORD *motor_record, long parameter_type )
{
	static const char fname[] = "mx_motor_get_parameter()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The get_parameter function is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = parameter_type;

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_parameter( MX_RECORD *motor_record, long parameter_type )
{
	static const char fname[] = "mx_motor_set_parameter()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The set_parameter function is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = parameter_type;

	status = ( *fptr ) ( motor );

	return status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_default_get_parameter_handler( MX_MOTOR *motor )
{
	static const char fname[] = "mx_motor_default_get_parameter_handler()";

	double raw_speed, raw_base_speed;
	double squared_raw_speed, squared_raw_base_speed;
	double raw_acceleration, raw_acceleration_distance, acceleration_time;
	double raw_start_position, raw_end_position;
	double real_raw_start_position, real_raw_end_position;
	double est_speed, est_acceleration_time, est_acceleration_distance;
	double est_acceleration_time_x2, est_acceleration_distance_x2;
	double est_acceleration;
	double est_move_distance, est_distance_at_full_speed;
	double x_mid, t_mid;
	long i;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s', parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string(motor->record,motor->parameter_type),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_CURRENT_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
	case MXLV_MTR_AXIS_ENABLE:
	case MXLV_MTR_CLOSED_LOOP:
	case MXLV_MTR_FAULT_RESET:
	case MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
	case MXLV_MTR_TRIGGER_MODE:
	case MXLV_MTR_WINDOW_IS_AVAILABLE:
	case MXLV_MTR_USE_WINDOW:
	case MXLV_MTR_WINDOW:

		/* These do not require anything to be done. */

		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		motor->proportional_gain = 0.0;
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		motor->integral_gain = 0.0;
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		motor->derivative_gain = 0.0;
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		motor->velocity_feedforward_gain = 0.0;
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		motor->acceleration_feedforward_gain = 0.0;
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		motor->integral_limit = 0.0;
		break;

	case MXLV_MTR_EXTRA_GAIN:
		motor->extra_gain = 0.0;
		break;

	case MXLV_MTR_ACCELERATION_TYPE:
		motor->acceleration_type = MXF_MTR_ACCEL_NONE;
		break;

	case MXLV_MTR_ACCELERATION_TIME:

		/* Update the current values of the speed, base speed,
		 * and acceleration parameters.
		 */

		mx_status =
		    mx_motor_update_speed_and_acceleration_parameters( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* We assume the acceleration rate is constant.
		 * 
		 * The values in motor->raw_acceleration_parameters
		 * are in raw units, not user units.
		 */

		MX_DEBUG( 2,("%s: motor->raw_acceleration_parameters[0] = %g",
				fname, motor->raw_acceleration_parameters[0]));

		MX_DEBUG( 2,("%s: motor->raw_speed = %g",
				fname, motor->raw_speed));

		MX_DEBUG( 2,("%s: motor->raw_base_speed = %g",
				fname, motor->raw_base_speed));

		switch( motor->acceleration_type ) {
		case MXF_MTR_ACCEL_RATE:
			raw_speed = motor->raw_speed;

			raw_base_speed = motor->raw_base_speed;

			raw_acceleration =
				motor->raw_acceleration_parameters[0];

			acceleration_time = mx_divide_safely(
						raw_speed - raw_base_speed,
						raw_acceleration );

			motor->acceleration_time = fabs( acceleration_time );
			break;

		case MXF_MTR_ACCEL_TIME:
			motor->acceleration_time
				= fabs( motor->raw_acceleration_parameters[0] );

			break;

		case MXF_MTR_ACCEL_NONE:
			motor->acceleration_time = 0;
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
	"Cannot compute the acceleration time for motor '%s' since it has "
	"an unsupported acceleration type of %ld", motor->record->name,
						motor->acceleration_type );
		}

		MX_DEBUG( 2,("%s: motor->acceleration_time = %g",
			fname, motor->acceleration_time));

		break;

	case MXLV_MTR_ACCELERATION_DISTANCE:

		/* Update the current values of the speed, base speed,
		 * and acceleration parameters.
		 */

		mx_status =
		    mx_motor_update_speed_and_acceleration_parameters( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* We assume the acceleration rate is constant.
		 * 
		 * The values in motor->raw_acceleration_parameters
		 * are in raw units, not user units.
		 */

		switch( motor->acceleration_type ) {
		case MXF_MTR_ACCEL_RATE:
		case MXF_MTR_ACCEL_TIME:

			MX_DEBUG( 2,(
			    "%s: motor->raw_acceleration_parameters[0] = %g",
				fname, motor->raw_acceleration_parameters[0]));

			raw_speed = motor->raw_speed;

			raw_base_speed = motor->raw_base_speed;

			MX_DEBUG( 2,("%s: raw_speed = %g, raw_base_speed = %g",
				fname, raw_speed, raw_base_speed ));

			if ( motor->acceleration_type == MXF_MTR_ACCEL_RATE )
			{
				MX_DEBUG( 2,("%s: using accel rate",fname));

				squared_raw_speed = raw_speed * raw_speed;

				MX_DEBUG( 2,("%s: squared_raw_speed = %g",
						fname, squared_raw_speed));

				squared_raw_base_speed =
						raw_base_speed * raw_base_speed;

				MX_DEBUG( 2,("%s: squared_raw_base_speed = %g",
						fname, squared_raw_base_speed));

				raw_acceleration =
					motor->raw_acceleration_parameters[0];

				MX_DEBUG( 2,("%s: raw_acceleration = %g",
						fname, raw_acceleration));

				raw_acceleration_distance = mx_divide_safely(
				    squared_raw_speed - squared_raw_base_speed,
					2.0 * raw_acceleration );
			} else {
				MX_DEBUG( 2,("%s: using accel time",fname));

				acceleration_time = 
					motor->raw_acceleration_parameters[0];

				raw_acceleration_distance =
				    0.5 * ( raw_speed + raw_base_speed )
					* acceleration_time;
			}

			MX_DEBUG( 2,("%s: raw_acceleration_distance = %g",
				fname, raw_acceleration_distance));

			motor->raw_acceleration_distance
					= fabs( raw_acceleration_distance );

			motor->acceleration_distance
			    = fabs( raw_acceleration_distance * motor->scale );

			MX_DEBUG( 2,
				("%s: motor '%s', acceleration_distance = %g",
				fname, motor->record->name,
				motor->acceleration_distance));

			break;

		case MXF_MTR_ACCEL_NONE:
			motor->raw_acceleration_distance = 0;
			motor->acceleration_distance = 0;
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
	"Cannot compute the acceleration distance for motor '%s' since it has "
	"an unsupported acceleration type of %ld", motor->record->name,
						motor->acceleration_type );
		}
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		raw_start_position = motor->raw_compute_extended_scan_range[0];
		raw_end_position   = motor->raw_compute_extended_scan_range[1];

		mx_status = mx_motor_get_acceleration_distance(
							motor->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		raw_acceleration_distance = motor->raw_acceleration_distance;

		if ( raw_end_position >= raw_start_position ) {

			/* Scanning in the positive direction. */

			real_raw_start_position = raw_start_position
						- raw_acceleration_distance;

			real_raw_end_position = raw_end_position
						+ raw_acceleration_distance;
		} else {

			/* Scanning in the negative direction. */

			real_raw_start_position = raw_start_position
						+ raw_acceleration_distance;

			real_raw_end_position = raw_end_position
						- raw_acceleration_distance;
		}

		motor->raw_compute_extended_scan_range[2]
						= real_raw_start_position;

		motor->raw_compute_extended_scan_range[3]
						= real_raw_end_position;
		break;

	case MXLV_MTR_ESTIMATED_MOVE_DURATIONS:
	case MXLV_MTR_TOTAL_ESTIMATED_MOVE_DURATION:

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
		MX_DEBUG(-2,
		("%s: must_recalculate_estimated_move_duration = %d",
		fname, (int) motor->must_recalculate_estimated_move_duration));
#endif

		if ( motor->must_recalculate_estimated_move_duration == FALSE )
		{
			return MX_SUCCESSFUL_RESULT;
		}

		mx_status = mx_motor_get_speed( motor->record, &est_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_motor_get_acceleration_time( motor->record,
						&est_acceleration_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_motor_get_acceleration_distance( motor->record,
						&est_acceleration_distance );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
		MX_DEBUG(-2,("%s: est_speed = %g", fname, est_speed));
		MX_DEBUG(-2,("%s: est_acceleration_time = %g",
					fname, est_acceleration_time));
		MX_DEBUG(-2,("%s: est_acceleration_distance = %g",
					fname, est_acceleration_distance));
#endif

		est_acceleration_time_x2 = 2.0 * est_acceleration_time;
		est_acceleration_distance_x2 = 2.0 * est_acceleration_distance;

		/* estimated_move_durations[0] is set to zero on the principle
		 * that it takes 0 time to get to the starting position.
		 */

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
		MX_DEBUG(-2,("%s: motor->estimated_move_positions = %p",
			fname, motor->estimated_move_positions));
		MX_DEBUG(-2,("%s: motor->estimated_move_durations = %p",
			fname, motor->estimated_move_durations));

		MX_DEBUG(-2,("%s: motor->estimated_move_positions[0] = %f",
			fname, motor->estimated_move_positions[0]));
		MX_DEBUG(-2,("%s: motor->estimated_move_durations[0] = %f",
			fname, motor->estimated_move_durations[0]));
#endif

		motor->estimated_move_durations[0] = 0.0;

		motor->total_estimated_move_duration = 0.0;

		/* We skip i = 0, since we set the duration value for [0]
		 * just above.
		 */

		for ( i = 1; i < motor->num_estimated_move_positions; i++ ) {
			est_move_distance = fabs(
				motor->estimated_move_positions[i]
				- motor->estimated_move_positions[i-1] );

			est_distance_at_full_speed = est_move_distance
						- est_acceleration_distance_x2;

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
			MX_DEBUG(-2,
		("%s: est_move_distance = %g, est_distance_at_full_speed = %g",
	 		fname, est_move_distance, est_distance_at_full_speed));
#endif
			if ( est_distance_at_full_speed >= 0.0 ) {

			    if ( fabs(est_speed) < (2*FLT_MIN) ) {
				/* Give up if est_speed is too small. */

				motor->total_estimated_move_duration = 0.0;
				return MX_SUCCESSFUL_RESULT;
			    }

			    motor->estimated_move_durations[i] =
				mx_divide_safely( est_distance_at_full_speed,
							est_speed )
				+ est_acceleration_time_x2;
			} else {
			    /* If we get here, the motor does not get all
			     * the way to full speed before we have to start 
			     * decelerating.
			     *
			     * NOTE: This logic assumes a trapezoidal
			     * velocity profile.  The _mid suffix refers
			     * to the position, velocity, and time at the
			     * midpoint of the move.
			     */

			    if ( fabs(est_acceleration_time) < (2*FLT_MIN) ) {
				/* Give up if est_acceleration_time
				 * is too small.
				 */

				motor->total_estimated_move_duration = 0.0;
				return MX_SUCCESSFUL_RESULT;
			    }

			    est_acceleration =
			mx_divide_safely(est_speed, est_acceleration_time);

			    x_mid = 0.5 * est_move_distance;

			    t_mid = sqrt( mx_divide_safely( 2.0 * x_mid,
							est_acceleration ) );

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
			    MX_DEBUG(-2,
			("%s: est_acceleration = %g, x_mid = %g, t_mid = %g",
			 	fname, est_acceleration, x_mid, t_mid));
#endif

			    motor->estimated_move_durations[i] = 2.0 * t_mid;
			}

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
			MX_DEBUG(-2,("%s: estimated_move_durations[%ld] = %g",
				fname, i, motor->estimated_move_durations[i]));
#endif

			motor->total_estimated_move_duration
				+= motor->estimated_move_durations[i];
		}

		motor->must_recalculate_estimated_move_duration = FALSE;

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
		MX_DEBUG(-2,("%s: total_estimated_move_duration = %g",
				fname, motor->total_estimated_move_duration));
		MX_DEBUG(-2,
		("%s: must_recalculate_estimated_move_duration = %d",
		fname, (int) motor->must_recalculate_estimated_move_duration));
#endif
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%ld) is not supported by the MX driver for motor '%s'.",
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type,
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_motor_default_set_parameter_handler( MX_MOTOR *motor )
{
	static const char fname[] = "mx_motor_default_set_parameter_handler()";

	MX_RECORD_FIELD *estimated_move_positions_field = NULL;
	MX_RECORD_FIELD *estimated_move_durations_field = NULL;
	double start_position, end_position;
	double time_for_move, requested_raw_speed;
	double raw_speed, raw_base_speed, raw_acceleration;
	double old_saved_speed, current_speed;
	long max_num_estimated_move_positions;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s', parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string(motor->record,motor->parameter_type),
		motor->parameter_type));


	switch( motor->parameter_type ) {
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
	case MXLV_MTR_SAVE_START_POSITIONS:
	case MXLV_MTR_LIMIT_SWITCH_AS_HOME_SWITCH:
	case MXLV_MTR_TRIGGER_MODE:
	case MXLV_MTR_WINDOW_IS_AVAILABLE:
	case MXLV_MTR_WINDOW:

		/* By default, nothing more is done with these parameters
		 * other than saving their new values.
		 */

		break;

	case MXLV_MTR_USE_WINDOW:
		if ( motor->window_is_available == FALSE ) {
			motor->use_window = FALSE;

			return mx_error( MXE_NOT_AVAILABLE, fname,
			"Windowing of position capture is not available "
			"for motor '%s'.", motor->record->name );
		}
		break;

		/* Servo gains are only supported if the individual motor's
		 * driver explicitly has support for them.  Thus, the default
		 * handler ignores the value given to it and just sets the
		 * gain to zero.
		 */

	case MXLV_MTR_PROPORTIONAL_GAIN:
		motor->proportional_gain = 0.0;
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		motor->integral_gain = 0.0;
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		motor->derivative_gain = 0.0;
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		motor->velocity_feedforward_gain = 0.0;
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		motor->acceleration_feedforward_gain = 0.0;
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		motor->integral_limit = 0.0;
		break;

	case MXLV_MTR_EXTRA_GAIN:
		motor->extra_gain = 0.0;
		break;

	case MXLV_MTR_ACCELERATION_TIME:

		/* Update the current values of the speed, base speed,
		 * and acceleration parameters.
		 */

		mx_status =
		    mx_motor_update_speed_and_acceleration_parameters( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* We assume the acceleration rate is constant.
		 *
		 * The values in motor->raw_acceleration_parameters
		 * are in raw units, not user units.
		 */

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;

		switch( motor->acceleration_type ) {
		case MXF_MTR_ACCEL_RATE:
			raw_speed = motor->raw_speed;

			raw_base_speed = motor->raw_base_speed;

			raw_acceleration = mx_divide_safely(
						raw_speed - raw_base_speed,
						motor->acceleration_time );

			motor->raw_acceleration_parameters[0] =
					fabs( raw_acceleration );
			break;

		case MXF_MTR_ACCEL_TIME:
			motor->raw_acceleration_parameters[0]
				= fabs( motor->acceleration_time );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
	"Cannot set the acceleration time for motor '%s' since it has "
	"an unsupported acceleration type of %ld", motor->record->name,
						motor->acceleration_type );
		}

		mx_status = mx_motor_set_raw_acceleration_parameters(
						motor->record, NULL );
		break;

	case MXLV_MTR_USE_START_POSITIONS:
		/* Most motor drivers do not make use of the saved start
		 * position facility.
		 */

		motor->use_start_positions = FALSE;
		break;

	case MXLV_MTR_SPEED:
		motor->raw_speed = 0.0;
		break;

	case MXLV_MTR_BASE_SPEED:
		motor->raw_base_speed = 0.0;
		break;

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		start_position = motor->raw_speed_choice_parameters[0];
		end_position = motor->raw_speed_choice_parameters[1];
		time_for_move = motor->raw_speed_choice_parameters[2];

		/* We assume the move is at a constant speed in terms
		 * of the current motor.
		 */

		requested_raw_speed = mx_divide_safely(
					end_position - start_position,
					time_for_move );

		requested_raw_speed = fabs( requested_raw_speed );

		MX_DEBUG( 2,("%s: start_position = %g, end_position = %g",
			fname, start_position, end_position));
		MX_DEBUG( 2,("%s: time_for_move = %g, requested_raw_speed = %g",
			fname, time_for_move, requested_raw_speed));

		mx_status = mx_motor_set_raw_speed( motor->record,
						requested_raw_speed );

		return mx_status;
		break;

	case MXLV_MTR_SAVE_SPEED:
		/* Read the motor speed to make sure its value is current. */

		mx_status = mx_motor_get_speed( motor->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_saved_speed = motor->raw_speed;

		MX_DEBUG( 2,("%s (save): motor '%s' raw_saved_speed = %g",
			fname, motor->record->name, motor->raw_saved_speed));
		break;

	case MXLV_MTR_RESTORE_SPEED:

		MX_DEBUG( 2,("%s (restore #1): motor '%s' raw_saved_speed = %g",
			fname, motor->record->name, motor->raw_saved_speed));

		old_saved_speed = motor->raw_saved_speed * fabs(motor->scale);

		mx_status = mx_motor_set_raw_speed( motor->record,
						motor->raw_saved_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Read the current speed to see if the
		 * speed restoration succeeded.
		 */

		mx_status = mx_motor_get_speed( motor->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		current_speed = motor->raw_speed * fabs(motor->scale);

		MX_DEBUG( 2,
	    ("%s (restore #2): motor '%s', old speed = %g, current speed = %g",
		 	fname, motor->record->name,
			old_saved_speed, current_speed ));

		if ( mx_difference( old_saved_speed, current_speed ) > 0.001 )
		{
			/* If the old and the current speeds are different,
			 * return an error.
			 */

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to restore the speed of motor '%s' to "
			"its original value of %g %s/second failed.  "
			"Instead, the current speed of motor '%s' "
			"is %g %s/second.",
			    motor->record->name, old_saved_speed, motor->units,
			    motor->record->name, current_speed, motor->units );
		}
		return MX_SUCCESSFUL_RESULT;
		break;

	case MXLV_MTR_NUM_ESTIMATED_MOVE_POSITIONS:
		max_num_estimated_move_positions = 1000000L;

		if ( motor->num_estimated_move_positions >
				max_num_estimated_move_positions )
		{
			mx_status = mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"The requested number of estimated "
				"move positions (%ld) for motor '%s' is "
				"greater than the maximum allowed value "
				"of %ld.",
					motor->num_estimated_move_positions,
					motor->record->name,
					max_num_estimated_move_positions );

			motor->num_estimated_move_positions
					= motor->estimated_move_array_size;

			return mx_status;
		}

		if ( motor->num_estimated_move_positions >
				motor->estimated_move_array_size )
		{
			/* If needed, create new estimated move arrays
			 * big enough to handle the number of estimated
			 * move steps.  It is not necessary to copy the
			 * contents of the old arrays, since they are
			 * invalid once we send a new value for the
			 * field 'num_estimated_move_positions'.
			 */

			double *new_positions;
			double *new_durations;

			new_positions = (double *)
				    calloc( motor->num_estimated_move_positions,
						sizeof(double) );

			if ( new_positions == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
					"Ran out of memory trying to "
					"allocate a new_positions array for "
					"estimated move calculations for "
					"motor '%s'.", motor->record->name );
			}

			new_durations = (double *)
				    calloc( motor->num_estimated_move_positions,
						sizeof(double) );

			if ( new_durations == NULL ) {
				mx_free( new_positions );

				return mx_error( MXE_OUT_OF_MEMORY, fname,
					"Ran out of memory trying to "
					"allocate a new_durations array for "
					"estimated move calculations for "
					"motor '%s'.", motor->record->name );
			}

			mx_free( motor->estimated_move_positions );
			mx_free( motor->estimated_move_durations );

			motor->estimated_move_positions = new_positions;
			motor->estimated_move_durations = new_durations;

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
			MX_DEBUG(-2,
			("%s: Created new move position and duration arrays.",
			 	fname));
			MX_DEBUG(-2,("%s: motor->estimated_move_positions = %p",
				fname, motor->estimated_move_positions));
			MX_DEBUG(-2,("%s: motor->estimated_move_durations = %p",
				fname, motor->estimated_move_durations));

			MX_DEBUG(-2,
			("%s: motor->estimated_move_positions[0] = %f",
				fname, motor->estimated_move_positions[0]));
			MX_DEBUG(-2,
			("%s: motor->estimated_move_durations[0] = %f",
				fname, motor->estimated_move_durations[0]));
#endif

			motor->estimated_move_array_size =
					motor->num_estimated_move_positions;

			/* Fixup the record field data structures so that 
			 * they know the new sizes of the fields.
			 */

			mx_status = mx_find_record_field( motor->record,
					"estimated_move_positions",
					&estimated_move_positions_field );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			estimated_move_positions_field->dimension[0]
				= motor->num_estimated_move_positions;

			mx_status = mx_find_record_field( motor->record,
					"estimated_move_durations",
					&estimated_move_durations_field );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			estimated_move_durations_field->dimension[0]
				= motor->num_estimated_move_positions;
		}

		motor->must_recalculate_estimated_move_duration = TRUE;

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
		MX_DEBUG(-2,
		("%s: must_recalculate_estimated_move_duration = %d",
		fname, (int) motor->must_recalculate_estimated_move_duration));
#endif
		break;

	/*-------------------*/

	case MXLV_MTR_ESTIMATED_MOVE_POSITIONS:

		motor->must_recalculate_estimated_move_duration = TRUE;

#if MX_MOTOR_DEBUG_ESTIMATED_MOVE_DURATION
		MX_DEBUG(-2,
		("%s: must_recalculate_estimated_move_duration = %d",
		fname, (int) motor->must_recalculate_estimated_move_duration));
#endif
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%ld) is not supported by the MX driver for motor '%s'.",
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type,
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_update_speed_and_acceleration_parameters( MX_MOTOR *motor )
{
	mx_status_type mx_status;

	mx_status = mx_motor_get_parameter(motor->record, MXLV_MTR_SPEED);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_parameter(motor->record, MXLV_MTR_BASE_SPEED);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->acceleration_type == MXF_MTR_ACCEL_NONE ) {
		mx_status = mx_motor_get_parameter(motor->record,
					MXLV_MTR_ACCELERATION_TYPE);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_motor_get_parameter(motor->record,
					MXLV_MTR_RAW_ACCELERATION_PARAMETERS);

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_motor_get_speed( MX_RECORD *motor_record, double *speed )
{
	static const char fname[] = "mx_motor_get_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Getting the motor speed is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_SPEED;

	status = ( *fptr ) ( motor );

	motor->speed = fabs(motor->scale) * motor->raw_speed;

	if ( speed != NULL ) {
		*speed = motor->speed;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_speed( MX_RECORD *motor_record, double speed )
{
	static const char fname[] = "mx_motor_set_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Setting the motor speed is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	status = mx_motor_check_speed_limits( motor, MXLV_MTR_SPEED, speed );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Change the speed. */

	motor->parameter_type = MXLV_MTR_SPEED;

	motor->speed = speed;

	motor->raw_speed = mx_divide_safely(motor->speed, fabs(motor->scale));

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_get_base_speed( MX_RECORD *motor_record, double *base_speed )
{
	static const char fname[] = "mx_motor_get_base_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Getting the motor base speed is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_BASE_SPEED;

	status = ( *fptr ) ( motor );

	motor->base_speed = fabs(motor->scale) * motor->raw_base_speed;

	if ( base_speed != NULL ) {
		*base_speed = motor->base_speed;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_base_speed( MX_RECORD *motor_record, double base_speed )
{
	static const char fname[] = "mx_motor_set_base_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Setting the motor base speed is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	status = mx_motor_check_speed_limits( motor, MXLV_MTR_BASE_SPEED,
							base_speed );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Change the base speed. */

	motor->parameter_type = MXLV_MTR_BASE_SPEED;

	motor->base_speed = base_speed;

	motor->raw_base_speed = mx_divide_safely( motor->base_speed,
							fabs(motor->scale) );

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_get_maximum_speed( MX_RECORD *motor_record, double *maximum_speed )
{
	static const char fname[] = "mx_motor_get_maximum_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the motor maximum speed is not supported by "
			"the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_MAXIMUM_SPEED;

	status = ( *fptr ) ( motor );

	motor->maximum_speed = fabs(motor->scale) * motor->raw_maximum_speed;

	if ( maximum_speed != NULL ) {
		*maximum_speed = motor->maximum_speed;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_maximum_speed( MX_RECORD *motor_record, double maximum_speed )
{
	static const char fname[] = "mx_motor_set_maximum_speed()";

#if 1
	return mx_error( MXE_UNSUPPORTED, fname,
		"This function is currently disabled." );
#else
	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Setting the motor maximum speed is not supported by "
			"the driver for record '%s'.",
			motor_record->name );
	}

	status = mx_motor_check_speed_limits( motor, MXLV_MTR_MAXIMUM_SPEED,
							maximum_speed );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Change the maximum speed. */

	motor->parameter_type = MXLV_MTR_MAXIMUM_SPEED;

	motor->maximum_speed = maximum_speed;

	motor->raw_maximum_speed = mx_divide_safely( motor->maximum_speed,
							fabs(motor->scale) );

	status = ( *fptr ) ( motor );

	return status;
#endif
}

MX_EXPORT mx_status_type
mx_motor_get_current_speed( MX_RECORD *motor_record, double *current_speed )
{
	static const char fname[] = "mx_motor_get_current_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the current motor speed is not supported "
		"by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_CURRENT_SPEED;

	status = ( *fptr ) ( motor );

	motor->current_speed =
		fabs(motor->scale) * motor->raw_current_speed;

	if ( current_speed != NULL ) {
		*current_speed = motor->current_speed;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_get_raw_speed( MX_RECORD *motor_record, double *raw_speed )
{
	static const char fname[] = "mx_motor_get_raw_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Getting the motor speed is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_SPEED;

	status = ( *fptr ) ( motor );

	motor->speed = fabs(motor->scale) * motor->raw_speed;

	*raw_speed = motor->raw_speed;

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_raw_speed( MX_RECORD *motor_record, double raw_speed )
{
	static const char fname[] = "mx_motor_set_raw_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	double motor_speed;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Setting the motor speed is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor_speed = raw_speed * fabs(motor->scale);

	status = mx_motor_check_speed_limits( motor,
					MXLV_MTR_SPEED, motor_speed );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Change the speed. */

	motor->parameter_type = MXLV_MTR_SPEED;

	motor->speed = motor_speed;

	motor->raw_speed = raw_speed;

	status = ( *fptr ) ( motor );

	return status;
}

/*
 * mx_motor_save_speed() saves both the speed and the value of
 * motor->synchronous_motion_mode.
 */

MX_EXPORT mx_status_type
mx_motor_save_speed( MX_RECORD *motor_record )
{
	static const char fname[] = "mx_motor_save_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Saving the motor speed is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->saved_synchronous_motion_mode = motor->synchronous_motion_mode;

	motor->parameter_type = MXLV_MTR_SAVE_SPEED;

	status = ( *fptr ) ( motor );

	return status;
}

/*
 * mx_motor_restore_speed() restores both the speed and the value of
 * motor->synchronous_motion_mode.
 */

MX_EXPORT mx_status_type
mx_motor_restore_speed( MX_RECORD *motor_record )
{
	static const char fname[] = "mx_motor_restore_speed()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Restoring the motor speed is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_RESTORE_SPEED;

	status = ( *fptr ) ( motor );

	if ( status.code != MXE_SUCCESS )
		return status;

	motor->synchronous_motion_mode = motor->saved_synchronous_motion_mode;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_motor_get_synchronous_motion_mode( MX_RECORD *motor_record,
					mx_bool_type *synchronous_motion_mode )
{
	static const char fname[] = "mx_motor_get_synchronous_motion_mode()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the synchronous motion mode is not supported "
			"by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_SYNCHRONOUS_MOTION_MODE;

	status = ( *fptr ) ( motor );

	if ( synchronous_motion_mode != NULL ) {
		*synchronous_motion_mode = motor->synchronous_motion_mode;
	}
	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_synchronous_motion_mode( MX_RECORD *motor_record,
					mx_bool_type synchronous_motion_mode )
{
	static const char fname[] = "mx_motor_set_synchronous_motion_mode()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Setting the synchronous motion mode is not supported "
			"by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_SYNCHRONOUS_MOTION_MODE;

	motor->synchronous_motion_mode = synchronous_motion_mode;

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_speed_between_positions( MX_RECORD *motor_record,
					double position1, double position2,
					double time_for_move )
{
	static const char fname[] = "mx_motor_set_speed_between_positions()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Setting the motor speed between two positions is not "
		"supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->speed_choice_parameters[0] = position1;
	motor->speed_choice_parameters[1] = position2;
	motor->speed_choice_parameters[2] = time_for_move;

	motor->raw_speed_choice_parameters[0] =
			mx_divide_safely( position1, motor->scale );

	motor->raw_speed_choice_parameters[1] =
			mx_divide_safely( position2, motor->scale );

	motor->raw_speed_choice_parameters[2] = time_for_move;

	motor->parameter_type = MXLV_MTR_SPEED_CHOICE_PARAMETERS;

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_get_acceleration_type( MX_RECORD *motor_record,
				long *acceleration_type )
{
	static const char fname[] = "mx_motor_get_acceleration_type()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Getting the acceleration type is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_ACCELERATION_TYPE;

	status = ( *fptr ) ( motor );

	if ( acceleration_type != NULL ) {
		*acceleration_type = motor->acceleration_type;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_get_raw_acceleration_parameters( MX_RECORD *motor_record,
				double *raw_acceleration_parameter_array )
{
	static const char fname[] =
				"mx_motor_get_raw_acceleration_parameters()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;
	int i;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Getting acceleration parameters is not supported by "
			"the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_RAW_ACCELERATION_PARAMETERS;

	status = ( *fptr ) ( motor );

	if ( raw_acceleration_parameter_array != (double *) NULL ) {
		for ( i = 0; i < MX_MOTOR_NUM_ACCELERATION_PARAMS; i++ ) {
			raw_acceleration_parameter_array[i] =
				motor->raw_acceleration_parameters[i];
		}
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_raw_acceleration_parameters( MX_RECORD *motor_record,
				double *raw_acceleration_parameter_array )
{
	static const char fname[] =
				"mx_motor_set_raw_acceleration_parameters()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;
	int i;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Setting acceleration parameters is not supported "
			"by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_RAW_ACCELERATION_PARAMETERS;

	if ( raw_acceleration_parameter_array != (double *) NULL ) {
		for ( i = 0; i < MX_MOTOR_NUM_ACCELERATION_PARAMS; i++ ) {
			motor->raw_acceleration_parameters[i] =
				raw_acceleration_parameter_array[i];
		}
	}

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_get_acceleration_time( MX_RECORD *motor_record,
				double *acceleration_time )
{
	static const char fname[] = "mx_motor_get_acceleration_time()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Getting the acceleration time is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_ACCELERATION_TIME;

	status = ( *fptr ) ( motor );

	if ( acceleration_time != (double *) NULL ) {
		*acceleration_time = motor->acceleration_time;
	}

	MX_DEBUG( 2,("%s: motor '%s' acceleration time = %g",
		fname, motor_record->name, motor->acceleration_time));

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_acceleration_time( MX_RECORD *motor_record,
				double acceleration_time )
{
	static const char fname[] = "mx_motor_set_acceleration_time()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Setting the acceleration time is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_ACCELERATION_TIME;

	motor->acceleration_time = acceleration_time;

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_get_acceleration_distance( MX_RECORD *motor_record,
				double *acceleration_distance )
{
	static const char fname[] = "mx_motor_get_acceleration_distance()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the acceleration distance is not supported "
			"by the driver for record '%s'.",
			motor_record->name );
	}

	motor->parameter_type = MXLV_MTR_ACCELERATION_DISTANCE;

	status = ( *fptr ) ( motor );

	if ( acceleration_distance != (double *) NULL ) {
		*acceleration_distance = motor->acceleration_distance;
	}

	MX_DEBUG( 2,("%s: motor '%s' acceleration distance = %g",
		fname, motor_record->name, motor->acceleration_distance));

	return status;
}

MX_EXPORT mx_status_type
mx_motor_send_control_command( MX_RECORD *motor_record,
				int command_type, int command )
{
	static const char fname[] = "mx_motor_send_control_command()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->latched_status = 0;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"Sending control commands is not supported by the driver for motor '%s'.",
			motor_record->name );
	}

	switch( command_type ) {
	case MXLV_MTR_AXIS_ENABLE:
		motor->axis_enable = command;
		break;
	case MXLV_MTR_CLOSED_LOOP:
		motor->closed_loop = command;
		break;
	case MXLV_MTR_FAULT_RESET:
		motor->fault_reset = command;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported command type %d requested for motor '%s'.",
			command_type, motor_record->name );
	}

	motor->parameter_type = command_type;

	mx_status = ( *fptr ) ( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_set_busy_start_interval( MX_RECORD *motor_record,
				double busy_start_interval_in_seconds )
{
	static const char fname[] = "mx_motor_set_busy_start_interval()";

	MX_MOTOR *motor;

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor record pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) motor_record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.",
			motor_record->name );
	}

	motor->busy_start_interval = busy_start_interval_in_seconds;

	if ( busy_start_interval_in_seconds < 0.0 ) {
		motor->busy_start_interval_enabled = FALSE;
	} else {
		motor->busy_start_interval_enabled = TRUE;

		motor->busy_start_ticks =
	    mx_convert_seconds_to_clock_ticks( busy_start_interval_in_seconds );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* If the difference between the current time and the start time of the
 * last move is less than the value of 'busy_start_interval', then the
 * flag busy_start_set will be set to TRUE.
 *
 * There exist motor controllers that have the misfeature that when you
 * send them a 'move' command, there may be a short time after the start
 * of the move where the controller reports that the motor is _not_ moving.
 *
 * While this may technically be true, it is not useful for higher level
 * logic that is trying to figure out when a commanded move has completed.
 * MX handles this by reporting that the motor is 'busy' for a period
 * after a move command equal to the value of 'motor->busy_start_interval'
 * in seconds.  This is only done if the busy start feature is enabled.
 */

MX_EXPORT mx_status_type
mx_motor_check_busy_start_interval( MX_RECORD *motor_record,
				mx_bool_type *busy_start_set )
{
	static const char fname[] = "mx_motor_check_busy_start_interval()";

	MX_MOTOR *motor;
	MX_CLOCK_TICK start_tick, busy_start_ticks;
	MX_CLOCK_TICK finish_tick, current_tick;
	int comparison;

	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor record pointer passed was NULL." );
	}
	if ( busy_start_set == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The busy_start_set pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) motor_record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.",
			motor_record->name );
	}

	if ( motor->busy_start_interval_enabled == FALSE ) {
		*busy_start_set = FALSE;
		return MX_SUCCESSFUL_RESULT;
	}

	motor->last_start_time =
		mx_convert_clock_ticks_to_seconds( motor->last_start_tick );

	start_tick = motor->last_start_tick;

	busy_start_ticks = motor->busy_start_ticks;

	finish_tick = mx_add_clock_ticks( start_tick, busy_start_ticks );

	current_tick = mx_current_clock_tick();

	comparison = mx_compare_clock_ticks( current_tick, finish_tick );

	if ( comparison <= 0 ) {
		*busy_start_set = TRUE;
	} else {
		*busy_start_set = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_motor_get_gain( MX_RECORD *motor_record, int gain_type, double *gain )
{
	static const char fname[] = "mx_motor_get_gain()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"Getting gains is not supported by the driver for motor '%s'.",
			motor_record->name );
	}

	motor->parameter_type = gain_type;

	mx_status = ( *fptr ) ( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gain != NULL ) {
		switch( gain_type ) {
		case MXLV_MTR_PROPORTIONAL_GAIN:
			*gain = motor->proportional_gain;
			break;
		case MXLV_MTR_INTEGRAL_GAIN:
			*gain = motor->integral_gain;
			break;
		case MXLV_MTR_DERIVATIVE_GAIN:
			*gain = motor->derivative_gain;
			break;
		case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
			*gain = motor->velocity_feedforward_gain;
			break;
		case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
			*gain = motor->acceleration_feedforward_gain;
			break;
		case MXLV_MTR_INTEGRAL_LIMIT:
			*gain = motor->integral_limit;
			break;
		case MXLV_MTR_EXTRA_GAIN:
			*gain = motor->extra_gain;
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported gain type %d requested for motor '%s'.",
				gain_type, motor_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_motor_set_gain( MX_RECORD *motor_record, int gain_type, double gain )
{
	static const char fname[] = "mx_motor_set_gain()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"Setting gains is not supported by the driver for motor '%s'.",
			motor_record->name );
	}

	switch( gain_type ) {
	case MXLV_MTR_PROPORTIONAL_GAIN:
		motor->proportional_gain = gain;
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		motor->integral_gain = gain;
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		motor->derivative_gain = gain;
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		motor->velocity_feedforward_gain = gain;
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		motor->acceleration_feedforward_gain = gain;
		break;
	case MXLV_MTR_INTEGRAL_LIMIT:
		motor->integral_limit = gain;
		break;
	case MXLV_MTR_EXTRA_GAIN:
		motor->extra_gain = gain;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported gain type %d requested for motor '%s'.",
			gain_type, motor_record->name );
	}

	motor->parameter_type = gain_type;

	mx_status = ( *fptr ) ( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_home_search_succeeded( MX_RECORD *motor_record,
				mx_bool_type *home_search_succeeded )
{
	static const char fname[] = "mx_motor_is_at_home_switch()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *get_status_fn ) ( MX_MOTOR * );
	mx_status_type ( *get_extended_status_fn ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_status_fn = fl_ptr->get_status;

	get_extended_status_fn = fl_ptr->get_extended_status;

	if ( get_status_fn != NULL ) {
		status = ( *get_status_fn )( motor );
	} else
	if ( get_extended_status_fn != NULL ) {
		status = ( *get_extended_status_fn )( motor );
	} else {
		/* No status functions are defined, so we report that
		 * the home search did not succeed.
		 */

		motor->status &= ( ~ MXSF_MTR_HOME_SEARCH_SUCCEEDED );
	}

	/* Be sure to add in any latched status bits. */

	motor->status |= motor->latched_status;

	if ( home_search_succeeded != NULL ) {
		if ( motor->status & MXSF_MTR_HOME_SEARCH_SUCCEEDED ) {
			*home_search_succeeded = TRUE;
		} else {
			*home_search_succeeded = FALSE;
		}
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_compute_extended_scan_range( MX_RECORD *motor_record,
					double start_position,
					double end_position,
					double *real_start_position,
					double *real_end_position )
{
	static const char fname[] = "mx_motor_compute_extended_scan_range()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Computing an extended scan range is not supported "
			"by the driver for record '%s'.",
			motor_record->name );
	}

	MX_DEBUG( 2,("%s: motor '%s', start_position = %g, end_position = %g",
		fname, motor_record->name, start_position, end_position));

	motor->compute_extended_scan_range[0] = start_position;
	motor->compute_extended_scan_range[1] = end_position;

	motor->raw_compute_extended_scan_range[0] =
			mx_divide_safely( start_position, motor->scale );

	motor->raw_compute_extended_scan_range[1] =
			mx_divide_safely( end_position, motor->scale );

	motor->parameter_type = MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE;

	status = ( *fptr ) ( motor );

	motor->compute_extended_scan_range[2] = motor->scale *
				motor->raw_compute_extended_scan_range[2];

	motor->compute_extended_scan_range[3] = motor->scale *
				motor->raw_compute_extended_scan_range[3];

#if 0
	{
		int i;

		for ( i = 0; i < 4; i++ ) {
			MX_DEBUG(-2,
		("%s: motor '%s', raw_compute_extended_scan_range[%d] = %g",
			fname, motor_record->name, i,
			motor->raw_compute_extended_scan_range[i]));
		}
		for ( i = 0; i < 4; i++ ) {
			MX_DEBUG(-2,
		("%s: motor '%s', compute_extended_scan_range[%d] = %g",
			fname, motor_record->name, i,
			motor->compute_extended_scan_range[i]));
		}
	}
#endif

	if ( real_start_position != (double *) NULL ) {
		*real_start_position = motor->compute_extended_scan_range[2];
	}

	if ( real_end_position != (double *) NULL ) {
		*real_end_position = motor->compute_extended_scan_range[3];
	}

	return status;
}

MX_EXPORT mx_status_type
mx_motor_compute_pseudomotor_position_from_real_position(
					MX_RECORD *motor_record,
					double real_position,
					double *pseudomotor_position,
					int recursion_flag )
{
	static const char fname[] =
		"mx_motor_compute_pseudomotor_position_from_real_position()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	MX_RECORD *next_level_motor_record;
	double next_level_pseudomotor_position;
	mx_status_type status;

	static int level = 0;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	level++;

	MX_DEBUG( 2,
	("%s(%d): motor '%s', real_position = %g, recursion_flag = %d",
	fname, level, motor_record->name, real_position, recursion_flag ));

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		level--;

		return mx_error( MXE_UNSUPPORTED, fname,
			"Computing a pseudomotor position from a real position "
			"is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	/* Perform special processing depending on what flags are set
	 * for this motor record.
	 */

	if ( motor->motor_flags & MXF_MTR_IS_PSEUDOMOTOR ) {

		/* If this is a pseudomotor and recursion is turned on,
		 * we must recurse down to the next lower layer of pseudo-
		 * motor.  We find the next level motor record by using
		 * the motor->real_motor_record pointer.
		 */

		if ( recursion_flag ) {
			next_level_motor_record = motor->real_motor_record;

			if ( next_level_motor_record == (MX_RECORD *)NULL ) {
				level--;

				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Cannot compute a unique pseudomotor position for motor '%s' since this type "
"of pseudomotor does not support it.", motor_record->name );
			}

			status =
		    mx_motor_compute_pseudomotor_position_from_real_position(
				next_level_motor_record,
				real_position,
				&next_level_pseudomotor_position,
				recursion_flag );

			if ( status.code != MXE_SUCCESS ) {
				level--;

				return status;
			}

			motor->compute_pseudomotor_position[0]
					= next_level_pseudomotor_position;
		} else {
			motor->compute_pseudomotor_position[0] = real_position;
		}

	} else if ( motor->motor_flags & MXF_MTR_IS_REMOTE_MOTOR ) {

		motor->compute_pseudomotor_position[0] = real_position;

	} else {
		/* If this is not a pseudomotor or a remote motor, we assume
		 * that this is a real motor and return without performing
		 * any transformation on the position.
		 */

		*pseudomotor_position = real_position;

		motor->compute_pseudomotor_position[1] = real_position;

		MX_DEBUG( 2,("%s(%d,real): *pseudomotor_position = %g",
			fname, level, *pseudomotor_position ));

		level--;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Perform this level of pseudomotor (or remote motor) conversion. */

	motor->compute_pseudomotor_position[1] = 0.0;

	motor->parameter_type = MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION;

	status = ( *fptr ) ( motor );

	*pseudomotor_position = motor->compute_pseudomotor_position[1];

	if ( motor->motor_flags & MXF_MTR_IS_REMOTE_MOTOR ) {
		MX_DEBUG( 2,("%s(%d,remote): pseudomotor_position = %g",
			fname, level, *pseudomotor_position ));
	} else {
		MX_DEBUG( 2,("%s(%d,pseudo): pseudomotor_position = %g",
			fname, level, *pseudomotor_position ));
	}

	level--;

	return status;
}

MX_EXPORT mx_status_type
mx_motor_compute_real_position_from_pseudomotor_position(
					MX_RECORD *motor_record,
					double pseudomotor_position,
					double *real_position,
					int recursion_flag )
{
	static const char fname[] =
		"mx_motor_compute_real_position_from_pseudomotor_position()";

	static int level = 0;

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	MX_RECORD *next_level_motor_record;
	double next_level_real_position;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	level++;

	MX_DEBUG( 2,
	("%s(%d): motor '%s', pseudomotor_position = %g, recursion_flag = %d",
		fname, level, motor_record->name, pseudomotor_position,
		recursion_flag ));

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		level--;

		return mx_error( MXE_UNSUPPORTED, fname,
		"Computing a real motor position from a pseudomotor "
		"position is not supported by the driver for record '%s'.",
			motor_record->name );
	}

	motor->compute_real_position[0] = pseudomotor_position;
	motor->compute_real_position[1] = 0.0;

	/* If this motor is not a pseudomotor or a remote motor, we can
	 * return immediately without performing any transformations
	 * on the position.
	 */

	if ( ((motor->motor_flags & MXF_MTR_IS_PSEUDOMOTOR) == 0)
	  && ((motor->motor_flags & MXF_MTR_IS_REMOTE_MOTOR) == 0) )
	{
		*real_position = pseudomotor_position;

		motor->compute_real_position[1] = pseudomotor_position;

		MX_DEBUG( 2,("%s(%d,real): *real_position = %g",
			fname, level, *real_position ));

		level--;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Perform this level of pseudomotor (or remote motor) conversion. */

	motor->parameter_type = MXLV_MTR_COMPUTE_REAL_POSITION;

	status = ( *fptr ) ( motor );

	if ( status.code != MXE_SUCCESS ) {
		level--;
		return status;
	}

	*real_position = motor->compute_real_position[1];

	/* If this is a remote motor, no further transformation is
	 * necessary and we return.
	 */

	if ( motor->motor_flags & MXF_MTR_IS_REMOTE_MOTOR ) {
		level--;

		MX_DEBUG( 2,("%s(%d,remote): *real_position = %g",
			fname, level, *real_position));

		return MX_SUCCESSFUL_RESULT;
	}

	/* If recursion is turned on, we must recurse down to the next
	 * lower layer of pseudomotor.
	 */

	if ( recursion_flag ) {

		next_level_motor_record = motor->real_motor_record;

		if ( next_level_motor_record == (MX_RECORD *) NULL ) {
			level--;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Cannot compute a unique real position for motor '%s' since this type "
"of pseudomotor does not support it.", motor_record->name );
		}

		status
		  = mx_motor_compute_real_position_from_pseudomotor_position(
				next_level_motor_record,
				*real_position,
				&next_level_real_position,
				recursion_flag );

		if ( status.code != MXE_SUCCESS ) {
			level--;
			return status;
		}

		*real_position = next_level_real_position;
	}

	motor->compute_real_position[1] = *real_position;

	MX_DEBUG( 2,("%s(%d,pseudo): *real_position = %g",
			fname, level, *real_position));

	level--;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_motor_get_real_motor_record( MX_RECORD *pseudomotor_record,
					MX_RECORD **real_motor_record )
{
	static const char fname[] = "mx_motor_get_real_motor_record()";

	MX_MOTOR *pseudomotor;
	MX_RECORD *next_level_real_motor_record;
	mx_status_type mx_status;

	if ( pseudomotor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pseudomotor_record pointer passed was NULL." );
	}
	if ( real_motor_record == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The real_motor_record pointer passed was NULL." );
	}

	*real_motor_record = NULL;

	if ( (pseudomotor_record->mx_superclass != MXR_DEVICE)
	  || (pseudomotor_record->mx_class != MXC_MOTOR) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The record '%s' passed is not a motor record.  "
		"This is due either to a bug in an MX driver or a "
		"bug in the MX database.",
			pseudomotor_record->name );
	}

	pseudomotor = (MX_MOTOR *) pseudomotor_record->record_class_struct;

	if ( pseudomotor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for motor record '%s' is NULL.",
			pseudomotor_record->name );
	}

	MX_DEBUG( 2,("%s: record '%s', pseudomotor->motor_flags = %lu",
		fname, pseudomotor_record->name, pseudomotor->motor_flags));

	if ( (pseudomotor->motor_flags & MXF_MTR_IS_PSEUDOMOTOR) == 0 ) {

		/* If this record is not a pseudomotor, then we have found
		 * the real motor record we were looking for, so return here.
		 */

		*real_motor_record = pseudomotor_record;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( pseudomotor->real_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The pseudomotor->real_motor_record pointer for record '%s' is NULL.  "
	"This means either that this record is not a pseudomotor record, "
	"or that the driver for this pseudomotor either has not yet been "
	"converted to being compatible with quick scanning.",
			pseudomotor_record->name );
	}

	/* We must recursively descend to the next lower pseudomotor. */

	mx_status = mx_motor_get_real_motor_record(
				pseudomotor->real_motor_record,
				&next_level_real_motor_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*real_motor_record = next_level_real_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_alternate_motor_can_use_this_motors_mce( MX_RECORD *motor_record,
					MX_RECORD *alternate_motor_record,
					int *motor_is_compatible )
{
	static const char fname[] =
		"mx_alternate_motor_can_use_this_motors_mce()";

	MX_RECORD *real_motor_record, *alternate_real_motor_record;
	mx_status_type mx_status;

	if ( motor_is_compatible == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor_is_compatible pointer passed is NULL." );
	}

	if ( ( motor_record == (MX_RECORD *) NULL )
	  || ( alternate_motor_record == (MX_RECORD *) NULL ) )
	{
		*motor_is_compatible = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_motor_get_real_motor_record( motor_record,
						&real_motor_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_real_motor_record( alternate_motor_record,
						&alternate_real_motor_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( alternate_real_motor_record == real_motor_record ) {
		*motor_is_compatible = TRUE;
	} else {
		*motor_is_compatible = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* For some pseudomotor types, this instructs the motor driver to compute 
 * start positions for the real motors that the pseudomotor is built from
 * for later use by a pseudomotor step scan.  This function will be ignored
 * by most motor drivers.  As of September 2002, it is only used by network,
 * slit, and translation pseudomotors.
 */
 
MX_EXPORT mx_status_type
mx_motor_save_start_positions( MX_RECORD *motor_record, double start_position )
{
	static const char fname[] = "mx_motor_save_start_positions()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG( 2,("%s: motor '%s' invoked for %g %s",
		fname, motor_record->name, start_position, motor->units ));

	/* Save the pseudomotor start positions. */

	motor->parameter_type = MXLV_MTR_SAVE_START_POSITIONS;

	motor->save_start_positions = start_position;

	motor->raw_saved_start_position = mx_divide_safely( start_position,
							motor->scale );

	status = ( *fptr ) ( motor );

	return status;
}

/* This function tells some pseudomotors to use start positions previously
 * saved by mx_motor_save_start_positions() when computing the real motor
 * positions for the next move.
 */

MX_EXPORT mx_status_type
mx_motor_use_start_positions( MX_RECORD *motor_record )
{
	static const char fname[] = "mx_motor_use_start_positions()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG( 2,("%s invoked for motor '%s'.", fname, motor_record->name ));

	/* Use the pseudomotor start position for the next commanded move. */

	motor->parameter_type = MXLV_MTR_USE_START_POSITIONS;

	motor->use_start_positions = TRUE;

	status = ( *fptr ) ( motor );

	return status;
}

/*----*/

MX_EXPORT mx_status_type
mx_motor_setup_triggered_move( MX_RECORD *motor_record,
				double triggered_move_destination )
{
	static const char fname[] = "mx_motor_setup_triggered_move()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *flist;
	mx_status_type ( *setup_triggered_move_fn ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
						&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	setup_triggered_move_fn = flist->setup_triggered_move;

	if ( setup_triggered_move_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Triggered moves are not supported by the driver '%s' "
		"for motor '%s'.",
			 mx_get_driver_name( motor_record ),
			motor_record->name );
	}

	MX_DEBUG(-2,("%s invoked for motor '%s'.", fname, motor_record->name ));

	motor->triggered_move_destination = triggered_move_destination;

	mx_status = ( *setup_triggered_move_fn ) ( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_trigger_move( MX_RECORD *motor_record )
{
	static const char fname[] = "mx_motor_trigger_move()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *flist;
	mx_status_type ( *trigger_move_fn ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
						&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger_move_fn = flist->trigger_move;

	if ( trigger_move_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Triggered moves are not supported by the driver '%s' "
		"for motor '%s'.",
			 mx_get_driver_name( motor_record ),
			motor_record->name );
	}

	MX_DEBUG(-2,("%s invoked for motor '%s'.", fname, motor_record->name ));

	mx_status = ( *trigger_move_fn ) ( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_get_trigger_mode( MX_RECORD *motor_record, long *trigger_mode )
{
	static const char fname[] = "mx_motor_get_trigger_mode()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
						&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG(-2,("%s invoked for motor '%s'.", fname, motor_record->name ));

	motor->parameter_type = MXLV_MTR_TRIGGER_MODE;

	mx_status = ( *get_parameter_fn ) ( motor );

	if ( trigger_mode != (long *) NULL ) {
		*trigger_mode = motor->trigger_mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_set_trigger_mode( MX_RECORD *motor_record, long trigger_mode )
{
	static const char fname[] = "mx_motor_set_trigger_mode()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
						&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG(-2,("%s invoked for motor '%s'.", fname, motor_record->name ));

	motor->parameter_type = MXLV_MTR_TRIGGER_MODE;

	motor->trigger_mode = trigger_mode;

	mx_status = ( *set_parameter_fn ) ( motor );

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mx_motor_get_window( MX_RECORD *motor_record,
			double *window,
			long num_window_parameters )
{
	static const char fname[] = "mx_motor_get_window()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *function_list;
	size_t window_bytes, max_window_bytes, bytes_left;
	char *ptr;
	mx_status_type ( *get_parameter ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->parameter_type = MXLV_MTR_WINDOW;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		mx_status = mx_motor_default_get_parameter_handler( motor );

		return mx_status;
	}

	mx_status = ( *get_parameter) ( motor );

	if ( window != NULL ) {
		max_window_bytes =
			MXU_MTR_NUM_WINDOW_PARAMETERS * sizeof(double);

		memset( window, 0, max_window_bytes );

		if ( num_window_parameters > MXU_MTR_NUM_WINDOW_PARAMETERS ) {
			num_window_parameters = MXU_MTR_NUM_WINDOW_PARAMETERS;
		}

		window_bytes = num_window_parameters * sizeof(double);

		/* Allow for the possibility that motor->window == window. */

		memmove( window, motor->window, window_bytes );

		if ( window_bytes < max_window_bytes ) {
			bytes_left = max_window_bytes - window_bytes;

			ptr = ((char *) window) + window_bytes;

			memset( ptr, 0, bytes_left );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_set_window( MX_RECORD *motor_record,
			double *window,
			long num_window_parameters )
{
	static const char fname[] = "mx_motor_set_window()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *function_list;
	size_t window_bytes, max_window_bytes, bytes_left;
	char *ptr;
	mx_status_type ( *set_parameter ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( window == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The window pointer passed was NULL." );
	}

	motor->parameter_type = MXLV_MTR_WINDOW;

	/*----*/

	max_window_bytes = MXU_MTR_NUM_WINDOW_PARAMETERS * sizeof(double);

	if ( num_window_parameters > MXU_MTR_NUM_WINDOW_PARAMETERS ) {
		num_window_parameters = MXU_MTR_NUM_WINDOW_PARAMETERS;
	}

	window_bytes = num_window_parameters * sizeof(double);

	/* Allow for the possibility that motor->window == window. */

	memmove( motor->window, window, window_bytes );

	if ( window_bytes < max_window_bytes ) {
		bytes_left = max_window_bytes - window_bytes;

		ptr = ((char *) window) + window_bytes;

		memset( ptr, 0, bytes_left );
	}

	/*----*/

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		mx_status = mx_motor_default_set_parameter_handler( motor );

		return mx_status;
	}

	mx_status = ( *set_parameter) ( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_get_window_is_available( MX_RECORD *motor_record,
				mx_bool_type *window_is_available )
{
	static const char fname[] = "mx_motor_get_window_is_available()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->parameter_type = MXLV_MTR_WINDOW_IS_AVAILABLE;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		mx_status = mx_motor_default_get_parameter_handler( motor );

		return mx_status;
	}

	mx_status = ( *get_parameter) ( motor );

	if ( window_is_available != NULL ) {
		*window_is_available = motor->window_is_available;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_get_use_window( MX_RECORD *motor_record,
				mx_bool_type *use_window )
{
	static const char fname[] = "mx_motor_get_use_window()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->parameter_type = MXLV_MTR_USE_WINDOW;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		mx_status = mx_motor_default_get_parameter_handler( motor );

		return mx_status;
	}

	mx_status = ( *get_parameter) ( motor );

	if ( use_window != NULL ) {
		*use_window = motor->use_window;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_set_use_window( MX_RECORD *motor_record,
				mx_bool_type use_window )
{
	static const char fname[] = "mx_motor_set_use_window()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->parameter_type = MXLV_MTR_USE_WINDOW;

	motor->use_window = motor->use_window;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		mx_status = mx_motor_default_set_parameter_handler( motor );

		return mx_status;
	}

	mx_status = ( *set_parameter) ( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_set_estimated_move_positions( MX_RECORD *motor_record,
					long num_positions,
					double *position_array )
{
	static const char fname[] = "mx_motor_set_estimated_move_positions()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *function_list;
	long i;
	mx_status_type ( *set_parameter ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->parameter_type = MXLV_MTR_NUM_ESTIMATED_MOVE_POSITIONS;

	motor->num_estimated_move_positions = num_positions;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		set_parameter = mx_motor_default_set_parameter_handler;
	}

	mx_status = (*set_parameter) ( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->estimated_move_positions == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Writing to the 'num_estimated_move_positions' field "
		"did _NOT_ create the motor->estimated_move_positions "
		"array for motor '%s'.", motor_record->name );
	}

	for ( i = 0; i < num_positions; i++ ) {

		motor->estimated_move_positions[i] = position_array[i];

#if MX_MOTOR_DEBUG_SET_ESTIMATED_MOVE_POSITIONS
		MX_DEBUG(-2,
		("%s: motor '%s', estimated_move_positions[%ld] = %g",
		 fname, motor_record->name, i,
		 motor->estimated_move_positions[i]));
#endif
	}

	motor->parameter_type = MXLV_MTR_ESTIMATED_MOVE_POSITIONS;

	mx_status = (*set_parameter) ( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_motor_get_estimated_move_durations( MX_RECORD *motor_record,
					long num_positions,
					double *duration_array )
{
	static const char fname[] = "mx_motor_get_estimated_move_durations()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_positions > motor->num_estimated_move_positions ) {
		num_positions = motor->num_estimated_move_positions;
	}

	motor->parameter_type = MXLV_MTR_ESTIMATED_MOVE_DURATIONS;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		get_parameter = mx_motor_default_get_parameter_handler;
	}

	mx_status = (*get_parameter) ( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( duration_array != (double *) NULL ) {
		memcpy( duration_array,
			motor->estimated_move_durations,
			num_positions * sizeof(double) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_motor_get_total_estimated_move_duration( MX_RECORD *motor_record,
					double *total_estimated_duration )
{
	static const char fname[] =
		"mx_motor_get_total_estimated_move_duration()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_MOTOR * );
	mx_status_type mx_status;

	mx_status = mx_motor_get_pointers( motor_record, &motor,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->bypass_estimated_move_calc ) {
		/* If this particular motor driver does not calculate
		 * useful estimated move times, then we can bypass that
		 * calculation and just return 0.
		 */

		motor->total_estimated_move_duration = 0.0;
	} else {
		motor->parameter_type = MXLV_MTR_TOTAL_ESTIMATED_MOVE_DURATION;

		get_parameter = function_list->get_parameter;

		if ( get_parameter == NULL ) {
			get_parameter = mx_motor_default_get_parameter_handler;
		}

		mx_status = (*get_parameter) ( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( total_estimated_duration != (double *) NULL ) {
		*total_estimated_duration =
				motor->total_estimated_move_duration;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === Move by steps functions. (MXC_MTR_STEPPER) === */

MX_EXPORT mx_status_type
mx_motor_move_relative_steps_with_report(MX_RECORD *motor_record,
			long relative_steps,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags)
{
	long current_position, new_position;
	mx_status_type status;

	status = mx_motor_get_position_steps(motor_record, &current_position);

	if ( status.code != MXE_SUCCESS )
		return status;

	new_position = current_position + relative_steps;

	status = mx_motor_move_absolute_steps_with_report( motor_record,
			new_position, move_report_fn, flags );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_move_absolute_steps_with_report(MX_RECORD *motor_record,
			long motor_steps,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags)
{
	static const char fname[]
			= "mx_motor_move_absolute_steps_with_report()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	long positive_limit, negative_limit;
	long position, backlash;
	long relative_motion, backlash_position;
	double dummy_position;
	int mask, test_mask, do_backlash;
	mx_status_type status, move_report_status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	motor->latched_status = 0;

	if ( motor->subclass != MXC_MTR_STEPPER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Motor '%s' is not a stepper motor.",
			motor_record->name );
	}

	fptr = fl_ptr->move_absolute;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"move_absolute function pointer is NULL.");
	}

	positive_limit = motor->raw_positive_limit.stepper;
	negative_limit = motor->raw_negative_limit.stepper;

	status = mx_motor_get_position_steps(motor_record, &position);

	if ( status.code != MXE_SUCCESS )
		return status;

	relative_motion = motor_steps - position;

	/* Do not do the move if it would be less than the deadband size. */

	if ( labs( relative_motion ) < motor->raw_move_deadband.stepper ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If a primary move will be done, save the old destination. */

	test_mask = MXF_MTR_GO_TO_BACKLASH_POSITION | MXF_MTR_ONLY_CHECK_LIMITS;

	if ( ( flags & test_mask ) == 0 ) {
		motor->old_destination = motor->destination;
	}

	/* Do we need to do a backlash correction this time? */

	backlash = motor->raw_backlash_correction.stepper;

	if ( flags & MXF_MTR_IGNORE_BACKLASH ) {
		do_backlash = FALSE;
	} else {
		do_backlash = TRUE;
	}

	if ( do_backlash ) {
		/* Perform the backlash if relative_motion and backlash
		 * are both non-zero and have the same signs.
		 */

		do_backlash = FALSE;

		if ( ( relative_motion > 0 ) && ( backlash > 0 ) ) {
			do_backlash = TRUE;
		} else if ( ( relative_motion < 0 ) && ( backlash < 0 ) ) {
			do_backlash = TRUE;
		}
	}

	/* Do the backlash correction, if necessary. */

	if ( do_backlash ) {
		backlash_position = motor_steps + backlash;

		/* Check software limits. */

		if ( backlash_position > positive_limit ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Backlash correction for '%s' to %ld steps would exceed positive limit "
	"at %ld steps.",
				motor_record->name,
				backlash_position,
				positive_limit);
		}

		if ( backlash_position < negative_limit ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Backlash correction for '%s' to %ld steps would exceed negative limit "
	"at %ld steps.",
				motor_record->name,
				backlash_position,
				negative_limit);
		}

		/* Do not do the backlash correction if we were only asked
		 * to check limits.
		 */

		if ( (flags & MXF_MTR_ONLY_CHECK_LIMITS) == 0 ) {

			/* Do the backlash correction. */

			/* Update destination. */

			motor->destination = motor->offset
				+ motor->scale * (double) backlash_position;

			/* Do the move. */

			motor->raw_destination.stepper = backlash_position;

			motor->backlash_move_in_progress = TRUE;

			motor->last_start_tick = mx_current_clock_tick();

			status = ( *fptr ) ( motor );

			if ( status.code != MXE_SUCCESS ) {
				motor->backlash_move_in_progress = FALSE;

				return status;
			}

			/* Leave now if we were not asked to wait for the
			 * completion of the backlash correction.
			 */

			mask = MXF_MTR_NOWAIT | MXF_MTR_GO_TO_BACKLASH_POSITION;

			if ( (flags & mask) == mask ) {
				return MX_SUCCESSFUL_RESULT;
			}

			/* Otherwise, display our progress on the way to
			 * the backlash position.
			 */

			if ( move_report_fn != NULL ) {
				move_report_status = (*move_report_fn)
						( flags, 1, &motor_record );

				status = MX_SUCCESSFUL_RESULT;
			} else {
				move_report_status = MX_SUCCESSFUL_RESULT;

				status = mx_wait_for_motor_stop(
						motor_record, flags );
			}

			motor->backlash_move_in_progress = FALSE;

			if ( status.code != MXE_SUCCESS )
				return status;

			/* Update copy of motor position in MOTOR structure. */

			status = mx_motor_get_position(
					motor_record, &dummy_position);

			if ( status.code != MXE_SUCCESS )
				return status;

			/* Return if move was aborted. */

			if ( move_report_status.code != MXE_SUCCESS )
				return move_report_status;
		}
	}

	/* Were we asked to only go to the backlash position? */

	if ( flags & MXF_MTR_GO_TO_BACKLASH_POSITION ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Check software limits. */

	if ( motor_steps > positive_limit ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Error: move of '%s' to %ld steps would exceed positive limit at %ld steps.",
			motor_record->name,
			motor_steps,
			positive_limit );
	}

	if ( motor_steps < negative_limit ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Error: move of '%s' to %ld steps would exceed negative limit at %ld steps.",
			motor_record->name,
			motor_steps,
			negative_limit );
	}

	/* Return now if we are only checking limits. */

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Update the destination. */

	motor->destination = motor->offset
				+ motor->scale * (double) motor_steps;

	/* Do the move. */

	motor->raw_destination.stepper = motor_steps;

	motor->last_start_tick = mx_current_clock_tick();

	status = ( *fptr ) ( motor );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( move_report_fn != NULL ) {
		move_report_status = move_report_fn(
						flags, 1, &motor_record);
	} else {
		move_report_status = MX_SUCCESSFUL_RESULT;
	}

	/* Update copy of motor position in MX_MOTOR structure, unless
	 * MXF_MTR_NOWAIT is set in 'flags'.
	 */

	if ( (flags & MXF_MTR_NOWAIT) == 0 ) {
		status = mx_motor_get_position(motor_record, &dummy_position);

		if ( status.code != MXE_SUCCESS )
			return status;
	}

	return move_report_status;
}

MX_EXPORT mx_status_type
mx_motor_get_position_steps( MX_RECORD *motor_record, long *motor_steps )
{
	static const char fname[] = "mx_motor_get_position_steps()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->subclass != MXC_MTR_STEPPER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Motor '%s' is not a stepper motor.",
			motor_record->name );
	}

	fptr = fl_ptr->get_position;

	if ( fptr == NULL ) {
		/* This driver does not have a get_position function.  See
		 * if there is a get_extended_status function instead.
		 */

		fptr = fl_ptr->get_extended_status;

		if ( fptr == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for motor '%s' does not define either a get_position "
	"or a get_extended_status driver function.  This is likely to be a "
	"bug in that driver.",
				mx_get_driver_name( motor_record ),
				motor_record->name );
		}
	}

	status = ( *fptr ) ( motor );

	/* Update position. */

	motor->position = motor->offset
			+ (motor->scale) * (double) motor->raw_position.stepper;

	*motor_steps = motor->raw_position.stepper;

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_position_steps( MX_RECORD *motor_record, long motor_steps )
{
	static const char fname[] = "mx_motor_set_position_steps()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->subclass != MXC_MTR_STEPPER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Motor '%s' is not a stepper motor.",
			motor_record->name );
	}

	fptr = fl_ptr->set_position;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"set_position_steps function pointer is NULL.");
	}

	/* Update set_position. */

	motor->raw_set_position.stepper = motor_steps;

	motor->set_position = motor->offset
			+ motor->scale * (double) motor_steps;

	/* Invoke the function. */

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_motor_steps_to_go( MX_RECORD *motor_record, long *steps_to_go )
{
	static const char fname[] = "mx_motor_steps_to_go()";

	MX_MOTOR *motor;
	long step_position, step_destination;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, NULL, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->subclass != MXC_MTR_STEPPER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Motor '%s' is not a stepper motor.",
			motor_record->name );
	}

	status = mx_motor_get_position_steps( motor_record, &step_position );

	if ( status.code != MXE_SUCCESS )
		return status;

	step_destination = motor->raw_destination.stepper;

	*steps_to_go = step_destination - step_position;

	return status;
}

/* === Move analog functions (MXC_MTR_ANALOG) === */

MX_EXPORT mx_status_type
mx_motor_move_absolute_analog_with_report(MX_RECORD *motor_record,
			double motor_position,
			MX_MOTOR_MOVE_REPORT_FUNCTION move_report_fn,
			unsigned long flags)
{
	static const char fname[]
		= "mx_motor_move_absolute_analog_with_report()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	double positive_limit, negative_limit;
	double present_position, backlash;
	double relative_motion, backlash_position;
	double test_var, dummy_position;
	int mask, test_mask, do_backlash;
	char units[80];
	mx_status_type status, move_report_status;

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
	MX_HRT_TIMING total_measurement, setup_measurement;
	MX_HRT_TIMING backlash_measurement, limits_measurement;
	MX_HRT_TIMING move_measurement, move_report_measurement;
	MX_HRT_TIMING update_measurement;

	MX_DEBUG(-2,("%s invoked for '%s'", fname, motor_record->name ));

	MX_HRT_START( total_measurement );
	MX_HRT_START( setup_measurement );
#endif

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	motor->latched_status = 0;

	if ( motor->subclass != MXC_MTR_ANALOG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Motor '%s' is not an analog motor.",
			motor->record->name );
	}

	fptr = fl_ptr->move_absolute;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"move_absolute function pointer is NULL.");
	}

	positive_limit = motor->raw_positive_limit.analog;
	negative_limit = motor->raw_negative_limit.analog;

	status = mx_motor_get_position_analog(motor_record, &present_position);

	if ( status.code != MXE_SUCCESS )
		return status;

	relative_motion = motor_position - present_position;

	/* Do not do the move if it would be less than the deadband size. */

	if ( fabs( relative_motion ) < motor->raw_move_deadband.analog ) {

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
		MX_HRT_END( total_measurement );
		MX_HRT_RESULTS( total_measurement, fname,
				"return '%s' due to deadband",
				motor_record->name );
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* If a primary move will be done, save the old destination. */

	test_mask = MXF_MTR_GO_TO_BACKLASH_POSITION | MXF_MTR_ONLY_CHECK_LIMITS;

	if ( ( flags & test_mask ) == 0 ) {
		motor->old_destination = motor->destination;
	}

	/* Do we need to do a backlash correction this time? */

	backlash = motor->raw_backlash_correction.analog;

	if ( flags & MXF_MTR_IGNORE_BACKLASH ) {
		do_backlash = FALSE;
	} else {
		do_backlash = TRUE;
	}

	if ( do_backlash ) {
		/* Perform the backlash if relative_motion and backlash
		 * are both non-zero and have the same signs.
		 */

		do_backlash = FALSE;

		if ( ( relative_motion > 0.0 ) && ( backlash > 0.0 ) ) {
			do_backlash = TRUE;
		} else if ( ( relative_motion < 0.0 ) && ( backlash < 0.0 ) ) {
			do_backlash = TRUE;
		}
	}

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
	MX_HRT_END( setup_measurement );
	MX_HRT_RESULTS( setup_measurement, fname, "setup" );

	MX_HRT_START( backlash_measurement );
#endif

	/* Do the backlash correction, if necessary. */

	/* MX currently doesn't record the raw units for analog motors.
	 * If the absolute value of the scale factor for this motor is
	 * very close to 1, I can assume that the raw units are the 
	 * same as the user units.  Otherwise, I don't know what the
	 * units are.  This is a kludge until I replace this with 
	 * something better.
	 */

	if ( do_backlash ) {
		backlash_position = motor_position + backlash;

		/* Check software limits. */

		if ( backlash_position > positive_limit ) {
			test_var = fabs( fabs( motor->scale ) - 1.0 );

			if ( test_var < MX_MOTOR_ANALOG_FUZZ ) {
				strlcpy( units, " ", sizeof(units) );
				strlcat( units, motor->units, sizeof(units) );
			} else {
				strlcpy( units, "", sizeof(units) );
			}

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Backlash correction for '%s' to %.*g%s would exceed positive limit at %.*g%s.",
				motor->record->name,
				motor->record->precision,
				backlash_position, units,
				motor->record->precision,
				positive_limit, units );
		}

		if ( backlash_position < negative_limit ) {
			test_var = fabs( fabs( motor->scale ) - 1.0 );

			if ( test_var < MX_MOTOR_ANALOG_FUZZ ) {
				strlcpy( units, " ", sizeof(units) );
				strlcat( units, motor->units, sizeof(units) );
			} else {
				strlcpy( units, "", sizeof(units) );
			}

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Backlash correction for '%s' to %.*g%s would exceed negative limit at %.*g%s.",
				motor->record->name,
				motor->record->precision,
				backlash_position, units,
				motor->record->precision,
				negative_limit, units );
		}

		/* Do not do the backlash correction if we were only asked
		 * to check limits.
		 */

		if ( (flags & MXF_MTR_ONLY_CHECK_LIMITS) == 0 ) {

			/* Do the backlash_correction. */

			/* Update the destination. */

			motor->destination = motor->offset
				+ motor->scale * backlash_position;

			/* Do the move. */

			motor->raw_destination.analog = backlash_position;

			motor->backlash_move_in_progress = TRUE;

			motor->last_start_tick = mx_current_clock_tick();

			status = ( *fptr ) ( motor );

			if ( status.code != MXE_SUCCESS ) {
				motor->backlash_move_in_progress = FALSE;

				return status;
			}

			/* Leave now if we were not asked to wait for the
			 * completion of the backlash correction.
			 */

			mask = MXF_MTR_NOWAIT | MXF_MTR_GO_TO_BACKLASH_POSITION;

			if ( (flags & mask) == mask ) {

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
				MX_HRT_END( total_measurement );
				MX_HRT_RESULTS( total_measurement, fname,
					"return '%s' due to NOWAIT backlash",
					motor_record->name );
#endif

				return MX_SUCCESSFUL_RESULT;
			}

			/* Otherwise, display our progress on the way to
			 * the backlash position.
			 */

			if ( move_report_fn != NULL ) {
				move_report_status = (*move_report_fn)
						( flags, 1, &motor_record );

				status = MX_SUCCESSFUL_RESULT;
			} else {
				move_report_status = MX_SUCCESSFUL_RESULT;

				status = mx_wait_for_motor_stop(
						motor_record, flags );
			}

			motor->backlash_move_in_progress = FALSE;

			if ( status.code != MXE_SUCCESS )
				return status;

			/* Update copy of motor position in MOTOR structure. */

			status = mx_motor_get_position(
					motor_record, &dummy_position);

			if ( status.code != MXE_SUCCESS )
				return status;

			/* Return if move was aborted. */

			if ( move_report_status.code != MXE_SUCCESS )
				return move_report_status;
		}
	}

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
	MX_HRT_END( backlash_measurement );
	MX_HRT_RESULTS( backlash_measurement, fname, "backlash" );

	MX_HRT_START( limits_measurement );
#endif

	/* Were we asked to only go to the backlash position? */

	if ( flags & MXF_MTR_GO_TO_BACKLASH_POSITION ) {

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
		MX_HRT_END( total_measurement );
		MX_HRT_RESULTS( total_measurement, fname,
				"return '%s' due to backlash only",
				motor_record->name );
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* Check software limits. */

	if ( motor_position > positive_limit ) {
		test_var = fabs( fabs( motor->scale ) - 1.0 );

		if ( test_var < MX_MOTOR_ANALOG_FUZZ ) {
			strlcpy( units, " ", sizeof(units) );
			strlcat( units, motor->units, sizeof(units) );
		} else {
			strlcpy( units, "", sizeof(units) );
		}

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Error: move of '%s' to %.*g%s would exceed positive limit at %.*g%s.",
			motor->record->name,
			motor->record->precision,
			motor_position, units,
			motor->record->precision,
			positive_limit, units );
	}

	if ( motor_position < negative_limit ) {
		test_var = fabs( fabs( motor->scale ) - 1.0 );

		if ( test_var < MX_MOTOR_ANALOG_FUZZ ) {
			strlcpy( units, " ", sizeof(units) );
			strlcat( units, motor->units, sizeof(units) );
		} else {
			strlcpy( units, "", sizeof(units) );
		}

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Error: move of '%s' to %.*g%s would exceed negative limit at %.*g%s.",
			motor->record->name,
			motor->record->precision,
			motor_position, units,
			motor->record->precision,
			negative_limit, units );
	}

	/* Return now if we are only checking limits. */

	if ( flags & MXF_MTR_ONLY_CHECK_LIMITS ) {

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
		MX_HRT_END( total_measurement );
		MX_HRT_RESULTS( total_measurement, fname,
				"return '%s' due to only checking limits",
				motor_record->name );
#endif
		return MX_SUCCESSFUL_RESULT;
	}

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
	MX_HRT_END( limits_measurement );
	MX_HRT_RESULTS( limits_measurement, fname, "limits" );

	MX_HRT_START( move_measurement );
#endif

	/* Update the destination. */

	motor->destination = motor->offset
				+ motor->scale * motor_position;

	/* Do the move. */

	motor->raw_destination.analog = motor_position;

	motor->last_start_tick = mx_current_clock_tick();

	status = ( *fptr ) ( motor );

	if ( status.code != MXE_SUCCESS )
		return status;

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
	MX_HRT_END( move_measurement );
	MX_HRT_RESULTS( move_measurement, fname, "move" );

	MX_HRT_START( move_report_measurement );
#endif

	if ( move_report_fn != NULL ) {
		move_report_status = move_report_fn(
						flags, 1, &motor_record);
	} else {
		move_report_status = MX_SUCCESSFUL_RESULT;
	}

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
	MX_HRT_END( move_report_measurement );
	MX_HRT_RESULTS( move_report_measurement, fname, "move report" );

	MX_HRT_START( update_measurement );
#endif

	/* Update copy of motor position in MX_MOTOR structure, unless
	 * MXF_MTR_NOWAIT is set in 'flags'.
	 */

	if ( (flags & MXF_MTR_NOWAIT) == 0 ) {
		status = mx_motor_get_position(motor_record, &dummy_position);

		if ( status.code != MXE_SUCCESS )
			return status;
	}

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
	MX_HRT_END( update_measurement );
	MX_HRT_END( total_measurement );

	MX_HRT_RESULTS( update_measurement, fname, "update" );
	MX_HRT_RESULTS( total_measurement, fname, "TOTAL" );
#endif

#if MX_MOTOR_DEBUG_MOVE_ABSOLUTE_ANALOG_TIMING
	MX_DEBUG(-2,("%s complete for '%s'", fname, motor_record->name ));
#endif

	return move_report_status;
}

MX_EXPORT mx_status_type
mx_motor_get_position_analog( MX_RECORD *motor_record,
				double *raw_position )
{
	static const char fname[] = "mx_motor_get_position_analog()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->subclass != MXC_MTR_ANALOG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Motor '%s' is not an analog motor.",
			motor_record->name );
	}

	fptr = fl_ptr->get_position;

	if ( fptr == NULL ) {
		/* This driver does not have a get_position function.  See
		 * if there is a get_extended_status function instead.
		 */

		fptr = fl_ptr->get_extended_status;

		if ( fptr == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for motor '%s' does not define either a get_position "
	"or a get_extended_status driver function.  This is likely to be a "
	"bug in that driver.",
				mx_get_driver_name( motor_record ),
				motor_record->name );
		}
	}

	status = ( *fptr ) ( motor );

	/* Update position. */

	motor->position = motor->offset
			+ (motor->scale) * motor->raw_position.analog;

	*raw_position = motor->raw_position.analog;

	return status;
}

MX_EXPORT mx_status_type
mx_motor_set_position_analog( MX_RECORD *motor_record,
						double raw_set_position )
{
	static const char fname[] = "mx_motor_set_position_analog()";

	MX_MOTOR *motor;
	MX_MOTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_MOTOR * );
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->subclass != MXC_MTR_ANALOG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Motor '%s' is not an analog motor.",
			motor_record->name );
	}

	fptr = fl_ptr->set_position;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"set_position_analog function pointer is NULL.");
	}

	/* Update set_position. */

	motor->raw_set_position.analog = raw_set_position;

	motor->set_position = motor->offset + motor->scale * raw_set_position;

	/* Invoke the function. */

	status = ( *fptr ) ( motor );

	return status;
}

MX_EXPORT mx_status_type
mx_is_motor_position_between_software_limits(
	MX_RECORD *motor_record, double proposed_position,
	int *result_flag, int generate_error_message )
{
	static const char fname[] =
		"mx_is_motor_position_between_software_limits()";

	MX_MOTOR *motor;
	double scale, offset, unscaled_proposed_position;
	long proposed_step_position;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, NULL, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	scale = motor->scale;
	offset = motor->offset;

	if ( fabs(scale) < MX_MOTOR_ANALOG_FUZZ ) {
		unscaled_proposed_position = 0;
	} else {
		unscaled_proposed_position
			= (proposed_position - offset) / scale;
	}

	MX_DEBUG( 2,("%s: motor '%s', proposed_position = %g, unscaled = %g",
		fname, motor_record->name, proposed_position,
		unscaled_proposed_position));

	switch( motor->subclass ) {
	case MXC_MTR_ANALOG:
	    status =
		mx_is_analog_motor_position_between_software_limits(
			motor_record, unscaled_proposed_position,
			result_flag, generate_error_message );
	    break;
	case MXC_MTR_STEPPER:
	    proposed_step_position = mx_round( unscaled_proposed_position );

	    status =
		mx_is_stepper_motor_position_between_software_limits(
			motor_record, proposed_step_position,
			result_flag, generate_error_message );
	    break;
	default:
	    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Unrecognized motor subclass = %ld", motor->subclass );
	}
	return status;
}

MX_EXPORT mx_status_type
mx_is_analog_motor_position_between_software_limits(
	MX_RECORD *motor_record, double proposed_analog_position,
	int *result_flag, int generate_error_message )
{
	static const char fname[] =
	    "mx_is_analog_motor_position_between_software_limits()";

	MX_MOTOR *motor;
	char message_buffer[1000];
	char units[80];
	double positive_limit, negative_limit, test_var;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, NULL, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->subclass != MXC_MTR_ANALOG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Motor '%s' is not an analog motor.",
			motor->record->name );
	}

	if ( result_flag == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"result_flag pointer passed was NULL." );
	}

	message_buffer[0] = '\0';
	*result_flag = 0;

	positive_limit = motor->raw_positive_limit.analog;
	negative_limit = motor->raw_negative_limit.analog;

	/* MX currently doesn't record the raw units for analog motors.
	 * If the absolute value of the scale factor for this motor is
	 * very close to 1, I can assume that the raw units are the 
	 * same as the user units.  Otherwise, I don't know what the
	 * units are.  This is a kludge until I replace this with 
	 * something better.
	 */

	if ( proposed_analog_position > positive_limit ) {
		*result_flag = 1;

		if ( generate_error_message ) {
			test_var = fabs( fabs( motor->scale ) - 1.0 );

			if ( test_var < MX_MOTOR_ANALOG_FUZZ ) {
				strlcpy( units, " ", sizeof(units) );
				strlcat( units, motor->units, sizeof(units) );
			} else {
				strlcpy( units, "", sizeof(units) );
			}

			snprintf( message_buffer,
				sizeof(message_buffer),
		"Move of '%s' to %.*g%s would exceed positive limit at %.*g%s.",
				motor_record->name,
				motor_record->precision,
				proposed_analog_position, units,
				motor_record->precision,
				positive_limit, units );
		}
	}
	if ( proposed_analog_position < negative_limit ) {
		*result_flag = -1;

		if ( generate_error_message ) {
			test_var = fabs( fabs( motor->scale ) - 1.0 );

			if ( test_var < MX_MOTOR_ANALOG_FUZZ ) {
				strlcpy( units, " ", sizeof(units) );
				strlcat( units, motor->units, sizeof(units) );
			} else {
				strlcpy( units, "", sizeof(units) );
			}

			snprintf( message_buffer,
				sizeof(message_buffer),
		"Move of '%s' to %.*g%s would exceed negative limit at %.*g%s.",
				motor_record->name,
				motor_record->precision,
				proposed_analog_position, units,
				motor_record->precision,
				negative_limit, units );
		}
	}

	if ( (*result_flag != 0) && generate_error_message ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
					"%s", message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_is_stepper_motor_position_between_software_limits(
	MX_RECORD *motor_record, long proposed_step_position,
	int *result_flag, int generate_error_message )
{
	static const char fname[] =
	    "mx_is_stepper_motor_position_between_software_limits()";

	MX_MOTOR *motor;
	char message_buffer[200];
	long positive_limit, negative_limit;
	mx_status_type status;

	status = mx_motor_get_pointers( motor_record,
					&motor, NULL, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->subclass != MXC_MTR_STEPPER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Motor '%s' is not an stepper motor.",
			motor->record->name );
	}

	if ( result_flag == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"result_flag pointer passed was NULL." );
	}

	message_buffer[0] = '\0';
	*result_flag = 0;

	positive_limit = motor->raw_positive_limit.stepper;
	negative_limit = motor->raw_negative_limit.stepper;

	if ( proposed_step_position > positive_limit ) {
		*result_flag = 1;

		if ( generate_error_message ) {
			snprintf( message_buffer,
				sizeof(message_buffer),
	"Move of '%s' to %ld steps would exceed positive limit at %ld steps.",
				motor_record->name,
				proposed_step_position,
				positive_limit );
		}
	}
	if ( proposed_step_position < negative_limit ) {
		*result_flag = -1;

		if ( generate_error_message ) {
			snprintf( message_buffer,
				sizeof(message_buffer),
	"Move of '%s' to %ld steps would exceed negative limit at %ld steps.",
				motor_record->name,
				proposed_step_position,
				negative_limit );
		}
	}

	if ( (*result_flag != 0) && generate_error_message ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
					"%s", message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_motor_vctest_extended_status( MX_RECORD_FIELD *record_field,
				int direction,
				mx_bool_type *value_changed_ptr )
{
	static const char fname[] = "mx_motor_vctest_extended_status()";

	MX_RECORD *record;
	MX_MOTOR *motor;
	MX_RECORD_FIELD *extended_status_field;
	MX_RECORD_FIELD *position_field;
	MX_RECORD_FIELD *status_field;

	mx_bool_type must_check_position;
	mx_bool_type must_check_status;

	mx_bool_type position_changed;
	mx_bool_type status_changed;

	mx_bool_type send_position_callbacks;
	mx_bool_type send_status_callbacks;
	mx_bool_type send_extended_status_callbacks;

	mx_status_type mx_status;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The record field pointer passed was NULL." );
	}
	if ( value_changed_ptr == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The value_changed_ptr passed was NULL." );
	}

	record = record_field->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for record field %p is NULL.",
			record_field );
	}

	motor = record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

#if MX_MOTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s invoked for record field '%s.%s'",
		fname, record->name, record_field->name ));
#endif

	/* What we do here depends on which field this is. */

	switch( record_field->label_value ) {
	case MXLV_MTR_POSITION:
		must_check_position = TRUE;
		must_check_status = FALSE;
		break;
	case MXLV_MTR_GET_STATUS:
		must_check_position = FALSE;
		must_check_status = TRUE;
		break;
	case MXLV_MTR_GET_EXTENDED_STATUS:
		must_check_position = TRUE;
		must_check_status = TRUE;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The record field '%s' passed to this function for record '%s' "
		"is not one of the allowed values of 'position', 'status', "
		"or 'extended_status'", record_field->name, record->name );
	}

#if MX_MOTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s: must_check_position = %d",
			fname, (int) must_check_position));
	MX_DEBUG(-2,("%s: must_check_status = %d",
			fname, (int) must_check_status));
#endif

	/* Get pointers to all of the fields. */

	extended_status_field =
	  &(record->record_field_array[ motor->extended_status_field_number ]);

	position_field =
		&(record->record_field_array[ motor->position_field_number ]);

	status_field =
		&(record->record_field_array[ motor->status_field_number ]);

	position_changed = FALSE;
	status_changed = FALSE;

	/* Test the position field. */

	if ( must_check_position ) {
		mx_status = mx_default_test_for_value_changed(
							position_field,
							direction,
							&position_changed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Test the status field. */

	if ( must_check_status ) {
		mx_status = mx_default_test_for_value_changed(
							status_field,
							direction,
							&status_changed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MX_MOTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s: position_changed = %d",
			fname, (int) position_changed));
	MX_DEBUG(-2,("%s: status_changed = %d",
			fname, (int) status_changed));
#endif

	/* If the position field or the status field changed, then we
	 * must update the extended_status field to match.
	 */

	snprintf( motor->extended_status, sizeof(motor->extended_status),
		MXP_MOTOR_EXTENDED_STATUS_FORMAT, record->precision,
		motor->position, motor->status );

	/* We do not send out value changed callbacks for the record field
	 * that was passed to us, since our caller will decide whether
	 * or not to do that.  However, we _do_ send out value changed
	 * callbacks for any other fields.
	 */

	send_position_callbacks        = FALSE;
	send_status_callbacks          = FALSE;
	send_extended_status_callbacks = FALSE;

	if ( record_field == position_field ) {
		if ( position_changed ) {
			send_extended_status_callbacks = TRUE;
		}
		if ( status_changed ) {
			send_status_callbacks = TRUE;
			send_extended_status_callbacks = TRUE;
		}
	} else
	if ( record_field == status_field ) {
		if ( position_changed ) {
			send_position_callbacks = TRUE;
			send_extended_status_callbacks = TRUE;
		}
		if ( status_changed ) {
			send_extended_status_callbacks = TRUE;
		}
	} else
	if ( record_field == extended_status_field ) {
		if ( position_changed ) {
			send_position_callbacks = TRUE;
		}
		if ( status_changed ) {
			send_status_callbacks = TRUE;
		}
	}

#if MX_MOTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s: send_position_callbacks = %d",
			fname, (int) send_position_callbacks));
	MX_DEBUG(-2,("%s: send_status_callbacks = %d",
			fname, (int) send_status_callbacks));
	MX_DEBUG(-2,("%s: send_extended_status_callbacks = %d",
			fname, (int) send_extended_status_callbacks));
#endif

	/* Send out position callbacks. */

	if ( send_position_callbacks ) {
		if ( position_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
					position_field, MXCBT_VALUE_CHANGED );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Send out status callbacks. */

	if ( send_status_callbacks ) {
		if ( status_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
					status_field, MXCBT_VALUE_CHANGED );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Send out extended_status callbacks. */

	if ( send_extended_status_callbacks ) {
		if ( extended_status_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
				extended_status_field, MXCBT_VALUE_CHANGED );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	if ( value_changed_ptr != NULL ) {
		switch( record_field->label_value ) {
		case MXLV_MTR_POSITION:
			*value_changed_ptr = position_changed;
			break;
		case MXLV_MTR_GET_STATUS:
			*value_changed_ptr = status_changed;
			break;
		case MXLV_MTR_GET_EXTENDED_STATUS:
			*value_changed_ptr = position_changed | status_changed;
			break;
		}

#if MX_MOTOR_DEBUG_VCTEST
		MX_DEBUG(-2,("%s: '%s.%s' value_changed = %d.",
			fname, record->name, record_field->name,
			*value_changed_ptr));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

