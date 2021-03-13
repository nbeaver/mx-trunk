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

	return MX_SUCCESSFUL_RESULT;
}

/*--------*/

static void
mxp_flowbus_set_ascii_byte( uint8_t *ptr, uint8_t value )
{
	uint8_t byte0, byte1;

	byte0 = value % 16L;
	byte1 = value / 16L;

	if ( byte0 >= 0xA ) {
		*ptr = 'A' + byte0;
	} else {
		*ptr = '0' + byte0;
	}

	ptr++;

	if ( byte1 >= 0xA ) {
		*ptr = 'A' + byte1;
	} else {
		*ptr = '0' + byte1;
	}

	return;
}

/* WARNING: mxi_flowbus_command() is incomplete (WML, 2021-03-12) */

MX_EXPORT mx_status_type
mxi_flowbus_command( MX_FLOWBUS *flowbus,
			unsigned long destination_node,
			char *binary_command,
			size_t command_length,
			char *binary_response,
			size_t max_response_length )
{
	static const char fname[] = "mxi_flowbus_command()";

	uint8_t *ptr = NULL;
	uint8_t message_length;
	int i, j;

	if ( flowbus == (MX_FLOWBUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FLOWBUS pointer passed was NULL." );
	}
	if ( binary_command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The binary_command pointer passed was NULL." );
	}
	if ( binary_response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The binary_response pointer passed was NULL." );
	}

	memset( flowbus->command_buffer, 0, sizeof(flowbus->command_buffer) );
	memset( flowbus->response_buffer, 0, sizeof(flowbus->response_buffer) );

	/* Construct the command to send to the FLOW-BUS device. */

	switch( flowbus->protocol_type ) {
	case MXT_FLOWBUS_ASCII:
		flowbus->command_buffer[0] = ':';

		message_length = command_length + 3;

		/* Set the command message length. */

		ptr = &(flowbus->command_buffer[1]);

		mxp_flowbus_set_ascii_byte( ptr, message_length );

		/* Set the destination_node. */

		ptr = &(flowbus->command_buffer[3]);

		mxp_flowbus_set_ascii_byte( ptr, destination_node );

		/* Copy in the ASCII representation of the message. */

		for ( i = 0; i < command_length; i++ ) {
			j = 2*i + 5;

			ptr = &(flowbus->command_buffer[j]);

			mxp_flowbus_set_ascii_byte( ptr, binary_command[i] );
		}

		/* Copy in the command terminator. */

		j = 2 * command_length + 5;

		flowbus->command_buffer[j] = '\r';

		/* The command should be completely formatted at this point. */
		break;

	case MXT_FLOWBUS_BINARY:
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal protocol type %lu requested for "
		"FLOW-BUS interface '%s'.  The allowed protocols are: "
		"1 = ASCII, 2 = Extended binary.",
			flowbus->protocol_type, flowbus->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

