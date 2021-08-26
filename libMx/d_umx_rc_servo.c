/*
 * Name:    d_umx_rc_servo.c
 *
 * Purpose: MX driver for UMX-controlled RC servo motors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_UMX_RC_SERVO_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_umx.h"
#include "d_umx_rc_servo.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_umx_rc_servo_record_function_list = {
	NULL,
	mxd_umx_rc_servo_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_umx_rc_servo_open
};

MX_MOTOR_FUNCTION_LIST mxd_umx_rc_servo_motor_function_list = {
	NULL,
	mxd_umx_rc_servo_move_absolute,
	mxd_umx_rc_servo_get_position
};

/* UMX RC servo motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_umx_rc_servo_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_UMX_RC_SERVO_STANDARD_FIELDS
};

long mxd_umx_rc_servo_num_record_fields
		= sizeof( mxd_umx_rc_servo_record_field_defaults )
			/ sizeof( mxd_umx_rc_servo_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_umx_rc_servo_rfield_def_ptr
			= &mxd_umx_rc_servo_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_umx_rc_servo_get_pointers( MX_MOTOR *motor,
				MX_UMX_RC_SERVO **umx_rc_servo,
				MX_RECORD **umx_record,
				const char *calling_fname )
{
	static const char fname[] = "mxd_umx_rc_servo_get_pointers()";

	MX_RECORD *record = NULL;
	MX_UMX_RC_SERVO *umx_rc_servo_ptr = NULL;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = motor->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_RECORD pointer for the MX_MOTOR pointer passed was NULL." );
	}

	umx_rc_servo_ptr = (MX_UMX_RC_SERVO *) record->record_type_struct;

	if ( umx_rc_servo_ptr == (MX_UMX_RC_SERVO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_UMX_RC_SERVO pointer for digital output '%s' is NULL.",
			record->name );
	}

	if ( umx_rc_servo != (MX_UMX_RC_SERVO **) NULL ) {
		*umx_rc_servo = umx_rc_servo_ptr;
	}

	if ( umx_record != (MX_RECORD **) NULL ) {
		*umx_record = umx_rc_servo_ptr->umx_record;

		if ( (*umx_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The 'umx_record' pointer for digital output '%s' is NULL.",
								record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_umx_rc_servo_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_umx_rc_servo_create_record_structures()";

	MX_MOTOR *motor;
	MX_UMX_RC_SERVO *umx_rc_servo;

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	umx_rc_servo = (MX_UMX_RC_SERVO *) malloc( sizeof(MX_UMX_RC_SERVO) );

	if ( umx_rc_servo == (MX_UMX_RC_SERVO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_UMX_RC_SERVO structure." );
	}

	record->record_class_struct = motor;
	record->record_type_struct = umx_rc_servo;
	record->class_specific_function_list =
				&mxd_umx_rc_servo_motor_function_list;

	motor->record = record;
	umx_rc_servo->record = record;

	/* A UMX RC servo motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The UMX RC servo does not have user-controllable acceleration. */

	motor->acceleration_type = MXF_MTR_ACCEL_NONE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_umx_rc_servo_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_umx_rc_servo_open()";

	MX_MOTOR *motor = NULL;
	MX_UMX_RC_SERVO *umx_rc_servo = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	umx_rc_servo = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_umx_rc_servo_get_pointers( motor,
					&umx_rc_servo, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"PUT RC%lu.servo_pin %lu",
		umx_rc_servo->servo_number,
		umx_rc_servo->servo_pin );

	mx_status = mx_umx_command( umx_record, command,
				response, sizeof(response), FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( record, NULL );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_umx_rc_servo_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_umx_rc_servo_move_absolute()";

	MX_UMX_RC_SERVO *umx_rc_servo = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	umx_rc_servo = NULL;

	mx_status = mxd_umx_rc_servo_get_pointers( motor,
					&umx_rc_servo, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"PUT RC%lu.destination %lu",
		umx_rc_servo->servo_number,
		motor->raw_destination.stepper );

	mx_status = mx_umx_command( umx_record, command,
				response, sizeof(response), FALSE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_umx_rc_servo_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_umx_rc_servo_get_position()";

	MX_UMX_RC_SERVO *umx_rc_servo;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	umx_rc_servo = NULL;

	mx_status = mxd_umx_rc_servo_get_pointers( motor,
					&umx_rc_servo, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"GET RC%lu.position",
		umx_rc_servo->servo_number );

	mx_status = mx_umx_command( umx_record, command,
				response, sizeof(response), FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "$%ld", &(motor->raw_position.stepper) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not see the position of UMX RC servo motor '%s' "
		"in the response '%s' to command '%s'.",
			motor->record->name, response, command );
	}

	return MX_SUCCESSFUL_RESULT;
}

