/*
 * Name:    d_itc503_motor.c 
 *
 * Purpose: MX motor driver to control Oxford Instruments ITC503
 *          temperature controllers as if they were motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define ITC503_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_ascii.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "d_itc503_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_itc503_motor_record_function_list = {
	NULL,
	mxd_itc503_motor_create_record_structures,
	mxd_itc503_motor_finish_record_initialization,
	NULL,
	mxd_itc503_motor_print_motor_structure,
	NULL,
	NULL,
	mxd_itc503_motor_open,
	NULL,
	NULL,
	mxd_itc503_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_itc503_motor_motor_function_list = {
	NULL,
	mxd_itc503_motor_move_absolute,
	NULL,
	NULL,
	mxd_itc503_motor_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_itc503_motor_get_extended_status
};

/* ITC503 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_itc503_motor_recfield_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_ITC503_MOTOR_STANDARD_FIELDS
};

mx_length_type mxd_itc503_motor_num_record_fields
		= sizeof( mxd_itc503_motor_recfield_defaults )
		/ sizeof( mxd_itc503_motor_recfield_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_itc503_motor_rfield_def_ptr
			= &mxd_itc503_motor_recfield_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_itc503_motor_get_pointers( MX_MOTOR *motor,
			MX_ITC503_MOTOR **itc503_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_itc503_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( itc503_motor == (MX_ITC503_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ITC503_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MOTOR pointer passed is NULL." );
	}

	*itc503_motor = (MX_ITC503_MOTOR *)
				motor->record->record_type_struct;

	if ( *itc503_motor == (MX_ITC503_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ITC503_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_itc503_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_itc503_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_ITC503_MOTOR *itc503_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	itc503_motor = (MX_ITC503_MOTOR *)
				malloc( sizeof(MX_ITC503_MOTOR) );

	if ( itc503_motor == (MX_ITC503_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ITC503_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = itc503_motor;
	record->class_specific_function_list
				= &mxd_itc503_motor_motor_function_list;

	motor->record = record;
	itc503_motor->record = record;

	/* The ITC503 temperature controller motor is treated
	 * as an analog motor.
	 */

	motor->subclass = MXC_MTR_ANALOG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[]
		= "mxd_itc503_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_ITC503_MOTOR *itc503_motor;
	double position, move_deadband, busy_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_itc503_motor_get_pointers( motor,
						&itc503_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = ITC503_MOTOR.\n\n");
	fprintf(file, "  name               = %s\n", record->name);

	switch( itc503_motor->controller_interface.record->mx_class ) {
	case MXI_RS232:
		fprintf(file,
		      "  RS-232 interface   = %s\n",
			itc503_motor->controller_interface.record->name);
		break;
	case MXI_GPIB:
		fprintf(file,
		      "  GPIB interface     = %s\n",
		      	itc503_motor->controller_interface.record->name);
		fprintf(file,
		      "  GPIB address       = %ld\n",
		      	itc503_motor->controller_interface.address);
		break;
	default:
		fprintf(file,
		      "  Unsupported interface = %s\n",
		      	itc503_motor->controller_interface.record->name);
		break;
	}

	busy_deadband = motor->scale * itc503_motor->busy_deadband;

	fprintf(file, "  busy deadband      = %g %s  (%g K)\n",
		busy_deadband, motor->units,
		itc503_motor->busy_deadband );

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
mxd_itc503_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_itc503_motor_open()";

	MX_ITC503_MOTOR *itc503_motor;
	MX_RECORD *interface_record;
	long gpib_address;
	int c_command_value;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	itc503_motor =
		(MX_ITC503_MOTOR *) record->record_type_struct;

	if ( itc503_motor == (MX_ITC503_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_MOTOR pointer for analog input '%s' is NULL",
			record->name );
	}

	interface_record = itc503_motor->controller_interface.record;

	/* Construct a command that will be used to tell the controller
	 * to terminate responses only with a <CR> character.
	 */

	if ( itc503_motor->isobus_address < 0 ) {
		sprintf( command, "Q0" );

	} else if ( itc503_motor->isobus_address <= 9 ) {
		sprintf( command, "@%dQ0", itc503_motor->isobus_address );

	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal ISOBUS address %d specified for ITC503 controller '%s'.",
			itc503_motor->isobus_address, record->name );
	}

	switch( interface_record->mx_class ) {
	case MXI_RS232:
		/* Verify that the RS-232 port has the right settings. */

		mx_status = mx_rs232_verify_configuration( interface_record,
				9600, 8, 'N', 1, 'N', 0x0d, 0x0d );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Reinitialize the serial port. */

		mx_status = mx_resynchronize_record( interface_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(1000);

		/* Discard any characters waiting to be output. */

		mx_status = mx_rs232_discard_unwritten_output(
					interface_record, ITC503_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Send the Q0 command. */

		mx_status = mx_rs232_putline( interface_record, command,
						NULL, ITC503_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Wait a moment and then discard any characters in the
		 * input buffer.
		 */

		mx_msleep(1000);

		mx_status = mx_rs232_discard_unread_input(
					interface_record, ITC503_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXI_GPIB:
		/* Send the Q0 command. */

		gpib_address = itc503_motor->controller_interface.address;

		mx_status = mx_gpib_putline( interface_record, gpib_address,
					command, NULL, ITC503_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
"Only RS-232 and GPIB interfaces are supported for ITC503 controller '%s'.  "
"Interface record '%s' is of unsupported type '%s'.",
			record->name, interface_record->name,
			mx_get_driver_name( interface_record ) );
	}

	/* Ask for the version number of the controller. */

	mx_status = mxd_itc503_motor_command( itc503_motor, "V",
						response, sizeof(response),
						ITC503_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strncmp( response, "JET", 3 ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"ITC503 motor '%s' did not return the expected "
		"version string in its response to the V command.  "
		"Response = '%s'", record->name, response );
	}

	/* Send a 'Cn' control command.  See the header file
	 * 'd_itc503_motor.h' for a description of this command.
	 */

	c_command_value = (int) (( itc503_motor->itc503_motor_flags ) & 0x3);

	sprintf( command, "C%d", c_command_value );

	mx_status = mxd_itc503_motor_command( itc503_motor, command,
						response, sizeof(response),
						ITC503_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_itc503_motor_move_absolute()";

	MX_ITC503_MOTOR *itc503_motor;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_itc503_motor_get_pointers(motor, &itc503_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the controller is in remote control mode before trying
	 * to send a move command by getting the controller status.
	 */

	mx_status = mxd_itc503_motor_command( itc503_motor, "X",
						response, sizeof(response),
						ITC503_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[4] != 'C' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"ITC503 motor '%s' did not return the expected "
		"status message in its response to the X command.  "
		"Response = '%s'.", motor->record->name, response );
	}

	/* If the character after 'C' is either '1' or '3', we are in
	 * remote control mode and can proceed.
	 */

	if ( ( response[5] != '1' ) && ( response[5] != '3' ) ) {

		return mx_error( MXE_PERMISSION_DENIED, fname,
		"ITC503 motor '%s' cannot change the temperature "
		"since the controller is not in remote control mode.  "
		"To fix this, go to the front panel of the ITC503 "
		"controller, push the LOC/REM button, and select REMOTE.",
		motor->record->name );
	}

	/* Send the move command. */

	sprintf( command, "T%f", motor->raw_destination.analog );

	mx_status = mxd_itc503_motor_command( itc503_motor,
					command, response, sizeof(response),
					ITC503_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the motor as being busy. */

	motor->busy = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_itc503_motor_soft_abort()";

	MX_ITC503_MOTOR *itc503_motor;
	double position;
	mx_status_type mx_status;

	mx_status = mxd_itc503_motor_get_pointers(motor, &itc503_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current_temperature. */

	mx_status = mxd_itc503_motor_get_extended_status( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Command the ITC503 to move to the current temperature. */

	motor->raw_destination.analog = motor->raw_position.analog;

	position = motor->offset + motor->raw_position.analog * motor->scale;

	motor->position = position;
	motor->destination = position;

	mx_status = mxd_itc503_motor_move_absolute( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_itc503_motor_get_extended_status()";

	MX_ITC503_MOTOR *itc503_motor;
	char response[80];
	int num_items;
	double measured_temperature, set_temperature, temperature_error;
	mx_status_type mx_status;

	mx_status = mxd_itc503_motor_get_pointers(motor, &itc503_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the set temperature. */

	mx_status = mxd_itc503_motor_command( itc503_motor, "R0",
						response, sizeof(response),
						ITC503_MOTOR_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "R%lg", &set_temperature );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Did not find the set temperature in the response to the R0 command "
	"sent to ITC503 controller '%s'.  Response = '%s'",
			motor->record->name, response );
	}

	/* Read the current temperature error. */

	mx_status = mxd_itc503_motor_command( itc503_motor, "R4",
						response, sizeof(response),
						ITC503_MOTOR_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "R%lg", &temperature_error );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Did not find the temperature error in the response to the R4 command "
	"sent to ITC503 controller '%s'.  Response = '%s'",
			motor->record->name, response );
	}

	measured_temperature = set_temperature - temperature_error;

	motor->raw_position.analog = measured_temperature;

	/* Check to see if the temperature error is less than
	 * the busy deadband.  If not, the motor is marked
	 * as busy.
	 */

	if ( fabs( temperature_error ) < itc503_motor->busy_deadband ) {
		motor->status = 0;
	} else {
		motor->status = MXSF_MTR_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

/********/

static mx_status_type
mxd_itc503_motor_wait_for_rs232_response( MX_RECORD *rs232_record,
						char *command )
{
	static const char fname[]
		= "mxd_itc503_motor_wait_for_rs232)response()";

	int i, max_retries;
	unsigned long wait_ms;
	uint32_t num_input_bytes_available;
	mx_status_type mx_status;

	max_retries = 50;
	wait_ms = 100;

	for ( i = 0; i <= max_retries; i++ ) {
		mx_status = mx_rs232_num_input_bytes_available( rs232_record,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_input_bytes_available > 0 )
			break;		/* Exit the for() loop. */

		mx_msleep( wait_ms );
	}

	if ( i > max_retries ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Timed out waiting for a response to the command '%s' from "
	"the ITC503 series controller attached to RS-232 port '%s'.  "
	"Are you sure it is connected and turned on?",
			command, rs232_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_command( MX_ITC503_MOTOR *itc503_motor,
						char *command,
						char *response,
						size_t max_response_length,
						int debug_flag )
{
	static const char fname[] = "mxd_itc503_motor_command()";

	MX_RECORD *interface_record;
	long gpib_address;
	char local_command_buffer[100];
	char *command_ptr;
	size_t length;
	int i, error_occurred;
	mx_status_type mx_status;

	if ( itc503_motor == (MX_ITC503_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ITC503_MOTOR pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	interface_record = itc503_motor->controller_interface.record;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The interface record pointer for ITC503 motor '%s' is NULL.",
			itc503_motor->record->name );
	}

	/* Format the command to be sent. */

	if ( itc503_motor->isobus_address < 0 ) {
		command_ptr = command;
	} else {
		command_ptr = local_command_buffer;

		sprintf( local_command_buffer, "@%d%s",
				itc503_motor->isobus_address, command );
	}

	error_occurred = FALSE;

	for ( i = 0; i <= itc503_motor->maximum_retries; i++ ) {

		if ( i > 0 ) {
			mx_info( "ITC503 controller '%s' command retry #%d.",
				itc503_motor->record->name, i );
		}

		/* Send the command and get the response. */

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: sending command '%s' to '%s'.",
			    fname, command_ptr, itc503_motor->record->name));
		}

		error_occurred = FALSE;

		if ( interface_record->mx_class == MXI_RS232 ) {
			mx_status = mx_rs232_putline( interface_record,
						command_ptr, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( response != NULL ) {
				/* Wait for the response. */

				mx_status =
				    mxd_itc503_motor_wait_for_rs232_response(
						interface_record, command_ptr );

				if ( mx_status.code != MXE_SUCCESS ) {
					error_occurred = TRUE;
				} else {
					mx_status = mx_rs232_getline(
						interface_record, response,
						max_response_length, NULL, 0);

					if ( mx_status.code != MXE_SUCCESS ) {
						error_occurred = TRUE;
					} else {
						/* Remove any trailing carriage
						 * return characters.
						 */

						length = strlen( response );

						if (length <
							max_response_length )
						{
							if ( response[length-1]
								== MX_CR )
							{
							    response[length-1]
								= '\0';
							}
						}
					}
				}
			}
		} else {	/* GPIB */

			gpib_address =
				itc503_motor->controller_interface.address;

			mx_status = mx_gpib_putline(
						interface_record, gpib_address,
						command_ptr, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( response != NULL ) {
				mx_status = mx_gpib_getline(
					interface_record, gpib_address,
					response, max_response_length, NULL, 0);

				if ( mx_status.code != MXE_SUCCESS ) {
					error_occurred = TRUE;
				}
			}
		}

		if ( error_occurred == FALSE ) {

			/* If the first character in the response is a
			 * question mark '?', then an error occurred.
			 */

			if ( response != NULL ) {
				if ( response[0] == '?' ) {

					mx_status = mx_error(
						MXE_DEVICE_ACTION_FAILED, fname,
			"The command '%s' to ITC503 controller '%s' failed.  "
			"Controller error message = '%s'", command_ptr,
					itc503_motor->record->name, response );

					error_occurred = TRUE;
				} else {
					if ( debug_flag ) {
						MX_DEBUG(-2,
					("%s: received response '%s' from '%s'",
					fname, response,
					itc503_motor->record->name));
					}
				}
			}
		}

		if ( error_occurred == FALSE ) {
			break;		/* Exit the for() loop. */
		}
	}

	if ( error_occurred ) {
		return mx_error( MXE_TIMED_OUT, fname,
	"The command '%s' to ITC503 controller '%s' is still failing "
	"after %d retries.  Giving up...", command_ptr,
				itc503_motor->record->name,
				itc503_motor->maximum_retries );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

