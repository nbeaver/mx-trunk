/*
 * Name:    d_powerpmac.c
 *
 * Purpose: MX driver for Delta Tau PowerPMAC motors.
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

#define POWERPMAC_DEBUG			TRUE

#define POWERPMAC_USE_JOG_COMMANDS	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"

#if HAVE_POWER_PMAC_LIBRARY

#include "gplib.h"	/* Delta Tau-provided include file. */

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "i_powerpmac.h"
#include "d_powerpmac.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_powerpmac_record_function_list = {
	NULL,
	mxd_powerpmac_create_record_structures,
	mxd_powerpmac_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_powerpmac_open
};

MX_MOTOR_FUNCTION_LIST mxd_powerpmac_motor_function_list = {
	NULL,
	mxd_powerpmac_move_absolute,
	mxd_powerpmac_get_position,
	mxd_powerpmac_set_position,
	mxd_powerpmac_soft_abort,
	mxd_powerpmac_immediate_abort,
	NULL,
	NULL,
	mxd_powerpmac_find_home_position,
	mxd_powerpmac_constant_velocity_move,
	mxd_powerpmac_get_parameter,
	mxd_powerpmac_set_parameter,
	mxd_powerpmac_simultaneous_start,
	mxd_powerpmac_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_powerpmac_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_POWERPMAC_STANDARD_FIELDS
};

long mxd_powerpmac_num_record_fields
		= sizeof( mxd_powerpmac_record_field_defaults )
			/ sizeof( mxd_powerpmac_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_rfield_def_ptr
			= &mxd_powerpmac_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_powerpmac_get_pointers( MX_MOTOR *motor,
			MX_POWERPMAC_MOTOR **powerpmac_motor,
			MX_POWERPMAC **powerpmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_powerpmac_get_pointers()";

	MX_POWERPMAC_MOTOR *powerpmac_motor_ptr;
	MX_RECORD *powerpmac_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	powerpmac_motor_ptr = (MX_POWERPMAC_MOTOR *)
				motor->record->record_type_struct;

	if ( powerpmac_motor_ptr == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POWERPMAC_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( powerpmac_motor != (MX_POWERPMAC_MOTOR **) NULL ) {
		*powerpmac_motor = powerpmac_motor_ptr;
	}

	if ( powerpmac == (MX_POWERPMAC **) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	powerpmac_record = powerpmac_motor_ptr->powerpmac_record;

	if ( powerpmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The powerpmac_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*powerpmac = (MX_POWERPMAC *) powerpmac_record->record_type_struct;

	if ( *powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POWERPMAC pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_powerpmac_create_record_structures()";

	MX_MOTOR *motor;
	MX_POWERPMAC_MOTOR *powerpmac_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	powerpmac_motor = (MX_POWERPMAC_MOTOR *)
				malloc( sizeof(MX_POWERPMAC_MOTOR) );

	if ( powerpmac_motor == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_POWERPMAC_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = powerpmac_motor;
	record->class_specific_function_list
				= &mxd_powerpmac_motor_function_list;

	motor->record = record;
	powerpmac_motor->record = record;

	/* A PowerPMAC motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* We express accelerations in counts/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_powerpmac_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_POWERPMAC_MOTOR *powerpmac_motor;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( powerpmac_motor->motor_number < 1 )
				|| ( powerpmac_motor->motor_number > 32 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal PowerPMAC motor number %ld for record '%s'.  "
		"The allowed range is 1 to 32.",
			powerpmac_motor->motor_number, record->name );
	}

	powerpmac_motor->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_powerpmac_open()";

	MX_MOTOR *motor;
	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get a pointer to the MotorData for this motor. */

	powerpmac_motor->motor_data =
	    &(powerpmac->shared_mem->Motor[ powerpmac_motor->motor_number ]);

	if ( powerpmac_motor->motor_data == (MotorData *) NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unable to get a pointer to the MotorData structure for "
		"PowerPMAC motor '%s'.", record->name );
	}

	powerpmac_motor->use_shm = TRUE;

	/* For PowerPMACs, the following call forces the
	 * MXSF_MTR_IS_BUSY bit on for 0.01 seconds after
	 * the start of a move.  This is done because
	 * sometimes the PowerPMAC controller will report
	 * the motor as not moving for a brief interval
	 * after the start of a move.
	 */

	mx_status = mx_motor_set_busy_start_interval( record, 0.01 );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_powerpmac_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_move_absolute()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	char command[20];
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_destination.analog;

	snprintf( command, sizeof(command), "J=%ld", motor_steps );

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac,
					command, NULL, 0, POWERPMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_get_position_steps()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	MotorData *motor_data = NULL;
	char response[50];
	int num_tokens;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( powerpmac_motor->use_shm ) {
		motor_data = powerpmac_motor->motor_data;

		motor->raw_position.analog =
			motor_data->ActPos - motor_data->HomePos;

#if POWERPMAC_DEBUG
		MX_DEBUG(-2,("%s: motor->raw_position.analog = %g",
			fname, motor->raw_position.analog));
#endif
	} else {
		mx_status = mxd_powerpmac_jog_command(
				powerpmac_motor, powerpmac, "P",
				response, sizeof response, POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_tokens = sscanf( response, "%lg",
				&(motor->raw_position.analog) );

		if ( num_tokens != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No position value seen in response to 'P' command.  "
			"num_tokens = %d, Response seen = '%s'",
				num_tokens, response );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_set_position()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Redefining the current position is not yet implemented "
		"for PowerPMAC-controlled motor '%s'.",
			motor->record->name );
}

MX_EXPORT mx_status_type
mxd_powerpmac_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_soft_abort()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac, "J/",
						NULL, 0, POWERPMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_immediate_abort()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
							&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac, "K",
						NULL, 0, POWERPMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_find_home_position()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	double raw_home_speed;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_powerpmac_get_motor_variable( powerpmac_motor,
						powerpmac, 23,
						MXFT_DOUBLE, &raw_home_speed,
						POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_home_speed = fabs( raw_home_speed );

	if ( motor->home_search < 0 ) {
		raw_home_speed = -raw_home_speed;
	}

	mx_status = mxd_powerpmac_set_motor_variable( powerpmac_motor,
						powerpmac, 23,
						MXFT_DOUBLE, raw_home_speed,
						POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac, "HM",
						NULL, 0, POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_constant_velocity_move()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->constant_velocity_move > 0 ) {
		mx_status = mxd_powerpmac_jog_command( powerpmac_motor,
						powerpmac, "J+",
						NULL, 0, POWERPMAC_DEBUG );
	} else
	if ( motor->constant_velocity_move < 0 ) {
		mx_status = mxd_powerpmac_jog_command( powerpmac_motor,
						powerpmac, "J-",
						NULL, 0, POWERPMAC_DEBUG );
	} else {
		mx_status = MX_SUCCESSFUL_RESULT;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_get_parameter()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mxd_powerpmac_get_motor_variable( powerpmac_motor,
						powerpmac, 22,
						MXFT_DOUBLE, &double_value,
						POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Reported by the controller in count/msec. */

		motor->raw_speed = 1000.0 * double_value;
		break;
	case MXLV_MTR_BASE_SPEED:
		/* The POWERPMAC doesn't seem to have the concept of
		 * a base speed, since it is oriented to servo motors.
		 */

		motor->raw_base_speed = 0.0;

		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mxd_powerpmac_get_motor_variable( powerpmac_motor,
						powerpmac, 19,
						MXFT_DOUBLE, &double_value,
						POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		double_value = 1000000.0 * double_value;

		motor->raw_acceleration_parameters[0] = double_value;

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;
	case MXLV_MTR_AXIS_ENABLE:
		mx_status = mxd_powerpmac_get_motor_variable( powerpmac_motor,
						powerpmac, 0,
						MXFT_BOOL, &double_value,
						POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mx_round(double_value) != 0 ) {
			motor->axis_enable = TRUE;
		} else {
			motor->axis_enable = FALSE;
		}
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mxd_powerpmac_get_motor_variable(
						powerpmac_motor,
						powerpmac, 30, MXFT_DOUBLE,
						&(motor->proportional_gain),
						POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mxd_powerpmac_get_motor_variable(
						powerpmac_motor,
						powerpmac, 33, MXFT_DOUBLE,
						&(motor->integral_gain),
						POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mxd_powerpmac_get_motor_variable(
						powerpmac_motor,
						powerpmac, 31, MXFT_DOUBLE,
						&(motor->derivative_gain),
						POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		mx_status = mxd_powerpmac_get_motor_variable(
						powerpmac_motor,
						powerpmac, 32, MXFT_DOUBLE,
					&(motor->velocity_feedforward_gain),
						POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		mx_status = mxd_powerpmac_get_motor_variable(
						powerpmac_motor,
						powerpmac, 35, MXFT_DOUBLE,
					&(motor->acceleration_feedforward_gain),
						POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_LIMIT:
		mx_status = mxd_powerpmac_get_motor_variable(
						powerpmac_motor,
						powerpmac, 34, MXFT_DOUBLE,
						&(motor->integral_limit),
						POWERPMAC_DEBUG );
		break;
	default:
		mx_status = mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_set_parameter()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	double double_value;
	long long_value;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		double_value = 0.001 * motor->raw_speed;

		mx_status = mxd_powerpmac_set_motor_variable( powerpmac_motor,
						powerpmac, 22,
						MXFT_DOUBLE, double_value,
						POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_BASE_SPEED:
		/* The POWERPMAC doesn't seem to have the concept of a
		 * base speed, since it is oriented to servo motors.
		 */

		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		/* These acceleration parameters are chosen such that the
		 * value of Ixx19 is used as counts/msec**2.
		 */

		double_value = motor->raw_acceleration_parameters[0];

		double_value = 1.0e-6 * double_value;

		mx_status = mxd_powerpmac_set_motor_variable( powerpmac_motor,
						powerpmac, 19,
						MXFT_DOUBLE, double_value,
						POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set Ixx20 (jog acceleration time) to 1.0 msec. */

		double_value = 1.0;

		mx_status = mxd_powerpmac_set_motor_variable( powerpmac_motor,
						powerpmac, 20,
						MXFT_DOUBLE, double_value,
						POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set Ixx21 (jog acceleration S-curve time) to 0. */

		double_value = 0.0;

		mx_status = mxd_powerpmac_set_motor_variable( powerpmac_motor,
						powerpmac, 21,
						MXFT_DOUBLE, double_value,
						POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = TRUE;
		}

		mx_status = mxd_powerpmac_set_motor_variable( powerpmac_motor,
					powerpmac, 0,
					MXFT_BOOL, motor->axis_enable,
					POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
			mx_status = mxd_powerpmac_jog_command(
						powerpmac_motor, powerpmac,
						"J/", NULL, 0, POWERPMAC_DEBUG );
		} else {
			mx_status = mxd_powerpmac_jog_command(
						powerpmac_motor, powerpmac,
						"K", NULL, 0, POWERPMAC_DEBUG );
		}
		break;
	case MXLV_MTR_FAULT_RESET:
		mx_status = mxd_powerpmac_jog_command(
					powerpmac_motor, powerpmac, "J/",
					NULL, 0, POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mxd_powerpmac_set_motor_variable(
					powerpmac_motor, powerpmac,
					30, MXFT_DOUBLE,
					motor->proportional_gain,
					POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mxd_powerpmac_set_motor_variable(
					powerpmac_motor, powerpmac,
					33, MXFT_DOUBLE,
					motor->integral_gain,
					POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mxd_powerpmac_set_motor_variable(
					powerpmac_motor, powerpmac,
					31, MXFT_DOUBLE,
					motor->derivative_gain,
					POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		mx_status = mxd_powerpmac_set_motor_variable(
					powerpmac_motor, powerpmac,
					32, MXFT_DOUBLE,
					motor->velocity_feedforward_gain,
					POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		mx_status = mxd_powerpmac_set_motor_variable(
					powerpmac_motor, powerpmac,
					35, MXFT_DOUBLE,
					motor->acceleration_feedforward_gain,
					POWERPMAC_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_LIMIT:
		long_value = mx_round( motor->integral_limit );

		if ( long_value ) {
			long_value = 1;
		}

		mx_status = mxd_powerpmac_set_motor_variable(
					powerpmac_motor, powerpmac,
					34, MXFT_DOUBLE,
					long_value,
					POWERPMAC_DEBUG );
		break;
	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_simultaneous_start( long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				unsigned long flags )
{
	static const char fname[] = "mxd_powerpmac_simultaneous_start()";

	MX_RECORD *powerpmac_interface_record = NULL;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	char command_buffer[500];
	char *ptr;
	double raw_position;
	long i, raw_steps;
	size_t length, buffer_left;
	mx_status_type mx_status;

	if ( num_motor_records <= 0 )
		return MX_SUCCESSFUL_RESULT;

	/* Construct the move command to send to the POWERPMAC. */

	ptr = command_buffer;

	*ptr = '\0';

	for ( i = 0; i < num_motor_records; i++ ) {
		motor_record = motor_record_array[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor_record->mx_type != MXT_MTR_POWERPMAC ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same type of motors.",
				motor_record_array[0]->name,
				motor_record->name );
		}

		powerpmac_motor = (MX_POWERPMAC_MOTOR *)
					motor_record->record_type_struct;

		if ( powerpmac_interface_record == (MX_RECORD *) NULL ) {
			powerpmac_interface_record
				= powerpmac_motor->powerpmac_record;

			powerpmac = (MX_POWERPMAC *)
				powerpmac_interface_record->record_type_struct;
		}

		/* Verify that the POWERPMAC motor records all belong
		 * to the same PowerPMAC controller.
		 */

		if ( powerpmac_interface_record
				!= powerpmac_motor->powerpmac_record )
		{
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different POWERPMAC interfaces, "
		"namely '%s' and '%s'.",
				motor_record_array[0]->name,
				motor_record->name,
				powerpmac_interface_record->name,
				powerpmac_motor->powerpmac_record->name );
		}

		/* Compute the new position in raw units. */

		raw_position =
			mx_divide_safely( position_array[i] - motor->offset,
						motor->scale );

		raw_steps = mx_round( raw_position );

		/* Append the part of the command referring to this motor. */

		length = strlen( command_buffer );
		buffer_left = sizeof(command_buffer) - length;
		ptr = command_buffer + length;

		snprintf( ptr, buffer_left,
		    "#%ldJ=%ld ", powerpmac_motor->motor_number, raw_steps );
	}

	if ( powerpmac_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"No powerpmac interface record pointer was found for "
		"record '%s'.", motor_record_array[0]->name );
	}

	/* Send the command to the POWERPMAC. */

	MX_DEBUG( 2,("%s: command = '%s'", fname, command_buffer));

	mx_status = mxi_powerpmac_command( powerpmac, command_buffer,
						NULL, 0, POWERPMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_powerpmac_get_status()";

	MX_POWERPMAC_MOTOR *powerpmac_motor = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	char response[50];
	unsigned long status[ MX_POWERPMAC_NUM_STATUS_CHARACTERS ];
	int i, length;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_get_pointers( motor, &powerpmac_motor,
						&powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Request the motor status. */

	mx_status = mxd_powerpmac_jog_command( powerpmac_motor, powerpmac, "?",
				response, sizeof response, POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = (int) strlen( response );

	if ( length < MX_POWERPMAC_NUM_STATUS_CHARACTERS ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Too few characters seen in response to motor status command.  "
	"Only saw %d characters.", length );
	}

	/* Change the reported motor status from ASCII characters
	 * to unsigned long integers.
	 *
	 * Note that the first character returned is a $ character.
	 * We must skip over that.
	 */

	for ( i = 0; i < MX_POWERPMAC_NUM_STATUS_CHARACTERS; i++ ) {
		status[i] = mx_hex_char_to_unsigned_long( response[i+1] );

#if 0
		MX_DEBUG(-2,("%s: status[%d] = %#lx",
			fname, i, status[i]));
#endif
	}

	/* Check for all the status bits that we are interested in. */

	motor->status = 0;

	/* ============ First word returned. ============== */

	/* ---------- First character returned. ----------- */

	/* Bits 30 and 31: (ignored) */

	/* Bit 29: Hardware negative limit set */

	if ( status[0] & 0x2 ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 28: Hardware positive limit set */

	if ( status[0] & 0x1 ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* ---------- Second character returned. ----------- */

	/* Bit 27: (ignored) */

	/* Bit 26: Fatal following error */

	if ( status[1] & 0x4 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 25: (ignored) */

	/* Bit 24: Amplifier fault */

	if ( status[1] & 0x1 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/* ---------- Third character returned. ----------- */

	/* Bit 23: Software negative limit set */

	if ( status[2] & 0x8 ) {
		motor->status |= MXSF_MTR_SOFT_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 22: Software positive limit set */

	if ( status[2] & 0x4 ) {
		motor->status |= MXSF_MTR_SOFT_POSITIVE_LIMIT_HIT;
	}

	/* Bits 20 and 21: (ignored) */

	/* ---------- Fourth character returned. ----------- */

	/* Bits 16 to 19: (ignored) */

	/* ---------- Fifth character returned. ----------- */

	/* Bit 15: Home complete */

	if ( status[4] & 0x8 ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	/* Bit 14: (ignored) */

	/* Bit 13: Closed-loop mode */

	if ( (status[4] & 0x2) == 0 ) {
		motor->status |= MXSF_MTR_OPEN_LOOP;
	}

	/* Bit 12: Amplifier enabled */

	if ( (status[4] & 0x1) == 0 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/* ---------- Sixth character returned. ----------- */

	/* Bit 11: In position */

	if ( (status[5] & 0x8) == 0 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* Note: all remaining bits in the response are ignored. */

	return MX_SUCCESSFUL_RESULT;
}

/*-----------*/

MX_EXPORT mx_status_type
mxd_powerpmac_jog_command( MX_POWERPMAC_MOTOR *powerpmac_motor,
			MX_POWERPMAC *powerpmac,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_powerpmac_jog_command()";

	char command_buffer[100];
	mx_status_type mx_status;

	if ( powerpmac_motor == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC_MOTOR pointer passed was NULL." );
	}
	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC pointer passed for motor '%s' was NULL.",
			powerpmac_motor->record->name );
	}

	snprintf( command_buffer, sizeof(command_buffer),
			"#%ld%s", powerpmac_motor->motor_number, command );

	mx_status = mxi_powerpmac_command( powerpmac, command_buffer,
				response, response_buffer_length, debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_powerpmac_get_motor_variable( MX_POWERPMAC_MOTOR *powerpmac_motor,
				MX_POWERPMAC *powerpmac,
				long variable_number,
				long variable_type,
				double *double_ptr,
				int debug_flag )
{
	static const char fname[] = "mxd_powerpmac_get_motor_variable()";

	char command_buffer[100];
	char response[100];
	char *response_ptr;
	int num_items;
	long long_value;
	mx_status_type mx_status;

	if ( powerpmac_motor == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC_MOTOR pointer passed was NULL." );
	}
	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC pointer passed for motor '%s' was NULL.",
			powerpmac_motor->record->name );
	}

	snprintf( command_buffer, sizeof(command_buffer),
	    "I%ld%02ld", powerpmac_motor->motor_number, variable_number );

	mx_status = mxi_powerpmac_command( powerpmac, command_buffer,
				response, sizeof response, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* On PowerPMAC systems, the returned I-variable is prefixed with
	 * an Ixx= string.  For example, if you ask for I130, the response
	 * looks like I130=40
	 */

	response_ptr = strchr( response, '=' );

	if ( response_ptr == NULL ) {
		response_ptr = response;
	} else {
		response_ptr++;
	}

	/* Parse the returned value. */

	switch( variable_type ) {
	case MXFT_LONG:
		num_items = sscanf( response_ptr, "%ld", &long_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot parse response to command '%s' as a long integer.  Response = '%s'",
				command_buffer, response );
		}

		*double_ptr = (double) long_value;
		break;
	case MXFT_BOOL:
		num_items = sscanf( response_ptr, "%ld", &long_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot parse response to command '%s' as a long integer.  Response = '%s'",
				command_buffer, response );
		}

		if ( long_value != 0 ) {
			*double_ptr = 1.0;
		} else {
			*double_ptr = 0.0;
		}
		break;
	case MXFT_DOUBLE:
		num_items = sscanf( response_ptr, "%lg", double_ptr );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot parse response to command '%s' as a double.  Response = '%s'",
				command_buffer, response );
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG, MXFT_BOOL, and MXFT_DOUBLE are supported." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_set_motor_variable( MX_POWERPMAC_MOTOR *powerpmac_motor,
				MX_POWERPMAC *powerpmac,
				long variable_number,
				long variable_type,
				double double_value,
				int debug_flag )
{
	static const char fname[] = "mxd_powerpmac_set_motor_variable()";

	char command_buffer[100];
	char response[100];
	long long_value;
	mx_status_type mx_status;

	if ( powerpmac_motor == (MX_POWERPMAC_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC_MOTOR pointer passed was NULL." );
	}
	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC pointer passed for motor '%s' was NULL.",
			powerpmac_motor->record->name );
	}

	switch( variable_type ) {
	case MXFT_LONG:
	case MXFT_BOOL:
		long_value = mx_round( double_value );

		if ( (variable_type == MXFT_BOOL)
		  && (long_value != 0) )
		{
			long_value = 1;
		}

		snprintf( command_buffer, sizeof(command_buffer),
			"I%ld%02ld=%ld",
			powerpmac_motor->motor_number,
			variable_number,
			long_value );
		break;
	case MXFT_DOUBLE:
		snprintf( command_buffer, sizeof(command_buffer),
			"I%ld%02ld=%f",
			powerpmac_motor->motor_number,
			variable_number,
			double_value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG, MXFT_BOOL, and MXFT_DOUBLE are supported." );
	}

	mx_status = mxi_powerpmac_command( powerpmac, command_buffer,
				response, sizeof response, debug_flag );

	return mx_status;
}

#endif /* HAVE_POWER_PMAC_LIBRARY */

