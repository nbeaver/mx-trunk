/*
 * Name:    d_pm304.c 
 *
 * Purpose: MX motor driver to control McLennan PM304 motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2004, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PM304_DEBUG		FALSE

#define PM304_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_clock.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "d_pm304.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_pm304_record_function_list = {
	NULL,
	mxd_pm304_create_record_structures,
	mxd_pm304_finish_record_initialization,
	NULL,
	mxd_pm304_print_motor_structure,
	NULL,
	NULL,
	mxd_pm304_open,
	mxd_pm304_close,
	NULL,
	mxd_pm304_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_pm304_motor_function_list = {
	mxd_pm304_motor_is_busy,
	mxd_pm304_move_absolute,
	mxd_pm304_get_position,
	mxd_pm304_set_position,
	mxd_pm304_soft_abort,
	mxd_pm304_immediate_abort,
	mxd_pm304_positive_limit_hit,
	mxd_pm304_negative_limit_hit,
	mxd_pm304_find_home_position,
	mxd_pm304_constant_velocity_move,
	mxd_pm304_get_parameter,
	mxd_pm304_set_parameter,
	NULL,
	mxd_pm304_get_status
};

/* McLennan PM304 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pm304_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PM304_STANDARD_FIELDS
};

long mxd_pm304_num_record_fields
		= sizeof( mxd_pm304_record_field_defaults )
			/ sizeof( mxd_pm304_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pm304_rfield_def_ptr
			= &mxd_pm304_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_pm304_get_pointers( MX_MOTOR *motor,
			MX_PM304 **pm304,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pm304_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( pm304 == (MX_PM304 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PM304 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pm304 = (MX_PM304 *) (motor->record->record_type_struct);

	if ( *pm304 == (MX_PM304 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PM304 pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_pm304_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_pm304_create_record_structures()";

	MX_MOTOR *motor;
	MX_PM304 *pm304;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	pm304 = (MX_PM304 *) malloc( sizeof(MX_PM304) );

	if ( pm304 == (MX_PM304 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PM304 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = pm304;
	record->class_specific_function_list
				= &mxd_pm304_motor_function_list;

	motor->record = record;

	/* An McLennan PM304 motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The PM304 reports accelerations in steps/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_pm304_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_PM304 *pm304;
	mx_status_type mx_status;

	pm304 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Server minimum delay time between commands. ***/

#if 1
	record->event_time_manager = (MX_EVENT_TIME_MANAGER *)
		malloc( sizeof( MX_EVENT_TIME_MANAGER ) );

	if ( record->event_time_manager == (MX_EVENT_TIME_MANAGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate an MX_EVENT_TIME_MANAGER structure." );
	}

	if ( pm304->minimum_event_interval >= 0.0 ) {
		mx_set_event_interval( record, pm304->minimum_event_interval );
	}
#endif

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_pm304_print_motor_structure()";

	MX_MOTOR *motor;
	MX_PM304 *pm304;
	double position, move_deadband;
	mx_status_type mx_status;

	pm304 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type          = PM304.\n\n");
	fprintf(file, "  name                = %s\n", record->name);
	fprintf(file, "  rs232               = %s\n",
						pm304->rs232_record->name);

	fprintf(file, "  axis number         = %ld\n", pm304->axis_number);
	fprintf(file, "  axis encoder number = %ld\n",
						pm304->axis_encoder_number);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
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
mxd_pm304_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pm304_open()";

	MX_MOTOR *motor;
	MX_PM304 *pm304;
	long mclennan_position, position_from_database, difference;
	double speed;
	mx_status_type mx_status;

	MX_DEBUG(2, ("%s invoked.", fname));

	pm304 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Flush out any unwritten and unread characters before sending
	 * commands to the PM304 indexer.
	 */

	mx_status = mx_rs232_discard_unwritten_output( pm304->rs232_record,
								PM304_DEBUG );

	if (mx_status.code != MXE_SUCCESS && mx_status.code != MXE_UNSUPPORTED)
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( pm304->rs232_record,
								PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save a copy of the position recorded in the database so that it
	 * can be compared with the position the McLennan controller thinks
	 * it is at.
	 */

	position_from_database = motor->raw_position.stepper;

	/* Ask the McLennan controller for what it thinks is the
	 * current position.
	 */

	mx_status = mx_motor_get_position_steps( record, &mclennan_position );

	/* This is the first time we have tried to talk to the McLennan
	 * controller, so we check to see if the controller is responding.
	 */

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NOT_READY:
		return mx_error( MXE_NOT_READY, fname,
"No response from the McLennan controller for '%s'.  Is it turned on?",
			record->name );
	default:
		return mx_status;
	}

	/* Does the position from the database agree with the position
	 * reported by the McLennan controller?
	 */

	difference = labs( mclennan_position - position_from_database );

	if ( difference > MX_PM304_POSITION_DISCREPANCY_THRESHOLD ) {

		/* They do not agree.  If the McLennan says that it is
		 * at or near zero, then it has probably been power cycled
		 * since the last time that MX talked to it.
		 */

		if ( labs( mclennan_position ) < MX_PM304_ZERO_THRESHOLD ) {

			/* The McLennan probably has been power cycled, so
			 * restore the position from the database.
			 */

			mx_warning(
"The McLennan controller '%s' appears to have been power cycled since "
"the last time MX spoke to it.  Attempting to restore the saved position "
"of %ld steps.", motor->record->name, position_from_database );

			mx_status = mx_motor_set_position_steps( motor->record,
						position_from_database );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Get the values of the speed, base speed, and acceleration.
	 *
	 * As it happens, for this controller and driver, asking for one
	 * of these values causes all of them to be updated, so we don't
	 * need to explicitly ask for all of them.
	 */

	mx_status = mx_motor_get_speed( record, &speed );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pm304_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_pm304_close()";

	MX_MOTOR *motor;
	MX_PM304 *pm304;
	double position;
	mx_status_type mx_status;

	pm304 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Flush out any unwritten and unread characters before sending
	 * commands to the PM304 indexer.
	 */

	mx_status = mx_rs232_discard_unwritten_output( pm304->rs232_record,
								PM304_DEBUG );

	if (mx_status.code != MXE_SUCCESS && mx_status.code != MXE_UNSUPPORTED)
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( pm304->rs232_record,
								PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the current position in the motor structure. */

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* At present, the position is the only parameter we read out.
	 * We rely on the battery backed memory of the controller
	 * for the rest of the controller parameters.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_pm304_resynchronize()";

	MX_PM304 *pm304;
	char response[80], banner[40];
	size_t length;
	mx_status_type mx_status;

	pm304 = (MX_PM304 *) record->record_type_struct;

	if ( pm304 == (MX_PM304 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PM304 pointer for motor is NULL." );
	}

	/* Discard any unread RS-232 input or unwritten RS-232 output. */

	mx_status = mx_rs232_discard_unwritten_output( pm304->rs232_record,
								PM304_DEBUG );

	if (mx_status.code != MXE_SUCCESS && mx_status.code != MXE_UNSUPPORTED)
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( pm304->rs232_record,
								PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send an identify command to verify that the controller is there. */

	mx_status = mxd_pm304_command( pm304, "id",
			response, sizeof response, PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that we got the identification message that we expected. */

	strlcpy( banner, "Mclennan Servo Supplies Ltd", sizeof(banner) );

	length = strlen( banner );

	if ( strncmp( response, banner, length ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Did not get Mclennan identification banner in response to an 'id' command.  "
"Instead got the response '%s'", response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_motor_is_busy()";

	MX_PM304 *pm304;
	char response[80];
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pm304_command( pm304, "os",
			response, sizeof response, PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[3] == '1' ) {
		motor->busy = FALSE;
	} else if ( response[3] == '0' ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unexpected character '%c' (0x%x) found in response '%s'",
			response[3], response[3], response );
	}

	mx_status = mxd_pm304_command( pm304, "co",
			response, sizeof response, PM304_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pm304_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_move_absolute()";

	MX_PM304 *pm304;
	char command[20];
	char response[20];
	long motor_steps;
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_destination.stepper;

	/* Format the move command and send it. */

	snprintf( command, sizeof(command), "ma%ld", motor_steps );

	mx_status = mxd_pm304_command( pm304, command,
			response, sizeof response, PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Did we get an "OK" response? */

	if ( strcmp( response, "OK" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not get 'OK' response from controller.  Instead got '%s'",
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_get_position()";

	MX_PM304 *pm304;
	char response[30];
	int num_items;
	long motor_steps;
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pm304_command( pm304, "oa",
			response, sizeof response, PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "AP=%ld", &motor_steps );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable position value '%s' was read.", response );
	}

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_set_position()";

	MX_PM304 *pm304;
	char command[40];
	char response[40];
	char expected_response[40];
	char c;
	long motor_steps;
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_set_position.stepper;

	snprintf( command, sizeof(command), "ap%ld", motor_steps );

	mx_status = mxd_pm304_command( pm304, command,
			response, sizeof response, PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "OK" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not get 'OK' response from controller.  Instead got '%s'",
			response );
	}

	/* Set the encoder position to match. */

	if ( pm304->axis_encoder_number >= 0 ) {

		snprintf( command, sizeof(command), "%ldap%ld",
				pm304->axis_encoder_number, motor_steps );

		mx_status = mxd_pm304_putline( pm304, command, PM304_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pm304_getline( pm304,
				response, sizeof(response), PM304_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( expected_response, sizeof(expected_response),
				"%ld:OK", pm304->axis_encoder_number );

		if ( strcmp( response, expected_response ) != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Did not get 'OK' response from encoder controller.  Instead got '%s'",
				response );
		}

		mx_status = mx_rs232_getchar( pm304->rs232_record,
						&c, MXF_232_WAIT );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_soft_abort()";

	MX_PM304 *pm304;
	char command[40];
	mx_bool_type busy;
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is the motor currently moving? */

	mx_status = mx_motor_is_busy( motor->record, &busy );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	/* Don't need to do anything if the motor is not moving. */

	if ( busy == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, we have to send the soft stop
	 * command (an ESC character).
	 */

	snprintf( command, sizeof(command), "%ld\033", pm304->axis_number );

	mx_status = mxd_pm304_putline( pm304, command, PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_immediate_abort()";

	MX_PM304 *pm304;
	char command[40];
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the immediate abort command (a ctrl-C character). */

	snprintf( command, sizeof(command), "%ld\003", pm304->axis_number );

	mx_status = mxd_pm304_putline( pm304, command, PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_positive_limit_hit()";

	MX_PM304 *pm304;
	char response[80];
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pm304_command( pm304, "os",
			response, sizeof response, PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[1] == '0' ) {
		motor->positive_limit_hit = FALSE;
	} else if ( response[1] == '1' ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = TRUE;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unexpected character '%c' (0x%x) found in response '%s'",
			response[1], response[1], response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_negative_limit_hit()";

	MX_PM304 *pm304;
	char response[80];
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pm304_command( pm304, "os",
			response, sizeof response, PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[0] == '0' ) {
		motor->negative_limit_hit = FALSE;
	} else if ( response[0] == '1' ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = TRUE;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unexpected character '%c' (0x%x) found in response '%s'",
			response[0], response[0], response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_find_home_position()";

	MX_PM304 *pm304;
	char command[20];
	char response[80];
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->home_search >= 0 ) {
		strlcpy( command, "ix", sizeof(command) );
	} else {
		strlcpy( command, "ix-1", sizeof(command) );
	}
		
	mx_status = mxd_pm304_command( pm304, command,
				response, sizeof(response), PM304_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pm304_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_constant_velocity_move()";

	MX_PM304 *pm304;
	char command[20];
	char response[80];
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->constant_velocity_move >= 0 ) {
		strlcpy( command, "cv", sizeof(command) );
	} else {
		strlcpy( command, "cv-1", sizeof(command) );
	}

	mx_status = mxd_pm304_command( pm304, command,
				response, sizeof(response), PM304_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pm304_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_get_parameter()";

	MX_PM304 *pm304;
	char response[80];
	double dummy;
	int num_items;
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mxd_pm304_command( pm304, "qs",
				response, sizeof(response), PM304_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "SV=%lg,SC=%lg,SA=%lg,SD=%lg",
			&( motor->raw_speed ),
			&dummy,
			&( motor->raw_acceleration_parameters[0] ),
			&( motor->raw_acceleration_parameters[1] ) );

		if ( num_items != 4 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse response to a Query Speeds command.  Response = '%s'",
				response );
		}

		/* This driver ignores the base speed. */

		motor->raw_base_speed = 0.0;
		motor->base_speed = 0.0;

		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;

		/* Update any scaled speeds that we were not explicitly
		 * asked for, since it will not happen automatically.
		 */

		if ( motor->parameter_type != MXLV_MTR_SPEED ) {

			motor->speed = fabs(motor->scale) * motor->raw_speed;
		}
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		motor->raw_maximum_speed = 400000;	/* steps per second */
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_set_parameter()";

	MX_PM304 *pm304;
	char command[20];
	char response[80];
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		snprintf( command, sizeof(command), "sv%ld",
			mx_round( motor->raw_speed ) );

		mx_status = mxd_pm304_command( pm304, command,
				response, sizeof(response), PM304_DEBUG );
		break;

	case MXLV_MTR_BASE_SPEED:
		/* Base speed changes are ignored. */

		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		snprintf( command, sizeof(command), "sa%ld",
			mx_round( motor->raw_acceleration_parameters[0] ));

		mx_status = mxd_pm304_command( pm304, command,
				response, sizeof(response), PM304_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command), "sd%ld",
			mx_round( motor->raw_acceleration_parameters[1] ));

		mx_status = mxd_pm304_command( pm304, command,
				response, sizeof(response), PM304_DEBUG );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pm304_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pm304_get_status()";

	MX_PM304 *pm304;
	char response[80];
	mx_status_type mx_status;

	pm304 = NULL;

	mx_status = mxd_pm304_get_pointers( motor, &pm304, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pm304_command( pm304, "os",
			response, sizeof(response), PM304_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	/* 0: Negative hard limit. */

	if ( response[0] == '1' ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* 1: Positive hard limit. */

	if ( response[1] == '1' ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* 2: Not error. */

	if ( response[2] == '0' ) {
		motor->status |= MXSF_MTR_ERROR;
	}

	/* 3: Controller idle. */

	if ( response[3] == '0' ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* 4: User abort. */

	if ( response[4] == '1' ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/* 5: Tracking abort. */

	if ( response[5] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* 6: Motor stalled. */

	if ( response[6] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* 7: Emergency stop. */

	if ( response[7] == '1' ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === Extra functions for the use of this driver. === */

#if PM304_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

MX_EXPORT mx_status_type
mxd_pm304_command( MX_PM304 *pm304, char *command, char *response,
		int response_buffer_length, int debug_flag )
{
	static const char fname[] = "mxd_pm304_command()";

	char prefix[20];
	int i, prefix_length, body_length;
	mx_status_type mx_status;
#if PM304_DEBUG_TIMING
	MX_HRT_RS232_TIMING command_timing, response_timing;
#endif

	if ( pm304 == NULL || command == NULL || response == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One or more of the pointers passed was NULL." );
	}

	/* Send the axis number prefix. */

	snprintf( prefix, sizeof(prefix), "%ld", pm304->axis_number );

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: command = '%s%s'", fname, prefix, command));
	}

	prefix_length = (int) strlen(prefix);

#if PM304_DEBUG_TIMING
	MX_HRT_RS232_START_COMMAND( command_timing, 1 + strlen(command) );
#endif

	for ( i = 0; i < prefix_length; i++ ) {
		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: sending char '%c' (0x%x)",
				fname, prefix[i], prefix[i] ));
		}

		mx_status = mx_rs232_putchar( pm304->rs232_record,
						prefix[i], MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Send the command string. */

	mx_status = mxd_pm304_putline( pm304, command, debug_flag );

#if PM304_DEBUG_TIMING
	MX_HRT_RS232_END_COMMAND( command_timing );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if PM304_DEBUG_TIMING
	MX_HRT_RS232_COMMAND_RESULTS( command_timing, command, fname );
#endif

	/* Get the response. */

#if PM304_DEBUG_TIMING
#if 0
	MX_HRT_RS232_START_RESPONSE( response_timing, pm304->rs232_record );
#else
	MX_HRT_RS232_START_RESPONSE( response_timing, NULL );
#endif
#endif

	mx_status = mxd_pm304_getline( pm304,
			response, response_buffer_length, debug_flag );

#if PM304_DEBUG_TIMING
	MX_HRT_RS232_END_RESPONSE( response_timing, strlen(response) );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if PM304_DEBUG_TIMING
	MX_HRT_TIME_BETWEEN_MEASUREMENTS( command_timing,
						response_timing, fname );

	MX_HRT_RS232_RESPONSE_RESULTS( response_timing, response, fname );
#endif

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: response = '%s'", fname, response ));
	}

	/* Check for the correct address prefix and then strip it off
	 * of the original string.  Note that this code assumes that
	 * axis prefixes like '1:' will be present in the responses.
	 */

	if ( strncmp( prefix, response, prefix_length ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Address prefix %ld not seen in response '%s'",
				pm304->axis_number, response );
	}

	if ( response[ prefix_length ] != ':' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Prefix terminator ':' not seen in response '%s'", response );
	}

	/* Now strip off the prefix. */

	body_length = (int) strlen(response) - prefix_length - 1;

	if ( body_length <= 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No body to response '%s'", response );
	}

	for ( i = 0; i < body_length; i++ ) {
		response[i] = response[i + prefix_length + 1];
	}

	response[body_length] = '\0';

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pm304_getline( MX_PM304 *pm304,
		char *buffer, int buffer_length, int debug_flag )
{
	static const char fname[] = "mxd_pm304_getline()";

	char c;
	int i;
	mx_status_type mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s invoked.", fname));
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	for ( i = 0; i < (buffer_length - 1) ; i++ ) {
		mx_status = mx_rs232_getchar( pm304->rs232_record,
							&c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS ) {
			/* Make the buffer contents a valid C string
			 * before returning, so that we can at least
			 * see what appeared before the error.
			 */

			buffer[i] = '\0';

			if ( debug_flag ) {
				MX_DEBUG(-2,
				("%s failed with status = %ld, c = 0x%x '%c'",
					fname, mx_status.code, c, c));
				MX_DEBUG(-2, ("-> buffer = '%s'", buffer) );
			}

			return mx_status;
		}

#if 0
		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received c = 0x%x '%c'",
					fname, c, c));
		}
#endif

		/* Sometimes, for TCP232 interfaces, the first character
		 * we get back is read as a null byte '\0'.  If this happens,
		 * throw the character away.  This shouldn't happen,
		 * but it does.
		 */

		if ( i == 0 && c == '\0' ) {
			i--;
			continue;	/* cycle the for() loop. */
		}

		/* Responses from the McLennan PM304 should be terminated
		 * by a carriage return followed by a line feed.
		 */

		if ( c != '\015' ) {
			buffer[i] = c;	/* char was not carriage return. */
		} else {
			buffer[i] = '\0';

			/* Eat the trailing line feed. */

			mx_status = mx_rs232_getchar( pm304->rs232_record,
							&c, MXF_232_WAIT );

			if ( mx_status.code != MXE_SUCCESS ) {
				if ( debug_flag ) {
					MX_DEBUG(-2,
	("%s: failed to see any character following a carriage return.",fname));
				}

				return mx_status;
			}

#if 0
			if ( debug_flag ) {
				MX_DEBUG(-2,("%s: received c = 0x%x '%c'",
						fname, c, c));
			}
#endif

			/* Was the trailing character really a line feed? */

			if ( c != '\012' ) {
				mx_status = mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Carriage return was followed by '%c' (0x%x) rather than line feed.",
					c, c );
			}

			break;		/* exit the for() loop */
		}
	}

	if ( debug_flag ) {

		MX_DEBUG(-2, ("%s: buffer = '%s'", fname, buffer) );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pm304_putline( MX_PM304 *pm304, char *buffer, int debug_flag )
{
	static const char fname[] = "mxd_pm304_putline()";

	char *ptr;
	char c;
	mx_status_type mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: buffer = '%s'", fname, buffer));
	}

	ptr = &buffer[0];

	/* Don't send zero length strings. */

	if ( strlen(ptr) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	while ( *ptr != '\0' ) {
		c = *ptr;

		mx_status = mx_rs232_putchar( pm304->rs232_record,
						c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0
		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: sent 0x%x '%c'", fname, c, c));
		}
#endif

		ptr++;
	}

	/* Send a carriage return character to end the line. */

	c = '\015';

	mx_status = mx_rs232_putchar( pm304->rs232_record, c, MXF_232_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sent 0x%x '%c'", fname, c, c));
	}

	return MX_SUCCESSFUL_RESULT;
}

