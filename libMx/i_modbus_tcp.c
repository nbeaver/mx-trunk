/*
 * Name:    i_modbus_tcp.c
 *
 * Purpose: MX driver for MODBUS/TCP fieldbus communication.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_MODBUS_TCP_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"

#if HAVE_TCPIP

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_socket.h"
#include "mx_modbus.h"
#include "i_modbus_tcp.h"

MX_RECORD_FUNCTION_LIST mxi_modbus_tcp_record_function_list = {
	NULL,
	mxi_modbus_tcp_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_modbus_tcp_open,
	mxi_modbus_tcp_close
};

MX_MODBUS_FUNCTION_LIST mxi_modbus_tcp_modbus_function_list = {
	mxi_modbus_tcp_command
};

MX_RECORD_FIELD_DEFAULTS mxi_modbus_tcp_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MODBUS_STANDARD_FIELDS,
	MXI_MODBUS_TCP_STANDARD_FIELDS
};

long mxi_modbus_tcp_num_record_fields
	= sizeof( mxi_modbus_tcp_record_field_defaults )
	/ sizeof( mxi_modbus_tcp_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_modbus_tcp_rfield_def_ptr
			= &mxi_modbus_tcp_record_field_defaults[0];

/* ---- */

/* A private function for the use of the driver. */

static mx_status_type
mxi_modbus_tcp_get_pointers( MX_MODBUS *modbus,
			MX_MODBUS_TCP **modbus_tcp,
			const char *calling_fname )
{
	const char fname[] = "mxi_modbus_tcp_get_pointers()";

	MX_RECORD *modbus_tcp_record;

	if ( modbus == (MX_MODBUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MODBUS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( modbus_tcp == (MX_MODBUS_TCP **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MODBUS_TCP pointer passed by '%s' was NULL.",
			calling_fname );
	}

	modbus_tcp_record = modbus->record;

	if ( modbus_tcp_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The modbus_tcp_record pointer for the "
			"MX_MODBUS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*modbus_tcp = (MX_MODBUS_TCP *) modbus_tcp_record->record_type_struct;

	if ( *modbus_tcp == (MX_MODBUS_TCP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MODBUS_TCP pointer for record '%s' is NULL.",
			modbus_tcp_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_modbus_tcp_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxi_modbus_tcp_create_record_structures()";

	MX_MODBUS *modbus;
	MX_MODBUS_TCP *modbus_tcp;

	/* Allocate memory for the necessary structures. */

	modbus = (MX_MODBUS *) malloc( sizeof(MX_MODBUS) );

	if ( modbus == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MODBUS structure." );
	}

	modbus_tcp = (MX_MODBUS_TCP *) malloc( sizeof(MX_MODBUS_TCP) );

	if ( modbus_tcp == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MODBUS_TCP structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = modbus;
	record->record_type_struct = modbus_tcp;
	record->class_specific_function_list
			= &mxi_modbus_tcp_modbus_function_list;

	modbus->record = record;
	modbus_tcp->record = record;

	modbus_tcp->socket = NULL;
	modbus_tcp->transaction_id = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_modbus_tcp_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_modbus_tcp_open()";

	MX_MODBUS *modbus;
	MX_MODBUS_TCP *modbus_tcp;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	modbus = (MX_MODBUS *) record->record_class_struct;

	mx_status = mxi_modbus_tcp_get_pointers( modbus, &modbus_tcp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( modbus_tcp->port_number != 502 ) {
		mx_warning(
		"MODBUS interface record '%s' is using TCP port number %ld "
		"rather than the standard MODBUS/TCP port number of 502.",
			record->name, modbus_tcp->port_number );
	}

	if ( modbus_tcp->unit_id > 0xff ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"MODBUS interface record '%s' is using illegal MODBUS/TCP "
	"unit identifier %#02lx.  The allowed values are from 0x0 to 0xff.  "
	"If you do not know what value to use for this, try using 0xff.",
			record->name, modbus_tcp->unit_id );
	}

	/* If the MODBUS/TCP socket is currently open, close it. */

	if ( mx_socket_is_open( modbus_tcp->socket ) ) {
		mx_status = mx_socket_close( modbus_tcp->socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now open the socket. */

	mx_status = mx_tcp_socket_open_as_client( &(modbus_tcp->socket),
			modbus_tcp->hostname, modbus_tcp->port_number,
			MXF_SOCKET_DISABLE_NAGLE_ALGORITHM | MXF_SOCKET_QUIET,
			MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_error( mx_status.code, fname,
"Failed to open TCP connection for record '%s' to host '%s' at port '%ld'.",
		  record->name, modbus_tcp->hostname, modbus_tcp->port_number );

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_modbus_tcp_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_modbus_tcp_close()";

	MX_MODBUS *modbus;
	MX_MODBUS_TCP *modbus_tcp;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	modbus = (MX_MODBUS *) record->record_class_struct;

	mx_status = mxi_modbus_tcp_get_pointers( modbus, &modbus_tcp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the MODBUS/TCP socket is currently open, close it. */

	if ( mx_socket_is_open( modbus_tcp->socket ) ) {
		mx_status = mx_socket_close( modbus_tcp->socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_modbus_tcp_reconnect_socket( MX_MODBUS *modbus )
{
	static const char fname[] = "mxi_modbus_tcp_reconnect_socket()";

	MX_MODBUS_TCP *modbus_tcp;
	mx_status_type mx_status;

	mx_status = mxi_modbus_tcp_get_pointers( modbus, &modbus_tcp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Throw away the old socket. */

	(void) mx_socket_close( modbus_tcp->socket );

	/* Try to open a new one. */

	mx_status = mx_tcp_socket_open_as_client(
			&(modbus_tcp->socket),
			modbus_tcp->hostname, modbus_tcp->port_number,
			MXF_SOCKET_DISABLE_NAGLE_ALGORITHM | MXF_SOCKET_QUIET,
			MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_error( mx_status.code, fname,
"Failed to open TCP connection for record '%s' to host '%s' at port '%ld'.",
				modbus_tcp->record->name,
				modbus_tcp->hostname,
				modbus_tcp->port_number );

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_modbus_tcp_send_request( MX_MODBUS *modbus )
{
	static const char fname[] = "mxi_modbus_tcp_send_request()";

	MX_MODBUS_TCP *modbus_tcp;
	uint16_t transaction_id, length;
	mx_status_type mx_status;

	mx_status = mxi_modbus_tcp_get_pointers( modbus, &modbus_tcp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the MODBUS/TCP header. */

	modbus_tcp->transaction_id++;

	transaction_id = modbus_tcp->transaction_id;

	modbus_tcp->send_header[MX_MODBUS_TCP_TRANSACTION_ID]
					= ( transaction_id >> 8 ) & 0xff;

	modbus_tcp->send_header[MX_MODBUS_TCP_TRANSACTION_ID+1]
					= transaction_id & 0xff;

	modbus_tcp->send_header[MX_MODBUS_TCP_PROTOCOL_ID] = 0;
	modbus_tcp->send_header[MX_MODBUS_TCP_PROTOCOL_ID+1] = 0;

	length = (uint16_t) modbus->request_length + 1;

	modbus_tcp->send_header[MX_MODBUS_TCP_LENGTH] = ( length >> 8 ) & 0xff;
	modbus_tcp->send_header[MX_MODBUS_TCP_LENGTH+1] = length & 0xff;

	modbus_tcp->send_header[MX_MODBUS_TCP_UNIT_ID]
			= (uint8_t) modbus_tcp->unit_id;

#if MXI_MODBUS_TCP_DEBUG
	{
		int i;

		fprintf( stderr, "Sending to MODBUS/TCP record '%s':\n",
				modbus->record->name );
		fprintf( stderr,
			"  TID = %#04x, LEN = %d, UID = %#02lx, FC = %#02x :",
				transaction_id, length, modbus_tcp->unit_id,
				modbus->request_pointer[0] );

		for ( i = 1; i < modbus->request_length; i++ ) {
			fprintf( stderr, " %#02x", modbus->request_pointer[i] );
		}

		fprintf( stderr, "\n" );
	}
#endif
	/* Send the MODBUS/TCP header. */

	mx_status = mx_socket_send( modbus_tcp->socket,
					modbus_tcp->send_header,
					MXU_MODBUS_TCP_HEADER_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the body of the message. */

	mx_status = mx_socket_send( modbus_tcp->socket,
					modbus->request_pointer,
					modbus->request_length );

	return mx_status;
}

static mx_status_type
mxi_modbus_tcp_receive_response( MX_MODBUS *modbus )
{
	static const char fname[] = "mxi_modbus_tcp_receive_response()";

	MX_MODBUS_TCP *modbus_tcp;
	uint8_t *receive_header;
	uint16_t transaction_id, protocol_id, length;
	uint8_t unit_id;
	mx_status_type mx_status;

	mx_status = mxi_modbus_tcp_get_pointers( modbus, &modbus_tcp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Receive the MODBUS/TCP header. */

	mx_status = mx_socket_receive( modbus_tcp->socket,
					modbus_tcp->receive_header,
					MXU_MODBUS_TCP_HEADER_LENGTH,
					NULL, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the values out of the header. */

	receive_header = modbus_tcp->receive_header;

	transaction_id = 
	  ((uint16_t) receive_header[MX_MODBUS_TCP_TRANSACTION_ID]) << 8;

	transaction_id |= 
	  ((uint16_t) receive_header[MX_MODBUS_TCP_TRANSACTION_ID+1]);

	if ( transaction_id != modbus_tcp->transaction_id ) {
		mx_warning( "Received transaction id %d does not match the "
		"transmitted transaction id %d for MODBUS/TCP interface '%s'.",
			transaction_id, modbus_tcp->transaction_id,
			modbus->record->name );
	}

	protocol_id =
	  ((uint16_t) receive_header[MX_MODBUS_TCP_PROTOCOL_ID]) << 8;

	protocol_id |=
	  ((uint16_t) receive_header[MX_MODBUS_TCP_PROTOCOL_ID+1]);

	if ( protocol_id != 0 ) {
		mx_warning( "Received MODBUS protocol id was %d rather than 0 "
		"for MODBUS/TCP interface '%s'.",
			protocol_id, modbus->record->name );
	}

	length =
	  ((uint16_t) receive_header[MX_MODBUS_TCP_LENGTH]) << 8;

	length |=
	  ((uint16_t) receive_header[MX_MODBUS_TCP_LENGTH+1]);

	/* Sometimes the Wago 750-841 fieldbus controller sends a bogus
	 * value for the length field, so we must be prepared to fix the
	 * value if necessary.  The extra '+ 1' is to compensate for the
	 * 'length--' statement further below.
	 */

	if ( length > (modbus->response_buffer_length + 1) ) {
#if 0
		mx_warning( "Response length %d is longer than the "
			"response buffer length of %d.  The length will be "
			"adjusted to %d.", length,
			modbus->response_buffer_length,
			modbus->response_buffer_length );
#endif

		length = modbus->response_buffer_length + 1;
	}

	if ( (length < 1) || (length > (MXU_MODBUS_TCP_ADU_LENGTH+1)) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Received message from MODBUS/TCP interface '%s' with an illegal "
	"length field of %d.  The allowed values are from 1 to %d.",
		modbus->record->name, length, MXU_MODBUS_TCP_ADU_LENGTH+1 );
	}

	unit_id = receive_header[MX_MODBUS_TCP_UNIT_ID];

	if ( unit_id != modbus_tcp->unit_id ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Received message from MODBUS/TCP interface '%s' has a unit id of %d "
	"rather than the expected value of %lu.",
			modbus->record->name, unit_id,
			modbus_tcp->unit_id );
	}

	/* Now that we have processed the header, receive the body of
	 * the message.
	 */

	length--;	/* Subtract 1 for the unit identifier field. */

	mx_status = mx_socket_receive( modbus_tcp->socket,
					modbus->response_pointer,
					length,
					NULL, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	modbus->actual_response_length = length;

#if MXI_MODBUS_TCP_DEBUG
	{
		int i;

		fprintf( stderr, "Received from MODBUS/TCP record '%s':\n",
				modbus->record->name );
		fprintf( stderr,
			"  TID = %#04x, LEN = %d, UID = %#02x, FC = %#02x :",
				transaction_id, length, unit_id,
				modbus->response_pointer[0] );

		for ( i = 1; i < length; i++ ) {
			fprintf( stderr, " %#02x", modbus->response_pointer[i]);
		}

		fprintf( stderr, "\n" );
	}
#endif

#if 1
	mx_status = mx_socket_discard_unread_input( modbus_tcp->socket );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_modbus_tcp_command( MX_MODBUS *modbus )
{
	static const char fname[] = "mxi_modbus_tcp_command()";

	mx_status_type mx_status;

	/* Send the request. */

	mx_status = mxi_modbus_tcp_send_request( modbus );

	if ( ( mx_status.code != MXE_SUCCESS )
	  && ( mx_status.code != MXE_NETWORK_CONNECTION_LOST ) )
	{
		return mx_status;
	}

	/* If there was an MXE_NETWORK_CONNECTION_LOST error, the server
	 * probably has closed the socket on its end, so we must reconnect
	 * and resend the command.
	 */

	if ( mx_status.code == MXE_NETWORK_CONNECTION_LOST ) {

		mx_status = mxi_modbus_tcp_reconnect_socket( modbus );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Now attempt to retransmit the command. */

		mx_status = mxi_modbus_tcp_send_request( modbus );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Read back the response. */

	mx_status = mxi_modbus_tcp_receive_response( modbus );

	/* If reading the response dies with an MXE_NETWORK_CONNECTION_LOST 
	 * error, we assume that the command was not successfully sent and
	 * needs to be resent.
	 */

	if ( mx_status.code == MXE_NETWORK_CONNECTION_LOST ) {

		MX_DEBUG(-2,("%s: retransmitting after failed receive.",
			fname));

		mx_status = mxi_modbus_tcp_reconnect_socket( modbus );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Attempt to retransmit the command. */

		mx_status = mxi_modbus_tcp_send_request( modbus );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Try to read back the response. */

		mx_status = mxi_modbus_tcp_receive_response( modbus );
	}

	return mx_status;
}

#endif /* HAVE_TCPIP */

