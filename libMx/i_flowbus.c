/*
 * Name:    i_flowbus.c
 *
 * Purpose: Bronkhorst FLOW-BUS ASCII protocol interface driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_FLOWBUS_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_flowbus.h"

MX_RECORD_FUNCTION_LIST mxi_flowbus_record_function_list = {
	NULL,
	mxi_flowbus_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_flowbus_open
};

MX_RECORD_FIELD_DEFAULTS mxi_flowbus_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_FLOWBUS_STANDARD_FIELDS
};

long mxi_flowbus_num_record_fields
		= sizeof( mxi_flowbus_record_field_defaults )
			/ sizeof( mxi_flowbus_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_flowbus_rfield_def_ptr
			= &mxi_flowbus_record_field_defaults[0];

/*---*/

static mx_status_type
mxi_flowbus_get_pointers( MX_RECORD *record,
				MX_FLOWBUS **flowbus,
				const char *calling_fname )
{
	static const char fname[] = "mxi_flowbus_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( flowbus == (MX_FLOWBUS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_FLOWBUS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*flowbus = (MX_FLOWBUS *) (record->record_type_struct);

	if ( *flowbus == (MX_FLOWBUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_FLOWBUS pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_flowbus_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_flowbus_create_record_structures()";

	MX_FLOWBUS *flowbus;

	/* Allocate memory for the necessary structures. */

	flowbus = (MX_FLOWBUS *) malloc( sizeof(MX_FLOWBUS) );

	if ( flowbus == (MX_FLOWBUS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_FLOWBUS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = flowbus;

	record->record_function_list = &mxi_flowbus_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	flowbus->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_flowbus_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_flowbus_open()";

	MX_FLOWBUS *flowbus = NULL;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));

	mx_status = mxi_flowbus_get_pointers( record, &flowbus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out what kind of protocol to use. */

	if ( strcmp( "ascii", flowbus->protocol_type_string ) == 0 ) {
		flowbus->protocol_type = MXT_FLOWBUS_ASCII;
	} else
	if ( strcmp( "binary", flowbus->protocol_type_string ) == 0 ) {
		flowbus->protocol_type = MXT_FLOWBUS_BINARY;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized FLOW-BUS protocol type '%s' requested "
		"for FLOW-BUS interface '%s'.  The allowed values are "
		"'ascii' and 'binary'.",
			flowbus->protocol_type_string, record->name );
	}

	/* Discard any unread input that may be left in the rs232 device. */

	mx_status = mx_rs232_discard_unread_input( flowbus->rs232_record,
								MXF_232_DEBUG );

	/* Ask for the status of node 3. */

	strlcpy( command, ":06030401210121", sizeof(command) );

	mx_status = mxi_flowbus_command( flowbus, command,
						response, sizeof(response) );

	return MX_SUCCESSFUL_RESULT;
}

/* WARNING: mxi_flowbus_command() is incomplete (WML, 2021-03-12) */

MX_EXPORT mx_status_type
mxi_flowbus_command( MX_FLOWBUS *flowbus,
			char *command,
			char *response,
			size_t max_response_length )
{
	static const char fname[] = "mxi_flowbus_command()";

	mx_status_type mx_status;

	if ( flowbus == (MX_FLOWBUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FLOWBUS pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed was NULL." );
	}

	/* Send the command using the requested protocol type. */

	mx_status = mx_rs232_putline( flowbus->rs232_record, command, NULL, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read back the response. */

	mx_status = mx_rs232_getline_with_timeout( flowbus->rs232_record,
						response, max_response_length,
						NULL, 1, 5.0 );

	return MX_SUCCESSFUL_RESULT;
}

