/*
 * Name:    i_galil_gclib.c
 *
 * Purpose: MX driver for Galil motor controllers via the gclib library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020-2021 Illinois Institute of Technology
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
	mxi_galil_gclib_finish_delayed_initialization,
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

	MXW_UNUSED( flags );

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
	int num_items;
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

	switch( gclib_status ) {
	case G_NO_ERROR:
		break;
	case G_TIMEOUT:
		return mx_error( MXE_TIMED_OUT, fname,
		"The attempt to open a Galil controller connection for "
		"record '%s' at '%s' timed out.  Is it possible that "
		"the Galil controller is turned off or disconnected?",
			record->name, galil_gclib->hostname );
		break;
	default:
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

	/* Figure out how many motors this controller can control. */

	mx_status = mxi_galil_gclib_command( galil_gclib,
			"QZ", response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: QZ response = '%s'", fname, response ));

	num_items = sscanf( response, "%lu",
			&(galil_gclib->maximum_num_motors ) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not see the maximum number of motor axes for "
		"Galil controller '%s' in the response '%s' to command 'QZ'.",
			galil_gclib->record->name, response );
	}

	galil_gclib->current_num_motors = 0;

	/* Allocate a nulled out array of motor record pointers. */
	
	galil_gclib->motor_record_array =
		calloc( galil_gclib->maximum_num_motors, sizeof(MX_RECORD *) );
	
	if ( galil_gclib->motor_record_array == (MX_RECORD **) NULL ) {

		mx_status = mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt to allocate memory for a %lu element array "
		"of MX_RECORD pointers failed for Galil controller '%s'.",
			galil_gclib->maximum_num_motors,
			galil_gclib->record->name );

		galil_gclib->maximum_num_motors = 0;

		return mx_status;
	}

	/* Find out what model of controller this is
	 * and its firmware revision.
	 */

	snprintf( command, sizeof(command),
		"%c%c", MX_CTRL_R, MX_CTRL_V );

	mx_status = mxi_galil_gclib_command( galil_gclib,
			command, response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_string_split( response, " ", &version_argc, &version_argv );

	MX_DEBUG(-2,("%s: version_argc = %d", fname, version_argc ));

	strlcpy( galil_gclib->controller_type,
		version_argv[0], sizeof(galil_gclib->controller_type) );

	strlcpy( galil_gclib->firmware_revision,
		version_argv[2], sizeof(galil_gclib->firmware_revision) );

	mx_free( version_argv );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_galil_gclib_finish_delayed_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_galil_gclib_finish_delayed_initialization()";

	MX_GALIL_GCLIB *galil_gclib = NULL;
	MX_RECORD_FIELD *motor_record_array_field = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	galil_gclib = (MX_GALIL_GCLIB *) record->record_type_struct;

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GALIL_GCLIB pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mx_find_record_field( record, "motor_record_array",
						&motor_record_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_record_array_field->dimension[0] =
			galil_gclib->current_num_motors;

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
		case MXLV_GALIL_GCLIB_PROGRAM_DOWNLOAD:
		case MXLV_GALIL_GCLIB_PROGRAM_DOWNLOAD_FILE:
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

	MXW_UNUSED( galil_gclib );

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
		case MXLV_GALIL_GCLIB_PROGRAM_DOWNLOAD:
			mx_status = mxi_galil_gclib_program_download(
				galil_gclib, galil_gclib->program_download );
			break;
		case MXLV_GALIL_GCLIB_PROGRAM_DOWNLOAD_FILE:
			mx_status = mxi_galil_gclib_program_download_file(
			    galil_gclib, galil_gclib->program_download_file );
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
	MX_RECORD *motor_record = NULL;
	MX_RECORD *found_motor_record = NULL;
	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	int i, num_items, error_code;
	unsigned long flags;
	mx_bool_type debug_flag;
	char *ptr = NULL;
	char local_response[100];
	char *buffer_ptr = NULL;
	char raw_motor_name;
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

			num_items = sscanf( buffer_ptr, "%d", &error_code );

			if ( num_items != 1 ) {
				error_code = 0;
			}

			switch( error_code ) {
			case 20:	/* Begin not valid with motor off. */

				/* First try to figure out which motor
				 * was turned off.
				 */

				num_items = sscanf( command, "BG%c",
							&raw_motor_name );

				if ( num_items != 1 ) {
					return mx_error(
					MXE_NOT_READY, fname,
					"Galil controller '%s' said that the "
					"attempt to start a motor move "
					"failed.  But our attempt to find "
					"the motor name in the command '%s' "
					"was unsuccessful.  The name should "
					"be the single character after "
					"the letters 'BG' at the start of "
					"the command.",
						galil_gclib->record->name,
						command );
				}

				motor_record = NULL;
				found_motor_record = NULL;

				for ( i = 0;
					i < galil_gclib->current_num_motors;
					i++ )
				{
					motor_record =
					    galil_gclib->motor_record_array[i];

					if (motor_record == (MX_RECORD *)NULL)
					{
						/* Try the next element
						 * in the array.
						 */
						continue;
					}

					galil_gclib_motor =
					    (MX_GALIL_GCLIB_MOTOR *)
					    motor_record->record_type_struct;

					if ( galil_gclib_motor->motor_name
						== raw_motor_name )
					{
					    found_motor_record = motor_record;
					}
					break;
				}

				if ( found_motor_record == (MX_RECORD *) NULL )
				{
					return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
					"Attempted to start raw motor '%c' "
					"moving for Galil controller '%s', "
					"but the motor was turned off.  "
					"However, axis '%c' does not have a "
					"matching MX record in the MX "
					"database, so it is not possible to "
					"turn on this motor.  But, there "
					"should not have been a way to "
					"send the move command '%s' either, "
					"so you should never see this "
					"error in the first place.",
						raw_motor_name,
						galil_gclib->record->name,
						raw_motor_name,
						command );
				}

				/* Then send the error. */

				return mx_error( MXE_NOT_READY, fname,
				"Attempted to start motor '%s' moving "
				"when the motor was turned off.  You should "
				"try enabling the motor by sending the "
				"number 1 to the '%s.axis_enable' field.  "
				"After that, try your move again.",
					found_motor_record->name,
					found_motor_record->name );
				break;
			default:
				return mx_error( MXE_INTERFACE_ACTION_FAILED,
				fname, "The command '%s' sent to Galil "
				"controller '%s' failed with TC error "
				"code = '%s'.", command,
				galil_gclib->record->name, buffer_ptr );

				break;
			}
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

MX_EXPORT mx_status_type
mxi_galil_gclib_program_download( MX_GALIL_GCLIB *galil_gclib,
				char *program_download_string )
{
	static const char fname[] = "mxi_galil_gclib_program_download()";

	GReturn gclib_status;

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_GALIL_GCLIB pointer passed was NULL." );
	}
	if ( program_download_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The program_download string pointer passed was NULL." );
	}

	gclib_status = GProgramDownload( galil_gclib->connection,
					program_download_string, NULL );

	if ( gclib_status != G_NO_ERROR ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to download a program to Galil controller '%s' "
		"failed with status = %d.",
		galil_gclib->record->name, (int) gclib_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_galil_gclib_program_download_file( MX_GALIL_GCLIB *galil_gclib,
				char *program_download_filename )
{
	static const char fname[] = "mxi_galil_gclib_program_download_file()";

	GReturn gclib_status;

	if ( galil_gclib == (MX_GALIL_GCLIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_GALIL_GCLIB pointer passed was NULL." );
	}
	if ( program_download_filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The program_download_file pointer passed was NULL." );
	}

	gclib_status = GProgramDownloadFile( galil_gclib->connection,
					program_download_filename, NULL );

	if ( gclib_status != G_NO_ERROR ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to download program file '%s' "
		"to Galil controller '%s' failed with status = %d.",
			program_download_filename,
			galil_gclib->record->name,
			(int) gclib_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

