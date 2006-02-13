/*
 * Name:    d_delta_motor.c 
 *
 * Purpose: MX motor driver to move a real motor relative to a fixed position.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_variable.h"
#include "d_delta_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_delta_motor_record_function_list = {
	NULL,
	mxd_delta_motor_create_record_structures,
	mxd_delta_motor_finish_record_initialization,
	NULL,
	mxd_delta_motor_print_motor_structure
};

MX_MOTOR_FUNCTION_LIST mxd_delta_motor_motor_function_list = {
	NULL,
	mxd_delta_motor_move_absolute,
	mxd_delta_motor_get_position,
	mxd_delta_motor_set_position,
	mxd_delta_motor_soft_abort,
	mxd_delta_motor_immediate_abort,
	NULL,
	NULL,
	mxd_delta_motor_find_home_position,
	NULL,
	mxd_delta_motor_get_parameter,
	mxd_delta_motor_set_parameter,
	NULL,
	mxd_delta_motor_get_status
};

/* Delta motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_delta_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_DELTA_MOTOR_STANDARD_FIELDS
};

mx_length_type mxd_delta_motor_num_record_fields
		= sizeof( mxd_delta_motor_record_field_defaults )
			/ sizeof( mxd_delta_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_delta_motor_rfield_def_ptr
			= &mxd_delta_motor_record_field_defaults[0];

static mx_status_type
mxd_delta_motor_get_fixed_position_value( MX_DELTA_MOTOR *delta_motor,
					double *fixed_position_value );

/* === */

/* A private function for the use of the driver. */

static mx_status_type
mxd_delta_motor_get_pointers( MX_MOTOR *motor,
			MX_DELTA_MOTOR **delta_motor,
			MX_RECORD **moving_motor_record,
			const char *calling_fname )
{
	MX_DELTA_MOTOR *delta_motor_ptr;

	static const char fname[] = "mxd_delta_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	delta_motor_ptr = (MX_DELTA_MOTOR *) motor->record->record_type_struct;

	if ( delta_motor_ptr == (MX_DELTA_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DELTA_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( delta_motor != (MX_DELTA_MOTOR **) NULL ) {
		*delta_motor = delta_motor_ptr;
	}

	if ( moving_motor_record != (MX_RECORD **) NULL ) {
		*moving_motor_record = delta_motor_ptr->moving_motor_record;

		if ( *moving_motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Moving motor record pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_delta_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_delta_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_DELTA_MOTOR *delta_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	delta_motor = (MX_DELTA_MOTOR *) malloc( sizeof(MX_DELTA_MOTOR) );

	if ( delta_motor == (MX_DELTA_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DELTA_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = delta_motor;
	record->class_specific_function_list
				= &mxd_delta_motor_motor_function_list;

	motor->record = record;
	delta_motor->record = record;

	/* A delta motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_delta_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_delta_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_RECORD *moving_motor_record;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	mx_status = mxd_delta_motor_get_pointers( motor, NULL,
						&moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->real_motor_record = moving_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_delta_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_delta_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_delta_motor_get_pointers( motor,
				&delta_motor, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type      = DELTA_MOTOR.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  moving motor    = %s\n", moving_motor_record->name);
	fprintf(file, "  fixed position  = %s\n",
				delta_motor->fixed_position_record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position        = %g %s\n",
		position, motor->units);
	fprintf(file, "  delta scale    = %g %s per unscaled delta unit.\n",
		motor->scale, motor->units);
	fprintf(file, "  delta offset   = %g %s.\n",
		motor->offset, motor->units);
	fprintf(file, "  backlash        = %g %s.\n",
		motor->backlash_correction, motor->units);
	fprintf(file, "  negative limit  = %g %s.\n",
		motor->negative_limit, motor->units);
	fprintf(file, "  positive limit  = %g %s.\n",
		motor->positive_limit, motor->units);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move_deadband        = %g %s\n\n",
		move_deadband, motor->units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_delta_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_move_absolute()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	double new_delta_position;
	double fixed_position_value;
	double new_moving_motor_position;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor,
				&delta_motor, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check the limits on this request. */

	new_delta_position = motor->raw_destination.analog;

	if ( new_delta_position > motor->raw_positive_limit.analog
	  || new_delta_position < motor->raw_negative_limit.analog )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested position %g is outside the allowed range of %g to %g",
			new_delta_position, motor->raw_negative_limit.analog,
			motor->raw_positive_limit.analog );
	}

	/* Get the fixed position value. */

	mx_status = mxd_delta_motor_get_fixed_position_value(
				delta_motor, &fixed_position_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the new moving motor position. */

	new_moving_motor_position = fixed_position_value + new_delta_position;

	/* Now perform the move. */

	mx_status = mx_motor_move_absolute( moving_motor_record,
				new_moving_motor_position, MXF_MTR_NOWAIT );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_delta_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_get_position()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	double fixed_position_value;
	double moving_motor_position;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor,
				&delta_motor, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current moving motor position. */

	mx_status = mx_motor_get_position(
			moving_motor_record, &moving_motor_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the fixed position value. */

	mx_status = mxd_delta_motor_get_fixed_position_value(
				delta_motor, &fixed_position_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the pseudo motor position. */

	motor->raw_position.analog
			= moving_motor_position - fixed_position_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_delta_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_set_position()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	double new_delta_position;
	double fixed_position_value;
	double new_moving_motor_position;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor,
				&delta_motor, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check the limits on this request. */

	new_delta_position = motor->raw_set_position.analog;

	if ( new_delta_position > motor->raw_positive_limit.analog
	  || new_delta_position < motor->raw_negative_limit.analog )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested position %g is outside the allowed range of %g to %g",
			new_delta_position, motor->raw_negative_limit.analog,
			motor->raw_positive_limit.analog );
	}

	/* Get the fixed position value. */

	mx_status = mxd_delta_motor_get_fixed_position_value(
				delta_motor, &fixed_position_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the new moving motor position. */

	new_moving_motor_position = fixed_position_value + new_delta_position;

	/* Now redefine the position. */

	mx_status = mx_motor_set_position( moving_motor_record,
						new_moving_motor_position );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_delta_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_soft_abort()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor,
				&delta_motor, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( moving_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_delta_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_immediate_abort()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor,
				&delta_motor, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_immediate_abort( moving_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_delta_motor_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_find_home_position()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	int direction;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor,
				&delta_motor, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->scale < 0.0 ) {
		direction = - ( motor->home_search );
	} else {
		direction = motor->home_search;
	}

	mx_status = mx_motor_find_home_position( moving_motor_record,
							direction );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_delta_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_constant_velocity_move()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	int direction;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor,
				&delta_motor, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->scale < 0.0 ) {
		direction = - ( motor->constant_velocity_move );
	} else {
		direction = motor->constant_velocity_move;
	}

	mx_status = mx_motor_constant_velocity_move( moving_motor_record,
							direction );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_delta_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_get_parameter()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	double speed, acceleration_time, fixed_position_value;
	double moving_motor_start, moving_motor_end;
	double real_moving_motor_start, real_moving_motor_end;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor, &delta_motor,
					&moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mx_motor_get_speed( moving_motor_record, &speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_speed = speed;
		break;
	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_motor_get_acceleration_time( moving_motor_record,
							&acceleration_time );

		motor->acceleration_time = acceleration_time;
		break;
	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		mx_status = mxd_delta_motor_get_fixed_position_value(
				delta_motor, &fixed_position_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		moving_motor_start = motor->raw_compute_extended_scan_range[0]
					+ fixed_position_value;
		moving_motor_end = motor->raw_compute_extended_scan_range[1]
					+ fixed_position_value;

		mx_status = mx_motor_compute_extended_scan_range(
					moving_motor_record,
				moving_motor_start, moving_motor_end,
			&real_moving_motor_start, &real_moving_motor_end );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_compute_extended_scan_range[2] =
				real_moving_motor_start - fixed_position_value;
		motor->raw_compute_extended_scan_range[3] =
				real_moving_motor_end - fixed_position_value;
		break;
	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		mx_status = mxd_delta_motor_get_fixed_position_value(
				delta_motor, &fixed_position_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->compute_pseudomotor_position[1] =
		  motor->compute_pseudomotor_position[0] - fixed_position_value;

		break;
	case MXLV_MTR_COMPUTE_REAL_POSITION:
		mx_status = mxd_delta_motor_get_fixed_position_value(
				delta_motor, &fixed_position_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->compute_real_position[1] =
			motor->compute_real_position[0] + fixed_position_value;

		break;
	default:
		mx_status = mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_delta_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_set_parameter()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	double fixed_position_value, time_for_move;
	double real_position1, real_position2;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor, &delta_motor,
					&moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mx_motor_set_speed( moving_motor_record,
						motor->raw_speed );
		break;
	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		mx_status = mxd_delta_motor_get_fixed_position_value(
				delta_motor, &fixed_position_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		real_position1 = motor->raw_speed_choice_parameters[0]
					+ fixed_position_value;
		real_position2 = motor->raw_speed_choice_parameters[1]
					+ fixed_position_value;
		time_for_move = motor->raw_speed_choice_parameters[2];

		mx_status = mx_motor_set_speed_between_positions(
					moving_motor_record,
					real_position1, real_position2,
					time_for_move );
		break;
	case MXLV_MTR_SAVE_SPEED:
		mx_status = mx_motor_save_speed( moving_motor_record );
		break;
	case MXLV_MTR_RESTORE_SPEED:
		mx_status = mx_motor_restore_speed( moving_motor_record );
		break;
	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_delta_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_delta_motor_get_status()";

	MX_DELTA_MOTOR *delta_motor;
	MX_RECORD *moving_motor_record;
	mx_hex_type motor_status;
	mx_status_type mx_status;

	mx_status = mxd_delta_motor_get_pointers( motor,
				&delta_motor, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status( moving_motor_record, &motor_status);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reverse the direction of the motor limit bits if needed. */

	motor->status = motor_status;

	if ( motor->scale < 0.0 ) {
		if ( motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_NEGATIVE_LIMIT_HIT );
		}

		if ( motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_POSITIVE_LIMIT_HIT );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*======== Private functions for the use of the driver only. ========*/

static mx_status_type
mxd_delta_motor_get_fixed_position_value(
		MX_DELTA_MOTOR *delta_motor,
		double *fixed_position_value )
{
	static const char fname[] = "mxd_delta_motor_get_fixed_position()";

	MX_RECORD *fixed_position_record;
	void *pointer_to_value;
	mx_bool_type fast_mode;
	mx_status_type mx_status;

	if ( delta_motor == (MX_DELTA_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL pointer passed." );
	}

	fixed_position_record = delta_motor->fixed_position_record;

	if ( fixed_position_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"fixed_position_record pointer for 'delta_motor' record '%s' is NULL.",
			delta_motor->record->name );
	}

	mx_status = mx_get_fast_mode( delta_motor->record, &fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fast_mode ) {
		*fixed_position_value = delta_motor->saved_fixed_position_value;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the fixed position value. */

	switch ( fixed_position_record->mx_superclass ) {
	case MXR_VARIABLE:
		switch( fixed_position_record->mx_type ) {
		case MXV_NET_DOUBLE:
		case MXV_EPI_DOUBLE:
			mx_status = mx_receive_variable(fixed_position_record);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Fall through to the next case. */

		case MXV_INL_DOUBLE:
			mx_status = mx_get_variable_pointer(
				fixed_position_record, &pointer_to_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			*fixed_position_value = *((double *) pointer_to_value);

			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
		"The variable type %ld for record '%s' is not supported "
		"for the fixed position of a 'delta_motor' in record '%s'.",
				fixed_position_record->mx_type,
				fixed_position_record->name,
				delta_motor->record->name );
		}
		break;
	case MXR_DEVICE:
		switch( fixed_position_record->mx_class ) {
		case MXC_MOTOR:
			mx_status = mx_motor_get_position(
				fixed_position_record, fixed_position_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
		"The device class %ld for record '%s' is not supported "
		"for the fixed position of a 'delta_motor' in record '%s'.",
				fixed_position_record->mx_class,
				fixed_position_record->name,
				delta_motor->record->name );
		}
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"The record superclass %ld for record '%s' is not supported "
		"for the fixed position of a 'delta_motor' in record '%s'.",
			fixed_position_record->mx_superclass,
			fixed_position_record->name,
			delta_motor->record->name );
	}

	delta_motor->saved_fixed_position_value = *fixed_position_value;

	return MX_SUCCESSFUL_RESULT;
}

