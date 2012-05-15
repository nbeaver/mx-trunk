/*
 * Name:    d_u500.c
 *
 * Purpose: MX driver for Aerotech Unidex 500 controlled motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2005, 2008-2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define U500_DEBUG	FALSE

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
#include "i_u500.h"
#include "d_u500.h"

/* Aerotech includes */

#include "Aerotype.h"
#include "Build.h"
#include "Quick.h"
#include "Wapi.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_u500_record_function_list = {
	NULL,
	mxd_u500_create_record_structures,
	mxd_u500_finish_record_initialization,
	NULL,
	mxd_u500_print_structure,
	mxd_u500_open,
	NULL,
	NULL,
	mxd_u500_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_u500_motor_function_list = {
	NULL,
	mxd_u500_move_absolute,
	mxd_u500_get_position,
	mxd_u500_set_position,
	mxd_u500_soft_abort,
	NULL,
	NULL,
	NULL,
	mxd_u500_find_home_position,
	mxd_u500_constant_velocity_move,
	mxd_u500_get_parameter,
	mxd_u500_set_parameter,
	NULL,
	mxd_u500_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_u500_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_U500_STANDARD_FIELDS
};

long mxd_u500_num_record_fields
		= sizeof( mxd_u500_record_field_defaults )
			/ sizeof( mxd_u500_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_u500_rfield_def_ptr
			= &mxd_u500_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_u500_get_pointers( MX_MOTOR *motor,
			MX_U500_MOTOR **u500_motor,
			MX_U500 **u500,
			const char *calling_fname )
{
	static const char fname[] = "mxd_u500_get_pointers()";

	MX_U500_MOTOR *u500_motor_ptr;
	MX_RECORD *u500_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	u500_motor_ptr = (MX_U500_MOTOR *) (motor->record->record_type_struct);

	if ( u500_motor_ptr == (MX_U500_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_U500_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( u500_motor != (MX_U500_MOTOR **) NULL ) {
		*u500_motor = u500_motor_ptr;
	}

	if ( u500 != (MX_U500 **) NULL ) {
		u500_record = u500_motor_ptr->u500_record;

		if ( u500_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The u500_record pointer for record '%s' is NULL.",
				motor->record->name );
		}

		*u500 = (MX_U500 *) u500_record->record_type_struct;

		if ( *u500 == (MX_U500 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_U500 pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_u500_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_u500_create_record_structures()";

	MX_MOTOR *motor;
	MX_U500_MOTOR *u500_motor = NULL;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	u500_motor = (MX_U500_MOTOR *) malloc( sizeof(MX_U500_MOTOR) );

	if ( u500_motor == (MX_U500_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_U500_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = u500_motor;
	record->class_specific_function_list = &mxd_u500_motor_function_list;

	motor->record = record;
	u500_motor->record = record;

	/* A U500 motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* We express accelerations in counts/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_u500_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_u500_finish_record_initialization()";

	MX_U500_MOTOR *u500_motor = NULL;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	u500_motor = (MX_U500_MOTOR *) record->record_type_struct;

	if ( islower(u500_motor->axis_name) ) {
		u500_motor->axis_name = toupper(u500_motor->axis_name);
	}

	switch( u500_motor->axis_name ) {
	case 'X':
		u500_motor->motor_number = 1;
		break;
	case 'Y':
		u500_motor->motor_number = 2;
		break;
	case 'Z':
		u500_motor->motor_number = 3;
		break;
	case 'U':
		u500_motor->motor_number = 4;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal U500 axis name '%c' for record '%s'.  "
			"The allowed names are X, Y, Z, and U.",
				u500_motor->axis_name, record->name );
		break;
	}

	u500_motor->last_start_time = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_u500_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_u500_print_structure()";

	MX_MOTOR *motor;
	MX_U500_MOTOR *u500_motor = NULL;
	double position, backlash;
	double negative_limit, positive_limit, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type      = U500 motor.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  controller name = %s\n",
						u500_motor->u500_record->name);
	fprintf(file, "  axis name       = %c\n", u500_motor->axis_name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
				record->name );
	}
	
	fprintf(file, "  position        = %g steps (%g %s)\n",
			motor->raw_position.analog, position, motor->units);
	fprintf(file, "  scale           = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset          = %g %s.\n",
			motor->offset, motor->units);
	
	backlash = motor->scale
			* (double) (motor->raw_backlash_correction.analog);
	
	fprintf(file, "  backlash        = %g steps (%g %s)\n",
			motor->raw_backlash_correction.analog,
			backlash, motor->units);
	
	negative_limit = motor->offset
		+ motor->scale * (double)(motor->raw_negative_limit.analog);
	
	fprintf(file, "  negative limit  = %g steps (%g %s)\n",
			motor->raw_negative_limit.analog,
			negative_limit, motor->units);

	positive_limit = motor->offset
		+ motor->scale * (double)(motor->raw_positive_limit.analog);

	fprintf(file, "  positive limit  = %g steps (%g %s)\n",
			motor->raw_positive_limit.analog,
			positive_limit, motor->units);

	move_deadband = motor->scale * (double)motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband   = %g steps (%g %s)\n",
			motor->raw_move_deadband.analog,
			move_deadband, motor->units );

	mx_status = mx_motor_get_status( record, NULL );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to get motion status for motor '%s'",
				record->name );
	}

	fprintf(file, "  MX motor status = %#lx\n\n", motor->status);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_u500_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_u500_open()";

	MX_MOTOR *motor;
	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	int board_number;
	char axis_name;
	SHORT motor_number, return_code;
	LONG plane;
	char command[200];
	int feedrate_parameter_number;
	double parameter_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	board_number = u500_motor->board_number;
	axis_name = u500_motor->axis_name;
	motor_number = u500_motor->motor_number;

	/* Enable the axis */

	sprintf( command, "EN %c", axis_name );

	mx_status = mxi_u500_command( u500, board_number, command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We need to know which plane the axis is allocated to in order to
	 * ask for the default speed for the axis.
	 */

	plane = WAPIGetPlane( motor_number );

	/* Select the board. */

	mx_status = mxi_u500_select_board( u500, board_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Open the parameter file for this board so that we can initialize
	 * our copy of the parameters from it.
	 */

	return_code = WAPIAerOpenParam(
			u500->parameter_filename[board_number-1] );

	if ( return_code != 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to open parameter file '%s' for motor '%s'.",
			u500->parameter_filename[board_number-1] );	
	}

	/* Get the current position. */

	mx_status = mxd_u500_get_position( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the motor speed. */

	if ( u500_motor->default_speed > 0.0 ) {

		/* If the default speed is set to a positive value,
		 * use this as the initial motor speed.
		 */

		motor->speed = u500_motor->default_speed;

		motor->raw_speed = mx_divide_safely(motor->speed, motor->scale);
	} else {
		/* Otherwise, get the initial motor speed from the
		 * Aerotech parameter file.
		 */

		mx_status = mxi_u500_read_parameter( u500,
					18 * plane + motor_number + 4,
					&parameter_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* The factor of 1.0e3 converts from steps/msec to steps/sec. */

		motor->raw_speed = 1.0e3 * fabs( parameter_value );

		motor->speed = motor->raw_speed * motor->scale;
	}

#if U500_DEBUG
	MX_DEBUG(-2,("%s: raw_speed = %g, speed = %g",
		fname, motor->raw_speed, motor->speed));
#endif

	/* Get the default acceleration. */

	mx_status = mxi_u500_read_parameter( u500, 100 * motor_number + 16,
						&parameter_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The factor of 1.0e6 converts from steps/msec**2 to steps/sec**2. */

	motor->raw_acceleration_parameters[0] = 1.0e6 * fabs( parameter_value );
	motor->raw_acceleration_parameters[1] = 0;
	motor->raw_acceleration_parameters[2] = 0;
	motor->raw_acceleration_parameters[3] = 0;

#if U500_DEBUG
	MX_DEBUG(-2,("%s: accleration = %g steps/sec**2",
		fname, motor->raw_acceleration_parameters[0]));
#endif

	/* Get the servo loop gains. */

	/* Proportional gain */

	mx_status = mxi_u500_read_parameter( u500, 100 * motor_number + 25,
						&(motor->proportional_gain) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Integral gain */

	mx_status = mxi_u500_read_parameter( u500, 100 * motor_number + 26,
						&(motor->integral_gain) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Derivative gain */

	mx_status = mxi_u500_read_parameter( u500, 100 * motor_number + 27,
						&(motor->derivative_gain) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Velocity feedforward gain */

	mx_status = mxi_u500_read_parameter( u500, 100 * motor_number + 28,
					&(motor->velocity_feedforward_gain) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Acceleration feedforward gain */

	mx_status = mxi_u500_read_parameter( u500, 100 * motor_number + 29,
				    &(motor->acceleration_feedforward_gain) );

	/* At this point, nominally we would close the parameter file,
	 * but there does not seem to be a function to do this, so we don't.
	 */

#if U500_DEBUG
	MX_DEBUG(-2,("%s: motor '%s' servo loop gains are:",
			fname, record->name));

	MX_DEBUG(-2,("%s: Kp = %g, Ki = %g, Kd = %g, Kvff = %g, Kaff = %g",
			fname, motor->proportional_gain,
			motor->integral_gain,
			motor->derivative_gain,
			motor->velocity_feedforward_gain,
			motor->acceleration_feedforward_gain));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_u500_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_u500_resynchronize()";

	MX_MOTOR *motor;
	MX_U500 *u500 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_u500_get_pointers( motor, NULL, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_resynchronize_record( u500->record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_u500_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_u500_move_absolute()";

	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	char command[200];
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_destination.analog;

	sprintf( command, "IN %c %ld %cF %ld",
		u500_motor->axis_name, motor_steps,
		u500_motor->axis_name, mx_round( motor->raw_speed ) );

	u500_motor->last_start_time = mx_high_resolution_time_as_double();

	mx_status = mxi_u500_command( u500, u500_motor->board_number, command );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_u500_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_u500_get_position()";

	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	AERERR_CODE wapi_status;
	mx_status_type mx_status;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_u500_select_board( u500, u500_motor->board_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	wapi_status = WAPIAerCheckStatus();

	if ( wapi_status != 0 ) {
		return mxi_u500_error( wapi_status, fname,
			"Attempt to check the U500 status for "
			"controller '%s' by motor '%s' failed.",
				u500->record->name, motor->record->name );
	}

	motor->raw_position.analog = WAPIAerReadPosition(
			(SHORT) (u500_motor->motor_number + 7) );

#if U500_DEBUG
	MX_DEBUG(-2,
	("%s: motor '%s', WAPIAerReadPosition(%d) = %g",
		fname, motor->record->name,
		(int)(u500_motor->motor_number + 7),
		motor->raw_position.analog));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_u500_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_u500_set_position()";

	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	char command[200];
	int board_number;
	char axis_name;
	mx_status_type mx_status;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	board_number = u500_motor->board_number;
	axis_name = u500_motor->axis_name;

	/* Make sure the software position matches the hardware position. */

	sprintf( command, "SOFTWARE POSITION %c", u500_motor->axis_name );

	mx_status = mxi_u500_command( u500, board_number, command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now we can redefine the current position. */

	sprintf( command, "SOFTWARE HOME %c %g",
		axis_name, motor->raw_set_position.analog );

	mx_status = mxi_u500_command( u500, board_number, command );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_u500_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_u500_soft_abort()";

	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_u500_command( u500, u500_motor->board_number, "AB" );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_u500_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_u500_find_home_position()";

	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	char command[200];
	mx_status_type mx_status;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "HO %c", u500_motor->axis_name );

	mx_status = mxi_u500_command( u500, u500_motor->board_number, command );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_u500_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_u500_constant_velocity_move()";

	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	char command[200];
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->constant_velocity_move >= 0 ) {
		sprintf( command, "FR %c %ld",
			u500_motor->axis_name, mx_round(motor->raw_speed) );
	} else {
		sprintf( command, "FR %c %ld",
			u500_motor->axis_name, - mx_round(motor->raw_speed) );
	}

	mx_status = mxi_u500_command( u500, u500_motor->board_number, command );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_u500_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_u500_get_parameter()";

	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	int board_number;
	double double_value;
	long long_value;
	char buffer[100];
	mx_status_type mx_status;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	board_number = u500_motor->board_number;

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
mxd_u500_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_u500_set_parameter()";

	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	int board_number;
	char axis_name;
	double double_value;
	long long_value;
	char command[200];
	AERERR_CODE wapi_status;
	mx_status_type mx_status;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	board_number = u500_motor->board_number;
	axis_name = u500_motor->axis_name;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* Just store the value for later use by the INDEX
		 * and FREERUN commands.
		 */
		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		/* The acceleration must be specified in counts/msec**2 */

		sprintf( command, "AC %c%ld",
			u500_motor->axis_name,
			1.0e-6 * motor->raw_acceleration_parameters[0] );

		mx_status = mxi_u500_command( u500, board_number, command );
		break;
	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = 1;

			snprintf( command, sizeof(command),
				"EN %c", u500_motor->axis_name );
		} else {
			snprintf( command, sizeof(command),
				"DI %c", u500_motor->axis_name );
		}

		mx_status = mxi_u500_command( u500, board_number, command );
		break;
#if 0
	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
		} else {
		}
		break;
#endif
	case MXLV_MTR_FAULT_RESET:
		/* First send an abort command. */

		mx_status = mxi_u500_command( u500, board_number, "AB" );

		/* FIXME: Do we need to insert a delay here for it to
		 * respond to the abort?
		 */

		mx_msleep(1000);

#if U500_DEBUG
		MX_DEBUG(-2,("%s: Invoking WAPIAerFaultAck()", fname));
#endif

		wapi_status = WAPIAerFaultAck();

		if ( wapi_status != 0 ) {
			return mxi_u500_error( wapi_status, fname,
			"Attempt to acknowledge a fault for motor '%s' failed.",
				motor->record->name );
		}
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		long_value = mx_round( motor->proportional_gain );

		sprintf( command, "GA %c KPOS%ld", axis_name, long_value );

		mx_status = mxi_u500_command( u500, board_number, command );
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		long_value = mx_round( motor->integral_gain );

		sprintf( command, "GA %c KI%ld", axis_name, long_value );

		mx_status = mxi_u500_command( u500, board_number, command );
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		long_value = mx_round( motor->derivative_gain );

		sprintf( command, "GA %c KP%ld", axis_name, long_value );

		mx_status = mxi_u500_command( u500, board_number, command );
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		long_value = mx_round( motor->velocity_feedforward_gain );

		sprintf( command, "GA %c VFF%ld", axis_name, long_value );

		mx_status = mxi_u500_command( u500, board_number, command );
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		long_value = mx_round( motor->acceleration_feedforward_gain );

		sprintf( command, "GA %c AFF%ld", axis_name, long_value );

		mx_status = mxi_u500_command( u500, board_number, command );
		break;
	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_u500_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_u500_get_status()";

	MX_U500_MOTOR *u500_motor = NULL;
	MX_U500 *u500 = NULL;
	ULONG status_word_1, status_word_2;
	ULONG enable_bitmask, busy_bitmask;
	AERERR_CODE wapi_status;
	double current_time;
	mx_status_type mx_status;

	mx_status = mxd_u500_get_pointers( motor, &u500_motor, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_u500_select_board( u500, u500_motor->board_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	wapi_status = WAPIAerCheckStatus();

	if ( wapi_status != 0 ) {
		return mxi_u500_error( wapi_status, fname,
			"Attempt to check the U500 status for "
			"controller '%s' by motor '%s' failed.",
				u500->record->name, motor->record->name );
	}

	status_word_1 = WAPIAerReadStatus( (SHORT) u500_motor->motor_number );

	status_word_2 = WAPIAerReadStatus( 5 );

#if U500_DEBUG
	MX_DEBUG(-2,("%s: status word 1 = %#x, status word 2 = %#x",
		fname, status_word_1, status_word_2));
#endif

	motor->status = 0;

	/*=== status_word_1 ===*/

	/* Bit 0: Position error */

	if ( status_word_1 & 0x1 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 1: RMS current error */

	if ( status_word_1 & 0x2 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 2: Integral error */

	if ( status_word_1 & 0x4 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 3: Hardware limit + */

	if ( status_word_1 & 0x8 ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* Bit 4: Hardware limit - */

	if ( status_word_1 & 0x10 ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 5: Software limit + */

	if ( status_word_1 & 0x20 ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* Bit 6: Software limit - */

	if ( status_word_1 & 0x40 ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 7: Driver fault */

	if ( status_word_1 & 0x80 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/* Bit 8: Feedback device error */

	if ( status_word_1 & 0x100 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 9: Global abort active (ignored) */

	/* Bits 10 and 11: Not used */

	/* Bit 12: Feedrate > max setting error (ignored) */

	/* Bit 13: Velocity error */

	if ( status_word_1 & 0x2000 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 14: Emergency stop (ignored) */

	/* Bits 15 to 30: Not used */

	/* Bit 31: Driver interlock open */

	if ( status_word_1 & 0x80000000 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/*=== status_word_2 ===*/

	enable_bitmask = ( 1 << (u500_motor->motor_number - 1) );

	if ( ( status_word_2 & enable_bitmask ) == 0 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	busy_bitmask = ( 1 << (u500_motor->motor_number + 3) );

	if ( status_word_2 & busy_bitmask ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* If a move command was sent "recently", then we unconditionally
	 * turn on the busy bit.  The units of time are expressed in
	 * seconds here.
	 */

	current_time = mx_high_resolution_time_as_double();

	if ( ( current_time - u500_motor->last_start_time ) < 0.5 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

#if U500_DEBUG
	MX_DEBUG(-2,("%s: motor->status = %#lx", fname, motor->status));
#endif

#if 0
	MX_DEBUG(-2,("%s: IS_BUSY               = %lu", fname,
			motor->status & MXSF_MTR_IS_BUSY ));

	MX_DEBUG(-2,("%s: POSITIVE_LIMIT_HIT    = %lu", fname,
			motor->status & MXSF_MTR_POSITIVE_LIMIT_HIT ));

	MX_DEBUG(-2,("%s: NEGATIVE_LIMIT_HIT    = %lu", fname,
			motor->status & MXSF_MTR_NEGATIVE_LIMIT_HIT ));

	MX_DEBUG(-2,("%s: HOME_SEARCH_SUCCEEDED = %lu", fname,
			motor->status & MXSF_MTR_HOME_SEARCH_SUCCEEDED ));

	MX_DEBUG(-2,("%s: FOLLOWING_ERROR       = %lu", fname,
			motor->status & MXSF_MTR_FOLLOWING_ERROR ));

	MX_DEBUG(-2,("%s: DRIVE_FAULT           = %lu", fname,
			motor->status & MXSF_MTR_DRIVE_FAULT ));

	MX_DEBUG(-2,("%s: AXIS_DISABLED         = %lu", fname,
			motor->status & MXSF_MTR_AXIS_DISABLED ));

	MX_DEBUG(-2,("%s: OPEN_LOOP             = %lu", fname,
			motor->status & MXSF_MTR_OPEN_LOOP ));
#endif
	return MX_SUCCESSFUL_RESULT;
}

