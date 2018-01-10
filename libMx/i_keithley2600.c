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

	mx_status = mxi_keithley2600_command( keithley2600, "IDN?",
					response, sizeof(response), flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	mx_bool_type debug, response_expected;

	if ( keithley2600 == (MX_KEITHLEY2600 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2600 pointer passed was NULL." );
	}

	if ( keithley2600_flags & MXF_KEITHLEY2600_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	/* Infer whether or not a response is expected. */

	if ( ( response == NULL) || ( max_response_length == 0 ) ) {
		response_expected = FALSE;
	} else {
		response_expected = TRUE;
	}

	if ( debug ) {
		fprintf( stderr, "Received '%s' from '%s'.\n",
			response, keithley2600->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

