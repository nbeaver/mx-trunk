/*
 * Name:    i_newport.c
 *
 * Purpose: MX driver for the Newport MM3000, MM4000/4005, and ESP
 *          series of motor controllers.
 *
 * Note:    For the MM4000, this driver _assumes_ that the value of the
 *          field "Terminator" under "General Setup" for the controller
 *          is set to CR/LF.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2004, 2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NEWPORT_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_generic.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_newport.h"

MX_RECORD_FUNCTION_LIST mxi_newport_record_function_list = {
	NULL,
	mxi_newport_create_record_structures,
	mxi_newport_finish_record_initialization,
	NULL,
	NULL,
	mxi_newport_open,
	NULL,
	NULL,
	mxi_newport_resynchronize
};

/* MM3000 interface data structures. */

MX_RECORD_FIELD_DEFAULTS mxi_mm3000_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NEWPORT_STANDARD_FIELDS
};

long mxi_mm3000_num_record_fields
		= sizeof( mxi_mm3000_record_field_defaults )
			/ sizeof( mxi_mm3000_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_mm3000_rfield_def_ptr
			= &mxi_mm3000_record_field_defaults[0];

/* MM4000 interface data structures. */

MX_RECORD_FIELD_DEFAULTS mxi_mm4000_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NEWPORT_STANDARD_FIELDS
};

long mxi_mm4000_num_record_fields
		= sizeof( mxi_mm4000_record_field_defaults )
			/ sizeof( mxi_mm4000_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_mm4000_rfield_def_ptr
			= &mxi_mm4000_record_field_defaults[0];

/* ESP interface data structures. */

MX_RECORD_FIELD_DEFAULTS mxi_esp_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NEWPORT_STANDARD_FIELDS
};

long mxi_esp_num_record_fields
		= sizeof( mxi_esp_record_field_defaults )
			/ sizeof( mxi_esp_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_esp_rfield_def_ptr
			= &mxi_esp_record_field_defaults[0];

/*==========================*/

/* Declare some private functions for the driver here. */

static mx_status_type mx_construct_mm3000_error_response(
					MX_NEWPORT *, char *, const char *);
static mx_status_type mx_construct_mm4000_error_response(
					MX_NEWPORT *, char *, const char *);
static mx_status_type mx_construct_esp_error_response(
					MX_NEWPORT *, char *, const char *);

/* A private function for the use of the driver. */

static mx_status_type
mxi_newport_get_pointers( MX_GENERIC *generic,
			MX_NEWPORT **newport,
			MX_INTERFACE **controller_interface,
			const char *calling_fname )
{
	const char fname[] = "mxi_newport_get_pointers()";

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GENERIC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( newport == (MX_NEWPORT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NEWPORT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( controller_interface == (MX_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( generic->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_GENERIC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*newport = (MX_NEWPORT *) generic->record->record_type_struct;

	if ( *newport == (MX_NEWPORT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NEWPORT pointer for record '%s' is NULL.",
			generic->record->name );
	}

	*controller_interface = &((*newport)->controller_interface);

	if ( *controller_interface == (MX_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE pointer for record '%s' is NULL.",
			generic->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_newport_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_newport_create_record_structures()";

	MX_GENERIC *generic;
	MX_NEWPORT *newport;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	newport = (MX_NEWPORT *) malloc( sizeof(MX_NEWPORT) );

	if ( newport == (MX_NEWPORT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_NEWPORT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = newport;
	record->class_specific_function_list = NULL;

	generic->record = record;
	newport->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_newport_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxi_newport_finish_record_initialization()";

	MX_NEWPORT *newport;
	MX_RECORD *interface_record;
	int i;

	newport = (MX_NEWPORT *) record->record_type_struct;

	if ( newport == (MX_NEWPORT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NEWPORT pointer for record '%s' is NULL.",
			record->name );
	}

	interface_record = newport->controller_interface.record;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Interface record pointer for Newport controller '%s' is NULL.",
			record->name );
	}

	/* Verify that the controller interface is either an RS-232
	 * interface or a GPIB interface.
	 */

	if ( interface_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an interface record.",
			interface_record->name );
	}

	if ( ( interface_record->mx_class != MXI_RS232 )
	  && ( interface_record->mx_class != MXI_GPIB ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an RS-232 or GPIB interface.",
			interface_record->name );
	}

	/* Mark the NEWPORT device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	/* Null out the list of attached motors. */

	for ( i = 0; i < MX_MAX_NEWPORT_MOTORS; i++ ) {
		newport->motor_record[i] = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_newport_open( MX_RECORD *record )
{
	const char fname[] = "mxi_newport_open()";

	MX_GENERIC *generic;
	MX_NEWPORT *newport;
	MX_INTERFACE *controller_interface;
	mx_status_type mx_status;

	MX_DEBUG(2, ("mxi_newport_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	generic = (MX_GENERIC *) record->record_class_struct;

	mx_status = mxi_newport_get_pointers( generic, &newport,
					&controller_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_newport_resynchronize( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* ******* Do controller type specific initialization. *******/

	if( record->mx_type == MXI_CTRL_MM3000 ) {

		/* Set error reporting to be compatible with the way
		 * the MM4000 is handled.  This means the following
		 * bits in the FO word are always set to particular
		 * values:
		 *
		 * B0 (set to 0) - Enable appended text and long error codes.
		 * B1 (set to 1) - Disable automatic RS232 error message.
		 */

		mx_status = mxi_newport_putline( newport,
			"FO|02;FO&FE", FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Disable the long BEEEEEEEPs after any error, since the
		 * controller doesn't send any characters out to the
		 * communications port until after the beep is over.
		 *
		 * This is done by setting bit B6 in the FS word.
		 */

		mx_status = mxi_newport_putline( newport, "FS|40", FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	MX_DEBUG(2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_newport_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxi_newport_resynchronize()";

	MX_GENERIC *generic;
	MX_NEWPORT *newport;
	MX_INTERFACE *controller_interface;
	char response[80], banner[40];
	size_t length;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	generic = (MX_GENERIC *) record->record_class_struct;

	mx_status = mxi_newport_get_pointers( generic, &newport,
					&controller_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any unread input or unwritten output. */

	mx_status = mxi_newport_discard_unwritten_output( generic,
							MXI_NEWPORT_DEBUG );

	if ( (mx_status.code != MXE_SUCCESS)
	  && (mx_status.code != MXE_UNSUPPORTED) ) {
		return mx_status;
	}

	mx_status = mxi_newport_discard_unread_input(
						generic, MXI_NEWPORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send an identify command to verify that the controller is there. */

	if ( record->mx_type == MXI_CTRL_ESP ) {
		mx_status = mxi_newport_command( newport, "VE?",
			response, sizeof response, MXI_NEWPORT_DEBUG );
	} else {
		mx_status = mxi_newport_command( newport, "VE",
			response, sizeof response, MXI_NEWPORT_DEBUG );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the command timed out. */

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NOT_READY:
		return mx_error( MXE_NOT_READY, fname,
		"No response from Newport controller '%s'.  Is it turned on?",
			record->name );
	default:
		return mx_status;
	}

	/* Verify that we got the identification message that we expected. */

	switch( record->mx_type ) {
	case MXI_CTRL_MM3000:
		strcpy( banner, "Newport Corp. MM3000 Version" );
		break;
	case MXI_CTRL_MM4000:
		strcpy( banner, " MM400" );
		break;
	case MXI_CTRL_ESP:
		strcpy( banner, "ESP" );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Attempted to resynchronize a record that is not a Newport motor controller.  "
"Instead, it is of type %ld.", record->mx_type );
	}

	length = strlen( banner );

	/* Sometimes the first character of the response gets lost, so we
	 * will try the banner comparison first as is and if that fails
	 * we will try it again after skipping the first character.
	 */

	if ( ( strncmp( response, banner, length ) != 0 )
	  && ( strncmp( response, banner+1, length-1 ) != 0 ) )
	{
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"The device for interface '%s' is not an MM3000, MM4000, or ESP series "
"controller.  The response to a 'VE' command was '%s'",
			record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_newport_discard_unread_input( MX_GENERIC *generic, int debug_flag )
{
	const char fname[] = "mxi_newport_discard_unread_input()";

	MX_NEWPORT *newport;
	MX_INTERFACE *controller_interface;
	mx_status_type mx_status;

	mx_status = mxi_newport_get_pointers( generic, &newport,
					&controller_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( controller_interface->record->mx_class == MXI_RS232 ) {

		mx_status = mx_rs232_discard_unread_input(
					controller_interface->record,
					debug_flag );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_newport_discard_unwritten_output( MX_GENERIC *generic, int debug_flag )
{
	const char fname[] = "mxi_newport_discard_unwritten_output()";

	MX_NEWPORT *newport;
	MX_INTERFACE *controller_interface;
	mx_status_type mx_status;

	mx_status = mxi_newport_get_pointers( generic, &newport,
					&controller_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( controller_interface->record->mx_class == MXI_RS232 ) {

		mx_status = mx_rs232_discard_unwritten_output(
					controller_interface->record,
					debug_flag );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
	return mx_status;
}

/* === Functions specific to this driver. === */

MX_EXPORT mx_status_type
mxi_newport_turn_motor_drivers_on( MX_NEWPORT *newport, int debug_flag )
{
	mx_status_type mx_status;

	mx_status = mxi_newport_command( newport, "MO", NULL, 0, debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_newport_command( MX_NEWPORT *newport, char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	const char fname[] = "mxi_newport_command()";

	MX_GENERIC *generic;
	unsigned long sleep_ms;
	unsigned long i, j, max_attempts;
	size_t prefix_length, response_length;
	char local_error_buffer[250];
	int local_error_buffer_length;
	mx_status_type mx_status, command_status;

	if ( newport == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_NEWPORT pointer passed was NULL." );
	}

	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	local_error_buffer_length = sizeof(local_error_buffer) - 1;

	if ( local_error_buffer_length < 4 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Error message buffer length of %d is too short.  Must be at least 4.",
			local_error_buffer_length );
	}

	generic = (MX_GENERIC *) newport->record->record_class_struct;

	if ( generic == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GENERIC pointer for NEWPORT record '%s' is NULL.",
			newport->record->name );
	}

	/* Send the command string. */

	mx_status = mxi_newport_putline( newport, command, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response, if one is expected. */

	i = 0;

	if ( newport->record->mx_type == MXI_CTRL_MM3000 ) {
		max_attempts = 2;
		sleep_ms = 1000;
	} else {
		max_attempts = 1;
		sleep_ms = 0;
	}

	if ( response != NULL ) {
		for ( i=0; i < max_attempts; i++ ) {
			if ( i > 0 ) {
				MX_DEBUG(-2,
			("%s: No response yet to '%s' command.  Retrying...",
					newport->record->name, command ));
			}

			mx_status = mxi_newport_getline( newport, response,
					response_buffer_length, debug_flag );

			if ( mx_status.code == MXE_SUCCESS ) {
				break;		/* Exit the for() loop. */

			} else if ( mx_status.code != MXE_NOT_READY ) {
				MX_DEBUG(-2,
			("*** Exiting with status = %ld",mx_status.code));
				return mx_status;
			}
			mx_msleep(sleep_ms);
		}

		if ( i >= max_attempts ) {
			mx_status = mxi_newport_discard_unread_input(
						generic, debug_flag );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread characters in buffer for record '%s'",
					newport->record->name );
			}

			mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"No response seen to '%s' command", command );
		}

		if ( newport->record->mx_type == MXI_CTRL_MM4000 ) {

			/* Most (but _not_ _all_) response strings from
			 * an MM4000 includes the command prefix (if any)
			 * followed by the two letter command (converted
			 * to uppercase).  The data for the response
			 * follows after.
			 * 
			 * For compatibility with the MM3000, we trim
			 * the echoed command and prefix off of the
			 * front of the response string, checking that
			 * we got a valid looking response first.
			 * Note that this code assumes that there are
			 * no unnecessary spaces in the command sent
			 * to the Newport controller.
			 */

			/* First, how long is the command prefix if present? 
			 * The prefix is assumed to be an integer.
			 */

			prefix_length = strspn( command, "0123456789" );

			for ( i = 0; i < (prefix_length + 2); i++ ) {
			    if ( response[i] != toupper( (int)(command[i]) ) ) {

				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Command and prefix in response '%s' does not match original command '%s'",
					response, command );
			    }
			}

			response_length = strlen( response );

			for ( i = 0, j = prefix_length + 2; 
			   i <= response_length; i++, j++ ) {
				response[i] = response[j];
			}
		}
	}

	/* Check to see if there was an error in the command we just did. */

	if ( newport->record->mx_type == MXI_CTRL_ESP ) {
		mx_status = mxi_newport_putline( newport, "TB?", debug_flag );
	} else {
		mx_status = mxi_newport_putline( newport, "TB", debug_flag );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read back the error string. */

	if ( i > 0 ) {
		mx_msleep( (i+1) * sleep_ms);
	}

	mx_status = mxi_newport_getline( newport, local_error_buffer,
			local_error_buffer_length, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* MM3000's and MM4000's have differently formatted responses
	 * to the TB command.
	 */

	command_status = MX_SUCCESSFUL_RESULT;

	switch ( newport->record->mx_type ) {
	case MXI_CTRL_MM3000:
		if ( local_error_buffer[0] != 'E' ) {
			mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Response to TB command was _not_ an error message string.  Response = '%s'",
				local_error_buffer );
		}

		if (strncmp( local_error_buffer, "E00", 3 ) != 0) {

			command_status = mx_construct_mm3000_error_response(
				newport, local_error_buffer, fname );
		}
		break;

	case MXI_CTRL_MM4000:
		if ( strcmp( local_error_buffer, "TB@ No error" ) != 0 ) {

			command_status = mx_construct_mm4000_error_response(
				newport, local_error_buffer, fname );
		}
		break;

	case MXI_CTRL_ESP:
		if ( local_error_buffer[0] != '0' ) {
			command_status = mx_construct_esp_error_response(
				newport, local_error_buffer, fname );
		}
		break;

	default:
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"Controller %s is not a Newport MM3000, MM4000, or ESP series controller.  "
"Device type = %ld.", newport->record->name, newport->record->mx_type );
	}

	if ( command_status.code != MXE_SUCCESS ) {
		mx_status = mxi_newport_discard_unread_input(
							generic, debug_flag );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread characters in buffer for record '%s'",
				newport->record->name );
		}
	}

	return command_status;
}

MX_EXPORT mx_status_type
mxi_newport_getline( MX_NEWPORT *newport,
		char *buffer, long buffer_length, int debug_flag )
{
	const char fname[] = "mxi_newport_getline()";

	MX_INTERFACE *controller_interface;
	mx_status_type mx_status;

	if ( newport == (MX_NEWPORT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NEWPORT pointer passed was NULL." );
	}
	if ( buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	controller_interface = &(newport->controller_interface);

	if ( controller_interface->record->mx_class == MXI_RS232 ) {

		mx_status = mx_rs232_getline( controller_interface->record,
					buffer, buffer_length, NULL, 0 );
	} else {
		mx_status = mx_gpib_getline( controller_interface->record,
					controller_interface->address,
					buffer, buffer_length, NULL, 0 );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_newport_putline( MX_NEWPORT *newport, char *buffer, int debug_flag )
{
	const char fname[] = "mxi_newport_putline()";

	MX_INTERFACE *controller_interface;
	mx_status_type mx_status;

	if ( newport == (MX_NEWPORT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NEWPORT pointer passed was NULL." );
	}
	if ( buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
	}

	controller_interface = &(newport->controller_interface);

	if ( controller_interface->record->mx_class == MXI_RS232 ) {
		mx_status = mx_rs232_putline( controller_interface->record,
					buffer, NULL, 0 );
	} else {
		mx_status = mx_gpib_putline( controller_interface->record,
					controller_interface->address,
					buffer, NULL, 0 );
	}

	return mx_status;
}

static mx_status_type
mx_construct_mm3000_error_response( MX_NEWPORT *newport,
				char *error_message,
				const char *fname )
{
	/* See Appendix A of the MM3000 User's Manual for what the
	 * original MM3000 error codes mean.
	 */

	static long translated_error_code[] = {
		MXE_SUCCESS,			/*  0 */
		MXE_INTERFACE_IO_ERROR,		/*  1 */
		MXE_DEVICE_IO_ERROR,		/*  2 */
		MXE_INTERFACE_IO_ERROR,		/*  3 */
		MXE_DEVICE_IO_ERROR,		/*  4 */
		MXE_DEVICE_IO_ERROR,		/*  5 */
		MXE_NOT_READY,			/*  6 */
		MXE_DEVICE_IO_ERROR,		/*  7 */
		MXE_DEVICE_ACTION_FAILED,	/*  8 */
		MXE_DEVICE_ACTION_FAILED,	/*  9 */
		MXE_DEVICE_ACTION_FAILED,	/* 10 */
		MXE_DEVICE_ACTION_FAILED,	/* 11 */
		MXE_INTERFACE_ACTION_FAILED,	/* 12 */
		MXE_INTERRUPTED,		/* 13 */
		MXE_OUT_OF_MEMORY,		/* 14 */
		MXE_ILLEGAL_ARGUMENT,		/* 15 */
		MXE_ILLEGAL_ARGUMENT,		/* 16 */
		MXE_INTERFACE_ACTION_FAILED,	/* 17 */
		MXE_ILLEGAL_ARGUMENT,		/* 18 */
		MXE_INTERFACE_ACTION_FAILED,	/* 19 */
		MXE_INTERFACE_ACTION_FAILED,	/* 20 */
		MXE_INTERFACE_ACTION_FAILED,	/* 21 */
		MXE_INTERFACE_ACTION_FAILED,	/* 22 */
		MXE_INTERFACE_IO_ERROR,		/* 23 */
		MXE_ILLEGAL_ARGUMENT,		/* 24 */
		MXE_CONTROLLER_INTERNAL_ERROR,	/* 25 */
		MXE_INTERFACE_IO_ERROR,		/* 26 */
		MXE_INTERFACE_IO_ERROR,		/* 27 */
		MXE_CONTROLLER_INTERNAL_ERROR,	/* 28 */
		MXE_NOT_READY,			/* 29 */
		MXE_INTERFACE_ACTION_FAILED,	/* 30 */
		MXE_INTERFACE_ACTION_FAILED,	/* 31 */
		MXE_INTERFACE_ACTION_FAILED,	/* 32 */
		MXE_DEVICE_ACTION_FAILED,	/* 33 */
		MXE_CONTROLLER_INTERNAL_ERROR,	/* 34 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 35 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 36 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 37 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 38 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 39 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 40 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 41 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 42 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 43 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 44 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 45 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 46 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 47 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 48 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 49 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 50 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 51 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 52 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 53 */
		MXE_LIMIT_WAS_EXCEEDED,		/* 54 */
		MXE_DEVICE_ACTION_FAILED,	/* 55 */
		MXE_DEVICE_ACTION_FAILED,	/* 56 */
		MXE_DEVICE_ACTION_FAILED,	/* 57 */
	};

	static int num_translated_codes = sizeof( translated_error_code )
				/ sizeof( translated_error_code[0] );

	long mx_error_code;
	int mm3000_error_code, num_tokens;

	num_tokens = sscanf( error_message, "E%d", &mm3000_error_code );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"No error code seen in response to TB command for controller '%s'.  "
	"Response = '%s'",
			newport->record->name,
			error_message );
	}

	if ( mm3000_error_code < 0
	  || mm3000_error_code >= num_translated_codes ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Illegal error code %d seen in response to TB command for controller '%s'.  "
"Response = '%s'", mm3000_error_code, newport->record->name, error_message );
	}

	mx_error_code = translated_error_code[ mm3000_error_code ];

	return mx_error( mx_error_code, fname,
		"MM3000 controller '%s' error message = '%s'",
			newport->record->name, error_message );
}
	

static mx_status_type
mx_construct_mm4000_error_response( MX_NEWPORT *newport,
				char *error_message,
				const char *fname )
{
	/* See Appendix A of the MM4000 User's Manual and the Appendix of
	 * the MM4000 User's Manual Addendum for what the original MM4000
	 * error codes mean.
	 */

	static struct {
		char mm4000_error_code;
		long mx_error_code;
	} error_code_table[] = {
		{'A',	MXE_CONTROLLER_INTERNAL_ERROR},
		{'B',	MXE_ILLEGAL_ARGUMENT},
		{'C',	MXE_ILLEGAL_ARGUMENT},
		{'D',	MXE_DEVICE_ACTION_FAILED},
		{'E',	MXE_ILLEGAL_ARGUMENT},
		{'F',	MXE_ILLEGAL_ARGUMENT},
		{'G',	MXE_ILLEGAL_ARGUMENT},
		{'H',	MXE_INTERFACE_ACTION_FAILED},
		{'I',	MXE_ILLEGAL_ARGUMENT},
		{'J',	MXE_ILLEGAL_ARGUMENT},
		{'K',	MXE_ILLEGAL_ARGUMENT},
		{'L',	MXE_INTERFACE_IO_ERROR},
		{'M',	MXE_OUT_OF_MEMORY},
		{'N',	MXE_ILLEGAL_ARGUMENT},
		{'O',	MXE_ILLEGAL_ARGUMENT},
		{'P',	MXE_ILLEGAL_ARGUMENT},
		{'Q',	MXE_ILLEGAL_ARGUMENT},
		{'R',	MXE_ILLEGAL_ARGUMENT},
		{'S',	MXE_INTERFACE_IO_ERROR},
		{'T',	MXE_DEVICE_ACTION_FAILED},
		{'U',	MXE_CONTROLLER_INTERNAL_ERROR},
		{'V',	MXE_ILLEGAL_ARGUMENT},
		{'W',	MXE_ILLEGAL_ARGUMENT},
		{'X',	MXE_ILLEGAL_ARGUMENT},
		{'Y',	MXE_ILLEGAL_ARGUMENT},
		{'Z',	MXE_ILLEGAL_ARGUMENT},
		{'[',	MXE_ILLEGAL_ARGUMENT},
		{'\\',	MXE_ILLEGAL_ARGUMENT},
		{']',	MXE_ILLEGAL_ARGUMENT},
		{'^',	MXE_ILLEGAL_ARGUMENT},
		{'_',	MXE_ILLEGAL_ARGUMENT},
		{'`',	MXE_ILLEGAL_ARGUMENT},
		{'a',	MXE_ILLEGAL_ARGUMENT},
		{'b',	MXE_ILLEGAL_ARGUMENT},
		{'c',	MXE_ILLEGAL_ARGUMENT},
		{'d',	MXE_ILLEGAL_ARGUMENT},
		{'e',	MXE_ILLEGAL_ARGUMENT},
		{'f',	MXE_ILLEGAL_ARGUMENT},
		{'g',	MXE_ILLEGAL_ARGUMENT},
		{'h',	MXE_WOULD_EXCEED_LIMIT}
	};

	static int num_error_codes = sizeof( error_code_table )
				/ sizeof( error_code_table[0] );

	long mx_error_code;
	int i, num_tokens;
	char mm4000_error_code;
	mx_status_type mx_status;

	num_tokens = sscanf( error_message, "TB%c", &mm4000_error_code );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"No error code seen in response to TB command for controller '%s'.  "
	"Response = '%s'", newport->record->name, error_message );
	}

	for ( i = 0; i < num_error_codes; i++ ) {
		if (error_code_table[i].mm4000_error_code == mm4000_error_code)
			break;		/* exit the for() loop */
	}

	if ( i >= num_error_codes ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Illegal error code '%d' seen in response to TB command for controller '%s'.  "
"Response = '%s'", mm4000_error_code, newport->record->name, error_message );
	}

	mx_error_code = error_code_table[i].mx_error_code;

	if ( mm4000_error_code == 'D' ) {
		mx_status = mx_error( mx_error_code, fname,
	"The motor driver power is probably turned off.  "
	"Try pressing the 'Motor on' button on MM4000 controller '%s'.  "
	"MM4000 error message = '%s'",
				newport->record->name,
				error_message );

	} else if ( mm4000_error_code == 'Q' ) {
		mx_status = mx_error( mx_error_code, fname,
	"The front panel menus for MM4000 controller '%s' "
	"are probably not at the top level.  "
	"Try pressing QUIT and/or NEXT menu items until you get to a menu "
	"titled 'Select Action'.  "
	"MM4000 error message = '%s'",
				newport->record->name,
				error_message );
	} else {
		mx_status = mx_error( mx_error_code, fname,
			"MM4000 controller '%s' error message = '%s'",
				newport->record->name, error_message );
	}
	return mx_status;
}

static mx_status_type
mx_construct_esp_error_response( MX_NEWPORT *newport,
				char *error_message,
				const char *fname )
{
	/* See Appendix A of the ESP User's Manual for what the original ESP
	 * error codes mean.
	 */

	static long no_axis_error_code_table[] = {
	MXE_SUCCESS,			/* 0, No error detected. */
	MXE_INTERFACE_IO_ERROR,		/* 1, PCI communication timeout. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 2, Reserved. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 3, Reserved. */
	MXE_INTERRUPTED,		/* 4, Emergency stop activated. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 5, Reserved. */
	MXE_ILLEGAL_ARGUMENT,		/* 6, Command does not exist. */
	MXE_WOULD_EXCEED_LIMIT,		/* 7, Parameter out of range. */
	MXE_HARDWARE_CONFIGURATION_ERROR, /* 8, Cable interlock error. */
	MXE_ILLEGAL_ARGUMENT,		/* 9, Axis number out of range. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 10, Reserved. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 11, Reserved. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 12, Reserved. */
	MXE_ILLEGAL_ARGUMENT,		/* 13, Group number missing. */
	MXE_ILLEGAL_ARGUMENT,		/* 14, Group number out of range. */
	MXE_ILLEGAL_ARGUMENT,		/* 15, Group number not assigned. */
	MXE_ILLEGAL_ARGUMENT,		/* 16, Group number already assigned. */
	MXE_ILLEGAL_ARGUMENT,		/* 17, Group axis out of range. */
	MXE_ILLEGAL_ARGUMENT,		/* 18, Group axis already assigned. */
	MXE_ILLEGAL_ARGUMENT,		/* 19, Group axis duplicated. */
	MXE_NOT_READY,			/* 20, Data acquisition is busy. */
	MXE_HARDWARE_CONFIGURATION_ERROR, /* 21, Data acquisition setup error.*/
	MXE_HARDWARE_CONFIGURATION_ERROR, /* 22, Data acquisition not enabled.*/
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 23, Servo tick failure. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 24, Reserved. */
	MXE_NOT_READY,			/* 25, Download in progress. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 26, Stored program not started. */
	MXE_ILLEGAL_ARGUMENT,		/* 27, Command not allowed. */
	MXE_INTERFACE_ACTION_FAILED,	/* 28, Stored program flash area full.*/
	MXE_ILLEGAL_ARGUMENT,		/* 29, Group parameter missing. */
	MXE_WOULD_EXCEED_LIMIT,		/* 30, Group parameter out of range. */
	MXE_WOULD_EXCEED_LIMIT,		/* 31, Group max velocity exceeded. */
	MXE_WOULD_EXCEED_LIMIT,		/* 32, Group max accel exceeded. */
	MXE_WOULD_EXCEED_LIMIT,		/* 33, Group max decel exceeded. */
	MXE_NOT_READY,			/* 34, No group move during homing. */
	MXE_NOT_FOUND,			/* 35, Program not found. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* 36, Reserved. */
	MXE_ILLEGAL_ARGUMENT,		/* 37, Axis number missing. */
	MXE_ILLEGAL_ARGUMENT,		/* 38, Command parameter missing. */
	MXE_ILLEGAL_ARGUMENT,		/* 39, Program label not found. */
	MXE_ILLEGAL_ARGUMENT,		/* 40, Last cmd cannot be repeated. */
	MXE_LIMIT_WAS_EXCEEDED,		/* 41, Max num prog labels exceeded. */
	};

	static int num_no_axis_error_codes = sizeof(no_axis_error_code_table)
					/ sizeof(no_axis_error_code_table[0]);

	static long axis_error_code_table[] = {
	MXE_HARDWARE_CONFIGURATION_ERROR, /* x00, Motor type not defined. */
	MXE_ILLEGAL_ARGUMENT,		/* x01, Parameter out of range. */
	MXE_HARDWARE_FAULT,		/* x02, Amplifier fault detected. */
	MXE_LIMIT_WAS_EXCEEDED,		/* x03, Following err thresh exceeded.*/
	MXE_LIMIT_WAS_EXCEEDED,		/* x04, Positive HW limit detected. */
	MXE_LIMIT_WAS_EXCEEDED,		/* x05, Negative HW limit detected. */
	MXE_LIMIT_WAS_EXCEEDED,		/* x06, Positive SW limit detected. */
	MXE_LIMIT_WAS_EXCEEDED,		/* x07, Negative SW limit detected. */
	MXE_HARDWARE_CONFIGURATION_ERROR, /* x08, Motor/stage not connected. */
	MXE_HARDWARE_FAULT,		/* x09, Feedback signal fault detected*/
	MXE_WOULD_EXCEED_LIMIT,		/* x10, Maximum velocity exceeded. */
	MXE_WOULD_EXCEED_LIMIT,		/* x11, Maximum accel exceeded. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* x12, Reserved. */
	MXE_NOT_READY,			/* x13, Motor not enabled. */
	MXE_CONTROLLER_INTERNAL_ERROR,	/* x14, Reserved. */
	MXE_WOULD_EXCEED_LIMIT,		/* x15, Maximum jerk exceeded. */
	MXE_WOULD_EXCEED_LIMIT,		/* x16, Maximum DAC offset exceeded. */
	MXE_PERMISSION_DENIED,		/* x17, ESP crit. settings protected. */
	MXE_HARDWARE_FAULT,		/* x18, ESP stage device error. */
	MXE_HARDWARE_FAULT,		/* x19, ESP stage data invalid. */
	MXE_INTERRUPTED,		/* x20, Homing aborted. */
	MXE_HARDWARE_CONFIGURATION_ERROR, /* x21, Motor current not defined. */
	MXE_HARDWARE_FAULT,		/* x22, Unidrive communication error. */
	MXE_HARDWARE_FAULT,		/* x23, Unidrive not detected. */
	MXE_WOULD_EXCEED_LIMIT,		/* x24, Speed out of range. */
	MXE_ILLEGAL_ARGUMENT,		/* x25, Invalid trajectory master axis*/
	MXE_NOT_READY,			/* x26, No param change during motion.*/
	MXE_ILLEGAL_ARGUMENT,		/* x27, Invalid home trajectory mode. */
	MXE_ILLEGAL_ARGUMENT,		/* x28, Invalid encoder step ratio. */
	MXE_HARDWARE_FAULT,		/* x29, Digital I/O interlock detected*/
	MXE_NOT_READY,			/* x30, Cmd not allowed during homing.*/
	MXE_ILLEGAL_ARGUMENT,		/* x31, Illegal cmd due to group assgn*/
	MXE_ILLEGAL_ARGUMENT,		/* x32, Invalid move trajectory mode. */
	};

	static int num_axis_error_codes = sizeof(axis_error_code_table)
					/ sizeof(axis_error_code_table[0]);

	MX_RECORD *motor_record;
	long mx_error_code;
	int axis_number, num_tokens;
	int esp_error_code, axis_error_code;
	char *ptr, *error_text;
	mx_status_type mx_status;

	/* Get the error code. */

	MX_DEBUG( 2,("%s: error_message = '%s'", fname, error_message));

	num_tokens = sscanf( error_message, "%d,", &esp_error_code );

	MX_DEBUG( 2,("%s: esp_error_code = %d", fname, esp_error_code));

	if ( num_tokens != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"No error code seen in response to TB? command for ESP controller '%s'.  "
"Response = '%s'", newport->record->name, error_message );
	}

	/* Find the error text. */

	ptr = strchr( error_message, ',' );

	if ( ptr == NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Corrupted response to TB? command for ESP controller '%s'.  "
	"No comma characters seen in response = '%s'",
			newport->record->name, error_message );
	}

	ptr++;

	ptr = strchr( ptr, ',' );

	if ( ptr == NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Corrupted response to TB? command for ESP controller '%s'.  "
	"Only one comma character seen in response = '%s'",
			newport->record->name, error_message );
	}

	error_text = ptr + 2;

	MX_DEBUG( 2,("%s: error_text = '%s'", fname, error_text));

	/* Is the error message axis specific? */

	if ( esp_error_code < 100 ) {
		/* Not axis specific. */

		if ( esp_error_code > num_no_axis_error_codes ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Illegal error code '%d' seen in response to TB? "
			"command for ESP controller '%s'.  Response = '%s'",
				esp_error_code,
				newport->record->name,
				error_message );
		}

		mx_error_code = no_axis_error_code_table[ esp_error_code ];

		mx_status = mx_error( mx_error_code, fname,
		"ESP controller '%s' error '%s'.  Error code = %d.",
			newport->record->name, error_text, esp_error_code );
	} else {
		/* Axis specific. */
		axis_number = esp_error_code / 100;

		axis_error_code = esp_error_code % 100;

		if ( axis_error_code > num_axis_error_codes ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Illegal error code '%d' seen in response to TB? command.  Response = '%s'",
				esp_error_code, error_message );
		}

		mx_error_code = axis_error_code_table[ axis_error_code ];

		if ( ( axis_number > MX_MAX_NEWPORT_MOTORS )
		  || ( axis_number <= 0 ) )
		{
			motor_record = NULL;
		} else {
			motor_record = newport->motor_record[axis_number - 1];
		}

		if ( motor_record == (MX_RECORD *) NULL ) {
			mx_status = mx_error( mx_error_code, fname,
	"ESP controller '%s', axis %d, error '%s', error code = %d.",
			newport->record->name, axis_number,
			error_text, esp_error_code );
		} else {
			mx_status = mx_error( mx_error_code, fname,
	"ESP controller '%s', motor '%s', error '%s', error code = %d.",
			newport->record->name, motor_record->name,
			error_text, esp_error_code );
		}
	}
	return mx_status;
}

