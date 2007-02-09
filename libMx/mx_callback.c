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
			unsigned long callback_type,
			mx_status_type ( *callback_function )(
						MX_CALLBACK *, void * ),
			void *callback_argument,
			MX_CALLBACK **callback_object )
{
	static const char fname[] = "mx_network_add_callback()";

	MX_CALLBACK *callback_ptr;
	MX_LIST_ENTRY *list_entry;
	MX_RECORD *server_record;
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

	server_record = nf->server_record;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The server_record pointer for network field '%s' is NULL.",
			nf->nfname );
	}

	server = server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	/* If the server structure does not already have a callback list,
	 * then set one up for it.
	 */

	if ( server->callback_list == (MX_LIST *) NULL ) {
		mx_status = mx_list_create( &(server->callback_list) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Find the message buffer for this server. */

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

	/* If the header is not at least 28 bytes long, then the server
	 * is too old to support callbacks.
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
	("%s: The callback ID of type %lu for handle (%lu,%lu) is %#lx",
		fname, callback_type, nf->record_handle, nf->field_handle,
		callback_id ));

	/* Create a new client-side callback structure. */

	callback_ptr = malloc( sizeof(MX_CALLBACK) );

	if ( callback_ptr == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_CALLBACK structure" );
	}

	callback_ptr->callback_class    = MXCB_NETWORK;
	callback_ptr->callback_id       = callback_id;
	callback_ptr->callback_function = callback_function;
	callback_ptr->callback_argument = callback_argument;
	callback_ptr->u.network.network_field = nf;

	/* Add the callback to the server record's callback list.
	 * 
	 * FIXME: Does this list entry need a destructor?
	 */

	mx_status = mx_list_entry_create( &list_entry, callback_ptr, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_list_add_entry( server->callback_list, list_entry );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_field_add_callback( MX_RECORD_FIELD *record_field,
			unsigned long callback_type,
			mx_status_type ( *callback_function )(
						MX_CALLBACK *, void * ),
			void *callback_argument,
			MX_CALLBACK **callback_object )
{
	static const char fname[] = "mx_field_add_callback()";

	MX_CALLBACK *callback_ptr;
	MX_RECORD *record;
	MX_LIST_HEAD *list_head;
	MX_HANDLE_TABLE *callback_handle_table;
	signed long callback_handle;
	mx_status_type mx_status;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	/* Get a pointer to the record list head struct so that we
	 * can add the new callback there.
	 */

	record = record_field->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The MX_RECORD_FIELD passed has a NULL MX_RECORD pointer.  "
		"This probably means that it is a temporary record field "
		"object used by the MX networking code.  Callbacks are not "
		"supported for such record fields." );
	}

	MX_DEBUG(-2,("%s invoked for record '%s', field '%s'.",
		fname, record->name, record_field->name ));

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Cannot get the MX_LIST_HEAD pointer for the record list "
		"containing the record '%s'.", record->name );
	}

	/* If the list head does not already have a server callback handle
	 * table, then create one now.  Otherwise, just use the table that
	 * is already there.
	 */
	
	if ( list_head->server_callback_handle_table != NULL ) {
		callback_handle_table = list_head->server_callback_handle_table;
	} else {
		/* The handle table starts with a single block of 100 handles.*/

		mx_status = mx_create_handle_table( &callback_handle_table,
							100, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		list_head->server_callback_handle_table = callback_handle_table;
	}

	/* Allocate an MX_CALLBACK structure to contain the callback info. */

	callback_ptr = malloc( sizeof(MX_CALLBACK) );

	if ( callback_ptr == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to allocate an MX_CALLBACK structure.");
	}

	callback_ptr->callback_class = MXCB_FIELD;
	callback_ptr->callback_function = callback_function;
	callback_ptr->callback_argument = callback_argument;

	/* Add this callback to the callback handle table. */

	mx_status = mx_create_handle( &callback_handle,
					callback_handle_table, callback_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	callback_ptr->callback_id =
		MX_NETWORK_MESSAGE_ID_MASK & (uint32_t) callback_handle;

	callback_ptr->callback_id |= MX_NETWORK_MESSAGE_IS_CALLBACK;

	if ( callback_object != (MX_CALLBACK **) NULL ) {
		*callback_object = callback_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

