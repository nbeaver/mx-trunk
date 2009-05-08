/*
 * Name:    d_pmac.c
 *
 * Purpose: MX driver for Delta Tau PMAC motors.
 *
 *          Note that this driver performs its work using the PMAC jog
 *          commands, rather than using a motion program.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PMAC_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mxconfig.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "d_pmac.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_pmac_record_function_list = {
	NULL,
	mxd_pmac_create_record_structures,
	mxd_pmac_finish_record_initialization,
	NULL,
	mxd_pmac_print_structure,
	NULL,
	NULL,
	NULL,
	mxd_pmac_close,
	NULL,
	mxd_pmac_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_pmac_motor_function_list = {
	NULL,
	mxd_pmac_move_absolute,
	mxd_pmac_get_position,
	mxd_pmac_set_position,
	mxd_pmac_soft_abort,
	mxd_pmac_immediate_abort,
	NULL,
	NULL,
	mxd_pmac_find_home_position,
	mxd_pmac_constant_velocity_move,
	mxd_pmac_get_parameter,
	mxd_pmac_set_parameter,
	mxd_pmac_simultaneous_start,
	mxd_pmac_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_pmac_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PMAC_STANDARD_FIELDS
};

long mxd_pmac_num_record_fields
		= sizeof( mxd_pmac_record_field_defaults )
			/ sizeof( mxd_pmac_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmac_rfield_def_ptr
			= &mxd_pmac_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_pmac_get_pointers( MX_MOTOR *motor,
			MX_PMAC_MOTOR **pmac_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmac_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( pmac_motor == (MX_PMAC_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pmac_motor = (MX_PMAC_MOTOR *) (motor->record->record_type_struct);

	if ( *pmac_motor == (MX_PMAC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PMAC_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pmac_motor_get_pmac_pointer( MX_PMAC_MOTOR *pmac_motor,
			MX_PMAC **pmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmac_motor_get_pmac_pointer()";

	MX_RECORD *pmac_record;

	if ( pmac_motor == (MX_PMAC_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( pmac == (MX_PMAC **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pmac_record = pmac_motor->pmac_record;

	if ( pmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The pmac_record pointer for record '%s' is NULL.",
			pmac_motor->record->name );
	}

	*pmac = (MX_PMAC *) pmac_record->record_type_struct;

	if ( *pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PMAC pointer for record '%s' is NULL.",
			pmac_motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_create_record_structures()";

	MX_MOTOR *motor;
	MX_PMAC_MOTOR *pmac_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	pmac_motor = (MX_PMAC_MOTOR *) malloc( sizeof(MX_PMAC_MOTOR) );

	if ( pmac_motor == (MX_PMAC_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PMAC_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = pmac_motor;
	record->class_specific_function_list
				= &mxd_pmac_motor_function_list;

	motor->record = record;
	pmac_motor->record = record;

	/* A PMAC motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* We express accelerations in counts/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_finish_record_initialization()";

	MX_PMAC_MOTOR *pmac_motor;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pmac_motor = (MX_PMAC_MOTOR *) record->record_type_struct;

	if ( ( pmac_motor->motor_number < 1 )
				|| ( pmac_motor->motor_number > 32 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Illegal PMAC motor number %ld for record '%s'.  Allowed range is 1 to 32.",
			pmac_motor->motor_number, record->name );
	}

	pmac_motor->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_print_structure()";

	MX_MOTOR *motor;
	MX_PMAC_MOTOR *pmac_motor = NULL;
	long motor_steps;
	double position, backlash;
	double negative_limit, positive_limit, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type     = PMAC motor.\n\n");

	fprintf(file, "  name           = %s\n", record->name);
	fprintf(file, "  port name      = %s\n", pmac_motor->pmac_record->name);
	fprintf(file, "  motor number   = %ld\n", pmac_motor->motor_number);

	mx_status = mx_motor_get_position_steps( record, &motor_steps );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
				record->name );
	}
	
	position = motor->offset + motor->scale
			* (double) motor_steps;
	
	fprintf(file, "  position       = %ld steps (%g %s)\n",
			motor_steps, position, motor->units);
	fprintf(file, "  scale          = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset         = %g %s.\n",
			motor->offset, motor->units);
	
	backlash = motor->scale
			* (double) (motor->raw_backlash_correction.stepper);
	
	fprintf(file, "  backlash       = %ld steps (%g %s)\n",
			motor->raw_backlash_correction.stepper,
			backlash, motor->units);
	
	negative_limit = motor->offset
	+ motor->scale * (double)(motor->raw_negative_limit.stepper);
	
	fprintf(file, "  negative limit = %ld steps (%g %s)\n",
			motor->raw_negative_limit.stepper,
			negative_limit, motor->units);

	positive_limit = motor->offset
	+ motor->scale * (double)(motor->raw_positive_limit.stepper);

	fprintf(file, "  positive limit = %ld steps (%g %s)\n",
			motor->raw_positive_limit.stepper,
			positive_limit, motor->units);

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband  = %ld steps (%g %s)\n\n",
			motor->raw_move_deadband.stepper,
			move_deadband, motor->units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_close( MX_RECORD *record )
{
	return mx_motor_get_position( record, NULL );
}

MX_EXPORT mx_status_type
mxd_pmac_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_resynchronize()";

	MX_MOTOR *motor;
	MX_PMAC_MOTOR *pmac_motor = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_resynchronize_record( pmac_motor->pmac_record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_pmac_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_move_absolute()";

	MX_PMAC_MOTOR *pmac_motor = NULL;
	MX_PMAC *pmac = NULL;
	char command[20];
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_motor_get_pmac_pointer( pmac_motor, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_destination.stepper;

	snprintf( command, sizeof(command), "J=%ld", motor_steps );

	mx_status = mxd_pmac_jog_command( pmac_motor, command,
						NULL, 0, PMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_get_position_steps()";

	MX_PMAC_MOTOR *pmac_motor = NULL;
	char response[50];
	int num_tokens;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_jog_command( pmac_motor, "P",
				response, sizeof response, PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The PMAC may report values back in terms of fractions of
	 * an encoder count.
	 */

	num_tokens = sscanf( response, "%lg", &double_value );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to 'P' command.  "
		"num_tokens = %d, Response seen = '%s'", num_tokens, response );
	}

	/* We round this to the nearest encoder count. */

	motor->raw_position.stepper = mx_round( double_value );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pmac_configure_position_registers( MX_PMAC *pmac,
					MX_PMAC_MOTOR *pmac_motor )
{
	char command[40];
	char *ptr;
	long num;
	size_t length, buffer_left;
	mx_status_type mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command, sizeof(command),
				"@%lx", pmac_motor->card_number );
	} else {
		strlcpy( command, "", sizeof(command) );
	}

	length = strlen( command );
	buffer_left = sizeof(command) - length;
	ptr = command + length;

	num = pmac_motor->motor_number;

	if ( pmac->pmac_type & MX_PMAC_TYPE_TURBO ) {
		snprintf( ptr, buffer_left,
			"M%ld64->D:$%lXC", num, 4 + 8 * num );
	} else {
		snprintf( ptr, buffer_left,
			"M%ld64->D:$%lX", num, 192 * (num - 1) + 2067 );
	}

	mx_status = mxi_pmac_command( pmac, command, NULL, 0, PMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_set_position()";

	MX_PMAC_MOTOR *pmac_motor = NULL;
	MX_PMAC *pmac = NULL;
	long motor_position_scale_factor;
	char command[40];
	char response[40];
	char *ptr;
	size_t length, buffer_left;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_motor_get_pmac_pointer( pmac_motor, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if the Mx64 command is defined or not. */

	if ( pmac->num_cards > 1 ) {
		snprintf( command, sizeof(command),
				"@%lx", pmac_motor->card_number );
	} else {
		strlcpy( command, "", sizeof(command) );
	}

	length = strlen( command );
	buffer_left = sizeof(command) - length;
	ptr = command + length;

	snprintf( ptr, buffer_left, "M%ld64->", pmac_motor->motor_number );

	mx_status = mxi_pmac_command( pmac, command,
				response, sizeof response, PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = response + strspn( response, " \t\r\n" );

	if ( *ptr == '*' ) {
		/* The Mx64 is not defined, so we attempt to define its
		 * recommended value here.
		 */

		mx_status = mxd_pmac_configure_position_registers(
							pmac, pmac_motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Get the motor position scale factor.  The normal value is 96. */

	mx_status = mxd_pmac_get_motor_variable( pmac_motor, 8, MXFT_LONG,
						&motor_position_scale_factor,
						PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the old reported position in motor steps or encoder ticks. */

	mx_status = mx_motor_get_position( motor->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Change the position offset. */

	if ( pmac->num_cards > 1 ) {

		length = strlen( command );
		buffer_left = sizeof(command) - length;
		ptr = command + length;

		snprintf( command, buffer_left,
				"@%lx", pmac_motor->card_number );
	} else {
		strlcpy( command, "", sizeof(command) );
	}

	length = strlen( command );
	buffer_left = sizeof(command) - length;
	ptr = command + length;

	snprintf( ptr, buffer_left,
			"#%ldHOMEZ M%ld64=%ld*32*I%ld08",
			pmac_motor->motor_number,
			pmac_motor->motor_number,
			motor->raw_set_position.stepper,
			pmac_motor->motor_number );

	mx_status = mxi_pmac_command( pmac, command,
				NULL, 0, PMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_soft_abort()";

	MX_PMAC_MOTOR *pmac_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_jog_command( pmac_motor, "J/",
						NULL, 0, PMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_immediate_abort()";

	MX_PMAC_MOTOR *pmac_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_jog_command( pmac_motor, "K",
						NULL, 0, PMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_find_home_position()";

	MX_PMAC_MOTOR *pmac_motor = NULL;
	double raw_home_speed;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_get_motor_variable( pmac_motor, 23,
						MXFT_DOUBLE, &raw_home_speed,
						PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_home_speed = fabs( raw_home_speed );

	if ( motor->home_search < 0 ) {
		raw_home_speed = -raw_home_speed;
	}

	mx_status = mxd_pmac_set_motor_variable( pmac_motor, 23,
						MXFT_DOUBLE, &raw_home_speed,
						PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_jog_command( pmac_motor, "HM",
						NULL, 0, PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_constant_velocity_move()";

	MX_PMAC_MOTOR *pmac_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->constant_velocity_move > 0 ) {
		mx_status = mxd_pmac_jog_command( pmac_motor, "J+",
						NULL, 0, PMAC_DEBUG );
	} else
	if ( motor->constant_velocity_move < 0 ) {
		mx_status = mxd_pmac_jog_command( pmac_motor, "J-",
						NULL, 0, PMAC_DEBUG );
	} else {
		mx_status = MX_SUCCESSFUL_RESULT;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_get_parameter()";

	MX_PMAC_MOTOR *pmac_motor = NULL;
	double double_value;
	long long_value;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mxd_pmac_get_motor_variable( pmac_motor, 22,
						MXFT_DOUBLE, &double_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Reported by the controller in count/msec. */

		motor->raw_speed = 1000.0 * double_value;
		break;
	case MXLV_MTR_BASE_SPEED:
		/* The PMAC doesn't seem to have the concept of a base speed,
		 * since it is oriented to servo motors.
		 */

		motor->raw_base_speed = 0.0;

		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mxd_pmac_get_motor_variable( pmac_motor, 19,
						MXFT_DOUBLE, &double_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		double_value = 1000000.0 * double_value;

		motor->raw_acceleration_parameters[0] = double_value;

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mxd_pmac_get_motor_variable( pmac_motor, 30,
						MXFT_LONG, &long_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->proportional_gain = (double) long_value;
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mxd_pmac_get_motor_variable( pmac_motor, 33,
						MXFT_LONG, &long_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->integral_gain = (double) long_value;
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mxd_pmac_get_motor_variable( pmac_motor, 31,
						MXFT_LONG, &long_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->derivative_gain = (double) long_value;
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		mx_status = mxd_pmac_get_motor_variable( pmac_motor, 32,
						MXFT_LONG, &long_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->velocity_feedforward_gain = (double) long_value;
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		mx_status = mxd_pmac_get_motor_variable( pmac_motor, 35,
						MXFT_LONG, &long_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->acceleration_feedforward_gain = (double) long_value;
		break;
	case MXLV_MTR_INTEGRAL_LIMIT:
		mx_status = mxd_pmac_get_motor_variable( pmac_motor, 34,
						MXFT_LONG, &long_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->integral_limit = (double) long_value;
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_set_parameter()";

	MX_PMAC_MOTOR *pmac_motor = NULL;
	double double_value;
	long long_value;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		double_value = 0.001 * motor->raw_speed;

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 22,
						MXFT_DOUBLE, &double_value,
						PMAC_DEBUG );
		break;
	case MXLV_MTR_BASE_SPEED:
		/* The PMAC doesn't seem to have the concept of a base speed,
		 * since it is oriented to servo motors.
		 */

		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		/* These acceleration parameters are chosen such that the
		 * value of Ixx19 is used as counts/msec**2.
		 */

		double_value = motor->raw_acceleration_parameters[0];

		double_value = 1.0e-6 * double_value;

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 19,
						MXFT_DOUBLE, &double_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set Ixx20 (jog acceleration time) to 1.0 msec. */

		double_value = 1.0;

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 20,
						MXFT_DOUBLE, &double_value,
						PMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set Ixx21 (jog acceleration S-curve time) to 0. */

		double_value = 0.0;

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 21,
						MXFT_DOUBLE, &double_value,
						PMAC_DEBUG );
		break;
	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = TRUE;
		}

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 0,
					MXFT_BOOL, &(motor->axis_enable),
					PMAC_DEBUG );
		break;
	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
			mx_status = mxd_pmac_jog_command( pmac_motor, "J/",
						NULL, 0, PMAC_DEBUG );
		} else {
			mx_status = mxd_pmac_jog_command( pmac_motor, "K",
						NULL, 0, PMAC_DEBUG );
		}
		break;
	case MXLV_MTR_FAULT_RESET:
		mx_status = mxd_pmac_jog_command( pmac_motor, "J/",
						NULL, 0, PMAC_DEBUG );
		break;
	case MXLV_MTR_PROPORTIONAL_GAIN:
		long_value = mx_round( motor->proportional_gain );

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 30,
					MXFT_LONG, &long_value, PMAC_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_GAIN:
		long_value = mx_round( motor->integral_gain );

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 33,
					MXFT_LONG, &long_value, PMAC_DEBUG );
		break;
	case MXLV_MTR_DERIVATIVE_GAIN:
		long_value = mx_round( motor->derivative_gain );

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 31,
					MXFT_LONG, &long_value, PMAC_DEBUG );
		break;
	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		long_value = mx_round( motor->velocity_feedforward_gain );

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 32,
					MXFT_LONG, &long_value, PMAC_DEBUG );
		break;
	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		long_value = mx_round( motor->acceleration_feedforward_gain );

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 35,
					MXFT_LONG, &long_value, PMAC_DEBUG );
		break;
	case MXLV_MTR_INTEGRAL_LIMIT:
		long_value = mx_round( motor->integral_limit );

		if ( long_value ) {
			long_value = 1;
		}

		mx_status = mxd_pmac_set_motor_variable( pmac_motor, 34,
					MXFT_LONG, &long_value, PMAC_DEBUG );
		break;
	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_simultaneous_start( long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				unsigned long flags )
{
	static const char fname[] = "mxd_pmac_simultaneous_start()";

	MX_RECORD *pmac_interface_record = NULL;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	MX_PMAC_MOTOR *pmac_motor = NULL;
	MX_PMAC *pmac = NULL;
	char command_buffer[500];
	char *ptr;
	double raw_position;
	long i, raw_steps;
	size_t length, buffer_left;
	mx_status_type mx_status;

	if ( num_motor_records <= 0 )
		return MX_SUCCESSFUL_RESULT;

	/* Construct the move command to send to the PMAC. */

	ptr = command_buffer;

	*ptr = '\0';

	for ( i = 0; i < num_motor_records; i++ ) {
		motor_record = motor_record_array[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor_record->mx_type != MXT_MTR_PMAC ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same type of motors.",
				motor_record_array[0]->name,
				motor_record->name );
		}

		pmac_motor = (MX_PMAC_MOTOR *) motor_record->record_type_struct;

		if ( pmac_interface_record == (MX_RECORD *) NULL ) {
			pmac_interface_record = pmac_motor->pmac_record;

			pmac = (MX_PMAC *)
				pmac_interface_record->record_type_struct;
		}

		/* Verify that the PMAC motor records all belong to the same
		 * PMAC interface.
		 */

		if ( pmac_interface_record != pmac_motor->pmac_record ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different PMAC interfaces, "
		"namely '%s' and '%s'.",
				motor_record_array[0]->name,
				motor_record->name,
				pmac_interface_record->name,
				pmac_motor->pmac_record->name );
		}

		/* Compute the new position in raw units. */

		raw_position =
			mx_divide_safely( position_array[i] - motor->offset,
						motor->scale );

		raw_steps = mx_round( raw_position );

		/* Append the part of the command referring to this motor. */

		if ( pmac->num_cards > 1 ) {

			length = strlen( command_buffer );
			buffer_left = sizeof(command_buffer) - length;
			ptr = command_buffer + length;

			snprintf( ptr, buffer_left,
				"@%lx", pmac_motor->card_number );
		}

		length = strlen( command_buffer );
		buffer_left = sizeof(command_buffer) - length;
		ptr = command_buffer + length;

		snprintf( ptr, buffer_left,
			"#%ldJ=%ld ", pmac_motor->motor_number, raw_steps );
	}

	if ( pmac_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"No pmac interface record pointer was found for record '%s'.",
		motor_record_array[0]->name );
	}

	/* Send the command to the PMAC. */

	MX_DEBUG( 2,("%s: command = '%s'", fname, command_buffer));

	mx_status = mxi_pmac_command( pmac, command_buffer,
						NULL, 0, PMAC_DEBUG );

	return mx_status;
}

#if HAVE_POWER_PMAC

static mx_status_type
mxd_pmac_get_power_pmac_status( MX_MOTOR *motor, MX_PMAC_MOTOR *pmac_motor )
{
	static const char fname[] = "mxd_pmac_get_power_pmac_status()";

	char response[50];
	unsigned long status[ MX_POWER_PMAC_NUM_STATUS_CHARACTERS ];
	int i, length;
	mx_status_type mx_status;

	/* Request the motor status. */

	mx_status = mxd_pmac_jog_command( pmac_motor, "?",
				response, sizeof response, PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = (int) strlen( response );

	if ( length < MX_POWER_PMAC_NUM_STATUS_CHARACTERS ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Too few characters seen in response to motor status command.  "
	"Only saw %d characters.", length );
	}

	/* Change the reported motor status from ASCII characters
	 * to unsigned long integers.
	 */

	for ( i = 0; i < MX_POWER_PMAC_NUM_STATUS_CHARACTERS; i++ ) {
		status[i] = mx_hex_char_to_unsigned_long( response[i] );

		MX_DEBUG( 2,("%s: status[%d] = %#lx",
			fname, i, status[i]));
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

#endif /* HAVE_POWER_PMAC */

static mx_status_type
mxd_pmac_get_turbo_pmac_status( MX_MOTOR *motor, MX_PMAC_MOTOR *pmac_motor )
{
	static const char fname[] = "mxd_pmac_get_turbo_pmac_status()";

	char response[50];
	unsigned long status[ MX_PMAC_NUM_STATUS_CHARACTERS ];
	int i, length;
	mx_status_type mx_status;

	/* Request the motor status. */

	mx_status = mxd_pmac_jog_command( pmac_motor, "?",
				response, sizeof response, PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = (int) strlen( response );

	if ( length < MX_PMAC_NUM_STATUS_CHARACTERS ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Too few characters seen in response to motor status command.  "
	"Only saw %d characters.", length );
	}

	/* Change the reported motor status from ASCII characters
	 * to unsigned long integers.
	 */

	for ( i = 0; i < MX_PMAC_NUM_STATUS_CHARACTERS; i++ ) {
		status[i] = mx_hex_char_to_unsigned_long( response[i] );

		MX_DEBUG( 2,("%s: status[%d] = %#lx",
			fname, i, status[i]));
	}
	
	/* Check for all the status bits that we are interested in.
	 *
	 * I used the Delta Tau Turbo PMAC Software Reference manual
	 * version 1.937 for the following information.  (W. Lavender)
	 */

	motor->status = 0;

	/* ============= First word returned. ============= */

	/* ----------- First character returned. ---------- */

	/* Bit 23: Motor Activated */

	if ( ( status[0] & 0x8 ) == 0 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/* Bit 22: Negative End Limit Set */

	if ( status[0] & 0x4 ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 21: Positive End Limit Set */

	if ( status[0] & 0x2 ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* Bit 20: (ignored) */

	/* ----------- Second character returned. ---------- */

	/* Bit 19: Amplifier Enabled */

	if ( ( status[1] & 0x8 ) == 0 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/* Bit 18: Open Loop Mode */

	if ( status[1] & 0x4 ) {
		motor->status |= MXSF_MTR_OPEN_LOOP;
	}

	/* Bit 17: Running Definite-Time Move. */

	if ( status[1] & 0x2 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* Bit 16: (ignored) */

	/* ----------- Third character returned. ---------- */

	/* Bits 15 and 14: (ignored) */

	/* Bit 13: Desired Velocity Zero (ignored) */

	/* If the Desired Velocity Zero bit is zero, then the In Position
	 * bit will be zero too, so there is no advantage that I can see
	 * to looking at this bit.  (W. Lavender)
	 */

	/* Bit 12: (ignored) */

	/* ----------- Fourth character returned. ---------- */

	/* Bits 11 to 8: (ignored) */

	/* ----------- Fifth character returned. ---------- */

	/* Bits 7 to 4: (ignored) */

	/* ----------- Sixth character returned. ---------- */

	/* Bits 3 to 0: (ignored) */

	/* ============= Second word returned. ============= */

	/* ----------- Seventh character returned. ---------- */

	/* Bits 23 to 20: (ignored) */

	/* ----------- Eighth character returned. ---------- */

	/* Bits 19 to 16: (ignored) */

	/* ----------- Ninth character returned. ---------- */

	/* Bits 15 to 12: (ignored) */

	/* ----------- Tenth character returned. ---------- */

	/* Bit 11: Stopped on Position Limit (ignored) */

	/* This bit does not tell you which limit was tripped,
	 * so the only thing you could do with it would be to
	 * set both the negative and positive bits, which defeats
	 * the purpose of having separate bits.  Thus, for now
	 * I ignore this bit and hope that using the limit
	 * specific bits from the first character returned are
	 * enough.  (W. Lavender)
	 */

	/* Bit 10: Home Complete */

	if ( status[9] & 0x4 ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	/* Bit 9: (ignored) */

	/* Bit 8: Phasing Reference Error */

	if ( status[9] & 0x1 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/* ----------- Eleventh character returned. ---------- */

	/* Bit 7: (ignored) */

	/* Bit 6: Integrated Fatal Following Error */

	if ( status[10] & 0x4 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 5: I^2T Amplifier Fault Error */

	if ( status[10] & 0x2 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/* Bit 4: (ignored) */

	/* ----------- Twelfth character returned. ---------- */

	/* Bit 3: Amplifier Fault Error */

	if ( status[11] & 0x8 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/* Bit 2: Fatal Following Error */

	if ( status[11] & 0x4 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 1: Warning Following Error (ignore) */

	/* Bit 0: In Position */

	if ( motor->status & MXSF_MTR_IS_BUSY ) {

		/* We have already determined that the motor is moving,
		 * so we do not need to look at the In Position flag.
		 */

	} else if ( status[11] & 0x1 ) {

		/* If In Position is set, then no move is in progress. */

		motor->status &= ( ~ MXSF_MTR_IS_BUSY );
	} else {
		/* If In Position is _not_ set, but an error flag is set,
		 * then no move is in progress.
		 */

		if ( motor->status & MXSF_MTR_FOLLOWING_ERROR ) {

			motor->status &= ( ~ MXSF_MTR_IS_BUSY );
		} else
		if ( motor->status & MXSF_MTR_DRIVE_FAULT ) {

			motor->status &= ( ~ MXSF_MTR_IS_BUSY );
		} else
		if ( motor->status & MXSF_MTR_AXIS_DISABLED ) {

			motor->status &= ( ~ MXSF_MTR_IS_BUSY );
		} else
		if ( motor->status & MXSF_MTR_OPEN_LOOP ) {

			motor->status &= ( ~ MXSF_MTR_IS_BUSY );
		} else {
			/* If we get to here, then we declare a move
			 * to be in progress.
			 */

			motor->status |= MXSF_MTR_IS_BUSY;
		}
	}

#if 0
	MX_DEBUG(-2,("%s: motor->status = %#lx", fname, motor->status));

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

MX_EXPORT mx_status_type
mxd_pmac_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_get_status()";

	MX_PMAC_MOTOR *pmac_motor;
	MX_PMAC *pmac;
	mx_status_type mx_status;

	mx_status = mxd_pmac_get_pointers( motor, &pmac_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_motor_get_pmac_pointer( pmac_motor, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( pmac->pmac_type ) {

#if HAVE_POWER_PMAC
	case MX_PMAC_TYPE_POWERPMAC:
		mx_status = mxd_pmac_get_power_pmac_status( motor, pmac_motor );
		break;
#endif /* HAVE_POWER_PMAC */

	default:
		mx_status = mxd_pmac_get_turbo_pmac_status( motor, pmac_motor );
		break;
	}

	return mx_status;
}

/*-----------*/

MX_EXPORT mx_status_type
mxd_pmac_jog_command( MX_PMAC_MOTOR *pmac_motor,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_pmac_jog_command()";

	MX_PMAC *pmac = NULL;
	char command_buffer[100];
	char *ptr;
	size_t length, buffer_left;
	mx_status_type mx_status;

	mx_status = mxd_pmac_motor_get_pmac_pointer( pmac_motor, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command_buffer, sizeof(command_buffer),
				"@%lx", pmac_motor->card_number );
	} else {
		strlcpy( command_buffer, "", sizeof(command_buffer) );
	}

	length = strlen( command_buffer );
	buffer_left = sizeof(command_buffer) - length;
	ptr = command_buffer + length;

	snprintf( ptr, buffer_left,
			"#%ld%s", pmac_motor->motor_number, command );

	mx_status = mxi_pmac_command( pmac, command_buffer,
				response, response_buffer_length, debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_get_motor_variable( MX_PMAC_MOTOR *pmac_motor,
				long variable_number,
				long variable_type,
				void *variable_ptr,
				int debug_flag )
{
	static const char fname[] = "mxd_pmac_get_motor_variable()";

	MX_PMAC *pmac = NULL;
	char command_buffer[100];
	char response[100];
	char *ptr;
	int num_items;
	long long_value;
	long *long_ptr;
	double double_value;
	double *double_ptr;
	size_t length, buffer_left;
	mx_status_type mx_status;

	mx_status = mxd_pmac_motor_get_pmac_pointer( pmac_motor, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command_buffer, sizeof(command_buffer),
			"@%lx", pmac_motor->card_number );
	} else {
		strlcpy( command_buffer, "", sizeof(command_buffer) );
	}

	length = strlen( command_buffer );
	buffer_left = sizeof(command_buffer) - length;
	ptr = command_buffer + length;

	if ( pmac->pmac_type & MX_PMAC_TYPE_TURBO ) {
		snprintf( ptr, buffer_left,
		    "I%02ld%02ld", pmac_motor->motor_number, variable_number );
	} else {
		snprintf( ptr, buffer_left,
		    "I%ld%02ld", pmac_motor->motor_number, variable_number );
	}

	mx_status = mxi_pmac_command( pmac, command_buffer,
				response, sizeof response, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( variable_type ) {
	case MXFT_LONG:
		num_items = sscanf( response, "%ld", &long_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot parse response to command '%s' as a long integer.  Response = '%s'",
				command_buffer, response );
		}
		long_ptr = (long *) variable_ptr;

		*long_ptr = long_value;
		break;
	case MXFT_DOUBLE:
		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Cannot parse response to command '%s' as a double.  Response = '%s'",
				command_buffer, response );
		}
		double_ptr = (double *) variable_ptr;

		*double_ptr = double_value;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG and MXFT_DOUBLE are supported." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_set_motor_variable( MX_PMAC_MOTOR *pmac_motor,
				long variable_number,
				long variable_type,
				void *variable_ptr,
				int debug_flag )
{
	static const char fname[] = "mxd_pmac_set_motor_variable()";

	MX_PMAC *pmac = NULL;
	char command_buffer[100];
	char response[100];
	char *ptr;
	long *long_ptr;
	double *double_ptr;
	size_t length, buffer_left;
	mx_status_type mx_status;

	mx_status = mxd_pmac_motor_get_pmac_pointer( pmac_motor, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		snprintf( command_buffer, sizeof(command_buffer),
				"@%lx", pmac_motor->card_number );
	} else {
		strlcpy( command_buffer, "", sizeof(command_buffer) );
	}

	length = strlen( command_buffer );
	buffer_left = sizeof(command_buffer) - length;
	ptr = command_buffer + length;

	switch( variable_type ) {
	case MXFT_LONG:
		long_ptr = (long *) variable_ptr;

		if ( pmac->pmac_type & MX_PMAC_TYPE_TURBO ) {
			snprintf( ptr, buffer_left,
				"I%02ld%02ld=%ld",
				pmac_motor->motor_number,
				variable_number,
				*long_ptr );
		} else {
			snprintf( ptr, buffer_left,
				"I%ld%02ld=%ld",
				pmac_motor->motor_number,
				variable_number,
				*long_ptr );
		}
		break;
	case MXFT_DOUBLE:
		double_ptr = (double *) variable_ptr;

		if ( pmac->pmac_type & MX_PMAC_TYPE_TURBO ) {
			snprintf( ptr, buffer_left,
				"I%02ld%02ld=%f",
				pmac_motor->motor_number,
				variable_number,
				*double_ptr );
		} else {
			snprintf( ptr, buffer_left,
				"I%ld%02ld=%f",
				pmac_motor->motor_number,
				variable_number,
				*double_ptr );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG and MXFT_DOUBLE are supported." );
	}

	mx_status = mxi_pmac_command( pmac, command_buffer,
				response, sizeof response, debug_flag );

	return mx_status;
}

