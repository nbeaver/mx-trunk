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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_process.h"
#include "mx_bit.h"
#include "mx_rs232.h"
#include "i_flowbus.h"

MX_RECORD_FUNCTION_LIST mxi_flowbus_record_function_list = {
	NULL,
	mxi_flowbus_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_flowbus_open,
	NULL,
	NULL,
	NULL,
	mxi_flowbus_special_processing_setup
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

static mx_status_type mxi_flowbus_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

static mx_status_type mxi_flowbus_show_nodes( MX_FLOWBUS *flowbus );

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
#if 0
	char command[80];
	char response[80];
#endif
	mx_status_type mx_status;

#if 0
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

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

	return mx_status;
}

/* ---- */

MX_EXPORT mx_status_type
mxi_flowbus_command( MX_FLOWBUS *flowbus,
			char *command,
			char *response,
			size_t max_response_length,
			unsigned long flowbus_command_flags )
{
	static const char fname[] = "mxi_flowbus_command()";

	unsigned long debug_flag, quiet_flag;

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

	/*---*/

	if ( flowbus->flowbus_flags & MXF_FLOWBUS_DEBUG ) {
		debug_flag = TRUE;
	} else
	if ( flowbus_command_flags & MXFCF_FLOWBUS_DEBUG ) {
		debug_flag = TRUE;
	} else {
		debug_flag = 0;
	}

	if ( flowbus_command_flags & MXFCF_FLOWBUS_QUIET ) {
		quiet_flag = MXE_QUIET;
	} else {
		quiet_flag = 0;
	}

	/*---*/

	/* Send the command using the requested protocol type. */

	mx_status = mx_rs232_putline( flowbus->rs232_record, command,
						NULL, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read back the response. */

	mx_status = mx_rs232_getline_with_timeout( flowbus->rs232_record,
						response, max_response_length,
						NULL, debug_flag, 5.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check for error responses. */

	if ( strcmp( response, ":0104" ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Flowbus interface '%s' said that it received "
		"an illegal command.  Command = '%s'",
			flowbus->record->name, command );
	} else
	if ( strcmp( response, ":0105" ) == 0 ) {
		return mx_error( (MXE_NOT_FOUND | quiet_flag), fname,
		"The node address specified in command '%s' "
		"for Flowbus interface '%s' does not exist.",
			command, flowbus->record->name );
		
	} else
	if ( strncmp( response, ":01", 3 ) == 0 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unrecognized error response '%s' received for command '%s' "
		"sent to Flowbus interface '%s'.",
			response, command, flowbus->record->name );
	} else
	if ( ( strncmp( response, ":04", 3 ) == 0 )
	  && ( strncmp( response + 5, "00", 2 ) == 0 ) )
	{
		/* An error status was returned. */

		char flowbus_status_string[3];
		char flowbus_offset_string[3];
		unsigned long flowbus_status;
		unsigned long error_offset;

		flowbus_status_string[0] = response[7];
		flowbus_status_string[1] = response[8];
		flowbus_status_string[2] = '\0';

		flowbus_status =
			mx_hex_string_to_unsigned_long( flowbus_status_string );

		flowbus_offset_string[0] = response[9];
		flowbus_offset_string[1] = response[10];
		flowbus_offset_string[2] = '\0';

		error_offset =
			mx_hex_string_to_unsigned_long( flowbus_offset_string );

		switch( flowbus_status ) {
		case 0:
			return MX_SUCCESSFUL_RESULT;
			break;
		case 0x01:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Process claimed (0x01, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x02:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Command error (0x02, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x03:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Process error (0x03, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x04:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter error (0x04, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x05:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter type error (0x05, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x06:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter value error (0x06, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x07:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Network not active (0x07, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x08:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Time-out start character(0x08, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x09:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Time-out serial line (0x09, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x0A:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Hardware memory error (0x0A, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x0B:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Node number error (0x0B, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x0C:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"General communication error (0x0C, %lu) for "
			"command '%s' sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x0D:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Read only parameter (0x0D, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x0E:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error PC communication (0x0E, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x0F:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"No RS232 connection (0x0F, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x10:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"PC out of memory (0x10, %lu) for command '%s' sent to "
			"Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x11:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Write only parameter (0x11, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x12:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"System configuration unknown (0x12, %lu) for "
			"command '%s' sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x13:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"No free node address (0x13, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x14:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Wrong interface type (0x14, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x15:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error serial port connection (0x15, %lu) for "
			"command '%s' sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x16:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error opening communication (0x16, %lu) for "
			"command '%s' sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x17:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Communication error (0x17, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x18:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error interface bus master (0x18, %lu) for "
			"command '%s' sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x19:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Timeout answer (0x19, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x1A:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"No start character (0x1A, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x1B:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error first digit (0x1B, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x1C:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Buffer overflow in host (0x1C, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x1D:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Buffer overflow (0x1D, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x1E:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"No answer found (0x1E, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x1F:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error closing communication (0x1F, %lu) for "
			"command '%s' sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x20:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Synchronization error (0x20, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x21:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Send error (0x21, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x22:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Protocol error (0x22, %lu) for command '%s' "
			"sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		case 0x23:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Buffer overflow in module (0x23, %lu) for "
			"command '%s' sent to Flowbus interface '%s'.",
				error_offset, command, flowbus->record->name );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname, 
			"Flowbus status code %lu received for command '%s' "
			"sent to Flowbus interface '%s'.  This error code "
			"is described in Section 3.6 of the Flowbus RS232 "
			"interface manual #9.17.027.",
			    flowbus_status, command, flowbus->record->name );
			break;
		}
	}

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
				size_t max_response_length,
				unsigned long flowbus_command_flags )
{
	static const char fname[] = "mxi_flowbus_send_parameter()";

	unsigned long message_length;

	uint8_t uint8_value;
	uint16_t uint16_value;
	uint32_t uint32_value;

	uint8_t expected_string_length = 0;

#if 0
	uint8_t sequence_number;
#endif

	char ascii_command_buffer[500];
	char ascii_response_buffer[500];

	char *value_string_ptr = NULL;
	char *status_string_ptr = NULL;

	unsigned long flowbus_status;

	mx_bool_type need_status_response = FALSE;

	mx_status_type mx_status;

	/**** Will we need to ask for a status response to be sent back? ****/

	if ( status_response == (char *) NULL ) {
		need_status_response = FALSE;
	} else
	if ( max_response_length == 0 ) {
		need_status_response = FALSE;
		status_response[0] = '\0';
	} else {
		need_status_response = TRUE;
	}

#if 0
	MX_DEBUG(-2,("%s: need_status_response = %d",
		fname, (int) need_status_response ));
#endif

	/******* Begin constructing the command to send to the device *******/

	memset( ascii_command_buffer, 0, sizeof(ascii_command_buffer) );
	memset( ascii_response_buffer, 0, sizeof(ascii_response_buffer) );

	ascii_command_buffer[0] = ':';

	ascii_command_buffer[1] = 'X';
	ascii_command_buffer[2] = 'X';

	/* The final 'message_length' is the total length of the
	 * message, except for the first two bytes, and the two
	 * byte CR-LF terminator at the end.
	 */

	message_length = 0;

	/*---*/

	/* Node address (field 2). */

	uint8_value = ( node_address & 0xFF );

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				2, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	if ( need_status_response ) {
	    uint8_value = 1;   /* 1 = Send Parameter with status (field 3) */
	} else {
	    uint8_value = 2;   /* 2 = Send Parameter without status (field 3) */
	}

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				3, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	/* Process number (field 4). */

	uint8_value = ( process_number & 0x7F );

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				4, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	/* Parameter type and parameter number (field 5). */

#if 0
	sequence_number = flowbus->sequence_number;
#endif

	/*---*/

	uint8_value = ( flowbus_parameter_type & 0x7F ) << 4;

	uint8_value |= ( parameter_number & 0x1F );

#if 0
	MX_DEBUG(-2,
	("%s: flowbus_parameter_type = %lu, parameter_number = %lu, "
			"uint8_value = %#lx",
			fname, flowbus_parameter_type,
			(unsigned long) parameter_number,
			(unsigned long) uint8_value ));
#endif

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				5, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	/* If this is a string field, then we must append 
	 * the expected string length (field 6).
	 */

	if ( flowbus_parameter_type == MXDT_FLOWBUS_STRING ) {
		expected_string_length =
			strlen( (char *) parameter_value_to_send );

		mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				6, MXFT_UINT8, &expected_string_length );

		message_length++;
	}

	/******* Copy in the parameter value *******/

	/* Get a pointer to the start of the parameter data. */

	if ( flowbus_parameter_type == MXDT_FLOWBUS_STRING ) {
		value_string_ptr = 13 + (char *) ascii_command_buffer;
	} else {
		value_string_ptr = 11 + (char *) ascii_command_buffer;
	}

	/*---*/

	switch( flowbus_parameter_type ) {
	case MXDT_FLOWBUS_UINT8:
		uint8_value = 0xFF &
			*( (uint8_t *) parameter_value_to_send );

		snprintf( value_string_ptr, 2+1,
			"%02X", (unsigned int) uint8_value );

		message_length += 1;
		break;

	case MXDT_FLOWBUS_USHORT:
		uint16_value = 0xFFFF &
			*( (uint16_t *) parameter_value_to_send );

		snprintf( value_string_ptr, 4+1,
			"%04X", (unsigned int) uint16_value );

		message_length += 2;
		break;

	case MXDT_FLOWBUS_ULONG_FLOAT:
		uint32_value = 0xFFFFFFFF &
			*( (uint32_t *) parameter_value_to_send );

		snprintf( value_string_ptr, 8+1,
			"%08lX", (unsigned long) uint32_value );

		message_length += 4;
		break;

	case MXDT_FLOWBUS_STRING:
		{
			size_t i, bytes_to_copy;
			char *parameter_string = NULL;

			size_t buffer_space_left = sizeof(ascii_command_buffer)
			    - ( value_string_ptr - ascii_command_buffer );

			if ( expected_string_length > buffer_space_left ) {
				bytes_to_copy = buffer_space_left;
			} else {
				bytes_to_copy = expected_string_length;
			}

			parameter_string = (char *) parameter_value_to_send;

			for ( i = 0; i < bytes_to_copy; i++ ) {

				uint8_value = parameter_string[i];

				snprintf( value_string_ptr, 2+1,
					"%02X", (unsigned int) uint8_value );

				value_string_ptr += 2;
			}

			message_length += bytes_to_copy;
		}
		break;
	}

	/* Message length (field 1). */

	uint8_value = message_length;

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				1, MXFT_UINT8, &uint8_value );

	/******* Send the command and receive the response *******/

	mx_status = mxi_flowbus_command( flowbus, ascii_command_buffer,
					ascii_response_buffer,
					max_response_length,
					flowbus_command_flags );

	flowbus->sequence_number++;

	flowbus->sequence_number &= 0x1F;

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check for the illegal command response. */

	if ( strcmp( ascii_response_buffer, ":0104" ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Flowbus interface '%s' said that it received "
		"an illegal command.  Command = '%s'",
			flowbus->record->name, ascii_command_buffer );
	}

	/* If we did not ask for a status response, then we are done. */

	if ( need_status_response == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/******* Parse the status from the device *******/

#if 0
	MX_DEBUG(-2,("%s: status = '%s'", fname, ascii_response_buffer));
#endif

	status_string_ptr = ascii_response_buffer;

	status_string_ptr += 7;   /* Move up to the start of the status word. */

	status_string_ptr[2] = '\0';  /* Null terminate the status word. */

	flowbus_status = mx_hex_string_to_unsigned_long( status_string_ptr );

	if ( flowbus_status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The Flowbus device returned an error code of %lu",
			flowbus_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ---- */

/* Note: For Flowbus string parameters, we must pass in to this function
 * the expected string length in the requested_parameter_value argument.
 */

MX_EXPORT mx_status_type
mxi_flowbus_request_parameter( MX_FLOWBUS *flowbus,
				unsigned long node_address,
				unsigned long process_number,
				unsigned long parameter_number,
				unsigned long flowbus_parameter_type,
				void *requested_parameter_value,
				size_t max_parameter_length,
				unsigned long flowbus_command_flags )
{
	static const char fname[] = "mxi_flowbus_request_parameter()";

	unsigned long message_length;

	uint8_t uint8_value;
	uint16_t uint16_value;
	uint32_t uint32_value;

	uint8_t expected_string_length, flowbus_string_length;

	size_t allowed_string_length;

	uint8_t sequence_number;

	char ascii_command_buffer[500];
	char ascii_response_buffer[500];

	uint8_t nibble0, nibble1;

	uint8_t command_code;

	char *value_string_ptr = NULL;

	unsigned long i, ulong_value;
	char byte_string[3];
	char *requested_string_ptr = NULL;

	mx_status_type mx_status;

	/******* Begin constructing the command to send to the device *******/

	memset( ascii_command_buffer, 0, sizeof(ascii_command_buffer) );
	memset( ascii_response_buffer, 0, sizeof(ascii_response_buffer) );

	ascii_command_buffer[0] = ':';

	ascii_command_buffer[1] = 'X';
	ascii_command_buffer[2] = 'X';

	/* The final 'message_length' is the total length of the
	 * message, except for the first two bytes, and the two
	 * byte CR-LF terminator at the end.
	 */

	message_length = 0;

	/*---*/

	/* Node address (field 2). */

	uint8_value = ( node_address & 0xff );

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				2, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	uint8_value = 4;	/* 4 = Request Parameter (field 3) */

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				3, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	/* Process number (field 4). */

	uint8_value = ( process_number & 0x7F );

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				4, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	/* Parameter type and sequence number (field 5). */

	sequence_number = flowbus->sequence_number;

	/*---*/

	uint8_value = ( flowbus_parameter_type & 0x7F ) << 4;

	uint8_value |= ( sequence_number & 0x1F );

#if 0
	MX_DEBUG(-2,("%s: flowbus_parameter_type = %lu, sequence_number = %lu, "
			"parameter_byte = %#lx",
			fname, flowbus_parameter_type,
			(unsigned long) sequence_number,
			(unsigned long) uint8_value ));
#endif

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				5, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	/* Process number (field 6). */

	uint8_value = ( process_number & 0x7F );

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				6, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	/* Parameter type and parameter number (field 7). */

	uint8_value = ( flowbus_parameter_type & 0x7F ) << 4;

	uint8_value |= ( parameter_number & 0x1F );

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				7, MXFT_UINT8, &uint8_value );

	message_length++;

	/*---*/

	/* If this is a string field, then we must append 
	 * the expected string length (field 8).
	 */

	if ( flowbus_parameter_type == MXDT_FLOWBUS_STRING ) {
		expected_string_length = max_parameter_length;

		mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				8, MXFT_UINT8, &expected_string_length );

		message_length++;
	}

	/*---*/

	/* Message length (field 1). */

	uint8_value = message_length;

	mxi_flowbus_format_string( ascii_command_buffer,
				sizeof(ascii_command_buffer),
				1, MXFT_UINT8, &uint8_value );

	/******* Send the command and receive the response *******/

	mx_status = mxi_flowbus_command( flowbus, ascii_command_buffer,
					ascii_response_buffer,
					sizeof(ascii_response_buffer),
					flowbus_command_flags );

	flowbus->sequence_number++;

	flowbus->sequence_number &= 0x1F;

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/******* Parse the response from the device *******/

#if 0
	MX_DEBUG(-2,("%s: response = '%s'", fname, ascii_response_buffer));
#endif

	/* FIXME? - For now we do not parse most of the message header. */

	/* The command code for a response here should be 2.  Is it? */

	nibble1 = 0xF & mx_hex_char_to_unsigned_long( ascii_response_buffer[5]);
	nibble0 = 0xF & mx_hex_char_to_unsigned_long( ascii_response_buffer[6]);

	command_code = (nibble1 << 4) | nibble0;

#if 0
	MX_DEBUG(-2,("response command code = %#lx",
		(unsigned long) command_code ));
#endif

	if ( command_code != 0x2 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The command code in the response should be 0x2.  "
		"However, it was %#lx.  response = '%s'.",
			(unsigned long) command_code,
			ascii_response_buffer );
	}

	/* Now we parse the returned parameter value. */

	value_string_ptr = 11 + (char *) ascii_response_buffer;

#if 0
	MX_DEBUG(-2,("value_string_ptr = '%s'", value_string_ptr));
#endif

	switch( flowbus_parameter_type ) {

	case MXDT_FLOWBUS_UINT8:
		value_string_ptr[2] = '\0';

		uint8_value =
	    0xFF & mx_hex_string_to_unsigned_long( value_string_ptr );

		*( (uint8_t *) requested_parameter_value ) = uint8_value;
		break;

	case MXDT_FLOWBUS_USHORT:
		value_string_ptr[4] = '\0'; 
		uint16_value = 
	    0xFFFF & mx_hex_string_to_unsigned_long( value_string_ptr );

		*( (uint16_t *) requested_parameter_value ) = uint16_value;
		break;

	case MXDT_FLOWBUS_ULONG_FLOAT:
		value_string_ptr[8] = '\0';

		uint32_value =
	    0xFFFFFFFF & mx_hex_string_to_unsigned_long( value_string_ptr );

		*( (uint32_t *) requested_parameter_value ) = uint32_value;
		break;

	case MXDT_FLOWBUS_STRING:
		nibble1 = 0xF &
			mx_hex_char_to_unsigned_long( value_string_ptr[0] );

		nibble0 = 0xF &
			mx_hex_char_to_unsigned_long( value_string_ptr[1] );

		flowbus_string_length = (size_t)( (nibble1 << 4) | nibble0 );

		if ( flowbus_string_length > max_parameter_length ) {
			allowed_string_length = max_parameter_length;
		} else {
			allowed_string_length = flowbus_string_length;
		}

		/* Move value_string_ptr to the beginning of the
		 * actual string data.
		 */

		value_string_ptr += 2;

		/* Copy out the string data. */

		requested_string_ptr = requested_parameter_value;

		for ( i = 0; i < allowed_string_length; i++ ) {
			byte_string[0] = value_string_ptr[2*i];
			byte_string[1] = value_string_ptr[2*i + 1];
			byte_string[2] = '\0';

			ulong_value =
				mx_hex_string_to_unsigned_long( byte_string );

			requested_string_ptr[i] =
				(unsigned char)( 0xff & ulong_value );
		}

		requested_string_ptr[ flowbus_string_length ] = '\0';
		break;
	}

	return mx_status;
}

/*==================================================================*/

MX_EXPORT mx_status_type
mxi_flowbus_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_FB_SHOW_NODES:
		case MXLV_FB_UINT8_VALUE:
		case MXLV_FB_USHORT_VALUE:
		case MXLV_FB_ULONG_VALUE:
		case MXLV_FB_FLOAT_VALUE:
		case MXLV_FB_STRING_VALUE:
			record_field->process_function
					= mxi_flowbus_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}


/*==================================================================*/

static mx_status_type
mxi_flowbus_process_function( void *record_ptr,
			void *record_field_ptr,
			void *socket_handler_ptr,
			int operation )
{
	static const char fname[] = "mxi_flowbus_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_FLOWBUS *flowbus;
	mx_status_type mx_status;

	unsigned long flowbus_parameter_type;
	void *flowbus_parameter_ptr;
	size_t flowbus_max_length;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	flowbus = (MX_FLOWBUS *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( record_field->label_value ) {
	case MXLV_FB_SHOW_NODES:
		break;
	case MXLV_FB_UINT8_VALUE:
		flowbus_parameter_type = MXDT_FLOWBUS_UINT8;
		flowbus_parameter_ptr = &(flowbus->uint8_value);
		flowbus_max_length = sizeof(unsigned char);
		break;
	case MXLV_FB_USHORT_VALUE:
		flowbus_parameter_type = MXDT_FLOWBUS_USHORT;
		flowbus_parameter_ptr = &(flowbus->ushort_value);
		flowbus_max_length = sizeof(unsigned short);
		break;
	case MXLV_FB_ULONG_VALUE:
		flowbus_parameter_type = MXDT_FLOWBUS_ULONG_FLOAT;
		flowbus_parameter_ptr = &(flowbus->ulong_value);
		flowbus_max_length = sizeof(unsigned long);
		break;
	case MXLV_FB_FLOAT_VALUE:
		flowbus_parameter_type = MXDT_FLOWBUS_ULONG_FLOAT;
		flowbus_parameter_ptr = &(flowbus->float_value);
		flowbus_max_length = sizeof(float);
		break;
	case MXLV_FB_STRING_VALUE:
		flowbus_parameter_type = MXDT_FLOWBUS_STRING;
		flowbus_parameter_ptr = flowbus->string_value;
		flowbus_max_length = MXU_FLOWBUS_MAX_STRING_LENGTH;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal datatype %lu requested for Flowbus interface '%s'.",
			record_field->label_value, record->name );
		break;
	}

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_FB_SHOW_NODES:
			break;
		case MXLV_FB_UINT8_VALUE:
		case MXLV_FB_USHORT_VALUE:
		case MXLV_FB_ULONG_VALUE:
		case MXLV_FB_FLOAT_VALUE:
		case MXLV_FB_STRING_VALUE:
			mx_status = mxi_flowbus_request_parameter( flowbus,
						flowbus->node_address,
						flowbus->process_number,
						flowbus->parameter_number,
						flowbus_parameter_type,
						flowbus_parameter_ptr,
						flowbus_max_length, 0 );
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_FB_SHOW_NODES:
			mx_status = mxi_flowbus_show_nodes( flowbus );
			break;
		case MXLV_FB_UINT8_VALUE:
		case MXLV_FB_USHORT_VALUE:
		case MXLV_FB_ULONG_VALUE:
		case MXLV_FB_FLOAT_VALUE:
		case MXLV_FB_STRING_VALUE:
			mx_status = mxi_flowbus_send_parameter( flowbus,
						flowbus->node_address,
						flowbus->process_number,
						flowbus->parameter_number,
						flowbus_parameter_type,
						flowbus_parameter_ptr,
						NULL, 0, 0 );
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

static mx_status_type
mxi_flowbus_show_nodes( MX_FLOWBUS *flowbus )
{
	static const char fname[] = "mxi_flowbus_show_nodes()";

	unsigned long node;
	char device_type[8];
	unsigned char identification_number;
	char firmware_version[8];
	char model_number[30];
	char serial_number[24];
	mx_status_type mx_status;

	if ( flowbus == (MX_FLOWBUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FLOWBUS pointer passed was NULL." );
	}

	for ( node = MXA_FLOWBUS_MIN_NODE_ADDRESS;
		node <= MXA_FLOWBUS_MAX_NODE_ADDRESS;
		node++ )
	{
		/* Begin by getting the device type string. */

		mx_status = mxi_flowbus_request_parameter( flowbus, node,
						113, 1,
						MXDT_FLOWBUS_STRING,
						device_type,
						sizeof(device_type)-1,
						MXFCF_FLOWBUS_QUIET );

		/* Strip off the 'quiet' flag if present. */

		mx_status.code &= (~MXE_QUIET);

		if ( mx_status.code == MXE_NOT_FOUND ) {
			continue;
		} else
		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}

		fprintf( stderr, "Node %3lu: type '%s'", node, device_type );

		/* Next we get the identification number. */

		mx_status = mxi_flowbus_request_parameter( flowbus, node,
						113, 12,
						MXDT_FLOWBUS_UINT8,
						&identification_number,
						sizeof(identification_number),
						MXFCF_FLOWBUS_QUIET );

		mx_status.code &= (~MXE_QUIET);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fprintf( stderr, ", id number %lu",
			(unsigned long) identification_number );

		/* Get the firmware version. */

		mx_status = mxi_flowbus_request_parameter( flowbus, node,
						113, 5,
						MXDT_FLOWBUS_STRING,
						firmware_version,
						sizeof(firmware_version)-1,
						MXFCF_FLOWBUS_QUIET );

		mx_status.code &= (~MXE_QUIET);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fprintf( stderr, ", firmware '%s'", firmware_version );

		/* Get the model number. */

		mx_status = mxi_flowbus_request_parameter( flowbus, node,
						113, 2,
						MXDT_FLOWBUS_STRING,
						model_number,
						sizeof(model_number)-1,
						MXFCF_FLOWBUS_QUIET );

		mx_status.code &= (~MXE_QUIET);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fprintf( stderr, ", model number '%s'", model_number );

		/* Get the serial number. */

		mx_status = mxi_flowbus_request_parameter( flowbus, node,
						113, 3,
						MXDT_FLOWBUS_STRING,
						serial_number,
						sizeof(serial_number)-1,
						MXFCF_FLOWBUS_QUIET );

		mx_status.code &= (~MXE_QUIET);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fprintf( stderr, ", serial number '%s'", serial_number );

		fprintf( stderr, "\n" );
	}

	return MX_SUCCESSFUL_RESULT;
}
