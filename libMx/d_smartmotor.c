/*
 * Name:    d_smartmotor.c 
 *
 * Purpose: MX motor driver to control Animatics SmartMotor motor controllers.
 *
 * Author:  William Lavender
 *
 * WARNING: This driver currently uses SmartMotor variable 'z' as a scratch
 *          pad for reading values that cannot be directly displayed and then
 *          displaying them with the 'Rz' command.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SMARTMOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "d_smartmotor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_smartmotor_record_function_list = {
	NULL,
	mxd_smartmotor_create_record_structures,
	mxd_smartmotor_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_smartmotor_open,
	NULL,
	NULL,
	mxd_smartmotor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_smartmotor_motor_function_list = {
	NULL,
	mxd_smartmotor_move_absolute,
	mxd_smartmotor_get_position,
	mxd_smartmotor_set_position,
	mxd_smartmotor_soft_abort,
	mxd_smartmotor_immediate_abort,
	NULL,
	NULL,
	mxd_smartmotor_find_home_position,
	mxd_smartmotor_constant_velocity_move,
	mxd_smartmotor_get_parameter,
	mxd_smartmotor_set_parameter,
	NULL,
	mxd_smartmotor_get_status,
	mxd_smartmotor_get_extended_status
};

/* SmartMotor SMARTMOTOR motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_smartmotor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SMARTMOTOR_STANDARD_FIELDS
};

long mxd_smartmotor_num_record_fields
		= sizeof( mxd_smartmotor_record_field_defaults )
			/ sizeof( mxd_smartmotor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_rfield_def_ptr
			= &mxd_smartmotor_record_field_defaults[0];

/* Private functions for the use of the driver. */

static mx_status_type
mxd_smartmotor_get_pointers( MX_MOTOR *motor,
			MX_SMARTMOTOR **smartmotor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_smartmotor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( smartmotor == (MX_SMARTMOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SMARTMOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*smartmotor = (MX_SMARTMOTOR *) (motor->record->record_type_struct);

	if ( *smartmotor == (MX_SMARTMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SMARTMOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_smartmotor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_smartmotor_create_record_structures()";

	MX_MOTOR *motor;
	MX_SMARTMOTOR *smartmotor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for an MX_MOTOR structure for record '%s'.",
			record->name );
	}

	smartmotor = (MX_SMARTMOTOR *) malloc( sizeof(MX_SMARTMOTOR) );

	if ( smartmotor == (MX_SMARTMOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for an MX_SMARTMOTOR structure for record '%s'.",
			record->name );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = smartmotor;
	record->class_specific_function_list
				= &mxd_smartmotor_motor_function_list;

	motor->record = record;
	smartmotor->record = record;

	/* A SmartMotor is treated as a stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The SMARTMOTOR reports accelerations in steps/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	motor->home_search = 0;

	smartmotor->home_search_succeeded = FALSE;
	smartmotor->historical_left_limit = FALSE;
	smartmotor->historical_right_limit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_smartmotor_open()";

	MX_MOTOR *motor;
	MX_SMARTMOTOR *smartmotor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the velocity and acceleration scale factors which depend
	 * on the type of SmartMotor being used.  The magic numbers of
	 * 32212.578, 7.9166433, and 2000 come from the SmartMotor manual.
	 */

	smartmotor->velocity_scale_factor = 32212.578 / 2000.0;

	smartmotor->acceleration_scale_factor = 7.9166433 / 2000.0;

	/* Verify the line terminators. */

	mx_status = mx_rs232_verify_configuration( smartmotor->rs232_record,
			(long) MXF_232_DONT_CARE, 8, 'N', 1, 'N',
			0x0d, 0x0d );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_smartmotor_resynchronize( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_smartmotor_resynchronize()";

	MX_MOTOR *motor;
	MX_SMARTMOTOR *smartmotor;
	char command[20];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( smartmotor->smartmotor_flags & MXF_SMARTMOTOR_ECHO_ON ) {
		strcpy( command, "ECHO" );
	} else {
		strcpy( command, "ECHO_OFF" );
	}

	mx_status = mx_rs232_putline( smartmotor->rs232_record, command,
						NULL, MXD_SMARTMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any unread RS-232 input. */

	mx_status = mx_rs232_discard_unread_input( smartmotor->rs232_record,
							MXD_SMARTMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the controller is there by asking for its
	 * firmware version.
	 */

	mx_status = mxd_smartmotor_command( smartmotor, "RSP",
						response, sizeof(response),
						MXD_SMARTMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld/%s", &(smartmotor->sample_rate),
					smartmotor->firmware_version );

	if ( num_items != 2 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Could not find the sample rate and firmware version in the "
	"response to an 'RSP' command for SmartMotor '%s'.  Response = '%s'",
			record->name, response );
	}

	/* Reset the motor status flags. */

	mx_status = mxd_smartmotor_command( smartmotor, "ZS",
					NULL, 0, MXD_SMARTMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the stored value of the motor speed. */

	mx_status = mx_motor_get_speed( record, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_move_absolute()";

	MX_SMARTMOTOR *smartmotor;
	char command[20];
	long raw_destination;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->home_search = 0;

	smartmotor->home_search_succeeded = FALSE;
	smartmotor->historical_left_limit = FALSE;
	smartmotor->historical_right_limit = FALSE;

	raw_destination = motor->raw_destination.stepper;

	/* Change to position mode, set the destination, and start the move. */

	sprintf( command, "ZS MP P=%ld G", raw_destination );

	mx_status = mxd_smartmotor_command( smartmotor, command,
					NULL, 0, MXD_SMARTMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_get_position()";

	MX_SMARTMOTOR *smartmotor;
	char response[30];
	int num_items;
	long raw_position;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_smartmotor_command( smartmotor, "RP",
					response, sizeof response,
					MXD_SMARTMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &raw_position );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Motor '%s' returned an unparseable position value '%s'.",
			motor->record->name, response );
	}

	motor->raw_position.stepper = raw_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_set_position()";

	MX_SMARTMOTOR *smartmotor;
	char command[40];
	long raw_position;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_position = motor->raw_set_position.stepper;

	sprintf( command, "O=%ld", raw_position );

	mx_status = mxd_smartmotor_command( smartmotor, command,
					NULL, 0, MXD_SMARTMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_soft_abort()";

	MX_SMARTMOTOR *smartmotor;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_smartmotor_command( smartmotor, "X",
					NULL, 0, MXD_SMARTMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_immediate_abort()";

	MX_SMARTMOTOR *smartmotor;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_smartmotor_command( smartmotor, "S",
					NULL, 0, MXD_SMARTMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_find_home_position()";

	MX_SMARTMOTOR *smartmotor;
	char command[20];
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->home_search == 0 ) {
		motor->home_search = 1;
	}

	/* Animatics smartmotors do not have a function specifically for
	 * home searches.  Thus, we instead command the motor to move to
	 * the software limit in the requested home direction and wait
	 * for the motor to trip a limit switch.
	 */

	motor->constant_velocity_move = motor->home_search;

	flags = smartmotor->smartmotor_flags;

	if ( flags & MXF_SMARTMOTOR_ENABLE_LIMIT_DURING_HOME_SEARCH ) {
		if ( motor->constant_velocity_move > 0 ) {
			strcpy( command, "UCP" );
		} else {
			strcpy( command, "UDM" );
		}
		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mxd_smartmotor_constant_velocity_move( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_constant_velocity_move()";

	MX_SMARTMOTOR *smartmotor;
	char command[20];
	double speed, velocity_factor;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	smartmotor->home_search_succeeded = FALSE;
	smartmotor->historical_left_limit = FALSE;
	smartmotor->historical_right_limit = FALSE;

	mx_status = mx_motor_get_speed( motor->record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Change to velocity mode, set the velocity, and start the move. */

	velocity_factor = motor->raw_speed * smartmotor->velocity_scale_factor;

	if ( motor->constant_velocity_move >= 0 ) {
		sprintf( command, "MV V=%ld G",
				mx_round( velocity_factor ) );
	} else {
		sprintf( command, "MV V=-%ld G",
				mx_round( velocity_factor ) );
	}

	mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_get_parameter()";

	MX_SMARTMOTOR *smartmotor;
	char response[80];
	int num_items;
	double velocity_factor, acceleration_factor;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mxd_smartmotor_command( smartmotor, "z=V Rz",
					response, sizeof(response),
					MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &velocity_factor );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to an 'RV' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->raw_speed =
		    fabs( velocity_factor ) / smartmotor->velocity_scale_factor;

		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mxd_smartmotor_command( smartmotor, "RA",
					response, sizeof(response),
					MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &acceleration_factor );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to an 'RA' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->raw_acceleration_parameters[0] =
	fabs( acceleration_factor ) / smartmotor->acceleration_scale_factor;

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		motor->raw_maximum_speed = 
				5000000.0 / smartmotor->velocity_scale_factor;

		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mxd_smartmotor_command( smartmotor, "RKP",
					response, sizeof(response),
					MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
					&(motor->proportional_gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to an 'RKP' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mxd_smartmotor_command( smartmotor, "RKI",
					response, sizeof(response),
					MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
					&(motor->integral_gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to an 'RKI' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mxd_smartmotor_command( smartmotor, "RKD",
					response, sizeof(response),
					MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
					&(motor->derivative_gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to an 'RKD' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		mx_status = mxd_smartmotor_command( smartmotor, "RKV",
					response, sizeof(response),
					MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
					&(motor->velocity_feedforward_gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to an 'RKV' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		mx_status = mxd_smartmotor_command( smartmotor, "RKA",
					response, sizeof(response),
					MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
				&(motor->acceleration_feedforward_gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to an 'RKA' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		mx_status = mxd_smartmotor_command( smartmotor, "RKL",
					response, sizeof(response),
					MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
					&(motor->integral_limit) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to an 'RKL' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;

	case MXLV_MTR_EXTRA_GAIN:
		/* For this controller, the 'extra' gain is the
		 * 'sample rate'.
		 */

		mx_status = mxd_smartmotor_command( smartmotor, "RKS",
					response, sizeof(response),
					MXD_SMARTMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
					&(motor->extra_gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to an 'RKS' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_set_parameter()";

	MX_SMARTMOTOR *smartmotor;
	char command[40];
	double velocity_factor, acceleration_factor;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		velocity_factor = motor->raw_speed
					* smartmotor->velocity_scale_factor;

		sprintf( command, "V=%ld", mx_round( velocity_factor ) );

		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		acceleration_factor = motor->raw_acceleration_parameters[0]
					* smartmotor->acceleration_scale_factor;

		sprintf( command, "A=%ld", mx_round( acceleration_factor ) );

		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		sprintf( command, "KP=%ld F",
			mx_round( motor->proportional_gain ) );

		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		sprintf( command, "KI=%ld F",
			mx_round( motor->proportional_gain ) );

		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		sprintf( command, "KD=%ld F",
			mx_round( motor->derivative_gain ) );

		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		sprintf( command, "KV=%ld F",
			mx_round( motor->velocity_feedforward_gain ) );

		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		sprintf( command, "KA=%ld F",
			mx_round( motor->acceleration_feedforward_gain ) );

		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		sprintf( command, "KL=%ld F",
			mx_round( motor->integral_limit ) );

		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

		break;

	case MXLV_MTR_EXTRA_GAIN:
		/* For this controller, the 'extra' gain is the
		 * 'sample rate'.
		 */

		sprintf( command, "KS=%ld F",
			mx_round( motor->extra_gain ) );

		mx_status = mxd_smartmotor_command( smartmotor, command,
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}
	return mx_status;
}

static mx_status_type
mxd_smartmotor_parse_status_word( MX_MOTOR *motor,
				MX_SMARTMOTOR *smartmotor,
				unsigned long status_word )
{
	motor->status = 0;

	/* Bit 0: Trajectory in progress. */

	if ( status_word & 0x1 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* Bit 1: Historical right limit. */

	if ( status_word & 0x2 ) {
		smartmotor->historical_right_limit = TRUE;
	}

	/* Bit 2: Historical left limit. */

	if ( status_word & 0x4 ) {
		smartmotor->historical_left_limit = TRUE;
	}

	/* Bit 3: Index report available (ignored). */

	/* Bit 4: Wraparound occurred.
	 *
	 *        We don't currently have a bit specifically for this, so we
	 *        generate a warning message and then set the generic error
	 *        bit.
	 */

	if ( status_word & 0x10 ) {
		motor->status |= MXSF_MTR_ERROR;

		mx_warning( "Wraparound error: "
	"The position of motor '%s' has gone outside the maximum range "
	"of values that the controller can handle, so the position value "
	"has wrapped and is wrong.", motor->record->name );
	}

	/* Bit 5: Excessive position error occurred. */

	if ( status_word & 0x20 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 6: Excessive temperature. */

	if ( status_word & 0x40 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;

		mx_warning("Excessive temperature fault for motor '%s'.",
				motor->record->name );
	}

	/* Bit 7: Motor is OFF. */

	if ( status_word & 0x80 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/* Bit 8: Hardware index input level.
	 *
	 *        FIXME: I am not 100% sure what this means - WML
	 */

	/* Bit 9: Right limit asserted. */

	if ( status_word & 0x200 ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* Bit 10: Left limit asserted. */

	if ( status_word & 0x400 ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 11: User math overflow occurred. */

	if ( status_word & 0x800 ) {
		motor->status |= MXSF_MTR_ERROR;

		mx_warning( "User math overflow occurred for motor '%s'.",
				motor->record->name );
	}

	/* Bit 12: User array index range error occurred. */

	if ( status_word & 0x1000 ) {
		motor->status |= MXSF_MTR_ERROR;

		mx_warning( "User array index range error occurred "
			"for motor '%s'.", motor->record->name );
	}

	/* Bit 13: Syntax error occurred. */

	if ( status_word & 0x2000 ) {
		motor->status |= MXSF_MTR_ERROR;

		mx_warning( "Syntax error in command occurred for motor '%s'.",
				motor->record->name );
	}

	/* Bit 14: Over current state occurred. */

	if ( status_word & 0x4000 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;

		mx_warning( "Over current state occurred for motor '%s'.",
				motor->record->name );
	}

	/* Bit 15: Program checksum/EEPROM failure. */

	if ( status_word & 0x8000 ) {
		motor->status |= MXSF_MTR_ERROR;

		mx_warning( "Program checksum/EEPROM failure for motor '%s'.",
				motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_smartmotor_check_home_search_status( MX_MOTOR *motor,
					MX_SMARTMOTOR *smartmotor )
{
	unsigned long flags, limit_bits;
	mx_status_type mx_status;

	MX_DEBUG( 2,("motor '%s': home_search = %ld",
		motor->record->name, motor->home_search));

	MX_DEBUG( 2,("motor '%s': historical_left_limit = %d",
		motor->record->name, (int) smartmotor->historical_left_limit));

	MX_DEBUG( 2,("motor '%s': historical_right_limit = %d",
		motor->record->name, (int) smartmotor->historical_right_limit));

	/* Make various tests to see if the home search has succeeded. */

	smartmotor->home_search_succeeded = FALSE;

	limit_bits = MXSF_MTR_POSITIVE_LIMIT_HIT
			| MXSF_MTR_NEGATIVE_LIMIT_HIT;

	if ( motor->status & limit_bits ) {
		smartmotor->home_search_succeeded = TRUE;
	}
	if ( smartmotor->historical_left_limit ) {
		smartmotor->home_search_succeeded = TRUE;
	}
	if ( smartmotor->historical_right_limit ) {
		smartmotor->home_search_succeeded = TRUE;
	}

	/* However, if the motor is still moving, the home search
	 * cannot be regarded as having succeeded.
	 */

	if ( motor->status & MXSF_MTR_IS_BUSY ) {
		smartmotor->home_search_succeeded = FALSE;
	}

	if ( smartmotor->home_search_succeeded ) {

		motor->home_search = FALSE;

		flags = smartmotor->smartmotor_flags;

		if ( flags & MXF_SMARTMOTOR_ENABLE_LIMIT_DURING_HOME_SEARCH ) {

			/* If limit switches were turned on only for the
			 * duration of the home search, then turn them
			 * off again.
			 */

			mx_status = mxd_smartmotor_command( smartmotor, "UCI",
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_smartmotor_command( smartmotor, "UDI",
						NULL, 0, MXD_SMARTMOTOR_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_get_status()";

	MX_SMARTMOTOR *smartmotor;
	char response[80];
	int num_items;
	unsigned long status_word;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the status request command. */

	mx_status = mxd_smartmotor_command( smartmotor, "RW",
						response, sizeof(response),
						MXD_SMARTMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &status_word );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Did not see the status word in the response to a 'RW' command "
	"sent to SmartMotor '%s'.  Response = '%s'.",
			motor->record->name, response );
	}

	mx_status = mxd_smartmotor_parse_status_word( motor,
						smartmotor, status_word );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->home_search ) {
		mx_status = mxd_smartmotor_check_home_search_status(
							motor, smartmotor );
	}

	if ( smartmotor->home_search_succeeded ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	} else {
		motor->status &= ( ~ MXSF_MTR_HOME_SEARCH_SUCCEEDED );
	}

	MX_DEBUG( 2,("%s: home_search_succeeded flag bit set = %lu",
		fname, motor->status & MXSF_MTR_HOME_SEARCH_SUCCEEDED ));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_smartmotor_get_extended_status()";

	MX_SMARTMOTOR *smartmotor;
	char response[80];
	int num_items;
	long raw_position;
	unsigned long status_word;
	mx_status_type mx_status;

	mx_status = mxd_smartmotor_get_pointers( motor, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the status request command. */

	mx_status = mxd_smartmotor_command( smartmotor, "RPW",
						response, sizeof(response),
						MXD_SMARTMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld,%lu", &raw_position, &status_word );

	if ( num_items != 2 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Did not see the position and status word in the response to "
	"a 'RPW' command sent to SmartMotor '%s'.  Response = '%s'.",
			motor->record->name, response );
	}

	motor->raw_position.stepper = raw_position;

	mx_status = mxd_smartmotor_parse_status_word( motor,
						smartmotor, status_word );

	if ( motor->home_search ) {
		mx_status = mxd_smartmotor_check_home_search_status(
							motor, smartmotor );
	}

	if ( smartmotor->home_search_succeeded ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	} else {
		motor->status &= ( ~ MXSF_MTR_HOME_SEARCH_SUCCEEDED );
	}

	MX_DEBUG( 2,("%s: home_search_succeeded flag bit set = %lu",
		fname, motor->status & MXSF_MTR_HOME_SEARCH_SUCCEEDED ));

	return mx_status;
}

/* === Extra functions for the use of this driver. === */

MX_EXPORT mx_status_type
mxd_smartmotor_command( MX_SMARTMOTOR *smartmotor, char *command,
			char *response, size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_smartmotor_command()";

	char address_prefix;
	mx_status_type mx_status;

	if ( smartmotor == (MX_SMARTMOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SMARTMOTOR pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'command' pointer passed was NULL." );
	}

	if ( debug_flag & MXD_SMARTMOTOR_DEBUG ) {
		MX_DEBUG(-2, ("%s: sending '%s' to motor '%s'.",
				fname, command, smartmotor->record->name));
	}

	/* Send the motor address prefix. */

	address_prefix = (char) ( 128 + smartmotor->motor_address );

	mx_status = mx_rs232_putchar( smartmotor->rs232_record,
						address_prefix, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the command. */

	mx_status = mx_rs232_putline( smartmotor->rs232_record,
					command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( smartmotor->smartmotor_flags & MXF_SMARTMOTOR_ECHO_ON ) {
		char echoed_command[100];

		/* If echoing is on, discard the echoed command line. */

		mx_status = mx_rs232_getline( smartmotor->rs232_record,
				echoed_command, sizeof(echoed_command),
				NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*** Get the response from the SmartMotor. ***/

	if ( response != NULL ) {

		mx_status = mx_rs232_getline( smartmotor->rs232_record,
				response, response_buffer_length, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#if 0
		{
			int i, response_length;

			response_length = strlen( response );

			for ( i = 0; i < response_length; i++ ) {
				MX_DEBUG(-2,("%s: response[%d] = %#x '%c'",
				fname, i, response[i], response[i]));
			}
		}
#endif
		if ( debug_flag & MXD_SMARTMOTOR_DEBUG ) {
			MX_DEBUG(-2, ("%s: received '%s' from motor '%s'",
				fname, response, smartmotor->record->name ));
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_smartmotor_check_port_name( MX_RECORD *record, char *port_name )
{
	static const char fname[] = "mxd_smartmotor_check_port_name()";

	char junk[30];
	size_t length;
	int i, channel, num_items;

	/* Force the port name to upper case. */

	length = strlen(port_name);

	for ( i = 0; i < length; i++ ) {
		if ( islower( (int)(port_name[i]) ) ) {
			port_name[i] = toupper( (int)(port_name[i]) );
		}
	}

	/* See if the port name has an allowed value. */

	if ( port_name[0] == 'U' ) {
		/* This is an onboard I/O port. */

		if ( ( length != 2 )
		  || ( port_name[1] < 'A' )
		  || ( port_name[1] > 'J' )
		  || ( port_name[1] == 'H' ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The requested port name '%s' for SmartMotor I/O port '%s' is not a "
	"valid onboard port name.  The allowed values are from 'UA' to 'UG', "
	"'UI', and 'UJ'.",
				record->name, port_name );
		}

		if ( record->mx_type == MXT_AOU_SMARTMOTOR ) {
			return mx_error( MXE_UNSUPPORTED, fname,
	"SmartMotor I/O port '%s' is configured to use onboard I/O port '%s'.  "
	"However, the SmartMotor does not support using onboard I/O ports as "
	"analog output ports.  You must use an Anilink I/O port "
	"for this instead.", record->name, port_name );
		}

		return MX_SUCCESSFUL_RESULT;
	} else
	if ( ( port_name[0] >= 'A' ) && ( port_name[0] <= 'H' ) ) {
		/* This is an Anilink I/O port. */

		/* Analog out port names should be only one character long. */

		if ( record->mx_type == MXT_AOU_SMARTMOTOR ) {
			if ( port_name[1] == '\0' ) {
				return MX_SUCCESSFUL_RESULT;
			} else {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The port name '%s' for SmartMotor analog output port '%s' "
		"should be only one character long.",
					port_name, record->name );
			}
		}

		num_items = sscanf( &port_name[1], "%d%20s", &channel, junk );

		/* There should _not_ be anything after the channel number. */

		if ( num_items > 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized junk '%s' after channel number %d "
			"in port name '%s' for SmartMotor I/O port '%s'.",
    				junk, channel, port_name, record->name );
		} else
		if ( num_items < 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"No channel number was found in the port name '%s' "
			"for SmartMotor I/O port '%s.",
				port_name, record->name );
		}

		if ( record->mx_type == MXT_AIN_SMARTMOTOR ) {
			if ( ( channel >= 1 ) && ( channel <= 4 ) ) {
				return MX_SUCCESSFUL_RESULT;
			} else {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The channel number %d for SmartMotor analog input port '%s' "
		"is outside the allowed range of 1-4.", channel, record->name );
			}
		} else {
			/* digital I/O */

			if ( ( channel >= 0 ) && ( channel <= 63 ) ) {
				return MX_SUCCESSFUL_RESULT;
			} else {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The channel number %d for SmartMotor digital I/O port '%s' "
		"is outside the allowed range of 0-63.", channel, record->name);
			}
		}
	} else
	if ( strcmp( port_name, "TEMP" ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The SmartMotor port name '%s' for SmartMotor I/O record '%s' is not "
	"valid since a valid port name starts with either the letter 'U' "
	"or a letter in the range from 'A' to 'H' or is the string 'TEMP'.",
			port_name, record->name );
	}
}

