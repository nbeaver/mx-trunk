/*
 * Name:    i_scipe.c
 *
 * Purpose: MX driver for SCIPE servers using the SCIPE protocol created
 *          by John Quintana of Northwestern University.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_generic.h"
#include "mx_rs232.h"
#include "i_scipe.h"

MX_RECORD_FUNCTION_LIST mxi_scipe_record_function_list = {
	mxi_scipe_initialize_type,
	mxi_scipe_create_record_structures,
	mxi_scipe_finish_record_initialization,
	mxi_scipe_delete_record,
	NULL,
	mxi_scipe_read_parms_from_hardware,
	mxi_scipe_write_parms_to_hardware,
	mxi_scipe_open,
	mxi_scipe_close,
	NULL,
	mxi_scipe_resynchronize,
};

MX_GENERIC_FUNCTION_LIST mxi_scipe_generic_function_list = {
	mxi_scipe_getchar,
	mxi_scipe_putchar,
	mxi_scipe_read,
	mxi_scipe_write,
	mxi_scipe_num_input_bytes_available,
	mxi_scipe_discard_unread_input,
	mxi_scipe_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_scipe_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_SCIPE_SERVER_STANDARD_FIELDS
};

long mxi_scipe_num_record_fields
		= sizeof( mxi_scipe_record_field_defaults )
			/ sizeof( mxi_scipe_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_scipe_rfield_def_ptr
			= &mxi_scipe_record_field_defaults[0];

#define MXI_SCIPE_SERVER_DEBUG		FALSE

/* A private function for the use of the driver. */

static mx_status_type
mxi_scipe_get_pointers( MX_GENERIC *generic,
			MX_SCIPE_SERVER **scipe_server,
			const char *calling_fname )
{
	static const char fname[] = "mxi_scipe_get_pointers()";

	MX_RECORD *scipe_server_record;

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GENERIC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scipe_server == (MX_SCIPE_SERVER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCIPE_SERVER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	scipe_server_record = generic->record;

	if ( scipe_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The scipe_server_record pointer for the "
			"MX_GENERIC pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*scipe_server = (MX_SCIPE_SERVER *)
			scipe_server_record->record_type_struct;

	if ( *scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCIPE_SERVER pointer for record '%s' is NULL.",
			scipe_server_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_scipe_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scipe_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_scipe_create_record_structures()";

	MX_GENERIC *generic;
	MX_SCIPE_SERVER *scipe_server;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	scipe_server = (MX_SCIPE_SERVER *) malloc( sizeof(MX_SCIPE_SERVER) );

	if ( scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_SCIPE_SERVER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = scipe_server;
	record->class_specific_function_list
				= &mxi_scipe_generic_function_list;

	generic->record = record;
	scipe_server->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scipe_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scipe_delete_record( MX_RECORD *record )
{
	MX_SCIPE_SERVER *scipe_server;

        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }

	scipe_server = (MX_SCIPE_SERVER *) record->record_type_struct;

        if ( scipe_server != NULL ) {
                free( scipe_server );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scipe_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scipe_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scipe_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_scipe_open()";

	MX_SCIPE_SERVER *scipe_server;
	MX_RS232 *rs232;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	scipe_server = (MX_SCIPE_SERVER *) (record->record_type_struct);

	if ( scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCIPE_SERVER pointer for record '%s' is NULL.",
		record->name);
	}

	/* Are the line terminators set correctly? */

	if ( scipe_server->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"rs232_record pointer for SCIPE interface '%s' is NULL.",
			record->name );
	}

	rs232 = (MX_RS232 *)
		scipe_server->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RS232 pointer for RS-232 record '%s' is NULL.",
			scipe_server->rs232_record->name );
	}

	if ( (rs232->read_terminators != 0x0d0a)
	  || (rs232->write_terminators != 0x0d0a) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The SCIPE interface '%s' requires that the line terminators "
	"of RS-232 record '%s' be a carriage return followed by a line feed.  "
	"Instead saw read terminator %#lx and write terminator %#lx.",
		record->name, scipe_server->rs232_record->name,
		rs232->read_terminators, rs232->write_terminators );
	}

	/* Synchronize with the SCIPE server. */

	status = mxi_scipe_resynchronize( record );

	return status;
}

MX_EXPORT mx_status_type
mxi_scipe_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_scipe_close()";

	MX_SCIPE_SERVER *scipe_server;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	scipe_server = (MX_SCIPE_SERVER *) (record->record_type_struct);

	if ( scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCIPE_SERVER pointer for record '%s' is NULL.",
		record->name);
	}

	status = mxi_scipe_command( scipe_server, "scipe quit",
					NULL, 0, NULL, NULL,
					MXI_SCIPE_SERVER_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxi_scipe_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_scipe_resynchronize()";

	MX_GENERIC *generic;
	MX_SCIPE_SERVER *scipe_server;
	char site_name[80], scipe_name[80];
	char response[80];
	char *result_ptr;
	int num_items, response_code;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	generic = (MX_GENERIC *) record->record_class_struct;

	status = mxi_scipe_get_pointers( generic,
				&scipe_server, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Throw away any pending input and output on the SCIPE port. */

	status = mx_rs232_discard_unwritten_output( scipe_server->rs232_record,
						MXI_SCIPE_SERVER_DEBUG );

	switch( status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;		/* Continue on. */
	default:
		return status;
	}

	status = mx_rs232_discard_unread_input( scipe_server->rs232_record,
						MXI_SCIPE_SERVER_DEBUG);

	switch( status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;		/* Continue on. */
	default:
		return status;
	}

	/* Verify that the SCIPE server is there by asking for its
	 * description.
	 */

	status = mxi_scipe_command( scipe_server, "scipe desc",
				response, sizeof response,
				&response_code, &result_ptr,
				MXI_SCIPE_SERVER_DEBUG);
	
	switch( status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_INTERFACE_IO_ERROR:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Cannot communicate with the SCIPE server at port '%s'.  "
"Is it running?", scipe_server->rs232_record->name );
	default:
		return status;
	}

	MX_DEBUG( 2,("%s: response_code = %d, result_ptr = '%s'",
			fname, response_code, result_ptr ));

	/* Parse the returned server description. */

	num_items = sscanf( result_ptr, "%s %s", site_name, scipe_name );

	if ( num_items != 2 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unable to interpret the response to a 'scipe desc' command.  "
		"Response = '%s'", response );
	}

	MX_DEBUG( 2,("%s: site_name = '%s', scipe_name = '%s'",
			fname, site_name, scipe_name));

	if ( strcmp( scipe_name, "SCIPE" ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The server '%s' does not appear to be a SCIPE server.  "
		"The response to a 'scipe desc' command was '%s'",
			scipe_server->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_scipe_getchar( MX_GENERIC *generic, char *c, int flags )
{
	static const char fname[] = "mxi_scipe_getchar()";

	MX_SCIPE_SERVER *scipe_server;
	mx_status_type status;

	status = mxi_scipe_get_pointers( generic,
				&scipe_server, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_getchar( scipe_server->rs232_record,
					c, flags );

	return status;
}

MX_EXPORT mx_status_type
mxi_scipe_putchar( MX_GENERIC *generic, char c, int flags )
{
	static const char fname[] = "mxi_scipe_putchar()";

	MX_SCIPE_SERVER *scipe_server;
	mx_status_type status;

	status = mxi_scipe_get_pointers( generic,
				&scipe_server, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_putchar( scipe_server->rs232_record,
					c, flags );

	return status;
}

MX_EXPORT mx_status_type
mxi_scipe_read( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_scipe_read()";

	MX_SCIPE_SERVER *scipe_server;
	mx_status_type status;

	status = mxi_scipe_get_pointers( generic,
				&scipe_server, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_read( scipe_server->rs232_record,
				buffer, count, NULL, 0 );

	return status;
}

MX_EXPORT mx_status_type
mxi_scipe_write( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_scipe_write()";

	MX_SCIPE_SERVER *scipe_server;
	mx_status_type status;

	status = mxi_scipe_get_pointers( generic,
				&scipe_server, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_write( scipe_server->rs232_record,
					buffer, count, NULL, 0 );

	return status;
}

MX_EXPORT mx_status_type
mxi_scipe_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available )
{
	static const char fname[] = "mxi_scipe_num_input_bytes_available()";

	MX_SCIPE_SERVER *scipe_server;
	mx_status_type status;

	status = mxi_scipe_get_pointers( generic,
				&scipe_server, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_num_input_bytes_available(
			scipe_server->rs232_record,
			num_input_bytes_available );

	return status;
}

MX_EXPORT mx_status_type
mxi_scipe_discard_unread_input( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_scipe_discard_unread_input()";

	MX_SCIPE_SERVER *scipe_server;
	mx_status_type status;

	status = mxi_scipe_get_pointers( generic,
				&scipe_server, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_discard_unread_input(
					scipe_server->rs232_record,
					debug_flag );

	return status;
}

MX_EXPORT mx_status_type
mxi_scipe_discard_unwritten_output( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_scipe_discard_unwritten_output()";

	MX_SCIPE_SERVER *scipe_server;
	mx_status_type status;

	status = mxi_scipe_get_pointers( generic,
				&scipe_server, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_discard_unwritten_output(
					scipe_server->rs232_record,
					debug_flag );

	return status;
}

/* === Functions specific to this driver. === */

MX_EXPORT mx_status_type
mxi_scipe_command( MX_SCIPE_SERVER *scipe_server, char *command,
		char *response, int response_buffer_length,
		int *response_code, char **result_ptr,
		int debug_flag )
{
	static const char fname[] = "mxi_scipe_command()";

	unsigned long sleep_ms;
	int i, max_attempts, num_items;
	mx_status_type status;

	MX_DEBUG(2,("%s invoked.", fname));

	if ( scipe_server == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_SCIPE_SERVER pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.");
	}

	/* Send the command string. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s'", fname, command ));
	}

	status = mx_rs232_putline( scipe_server->rs232_record,
					command, NULL, debug_flag );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Get the response, if one is expected. */

	i = 0;

	max_attempts = 1;
	sleep_ms = 0;

	if ( response != NULL ) {

		/* Read in the SCIPE server response. */

		for ( i=0; i < max_attempts; i++ ) {
			if ( i > 0 ) {
				MX_DEBUG(-2,
			("%s: No response yet to '%s' command.  Retrying...",
				scipe_server->record->name, command ));
			}

			status = mx_rs232_getline( scipe_server->rs232_record,
					response, response_buffer_length,
					NULL, debug_flag );

			if ( status.code == MXE_SUCCESS ) {
				break;		/* Exit the for() loop. */

			} else if ( status.code != MXE_NOT_READY ) {
				MX_DEBUG(-2,
				("*** Exiting with status = %ld",status.code));
				return status;
			}
			mx_msleep(sleep_ms);
		}

		if ( i >= max_attempts ) {
			status = mx_rs232_discard_unread_input(
					scipe_server->rs232_record,
					debug_flag );

			if ( status.code != MXE_SUCCESS ) {
				mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread characters in buffer for record '%s'",
					scipe_server->record->name );
			}

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"No response seen to '%s' command", command );
		}

		if ( status.code != MXE_SUCCESS )
			return status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s'", fname, response));
		}

		/* Get the response code if requested. */

		if ( response_code != NULL ) {
			num_items = sscanf( response, "%d", response_code );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Cannot find the response code in the SCIPE server response '%s'",
					response );
			}
		}

		/* Get a pointer to the result string if requested. */

		if ( result_ptr != NULL ) {

			/* If it is present, the result string will be
			 * separated from the response code by a space.
			 */

			*result_ptr = strchr( response, ' ' );

			/* Skip the first space character. */

			if ( *result_ptr != NULL ) {
				(*result_ptr)++;
			}
		}
	}
	MX_DEBUG(2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

static struct {
	int response_code;
	char error_message[MX_SCIPE_ERROR_MESSAGE_LENGTH+1];
} mxi_scipe_error_message_table[]
	= {
{ MXF_SCIPE_OK,				"OK" },
{ MXF_SCIPE_OK_WITH_RESULT,		"OK with result" },
{ MXF_SCIPE_NOT_MOVING,			"Not moving" },
{ MXF_SCIPE_MOVING,			"Moving" },
{ MXF_SCIPE_ACTUATOR_GENERAL_FAULT,	"Actuator general fault" },
{ MXF_SCIPE_UPPER_BOUND_FAULT,		"Upper bound fault" },
{ MXF_SCIPE_LOWER_BOUND_FAULT,		"Lower bound fault" },
{ MXF_SCIPE_NOT_COLLECTING,		"Not collecting" },
{ MXF_SCIPE_COLLECTING,			"Collecting" },
{ MXF_SCIPE_DETECTOR_GENERAL_FAULT,	"Detector general fault" },
{ MXF_SCIPE_COMMAND_NOT_FOUND,		"Command not found" },
{ MXF_SCIPE_OBJECT_NOT_FOUND,		"Object not found" },
{ MXF_SCIPE_ERROR_IN_COMMAND,		"Error in command" },
{ MXF_SCIPE_ERROR_IN_OBJECT_PROCESSING,	"Error in object processing" }
};

static int
mxi_scipe_num_error_messages = ( sizeof( mxi_scipe_error_message_table )
				/ sizeof( mxi_scipe_error_message_table[0] ) );


MX_EXPORT char *
mxi_scipe_strerror( int response_code )
{
	char *message;
	int i;

	message = NULL;

	for ( i = 0; i < mxi_scipe_num_error_messages; i++ ) {

		if ( response_code
			== (mxi_scipe_error_message_table[i]).response_code )
		{
			message
			    = mxi_scipe_error_message_table[i].error_message;

			break;
		}
	}

	return message;
}

MX_EXPORT long
mxi_scipe_get_mx_status( int response_code )
{
	static const char fname[] = "mxi_scipe_get_mx_status()";

	long mx_status_code;

	mx_status_code = MXE_SUCCESS;

	if ( response_code < MXF_SCIPE_COMMAND_NOT_FOUND ) {
		return mx_status_code;
	}

	switch( response_code ) {
	case MXF_SCIPE_OBJECT_NOT_FOUND:
		mx_status_code = MXE_NOT_FOUND;
		break;
	case MXF_SCIPE_COMMAND_NOT_FOUND:
	case MXF_SCIPE_ERROR_IN_COMMAND:
		mx_status_code = MXE_INTERFACE_IO_ERROR;
		break;
	case MXF_SCIPE_ERROR_IN_OBJECT_PROCESSING:
		mx_status_code = MXE_DEVICE_IO_ERROR;
		break;
	default:
		(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"SCIPE returned an unknown response code %d.",
			response_code );

		mx_status_code = MXE_INTERFACE_IO_ERROR;
		break;
	}

	return mx_status_code;
}
