/*
 * Name:    i_i404.c
 *
 * Purpose: MX driver for the Pyramid Technical Consultants I404
 *          digital electrometer.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_I404_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "i_i404.h"

MX_RECORD_FUNCTION_LIST mxi_i404_record_function_list = {
	NULL,
	mxi_i404_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_i404_open,
	NULL,
	NULL,
	NULL,
	mxi_i404_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_i404_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_I404_STANDARD_FIELDS
};

long mxi_i404_num_record_fields
		= sizeof( mxi_i404_record_field_defaults )
			/ sizeof( mxi_i404_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_i404_rfield_def_ptr
			= &mxi_i404_record_field_defaults[0];

static mx_status_type
mxi_i404_process_function( void *record_ptr,
			void *record_field_ptr,
			int operation );

MX_EXPORT mx_status_type
mxi_i404_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_i404_create_record_structures()";

	MX_I404 *i404;

	/* Allocate memory for the necessary structures. */

	i404 = (MX_I404 *) malloc( sizeof(MX_I404) );

	if ( i404 == (MX_I404 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_I404 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = i404;

	record->record_function_list = &mxi_i404_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	i404->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_i404_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_i404_open()";

	MX_I404 *i404;

	char response[100];
	char copy_of_response[100];
	int argc, num_items;
	char **argv;

	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	i404 = (MX_I404 *) record->record_type_struct;

	if ( i404 == (MX_I404 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_I404 pointer for record '%s' is NULL.", record->name);
	}

#if MXI_I404_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name ));
#endif

	/* Discard any characters waiting to be sent or received. */

	mx_status = mx_rs232_discard_unwritten_output(
				i404->rs232_record, MXI_I404_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input(
				i404->rs232_record, MXI_I404_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that this is a I404 controller. */

	mx_status = mxi_i404_command( i404, "*IDN?",
					response, sizeof(response),
					MXI_I404_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( copy_of_response, response, sizeof(copy_of_response) );

	mx_string_split( copy_of_response, ",", &argc, &argv );

	if ( argc != 4 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find 4 tokens in the response '%s' to "
		"the *IDN? command sent to '%s'.",
			response, record->name );
	}
	if ( strcmp( argv[0], "Pyramid_Technical_Consultants" ) != 0 ) {
		free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Controller '%s' is not a Pyramid Technical Consultants "
		"device.  The response to '*IDN?' was '%s'.",
			record->name, response );
	}
	if ( strcmp( argv[1], "I404" ) != 0 ) {
		free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Controller '%s' is not a I404 controller.  "
		"The response to '*IDN?' was '%s'.",
			record->name, response );
	}

	/* Get the version number. */

	num_items = sscanf( argv[3], "%lf", &(i404->version) );

	if ( num_items != 1 ) {
		mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find the I404 version number in the token '%s' "
		"contained in the response '%s' to '*IDN?' by controller '%s'.",
			argv[3], response, record->name );

		free( argv );

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_i404_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {
		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_I404_CONFIGURE_CAPACITOR:
		case MXLV_I404_CONFIGURE_RESOLUTION:
		case MXLV_I404_CALIBRATION_GAIN:
		case MXLV_I404_CALIBRATION_SOURCE:
			record_field->process_function
				= mxi_i404_process_function;
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_i404_command( MX_I404 *i404,
		char *command,
		char *response,
		size_t max_response_length,
		unsigned long debug_flag )
{
	static const char fname[] = "mxi_i404_command()";

	MX_RS232 *rs232;
	char local_command[100];
	char command_status;
	mx_status_type mx_status;

	if ( i404 == (MX_I404 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_I404 pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}
	if ( i404->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The RS-232 record pointer for I404 interface '%s' is NULL.",
			i404->record->name );
	}

	rs232 = i404->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for record '%s' is NULL.",
			i404->rs232_record->name );
	}

	if ( i404->address < 0 ) {
		strlcpy( local_command, command, sizeof(local_command) );
	} else {
		snprintf( local_command, sizeof(local_command),
			"#%ld;", i404->address );

		strlcat( local_command, command, sizeof(local_command) );
	}

	/* Send the command and get the response. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending command '%s' to '%s'.",
		    fname, local_command, i404->record->name));
	}

	mx_status = mx_rs232_putline( i404->rs232_record, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The first byte that is returned tells us whether or not
	 * an error occurred.  ACK (0x6) means OK.  BEL (0x7) means
	 * that an error occurred.
	 */

	mx_status = mx_rs232_getchar_with_timeout( i404->rs232_record,
					&command_status, 0, rs232->timeout );

	switch( command_status ) {
	case MX_ACK:		/* Success */
		break;
	case MX_BEL:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"An error occurred with command '%s' sent to '%s'.",
			local_command, i404->record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"An unrecognized command status %#x was returned for "
		"command '%s' sent to '%s'.",
			command_status, local_command, i404->record->name );
		break;
	}

	if ( response != NULL ) {
		/* Get the response. */

		mx_status = mx_rs232_getline( i404->rs232_record,
				response, max_response_length, NULL, 0);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( ( response != NULL ) && debug_flag ) {

		MX_DEBUG(-2,("%s: received response '%s' from '%s'.",
			fname, response, i404->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_i404_process_function( void *record_ptr,
			void *record_field_ptr,
			int operation )
{
	static const char fname[] = "mxi_i404_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_I404 *i404;
	char command[40];
	char response[200];
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	i404 = (MX_I404 *) record->record_type_struct;

	if ( i404 == (MX_I404 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_I404 pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_I404_CONFIGURE_CAPACITOR:
			mx_status = mxi_i404_command( i404, "CONF:CAPACITOR?",
						response, sizeof(response),
						MXI_I404_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			i404->configure_capacitor = atol( response );
			break;
		case MXLV_I404_CONFIGURE_RESOLUTION:
			mx_status = mxi_i404_command( i404, "CONF:RESOLUTION?",
						response, sizeof(response),
						MXI_I404_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			i404->configure_resolution = atol( response );
			break;
		case MXLV_I404_CALIBRATION_GAIN:
			mx_status = mxi_i404_command( i404, "CALIB:GAIN?",
						response, sizeof(response),
						MXI_I404_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			i404->calibration_gain = atol( response );
			break;
		case MXLV_I404_CALIBRATION_SOURCE:
			mx_status = mxi_i404_command( i404, "CALIB:SOURCE?",
						response, sizeof(response),
						MXI_I404_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			i404->calibration_source = atol( response );
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_I404_CONFIGURE_CAPACITOR:
			snprintf( command, sizeof(command),
				"CONF:CAPACITOR %ld",
				i404->configure_capacitor );
			break;
		case MXLV_I404_CONFIGURE_RESOLUTION:
			snprintf( command, sizeof(command),
				"CONF:RESOLUTION %ld",
				i404->configure_resolution );
			break;
		case MXLV_I404_CALIBRATION_GAIN:
			strlcpy( command, "CALIB:GAIN", sizeof(command) );
			break;
		case MXLV_I404_CALIBRATION_SOURCE:
			snprintf( command, sizeof(command),
				"CALIB:SOURCE %ld",
				i404->calibration_source );
			break;
		}

		mx_status = mxi_i404_command( i404, command, 
						NULL, 0, MXI_I404_DEBUG );
					
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unknown operation code = %d for record '%s'.",
			operation, record->name );
	}

	return mx_status;
}

