/*
 * Name:    d_linkam_t9x_temp.c
 *
 * Purpose: MX driver for the temperature control part of Linkam T9x series
 *          cooling system controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_LINKAM_T9X_TEMP_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "i_linkam_t9x.h"
#include "d_linkam_t9x_temp.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_linkam_t9x_temp_record_function_list = {
	NULL,
	mxd_linkam_t9x_temp_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_linkam_t9x_temp_open
};

MX_MOTOR_FUNCTION_LIST mxd_linkam_t9x_temp_motor_function_list = {
	NULL,
	mxd_linkam_t9x_temp_move_absolute,
	NULL,
	NULL,
	mxd_linkam_t9x_temp_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_linkam_t9x_temp_get_parameter,
	mxd_linkam_t9x_temp_set_parameter,
	NULL,
	NULL,
	mxd_linkam_t9x_temp_get_extended_status
};

MX_RECORD_FIELD_DEFAULTS mxd_linkam_t9x_temp_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_LINKAM_T9X_TEMP_STANDARD_FIELDS
};

long mxd_linkam_t9x_temp_num_record_fields
		= sizeof( mxd_linkam_t9x_temp_record_field_defaults )
			/ sizeof( mxd_linkam_t9x_temp_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_linkam_t9x_temp_rfield_def_ptr
			= &mxd_linkam_t9x_temp_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_linkam_t9x_temp_get_pointers( MX_MOTOR *motor,
			MX_LINKAM_T9X_TEMPERATURE **linkam_t9x_temp,
			MX_LINKAM_T9X **linkam_t9x,
			const char *calling_fname )
{
	static const char fname[] = "mxd_linkam_t9x_temp_get_pointers()";

	MX_LINKAM_T9X_TEMPERATURE *linkam_t9x_temp_ptr;
	MX_RECORD *linkam_t9x_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	linkam_t9x_temp_ptr = (MX_LINKAM_T9X_TEMPERATURE *)
					motor->record->record_type_struct;

	if ( linkam_t9x_temp_ptr == (MX_LINKAM_T9X_TEMPERATURE *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINKAM_T9X_TEMPERATURE pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( linkam_t9x_temp != (MX_LINKAM_T9X_TEMPERATURE **) NULL ) {
		*linkam_t9x_temp = linkam_t9x_temp_ptr;
	}

	if ( linkam_t9x != (MX_LINKAM_T9X **) NULL ) {
		linkam_t9x_record = linkam_t9x_temp_ptr->linkam_t9x_record;

		if ( linkam_t9x_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		  "The linkam_t9x_record pointer for record '%s' is NULL.",
				motor->record->name );
		}

		*linkam_t9x = (MX_LINKAM_T9X *)
				linkam_t9x_record->record_type_struct;

		if ( *linkam_t9x == (MX_LINKAM_T9X *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		  "The MX_LINKAM_T9X pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_temp_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_linkam_t9x_temp_create_record_structures()";

	MX_MOTOR *motor;
	MX_LINKAM_T9X_TEMPERATURE *linkam_t9x_temperature = NULL;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	linkam_t9x_temperature = (MX_LINKAM_T9X_TEMPERATURE *)
				malloc( sizeof(MX_LINKAM_T9X_TEMPERATURE) );

	if ( linkam_t9x_temperature == (MX_LINKAM_T9X_TEMPERATURE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_LINKAM_T9X_TEMPERATURE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = linkam_t9x_temperature;
	record->class_specific_function_list
		= &mxd_linkam_t9x_temp_motor_function_list;

	motor->record = record;
	linkam_t9x_temperature->record = record;

	/* The temperature control part of a Linkam T9x controller
	 * is treated as an analog motor.
	 */

	motor->subclass = MXC_MTR_ANALOG;

	/* We express accelerations in counts/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_temp_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_linkam_t9x_temp_open()";

	MX_MOTOR *motor = NULL;
	MX_LINKAM_T9X_TEMPERATURE *linkam_t9x_temp = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = record->record_class_struct;

	mx_status = mxd_linkam_t9x_temp_get_pointers( motor,
					&linkam_t9x_temp, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the initial ramp speed in degrees C/second. */

	MX_DEBUG(-2,("%s: linkam_t9x_temp->initial_speed = %f",
		fname, linkam_t9x_temp->initial_speed ));

	mx_status = mx_motor_set_speed( record,
				linkam_t9x_temp->initial_speed );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_linkam_t9x_temp_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_temp_move_absolute()";

	MX_LINKAM_T9X_TEMPERATURE *linkam_t9x_temp = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	char command[80];
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_temp_get_pointers( motor,
					&linkam_t9x_temp, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the destination. */

	motor_steps = mx_round( 10.0 * motor->raw_destination.analog );

	snprintf( command, sizeof(command), "L1%ld", motor_steps );

	mx_status = mxi_linkam_t9x_command( linkam_t9x, command,
					NULL, 0, MXD_LINKAM_T9X_TEMP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the move. */

	mx_status = mxi_linkam_t9x_command( linkam_t9x, "S",
					NULL, 0, MXD_LINKAM_T9X_TEMP_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_temp_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_temp_soft_abort()";

	MX_LINKAM_T9X_TEMPERATURE *linkam_t9x_temp = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_temp_get_pointers( motor,
					&linkam_t9x_temp, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_linkam_t9x_command( linkam_t9x, "O",
					NULL, 0, MXD_LINKAM_T9X_TEMP_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_temp_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_temp_get_parameter()";

	MX_LINKAM_T9X_TEMPERATURE *linkam_t9x_temp = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_temp_get_pointers( motor,
					&linkam_t9x_temp, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
	case MXLV_MTR_PROPORTIONAL_GAIN:
	case MXLV_MTR_INTEGRAL_GAIN:
	case MXLV_MTR_DERIVATIVE_GAIN:
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_temp_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_temp_set_parameter()";

	MX_LINKAM_T9X_TEMPERATURE *linkam_t9x_temp = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	char command[80];
	long t9x_speed;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_temp_get_pointers( motor,
					&linkam_t9x_temp, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* MX uses C/sec for speed.  We must convert this to the
		 * 100 * C/min used by the controller serial commands.
		 */

		t9x_speed = mx_round( 100.0 * 60.0 * motor->raw_speed );

		snprintf( command, sizeof(command), "R1%ld", t9x_speed );

		mx_status = mxi_linkam_t9x_command( linkam_t9x,
						command, NULL, 0,
						MXD_LINKAM_T9X_TEMP_DEBUG );
		break;
	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_temp_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_temp_get_extended_status()";

	MX_LINKAM_T9X_TEMPERATURE *linkam_t9x_temp = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_temp_get_pointers( motor,
					&linkam_t9x_temp, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Ask the Linkam T9x controller for its current status. */

	mx_status = mxi_linkam_t9x_get_status( linkam_t9x,
						MXD_LINKAM_T9X_TEMP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The current temperature has already been compute by the function
	 * mxi_linkam_t9x_get_status(), so we merely fetch the value from
	 * where it was put.
	 */

	motor->raw_position.analog = linkam_t9x->temperature;

	/* Get the motor status from the status byte and the error byte. */

	motor->status = 0;

	switch( linkam_t9x->status_byte ) {
	case 0x01:
		break;
	case 0x10:
	case 0x20:
		motor->status |= MXSF_MTR_IS_BUSY;
		break;
	case 0x30:
	case 0x40:
	case 0x50:
		/* The various hold conditions are treated as not busy. */
		break;
	default:
		mx_warning( "Unexpected status byte value %#x seen for "
		"Linkam T9x controller '%s'",
			linkam_t9x->status_byte,
			linkam_t9x_temp->record->name );
		break;
	}

	mx_status = mxi_linkam_t9x_set_motor_status_from_error_byte(
						linkam_t9x, motor->record );

	return mx_status;
}

/*-----------*/


