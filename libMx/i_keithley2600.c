/*
 * Name:    i_keithley2600.c
 *
 * Purpose: MX driver for the Keithley 2600 series of SourceMeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_KEITHLEY2600_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_rs232.h"
#include "i_keithley2600.h"

MX_RECORD_FUNCTION_LIST mxi_keithley2600_record_function_list = {
	NULL,
	mxi_keithley2600_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_keithley2600_open,
	NULL,
	NULL,
	NULL,
	mxi_keithley2600_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_keithley2600_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_KEITHLEY2600_STANDARD_FIELDS
};

long mxi_keithley2600_num_record_fields
		= sizeof( mxi_keithley2600_record_field_defaults )
			/ sizeof( mxi_keithley2600_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_keithley2600_rfield_def_ptr
			= &mxi_keithley2600_record_field_defaults[0];

/*---*/

static mx_status_type mxi_keithley2600_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/*---*/

MX_EXPORT mx_status_type
mxi_keithley2600_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_keithley2600_create_record_structures()";

	MX_KEITHLEY2600 *keithley2600;

	/* Allocate memory for the necessary structures. */

	keithley2600 = (MX_KEITHLEY2600 *) malloc( sizeof(MX_KEITHLEY2600) );

	if ( keithley2600 == (MX_KEITHLEY2600 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_KEITHLEY2600 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = keithley2600;

	record->record_function_list = &mxi_keithley2600_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	keithley2600->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley2600_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_keithley2600_open()";

	MX_KEITHLEY2600 *keithley2600 = NULL;
	unsigned long flags;
	char response[80];
	char format[80];
	int num_items;
	size_t len;
	mx_status_type mx_status;

#if MXI_KEITHLEY2600_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	keithley2600 = (MX_KEITHLEY2600 *) record->record_type_struct;

	if ( keithley2600 == (MX_KEITHLEY2600 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_KEITHLEY2600 pointer for record '%s' is NULL.",
			record->name);
	}

	flags = keithley2600->keithley2600_flags;

	/* Connect to the device. */

	keithley2600->rs232_record =
		    mx_get_record( record, keithley2600->rs232_record_name );

	if ( keithley2600->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The requested RS232 record '%s' was not found "
		"in the MX database for record '%s'.",
			keithley2600->rs232_record_name,
			record->name );
	}

	/* Discard any garbage left over by a previous session. */

	mx_status = mx_rs232_discard_unread_input(
			keithley2600->rs232_record, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the Keithley 2600 is responding to commands. */

	mx_status = mxi_keithley2600_command( keithley2600, "*IDN?",
					response, sizeof(response), flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the response. */

	snprintf( format, sizeof(format),
		"Keithley Instruments Inc., Model %%%ds %%%ds %%%ds",
			(int) sizeof(keithley2600->model_name),
			(int) sizeof(keithley2600->serial_number),
			(int) sizeof(keithley2600->firmware_version) );

	num_items = sscanf( response, format,
			keithley2600->model_name,
			keithley2600->serial_number,
			keithley2600->firmware_version );

	if ( num_items != 3 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The device expected for '%s' on serial connection '%s' "
		"is not a Keithley 2600.  "
		"Its response to an '*IDN?' command was '%s'.",
		record->name, keithley2600->rs232_record->name, response );
	}

	/* Zap trailing comma (,) characters if present. */

	len = strlen( keithley2600->model_name );

	if ( (keithley2600->model_name)[len-1] == ',' ) {
		(keithley2600->model_name)[len-1] = '\0';
	}

	len = strlen( keithley2600->serial_number );

	if ( (keithley2600->serial_number)[len-1] == ',' ) {
		(keithley2600->serial_number)[len-1] = '\0';
	}

	/*======== Configure global parameters. ========*/

	/* Select ASCII data format. */

	mx_status = mxi_keithley2600_command( keithley2600,
				"format.data = format.ASCII",
				NULL, 0,
				keithley2600->keithley2600_flags );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_keithley2600_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case 0:
			record_field->process_function
					= mxi_keithley2600_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*==================================================================*/

static mx_status_type
mxi_keithley2600_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_keithley2600_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_KEITHLEY2600 *keithley2600;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	keithley2600 = (MX_KEITHLEY2600 *) record->record_type_struct;

	MXW_UNUSED( keithley2600 );

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case 0:
			/* Just return the string most recently written
			 * by the process function.
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
		case 0:
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

/*==================================================================*/

MX_EXPORT mx_status_type
mxi_keithley2600_command( MX_KEITHLEY2600 *keithley2600,
				char *command,
				char *response,
				unsigned long max_response_length,
				unsigned long keithley2600_flags )
{
	static const char fname[] = "mxi_keithley2600_command()";

	MX_RS232 *rs232 = NULL;
	int i, max_attempts;
	mx_bool_type debug, response_expected;
	mx_status_type mx_status;

	if ( keithley2600 == (MX_KEITHLEY2600 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2600 pointer passed was NULL." );
	}

	if ( keithley2600->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The rs232_record pointer for record '%s' is NULL.",
			keithley2600->record->name );
	}

	rs232 = keithley2600->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL )  {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RS232 pointer for record '%s' is NULL.",
			keithley2600->rs232_record->name );
	}

	if ( keithley2600_flags & MXF_KEITHLEY2600_DEBUG_RS232 ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	max_attempts = 5;

	for ( i = 0; i < max_attempts; i++ ) {

		if ( debug ) {
			fprintf( stderr, "Sending '%s' to '%s'.\n",
				command, keithley2600->record->name );
		}

		mx_status = mx_rs232_putline( keithley2600->rs232_record,
						command, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Infer whether or not a response is expected. */

		if ( ( response == NULL) || ( max_response_length == 0 ) ) {
			response_expected = FALSE;
		} else {
			response_expected = TRUE;
		}

		if ( response_expected == FALSE ) {
			return MX_SUCCESSFUL_RESULT;
		}

		mx_status = mx_rs232_getline_with_timeout(
						keithley2600->rs232_record,
						response, max_response_length,
						NULL, 0, rs232->timeout );

		if ( mx_status.code == MXE_SUCCESS ) {
			if ( debug ) {
				fprintf( stderr, "Received '%s' from '%s'.\n",
					response, keithley2600->record->name );
			}

			break;		/* Exit the for() loop. */
		} else
		if ( mx_status.code == MXE_TIMED_OUT ) {
			(void) mx_rs232_putline( keithley2600->rs232_record,
						"\r\n", NULL, 0x1 );
			mx_msleep(500);

			(void) mx_rs232_getline_with_timeout(
					keithley2600->rs232_record,
					response, max_response_length,
					NULL, 0x1, rs232->timeout );
		} else {
			return mx_status;
		}

		fprintf( stderr, "Retrying command '%s' for '%s': retry = %d\n",
			command, keithley2600->record->name, i+1 );
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not receive a response to the command '%s' sent "
		"to '%s' after %d attempts.",
			command, keithley2600->record->name, i );
	}

	return MX_SUCCESSFUL_RESULT;
}

