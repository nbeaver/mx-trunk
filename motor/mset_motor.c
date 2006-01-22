/* 
 * Name:    mset_motor.c
 *
 * Purpose: Motor set functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "motor.h"
#include "mdialog.h"

#ifndef max
#define max(a,b) ( (a) >= (b) ? (a) : (b) )
#endif

static int
motor_set_motor_get_step_value( MX_MOTOR *motor,
				int argc, char *argv[],
				long *step_value )
{
	static const char cname[] = "set motor";

	int count;

	if ( argc <= 2 ) {
		fprintf(output,
		"%s: must specify step count if using '%s' keyword.\n",
			cname, argv[1] );
		return FAILURE;
	}

	if ( motor->subclass != MXC_MTR_STEPPER ) {
		fprintf(output,
	"%s: Can't use steps option since '%s' is not a stepping motor.\n",
			cname, motor->record->name);

		return FAILURE;
	}

	count = sscanf( argv[2], "%ld", step_value );

	if ( count != 1 ) {
		return FAILURE;
	}
	return SUCCESS;
}

int
motor_set_motor( MX_RECORD *motor_record, int argc, char *argv[] )
{
	static const char cname[] = "set motor";

	MX_MOTOR *motor;
	char buffer[80];
	double position, reciprocal, deadband, speed;
	long motor_steps;
	size_t length0, length1;
	int count, save_status;
	int status, string_length;
	mx_status_type mx_status;

	if ( argc <= 1 ) {
		return FAILURE;
	}

	if ( motor_record == (MX_RECORD *) NULL ) {
		fprintf( output,
			"%s: motor_record pointer passed is NULL.\n", cname );

		return FAILURE;
	}

	if ( motor_record->mx_class != MXC_MOTOR ) {
		fprintf( output,
			"%s: The record named '%s' is not a motor record.\n",
			cname, motor_record->name );

		return FAILURE;
	}

	motor = (MX_MOTOR *)(motor_record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		fprintf( output,
	"Internal error in '%s': MX_MOTOR pointer for record '%s' is NULL.\n",
			cname, motor_record->name );

		return FAILURE;
	}

#if 0
	fprintf( output,
	"motor_record = 0x%p, motor_record->name = 0x%p, motor = 0x%p\n",
			motor_record, motor_record->name, motor );
	fprintf( output, "Motor name = '%s'\n", motor_record->name );
	fprintf( output, "argc = %d, argv[0] = '%s'\n", argc, argv[0] );
	fprintf( output, "argv[1] = '%s'\n", argv[1] );
#endif
	/* The following confirmation dialog was requested by some users.
	 * It doesn't constitute real security, but they like it.
	 */

	if ( motor_parameter_warning_flag == 0 ) {

		/* Don't generate any warning messages at all. */

	} else if ( motor_parameter_warning_flag == 1 ) {

		fprintf( output,
	"Changing motor parameters may have unexpected consequences.\n" );

		string_length = sizeof(buffer) - 1;

		status = motor_get_string( output,
			" Are you sure you want to do this (Y/N) ? ",
			NULL, &string_length, buffer );

		if ( status == FAILURE )
			return status;

		if ( buffer[0] != 'Y' && buffer[0] != 'y' )
			return FAILURE;
	} else {
		fprintf( output,
		"You are not permitted to change motor parameters.\n\n" );

		return FAILURE;
	}

	/* Figure out which command was requested and execute it. */

	mx_status = MX_SUCCESSFUL_RESULT;

	length0 = max( strlen( argv[0] ), 1 );

	if ( strncmp( argv[0], "zero", max(length0, 1) ) == 0 ) {

		mx_status = mx_motor_zero_position_value( motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else if (strncmp( "position", argv[0], max(length0, 7) ) == 0) {

		length1 = max( strlen( argv[1] ), 1 );

		if ( strncmp( "steps", argv[1], length1 ) == 0 ) {

			status = motor_set_motor_get_step_value( motor,
					argc, argv, &motor_steps );

			if ( status != SUCCESS )
				return status;

			mx_status = mx_motor_set_position_steps(
						motor_record, motor_steps );
		} else {
			count = sscanf( argv[1], "%lg", &position );

			if ( count != 1 ) {
				return FAILURE;
			}

			mx_status = mx_motor_set_position(
						motor_record, position );
		}

	} else if (strncmp( "positive_limit", argv[0], max(length0, 7)) == 0) {

		length1 = max( strlen( argv[1] ), 1 );

		if ( strncmp( "steps", argv[1], length1 ) == 0 ) {

			status = motor_set_motor_get_step_value( motor,
					argc, argv, &motor_steps );

			if ( status != SUCCESS )
				return status;

			motor->raw_positive_limit.stepper = motor_steps;

			motor->positive_limit = motor->offset
					+ motor->scale * (double) motor_steps;
		} else {
			count = sscanf( argv[1], "%lg", &position );

			if ( count != 1 ) {
				return FAILURE;
			}

			motor->positive_limit = position;

			if ( mx_difference( motor->scale, 0.0 ) < 1.0e-9 ) {
				reciprocal = 0.0;
			} else {
				reciprocal = 1.0 / motor->scale;
			}
			switch( motor->subclass ) {
			case MXC_MTR_STEPPER:
				motor_steps = mx_round(
					reciprocal*(position - motor->offset));

				motor->raw_positive_limit.stepper
							= motor_steps;

				break;
			case MXC_MTR_ANALOG:
				position = reciprocal
						* (position - motor->offset);

				motor->raw_positive_limit.analog = position;

				break;
			}
		}

	} else if (strncmp( "negative_limit", argv[0], max(length0, 1)) == 0) {

		length1 = max( strlen( argv[1] ), 1 );

		if ( strncmp( "steps", argv[1], length1 ) == 0 ) {

			status = motor_set_motor_get_step_value( motor,
					argc, argv, &motor_steps );

			if ( status != SUCCESS )
				return status;

			motor->raw_negative_limit.stepper = motor_steps;

			motor->negative_limit = motor->offset
					+ motor->scale * (double) motor_steps;
		} else {
			count = sscanf( argv[1], "%lg", &position );

			if ( count != 1 ) {
				return FAILURE;
			}

			motor->negative_limit = position;

			if ( mx_difference( motor->scale, 0.0 ) < 1.0e-9 ) {
				reciprocal = 0.0;
			} else {
				reciprocal = 1.0 / motor->scale;
			}
			switch( motor->subclass ) {
			case MXC_MTR_STEPPER:
				motor_steps = mx_round(
					reciprocal*(position - motor->offset));

				motor->raw_negative_limit.stepper
							= motor_steps;

				break;
			case MXC_MTR_ANALOG:
				position = reciprocal
						* (position - motor->offset);

				motor->raw_negative_limit.analog = position;

				break;
			}
		}

	} else if (strncmp( "backlash_correction",
					argv[0], max(length0, 3)) == 0)
	{

		length1 = max( strlen( argv[1] ), 1 );

		if ( strncmp( "steps", argv[1], length1 ) == 0 ) {

			status = motor_set_motor_get_step_value( motor,
					argc, argv, &motor_steps );

			if ( status != SUCCESS )
				return status;

			motor->raw_backlash_correction.stepper = motor_steps;

			motor->backlash_correction = motor->offset
					+ motor->scale * (double) motor_steps;
		} else {
			count = sscanf( argv[1], "%lg", &position );

			if ( count != 1 ) {
				return FAILURE;
			}

			motor->backlash_correction = position;

			if ( mx_difference( motor->scale, 0.0 ) < 1.0e-9 ) {
				reciprocal = 0.0;
			} else {
				reciprocal = 1.0 / motor->scale;
			}
			switch( motor->subclass ) {
			case MXC_MTR_STEPPER:
				motor_steps = mx_round(
					reciprocal*(position - motor->offset));

				motor->raw_backlash_correction.stepper
							= motor_steps;

				break;
			case MXC_MTR_ANALOG:
				position = reciprocal
						* (position - motor->offset);

				motor->raw_backlash_correction.analog
					= position;

				break;
			}
		}

	} else if (strncmp( "speed", argv[0], max(length0, 1)) == 0) {

		count = sscanf( argv[1], "%lg", &speed );

		if ( count != 1 ) {
			return FAILURE;
		}

		mx_status = mx_motor_set_speed( motor_record, speed );

	} else if (strncmp( "base_speed", argv[0], max(length0, 3)) == 0) {
		fprintf( output,
			"'set motor ... base_speed' not yet implemented.\n\n");
		return FAILURE;

	} else if (strncmp( "acceleration", argv[0], max(length0, 1)) == 0) {
		fprintf( output,
		"'set motor ... acceleration' not yet implemented.\n\n");
		return FAILURE;

	} else if (strncmp( "deadband", argv[0], max(length0, 1)) == 0) {

		length1 = max( strlen( argv[1] ), 1 );

		if ( strncmp( "steps", argv[1], length1 ) == 0 ) {

			status = motor_set_motor_get_step_value( motor,
					argc, argv, &motor_steps );

			if ( status != SUCCESS )
				return status;

			motor->raw_move_deadband.stepper = motor_steps;

		} else {
			count = sscanf( argv[1], "%lg", &deadband );

			if ( count != 1 ) {
				return FAILURE;
			}
			if ( mx_difference( motor->scale, 0.0 ) < 1.0e-9 ) {
				reciprocal = 0.0;
			} else {
				reciprocal = 1.0 / motor->scale;
			}
			switch( motor->subclass ) {
			case MXC_MTR_STEPPER:
				motor->raw_move_deadband.stepper
					= mx_round(reciprocal * deadband);
				break;
			case MXC_MTR_ANALOG:
				motor->raw_move_deadband.analog
					= reciprocal * deadband;
				break;
			}
		}

	} else {
		fprintf( output, "Unrecognized 'set motor' parameter.\n\n" );

		return FAILURE;
	}

	/* Invoke the save motor function to save the 'motor.dat' file. */

	if ( allow_motor_database_updates ) {
		snprintf( buffer, sizeof(buffer),
			"save motor %s", motor_savefile );

		save_status = cmd_execute_command_line(
			command_list_length, command_list, buffer);

		if ( save_status == FAILURE ) {
			fprintf( output,
				"%s: Failed to save motor file '%s'\n",
				cname, motor_savefile );
		}
	}

	if ( mx_status.code == MXE_SUCCESS ) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

