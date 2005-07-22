/*
 * Name:    d_stp100.c 
 *
 * Purpose: MX motor driver to control Pontech STP100 motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define STP100_MOTOR_DEBUG		FALSE

#define STP100_MOTOR_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_unistd.h"

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_clock.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "d_stp100.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_stp100_motor_record_function_list = {
	NULL,
	mxd_stp100_motor_create_record_structures,
	mxd_stp100_motor_finish_record_initialization,
	NULL,
	mxd_stp100_motor_print_motor_structure,
	NULL,
	NULL,
	mxd_stp100_motor_open,
	NULL,
	NULL,
	mxd_stp100_motor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_stp100_motor_motor_function_list = {
	mxd_stp100_motor_is_busy,
	mxd_stp100_motor_move_absolute,
	mxd_stp100_motor_get_position,
	mxd_stp100_motor_set_position,
	mxd_stp100_motor_soft_abort,
	mxd_stp100_motor_immediate_abort,
	mxd_stp100_motor_positive_limit_hit,
	mxd_stp100_motor_negative_limit_hit,
	mxd_stp100_motor_find_home_position,
	mxd_stp100_motor_constant_velocity_move,
	mxd_stp100_motor_get_parameter,
	mxd_stp100_motor_set_parameter
};

/* Pontech STP100 motor controller data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_stp100_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_STP100_MOTOR_STANDARD_FIELDS
};

long mxd_stp100_motor_num_record_fields
		= sizeof( mxd_stp100_motor_record_field_defaults )
			/ sizeof( mxd_stp100_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_stp100_motor_rfield_def_ptr
			= &mxd_stp100_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_stp100_motor_get_pointers( MX_MOTOR *motor,
			MX_STP100_MOTOR **stp100_motor,
			const char *calling_fname )
{
	const char fname[] = "mxd_stp100_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( stp100_motor == (MX_STP100_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_STP100_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*stp100_motor = (MX_STP100_MOTOR *) motor->record->record_type_struct;

	if ( *stp100_motor == (MX_STP100_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_STP100_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_stp100_motor_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_stp100_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_STP100_MOTOR *stp100_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	stp100_motor = (MX_STP100_MOTOR *) malloc( sizeof(MX_STP100_MOTOR) );

	if ( stp100_motor == (MX_STP100_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_STP100_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = stp100_motor;
	record->class_specific_function_list
				= &mxd_stp100_motor_motor_function_list;

	motor->record = record;
	stp100_motor->record = record;

	/* An Pontech STP100 motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* This driver stores the acceleration time. */

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	motor->raw_speed = 0.0;
	motor->raw_base_speed = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_stp100_motor_finish_record_initialization()";

	MX_STP100_MOTOR *stp100_motor;
	mx_status_type status;

	stp100_motor = (MX_STP100_MOTOR *) record->record_type_struct;

	if ( stp100_motor == (MX_STP100_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"record_type_struct for record '%s' is NULL.",
			record->name );
	}

	status = mx_motor_finish_record_initialization( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Check for a valid board number. */

	if ( (stp100_motor->board_number < 0)
	  || (stp100_motor->board_number > 255) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal STP100_MOTOR board number %d.  The allowed range is (0-255).",
			stp100_motor->board_number );
	}

	/* Check for valid pin numbers. */

	switch( stp100_motor->positive_limit_switch_pin ) {
	case 3:
	case 5:
	case 6:
	case 8:
	case -3:
	case -5:
	case -6:
	case -8:
		break;
	case 0:
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal positive limit switch pin number %d.",
			stp100_motor->positive_limit_switch_pin );
		break;
	}
	switch( stp100_motor->negative_limit_switch_pin ) {
	case 3:
	case 5:
	case 6:
	case 8:
	case -3:
	case -5:
	case -6:
	case -8:
		if ( abs( stp100_motor->negative_limit_switch_pin )
			== abs( stp100_motor->positive_limit_switch_pin ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The positive and negative limit switches "
				"are assigned to the same pin number %d.",
				abs( stp100_motor->negative_limit_switch_pin ));
		}
		break;
	case 0:
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal positive limit switch pin number %d.",
			stp100_motor->positive_limit_switch_pin );
		break;
	}

	return status;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_stp100_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_STP100_MOTOR *stp100_motor;
	double position, move_deadband;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = STP100_MOTOR.\n\n");
	fprintf(file, "  name               = %s\n", record->name);
	fprintf(file, "  rs232              = %s\n",
				stp100_motor->rs232_record->name);

	fprintf(file, "  board number       = %d\n",
				stp100_motor->board_number);

	fprintf(file, "  positive limit switch pin = %d\n",
				stp100_motor->positive_limit_switch_pin );

	fprintf(file, "  negative limit switch pin = %d\n",
				stp100_motor->negative_limit_switch_pin );

	fprintf(file, "  home switch pin           = %d\n",
				stp100_motor->home_switch_pin );

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position           = %g %s  (%ld steps)\n",
		motor->position, motor->units,
		motor->raw_position.stepper );
	fprintf(file, "  scale              = %g %s per step.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
		motor->offset, motor->units);

	fprintf(file, "  backlash           = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper);

	fprintf(file, "  negative limit     = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit     = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband      = %g %s  (%ld steps)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_open( MX_RECORD *record )
{
	const char fname[] = "mxd_stp100_motor_open()";

	MX_MOTOR *motor;
	MX_STP100_MOTOR *stp100_motor;
	double raw_acceleration_parameters[ MX_MOTOR_NUM_ACCELERATION_PARAMS ];
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_stp100_motor_resynchronize( record );

	if ( status.code != MXE_SUCCESS )
		return status;

#if 0
	{
		double acceleration_time;

		MX_DEBUG(-2,("%s: about to read the acceleration time.",fname));

		status = mx_motor_get_acceleration_time( record,
							&acceleration_time );

		if ( status.code != MXE_SUCCESS )
			return status;

		MX_DEBUG(-2,("%s: motor '%s' acceleration_time = %g seconds",
				fname, record->name, acceleration_time ));
	}
#endif

	/* Update the speed, base speed, and acceleration. */

	status = mx_motor_set_speed( record, stp100_motor->default_speed );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_motor_set_base_speed( record,
					stp100_motor->default_base_speed );

	if ( status.code != MXE_SUCCESS )
		return status;

	raw_acceleration_parameters[0] =
			stp100_motor->default_acceleration_time;

	raw_acceleration_parameters[1] = 0.0;
	raw_acceleration_parameters[2] = 0.0;
	raw_acceleration_parameters[3] = 0.0;

	status = mx_motor_set_raw_acceleration_parameters( record,
						raw_acceleration_parameters );
	return status;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxd_stp100_motor_resynchronize()";

	MX_STP100_MOTOR *stp100_motor;
	double position;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed is NULL." );
	}

	stp100_motor = (MX_STP100_MOTOR *) record->record_type_struct;

	if ( stp100_motor == (MX_STP100_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_STP100_MOTOR pointer for motor '%s' is NULL.",
			record->name );
	}

	/* Discard any unread RS-232 input or unwritten RS-232 output. */

	status = mx_rs232_discard_unwritten_output( stp100_motor->rs232_record,
							STP100_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS && status.code != MXE_UNSUPPORTED )
		return status;

	status = mx_rs232_discard_unread_input( stp100_motor->rs232_record,
							STP100_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Send a get_position() command to verify that the controller
	 * is there and to update the current position in the motor structure.
	 */

	status = mx_motor_get_position( record, &position );

	/* Check to see if the controller responded. */

	switch( status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NOT_READY:
		return mx_error( MXE_NOT_READY, fname,
"No response from the Pontech controller for '%s'.  Is it turned on?",
			record->name );
		break;
	default:
		return status;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_is_busy( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_is_busy()";

	MX_STP100_MOTOR *stp100_motor;
	char response[80];
	long difference;
	int num_items;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Ask for the difference between the destination position and
	 * the current position.  If it is non-zero, the motor is moving.
	 */

	status = mxd_stp100_motor_command( stp100_motor, "RT",
			response, sizeof response, STP100_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%ld", &difference );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unable to find a position difference value in the response "
		"from the STP100_MOTOR.  Response = '%s'", response );
	}

	if ( difference == 0 ) {
		motor->busy = FALSE;
	} else {
		motor->busy = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_move_absolute()";

	MX_STP100_MOTOR *stp100_motor;
	char command[80];
	long motor_steps;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Setup the positive limit switch. */

	if ( stp100_motor->positive_limit_switch_pin != 0 ) {

		if ( stp100_motor->positive_limit_switch_pin > 0 ) {

			/* Active closed. */

			sprintf( command, "TC%d",
				stp100_motor->positive_limit_switch_pin );
		} else {

			/* Active open. */

			sprintf( command, "TS%d",
				stp100_motor->positive_limit_switch_pin );
		}
		status = mxd_stp100_motor_command( stp100_motor, command,
						NULL, 0, STP100_MOTOR_DEBUG );
	}

	/* Setup the negative limit switch. */

	if ( stp100_motor->negative_limit_switch_pin != 0 ) {

		if ( stp100_motor->negative_limit_switch_pin > 0 ) {

			/* Active closed. */

			sprintf( command, "TC%d",
				stp100_motor->negative_limit_switch_pin );
		} else {

			/* Active open. */

			sprintf( command, "TS%d",
				stp100_motor->negative_limit_switch_pin );
		}
		status = mxd_stp100_motor_command( stp100_motor, command,
						NULL, 0, STP100_MOTOR_DEBUG );
	}

	/* Send the move command. */

	motor_steps = motor->raw_destination.stepper;

	sprintf( command, "MI%ld", motor_steps );

	status = mxd_stp100_motor_command( stp100_motor, command,
						NULL, 0, STP100_MOTOR_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_get_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_get_position()";

	MX_STP100_MOTOR *stp100_motor;
	char response[80];
	int num_items;
	long motor_steps;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_stp100_motor_command( stp100_motor, "RC",
			response, sizeof response, STP100_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%ld", &motor_steps );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable position value '%s' was read.", response );
	}

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_set_position()";

	MX_STP100_MOTOR *stp100_motor;
	char command[80];
	long motor_steps;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	motor_steps = motor->raw_set_position.stepper;

	sprintf( command, "HM%ld", motor_steps );

	status = mxd_stp100_motor_command( stp100_motor, command,
						NULL, 0, STP100_MOTOR_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_soft_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_soft_abort()";

	MX_STP100_MOTOR *stp100_motor;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_stp100_motor_command( stp100_motor, "H0",
						NULL, 0, STP100_MOTOR_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_immediate_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_immediate_abort()";

	MX_STP100_MOTOR *stp100_motor;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_stp100_motor_command( stp100_motor, "HI",
						NULL, 0, STP100_MOTOR_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_positive_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_positive_limit_hit()";

	MX_STP100_MOTOR *stp100_motor;
	char command[80];
	char response[80];
	int num_items, pin_state;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( stp100_motor->positive_limit_switch_pin == 0 ) {
		motor->positive_limit_hit = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	sprintf( command, "RP%d", stp100_motor->positive_limit_switch_pin );

	status = mxd_stp100_motor_command( stp100_motor, command,
			response, sizeof response, STP100_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%d", &pin_state );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Cannot find the positive limit switch status in the response '%s'.",
			response );
	}

	/* 
	 * The interpretation of the result depends on whether the limit
	 * switch is active closed or active open.
	 */

	if ( stp100_motor->positive_limit_switch_pin > 0 ) {
	
		/* Active closed */

		if ( pin_state == 0 ) {
			motor->positive_limit_hit = TRUE;
		} else {
			motor->positive_limit_hit = FALSE;
		}
	} else {
	
		/* Active open */

		if ( pin_state == 0 ) {
			motor->positive_limit_hit = FALSE;
		} else {
			motor->positive_limit_hit = TRUE;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_negative_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_negative_limit_hit()";

	MX_STP100_MOTOR *stp100_motor;
	char command[80];
	char response[80];
	int num_items, pin_state;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( stp100_motor->negative_limit_switch_pin == 0 ) {
		motor->negative_limit_hit = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	sprintf( command, "RP%d", stp100_motor->negative_limit_switch_pin );

	status = mxd_stp100_motor_command( stp100_motor, command,
			response, sizeof response, STP100_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%d", &pin_state );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Cannot find the negative limit switch status in the response '%s'.",
			response );
	}

	/* 
	 * The interpretation of the result depends on whether the limit
	 * switch is active closed or active open.
	 */

	if ( stp100_motor->negative_limit_switch_pin > 0 ) {
	
		/* Active closed */

		if ( pin_state == 0 ) {
			motor->negative_limit_hit = TRUE;
		} else {
			motor->negative_limit_hit = FALSE;
		}
	} else {
	
		/* Active open */

		if ( pin_state == 0 ) {
			motor->negative_limit_hit = FALSE;
		} else {
			motor->negative_limit_hit = TRUE;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_find_home_position()";

	MX_STP100_MOTOR *stp100_motor;
	char command[80];
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( stp100_motor->home_switch_pin < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"A home switch pin number is not defined for motor '%s'.",
			motor->record->name );
	}

	if ( motor->home_search >= 0 ) {
		sprintf( command, "TC%dH+", stp100_motor->home_switch_pin );
	} else {
		sprintf( command, "TC%dH-", stp100_motor->home_switch_pin );
	}
		
	status = mxd_stp100_motor_command( stp100_motor, command,
						NULL, 0, STP100_MOTOR_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_constant_velocity_move( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_constant_velocity_move()";

	MX_STP100_MOTOR *stp100_motor;
	char command[20];
	char response[80];
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->constant_velocity_move >= 0 ) {
		strcpy( command, "H+" );
	} else {
		strcpy( command, "H-" );
	}

	status = mxd_stp100_motor_command( stp100_motor, command,
						response, sizeof(response),
						STP100_MOTOR_DEBUG );

	return status;
}

static mx_status_type
mxd_stp100_motor_get_factor( MX_STP100_MOTOR *stp100_motor,
				char *command, int *factor )
{
	const char fname[] = "mxd_stp100_motor_get_factor()";

	char response[80];
	int num_items;
	mx_status_type status;

	status = mxd_stp100_motor_command( stp100_motor, command,
					response, sizeof(response),
					STP100_MOTOR_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%d", factor );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"The response '%s' to the command '%s' for motor '%s' did not contain "
	"the expected acceleration or speed factor.",
			response, command, stp100_motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_get_parameter( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_get_parameter()";

	MX_STP100_MOTOR *stp100_motor;
	int step_delay, minimum_step_delay, total_step_delay;
	int acceleration_factor;
	double microseconds_per_step, numerator;
	double acceleration_seconds, acceleration_ticks;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	/* This controller expresses all speed parameters in units of
	 * 1.6 microsecond time intervals per step.
	 */

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:

		status = mxd_stp100_motor_get_factor( stp100_motor,
						"RSD", &step_delay );

		if ( status.code != MXE_SUCCESS )
			return status;

		microseconds_per_step = 1.6 * (double) step_delay;

		motor->raw_speed = mx_divide_safely( 1000000.0,
						microseconds_per_step );
		break;

	case MXLV_MTR_BASE_SPEED:

		/* Get the step delay. */
   
		status = mxd_stp100_motor_get_factor( stp100_motor,
						"RSD", &step_delay );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Get the minimum step delay. */

		status = mxd_stp100_motor_get_factor( stp100_motor,
						"RSM", &minimum_step_delay );

		if ( status.code != MXE_SUCCESS )
			return status;

		total_step_delay = step_delay + minimum_step_delay;

		microseconds_per_step = 1.6 * (double) total_step_delay;

		motor->raw_base_speed = mx_divide_safely( 1000000.0,
						microseconds_per_step );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		/* The smallest possible total step delay is 6 according
		 * to the manual.
		 */

		total_step_delay = 6;

		microseconds_per_step = 1.6 * (double) total_step_delay;

		motor->raw_maximum_speed = mx_divide_safely( 1000000.0,
						microseconds_per_step );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		/* Get the step delay. */
   
		status = mxd_stp100_motor_get_factor( stp100_motor,
						"RSD", &step_delay );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Get the minimum step delay. */

		status = mxd_stp100_motor_get_factor( stp100_motor,
						"RSM", &minimum_step_delay );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Get the acceleration factor. */

		status = mxd_stp100_motor_get_factor( stp100_motor,
						"RSA", &acceleration_factor );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Compute the acceleration time in 1.6 microsecond ticks. */

		numerator = (double) ( minimum_step_delay *
				( minimum_step_delay + 2 * step_delay ) );

		acceleration_ticks = mx_divide_safely( numerator,
					2.0 * (double) acceleration_factor );

		/* Convert the acceleration_time to seconds. */

		acceleration_seconds = 1.6e-6 * acceleration_ticks;

		motor->raw_acceleration_parameters[0] = acceleration_seconds;
		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;

		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_stp100_motor_set_parameter( MX_MOTOR *motor )
{
	const char fname[] = "mxd_stp100_motor_set_parameter()";

	MX_STP100_MOTOR *stp100_motor;
	char command[80];
	int step_delay, minimum_step_delay, total_step_delay;
	double microseconds_per_step, minimum_base_speed;
	double acceleration_factor, numerator;
	double acceleration_seconds, acceleration_ticks;
	mx_status_type status;

	status = mxd_stp100_motor_get_pointers( motor, &stp100_motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		microseconds_per_step = mx_divide_safely( 1000000.0,
							motor->raw_speed );

		step_delay = (int) mx_round( microseconds_per_step / 1.6 );

		sprintf( command, "SD%d", step_delay );

		status = mxd_stp100_motor_command( stp100_motor, command,
						NULL, 0, STP100_MOTOR_DEBUG );

		return status;
		break;

	case MXLV_MTR_BASE_SPEED:
		/* Get the slew speed step delay. */

		status = mxd_stp100_motor_get_factor( stp100_motor,
						"RSD", &step_delay );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Compute the total step delay for the base speed. */

		microseconds_per_step = mx_divide_safely( 1000000.0,
						motor->raw_base_speed );

		total_step_delay = (int) mx_round(microseconds_per_step / 1.6);

#define MX_MAX_16BIT_INTEGER	65535.0

		if ( total_step_delay > MX_MAX_16BIT_INTEGER ) {

			minimum_base_speed = motor->scale *
		mx_divide_safely( 1000000.0, 1.6 * MX_MAX_16BIT_INTEGER );

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested base speed %g %s/sec for motor '%s' is too slow.  "
	"The minimum allowed base speed for this motor is %g %s/sec.",
				motor->base_speed, motor->units,
				motor->record->name,
				minimum_base_speed, motor->units );
		}

		minimum_step_delay = total_step_delay - step_delay;

		sprintf( command, "SM%d", minimum_step_delay );

		status = mxd_stp100_motor_command( stp100_motor, command,
						NULL, 0, STP100_MOTOR_DEBUG );

		return status;
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		/* The smallest possible total step delay is 6 according
		 * to the manual.
		 */

		microseconds_per_step = mx_divide_safely( 1000000.0,
						motor->raw_maximum_speed );

		total_step_delay = (int) mx_round(microseconds_per_step / 1.6);

		/* If the total step delay is less than the minimum,
		 * set it to the minimum.
		 */

		if ( total_step_delay < 6 ) {
			total_step_delay = 6;

			microseconds_per_step = 1.6 * (double) total_step_delay;

			motor->raw_maximum_speed = mx_divide_safely( 1000000.0,
						microseconds_per_step );

			motor->maximum_speed = fabs(motor->scale)
					* motor->raw_maximum_speed;
		}
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		/* Get the step delay. */
   
		status = mxd_stp100_motor_get_factor( stp100_motor,
						"RSD", &step_delay );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Get the minimum step delay. */

		status = mxd_stp100_motor_get_factor( stp100_motor,
						"RSM", &minimum_step_delay );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Convert the acceleration time in seconds to acceleration
		 * time in units of 1.6 microsecond ticks.
		 */

		acceleration_seconds = motor->raw_acceleration_parameters[0];

		acceleration_ticks = acceleration_seconds / 1.6e-6;

		/* Compute the acceleration factor. */

		numerator = (double) ( minimum_step_delay *
				( minimum_step_delay + 2 * step_delay ) );

		acceleration_factor = mx_divide_safely( numerator,
					2.0 * acceleration_ticks );

		sprintf( command, "SA%ld", mx_round( acceleration_factor ) );

		status = mxd_stp100_motor_command( stp100_motor, command,
						NULL, 0, STP100_MOTOR_DEBUG );

		return status;
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === Extra functions for the use of this driver. === */

#if STP100_MOTOR_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

MX_EXPORT mx_status_type
mxd_stp100_motor_command( MX_STP100_MOTOR *stp100_motor,
			char *command,
			char *response,
			int response_buffer_length,
			int debug_flag )
{
	const char fname[] = "mxd_stp100_motor_command()";

	char command_string[100];
	int i, max_attempts;
	unsigned long sleep_ms;
	mx_status_type status;
#if STP100_MOTOR_DEBUG_TIMING
	MX_HRT_RS232_TIMING command_timing, response_timing;
#endif

	if ( stp100_motor == (MX_STP100_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL MX_STP100_MOTOR pointer passed." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL command buffer pointer passed." );
	}

	/* Construct the complete command with the board number prefix. */

	sprintf( command_string, "BD%d,%s",
				stp100_motor->board_number, command );

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: sending '%s'", fname, command_string));
	}

#if STP100_MOTOR_DEBUG_TIMING
	MX_HRT_RS232_START_COMMAND( command_timing, 2 + strlen(command) );
#endif

	/* Send the command string. */

	status = mx_rs232_putline( stp100_motor->rs232_record,
					command_string, NULL, 0 );

#if STP100_MOTOR_DEBUG_TIMING
	MX_HRT_RS232_END_COMMAND( command_timing );
#endif

	if ( status.code != MXE_SUCCESS )
		return status;

#if STP100_MOTOR_DEBUG_TIMING
	MX_HRT_RS232_COMMAND_RESULTS( command_timing, command_string, fname );
#endif

	/* Get the response, if one is expected. */

	i = 0;

	max_attempts = 1;
	sleep_ms = 0;

	if ( response != NULL ) {
		for ( i = 0; i < max_attempts; i++ ) {
			if ( i > 0 ) {
				MX_DEBUG(-2,
			("%s: No response yet to '%s' command.  Retrying...",
			stp100_motor->rs232_record->name, command_string ));
			}

#if STP100_MOTOR_DEBUG_TIMING
			MX_HRT_RS232_START_RESPONSE( response_timing,
						stp100_motor->rs232_record );
#endif

			status = mx_rs232_getline( stp100_motor->rs232_record,
					response, response_buffer_length,
					NULL, MXF_232_WAIT );

#if STP100_MOTOR_DEBUG_TIMING
			MX_HRT_RS232_END_RESPONSE( response_timing,
						2 + strlen(response) );
#endif

			if ( status.code == MXE_SUCCESS ) {
				break;		/* Exit the for() loop. */

			} else if ( status.code != MXE_NOT_READY ) {
				MX_DEBUG(-2,
				("*** Exiting with status = %ld",status.code));
				return status;
			}
			mx_msleep(sleep_ms);
		}

#if STP100_MOTOR_DEBUG_TIMING
		MX_HRT_TIME_BETWEEN_MEASUREMENTS( command_timing,
						response_timing, fname );

		MX_HRT_RS232_RESPONSE_RESULTS( response_timing,
						response, fname );
#endif

		if ( i >= max_attempts ) {
			status = mx_rs232_discard_unread_input(
				stp100_motor->rs232_record, debug_flag );

			if ( status.code != MXE_SUCCESS ) {
				mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread characters in buffer for record '%s'",
					stp100_motor->rs232_record->name );
			}

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"No response seen to '%s' command", command );
		}
		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s'", fname, response));
		}
	}

	MX_DEBUG(2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

