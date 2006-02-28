/*
 * Name:    d_cryostream600_motor.c 
 *
 * Purpose: MX motor driver to control Cryostream 600 series
 *          temperature controllers as if they were motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "d_cryostream600_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_cryostream600_motor_record_function_list = {
	NULL,
	mxd_cryostream600_motor_create_record_structures,
	mxd_cryostream600_motor_finish_record_initialization,
	mxd_cryostream600_motor_delete_record,
	mxd_cryostream600_motor_print_motor_structure,
	NULL,
	NULL,
	mxd_cryostream600_motor_open,
	NULL,
	NULL,
	mxd_cryostream600_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_cryostream600_motor_motor_function_list = {
	NULL,
	mxd_cryostream600_motor_move_absolute,
	NULL,
	NULL,
	mxd_cryostream600_motor_soft_abort,
	NULL,
	mxd_cryostream600_motor_positive_limit_hit,
	mxd_cryostream600_motor_negative_limit_hit,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler,
	NULL,
	NULL,
	mxd_cryostream600_motor_get_extended_status
};

#define CRYOSTREAM600_MOTOR_DEBUG	FALSE

#define MAX_RESPONSE_TOKENS             10
#define RESPONSE_TOKEN_LENGTH           10

/* Cryostream 600 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_cryostream600_motor_recfield_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_CRYOSTREAM600_MOTOR_STANDARD_FIELDS
};

long mxd_cryostream600_motor_num_record_fields
		= sizeof( mxd_cryostream600_motor_recfield_defaults )
		/ sizeof( mxd_cryostream600_motor_recfield_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_cryostream600_motor_rfield_def_ptr
			= &mxd_cryostream600_motor_recfield_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_cryostream600_motor_get_pointers( MX_MOTOR *motor,
			MX_CRYOSTREAM600_MOTOR **cryostream600_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_cryostream600_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( cryostream600_motor == (MX_CRYOSTREAM600_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CRYOSTREAM600_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MOTOR pointer passed is NULL." );
	}

	*cryostream600_motor = (MX_CRYOSTREAM600_MOTOR *)
				motor->record->record_type_struct;

	if ( *cryostream600_motor == (MX_CRYOSTREAM600_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_CRYOSTREAM600_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_cryostream600_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_cryostream600_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_CRYOSTREAM600_MOTOR *cryostream600_motor;
	long dimension[2];
	size_t dimension_size[2];

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	cryostream600_motor = (MX_CRYOSTREAM600_MOTOR *)
				malloc( sizeof(MX_CRYOSTREAM600_MOTOR) );

	if ( cryostream600_motor == (MX_CRYOSTREAM600_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_CRYOSTREAM600_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = cryostream600_motor;
	record->class_specific_function_list
				= &mxd_cryostream600_motor_motor_function_list;

	motor->record = record;
	cryostream600_motor->motor = motor;

	/* The Cryostream 600 temperature controller motor is treated
	 * as an analog motor.
	 */

	motor->subclass = MXC_MTR_ANALOG;

	/* Create an array to store the responses from the controller in. */

	cryostream600_motor->max_response_tokens = MAX_RESPONSE_TOKENS;
	cryostream600_motor->max_token_length = RESPONSE_TOKEN_LENGTH;

	dimension[0] = cryostream600_motor->max_response_tokens;
	dimension[1] = cryostream600_motor->max_token_length;

	dimension_size[0] = sizeof(char);
	dimension_size[1] = sizeof(char *);

	cryostream600_motor->response_token_array = (char **)
		mx_allocate_array( 2, dimension, dimension_size );

	if ( cryostream600_motor->response_token_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate a %ld by %ld string array.",
		dimension[0], dimension[1] );
	}

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_cryostream600_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_CRYOSTREAM600_MOTOR *cryostream600_motor;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We do not need to check the validity of the motor pointer since
	 * mx_motor_finish_record_initialization() will have done it for us.
	 */

	motor = (MX_MOTOR *) record->record_class_struct;

	/* But we should check the cryostream600_motor pointer, just in case
	 * something has been overwritten.
	 */

	cryostream600_motor = (MX_CRYOSTREAM600_MOTOR *)
					record->record_type_struct;

	if ( cryostream600_motor == (MX_CRYOSTREAM600_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_CRYOSTREAM600_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	/* Set the initial speeds and accelerations.  Please note that
	 * while the controller itself uses ramp rates in K per hour,
	 * the MX routines themselves use speeds in raw units per second.
	 * Thus, we have to convert the controller ramp rate into an
	 * internal K per second value.
	 */

	motor->raw_speed = cryostream600_motor->ramp_rate / 3600.0;
	motor->raw_base_speed = 0.0;
	motor->raw_maximum_speed = 1.0e30;  /* Large enough not to matter. */

	motor->speed = fabs(motor->scale) * motor->raw_speed;
	motor->base_speed = fabs(motor->scale) * motor->raw_base_speed;
	motor->maximum_speed = fabs(motor->scale) * motor->raw_maximum_speed;

	motor->raw_acceleration_parameters[0] = 0.0;
	motor->raw_acceleration_parameters[1] = 0.0;
	motor->raw_acceleration_parameters[2] = 0.0;
	motor->raw_acceleration_parameters[3] = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_delete_record( MX_RECORD *record )
{
	MX_CRYOSTREAM600_MOTOR *cryostream600_motor;
	long dimension[2];
	size_t dimension_size[2];

	if ( record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	cryostream600_motor = (MX_CRYOSTREAM600_MOTOR *)
					record->record_type_struct;

	if ( ( cryostream600_motor != NULL )
	  && ( cryostream600_motor->response_token_array != NULL ) )
	{
		dimension[0] = cryostream600_motor->max_response_tokens;
		dimension[1] = cryostream600_motor->max_token_length;

		dimension_size[0] = sizeof(char);
		dimension_size[1] = sizeof(char *);

		(void) mx_free_array(
				cryostream600_motor->response_token_array,
				2, dimension, dimension_size );
	}
	(void) mx_default_delete_record_handler( record );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[]
		= "mxd_cryostream600_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_CRYOSTREAM600_MOTOR *cryostream600_motor;
	double position, move_deadband, busy_deadband;
	double speed, ramp_rate;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_cryostream600_motor_get_pointers( motor,
						&cryostream600_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = CRYOSTREAM600_MOTOR.\n\n");
	fprintf(file, "  name               = %s\n", record->name);

	fprintf(file, "  RS-232 interface   = %s\n",
		cryostream600_motor->rs232_record->name);

	speed = motor->scale * motor->raw_speed;
	ramp_rate = 3600.0 * motor->raw_speed;

	fprintf(file, "  ramp rate          = %g %s/sec  (%g K/sec, %g K/hour)\n",
		speed, motor->units,
		motor->raw_speed, ramp_rate );

	busy_deadband = motor->scale * cryostream600_motor->busy_deadband;

	fprintf(file, "  busy deadband      = %g %s  (%g K)\n",
		busy_deadband, motor->units,
		cryostream600_motor->busy_deadband );

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position           = %g %s  (%g K)\n",
		motor->position, motor->units,
		motor->raw_position.analog );
	fprintf(file, "  scale              = %g %s per K.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
		motor->offset, motor->units);

	fprintf(file, "  backlash           = %g %s  (%g K)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog);

	fprintf(file, "  negative limit     = %g %s  (%g K)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit     = %g %s  (%g K)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband      = %g %s  (%g K)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_cryostream600_motor_open()";

	MX_CRYOSTREAM600_MOTOR *cryostream600_motor;
	char response[80];
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	cryostream600_motor =
		(MX_CRYOSTREAM600_MOTOR *) record->record_type_struct;

	if ( cryostream600_motor == (MX_CRYOSTREAM600_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CRYOSTREAM600_MOTOR pointer for analog input '%s' is NULL",
			record->name );
	}

	/* Throw away any unread and unwritten characters. */

	mx_status = mx_rs232_discard_unread_input(
			cryostream600_motor->rs232_record,
			CRYOSTREAM600_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unwritten_output(
			cryostream600_motor->rs232_record,
			CRYOSTREAM600_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Ask for the version number of the controller. */

	mx_status = mxd_cryostream600_motor_command( cryostream600_motor, "V$",
						response, sizeof(response),
						CRYOSTREAM600_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[0] != 'V' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cryostream motor '%s' did not return the expected "
		"version string in its response to the V$ command.  "
		"Response = '%s'", record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_cryostream600_motor_move_absolute()";

	MX_CRYOSTREAM600_MOTOR *cryostream600_motor;
	char command[80];
	char response[80];
	double ramp_rate;
	mx_status_type mx_status;

	mx_status = mxd_cryostream600_motor_get_pointers( motor,
						&cryostream600_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the controller is in SLAVE mode before trying to
	 * send a move command by getting the controller status.
	 */

	mx_status = mxd_cryostream600_motor_command( cryostream600_motor, "S$",
						response, sizeof(response),
						CRYOSTREAM600_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[0] != 'S' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cryostream motor '%s' did not return the expected "
		"status message in its response to the S$ command.  "
		"Response = '%s'.", motor->record->name, response );
	}

	/* The second token must be either "S" for SLAVE mode
	 * or "L" for LOCAL mode.
	 */

	if ( response[2] != 'S' ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"Cryostream motor '%s' cannot change the "
		"cryostream temperature "
		"since the controller is not in SLAVE mode.  "
		"To fix this, go to the front panel of the Cryostream "
		"controller, push the PROGRAM button, and select SLAVE "
		"from the list of phases.", motor->record->name );
	}

	/* Convert the motor speed in K per second back to a ramp rate
	 * in K per hour.
	 */

	ramp_rate = motor->raw_speed * 3600.0;

	/* Send the move command. */

	sprintf( command, "C:R:%.1f:%.1f:$",
			motor->raw_destination.analog, ramp_rate );

	mx_status = mxd_cryostream600_motor_command( cryostream600_motor,
					command, response, sizeof(response),
					CRYOSTREAM600_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[0] != 'A' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cryostream motor '%s' did not return the expected "
		"acknowledgement message in its response "
		"to the '%s' command.  Response = '%s'.",
			motor->record->name, command, response );
	}

	if ( response[2] != '1' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cryostream motor '%s' said that the command '%s' "
		"that we just sent was invalid.",
			motor->record->name, command );
	}

	/* Mark the motor as being busy. */

	motor->busy = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_cryostream600_motor_soft_abort()";

	MX_CRYOSTREAM600_MOTOR *cryostream600_motor;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_cryostream600_motor_get_pointers( motor,
					&cryostream600_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the motor as not busy. */

	motor->busy = FALSE;

	/* Unconditionally send a command to hold at the current temperature.
	 * If we are not in slave mode, the command will fail.
	 */

	strcpy( command, "C:H:$" );

	mx_status = mxd_cryostream600_motor_command( cryostream600_motor,
					command, response, sizeof(response),
					CRYOSTREAM600_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[0] != 'A' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cryostream motor '%s' did not return the expected "
		"acknowledgement message in its response "
		"to the '%s' command.  Response = '%s'",
			motor->record->name, command, response );
	}

	/* If we get an acknowledgement message saying that the command
	 * succeeded, then we are done.
	 */

	if ( strcmp( response, "A:1:$" ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* The acknowledgement message says that the command failed.
	 * Check to see if this is because we were in SLAVE mode.
	 */

	mx_status = mxd_cryostream600_motor_command( cryostream600_motor, "S$",
						response, sizeof(response),
						CRYOSTREAM600_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[0] == 'S' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cryostream motor '%s' did not return the expected "
		"status message in its response to the S$ command.  "
		"Response = '%s'", motor->record->name, response );
	}

	/* The second token must be either "S" for SLAVE mode
	 * or "L" for LOCAL mode.
	 */

	if ( response[2] != 'S' ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"The soft abort for Cryostream motor '%s' failed since the "
		"controller was not in SLAVE mode.",
			motor->record->name );
	}

	return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The soft abort for Cryostream motor '%s' failed for an "
		"unknown reason.", motor->record->name );
}

/* The Cryostream 600 series controllers do not have anything resembling
 * limit switches, so we always say that the limits have not been hit.
 */

MX_EXPORT mx_status_type
mxd_cryostream600_motor_positive_limit_hit( MX_MOTOR *motor )
{
	motor->positive_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_negative_limit_hit( MX_MOTOR *motor )
{
	motor->negative_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[]
		= "mxd_cryostream600_motor_get_extended_status()";

	MX_CRYOSTREAM600_MOTOR *cryostream600_motor;
	char response[80];
	char **token_array;
	double set_temperature, temperature_error, difference;
	mx_status_type mx_status;

	mx_status = mxd_cryostream600_motor_get_pointers( motor,
					&cryostream600_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Request the status of the controller. */

	token_array = cryostream600_motor->response_token_array;

	mx_status = mxd_cryostream600_motor_command( cryostream600_motor, "S$",
						response, sizeof(response),
						CRYOSTREAM600_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[0] != 'S' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cryostream motor '%s' did not return the expected "
		"status message in its response to the S$ command.  "
		"Response = '%s'.", motor->record->name, response );
	}

	mx_status = mxd_cryostream600_motor_parse_response( cryostream600_motor,
					response, CRYOSTREAM600_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save the cryostream temperature as the motor position. */

	set_temperature = atof( token_array[2] );
	temperature_error = atof( token_array[3] );

	motor->raw_position.analog = set_temperature + temperature_error;

	/* If the motor is closer to the destination than the busy deadband,
	 * then tell the controller to hold at the current temperature
	 * using the driver soft_abort function.
	 */

	if ( motor->busy ) {
		difference = motor->raw_destination.analog
				- motor->raw_position.analog;

		if ( fabs(difference) < cryostream600_motor->busy_deadband ) {
			(void) mxd_cryostream600_motor_soft_abort(
					cryostream600_motor->motor );
		}
	}

	/* Set the motor status flags. */

	motor->status = 0;

	if ( motor->busy ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

/********/

static mx_status_type
mxd_cryostream600_motor_wait_for_response( MX_RECORD *rs232_record,
						char *command )
{
	static const char fname[]
		= "mxd_cryostream600_motor_wait_for_response()";

	int i, max_retries;
	unsigned long num_input_bytes_available;
	unsigned long wait_ms;
	mx_status_type mx_status;

	max_retries = 50;
	wait_ms = 100;

	for ( i = 0; i <= max_retries; i++ ) {
		mx_status = mx_rs232_num_input_bytes_available( rs232_record,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_input_bytes_available > 0 ) {

			/* Exit the for() loop. */

			break;
		}

		mx_msleep( wait_ms );
	}

	if ( i > max_retries ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Timed out waiting for a response to the command '%s' from "
	"the Cryostream 600 series controller attached to RS-232 port '%s'.  "
	"Are you sure it is connected and turned on?",
			command, rs232_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_command( MX_CRYOSTREAM600_MOTOR *cryostream600_motor,
						char *command,
						char *response,
						size_t max_response_length,
						int debug_flag )
{
	static const char fname[] = "mxd_cryostream600_motor_command()";

	MX_RECORD *rs232_record;
	int i, command_length;
	int done_command_was_successful;
	unsigned long wait_ms;
	mx_status_type mx_status;

	if ( cryostream600_motor == (MX_CRYOSTREAM600_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CRYOSTREAM600_MOTOR pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	rs232_record = cryostream600_motor->rs232_record;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for Cryostream motor '%s' is NULL.",
			cryostream600_motor->motor->record->name );
	}

	/* Send the command.
	 *
	 * According to the manual, the Cryostream controller requires
	 * a 300 millisecond intercharacter delay for it to reliably
	 * receive commands.
	 */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending command '%s'.", fname, command));
	}

	command_length = (int) strlen( command );

	wait_ms = 300;

	for ( i = 0; i < command_length; i++ ) {

		mx_status = mx_rs232_putchar( rs232_record,
						command[i], MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep( wait_ms );
	}

	/* Wait for the response. */

	mx_status = mxd_cryostream600_motor_wait_for_response(
						rs232_record, command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response. */

	mx_status = mx_rs232_getline( rs232_record,
				response, max_response_length, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the first character in the response is a 'D', then this is
	 * actually the completion packet for some old command.  If so,
	 * then we need to read a second time to get the response we
	 * actually want.
	 */

	if ( response[0] == 'D' ) {
		if ( response[2] == '1' ) {
			done_command_was_successful = TRUE;
		} else {
			done_command_was_successful = FALSE;
		}

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received Done response '%s'.",
				fname, response));
		}

		/* Read again to get the real response. */

		mx_status = mxd_cryostream600_motor_wait_for_response(
						rs232_record, command );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_rs232_getline( rs232_record,
				response, max_response_length, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received Real response '%s'.",
				fname, response));
		}

		/* If the completion packet says that the old command
		 * was successful, tell the controller to hold at the
		 * current temperature by invoking the driver 'soft abort'
		 * function.
		 */

		if ( done_command_was_successful ) {
			(void) mxd_cryostream600_motor_soft_abort(
					cryostream600_motor->motor );
		}
	} else {
		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received response '%s'.",
						fname, response));
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_motor_parse_response(
				MX_CRYOSTREAM600_MOTOR *cryostream600_motor,
				char *response,
				int debug_flag )
{
	static const char fname[] = "mxd_cryostream600_motor_parse_response()";

	char **token_array;
	char *token_ptr, *colon_ptr;
	long i, token_length;

	if ( cryostream600_motor == (MX_CRYOSTREAM600_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CRYOSTREAM600_MOTOR pointer passed was NULL." );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed was NULL." );
	}

	token_array = cryostream600_motor->response_token_array;

	token_ptr = response;

	for ( i = 0; i < cryostream600_motor->max_response_tokens; i++ ) {

		colon_ptr = strchr( token_ptr, ':' );

		if ( colon_ptr == NULL ) {
			if ( *token_ptr == '$' ) {

				/* We have found the end of the response,
				 * so break out of the loop.
				 */

				break;
			}
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"The response termination character '$' was not found "
			"in the response message '%s'.", response );
		}

		token_length = colon_ptr - token_ptr + 1;

		if ( token_length > cryostream600_motor->max_token_length )
			token_length = cryostream600_motor->max_token_length;

		strlcpy( token_array[i], token_ptr, token_length );

		token_ptr = colon_ptr + 1;
	}

	cryostream600_motor->num_tokens_returned = i;

	/* Make sure that unused trailing tokens are set to empty strings. */

	for ( i = cryostream600_motor->num_tokens_returned;
		i < cryostream600_motor->max_response_tokens;
		i++ )
	{
		token_array[i][0] = '\0';
	}

	/* Display the returned tokens if requested. */

	if ( debug_flag ) {
		for ( i = 0; i < cryostream600_motor->num_tokens_returned; i++ )
		{
			MX_DEBUG(-2,("%s: token_array[%d] = '%s'",
				fname, i, token_array[i]));
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

