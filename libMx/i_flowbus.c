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
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
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

/* ---- */

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

/* ---- */

MX_EXPORT mx_status_type
mxi_flowbus_send_parameter( MX_FLOWBUS *flowbus,
				unsigned long node_address,
				unsigned long process_number,
				unsigned long parameter_number,
				unsigned long flowbus_parameter_type,
				void *parameter_value_to_send,
				char *status_response,
				size_t max_response_length )
{
	static const char fname[] = "mxi_flowbus_send_parameter()";

	unsigned long message_length;
	uint8_t parameter_byte;

	uint8_t uint8_value;
	uint16_t uint16_value;
	uint32_t uint32_value;
	size_t string_length;

	char ascii_command_buffer[500];
	char ascii_response_buffer[500];

	mx_status_type mx_status;

	memset( ascii_command_buffer, 0, sizeof(ascii_command_buffer) );
	memset( ascii_response_buffer, 0, sizeof(ascii_response_buffer) );

	ascii_command_buffer[0] = ':';

	/* The final 'message_length' is the total length of the
	 * message, except for the first two bytes, and the two
	 * byte CR-LF terminator at the end.
	 */

	message_length = 0;

	/*---*/

	ascii_command_buffer[2] = ( node_address & 0xff );

	message_length++;

	/*---*/

	if ( status_response == (char *) NULL ) {
		ascii_command_buffer[3] = 2;		/* no response */
	} else {
		ascii_command_buffer[3] = 1;		/* response expected */
	}

	message_length++;

	/*---*/

	ascii_command_buffer[4] = ( process_number & 0x7F );

	message_length++;

	/*---*/

	switch( flowbus_parameter_type ) {
	case MXDT_FLOWBUS_UCHAR:	/* unsigned char */

		uint8_value = *(( uint8_t * ) parameter_value_to_send);

		ascii_command_buffer[6] = uint8_value;

		message_length++;
		break;

	case MXDT_FLOWBUS_USHORT:	/* unsigned short */

		uint16_value = *(( uint16_t * ) parameter_value_to_send);

		switch( mx_native_byteorder() ) {
		case MX_DATAFMT_BIG_ENDIAN:
		    ascii_command_buffer[6] = ( uint16_value >> 8 ) & 0xff;
		    ascii_command_buffer[7] = ( uint16_value & 0xff );
		    break;
		case MX_DATAFMT_LITTLE_ENDIAN:
		    ascii_command_buffer[7] = ( uint16_value >> 8 ) & 0xff;
		    ascii_command_buffer[6] = ( uint16_value & 0xff );
		    break;
		}

		message_length += 2;
		break;

	case MXDT_FLOWBUS_ULONG_FLOAT: /* 32-bit unsigned long or float */

		uint32_value = *(( uint32_t * ) parameter_value_to_send);

		switch( mx_native_byteorder() ) {
		case MX_DATAFMT_BIG_ENDIAN:
		    ascii_command_buffer[6] = ( uint32_value >> 24 ) & 0xff;
		    ascii_command_buffer[7] = ( uint32_value >> 16 ) & 0xff;
		    ascii_command_buffer[8] = ( uint32_value >> 8 ) & 0xff;
		    ascii_command_buffer[9] = ( uint32_value & 0xff );
		    break;
		case MX_DATAFMT_LITTLE_ENDIAN:
		    ascii_command_buffer[9] = ( uint32_value >> 24 ) & 0xff;
		    ascii_command_buffer[8] = ( uint32_value >> 16 ) & 0xff;
		    ascii_command_buffer[7] = ( uint32_value >> 8 ) & 0xff;
		    ascii_command_buffer[6] = ( uint32_value & 0xff );
		    break;
		}

		message_length += 4;
		break;

	case MXDT_FLOWBUS_STRING:	/* string */

		string_length = strlen( (char *) parameter_value_to_send );

		if ( string_length > UCHAR_MAX ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"FlowBus '%s', node %lu, process %lu, parameter %lu "
			"is longer (%ld) than the maximum allowed value "
			"of %d.  String = '%s'",
				flowbus->record->name, node_address,
				process_number, parameter_number,
				(unsigned long) string_length, UCHAR_MAX,
				(char *) parameter_value_to_send );
		}

		ascii_command_buffer[6] = string_length;

		message_length++;

		strlcpy( &(ascii_command_buffer[7]),
				parameter_value_to_send,
				UCHAR_MAX );
				
		message_length += string_length;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Attempted to write unsupported FlowBus datatype (%lu) to "
		"FlowBus '%s', node %lu, process %lu, parameter %lu.  "
		"Allowed datatypes are unsigned char (%d), "
		"unsigned short (%d), unsigned long (%d), "
		"float (%d), and string (%d).",
			flowbus_parameter_type, flowbus->record->name,
			node_address, process_number, parameter_number,
			MXDT_FLOWBUS_UCHAR, MXDT_FLOWBUS_USHORT,
			MXDT_FLOWBUS_ULONG_FLOAT, MXDT_FLOWBUS_ULONG_FLOAT,
			MXDT_FLOWBUS_STRING );
		break;
	}

	/*---*/

	parameter_byte = ( flowbus_parameter_type & 0x3 ) << 5;

	parameter_byte |= ( parameter_number & 0x1F );

	ascii_command_buffer[5] = parameter_byte;

	message_length++;
	
	/*---*/

	ascii_command_buffer[1] = message_length;

	/*---*/

	/* Handle command type 02 */

	if ( status_response == (char *) NULL ) {
		mx_status = mxi_flowbus_command( flowbus,
						ascii_command_buffer,
						NULL, 0 );

		return mx_status;
	}

	/* Handle command type 01 */

	mx_status = mxi_flowbus_command( flowbus, ascii_command_buffer,
						ascii_response_buffer, 
						max_response_length );
						
	mx_warning( "%s: FIXME: Implement handling of status response.", fname);

	return mx_status;
}

/* ---- */

MX_EXPORT mx_status_type
mxi_flowbus_request_parameter( MX_FLOWBUS *flowbus,
				unsigned long node_address,
				unsigned long process_number,
				unsigned long parameter_number,
				unsigned long flowbus_parameter_type,
				void *requested_parameter_value,
				size_t max_parameter_length )
{
#if 0
	static const char fname[] = "mxi_flowbus_request_parameter()";
#endif

	unsigned long message_length;
	uint8_t parameter_byte;
	size_t string_length;

	char ascii_command_buffer[500];
	char ascii_response_buffer[500];

	mx_status_type mx_status;

	memset( ascii_command_buffer, 0, sizeof(ascii_command_buffer) );
	memset( ascii_response_buffer, 0, sizeof(ascii_response_buffer) );

	ascii_command_buffer[0] = ':';

	/* The final 'message_length' is the total length of the
	 * message, except for the first two bytes, and the two
	 * byte CR-LF terminator at the end.
	 */

	message_length = 0;

	/*---*/

	ascii_command_buffer[2] = ( node_address & 0xff );

	message_length++;

	/*---*/

	ascii_command_buffer[3] = 4;		/* Request Parameter */

	message_length++;

	/*---*/

	ascii_command_buffer[4] = ( process_number & 0x7F );

	message_length++;

	/*---*/

	parameter_byte = ( flowbus_parameter_type & 0x3 ) << 5;

	parameter_byte |= ( (flowbus->sequence_number) & 0x1F );

	ascii_command_buffer[5] = parameter_byte;

	message_length++;

	/*---*/

	ascii_command_buffer[6] = ( process_number & 0x7F );

	message_length++;

	/*---*/

	parameter_byte = ( flowbus_parameter_type & 0x3 ) << 5;

	parameter_byte |= ( parameter_number & 0x1F );

	ascii_command_buffer[7] = parameter_byte;

	message_length++;

	/*---*/

	string_length = 0;

	ascii_command_buffer[8] = string_length;

	message_length++;

	/*---*/

	ascii_command_buffer[1] = message_length;

	/*---*/

	mx_status = mxi_flowbus_command( flowbus, ascii_command_buffer,
					ascii_response_buffer,
					sizeof(ascii_response_buffer) );

	return MX_SUCCESSFUL_RESULT;
}

