/*
 * Name:    pr_motor.c
 *
 * Purpose: Functions used to process MX motor record events.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_motor.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

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
		case MXLV_MTR_BUSY:
		case MXLV_MTR_DESTINATION:
		case MXLV_MTR_POSITION:
		case MXLV_MTR_SET_POSITION:
		case MXLV_MTR_SOFT_ABORT:
		case MXLV_MTR_IMMEDIATE_ABORT:
		case MXLV_MTR_POSITIVE_LIMIT_HIT:
		case MXLV_MTR_NEGATIVE_LIMIT_HIT:
		case MXLV_MTR_HOME_SEARCH:
		case MXLV_MTR_CONSTANT_VELOCITY_MOVE:
		case MXLV_MTR_SPEED:
		case MXLV_MTR_BASE_SPEED:
		case MXLV_MTR_MAXIMUM_SPEED:
		case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		case MXLV_MTR_ACCELERATION_TIME:
		case MXLV_MTR_ACCELERATION_DISTANCE:
		case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		case MXLV_MTR_SAVE_SPEED:
		case MXLV_MTR_RESTORE_SPEED:
		case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		case MXLV_MTR_COMPUTE_REAL_POSITION:
		case MXLV_MTR_GET_STATUS:
		case MXLV_MTR_GET_EXTENDED_STATUS:
		case MXLV_MTR_SAVE_START_POSITIONS:

		case MXLV_MTR_AXIS_ENABLE:
		case MXLV_MTR_CLOSED_LOOP:
		case MXLV_MTR_FAULT_RESET:

		case MXLV_MTR_PROPORTIONAL_GAIN:
		case MXLV_MTR_INTEGRAL_GAIN:
		case MXLV_MTR_DERIVATIVE_GAIN:
		case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		case MXLV_MTR_INTEGRAL_LIMIT:
		case MXLV_MTR_EXTRA_GAIN:

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
	mx_status_type status;

	double start_position, end_position;
	double pseudomotor_position, real_position;
	int flags;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	motor = (MX_MOTOR *) (record->record_class_struct);

	motor_function_list = (MX_MOTOR_FUNCTION_LIST *)
				(record->class_specific_function_list);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_MTR_POSITION:
			status = mx_motor_get_position( record, NULL );
			break;
		case MXLV_MTR_BUSY:
			status = mx_motor_is_busy( record, NULL );
			break;
		case MXLV_MTR_NEGATIVE_LIMIT_HIT:
			status = mx_motor_negative_limit_hit( record, NULL );
			break;
		case MXLV_MTR_POSITIVE_LIMIT_HIT:
			status = mx_motor_positive_limit_hit( record, NULL );
			break;
		case MXLV_MTR_SPEED:
			status = mx_motor_get_speed( record, NULL );

			break;
		case MXLV_MTR_BASE_SPEED:
			status = mx_motor_get_base_speed( record, NULL );

			break;
		case MXLV_MTR_MAXIMUM_SPEED:
			status = mx_motor_get_maximum_speed( record, NULL );

			break;
		case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
			status = mx_motor_get_synchronous_motion_mode(
							record, NULL );
			break;
		case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
			status = mx_motor_get_raw_acceleration_parameters(
							record, NULL );

			break;
		case MXLV_MTR_ACCELERATION_TIME:
			status = mx_motor_get_acceleration_time( record, NULL );
			break;
		case MXLV_MTR_ACCELERATION_DISTANCE:
			status = mx_motor_get_acceleration_distance(
							record, NULL );
			break;
		case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
			start_position = motor->compute_extended_scan_range[0];
			end_position = motor->compute_extended_scan_range[1];

			status = mx_motor_compute_extended_scan_range( record,
				start_position, end_position, NULL, NULL );
			break;
		case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
			real_position = motor->compute_pseudomotor_position[0];

			status =
		mx_motor_compute_pseudomotor_position_from_real_position(
			record, real_position, &pseudomotor_position, TRUE );

			break;
		case MXLV_MTR_COMPUTE_REAL_POSITION:
			pseudomotor_position = motor->compute_real_position[0];

			status =
		mx_motor_compute_real_position_from_pseudomotor_position(
			record, pseudomotor_position, &real_position, TRUE );

			break;
		case MXLV_MTR_GET_STATUS:
			status = mx_motor_get_status( record, NULL );
			break;
		case MXLV_MTR_GET_EXTENDED_STATUS:
			status = mx_motor_get_extended_status( record,
								NULL, NULL );
			break;
		case MXLV_MTR_PROPORTIONAL_GAIN:
			status = mx_motor_get_gain( record,
					MXLV_MTR_PROPORTIONAL_GAIN, NULL );
			break;
		case MXLV_MTR_INTEGRAL_GAIN:
			status = mx_motor_get_gain( record,
					MXLV_MTR_INTEGRAL_GAIN, NULL );
			break;
		case MXLV_MTR_DERIVATIVE_GAIN:
			status = mx_motor_get_gain( record,
					MXLV_MTR_DERIVATIVE_GAIN, NULL );
			break;
		case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
			status = mx_motor_get_gain( record,
				MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN, NULL );
			break;
		case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
			status = mx_motor_get_gain( record,
				MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN, NULL );
			break;
		case MXLV_MTR_INTEGRAL_LIMIT:
			status = mx_motor_get_gain( record,
					MXLV_MTR_INTEGRAL_LIMIT, NULL );
			break;
		case MXLV_MTR_EXTRA_GAIN:
			status = mx_motor_get_gain( record,
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
		case MXLV_MTR_DESTINATION:
			flags = MXF_MTR_NOWAIT | MXF_MTR_IGNORE_KEYBOARD
					| MXF_MTR_IGNORE_BACKLASH;

			status = mx_motor_move_absolute( record,
					motor->destination, flags );

			break;
		case MXLV_MTR_SET_POSITION:
			status = mx_motor_set_position( record,
						motor->set_position );

			break;
		case MXLV_MTR_SOFT_ABORT:
			status = mx_motor_soft_abort( record );

			/* Set the soft abort flag back to zero. */

			motor->soft_abort = 0;
			break;
		case MXLV_MTR_IMMEDIATE_ABORT:
			status = mx_motor_immediate_abort( record );

			/* Set the immediate abort flag back to zero. */

			motor->immediate_abort = 0;
			break;
		case MXLV_MTR_CONSTANT_VELOCITY_MOVE:
			status = mx_motor_constant_velocity_move( record,
					motor->constant_velocity_move );
			break;
		case MXLV_MTR_HOME_SEARCH:
			status = mx_motor_find_home_position( record,
					motor->home_search );
			break;
		case MXLV_MTR_SPEED:
			status = mx_motor_set_speed( record, motor->speed );

			break;
		case MXLV_MTR_BASE_SPEED:
			status = mx_motor_set_base_speed( record,
							motor->base_speed );

			break;
		case MXLV_MTR_MAXIMUM_SPEED:
			status = mx_motor_set_base_speed( record,
							motor->maximum_speed );

			break;
		case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
			status = mx_motor_set_raw_acceleration_parameters(
				record, motor->raw_acceleration_parameters );

			break;
		case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
			position1 = motor->speed_choice_parameters[0];
			position2 = motor->speed_choice_parameters[1];
			time_for_move = motor->speed_choice_parameters[2];

			status = mx_motor_set_speed_between_positions( record,
					position1, position2, time_for_move );
			break;
		case MXLV_MTR_SAVE_SPEED:
			status = mx_motor_save_speed( record );
			break;
		case MXLV_MTR_RESTORE_SPEED:
			status = mx_motor_restore_speed( record );
			break;
		case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
			status = mx_motor_set_synchronous_motion_mode( record,
					motor->synchronous_motion_mode );
			break;
		case MXLV_MTR_SAVE_START_POSITIONS:
			status = mx_motor_save_start_positions( record,
					motor->save_start_positions );
			break;
	
		case MXLV_MTR_AXIS_ENABLE:
			status = mx_motor_send_control_command( record,
					MXLV_MTR_AXIS_ENABLE,
					motor->axis_enable );
			break;
		case MXLV_MTR_CLOSED_LOOP:
			status = mx_motor_send_control_command( record,
					MXLV_MTR_CLOSED_LOOP,
					motor->closed_loop );
			break;
		case MXLV_MTR_FAULT_RESET:
			status = mx_motor_send_control_command( record,
					MXLV_MTR_FAULT_RESET,
					motor->fault_reset );
			break;

		case MXLV_MTR_PROPORTIONAL_GAIN:
			status = mx_motor_set_gain( record,
					MXLV_MTR_PROPORTIONAL_GAIN,
					motor->proportional_gain );
			break;
		case MXLV_MTR_INTEGRAL_GAIN:
			status = mx_motor_set_gain( record,
					MXLV_MTR_INTEGRAL_GAIN,
					motor->integral_gain );
			break;
		case MXLV_MTR_DERIVATIVE_GAIN:
			status = mx_motor_set_gain( record,
					MXLV_MTR_DERIVATIVE_GAIN,
					motor->derivative_gain );
			break;
		case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
			status = mx_motor_set_gain( record,
					MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN,
					motor->velocity_feedforward_gain );
			break;
		case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
			status = mx_motor_set_gain( record,
					MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN,
					motor->acceleration_feedforward_gain );
			break;
		case MXLV_MTR_INTEGRAL_LIMIT:
			status = mx_motor_set_gain( record,
					MXLV_MTR_INTEGRAL_LIMIT,
					motor->integral_limit );
			break;
		case MXLV_MTR_EXTRA_GAIN:
			status = mx_motor_set_gain( record,
					MXLV_MTR_EXTRA_GAIN,
					motor->extra_gain );
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

	return status;
}

