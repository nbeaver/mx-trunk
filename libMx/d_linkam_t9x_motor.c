/*
 * Name:    d_linkam_t9x_motor.c
 *
 * Purpose: MX driver for the motion control part of Linkam T9x series
 *          cooling system controllers.
 *
 * Author:  William Lavender
 *
 * Note:    Raw units for this driver are always in um (micrometers).
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010, 2013, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_LINKAM_T9X_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_hrt.h"
#include "mx_motor.h"
#include "i_linkam_t9x.h"
#include "d_linkam_t9x_motor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_linkam_t9x_motor_record_function_list = {
	NULL,
	mxd_linkam_t9x_motor_create_record_structures,
	mxd_linkam_t9x_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_linkam_t9x_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_linkam_t9x_motor_motor_function_list = {
	NULL,
	mxd_linkam_t9x_motor_move_absolute,
	NULL,
	NULL,
	mxd_linkam_t9x_motor_soft_abort,
	mxd_linkam_t9x_motor_immediate_abort,
	NULL,
	NULL,
	mxd_linkam_t9x_motor_raw_home_command,
	NULL,
	mxd_linkam_t9x_motor_get_parameter,
	mxd_linkam_t9x_motor_set_parameter,
	NULL,
	NULL,
	mxd_linkam_t9x_motor_get_extended_status
};

MX_RECORD_FIELD_DEFAULTS mxd_linkam_t9x_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_LINKAM_T9X_MOTOR_STANDARD_FIELDS
};

long mxd_linkam_t9x_motor_num_record_fields
		= sizeof( mxd_linkam_t9x_motor_record_field_defaults )
			/ sizeof( mxd_linkam_t9x_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_linkam_t9x_motor_rfield_def_ptr
			= &mxd_linkam_t9x_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_linkam_t9x_motor_get_pointers( MX_MOTOR *motor,
			MX_LINKAM_T9X_MOTOR **linkam_t9x_motor,
			MX_LINKAM_T9X **linkam_t9x,
			const char *calling_fname )
{
	static const char fname[] = "mxd_linkam_t9x_motor_get_pointers()";

	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor_ptr;
	MX_RECORD *linkam_t9x_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	linkam_t9x_motor_ptr = (MX_LINKAM_T9X_MOTOR *)
					motor->record->record_type_struct;

	if ( linkam_t9x_motor_ptr == (MX_LINKAM_T9X_MOTOR *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINKAM_T9X_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( linkam_t9x_motor != (MX_LINKAM_T9X_MOTOR **) NULL ) {
		*linkam_t9x_motor = linkam_t9x_motor_ptr;
	}

	if ( linkam_t9x != (MX_LINKAM_T9X **) NULL ) {
		linkam_t9x_record = linkam_t9x_motor_ptr->linkam_t9x_record;

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
mxd_linkam_t9x_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_linkam_t9x_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	linkam_t9x_motor = (MX_LINKAM_T9X_MOTOR *)
				malloc( sizeof(MX_LINKAM_T9X_MOTOR) );

	if ( linkam_t9x_motor == (MX_LINKAM_T9X_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_LINKAM_T9X_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = linkam_t9x_motor;
	record->class_specific_function_list
		= &mxd_linkam_t9x_motor_motor_function_list;

	motor->record = record;
	linkam_t9x_motor->record = record;

	/* The motion control part of a Linkam T9x controller
	 * is treated as an analog motor.
	 */

	motor->subclass = MXC_MTR_ANALOG;

	motor->acceleration_type = MXF_MTR_ACCEL_NONE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_linkam_t9x_motor_finish_record_initialization()";

	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	linkam_t9x_motor = (MX_LINKAM_T9X_MOTOR *) record->record_type_struct;

	if ( linkam_t9x_motor == (MX_LINKAM_T9X_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINKHAM_T9X_MOTOR pointer for motor '%s' is NULL.",
			record->name );
	}

	if ( islower( (int) linkam_t9x_motor->axis_name) ) {
		linkam_t9x_motor->axis_name =
			toupper( (int) linkam_t9x_motor->axis_name);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_linkam_t9x_motor_open()";

	MX_MOTOR *motor = NULL;
	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = record->record_class_struct;

	mx_status = mxd_linkam_t9x_motor_get_pointers( motor,
					&linkam_t9x_motor, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the initial ramp speed in micrometers/second. */

#if MXD_LINKAM_T9X_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: linkam_t9x_motor->initial_speed = %f",
		fname, linkam_t9x_motor->initial_speed ));
#endif

	mx_status = mx_motor_set_speed( record,
				linkam_t9x_motor->initial_speed );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_linkam_t9x_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_motor_move_absolute()";

	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	char command[80];
	double current_position, absolute_destination, new_destination;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_motor_get_pointers( motor,
					&linkam_t9x_motor, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current position. */

	mx_status = mxd_linkam_t9x_motor_get_extended_status( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	current_position = motor->raw_position.analog;

	/* Construct the move command. */

	absolute_destination = motor->raw_destination.analog;

	if ( linkam_t9x->moves_are_relative ) {
		new_destination = absolute_destination - current_position;
	} else {
		new_destination = absolute_destination;
	}

	switch( linkam_t9x_motor->axis_name ) {
	case 'X':
		snprintf( command, sizeof(command),
			"MMX%ld", mx_round(new_destination) );
		break;
	case 'Y':
		snprintf( command, sizeof(command),
			"MMY%ld", mx_round(new_destination) );
		break;
	case 'Z':
		snprintf( command, sizeof(command),
			"MMZ%ld", mx_round(new_destination) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The axis name '%c' specified for Linkam T9x motor '%s' "
		"is not valid.  The allowed names are 'X', 'Y', and 'Z'.",
			linkam_t9x_motor->axis_name,
			motor->record->name );
	}

	/* Start the move. */

	mx_status = mxi_linkam_t9x_command( linkam_t9x, command,
					NULL, 0, MXD_LINKAM_T9X_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_motor_soft_abort()";

	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	char command[5];
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_motor_get_pointers( motor,
					&linkam_t9x_motor, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( linkam_t9x_motor->axis_name == 'Z' ) {
		strlcpy( command, "MSZ", sizeof(command) );
	} else {
		strlcpy( command, "MSX", sizeof(command) );
	}

	mx_status = mxi_linkam_t9x_command( linkam_t9x, command,
					NULL, 0, MXD_LINKAM_T9X_MOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_motor_immediate_abort()";

	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_motor_get_pointers( motor,
					&linkam_t9x_motor, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_linkam_t9x_command( linkam_t9x, "MSA",
					NULL, 0, MXD_LINKAM_T9X_MOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_motor_raw_home_command()";

	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	char command[10];
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_motor_get_pointers( motor,
					&linkam_t9x_motor, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->raw_home_command == 0 ) {
		strlcpy( command, "MF1", sizeof(command) );
	} else {
		strlcpy( command, "MF2", sizeof(command) );
	}

	mx_status = mxi_linkam_t9x_command( linkam_t9x, command,
					NULL, 0, MXD_LINKAM_T9X_MOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_motor_get_parameter()";

	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_motor_get_pointers( motor,
					&linkam_t9x_motor, &linkam_t9x, fname );

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
mxd_linkam_t9x_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_linkam_t9x_motor_set_parameter()";

	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	char command[80];
	long t9x_speed;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_motor_get_pointers( motor,
					&linkam_t9x_motor, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* MX uses um/sec for speed.  We must multiply this by
		 * 10 to get the controller speed.
		 */

		t9x_speed = mx_round( 10.0 * motor->raw_speed );

		if ( linkam_t9x_motor->axis_name == 'Z' ) {
			snprintf( command, sizeof(command),
				"MVZ%ld", t9x_speed );
		} else {
			snprintf( command, sizeof(command),
				"MVX%ld", t9x_speed );
		}

		mx_status = mxi_linkam_t9x_command( linkam_t9x,
						command, NULL, 0,
						MXD_LINKAM_T9X_MOTOR_DEBUG );
		break;
	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_linkam_t9x_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] =
		"mxd_linkam_t9x_motor_get_extended_status()";

	MX_LINKAM_T9X_MOTOR *linkam_t9x_motor = NULL;
	MX_LINKAM_T9X *linkam_t9x = NULL;
	char response[80];
	char *string_ptr, *comma_ptr;
	int num_items;
	double controller_position;
	unsigned char ms;
	mx_status_type mx_status;

	mx_status = mxd_linkam_t9x_motor_get_pointers( motor,
					&linkam_t9x_motor, &linkam_t9x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_linkam_t9x_command( linkam_t9x, "Mp",
					response, sizeof(response),
					MXD_LINKAM_T9X_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Valid responses starts with the character 'M' followed
	 * by the value of the motor status byte.
	 */

	if ( response[0] != 'M' ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The first character in the response '%s' to an 'Mp' command "
		"for motor '%s' is not 'M'.  Instead, it is %c (%#x).",
				response, motor->record->name,
				response[0], response[0] );
	}

	/* Figure out whether or not this motor is moving by looking
	 * at the motor status byte.
	 */

	ms = response[1];

#if MXD_LINKAM_T9X_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: motor status = %#x", fname, ms));
#endif

	motor->status = 0;

	switch( linkam_t9x_motor->axis_name ) {
	case 'X':
		if ( (ms & 0x1) == 0 ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}
		break;
	case 'Y':
		if ( (ms & 0x2) == 0 ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}
		break;
	case 'Z':
		if ( (ms & 0x4) == 0 ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Invalid axis name '%c' for motor '%s'.",
			linkam_t9x_motor->axis_name,
			motor->record->name );
		break;
	}

	/* Get the motor position. */

	string_ptr = response + 2;

	switch( linkam_t9x_motor->axis_name ) {
	case 'X':
		/* Do nothing. */
		break;

	case 'Z':
		comma_ptr = strchr( string_ptr, ',' );

		if ( comma_ptr == NULL ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Unable to find the motor position in the "
			"response '%s' to an 'Mp' command for motor '%s'.",
				response, motor->record->name );
		}

		string_ptr = comma_ptr + 1;

		/* Do _not_ insert a 'break' statement here.  We 
		 * _intentionally_ fall through to the following
		 * case 'Y', since for 'Z' we want to skip over
		 * _two_ commas.
		 */
	case 'Y':
		comma_ptr = strchr( string_ptr, ',' );

		if ( comma_ptr == NULL ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Unable to find the motor position in the "
			"response '%s' to an 'Mp' command for motor '%s'.",
				response, motor->record->name );
		}

		string_ptr = comma_ptr + 1;
		break;
	}

	num_items = sscanf( string_ptr, "%lg", &controller_position );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Could not find the motor position in the "
		"response '%s' to an 'Mp' command for motor '%s'.",
			response, motor->record->name );
	}

	if( linkam_t9x_motor->axis_name == 'Z' ) {
		motor->raw_position.analog = 0.1 * controller_position;
	} else {
		motor->raw_position.analog = controller_position;
	}

	return mx_status;
}

/*-----------*/


