/*
 * Name:    d_mdrive.c 
 *
 * Purpose: MX motor driver to control Intelligent Motion Systems MDrive
 *          motor controllers.
 *
 * WARNING: At present, this driver assumes that you have a multidrop
 *          system that you are using via Party Mode.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MDRIVE_DEBUG	FALSE

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
#include "d_mdrive.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mdrive_record_function_list = {
	NULL,
	mxd_mdrive_create_record_structures,
	mxd_mdrive_finish_record_initialization,
	NULL,
	mxd_mdrive_print_motor_structure,
	NULL,
	NULL,
	mxd_mdrive_open,
	NULL,
	NULL,
	mxd_mdrive_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_mdrive_motor_function_list = {
	NULL,
	mxd_mdrive_move_absolute,
	mxd_mdrive_get_position,
	mxd_mdrive_set_position,
	mxd_mdrive_soft_abort,
	mxd_mdrive_immediate_abort,
	NULL,
	NULL,
	mxd_mdrive_find_home_position,
	mxd_mdrive_constant_velocity_move,
	mxd_mdrive_get_parameter,
	mxd_mdrive_set_parameter,
	NULL,
	mxd_mdrive_get_status
};

/* MDrive MDRIVE motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mdrive_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_MDRIVE_STANDARD_FIELDS
};

mx_length_type mxd_mdrive_num_record_fields
		= sizeof( mxd_mdrive_record_field_defaults )
			/ sizeof( mxd_mdrive_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mdrive_rfield_def_ptr
			= &mxd_mdrive_record_field_defaults[0];

/* Private functions for the use of the driver. */

static mx_status_type
mxd_mdrive_get_pointers( MX_MOTOR *motor,
			MX_MDRIVE **mdrive,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mdrive_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mdrive == (MX_MDRIVE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MDRIVE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mdrive = (MX_MDRIVE *) (motor->record->record_type_struct);

	if ( *mdrive == (MX_MDRIVE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MDRIVE pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_mdrive_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_mdrive_create_record_structures()";

	MX_MOTOR *motor;
	MX_MDRIVE *mdrive;
	int i;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for an MX_MOTOR structure for record '%s'.",
			record->name );
	}

	mdrive = (MX_MDRIVE *) malloc( sizeof(MX_MDRIVE) );

	if ( mdrive == (MX_MDRIVE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for an MX_MDRIVE structure for record '%s'.",
			record->name );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = mdrive;
	record->class_specific_function_list
				= &mxd_mdrive_motor_function_list;

	motor->record = record;
	mdrive->record = record;

	/* An MDrive motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The MDRIVE reports accelerations in steps/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	for ( i = 0; i < MXD_MDRIVE_NUM_IO_POINTS; i++ ) {
		mdrive->io_setup[i] = MXF_MDRIVE_UNKNOWN;
	}

	mdrive->negative_limit_switch = 0;
	mdrive->positive_limit_switch = 0;
	mdrive->home_switch = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mdrive_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mdrive_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_mdrive_print_motor_structure()";

	MX_MOTOR *motor;
	MX_MDRIVE *mdrive;
	double position, move_deadband;
	int i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type            = MDRIVE\n\n");
	fprintf(file, "  name                  = %s\n", record->name);
	fprintf(file, "  rs232                 = %s\n",
						mdrive->rs232_record->name);

	fprintf(file, "  axis name             = %d\n", mdrive->axis_name);
	fprintf(file, "  I/O setup             = %d",
						mdrive->io_setup[0]);

	for ( i = 1; i <= 4; i++ ) {
		fprintf(file, " %d", mdrive->io_setup[i]);
	}
	fprintf(file, "\n");

	fprintf(file, "  negative limit switch = %d\n",
						mdrive->negative_limit_switch);
	fprintf(file, "  positive limit switch = %d\n",
						mdrive->positive_limit_switch);
	fprintf(file, "  home switch           = %d\n",
						mdrive->home_switch);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read the position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position            = %g %s  (%ld steps)\n",
		motor->position, motor->units,
		(long) motor->raw_position.stepper );
	fprintf(file, "  scale               = %g %s per step.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset              = %g %s.\n",
		motor->offset, motor->units);

	fprintf(file, "  backlash            = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		(long) motor->raw_backlash_correction.stepper);

	fprintf(file, "  negative limit      = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		(long) motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit      = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		(long) motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband       = %g %s  (%ld steps)\n\n",
		move_deadband, motor->units,
		(long) motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mdrive_get_io_config( MX_MDRIVE *mdrive, int io_point_number )
{
	static const char fname[] = "mxd_mdrive_get_io_config()";

	char command[20];
	char response[80];
	int i, num_items;
	mx_status_type mx_status;

	if ( mdrive == (MX_MDRIVE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MDRIVE pointer passed was NULL." );
	}

	if ( (io_point_number < 1) || (io_point_number > 5) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
 "I/O point number %d for MDrive '%s' is outside the allowed range of 1 to 5.",
			io_point_number, mdrive->record->name );
	}

	sprintf( command, "PR S%d", io_point_number );

	mx_status = mxd_mdrive_command( mdrive, command,
			response, sizeof response, MXD_MDRIVE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	i = io_point_number - 1;

	num_items = sscanf( response, "%d", &(mdrive->io_setup[i]) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Could not find the setup value for I/O point %d of MDrive '%s' "
	"in the response to the command '%s'.  Response = '%s'",
			io_point_number, mdrive->record->name,
			command, response );
	}

	switch( mdrive->io_setup[i] ) {
	case MXF_MDRIVE_IN_HOME:
		mdrive->home_switch = io_point_number;
		break;
	case MXF_MDRIVE_IN_LIMIT_PLUS:
		mdrive->positive_limit_switch = io_point_number;
		break;
	case MXF_MDRIVE_IN_LIMIT_MINUS:
		mdrive->negative_limit_switch = io_point_number;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mdrive_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mdrive_open()";

	MX_MOTOR *motor;
	MX_MDRIVE *mdrive;
	MX_RS232 *rs232;
	int i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify the line terminators. */

	mx_status = mx_rs232_verify_configuration( mdrive->rs232_record,
			(long) MXF_232_DONT_CARE, (int) MXF_232_DONT_CARE,
			(char) MXF_232_DONT_CARE, (int) MXF_232_DONT_CARE,
			(char) MXF_232_DONT_CARE,
			0x0d0a, 0x0a );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232 = (MX_RS232 *) mdrive->rs232_record->record_class_struct;

	mdrive->num_write_terminator_chars = rs232->num_write_terminator_chars;

	mx_status = mxd_mdrive_resynchronize( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out how each I/O point is configured. */

	for ( i = 1; i <= MXD_MDRIVE_NUM_IO_POINTS; i++ ) {
		mx_status = mxd_mdrive_get_io_config( mdrive, i );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mdrive_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_mdrive_resynchronize()";

	MX_MOTOR *motor;
	MX_MDRIVE *mdrive;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the MDrive is in party line mode by sending two <LF>s. */

	mx_status = mx_rs232_putchar(mdrive->rs232_record, MX_LF, MXF_232_WAIT);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putchar(mdrive->rs232_record, MX_LF, MXF_232_WAIT);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Unconditionally turn off echoing of command inputs.
	 * This is done to eliminate the extra overhead of
	 * reading and discarding the echoed commands in the
	 * MX driver.  The leading <LF> character is there to
	 * ensure that the controller is in party line mode.
	 */

	sprintf( command, "%cEM=1", mdrive->axis_name );

	mx_status = mx_rs232_putline( mdrive->rs232_record, command,
					NULL, MXD_MDRIVE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read back at least one line of input. */

	mx_status = mx_rs232_getline( mdrive->rs232_record, 
			response, sizeof(response), NULL, MXD_MDRIVE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any unread RS-232 input. */

	mx_status = mx_rs232_discard_unread_input( mdrive->rs232_record,
							MXD_MDRIVE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the controller is there by asking for its
	 * firmware version.
	 */

	mx_status = mxd_mdrive_command( mdrive, "PR VR",
				response, sizeof(response), MXD_MDRIVE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reset the stall flag. */

	mx_status = mxd_mdrive_command( mdrive, "ST=0",
						NULL, 0, MXD_MDRIVE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reset the error flag. */

	mx_status = mxd_mdrive_command( mdrive, "ER=0",
						NULL, 0, MXD_MDRIVE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable the drive. */

	mx_status = mxd_mdrive_command( mdrive, "DE=1",
						NULL, 0, MXD_MDRIVE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mdrive_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_move_absolute()";

	MX_MDRIVE *mdrive;
	char command[20];
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_destination.stepper;

	/* Format the move command and send it. */

	sprintf( command, "MA %ld", motor_steps );

	mx_status = mxd_mdrive_command( mdrive, command,
						NULL, 0, MXD_MDRIVE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mdrive_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_get_position()";

	MX_MDRIVE *mdrive;
	char response[30];
	int num_items;
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mdrive_command( mdrive, "PR P",
			response, sizeof response, MXD_MDRIVE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &motor_steps );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Motor '%s' returned an unparseable position value '%s'.",
			motor->record->name, response );
	}

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mdrive_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_set_position()";

	MX_MDRIVE *mdrive;
	char command[40];
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_set_position.stepper;

	sprintf( command, "P=%ld", motor_steps );

	mx_status = mxd_mdrive_command( mdrive, command,
						NULL, 0, MXD_MDRIVE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mdrive_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_soft_abort()";

	MX_MDRIVE *mdrive;
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Command the MDrive to slew at zero velocity. */

	mx_status = mxd_mdrive_command( mdrive, "SL=0",
						NULL, 0, MXD_MDRIVE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mdrive_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_immediate_abort()";

	MX_MDRIVE *mdrive;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* From the manual: The ESCAPE key will stop the user program 
	 * and stop the motor with no decel rate.
	 */

	sprintf( command, "%c", MX_ESC );

	mx_status = mxd_mdrive_command( mdrive, command,
						NULL, 0, MXD_MDRIVE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mdrive_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_find_home_position()";

	MX_MDRIVE *mdrive;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->home_search >= 0 ) {
		strcpy( command, "HM=3" );
	} else {
		strcpy( command, "HM=1" );
	}
		
	mx_status = mxd_mdrive_command( mdrive, command,
						NULL, 0, MXD_MDRIVE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mdrive_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_constant_velocity_move()";

	MX_MDRIVE *mdrive;
	char command[20];
	double speed;
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_speed( motor->record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->constant_velocity_move >= 0 ) {
		sprintf( command, "SL=%ld",
				mx_round( motor->raw_speed ) );
	} else {
		sprintf( command, "SL=-%ld",
				mx_round( motor->raw_speed ) );
	}

	mx_status = mxd_mdrive_command( mdrive, command,
						NULL, 0, MXD_MDRIVE_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mdrive_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_get_parameter()";

	MX_MDRIVE *mdrive;
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mxd_mdrive_command( mdrive, "PR VM",
				response, sizeof(response), MXD_MDRIVE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &(motor->raw_speed) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to a 'PR VM' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;
	case MXLV_MTR_BASE_SPEED:
		mx_status = mxd_mdrive_command( mdrive, "PR VI",
				response, sizeof(response), MXD_MDRIVE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &(motor->raw_base_speed) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to a 'PR VI' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mxd_mdrive_command( mdrive, "PR A",
				response, sizeof(response), MXD_MDRIVE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
				&(motor->raw_acceleration_parameters[0]) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to a 'PR A' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		mx_status = mxd_mdrive_command( mdrive, "PR D",
				response, sizeof(response), MXD_MDRIVE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
				&(motor->raw_acceleration_parameters[1]) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to parse response to a 'PR D' command "
			"for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		motor->raw_maximum_speed = 5000000; /* steps per second */
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mdrive_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_set_parameter()";

	MX_MDRIVE *mdrive;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		sprintf( command, "VM=%ld", mx_round( motor->raw_speed ) );

		mx_status = mxd_mdrive_command( mdrive, command,
						NULL, 0, MXD_MDRIVE_DEBUG );
		break;

	case MXLV_MTR_BASE_SPEED:
		sprintf( command, "VI=%ld", mx_round( motor->raw_base_speed ) );

		mx_status = mxd_mdrive_command( mdrive, command,
						NULL, 0, MXD_MDRIVE_DEBUG );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		sprintf( command, "A=%ld",
			mx_round( motor->raw_acceleration_parameters[0] ));

		mx_status = mxd_mdrive_command( mdrive, command,
						NULL, 0, MXD_MDRIVE_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		sprintf( command, "D=%ld",
			mx_round( motor->raw_acceleration_parameters[1] ));

		mx_status = mxd_mdrive_command( mdrive, command,
						NULL, 0, MXD_MDRIVE_DEBUG );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
		break;
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mdrive_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mdrive_get_status()";

	MX_MDRIVE *mdrive;
	char command[80];
	char response[80];
	char *ptr;
	int i, num_chars_expected;
	mx_status_type mx_status;

	mx_status = mxd_mdrive_get_pointers( motor, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	strcpy( command, "PR MV ST" );

	ptr = command + strlen(command);

	num_chars_expected = 2;

	if ( mdrive->negative_limit_switch != 0 ) {
		sprintf( ptr, " I%d", mdrive->negative_limit_switch );

		ptr = command + strlen(command);

		num_chars_expected++;
	}

	if ( mdrive->positive_limit_switch != 0 ) {
		sprintf( ptr, " I%d", mdrive->positive_limit_switch );

		ptr = command + strlen(command);

		num_chars_expected++;
	}

	if ( mdrive->home_switch != 0 ) {
		sprintf( ptr, " I%d", mdrive->home_switch );

		ptr = command + strlen(command);

		num_chars_expected++;
	}

	MX_DEBUG( 2,("%s: num_chars_expected = %d", fname, num_chars_expected));

	/* Send the status request command. */

	mx_status = mxd_mdrive_command( mdrive, command,
			response, sizeof(response), MXD_MDRIVE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strlen( response ) != num_chars_expected ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to get the motion status from the response to "
		"a '%s' command for motor '%s'.  Response = '%s'",
			command, motor->record->name, response );
	}

	if ( response[0] == '1' ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	if ( response[1] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	i = 2;

	if ( mdrive->negative_limit_switch != 0 ) {

		if ( response[i] == '1' ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		}

		i++;
	}

	if ( mdrive->positive_limit_switch != 0 ) {

		if ( response[i] == '1' ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		}

		i++;
	}

	if ( mdrive->home_switch != 0 ) {

		if ( response[i] == '1' ) {
			motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
		}

		i++;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === Extra functions for the use of this driver. === */

MX_EXPORT mx_status_type
mxd_mdrive_command( MX_MDRIVE *mdrive, char *command,
			char *response, int response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_mdrive_command()";

	char prefix[20], local_response_buffer[100];
	char *discard_ptr;
	int command_length, prefix_length, num_chars_to_discard;
	int local_response_buffer_length, discard_buffer_length;
	int mdrive_error_code;
	long mx_error_code;
	const char *mdrive_error_message;
	
	mx_status_type mx_status;

	if ( mdrive == (MX_MDRIVE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MDRIVE pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'command' pointer passed was NULL." );
	}

	/* Send the axis name prefix. */

	sprintf( prefix, "%c", mdrive->axis_name );

	if ( debug_flag & MXD_MDRIVE_DEBUG ) {
		MX_DEBUG(-2, ("%s: command = '%s%s'", fname, prefix, command));
	}

	prefix_length = strlen( prefix );

	command_length = prefix_length + strlen( command );

#if 0
	/* The MDrive always echoes the transmitted command line
	 * _including_ the line terminators and then sends <CR><LF>
	 * before returning a response.  Figure out how many characters
	 * will need to be thrown away.
	 */

	num_chars_to_discard = command_length
				+ mdrive->num_write_terminator_chars + 2;
#else
	num_chars_to_discard = 2;
#endif

	local_response_buffer_length = sizeof( local_response_buffer ) - 1;

	if ( ( local_response_buffer_length < response_buffer_length )
	  && ( response != NULL ) )
	{
		discard_buffer_length = response_buffer_length;
		discard_ptr = response;
	} else {
		discard_buffer_length = local_response_buffer_length;
		discard_ptr = local_response_buffer;
	}

	if ( num_chars_to_discard > discard_buffer_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"***Programming bug*** "
	"The number of echoed characters to discard (%d) is greater than "
	"the largest available response buffer (%d).  Please increase the "
	"size of your response buffer.", num_chars_to_discard,
					discard_buffer_length );
	}

	/*** Send the command string to the MDrive. ***/

	MX_DEBUG( 2,("%s: *** sending command prefix '%s' ***",
			fname, prefix ));

	mx_status = mx_rs232_write( mdrive->rs232_record,
				prefix, prefix_length,
				NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: *** sending command '%s' ***", fname, command));

	mx_status = mx_rs232_putline( mdrive->rs232_record,
					command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Discard the echoed command line. ***/

	MX_DEBUG( 2,("%s: *** discarding %d echoed characters. ***",
				fname, num_chars_to_discard));

	mx_status = mx_rs232_read( mdrive->rs232_record, discard_ptr,
				num_chars_to_discard, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Get the response from the MDrive. ***/

	if ( response != NULL ) {

		/* Delete anything already in the buffer. */

		response[0] = '\0';

		MX_DEBUG( 2,("%s: *** reading response_line. ***", fname));

		mx_status = mx_rs232_getline( mdrive->rs232_record,
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
		if ( debug_flag & MXD_MDRIVE_DEBUG ) {
			MX_DEBUG(-2, ("%s: response = '%s'", fname, response ));
		}
	}

	MX_DEBUG( 2,
		("%s: *** starting to discard unread characters. ***",fname));

	mx_status = mx_rs232_discard_unread_input( mdrive->rs232_record,
							FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
		("%s: *** finished discarding unread characters. ***",fname));

	/* See if there were any errors in the last command. */

	if ( debug_flag & MXD_MDRIVE_SKIP_ERROR_STATUS ) {
		MX_DEBUG( 2,
			("%s: *** NOT checking for MDrive errors. ***", fname));

		return MX_SUCCESSFUL_RESULT;

	} else {
		MX_DEBUG( 2,
			("%s: *** checking for MDrive errors. ***", fname));

		mx_status = mxd_mdrive_get_error_code( mdrive,
					&mx_error_code,
					&mdrive_error_code,
					&mdrive_error_message );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mdrive_error_code == 0 ) {
			return MX_SUCCESSFUL_RESULT;
		}

		return mx_error( mx_error_code, fname,
		"An error occurred for the command '%s' sent to "
		"MDrive controller '%s'.  Error code = %d, "
		"error message = '%s'.",
		command, mdrive->record->name,
		mdrive_error_code, mdrive_error_message );
	}
}

/* === */

MX_EXPORT mx_status_type
mxd_mdrive_get_error_code( MX_MDRIVE *mdrive,
				long *mx_error_code,
				int *mdrive_error_code,
				const char **error_message )
{
	static const char fname[] = "mxd_mdrive_get_error_code()";

	static struct {
		const int mdrive_error_code;
		const long mx_error_code;
		const char error_message[80];
	} error_code_table[] = {
		{ 0,  MXE_SUCCESS, "No error" },
		{ 1,  MXE_HARDWARE_FAULT, "I/O1 Fault" },
		{ 2,  MXE_HARDWARE_FAULT, "I/O2 Fault" },
		{ 3,  MXE_HARDWARE_FAULT, "I/O3 Fault" },
		{ 4,  MXE_HARDWARE_FAULT, "I/O4 Fault" },
		{ 5,  MXE_HARDWARE_FAULT, "I/O5 Fault" },
		{ 6,  MXE_HARDWARE_CONFIGURATION_ERROR,
			"An I/O is already set to this type." },
		{ 7,  MXE_READ_ONLY, "Tried to set an Input or defined I/O." },
		{ 8,  MXE_ILLEGAL_ARGUMENT,
			"Tried to set an I/O to an incorrect I/O type." },
		{ 9,  MXE_READ_ONLY,
			"Tried to write to I/O set as input or is '\"TYPED\"."},
		{ 10, MXE_ILLEGAL_ARGUMENT, "Illegal I/O number." },

		{ 20, MXE_ILLEGAL_ARGUMENT,
			"Tried to set unknown variable or flag." },
		{ 21, MXE_ILLEGAL_ARGUMENT,
			"Tried to set an incorrect value." },
		{ 22, MXE_WOULD_EXCEED_LIMIT,
		  "VI (base speed) set greater than or equal to VM (speed)." },
		{ 23, MXE_WOULD_EXCEED_LIMIT,
		  "VM (speed) is set less than or equal to VI (base speed)." },
		{ 24, MXE_ILLEGAL_ARGUMENT, "Illegal data entered." },
		{ 25, MXE_READ_ONLY, "Variable or flag is read only." },
		{ 26, MXE_UNSUPPORTED,
	"Variable or flag is not allowed to be incremented or decremented." },
		{ 27, MXE_NOT_FOUND, "Trip not defined." },
		{ 28, MXE_UNSUPPORTED,
			"Trying to redefine a program label or variable." },
		{ 29, MXE_UNSUPPORTED,
			"Trying to redefine an embedded command or variable." },
		{ 30, MXE_NOT_FOUND, "Unknown label or user variable." },
		{ 31, MXE_OUT_OF_MEMORY,
			"Program label or user variable table is full." },
		{ 32, MXE_UNSUPPORTED, "Trying to set a label (LB)." },

		{ 40, MXE_NOT_READY, "Program not running." },
		{ 41, MXE_NOT_READY, "Program running." },
		{ 42, MXE_ILLEGAL_ARGUMENT, "Illegal program address." },
		{ 43, MXE_WOULD_EXCEED_LIMIT,
			"Tried to overflow program stack." },
		{ 44, MXE_PERMISSION_DENIED, "Program locked." },

		{ 60, MXE_ILLEGAL_ARGUMENT, "Tried to enter unknown command." },
		{ 61, MXE_HARDWARE_CONFIGURATION_ERROR,
			"Trying to set illegal BAUD rate." },

		{ 80, MXE_NOT_FOUND, "HOME switch not defined." },
		{ 81, MXE_NOT_FOUND, "HOME type not defined." },
		{ 82, MXE_DEVICE_ACTION_FAILED,
			"Went to both LIMITS and did not find home." },
		{ 83, MXE_INTERRUPTED, "Reached positive LIMIT switch." },
		{ 84, MXE_INTERRUPTED, "Reached negative LIMIT switch." },
		{ 85, MXE_NOT_READY,
    "MA (move absolute) or MR (move relative) not allowed while in motion." },
		{ 86, MXE_DEVICE_ACTION_FAILED, "Stall detected." },
	};

	static int num_error_codes = sizeof( error_code_table )
				/ sizeof( error_code_table[0] );

	char response[80];
	int i, num_items, debug_flag;
	mx_status_type mx_status;

	*mx_error_code = MXE_SUCCESS;
	*mdrive_error_code = 0;
	*error_message = NULL;

	/* The MXD_MDRIVE_SKIP_ERROR_STATUS flag bit must be set here.
	 * Otherwise, we would get an infinite recursion loop, where
	 * mxd_mdrive_command() recursively invoked itself over and over
	 * until the process was killed or the computer ran out of memory.
	 */

#if 0
	debug_flag = MXD_MDRIVE_DEBUG | MXD_MDRIVE_SKIP_ERROR_STATUS;
#else
	debug_flag = MXD_MDRIVE_SKIP_ERROR_STATUS;
#endif

	mx_status = mxd_mdrive_command( mdrive, "PR ER",
				response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", mdrive_error_code );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Motor '%s' returned an unparseable error status '%s'.",
			mdrive->record->name, response );
	}

	MX_DEBUG( 2,("%s: *mdrive_error_code = %d",
			fname, *mdrive_error_code));

	for ( i = 0; i < num_error_codes; i++ ) {
		if (error_code_table[i].mdrive_error_code == *mdrive_error_code)
			break;		/* Exit the for() loop. */
	}

	if ( i >= num_error_codes ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Illegal error code '%d' seen in response to 'PR ER' "
			"command for controller '%s'.  Response = '%s'",
			*mdrive_error_code, mdrive->record->name, response );
	}

	*mx_error_code = error_code_table[i].mx_error_code;

	*error_message = error_code_table[i].error_message;

	/* If set, reset the MDrive error code. */

	if ( *mdrive_error_code != 0 ) {
		(void) mxd_mdrive_command(mdrive, "ER=0", NULL, 0, debug_flag);
	}

	return MX_SUCCESSFUL_RESULT;
}

