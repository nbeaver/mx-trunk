/*
 * Name:    mx_spec.c
 *
 * Purpose: Client side part of Spec network protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_inttypes.h"
#include "mx_socket.h"
#include "mx_spec.h"
#include "n_spec.h"

#define SPEC_DEBUG		FALSE

#define SPEC_DEBUG_MESSAGE	FALSE

#define SPEC_DEBUG_TIMING	FALSE

#if SPEC_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

static mx_status_type
mx_spec_get_pointers( MX_RECORD *spec_server_record,
			MX_SPEC_SERVER **spec_server,
			MX_SOCKET **spec_server_socket,
			const char *calling_fname )
{
	static const char fname[] = "mx_spec_get_pointers()";

	MX_SPEC_SERVER *spec_server_ptr;

	if ( spec_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	spec_server_ptr = (MX_SPEC_SERVER *)
				spec_server_record->record_type_struct;

	if ( spec_server_ptr == (MX_SPEC_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SPEC_SERVER pointer for record '%s' is NULL.",
			spec_server_record->name );
	}

	if ( spec_server != (MX_SPEC_SERVER **) NULL ) {
		*spec_server = spec_server_ptr;
	}

	if ( spec_server_socket != (MX_SOCKET **) NULL ) {
		*spec_server_socket = spec_server_ptr->socket;

		if ( *spec_server_socket == NULL ) {
			return mx_error(
				MXE_NETWORK_CONNECTION_LOST, calling_fname,
				"Spec server '%s' is not connected.",
					spec_server_ptr->server_id );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_spec_send_message( MX_RECORD *spec_server_record,
                        int spec_command_code,
                        int spec_datatype,
                        unsigned spec_array_data_rows,
                        unsigned spec_array_data_cols,
                        unsigned spec_data_length,
                        char *spec_property_name,
                        void *data_pointer )
{
	static const char fname[] = "mx_spec_send_message()";

	MX_SPEC_SERVER *spec_server;
	MX_SOCKET *spec_server_socket;
	struct svr_head message_header;
	size_t message_header_size;
	mx_status_type mx_status;

	mx_status = mx_spec_get_pointers( spec_server_record,
				&spec_server, &spec_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if SPEC_DEBUG_MESSAGE
	MX_DEBUG(-2,("%s: server = '%s', property = '%s', "
		"command code = %d, datatype = %d, "
		"rows = %u, cols = %u, len = %u, data_pointer = %p",
			fname, spec_server_record->name,
			spec_property_name, spec_command_code,
			spec_datatype, spec_array_data_rows,
			spec_array_data_cols, spec_data_length,
			data_pointer ));
#endif

	message_header_size = sizeof(struct svr_head);

	message_header.magic = SV_SPEC_MAGIC;
	message_header.vers = SV_VERSION;
	message_header.size = message_header_size;
	message_header.sn = 0;
	message_header.sec = 0;
	message_header.usec = 0;
	message_header.cmd = spec_command_code;
	message_header.type = spec_datatype;
	message_header.rows = spec_array_data_rows;
	message_header.cols = spec_array_data_cols;
	message_header.len = spec_data_length;

	strlcpy( message_header.name, spec_property_name, SV_NAME_LEN );

	/* Send the message header. */

	mx_status = mx_socket_send( spec_server_socket,
					&message_header,
					message_header_size );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now send the message data. */

	if (data_pointer != NULL) {
		mx_status = mx_socket_send( spec_server_socket,
						data_pointer,
						spec_data_length );
	}

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_spec_receive_message( MX_RECORD *spec_server_record,
                        int *spec_command_code,
                        int *spec_datatype,
                        unsigned *spec_array_data_rows,
                        unsigned *spec_array_data_cols,
                        unsigned maximum_data_length,
                        unsigned *spec_data_length,
                        char *spec_property_name,
                        void *data_pointer )
{
	static const char fname[] = "mx_spec_receive_message()";

	MX_SPEC_SERVER *spec_server;
	MX_SOCKET *spec_server_socket;
	struct svr_head message_header;
	size_t expected_message_header_size;
	mx_status_type mx_status;

	mx_status = mx_spec_get_pointers( spec_server_record,
				&spec_server, &spec_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	expected_message_header_size = sizeof(struct svr_head);

	/* Receive the header of the message. */

	mx_status = mx_socket_receive( spec_server_socket,
					&message_header,
					expected_message_header_size,
					NULL, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( message_header.magic != SV_SPEC_MAGIC ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Wrong Spec magic number %#x found in header from Spec server '%s'.",
			message_header.magic, spec_server_record->name );
	}
	if ( message_header.vers != SV_VERSION ) {
		mx_warning( "Spec server '%s' is using protocol version %d, "
				"but we were expecting version %d.",
				spec_server_record->name,
				message_header.vers,
				SV_VERSION );
	}
	if ( message_header.size != expected_message_header_size ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"The message header size %u for the header sent by Spec server '%s' "
	"does not match the expected value of %u.",
			message_header.size, spec_server_record->name,
			(unsigned int) expected_message_header_size );
	}

	if ( spec_command_code != NULL ) {
		*spec_command_code = message_header.cmd;
	}
	if ( spec_datatype != NULL ) {
		*spec_datatype = message_header.type;
	}
	if ( spec_array_data_rows != NULL ) {
		*spec_array_data_rows = message_header.rows;
	}
	if ( spec_array_data_cols != NULL ) {
		*spec_array_data_cols = message_header.cols;
	}
	if ( spec_data_length != NULL ) {
		*spec_data_length = message_header.len;
	}
	if ( spec_property_name != NULL ) {
		strlcpy( spec_property_name,
				message_header.name, SV_NAME_LEN );
	}

	if ( message_header.len > maximum_data_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The message data size of %u bytes from Spec server '%s' "
		"is larger than the maximum length %u specified for "
		"this call.", message_header.len,
			spec_server_record->name,
			maximum_data_length );
	}

	/* Now receive the data for the message. */

	if ( data_pointer != NULL ) {
		mx_status = mx_socket_receive( spec_server_socket,
						data_pointer,
						message_header.len,
						NULL, NULL, 0 );
	}

#if SPEC_DEBUG_MESSAGE
	MX_DEBUG(-2,(
	"%s: server = '%s', maximum length = %u, data_pointer = %p",
			fname, spec_server_record->name,
			maximum_data_length, data_pointer ));

	if ( spec_data_length != NULL ) {
		MX_DEBUG(-2,("%s: spec_data_length = %u",
			fname, *spec_data_length));
	}
#endif

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_spec_send_command( MX_RECORD *spec_server_record,
			int spec_command_code,
			char *command )
{
	int data_length;
	mx_status_type mx_status;

	data_length = strlen( command ) + 1;

	mx_status = mx_spec_send_message( spec_server_record,
					spec_command_code,
					SV_ARR_STRING,
					1, data_length, data_length, "",
					command );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_spec_receive_response_line( MX_RECORD *spec_server_record,
				char *response,
				size_t response_buffer_length )
{
	static const char fname[] = "mx_spec_receive_response_line()";

	MX_SPEC_SERVER *spec_server;
	MX_SOCKET *spec_server_socket;
	int spec_command_code, spec_datatype;
	unsigned data_rows, data_cols, data_length;
	char spec_property_name[SV_NAME_LEN+1];
	mx_status_type mx_status;

	mx_status = mx_spec_get_pointers( spec_server_record,
				&spec_server, &spec_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response buffer pointer passed was NULL." );
	}

#if SPEC_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'",
		fname, spec_server_record->name ));
#endif
	/* Read the response line from the server. */

	mx_status = mx_spec_receive_message( spec_server_record,
				&spec_command_code, &spec_datatype,
				&data_rows, &data_cols,
				response_buffer_length,
				&data_length, spec_property_name,
				response );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if SPEC_DEBUG
	MX_DEBUG(-2,("%s: spec_command_code = %d, spec_datatype = %d",
				fname, spec_command_code, spec_datatype));
	MX_DEBUG(-2,("%s: data_rows = %u, data_cols = %u, data_length = %u",
				fname, data_rows, data_cols, data_length));
	MX_DEBUG(-2,("%s: spec_property_name = '%s'",
				fname, spec_property_name));
	MX_DEBUG(-2,("%s: response = '%s'",
				fname, response));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_spec_num_response_bytes_available( MX_RECORD *spec_server_record,
					int *num_bytes_available )
{
	static const char fname[] = "mx_spec_num_response_bytes_available()";

	MX_SPEC_SERVER *spec_server;
	MX_SOCKET *spec_server_socket;
	mx_status_type mx_status;

	mx_status = mx_spec_get_pointers( spec_server_record,
				&spec_server, &spec_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_available == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available argument passed was NULL." );
	}

	/* Check the server socket to see if any bytes are available. */

	if ( mx_socket_is_open( spec_server_socket ) == FALSE ) {
		return mx_error( MXE_NETWORK_CONNECTION_LOST, fname,
		"Spec server'%s' is not connected.", spec_server->server_id );
	}

	mx_status = mx_socket_num_input_bytes_available( spec_server->socket,
						num_bytes_available );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_spec_discard_unread_responses( MX_RECORD *spec_server_record )
{
	static const char fname[] = "mx_spec_discard_unread_responses()";

	MX_SOCKET *spec_server_socket;
	mx_status_type mx_status;

	mx_status = mx_spec_get_pointers( spec_server_record, NULL,
					&spec_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_discard_unread_input( spec_server_socket );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_spec_get_array( MX_RECORD *spec_server_record,
			char *property_name,
			long spec_datatype,
			long num_dimensions,
			long *dimension_array,
			void *value )
{
	static const char fname[] = "mx_spec_get_array()";

	unsigned data_rows, data_columns, data_length;
	unsigned received_data_rows, received_data_columns;
	unsigned received_data_length;
	int received_command_code, received_datatype;
	char received_property_name[SV_NAME_LEN];
	mx_status_type mx_status;

	if ( ( num_dimensions < 1 ) || ( num_dimensions > 2 ) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested number of dimensions %ld for Spec server '%s' "
		"is outside the allowed range of values 1 or 2.",
			num_dimensions, spec_server_record->name );
	}

	switch( spec_datatype ) {
	case SV_DOUBLE:
		return mx_error( MXE_UNSUPPORTED, fname,
"Spec server '%s' currently cannot send data using the SV_DOUBLE datatype.",
			spec_server_record->name );
	case SV_STRING:
		if ( num_dimensions != 1 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Only 1-dimensional strings are supported by Spec "
			"server '%s' for datatype SV_STRING.",
				spec_server_record->name );
		}
		
		data_rows = 1;
		data_columns = dimension_array[0];
		data_length = data_columns;
		break;
	case SV_ERROR:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
    "A Spec client cannot transmit an SV_ERROR message to Spec server '%s'.",
			spec_server_record->name );
	case SV_ASSOC:
	case SV_ARR_DOUBLE:
	case SV_ARR_FLOAT:
	case SV_ARR_LONG:
	case SV_ARR_ULONG:
	case SV_ARR_SHORT:
	case SV_ARR_USHORT:
	case SV_ARR_CHAR:
	case SV_ARR_UCHAR:
	case SV_ARR_STRING:
		if ( num_dimensions == 1 ) {
			data_rows = 1;
			data_columns = dimension_array[0];
			data_length = data_columns;
		} else {
			data_rows = dimension_array[0];
			data_columns = dimension_array[1];
			data_length = data_rows * data_columns;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Unrecognized Spec datatype %ld for Spec server '%s', property '%s'.",
			spec_datatype, spec_server_record->name,
			property_name );
	}

	/* Multiply the data length by the size in bytes of an individual
	 * data element.
	 */

	switch( spec_datatype ) {
	case SV_STRING:
	case SV_ASSOC:
	case SV_ARR_STRING:
	case SV_ARR_CHAR:
	case SV_ARR_UCHAR:
		data_length *= 1;
		break;
	case SV_ARR_SHORT:
	case SV_ARR_USHORT:
		data_length *= 2;
		break;
	case SV_ARR_LONG:
	case SV_ARR_ULONG:
	case SV_ARR_FLOAT:
		data_length *= 4;
		break;
	case SV_ARR_DOUBLE:
		data_length *= 8;
		break;
	}

	/* Send the command to Spec. */

	mx_status = mx_spec_send_message( spec_server_record,
					SV_CHAN_READ, spec_datatype,
					data_rows, data_columns, data_length,
					property_name, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the response. */

	mx_status = mx_spec_receive_message( spec_server_record,
					&received_command_code,
					&received_datatype,
					&received_data_rows,
					&received_data_columns,
					data_length,
					&received_data_length,
					received_property_name,
					value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_spec_put_array( MX_RECORD *spec_server_record,
			char *property_name,
			long spec_datatype,
			long num_dimensions,
			long *dimension_array,
			void *value )
{
	static const char fname[] = "mx_spec_put_array()";

	unsigned data_rows, data_columns, data_length;
	mx_status_type mx_status;

	if ( ( num_dimensions < 1 ) || ( num_dimensions > 2 ) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested number of dimensions %ld for Spec server '%s' "
		"is outside the allowed range of values 1 or 2.",
			num_dimensions, spec_server_record->name );
	}

	switch( spec_datatype ) {
	case SV_DOUBLE:
		return mx_error( MXE_UNSUPPORTED, fname,
"Spec server '%s' currently cannot receive data using the SV_DOUBLE datatype.",
			spec_server_record->name );
	case SV_STRING:
		if ( num_dimensions != 1 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Only 1-dimensional strings are supported by Spec "
			"server '%s' for datatype SV_STRING.",
				spec_server_record->name );
		}
		
		data_rows = 1;
		data_columns = dimension_array[0];
		data_length = data_columns;
		break;
	case SV_ERROR:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
    "A Spec client cannot transmit an SV_ERROR message to Spec server '%s'.",
			spec_server_record->name );
	case SV_ASSOC:
	case SV_ARR_DOUBLE:
	case SV_ARR_FLOAT:
	case SV_ARR_LONG:
	case SV_ARR_ULONG:
	case SV_ARR_SHORT:
	case SV_ARR_USHORT:
	case SV_ARR_CHAR:
	case SV_ARR_UCHAR:
	case SV_ARR_STRING:
		if ( num_dimensions == 1 ) {
			data_rows = 1;
			data_columns = dimension_array[0];
			data_length = data_columns;
		} else {
			data_rows = dimension_array[0];
			data_columns = dimension_array[1];
			data_length = data_rows * data_columns;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Unrecognized Spec datatype %ld for Spec server '%s', property '%s'.",
			spec_datatype, spec_server_record->name,
			property_name );
	}

	/* Multiply the data length by the size in bytes of an individual
	 * data element.
	 */

	switch( spec_datatype ) {
	case SV_STRING:
	case SV_ASSOC:
	case SV_ARR_STRING:
	case SV_ARR_CHAR:
	case SV_ARR_UCHAR:
		data_length *= 1;
		break;
	case SV_ARR_SHORT:
	case SV_ARR_USHORT:
		data_length *= 2;
		break;
	case SV_ARR_LONG:
	case SV_ARR_ULONG:
	case SV_ARR_FLOAT:
		data_length *= 4;
		break;
	case SV_ARR_DOUBLE:
		data_length *= 8;
		break;
	}

	/* Send the command to Spec. */

	mx_status = mx_spec_send_message( spec_server_record,
					SV_CHAN_SEND, spec_datatype,
					data_rows, data_columns, data_length,
					property_name, value );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_spec_get_string( MX_RECORD *spec_server_record,
			char *property_name,
			char *string_value,
			size_t max_string_length )
{
	int returned_command_code, returned_datatype;
	unsigned returned_data_rows, returned_data_cols, returned_data_length;
	char returned_property_name[SV_NAME_LEN];
	mx_status_type mx_status;

#if SPEC_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	/* Request the string. */

#if SPEC_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_spec_send_message( spec_server_record,
						SV_CHAN_READ,
						SV_STRING,
						0, 0, 0,
						property_name, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Receive the string. */

	mx_status = mx_spec_receive_message( spec_server_record,
						&returned_command_code,
						&returned_datatype,
						&returned_data_rows,
						&returned_data_cols,
						max_string_length,
						&returned_data_length,
						returned_property_name,
						string_value );

#if SPEC_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, "mx_spec_get_string()", property_name );
#endif

#if SPEC_DEBUG
	MX_DEBUG(-2,("mx_spec_get_string(): received '%s' for '%s' from '%s'",
		string_value, property_name, spec_server_record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mx_spec_put_string( MX_RECORD *spec_server_record,
			char *property_name,
			char *string_value )
{
	size_t string_length;
	mx_status_type mx_status;

#if SPEC_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( string_value == NULL ) {
		string_length = 0;
	} else {
		string_length = strlen(string_value) + 1;
	}

	/* Send the string. */

#if SPEC_DEBUG
	MX_DEBUG(-2,("mx_spec_put_string(): sending '%s' for '%s' to '%s'",
		string_value, property_name, spec_server_record->name ));
#endif

#if SPEC_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_spec_send_message( spec_server_record,
						SV_CHAN_SEND,
						SV_STRING,
						1, string_length,
						string_length,
						property_name,
						string_value );

#if SPEC_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, "mx_spec_put_string()", property_name );
#endif

	return mx_status;
}

/*
 * WARNING: The following functions use MX datatypes rather than Spec datatypes.
 */

MX_EXPORT mx_status_type
mx_spec_get_number( MX_RECORD *spec_server_record,
			char *property_name,
			long mx_datatype,
			void *value )
{
	static const char fname[] = "mx_spec_get_number()";

	char string_buffer[80];
	char format[20];
	int num_items;
	mx_status_type mx_status;

	mx_status = mx_spec_get_string( spec_server_record,
					property_name,
					string_buffer,
					sizeof(string_buffer) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This code uses C99 style formatting macros from "mx_inttypes.h". */

	switch( mx_datatype ) {
	case MXFT_CHAR:
		strlcpy( format, "%c", sizeof(format) );
		break;
	case MXFT_UCHAR:
		strlcpy( format, "%c", sizeof(format) );
		break;
	case MXFT_INT8:
		strlcpy( format, "%" SCNd8, sizeof(format) );
		break;
	case MXFT_UINT8:
		strlcpy( format, "%" SCNu8, sizeof(format) );
		break;
	case MXFT_INT16:
		strlcpy( format, "%" SCNd16, sizeof(format) );
		break;
	case MXFT_UINT16:
		strlcpy( format, "%" SCNu16, sizeof(format) );
		break;
	case MXFT_INT32:
		strlcpy( format, "%" SCNd32, sizeof(format) );
		break;
	case MXFT_UINT32:
		strlcpy( format, "%" SCNu32, sizeof(format) );
		break;
	case MXFT_INT64:
		strlcpy( format, "%" SCNd64, sizeof(format) );
		break;
	case MXFT_UINT64:
		strlcpy( format, "%" SCNu64, sizeof(format) );
		break;
	case MXFT_HEX:
		strlcpy( format, "%" SCNx32, sizeof(format) );
		break;
	case MXFT_FLOAT:
		strlcpy( format, "%g", sizeof(format) );
		break;
	case MXFT_DOUBLE:
		strlcpy( format, "%lg", sizeof(format) );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Unsupported MX datatype %ld passed for property '%s' of Spec server '%s'.",
			mx_datatype, property_name, spec_server_record->name );
	}

	num_items = sscanf( string_buffer, format, value );

	if ( num_items != 1 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not find data of type %s in the response from "
		"Spec server '%s' for property '%s'.  Response = '%s'",
			mx_get_field_type_string( mx_datatype ),
			spec_server_record->name, property_name,
			string_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_spec_put_number( MX_RECORD *spec_server_record,
			char *property_name,
			long mx_datatype,
			void *value )
{
	static const char fname[] = "mx_spec_put_number()";

	char buffer[80];
	mx_status_type mx_status;

	switch( mx_datatype ) {
	case MXFT_CHAR:
		snprintf( buffer, sizeof(buffer),
				"%c", *((char *) value));
		break;
	case MXFT_UCHAR:
		snprintf( buffer, sizeof(buffer),
				"%c", *((unsigned char *) value));
		break;
	case MXFT_INT8:
		snprintf( buffer, sizeof(buffer),
				"%" PRId8, *((int8_t *) value));
		break;
	case MXFT_UINT8:
		snprintf( buffer, sizeof(buffer),
				"%" PRIu8, *((uint8_t *) value));
		break;
	case MXFT_INT16:
		snprintf( buffer, sizeof(buffer),
				"%" PRId16, *((int16_t *) value));
		break;
	case MXFT_UINT16:
		snprintf( buffer, sizeof(buffer),
				"%" PRIu16, *((uint16_t *) value));
		break;
	case MXFT_INT32:
		snprintf( buffer, sizeof(buffer),
				"%" PRId32, *((int32_t *) value));
		break;
	case MXFT_UINT32:
		snprintf( buffer, sizeof(buffer),
				"%" PRIu32, *((uint32_t *) value));
		break;
	case MXFT_INT64:
		snprintf( buffer, sizeof(buffer),
				"%" PRId64, *((int64_t *) value));
		break;
	case MXFT_UINT64:
		snprintf( buffer, sizeof(buffer),
				"%" PRIu64, *((uint64_t *) value));
		break;
	case MXFT_HEX:
		snprintf( buffer, sizeof(buffer),
				"%" PRIx32, *((uint32_t *) value));
		break;
	case MXFT_FLOAT:
		snprintf( buffer, sizeof(buffer),
				"%g", *((float *) value));
		break;
	case MXFT_DOUBLE:
		snprintf( buffer, sizeof(buffer),
				"%g", *((double *) value));
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Unsupported MX datatype %ld passed for property '%s' of Spec server '%s'.",
			mx_datatype, property_name, spec_server_record->name );
	}

	mx_status = mx_spec_put_string( spec_server_record,
					property_name,
					buffer );

	return mx_status;
}

