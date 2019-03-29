/*
 * Name:    i_picomotor.c
 *
 * Purpose: MX driver for New Focus Picomotor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2007, 2010, 2017, 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PICOMOTOR_DEBUG		FALSE

#define MXI_PICOMOTOR_DEBUG_TIMING	FALSE

#define MXI_PICOMOTOR_DEBUG_GETLINE	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_hrt.h"
#include "mx_rs232.h"
#include "i_picomotor.h"

MX_RECORD_FUNCTION_LIST mxi_picomotor_record_function_list = {
	NULL,
	mxi_picomotor_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_picomotor_open,
	NULL,
	NULL,
	mxi_picomotor_resynchronize,
	mxi_picomotor_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_picomotor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PICOMOTOR_STANDARD_FIELDS
};

long mxi_picomotor_num_record_fields
		= sizeof( mxi_picomotor_record_field_defaults )
			/ sizeof( mxi_picomotor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_picomotor_rfield_def_ptr
			= &mxi_picomotor_record_field_defaults[0];

static mx_status_type mxi_picomotor_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/* A private function for the use of the driver. */

static mx_status_type
mxi_picomotor_get_pointers( MX_RECORD *record,
			MX_PICOMOTOR_CONTROLLER **picomotor_controller,
			const char *calling_fname )
{
	static const char fname[] = "mxi_picomotor_get_pointers()";

	MX_PICOMOTOR_CONTROLLER *picomotor_controller_pointer;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	picomotor_controller_pointer =
		(MX_PICOMOTOR_CONTROLLER *) record->record_type_struct;

	if ( picomotor_controller_pointer == (MX_PICOMOTOR_CONTROLLER *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PICOMOTOR_CONTROLLER pointer for record '%s' is NULL.",
			record->name );
	}

	if ( picomotor_controller != (MX_PICOMOTOR_CONTROLLER **) NULL ) {
		*picomotor_controller = picomotor_controller_pointer;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_picomotor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_picomotor_create_record_structures()";

	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	long i, j;

	/* Allocate memory for the necessary structures. */

	picomotor_controller = (MX_PICOMOTOR_CONTROLLER *)
			malloc( sizeof(MX_PICOMOTOR_CONTROLLER) );

	if ( picomotor_controller == (MX_PICOMOTOR_CONTROLLER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PICOMOTOR_CONTROLLER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = picomotor_controller;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	picomotor_controller->record = record;

	for ( i = 0; i < MX_MAX_PICOMOTOR_DRIVERS; i++ ) {
		for ( j = 0; j < MX_MAX_PICOMOTORS_PER_DRIVER; j++ ) {
			picomotor_controller->motor_record_array[i][j] = NULL;
		}
	}

	/* According to the vendor, Picomotor controllers do not like it
	 * if you send a new command within 70 milliseconds of the last
	 * command.
	 */

	picomotor_controller->minimum_time_between_commands
		= mx_convert_seconds_to_high_resolution_time(
			0.001 * (double) MX_MINIMUM_PICOMOTOR_COMMAND_INTERVAL);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_picomotor_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_picomotor_open()";

	MX_PICOMOTOR_CONTROLLER *picomotor_controller = NULL;
	MX_RS232 *rs232 = NULL;
	mx_status_type mx_status;

	picomotor_controller = NULL;

	mx_status = mxi_picomotor_get_pointers( record,
					&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( picomotor_controller->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the RS-232 record "
		"used by Picomotor controller '%s' is NULL.", record->name );
	}

	rs232 = (MX_RS232 *)
		picomotor_controller->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for the RS-232 record "
		"used by Picomotor controller '%s' is NULL.", record->name );
	}

#if 0
	{
		MX_RECORD *motor_record;
		int i, j;

		for ( i = 0; i < MX_MAX_PICOMOTOR_DRIVERS; i++ ) {
			for ( j = 0; j < MX_MAX_PICOMOTORS_PER_DRIVER; j++ ) {
				motor_record =
				 picomotor_controller->motor_record_array[i][j];

				if ( motor_record == NULL ) {
					MX_DEBUG(-2,("%s: motor[%d][%d] = NULL",
					    fname, i, j));
				} else {
					MX_DEBUG(-2,("%s: motor[%d][%d] = '%s'",
					    fname, i, j, motor_record->name));
				}
			}
		}
	}
#endif

	picomotor_controller->next_command_time = mx_high_resolution_time();

	mx_status = mx_rs232_verify_configuration(
			picomotor_controller->rs232_record,
			19200, 8, MXF_232_NO_PARITY, 1,
			MXF_232_NO_FLOW_CONTROL, 0x0d0a, 0x0d0a,
			rs232->timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Resynchronize with the controller. */

	mx_status = mxi_picomotor_resynchronize( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_picomotor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_picomotor_resynchronize()";

	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char *response_ptr;
	int i, num_items, driver_number, raw_driver_type, flags;
	long current_motor_number;
	mx_status_type mx_status;

	picomotor_controller = NULL;

	mx_status = mxi_picomotor_get_pointers( record,
					&picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get rid of any existing characters in the input and output
	 * buffers and do it quietly.
	 */

	mx_status = mx_rs232_discard_unwritten_output(
				picomotor_controller->rs232_record, FALSE );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;		/* Continue on. */
	default:
		return mx_status;
	}

	mx_status = mx_rs232_discard_unread_input(
				picomotor_controller->rs232_record, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the controller is there by asking it for its
	 * firmware_version.
	 */

	mx_status = mxi_picomotor_command( picomotor_controller, "VER",
				response, sizeof(response),
				MXI_PICOMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The response may begin with a '>' or '?' character. */

	if ( ( response[0] == '>' ) || ( response[0] == '?' ) ) {
		response_ptr = response + 1;
	} else {
		response_ptr = response;
	}

	num_items = sscanf( response_ptr, "Version %d.%d.%d",
			&(picomotor_controller->firmware_major_version),
			&(picomotor_controller->firmware_minor_version),
			&(picomotor_controller->firmware_revision) );

	if ( num_items != 3 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Controller '%s' cannot be a Picomotor controller, "
		"since it did not give a valid response to a 'VER' command.  "
		"Instead, it gave the response '%s'",
			record->name, response );
	}

	mx_msleep( MX_MINIMUM_PICOMOTOR_COMMAND_INTERVAL );

	/* Find out what kinds of drivers are used by this controller. */

	mx_status = mxi_picomotor_command( picomotor_controller, "DRT", NULL, 0,
					MXI_PICOMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < MX_MAX_PICOMOTOR_DRIVERS; i++ ) {

		/* Read out the response lines from the controller. */

		if ( i == 0 ) {
			flags = MXI_PICOMOTOR_DEBUG;
		} else {
			flags =
		  MXI_PICOMOTOR_DEBUG | MXF_PICOMOTOR_TERMINATE_ON_STATUS_CHAR;
		}

		mx_status = mxi_picomotor_getline( picomotor_controller,
					response, sizeof(response), flags );

		if ( mx_status.code == MXE_END_OF_DATA ) {
			break;		/* Exit the for() loop. */
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXI_PICOMOTOR_DEBUG
		MX_DEBUG(-2,("%s: Line %d = '%s'", fname, i, response));
#endif

		num_items = sscanf( response, "A%d=%d",
				&driver_number, &raw_driver_type );

		if ( num_items != 2 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No driver type seen in response to 'DRT' command.  "
			"Response seen = '%s'", response );
		}

#if 0
		MX_DEBUG(-2,("%s: driver_number = %d, raw_driver_type = %d",
			fname, driver_number, raw_driver_type));
#endif

		if ( (driver_number < 1)
		  || (driver_number > MX_MAX_PICOMOTOR_DRIVERS) )
		{
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Driver number %d found in the response '%s' "
			"to the 'DRT' command is outside the allowed "
			"range of 1 to %d.",
			driver_number, response, MX_MAX_PICOMOTOR_DRIVERS );
		}

		switch( raw_driver_type ) {
		case 1:
			picomotor_controller->driver_type[driver_number - 1]
					= MXT_PICOMOTOR_8753_DRIVER;
#if MXI_PICOMOTOR_DEBUG
			MX_DEBUG(-2,("%s: Picomotor controller %d is an 8753",
				fname, driver_number));
#endif
			break;
		case 2:
			picomotor_controller->driver_type[driver_number - 1]
					= MXT_PICOMOTOR_8751_DRIVER;
#if MXI_PICOMOTOR_DEBUG
			MX_DEBUG(-2,("%s: Picomotor controller %d is an 8751",
				fname, driver_number));
#endif
			break;
		default:
			picomotor_controller->driver_type[driver_number - 1]
					= MXT_PICOMOTOR_UNKNOWN_DRIVER;

			mx_warning( "Unrecognized driver type %d returned "
			"for 'DRT' command sent to Picomotor controller '%s'.  "
			"Response = '%s'.",
			    raw_driver_type, record->name, response );
			break;
		}
#if 0
		MX_DEBUG(-2,("%s: driver_number = %d, driver_type[%d] = %ld",
			fname, driver_number, driver_number - 1,
			picomotor_controller->driver_type[driver_number - 1]));
#endif
	}

	/* Set the 'current_motor_number' entries for all drivers. */

	for ( i = 0; i < MX_MAX_PICOMOTOR_DRIVERS; i++ ) {

		switch( picomotor_controller->driver_type[i] ) {
		case MXT_PICOMOTOR_8753_DRIVER:
			snprintf( command, sizeof(command),
				"CHL A%d", i+1 );

			mx_status = mxi_picomotor_command(
				picomotor_controller,
				command, response, sizeof(response),
				MXI_PICOMOTOR_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%ld",
					    &current_motor_number );

			if ( num_items != 1 ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Cannot find the current motor number in the "
				"response '%s' to command '%s' for "
				"Picomotor interface '%s'.",
					response, command, record->name );
			}

			picomotor_controller->current_motor_number[i]
				= current_motor_number;
			break;
		default:
			picomotor_controller->current_motor_number[i] = -1;
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxi_picomotor_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_picomotor_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_PICOMOTOR_COMMAND:
		case MXLV_PICOMOTOR_RESPONSE:
		case MXLV_PICOMOTOR_COMMAND_WITH_RESPONSE:
			record_field->process_function
					    = mxi_picomotor_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_picomotor_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxi_picomotor_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	picomotor_controller = (MX_PICOMOTOR_CONTROLLER *)
					record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_PICOMOTOR_RESPONSE:
			/* Nothing to do since the necessary string is
			 * already stored in the 'response' field.
			 */

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_PICOMOTOR_COMMAND:
			mx_status = mxi_picomotor_command(
				picomotor_controller,
				picomotor_controller->command,
				NULL, 0, MXI_PICOMOTOR_DEBUG );

			break;
		case MXLV_PICOMOTOR_COMMAND_WITH_RESPONSE:
			mx_status = mxi_picomotor_command(
				picomotor_controller,
				picomotor_controller->command,
				picomotor_controller->response,
				MXU_PICOMOTOR_MAX_COMMAND_LENGTH,
				MXI_PICOMOTOR_DEBUG );

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

/* === Functions specific to this driver. === */

#if MXI_PICOMOTOR_DEBUG_TIMING	
#include "mx_hrt_debug.h"
#endif

static mx_status_type
mxi_picomotor_handle_errors(
		MX_PICOMOTOR_CONTROLLER *picomotor_controller,
		char *response_buffer,
		char response_status )
{
	static const char fname[] = "mxi_picomotor_handle_errors()";

	MX_DEBUG( 2,("%s: Warning or error response = '%s'",
		fname, response_buffer ));

	return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Warning or error in response '%s' for Picomotor "
		"controller '%s'.", response_buffer,
		picomotor_controller->record->name );
}

MX_EXPORT mx_status_type
mxi_picomotor_command( MX_PICOMOTOR_CONTROLLER *picomotor_controller,
		char *command,
		char *response, size_t max_response_length,
		int command_flags )
{
	static const char fname[] = "mxi_picomotor_command()";

	char response_buffer[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	struct timespec current_time;
	int i, max_attempts, comparison;
	mx_status_type mx_status;

#if MXI_PICOMOTOR_DEBUG_TIMING	
	MX_HRT_RS232_TIMING command_timing, response_timing;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( picomotor_controller == (MX_PICOMOTOR_CONTROLLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PICOMOTOR_CONTROLLER pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}
	if ( picomotor_controller->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for record '%s' is NULL.",
			picomotor_controller->record->name );
	}

	/* The Picomotor controller does not like it if we send commands
	 * to it too close together in time.  If we are trying to send a
	 * command before 'next_command_time', wait until next_command_time
	 * has arrived.
	 */

	for (;;) {
		current_time = mx_high_resolution_time();

		comparison = mx_compare_high_resolution_times(
				picomotor_controller->next_command_time,
				current_time );

		if ( comparison <= 0 ) {
			/* The 'next_command_time' has arrived, so break
			 * out of the for(;;) loop.
			 */

			 break;
		}
	}

	/* How many times do we retry sending the command? */

	if ( command_flags & MXF_PICOMOTOR_NO_COMMAND_RETRY ) {
		max_attempts = 1;
	} else {
		max_attempts = 5;
	}

	for ( i = 0; i < max_attempts; i++ ) {
		/* Send the command. */

		if ( command_flags & MXF_PICOMOTOR_DEBUG ) {
			MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, picomotor_controller->record->name));
		}

#if MXI_PICOMOTOR_DEBUG_TIMING	
		MX_HRT_RS232_START_COMMAND( command_timing,
						2 + strlen(command) );
#endif

		mx_status = mx_rs232_putline(
					picomotor_controller->rs232_record,
					command, NULL, 0 );

#if MXI_PICOMOTOR_DEBUG_TIMING
		MX_HRT_RS232_END_COMMAND( command_timing );
#endif

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the response, if one is expected. */

		response_buffer[0] = '\0';

		if ( response == (char *) NULL ) {

#if MXI_PICOMOTOR_DEBUG_TIMING
			MX_HRT_RS232_COMMAND_RESULTS( command_timing,
				command, fname );
#endif
			/* No response is expected, so break out of
			 * the command retry loop.
			 */

			break;
		} else {

#if MXI_PICOMOTOR_DEBUG_TIMING
			MX_HRT_RS232_START_RESPONSE( response_timing, NULL );
#endif
			mx_status = mxi_picomotor_getline( picomotor_controller,
				response_buffer, sizeof(response_buffer),
				command_flags );

#if MXI_PICOMOTOR_DEBUG_TIMING
			MX_HRT_RS232_END_RESPONSE( response_timing,
						strlen(response_buffer) );
			MX_HRT_RS232_COMMAND_RESULTS( command_timing,
							command, fname );
			MX_HRT_TIME_BETWEEN_MEASUREMENTS( command_timing,
							response_timing, fname);
			MX_HRT_RS232_RESPONSE_RESULTS( response_timing,
							response_buffer, fname);
#endif
			if ( mx_status.code == MXE_SUCCESS ) {
				/* We successfully received a command response,
				 * so break out of the for() loop.
				 */

				break;
			} else
			if ( mx_status.code == MXE_TIMED_OUT ) {
				/* Did not receive a response. */

				mx_warning(
"No response received for the command '%s' to Picomotor controller '%s'."
"  Attempting to retry the command.  Retry attempt %d.",
					fname, command, i+1 );
			} else {
				/* For any other error, abort now. */

				return mx_status;
			}
		}

		mx_msleep( MX_MINIMUM_PICOMOTOR_COMMAND_INTERVAL );
	}

	/* Did we exceed the maximum number of retries? */

	if ( i >= max_attempts ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unable to get a response from Picomotor controller '%s' "
		"after %d attempts to send the command '%s'.",
			picomotor_controller->record->name,
			max_attempts, command );
	}

	/* If the caller wanted a response, copy it from the
	 * response buffer.
	 */

	if ( response != (char *) NULL ) {

		strlcpy( response, response_buffer, max_response_length );

		if ( command_flags & MXF_PICOMOTOR_DEBUG ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, response,
				picomotor_controller->record->name));
		}
	}

	/* Calculate the next time that a command will be allowed. */

	current_time = mx_high_resolution_time();

	picomotor_controller->next_command_time = mx_add_high_resolution_times(
	    current_time, picomotor_controller->minimum_time_between_commands );

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_picomotor_getline( MX_PICOMOTOR_CONTROLLER *picomotor_controller,
		char *response,
		size_t max_response_length,
		int command_flags )
{
	static const char fname[] = "mxi_picomotor_getline()";

	MX_RS232 *rs232;
	char c, termchar;
	char *termarray;
	long i, start_index, num_terminator_chars, num_terminator_chars_seen;
	mx_status_type mx_status;

#if MXI_PICOMOTOR_DEBUG_GETLINE
	MX_DEBUG(-2,("%s: command_flags = %#x", fname, command_flags));
#endif

	rs232 = (MX_RS232 *)
		picomotor_controller->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for RS232 record '%s' "
		"used by '%s' is NULL.",
			picomotor_controller->rs232_record->name,
			picomotor_controller->record->name );
	}

	/*
	 * At the beginning of each line, the controller may transmit
	 * character that tells you whether the most recent command 
	 * succeeded or not.
	 *
	 * > means the command succeeded.
	 * ? means the command did not succeed.
	 */

	start_index = 0;

	mx_status = mx_rs232_getchar_with_timeout(
				picomotor_controller->rs232_record,
				&c,
				MXF_232_WAIT, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Was there a warning or error in the response?  If so,
	 * return an appropriate error to the caller.
	 */

	switch( c ) {
	case '>':
		/* There was no error. */
#if MXI_PICOMOTOR_DEBUG_GETLINE
		MX_DEBUG(-2,("%s: > character seen.", fname));
#endif
		if ( command_flags & MXF_PICOMOTOR_TERMINATE_ON_STATUS_CHAR ) {
			return mx_error( (MXE_END_OF_DATA | MXE_QUIET), fname,
				"No more output available." );
		}
		break;
	case '?':
		return mxi_picomotor_handle_errors( picomotor_controller,
							response, c );
	default:
		/* The returned character was not a status character,
		 * so we just copy it to the response buffer.
		 */

#if MXI_PICOMOTOR_DEBUG_GETLINE
		MX_DEBUG(-2,("%s: char #0 = %#x '%c'", fname, c, c));
#endif

		if ( max_response_length > 1 ) {
			response[0] = c;
			start_index = 1;
		}
		break;
	}

	/* The Picomotor controller transmits a character after the
	 * line terminators that we need, but the 'tcp232' driver will
	 * currently throw that character away if we use getline, so
	 * we read it a character at a time instead.
	 */

	termarray = rs232->read_terminator_array;
	num_terminator_chars = rs232->num_read_terminator_chars;

	num_terminator_chars_seen = 0;

	for ( i = start_index; i < max_response_length; i++ ) {
	
		if ( num_terminator_chars_seen >= num_terminator_chars ) {
			break;		/* Exit the for() loop. */
		}

		mx_status = mx_rs232_getchar_with_timeout(
			picomotor_controller->rs232_record,
			&c, MXF_232_WAIT, 1.0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXI_PICOMOTOR_DEBUG_GETLINE
		MX_DEBUG(-2,("%s: char #%ld = %#x '%c'", fname, i, c, c));
#endif

		termchar = termarray[ num_terminator_chars_seen ];

		if ( c == termchar ) {
			num_terminator_chars_seen++;

			continue; /* Go to the top of the for() loop. */
		}

		response[i] = c;
	}

	if ( i >= max_response_length ) {
		return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
		"The number of characters in the response from "
		"controller '%s' exceeded the response buffer "
		"size of %lu",
			picomotor_controller->record->name,
			(unsigned long) max_response_length );
	}

	/* Null terminate the response. */

	response[i - num_terminator_chars] = '\0';

	return MX_SUCCESSFUL_RESULT;
}

