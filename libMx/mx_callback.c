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

#define MX_CALLBACK_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_interval_timer.h"
#include "mx_virtual_timer.h"
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

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
	MX_DEBUG(-2,("%s: server = '%s', nfname = '%s', handle = (%ld,%ld)",
		fname, nf->server_record->name, nf->nfname,
		nf->record_handle, nf->field_handle ));
#endif

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


	mx_network_update_message_id( &(server->last_rpc_message_id) );

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

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,
	("%s: The callback ID of type %lu for handle (%lu,%lu) is %#lx",
		fname, callback_type, nf->record_handle, nf->field_handle,
		callback_id ));
#endif

	/* Create a new client-side callback structure. */

	callback_ptr = malloc( sizeof(MX_CALLBACK) );

	if ( callback_ptr == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_CALLBACK structure" );
	}

	callback_ptr->callback_class    = MXCB_NETWORK;
	callback_ptr->callback_type     = callback_type;
	callback_ptr->callback_id       = callback_id;
	callback_ptr->callback_function = callback_function;
	callback_ptr->callback_argument = callback_argument;
	callback_ptr->u.network_field   = nf;

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

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

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
	MX_LIST *callback_list;
	MX_LIST_ENTRY *list_entry;

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

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', field '%s'.",
		fname, record->name, record_field->name ));
#endif

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Cannot get the MX_LIST_HEAD pointer for the record list "
		"containing the record '%s'.", record->name );
	}

	/* If database callbacks are not already initialized, then
	 * initialize them now.
	 */

	if ( list_head->callback_timer == NULL ) {
		mx_status = mx_field_initialize_callbacks( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	callback_handle_table = list_head->server_callback_handle_table;

	/* If the record field does not yet have a callback list, then
	 * create one now.  Otherwise, just use the list that is 
	 * already there.
	 */

	if ( record_field->callback_list == NULL ) {
		mx_status = mx_list_create( &callback_list );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		record_field->callback_list = callback_list;
	} else {
		callback_list = record_field->callback_list;
	}

	/* Allocate an MX_CALLBACK structure to contain the callback info. */

	callback_ptr = malloc( sizeof(MX_CALLBACK) );

	if ( callback_ptr == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to allocate an MX_CALLBACK structure.");
	}

	callback_ptr->callback_class    = MXCB_FIELD;
	callback_ptr->callback_type     = callback_type;
	callback_ptr->callback_function = callback_function;
	callback_ptr->callback_argument = callback_argument;
	callback_ptr->u.record_field    = record_field;

	MX_DEBUG(-2,("%s: callback_ptr->callback_function = %p",
		fname, callback_ptr->callback_function));

	/* Add this callback to the server's callback handle table. */

	mx_status = mx_create_handle( &callback_handle,
					callback_handle_table, callback_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	callback_ptr->callback_id =
		MX_NETWORK_MESSAGE_ID_MASK & (uint32_t) callback_handle;

	callback_ptr->callback_id |= MX_NETWORK_MESSAGE_IS_CALLBACK;

	/* Add this callback to the record field's callback list. */

	mx_status = mx_list_entry_create( &list_entry, callback_ptr, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_list_add_entry( callback_list, list_entry );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, return the new callback object to the caller. */

	if ( callback_object != (MX_CALLBACK **) NULL ) {
		*callback_object = callback_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

static void
mx_field_callback_function( MX_VIRTUAL_TIMER *callback_timer,
				void *callback_args )
{
	MX_LIST_HEAD *list_head;
#if 0
	static const char fname[] = "mx_field_callback_function()";

	MX_CLOCK_TICK current_clock_tick;

	current_clock_tick = mx_current_clock_tick();

	MX_DEBUG(-2,("%s: clock tick = (%lu,%lu)",
	fname, current_clock_tick.high_order, current_clock_tick.low_order));
#endif

	list_head = callback_args;

	list_head->callback_timer_count++;
	
	list_head->callback_timer_expired = TRUE;
}

MX_EXPORT mx_status_type
mx_field_initialize_callbacks( MX_RECORD *record )
{
	static const char fname[] = "mx_field_initialize_callbacks()";

	MX_LIST_HEAD *list_head;
	MX_HANDLE_TABLE *callback_handle_table;
	MX_INTERVAL_TIMER *master_timer;
	MX_VIRTUAL_TIMER *callback_timer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			record->name );
	}

	/* If the list head does not already have a server callback handle
	 * table, then create one now.  Otherwise, just use the table that
	 * is already there.
	 */
	
	if ( list_head->server_callback_handle_table == NULL ) {

		/* The handle table starts with a single block of 100 handles.*/

		mx_status = mx_create_handle_table( &callback_handle_table,
							100, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		list_head->server_callback_handle_table = callback_handle_table;
	}

	/* If the list head does not have a master timer, then create one
	 * with a timer period of 100 milliseconds.
	 */

	if ( list_head->master_timer == NULL ) {
		mx_status = mx_virtual_timer_create_master(&master_timer, 0.1);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		list_head->master_timer = master_timer;
	}

	/* If the list head does not have a callback timer, then create one
	 * derived from the master timer.
	 */

	if ( list_head->callback_timer == NULL ) {
		mx_status = mx_virtual_timer_create( &callback_timer,
						list_head->master_timer,
						MXIT_PERIODIC_TIMER,
						mx_field_callback_function,
						list_head );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		list_head->callback_timer = callback_timer;

		list_head->callback_timer_expired = FALSE;
		list_head->callback_timer_count = 0;

		mx_status = mx_virtual_timer_start( list_head->callback_timer,
							0.1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_field_invoke_callback_list( MX_RECORD_FIELD *field,
				unsigned long callback_type )
{
	static const char fname[] = "mx_field_invoke_callback_list()";

	MX_LIST *callback_list;
	MX_LIST_ENTRY *list_start, *list_entry, *next_list_entry;
	MX_CALLBACK *callback;
	mx_status_type mx_status;

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s invoked for field '%s', callback_type = %lu",
		fname, field->name, callback_type));
#endif

	callback_list = field->callback_list;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: callback_list = %p", fname, callback_list));
#endif

	if ( callback_list == (MX_LIST *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	list_start = callback_list->list_start;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: list_start = %p", fname, list_start));
#endif

	if ( list_start == (MX_LIST_ENTRY *) NULL ) {
#if 0
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The list_start entry for callback list %p used by "
		"record field '%s.%s' is NULL.", callback_list,
			field->record->name, field->name );
#else
		mx_warning("%s: The list_start entry for callback list %p "
		"used by record field '%s.%s' is NULL.", fname,
			callback_list, field->record->name, field->name );

		return MX_SUCCESSFUL_RESULT;
#endif
	}

	list_entry = list_start;

	do {

#if MX_CALLBACK_DEBUG
		MX_DEBUG(-2,("%s: list_entry = %p", fname, list_entry));
#endif

		next_list_entry = list_entry->next_list_entry;

		if ( next_list_entry == (MX_LIST_ENTRY *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The next_list_entry pointer for list entry %p "
			"of callback list %p for record field '%s.%s' is NULL.",
				list_entry, callback_list,
				field->record->name, field->name );
		}

		callback = list_entry->list_entry_data;

		if ( callback != (MX_CALLBACK *) NULL ) {
			
			if ( callback->callback_function != NULL ) {
				/* Invoke the callback. */

				mx_status = mx_invoke_callback( callback );

				/* If the callback is invalid, remove the
				 * callback from the callback list.
				 */

				if ( mx_status.code == MXE_INVALID_CALLBACK ) {

					mx_status = mx_list_delete_entry(
						callback_list, list_entry );

					if ( mx_status.code == MXE_SUCCESS ) {
						mx_list_entry_destroy(
							list_entry );
					}

					/* If the callback we just deleted
					 * was the list_start callback, then
					 * we must load the new value of the
					 * list_start callback.
					 */

					list_start = callback_list->list_start;

					if (list_start == (MX_LIST_ENTRY *)NULL)
					{
						mx_warning(
				"%s: The list_start entry ended up as NULL "
				"after a callback was deleted.", fname );

						return MX_SUCCESSFUL_RESULT;
					}
				}
			}
		}

		list_entry = next_list_entry;

	} while ( list_entry != list_start );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_invoke_callback( MX_CALLBACK *callback )
{
	static const char fname[] = "mx_invoke_callback()";

	mx_status_type (*function)( MX_CALLBACK *, void * );
	void *argument;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for callback %p", fname, callback));

	if ( callback == (MX_CALLBACK *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	function = callback->callback_function;
	argument = callback->callback_argument;

	MX_DEBUG(-2,("%s: function = %p, argument = %p",
		fname, function, argument));

	if ( function == NULL ) {
		MX_DEBUG(-2,("%s: Aborting since function == NULL.", fname));

		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG(-2,("%s: callback->active = %d", fname, callback->active));

	if ( callback->active ) {
		MX_DEBUG(-2,("%s: Aborting since callback->active != 0", fname));

		return MX_SUCCESSFUL_RESULT;
	}

	MX_DEBUG(-2,
	("%s: REALLY invoking callback function %p", fname, function));

	callback->active = TRUE;

	mx_status = (*function)( callback, argument );

	callback->active = FALSE;

	return mx_status;
}

