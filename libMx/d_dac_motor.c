/*
 * Name:    d_dac_motor.c 
 *
 * Purpose: MX motor driver to control an MX analog output device as if
 *          it were a motor.  The most common use of this is to control
 *          piezoelectric motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2006 Illinois Institute of Technology
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
#include "mx_analog_output.h"
#include "d_dac_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_dac_motor_record_function_list = {
	NULL,
	mxd_dac_motor_create_record_structures,
	mx_motor_finish_record_initialization
};

MX_MOTOR_FUNCTION_LIST mxd_dac_motor_motor_function_list = {
	mxd_dac_motor_motor_is_busy,
	mxd_dac_motor_move_absolute,
	mxd_dac_motor_get_position,
	mxd_dac_motor_set_position,
	mxd_dac_motor_soft_abort,
	mxd_dac_motor_immediate_abort,
	mxd_dac_motor_positive_limit_hit,
	mxd_dac_motor_negative_limit_hit
};

/* DAC motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_dac_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_DAC_MOTOR_STANDARD_FIELDS
};

mx_length_type mxd_dac_motor_num_record_fields
		= sizeof( mxd_dac_motor_record_field_defaults )
			/ sizeof( mxd_dac_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_dac_motor_rfield_def_ptr
			= &mxd_dac_motor_record_field_defaults[0];

/* === */

static mx_status_type
mxd_dac_motor_get_pointers( MX_MOTOR *motor,
			MX_DAC_MOTOR **dac_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_dac_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dac_motor == (MX_DAC_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DAC_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*dac_motor = (MX_DAC_MOTOR *) motor->record->record_type_struct;

	if ( *dac_motor == (MX_DAC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DAC_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_dac_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_dac_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_DAC_MOTOR *dac_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	dac_motor = (MX_DAC_MOTOR *) malloc( sizeof(MX_DAC_MOTOR) );

	if ( dac_motor == (MX_DAC_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_DAC_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = dac_motor;
	record->class_specific_function_list
				= &mxd_dac_motor_motor_function_list;

	motor->record = record;

	/* A DAC motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_motor_is_busy( MX_MOTOR *motor )
{
	/* A DAC motor is never busy. */

	motor->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dac_motor_move_absolute()";

	MX_DAC_MOTOR *dac_motor;
	mx_status_type mx_status;

	mx_status = mxd_dac_motor_get_pointers( motor, &dac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_analog_output_write( dac_motor->dac_record,
					motor->raw_destination.analog );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dac_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dac_motor_get_position()";

	MX_DAC_MOTOR *dac_motor;
	double present_value;
	mx_status_type mx_status;

	mx_status = mxd_dac_motor_get_pointers( motor, &dac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_analog_output_read( dac_motor->dac_record,
						&present_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = present_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dac_motor_set_position()";

	double negative_limit, positive_limit;
	double position, old_position, position_delta;
	mx_status_type mx_status;

	/* The only plausible interpretation I can think of
	 * for this function here is to change the motor
	 * offset value.  We must check whether or not doing
	 * such a thing would cause the new value of the
	 * motor position to fall outside the software limits.
	 */

	position = motor->raw_set_position.analog;

	negative_limit = motor->raw_negative_limit.analog;
	positive_limit = motor->raw_positive_limit.analog;

	if ( position < negative_limit ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Requested new set point %g would exceed the negative limit of %g",
			position, negative_limit );
	}
	if ( position < positive_limit ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Requested new set point %g would exceed the positive limit of %g",
			position, positive_limit );
	}

	mx_status = mx_motor_get_position( motor->record, &old_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	position_delta = position - old_position;

	motor->offset += position_delta;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_soft_abort( MX_MOTOR *motor )
{
	/* This routine does nothing. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_immediate_abort( MX_MOTOR *motor )
{
	/* This routine does nothing. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_positive_limit_hit( MX_MOTOR *motor )
{
	double present_position;
	mx_status_type mx_status;

	mx_status = mx_motor_get_position( motor->record, &present_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( present_position > motor->raw_positive_limit.analog ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_negative_limit_hit( MX_MOTOR *motor )
{
	double present_position;
	mx_status_type mx_status;

	mx_status = mx_motor_get_position( motor->record, &present_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( present_position < motor->raw_negative_limit.analog ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

