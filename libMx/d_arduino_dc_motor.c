/*
 * Name:    d_arduino_dc_motor.c 
 *
 * Purpose: MX motor driver for using the Arduino Motor Shield to control
 *          one of the two available DC motor channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ARDUINO_DC_MOTOR_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "mx_encoder.h"
#include "mx_motor.h"
#include "d_arduino_dc_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_arduino_dc_motor_record_function_list = {
	NULL,
	mxd_arduino_dc_motor_create_record_structures,
	mxd_arduino_dc_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_arduino_dc_motor_open,
};

MX_MOTOR_FUNCTION_LIST mxd_arduino_dc_motor_motor_function_list = {
	mxd_arduino_dc_motor_is_busy,
	mxd_arduino_dc_motor_move_absolute,
	mxd_arduino_dc_motor_get_position,
	mxd_arduino_dc_motor_set_position,
	NULL,
	mxd_arduino_dc_motor_immediate_abort,
	NULL,
	NULL,
	NULL,
	mxd_arduino_dc_motor_constant_velocity_move,
	mxd_arduino_dc_motor_get_parameter,
	mxd_arduino_dc_motor_set_parameter
};

/* Pontech ARDUINO_DC_MOTOR motor controller data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_arduino_dc_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_ARDUINO_DC_MOTOR_STANDARD_FIELDS
};

long mxd_arduino_dc_motor_num_record_fields
		= sizeof( mxd_arduino_dc_motor_record_field_defaults )
			/ sizeof( mxd_arduino_dc_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_arduino_dc_motor_rfield_def_ptr
			= &mxd_arduino_dc_motor_record_field_defaults[0];

/* === */

static mx_status_type
mxd_arduino_dc_motor_get_pointers( MX_MOTOR *motor,
			MX_ARDUINO_DC_MOTOR **arduino_dc_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_arduino_dc_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( arduino_dc_motor == (MX_ARDUINO_DC_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ARDUINO_DC_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*arduino_dc_motor = (MX_ARDUINO_DC_MOTOR *)
				motor->record->record_type_struct;

	if ( *arduino_dc_motor == (MX_ARDUINO_DC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ARDUINO_DC_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_arduino_dc_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_ARDUINO_DC_MOTOR *arduino_dc_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_MOTOR structure." );
	}

	arduino_dc_motor = (MX_ARDUINO_DC_MOTOR *)
				malloc( sizeof(MX_ARDUINO_DC_MOTOR) );

	if ( arduino_dc_motor == (MX_ARDUINO_DC_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_ARDUINO_DC_MOTOR structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = arduino_dc_motor;
	record->class_specific_function_list =
				&mxd_arduino_dc_motor_motor_function_list;

	motor->record = record;
	arduino_dc_motor->record = record;

	/* An Arduino DC motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* This driver stores the acceleration time. */

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	motor->raw_speed = 0.0;
	motor->raw_base_speed = 0.0;

	arduino_dc_motor->move_in_progress = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_arduino_dc_motor_finish_record_initialization()";

	MX_ARDUINO_DC_MOTOR *arduino_dc_motor;
	mx_status_type mx_status;

	arduino_dc_motor = (MX_ARDUINO_DC_MOTOR *) record->record_type_struct;

	if ( arduino_dc_motor == (MX_ARDUINO_DC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"record_type_struct for record '%s' is NULL.",
			record->name );
	}

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_arduino_dc_motor_open()";

	MX_MOTOR *motor = NULL;
	MX_ARDUINO_DC_MOTOR *arduino_dc_motor = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_arduino_dc_motor_get_pointers(motor,
						&arduino_dc_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if a position encoder is available. */

	if ( strlen( arduino_dc_motor->encoder_record_name ) == 0 ) {
		arduino_dc_motor->encoder_record = NULL;
	} else {
		arduino_dc_motor->encoder_record = mx_get_record( record,
					arduino_dc_motor->encoder_record_name );

		if ( arduino_dc_motor->encoder_record == NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"The encoder record '%s' for Arduino DC motor '%s' "
			"was not found.",
				arduino_dc_motor->encoder_record_name,
				record->name );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_arduino_dc_motor_is_busy()";

	MX_ARDUINO_DC_MOTOR *arduino_dc_motor = NULL;
	unsigned long encoder_status;
	mx_status_type mx_status;

	mx_status = mxd_arduino_dc_motor_get_pointers(motor,
						&arduino_dc_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( arduino_dc_motor->encoder_record == (MX_RECORD *) NULL ) {

		if ( arduino_dc_motor->move_in_progress ) {
			motor->busy = TRUE;
		} else {
			motor->busy = FALSE;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_encoder_get_status( arduino_dc_motor->encoder_record,
						&encoder_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( encoder_status & MXSF_ENC_MOVING ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_arduino_dc_motor_move_absolute()";

	MX_ARDUINO_DC_MOTOR *arduino_dc_motor = NULL;
	long direction;
	mx_status_type mx_status;

	mx_status = mxd_arduino_dc_motor_get_pointers(motor,
						&arduino_dc_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->raw_destination.analog >= 0.0 ) {
		direction = 1;
	} else {
		direction = -1;
	}

	mx_status = mx_motor_constant_velocity_move( motor->record,
							direction );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_arduino_dc_motor_get_position()";

	MX_ARDUINO_DC_MOTOR *arduino_dc_motor = NULL;
	double raw_position;
	mx_status_type mx_status;

	mx_status = mxd_arduino_dc_motor_get_pointers(motor,
						&arduino_dc_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( arduino_dc_motor->encoder_record == NULL ) {
		raw_position = 0.0;
	} else {
		mx_status = mx_encoder_read( arduino_dc_motor->encoder_record,
							&raw_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	motor->raw_position.analog = raw_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_arduino_dc_motor_set_position()";

	MX_ARDUINO_DC_MOTOR *arduino_dc_motor = NULL;
	double raw_position;
	mx_status_type mx_status;

	mx_status = mxd_arduino_dc_motor_get_pointers(motor,
						&arduino_dc_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( arduino_dc_motor->encoder_record == NULL ) {
		motor->raw_position.analog = 0.0;

		return MX_SUCCESSFUL_RESULT;
	}

	raw_position = motor->raw_set_position.analog;

	mx_status = mx_encoder_write( arduino_dc_motor->encoder_record,
								raw_position );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_arduino_dc_motor_immediate_abort()";

	MX_ARDUINO_DC_MOTOR *arduino_dc_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_arduino_dc_motor_get_pointers(motor,
						&arduino_dc_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the requested speed to 0. */

	mx_status = mx_motor_set_speed( motor->record, 0.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	mx_msleep(5000);
#endif

	/* Set the brake. */

	mx_status = mx_digital_output_write(
			arduino_dc_motor->brake_pin_record, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] =
		"mxd_arduino_dc_motor_constant_velocity_move()";

	MX_ARDUINO_DC_MOTOR *arduino_dc_motor = NULL;
	unsigned long direction;
	unsigned long pwm_value;
	double pwm_real8;
	mx_status_type mx_status;

	mx_status = mxd_arduino_dc_motor_get_pointers(motor,
						&arduino_dc_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the direction of the next move. */

	if ( motor->speed >= 0.0 ) {
		direction = 1;
	} else {
		direction = 0;
	}

	mx_status = mx_digital_output_write(
			arduino_dc_motor->direction_pin_record, direction );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Release the brake. */

	mx_status = mx_digital_output_write(
			arduino_dc_motor->brake_pin_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the motor speed. */

	/* We start by reading the most recently requested speed. */

	mx_status = mx_motor_get_speed( motor->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Convert the requested speed into a PWM value. */

	pwm_real8 = mx_divide_safely( motor->speed, 256.0 );

	pwm_real8 = fabs( pwm_real8 );

	pwm_value = mx_round( pwm_real8 );

	/* Set the PWM value.  This should start the move. */

	mx_status = mx_digital_output_write(
			arduino_dc_motor->pwm_pin_record, pwm_value );

	if ( mx_status.code == MXE_SUCCESS ) {
		arduino_dc_motor->move_in_progress = TRUE;
	} else {
		arduino_dc_motor->move_in_progress = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_arduino_dc_motor_get_parameter()";

	MX_ARDUINO_DC_MOTOR *arduino_dc_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_arduino_dc_motor_get_pointers(motor,
						&arduino_dc_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* The speed parameter is used when a throttle 't' command
		 * is requested.
		 */
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_arduino_dc_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_arduino_dc_motor_set_parameter()";

	MX_ARDUINO_DC_MOTOR *arduino_dc_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_arduino_dc_motor_get_pointers(motor,
						&arduino_dc_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* The speed parameter is used when a throttle 't' command
		 * is requested.
		 */
		break;
	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

