/*
 * Name:    i_galil_gclib.c
 *
 * Purpose: MX driver for Galil motor controllers via the gclib library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_GALIL_GCLIB_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_process.h"
#include "mx_ascii.h"
#include "mx_motor.h"
#include "mx_analog_output.h"
#include "i_galil_gclib.h"
#include "d_galil_gclib_motor.h"
#include "d_galil_gclib_aoutput.h"

MX_RECORD_FUNCTION_LIST mxi_galil_gclib_record_function_list = {
	NULL,
	mxi_galil_gclib_create_record_structures,
	mxi_galil_gclib_finish_record_initialization,
	NULL,
	NULL,
	mxi_galil_gclib_open,
	NULL,
	NULL,
	NULL,
	mxi_galil_gclib_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_galil_gclib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_GALIL_GCLIB_STANDARD_FIELDS
};

long mxi_galil_gclib_num_record_fields
		= sizeof( mxi_galil_gclib_record_field_defaults )
			/ sizeof( mxi_galil_gclib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_galil_gclib_rfield_def_ptr
			= &mxi_galil_gclib_record_field_defaults[0];

/*---*/

static mx_status_type mxi_galil_gclib_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_galil_gclib_create_record_structures()";

	MX_GALIL_GCLIB *galil_gclib = NULL;

	/* Allocate memory for the necessary structures. */

	galil_gclib = (MX_GALIL_GCLIB *) malloc( sizeof(MX_GALIL_GCLIB) );

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for an MX_GALIL_GCLIB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = galil_gclib;

	record->record_function_list = &mxi_galil_gclib_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	galil_gclib->record = record;
	galil_gclib->error_code = 0;
	galil_gclib->gclib_status = 0;
	galil_gclib->command_file[0] = '\0';

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_galil_gclib_finish_record_initialization()";

	MX_GALIL_GCLIB *galil_gclib = NULL;
	unsigned long flags;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	galil_gclib = (MX_GALIL_GCLIB *) record->record_type_struct;

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GALIL_GCLIB pointer for record '%s' is NULL.",
			record->name );
	}

	flags = galil_gclib->galil_gclib_flags;

	flags = flags;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_galil_gclib_open()";

	MX_GALIL_GCLIB *galil_gclib = NULL;
	GReturn gclib_status;
	char command[80];
	char response[80];
	char connection_string[MXU_HOSTNAME_LENGTH + 8];
	int version_argc;
	char **version_argv;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	galil_gclib = (MX_GALIL_GCLIB *) record->record_type_struct;

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GALIL_GCLIB pointer for record '%s' is NULL.",
			record->name );
	}

#if MXI_GALIL_GCLIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	gclib_status = GVersion( galil_gclib->gclib_version,
				sizeof(galil_gclib->gclib_version) );

	if ( gclib_status != G_NO_ERROR ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to get the version of Galil gclib for record '%s' "
		"failed with status = %d.", record->name, (int) gclib_status );
	}

#if MXI_GALIL_GCLIB_DEBUG
	MX_DEBUG(-2,("%s: Galil gclib version = '%s'.",
		fname, galil_gclib->gclib_version ));
#endif
	/* Make the connection to this controller. */

	snprintf( connection_string, sizeof(connection_string),
		"%s -d", galil_gclib->hostname );

	gclib_status = GOpen( connection_string, &(galil_gclib->connection) );

	if ( gclib_status != G_NO_ERROR ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to open a Galil controller connection for "
		"record '%s' at '%s' failed with status = %d.",
		record->name, galil_gclib->hostname, (int) gclib_status );
	}

#if MXI_GALIL_GCLIB_DEBUG
	MX_DEBUG(-2,("%s: galil_gclib->connection = %p",
		fname, galil_gclib->connection));
#endif

	/* Print out some information from the controller. */

	gclib_status = GInfo( galil_gclib->connection,
				response, sizeof(response) );

	if ( gclib_status != G_NO_ERROR ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to get the connection information for "
		"Galil controller '%s' failed with status = %d.",
			record->name, (int) gclib_status );
	}

	MX_DEBUG(-2,("%s: Connection info = '%s'", fname, response ));

	mx_status = mxi_galil_gclib_command( galil_gclib,
			"MG TIME", response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: MG TIME response = '%s'", fname, response ));

	snprintf( command, sizeof(command),
		"%c%c", MX_CTRL_R, MX_CTRL_V );

	mx_status = mxi_galil_gclib_command( galil_gclib,
			command, response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_string_split( response, " ", &version_argc, &version_argv );

	MX_DEBUG(-2,("%s: version_argc = %d", fname, version_argc ));

#if 1
	MX_DEBUG(-2,("MARKER 1"));

	if ( version_argc >= 3 ) {
		MX_DEBUG(-2,("MARKER 2.1, version_argc = %d", version_argc));
	} else {
		MX_DEBUG(-2,("MARKER 2.2, version_argc = %d", version_argc));
		mx_free( version_argv );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Did not find the controller type and firmware revision "
		"in the response '%s' to command 'ctrl-R ctrl-V' for "
		"Galil controller '%s'.  Only %d tokens were seen, "
		"instead of 3 tokens.",
			response, record->name, version_argc );
	}

	MX_DEBUG(-2,("MARKER 3"));
#endif

	strlcpy( galil_gclib->controller_type,
		version_argv[0], sizeof(galil_gclib->controller_type) );

	strlcpy( galil_gclib->firmware_revision,
		version_argv[2], sizeof(galil_gclib->firmware_revision) );

	mx_free( version_argv );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_GALIL_GCLIB_COMMAND:
		case MXLV_GALIL_GCLIB_COMMAND_FILE:
		case MXLV_GALIL_GCLIB_ERROR_CODE:
		case MXLV_GALIL_GCLIB_GCLIB_STATUS:
			record_field->process_function
					= mxi_galil_gclib_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxi_galil_gclib_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxi_galil_gclib_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_GALIL_GCLIB *galil_gclib;
	char command[80], response[80];
	int num_items;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	galil_gclib = (MX_GALIL_GCLIB *) (record->record_type_struct);

	galil_gclib = galil_gclib;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_GALIL_GCLIB_ERROR_CODE:
			strlcpy( command, "TC1", sizeof(command) );

			mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lu",
					&(galil_gclib->error_code) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Could not find the error code in the "
				"response '%s' to command '%s' for "
				"Galil controller '%s'.",
					response, command, record->name );
			}
			break;

		case MXLV_GALIL_GCLIB_GCLIB_STATUS:
			/* Nothing to do.  We just report the most recently
			 * recived gclib status.
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
		case MXLV_GALIL_GCLIB_COMMAND:
			mx_status = mxi_galil_gclib_command( galil_gclib,
				galil_gclib->command,
				galil_gclib->response,
				MXU_GALIL_GCLIB_BUFFER_LENGTH );
			break;
		case MXLV_GALIL_GCLIB_COMMAND_FILE:
			mx_status = mxi_galil_gclib_run_command_file(
				galil_gclib, galil_gclib->command_file );
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

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_command( MX_GALIL_GCLIB *galil_gclib,
		char *command, char *response, size_t response_length )
{
	static const char fname[] = "mxi_galil_gclib_command()";

	GReturn gclib_status;
	unsigned long flags;
	mx_bool_type debug_flag;
	char *ptr = NULL;
	char local_response[100];
	char *buffer_ptr = NULL;
	size_t buffer_length;

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_GALIL_GCLIB pointer passed was NULL." );
	}

	if ( (response == NULL) || (response_length == 0) ) {
		buffer_ptr = local_response;
		buffer_length = sizeof(local_response);
	} else {
		buffer_ptr = response;
		buffer_length = response_length;
	}

	flags = galil_gclib->galil_gclib_flags;

	if ( flags & MXF_GALIL_GCLIB_DEBUG_COMMANDS ) {
		debug_flag = TRUE;
	} else {
		debug_flag = FALSE;
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
		fname, command, galil_gclib->record->name ));
	}

	gclib_status = GCommand( galil_gclib->connection,
			command, buffer_ptr, buffer_length, 0 );

	galil_gclib->gclib_status = gclib_status;

	switch( gclib_status ) {
	case G_NO_ERROR:
		break;
	case G_BAD_RESPONSE_QUESTION_MARK:
		/* If we get this status code, then send a TC (Tell Error Code)
		 * command to the controller to find out more information
		 * about the error.
		 */

		gclib_status = GCommand( galil_gclib->connection,
				"TC1", buffer_ptr, buffer_length, 0 );

		if ( gclib_status == G_NO_ERROR ) {
			ptr = strchr( buffer_ptr, MX_CR );

			if ( ptr != NULL ) {
				*ptr = '\0';
			}

			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"The command '%s' sent to Galil controller '%s' "
			"failed with TC error status = '%s'.", command,
				galil_gclib->record->name, buffer_ptr );
			break;
		} else {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"A TC command was sent to '%s' to find out why the "
			"most recent command failed, but the TC command "
			"itself failed with gclib_status = %d, so we have "
			"to give up on trying to figure out why "
			"there was an error.", galil_gclib->record->name,
					gclib_status );
		}
		break;
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The command '%s' sent to Galil controller '%s' "
		"failed with status = %d.", command,
			galil_gclib->record->name, (int) gclib_status );
		break;
	}

	if ( debug_flag && (buffer_ptr == response) ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'.",
		fname, buffer_ptr, galil_gclib->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_run_command_file( MX_GALIL_GCLIB *galil_gclib,
					char *command_filename )
{
	static const char fname[] = "mxi_galil_gclib_run_command_file()";

	FILE *command_file = NULL;
	char command_buffer[500];
	int saved_errno;
	mx_status_type mx_status;

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_GALIL_GCLIB pointer passed was NULL." );
	}
	if ( command_filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command_filename pointer passed was NULL." );
	}

	command_file = fopen( command_filename, "r" );

	if ( command_file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open command file '%s' for Galil controller "
		"'%s' failed.  errno = %d, error message = '%s'.",
			command_filename, galil_gclib->record->name,
			saved_errno, strerror(saved_errno) );
	}

	while (TRUE) {
		fgets( command_buffer, sizeof(command_buffer), command_file );

		if ( ferror(command_file) ) {
			saved_errno = errno;

			fclose(command_file);

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while reading from Galil "
			"command file '%s' for Galil controller '%s'.  "
			"errno = %d, error message = '%s'.",
				command_filename, galil_gclib->record->name,
				saved_errno, strerror(saved_errno) );
		}

		if ( feof(command_file) ) {
			/* We have reached the end of the command file,
			 * so we return now.
			 */

			fclose(command_file);

			return MX_SUCCESSFUL_RESULT;
		}

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command_buffer, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_error( MXE_UNKNOWN_ERROR, fname,
	"Somehow we broke out of a while(TRUE) loop for Galil controller '%s'."
	"  This should never happen and should be reported.",
		galil_gclib->record->name );
}
