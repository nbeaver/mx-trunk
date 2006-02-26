/*
 * Name:    d_kohzu_sc.c
 *
 * Purpose: MX driver for the SC-200, SC-400, and SC-800 motor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KOHZU_SC_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "i_kohzu_sc.h"
#include "d_kohzu_sc.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_kohzu_sc_record_function_list = {
	NULL,
	mxd_kohzu_sc_create_record_structures,
	mxd_kohzu_sc_finish_record_initialization,
	NULL,
	mxd_kohzu_sc_print_structure,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_kohzu_sc_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_kohzu_sc_motor_function_list = {
	NULL,
	mxd_kohzu_sc_move_absolute,
	mxd_kohzu_sc_get_position,
	mxd_kohzu_sc_set_position,
	mxd_kohzu_sc_soft_abort,
	mxd_kohzu_sc_immediate_abort,
	NULL,
	NULL,
	mxd_kohzu_sc_find_home_position,
	mxd_kohzu_sc_constant_velocity_move,
	mxd_kohzu_sc_get_parameter,
	mxd_kohzu_sc_set_parameter,
	mxd_kohzu_sc_simultaneous_start,
	mxd_kohzu_sc_get_status
};

/* Compumotor motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_kohzu_sc_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_KOHZU_SC_MOTOR_STANDARD_FIELDS
};

long mxd_kohzu_sc_num_record_fields
		= sizeof( mxd_kohzu_sc_record_field_defaults )
			/ sizeof( mxd_kohzu_sc_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_kohzu_sc_rfield_def_ptr
			= &mxd_kohzu_sc_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_kohzu_sc_get_pointers( MX_MOTOR *motor,
			MX_KOHZU_SC_MOTOR **kohzu_sc_motor,
			MX_KOHZU_SC **kohzu_sc,
			const char *calling_fname )
{
	static const char fname[] = "mxd_kohzu_sc_get_pointers()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor_pointer;
	MX_RECORD *kohzu_sc_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	kohzu_sc_motor_pointer = (MX_KOHZU_SC_MOTOR *)
				motor->record->record_type_struct;

	if ( kohzu_sc_motor_pointer == (MX_KOHZU_SC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KOHZU_SC_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( kohzu_sc_motor != (MX_KOHZU_SC_MOTOR **) NULL ) {
		*kohzu_sc_motor = kohzu_sc_motor_pointer;
	}

	if ( kohzu_sc != (MX_KOHZU_SC **) NULL ) {
		kohzu_sc_record = kohzu_sc_motor_pointer->kohzu_sc_record;

		if ( kohzu_sc_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The kohzu_sc_record pointer for record '%s' is NULL.",
				motor->record->name );
		}

		*kohzu_sc = (MX_KOHZU_SC *)
				kohzu_sc_record->record_type_struct;

		if ( *kohzu_sc == (MX_KOHZU_SC *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_KOHZU_SC pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_kohzu_sc_create_record_structures()";

	MX_MOTOR *motor;
	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	kohzu_sc_motor = (MX_KOHZU_SC_MOTOR *)
				malloc( sizeof(MX_KOHZU_SC_MOTOR) );

	if ( kohzu_sc_motor == (MX_KOHZU_SC_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_KOHZU_SC_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = kohzu_sc_motor;
	record->class_specific_function_list
				= &mxd_kohzu_sc_motor_function_list;

	motor->record = record;

	/* A Kohzu SC-x00 motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The Kohzu SC-x00 reports acceleration times in seconds. */

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_kohzu_sc_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_KOHZU_SC *kohzu_sc;
	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	int i;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Add the motor record to the interface record's motor_array. */

	i = kohzu_sc_motor->axis_number - 1;

	kohzu_sc->motor_array[i] = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_kohzu_sc_print_structure()";

	MX_MOTOR *motor;
	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = KOHZU_SC_MOTOR motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  controller name   = %s\n",
					kohzu_sc->record->name);
	fprintf(file, "  axis number       = %ld\n",
					kohzu_sc_motor->axis_number);
	fprintf(file, "  kohzu_sc_flags    = %#lx\n",
					kohzu_sc_motor->kohzu_sc_flags);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			motor->raw_position.stepper );
	fprintf(file, "  scale             = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit    = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit    = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband     = %g %s  (%ld steps)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  speed             = %g %s/s  (%g steps/s)\n",
		speed, motor->units,
		motor->raw_speed );

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_kohzu_sc_resynchronize()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	kohzu_sc_motor = (MX_KOHZU_SC_MOTOR *) record->record_type_struct;

	if ( kohzu_sc_motor == (MX_KOHZU_SC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_KOHZU_SC_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxi_kohzu_sc_resynchronize(
			kohzu_sc_motor->kohzu_sc_record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_kohzu_sc_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_move_absolute()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"APS%ld/2/0/0/%ld/0/0/1",
			kohzu_sc_motor->axis_number,
			motor->raw_destination.stepper );

	mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
					NULL, 0, KOHZU_SC_MOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_get_position()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char response[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char token[80];
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"RDP%ld/0", kohzu_sc_motor->axis_number );

	mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
			response, sizeof response, KOHZU_SC_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_kohzu_sc_select_token( response, 2,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.stepper = mx_string_to_long( token );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_set_position()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[100];
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "WRP%ld/%ld",
				kohzu_sc_motor->axis_number,
				motor->raw_set_position.stepper );

	mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
			NULL, 0, KOHZU_SC_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_soft_abort()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"STP%ld/0", kohzu_sc_motor->axis_number );

	mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
					NULL, 0, KOHZU_SC_MOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_immediate_abort()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"STP%ld/1", kohzu_sc_motor->axis_number );

	mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
					NULL, 0, KOHZU_SC_MOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_find_home_position()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"ORG%ld/2/0/0/3/1", kohzu_sc_motor->axis_number );

	mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
					NULL, 0, KOHZU_SC_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_constant_velocity_move()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	int rotational_direction;
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the direction of the move. */

	if ( motor->constant_velocity_move >= 0 ) {
		rotational_direction = 1;
	} else {
		rotational_direction = 0;
	}

	snprintf( command, sizeof(command),
			"FRP%ld/2/0/0/%d/1",
			kohzu_sc_motor->axis_number,
			rotational_direction );
		
	mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
					NULL, 0, KOHZU_SC_MOTOR_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_get_parameter()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char response[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char token[80];
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if KOHZU_SC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		/* We get the speed, base speed, and acceleration time
		 * all with one command.
		 */

		snprintf( command, sizeof(command),
			"RMS%ld", kohzu_sc_motor->axis_number );

		mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
				response, sizeof(response),
				KOHZU_SC_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Start speed */

		mx_status = mxi_kohzu_sc_select_token( response, 2,
							token, sizeof(token) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_base_speed = atof( token );

		/* Maximum speed */

		mx_status = mxi_kohzu_sc_select_token( response, 3,
							token, sizeof(token) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_speed = atof( token );

		/* Accelerating time */

		mx_status = mxi_kohzu_sc_select_token( response, 15,
							token, sizeof(token) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_acceleration_parameters[0] = 0.01 * atof( token );

		/* Decelerating time */

		mx_status = mxi_kohzu_sc_select_token( response, 16,
							token, sizeof(token) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_acceleration_parameters[1] = 0.01 * atof( token );

		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;

#if KOHZU_SC_MOTOR_DEBUG
		MX_DEBUG(-2,("%s: motor '%s', raw_base_speed = %g",
			fname, motor->record->name, motor->raw_base_speed));
		MX_DEBUG(-2,("%s: motor '%s', raw_speed = %g",
			fname, motor->record->name, motor->raw_speed));
		MX_DEBUG(-2,
		("%s: motor '%s', raw_acceleration_parameters[0] = %g",
			fname, motor->record->name,
			motor->raw_acceleration_parameters[0]));
		MX_DEBUG(-2,
		("%s: motor '%s', raw_acceleration_parameters[1] = %g",
			fname, motor->record->name,
			motor->raw_acceleration_parameters[1]));
#endif
		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		/* As documented in the manual. */

		motor->raw_maximum_speed = 4095500;  /* steps per second */
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_set_parameter()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	unsigned long kohzu_base_speed, kohzu_speed, kohzu_acceleration_time;
	int excitation_output;
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if KOHZU_SC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		/* Speed, base speed, and acceleration time are all set
		 * by one big command.
		 */

		kohzu_base_speed = mx_round( fabs( motor->raw_base_speed ) );
		kohzu_speed = mx_round( fabs( motor->raw_speed ) );

		kohzu_acceleration_time = mx_round(
			100.0 * fabs( motor->raw_acceleration_parameters[0] ) );

		if ( kohzu_base_speed < 1 )
			kohzu_base_speed = 1;

		if ( kohzu_base_speed > 4095500 )
			kohzu_base_speed = 4095500;

		if ( kohzu_speed < 1 )
			kohzu_speed = 1;

		if ( kohzu_speed > 4095500 )
			kohzu_speed = 4095500;

		if ( kohzu_acceleration_time < 1 )
			kohzu_acceleration_time = 1;

		if ( kohzu_acceleration_time > 1000000 )
			kohzu_acceleration_time = 1000000;

		snprintf( command, sizeof(command),
				"ASI%ld/%lu/%lu/%lu/%lu/0/0/0/1/1/0/0/0/0",
				kohzu_sc_motor->axis_number,
				kohzu_base_speed,
				kohzu_speed,
				kohzu_acceleration_time,
				kohzu_acceleration_time );
		
		mx_status = mxi_kohzu_sc_command( kohzu_sc,
				command, NULL, 0, KOHZU_SC_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		motor->raw_maximum_speed = 4095500;  /* steps per second */

		return MX_SUCCESSFUL_RESULT;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = 1;
			excitation_output = 0;
		} else {
			excitation_output = 1;
		}

		snprintf( command, sizeof(command),
				"COF%ld/%d",
				kohzu_sc_motor->axis_number,
				excitation_output );

		mx_status = mxi_kohzu_sc_command( kohzu_sc,
				command, NULL, 0, KOHZU_SC_MOTOR_DEBUG );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_simultaneous_start( long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				int flags )
{
	static const char fname[] = "mxd_kohzu_sc_simultaneous_start()";

	MX_RECORD *motor_record, *current_interface_record;
	MX_MOTOR *motor;
	MX_KOHZU_SC *kohzu_sc;
	MX_KOHZU_SC *current_kohzu_sc;
	MX_KOHZU_SC_MOTOR *current_kohzu_sc_motor;
	int i;

	mx_status_type mx_status;

	kohzu_sc = NULL;

	for ( i = 0; i < num_motor_records; i++ ) {
		motor_record = motor_record_array[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor_record->mx_type != MXT_MTR_KOHZU_SC ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same type of motors.",
				motor_record_array[0]->name,
				motor_record->name );
		}

		current_kohzu_sc_motor = (MX_KOHZU_SC_MOTOR *)
					motor_record->record_type_struct;

		current_interface_record
			= current_kohzu_sc_motor->kohzu_sc_record;

		current_kohzu_sc = (MX_KOHZU_SC *)
			current_interface_record->record_type_struct;

		if ( kohzu_sc == (MX_KOHZU_SC *) NULL )
		{
			kohzu_sc = current_kohzu_sc;
		}

		if ( kohzu_sc != current_kohzu_sc ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different Kohzu SC-x00 "
		"interfaces, namely '%s' and '%s'.",
				motor_record_array[0]->name,
				motor_record->name,
				kohzu_sc->record->name,
				current_interface_record->name );
		}
	}

	/* Start the move. */

	mx_status = mxi_kohzu_sc_multiaxis_move( kohzu_sc,
					num_motor_records,
					motor_record_array,
					position_array,
					TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_kohzu_sc_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_kohzu_sc_get_status()";

	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	MX_KOHZU_SC *kohzu_sc;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char response[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char token[80];
	unsigned long driving_operation, org_signal;
	unsigned long cw_limit_signal, ccw_limit_signal, error_signal;
	mx_status_type mx_status;

	mx_status = mxd_kohzu_sc_get_pointers( motor, &kohzu_sc_motor,
						&kohzu_sc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"STR1/%ld", kohzu_sc_motor->axis_number );

	mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
			response, sizeof response, KOHZU_SC_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*----*/

	mx_status = mxi_kohzu_sc_select_token( response, 3,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	driving_operation = mx_string_to_unsigned_long( token );

	/*----*/

	mx_status = mxi_kohzu_sc_select_token( response, 5,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	org_signal = mx_string_to_unsigned_long( token );

	/*----*/

	mx_status = mxi_kohzu_sc_select_token( response, 6,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cw_limit_signal = mx_string_to_unsigned_long( token );

	/*----*/

	mx_status = mxi_kohzu_sc_select_token( response, 7,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ccw_limit_signal = mx_string_to_unsigned_long( token );

	/*----*/

	mx_status = mxi_kohzu_sc_select_token( response, 9,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	error_signal = mx_string_to_unsigned_long( token );

	/*----*/

	/* Check all of the status bits that we are interested in. */

	motor->status = 0;

	if ( driving_operation ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	if ( org_signal ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	if ( cw_limit_signal ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	if ( ccw_limit_signal ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

#if KOHZU_SC_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: motor '%s' driving_operation = %lu",
		fname, motor->record->name, driving_operation));
	MX_DEBUG(-2,("%s: motor '%s' org_signal = %lu",
		fname, motor->record->name, org_signal));
	MX_DEBUG(-2,("%s: motor '%s' cw_limit_signal = %lu",
		fname, motor->record->name, cw_limit_signal));
	MX_DEBUG(-2,("%s: motor '%s' ccw_limit_signal = %lu",
		fname, motor->record->name, ccw_limit_signal));
	MX_DEBUG(-2,("%s: motor '%s' error_signal = %lu",
		fname, motor->record->name, error_signal));
	MX_DEBUG(-2,("%s: motor->status = %#lx", fname, motor->status));
#endif

	return MX_SUCCESSFUL_RESULT;
}

