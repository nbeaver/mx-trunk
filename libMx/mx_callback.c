/*
 * Name:    mx_callback.c
 *
 * Purpose: MX callback handling functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
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
#include "mx_net.h"
#include "mx_callback.h"

MX_EXPORT mx_status_type
mx_network_add_callback( MX_NETWORK_FIELD *nf,
			long callback_type,
			mx_status_type ( *callback_function )(
						MX_NETWORK_FIELD *, void * ),
			void *callback_argument )
{
	static const char fname[] = "mx_network_add_callback()";

	MX_RECORD *server_record;
	MX_LIST_HEAD *list_head;
	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	uint32_t *header, *uint32_message;
	unsigned long header_length, message_length, message_type, status_code;
	unsigned long data_type, message_id, callback_id;
	int new_handle_needed;
	mx_status_type mx_status;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked.", fname));
	MX_DEBUG(-2,("%s: server = '%s', nfname = '%s', handle = (%ld,%ld)",
		fname, nf->server_record->name, nf->nfname,
		nf->record_handle, nf->field_handle ));

	/* Make sure the network field is connected. */

	mx_status = mx_need_to_get_network_handle( nf, &new_handle_needed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( new_handle_needed ) {
		mx_status = mx_network_field_connect( nf );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* We must find a pointer to the list head structure for the
	 * current database, so that we can add the callback to the
	 * list there.
	 */

	server_record = nf->server_record;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The server_record pointer for network field '%s' is NULL.",
			nf->nfname );
	}

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the MX database is NULL." );
	}

	/* Find the message buffer for this server. */

	server = server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	message_buffer = server->message_buffer;

	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_MESSAGE_BUFFER for server '%s' is NULL.",
			server_record->name );
	}

	/* Construct a message to the server asking it to add a callback. */

	/*--- First, construct the header. ---*/

	header = message_buffer->u.uint32_buffer;

	header[MX_NETWORK_MAGIC]          = mx_htonl(MX_NETWORK_MAGIC_VALUE);
	header[MX_NETWORK_HEADER_LENGTH]  = mx_htonl(MXU_NETWORK_HEADER_LENGTH);
	header[MX_NETWORK_MESSAGE_TYPE]   = mx_htonl(MX_NETMSG_ADD_CALLBACK);
	header[MX_NETWORK_STATUS_CODE]    = mx_htonl(MXE_SUCCESS);
	header[MX_NETWORK_DATA_TYPE]      = mx_htonl(MXFT_ULONG);

	server->last_rpc_message_id++;

	if ( server->last_rpc_message_id == 0 )
		server->last_rpc_message_id = 1;

	header[MX_NETWORK_MESSAGE_ID] = mx_htonl(server->last_rpc_message_id);

	/*--- Then, fill in the message body. ---*/

	uint32_message = header + MXU_NETWORK_NUM_HEADER_VALUES;

	uint32_message[0] = mx_htonl( nf->record_handle );
	uint32_message[1] = mx_htonl( nf->field_handle );
	uint32_message[2] = mx_htonl( callback_type );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( 3 * sizeof(uint32_t) );

	/******** Now send the message. ********/

	mx_status = mx_network_send_message( server_record, message_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/******** Wait for the response. ********/

	mx_status = mx_network_wait_for_message_id( server_record,
						message_buffer,
						server->last_rpc_message_id,
						server->timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* These pointers may have been changed by buffer reallocation
	 * in mx_network_wait_for_message_id(), so we read them again.
	 */

	header = message_buffer->u.uint32_buffer;

	uint32_message = header + MXU_NETWORK_NUM_HEADER_VALUES;

	/* Read the header of the returned message. */

	header_length  = mx_ntohl( header[MX_NETWORK_HEADER_LENGTH] );
	message_length = mx_ntohl( header[MX_NETWORK_MESSAGE_LENGTH] );
	message_type   = mx_ntohl( header[MX_NETWORK_MESSAGE_TYPE] );
	status_code    = mx_ntohl( header[MX_NETWORK_STATUS_CODE] );

	if ( message_type != mx_server_response(MX_NETMSG_ADD_CALLBACK) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Message type for response was not %#lx.  "
		"Instead it was of type = %#lx.",
		    (unsigned long) mx_server_response(MX_NETMSG_ADD_CALLBACK),
		    (unsigned long) message_type );
	}

	/* If the header is not at least 28 bytes long, the the server
	 * does not support callbacks.
	 */

	if ( header_length < 28 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Server '%s' does not support callbacks.",
			server_record->name );
	}

	data_type  = mx_ntohl( header[MX_NETWORK_DATA_TYPE] );
	message_id = mx_ntohl( header[MX_NETWORK_MESSAGE_ID] );

	/* Get the callback ID from the returned message. */

	callback_id = mx_ntohl( uint32_message[0] );

	MX_DEBUG(-2,
	("%s: The callback ID of type %lu for handle (%lu,%lu) is %lu",
		fname, callback_type, nf->record_handle, nf->field_handle,
		callback_id ));

	return mx_error( MXE_NOT_YET_IMPLEMENTED,
			fname, "Not yet implemented.");
}

