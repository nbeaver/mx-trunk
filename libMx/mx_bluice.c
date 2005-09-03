/*
 * Name:    mx_bluice.c
 *
 * Purpose: Client side part of Blu-Ice network protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_bluice.h"

#define BLUICE_DEBUG		TRUE

#define BLUICE_DEBUG_MESSAGE	TRUE

#define BLUICE_DEBUG_TIMING	TRUE

#if BLUICE_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

static mx_status_type
mx_bluice_get_pointers( MX_RECORD *bluice_server_record,
			MX_BLUICE_SERVER **bluice_server,
			MX_SOCKET **bluice_server_socket,
			const char *calling_fname )
{
	static const char fname[] = "mx_bluice_get_pointers()";

	MX_BLUICE_SERVER *bluice_server_ptr;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_server_ptr = (MX_BLUICE_SERVER *)
				bluice_server_record->record_class_struct;

	if ( bluice_server_ptr == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for record '%s' is NULL.",
			bluice_server_record->name );
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = bluice_server_ptr;
	}

	if ( bluice_server_socket != (MX_SOCKET **) NULL ) {
		*bluice_server_socket = bluice_server_ptr->socket;

		if ( *bluice_server_socket == NULL ) {
			return mx_error(
				MXE_NETWORK_CONNECTION_LOST, calling_fname,
				"Blu-Ice server '%s' is not connected.",
					bluice_server_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_send_message( MX_RECORD *bluice_server_record,
			char *text_data,
			char *binary_data,
			size_t binary_data_length,
			size_t required_data_length,
			int send_header )
{
	static const char fname[] = "mx_bluice_send_message()";

	MX_BLUICE_SERVER *bluice_server;
	MX_SOCKET *bluice_server_socket;
	char message_header[MX_BLUICE_MSGHDR_LENGTH];
	char null_pad[500];
	size_t text_data_length, bytes_sent, null_bytes_to_send;
	mx_status_type mx_status;

	mx_status = mx_bluice_get_pointers( bluice_server_record,
				&bluice_server, &bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( required_data_length > sizeof(null_pad) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The required data length %lu is longer than the "
		"maximum allowed length of %lu.",
			(unsigned long) required_data_length,
			(unsigned long) sizeof(null_pad) );
	}

#if BLUICE_DEBUG_MESSAGE
	MX_DEBUG(-2,("%s: sending '%s' to server '%s'.",
			fname, text_data, bluice_server_record->name ));

	if ( ( binary_data_length > 0 ) && ( binary_data != NULL ) ) {
		size_t i;

		fprintf(stderr, "%s: Binary data: ", fname);

		for ( i = 0; i < binary_data_length; i++ ) {
			fprintf(stderr, "%#x ", binary_data[i]);
		}
		fprintf(stderr, "\n");
	}
#endif

	bytes_sent = 0;

	text_data_length = strlen( text_data ) + 1;

	if ( send_header ) {
		sprintf( message_header, "%*lu%*lu",
			MX_BLUICE_MSGHDR_TEXT_LENGTH,
			(unsigned long) text_data_length,
			MX_BLUICE_MSGHDR_BINARY_LENGTH,
			(unsigned long) binary_data_length );

		/* Send the message header. */

		mx_status = mx_socket_send( bluice_server_socket,
					&message_header,
					MX_BLUICE_MSGHDR_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bytes_sent += MX_BLUICE_MSGHDR_LENGTH;
	}

	/* Send the text data. */

	mx_status = mx_socket_send( bluice_server_socket,
					text_data,
					text_data_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bytes_sent += text_data_length;

	/* Send the binary data (if any). */

	if ( ( binary_data_length > 0 ) && ( binary_data != NULL ) ) {
		mx_status = mx_socket_send( bluice_server_socket,
						binary_data,
						binary_data_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bytes_sent += binary_data_length;
	}

	if ( required_data_length >= 0 ) {
		if ( bytes_sent < required_data_length ) {
			null_bytes_to_send = required_data_length - bytes_sent;

			memset( null_pad, 0, null_bytes_to_send );

			mx_status = mx_socket_send( bluice_server_socket,
						null_pad,
						null_bytes_to_send );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_receive_message( MX_RECORD *bluice_server_record,
				char *data_buffer,
				size_t data_buffer_length,
				size_t *actual_data_length )
{
	static const char fname[] = "mx_bluice_receive_message()";

	MX_BLUICE_SERVER *bluice_server;
	MX_SOCKET *bluice_server_socket;
	char message_header[MX_BLUICE_MSGHDR_LENGTH];
	size_t text_data_length, binary_data_length, total_data_length;
	size_t maximum_length;
	char *data_pointer;
	mx_status_type mx_status;

	mx_status = mx_bluice_get_pointers( bluice_server_record,
				&bluice_server, &bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_receive( bluice_server_socket,
					message_header,
					MX_BLUICE_MSGHDR_LENGTH,
					NULL, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the message header is null terminated. */

	message_header[MX_BLUICE_MSGHDR_LENGTH-1] = '\0';

	/* Get the length of the binary data. */

	binary_data_length = atol( &message_header[MX_BLUICE_MSGHDR_BINARY] );

	if ( binary_data_length != 0 ) {
		mx_warning( "The binary data length for the message being "
		"received from Blu-Ice server '%s' is not 0.  "
		"Instead, it is %lu.",
			bluice_server_record->name,
			(unsigned long) binary_data_length );
	}

	/* Get the length of the text data. */

	message_header[MX_BLUICE_MSGHDR_BINARY] = '\0'; /* Null terminate it. */

	text_data_length = atol( message_header );

	total_data_length = text_data_length + binary_data_length;

#if 0
	MX_DEBUG(-2,("%s: text = %lu, binary = %lu, total = %lu",
		fname, (unsigned long) text_data_length,
		(unsigned long) binary_data_length,
		(unsigned long) total_data_length));
#endif

	if ( data_buffer == NULL ) {
		data_pointer = bluice_server->receive_buffer;
		maximum_length = bluice_server->receive_buffer_length;
	} else {
		data_pointer = data_buffer;
		maximum_length = data_buffer_length;
	}

	if ( total_data_length > maximum_length ) {
		mx_warning( "Truncating a %lu byte message body from "
		"Blu-Ice server '%s' to %lu bytes.",
			(unsigned long) total_data_length,
			bluice_server_record->name,
			(unsigned long) maximum_length );

		total_data_length = maximum_length;
	}

	/* Now receive the data for the message. */

	mx_status = mx_socket_receive( bluice_server_socket,
					data_pointer,
					total_data_length,
					actual_data_length,
					NULL, 0 );

#if BLUICE_DEBUG_MESSAGE
	MX_DEBUG(-2,("%s: received '%s' from server '%s'.",
		fname, data_pointer, bluice_server_record->name));
#endif

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_num_unread_bytes_available( MX_RECORD *bluice_server_record,
					int *num_bytes_available )
{
	static const char fname[] = "mx_bluice_num_unread_bytes_available()";

	MX_BLUICE_SERVER *bluice_server;
	MX_SOCKET *bluice_server_socket;
	mx_status_type mx_status;

	mx_status = mx_bluice_get_pointers( bluice_server_record,
				&bluice_server, &bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_available == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available argument passed was NULL." );
	}

	/* Check the server socket to see if any bytes are available. */

	if ( mx_socket_is_open( bluice_server_socket ) == FALSE ) {
		return mx_error( MXE_NETWORK_CONNECTION_LOST, fname,
			"Blu-Ice server'%s' is not connected.",
	    		bluice_server_record->name );
	}

	mx_status = mx_socket_num_input_bytes_available( bluice_server->socket,
							num_bytes_available );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_bluice_discard_unread_bytes( MX_RECORD *bluice_server_record )
{
	static const char fname[] = "mx_bluice_discard_unread_bytes()";

	MX_SOCKET *bluice_server_socket;
	mx_status_type mx_status;

	mx_status = mx_bluice_get_pointers( bluice_server_record, NULL,
					&bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_discard_unread_input( bluice_server_socket );

	return mx_status;
}

