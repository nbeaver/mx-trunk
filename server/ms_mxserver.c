/*
 * Name:    ms_mxserver.c
 *
 * Purpose: This file implements the server side of the MX network protocol
 *          for the generic MX server.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* Normally, you should leave NETWORK_DEBUG_MESSAGES turned on. */

#define NETWORK_DEBUG_MESSAGES		TRUE

/* NETWORK_DEBUG displays the messages transmitted to and from the server
 * while NETWORK_DEBUG_VERBOSE shows much more information.
 */

#define NETWORK_DEBUG			FALSE

#define NETWORK_DEBUG_VERBOSE		FALSE

#define NETWORK_DEBUG_TIMING		FALSE

#define NETWORK_DEBUG_FIELD_NAMES	FALSE

#define NETWORK_DEBUG_HANDLES		FALSE

#define NETWORK_DEBUG_HEADER_LENGTH	FALSE

#define NETWORK_DEBUG_CALLBACKS		FALSE

#define NETWORK_PROTECT_VM_HANDLE_TABLE	FALSE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"
#include "mx_inttypes.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_handle.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_net_socket.h"
#include "mx_pipe.h"
#include "mx_array.h"
#include "mx_list.h"
#include "mx_bit.h"
#include "mx_process.h"
#include "mx_callback.h"
#include "mx_security.h"

#include "ms_mxserver.h"

#if HAVE_XDR
#  include "mx_xdr.h"
#endif

#if NETWORK_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

/*
 * If NETWORK_DEBUG_VERBOSE is TRUE, force NETWORK_DEBUG to TRUE.
 */

#if NETWORK_DEBUG_VERBOSE
#  undef NETWORK_DEBUG
#  undef NETWORK_DEBUG_FIELD_NAMES

#  define NETWORK_DEBUG			TRUE
#  define NETWORK_DEBUG_FIELD_NAMES	TRUE
#endif

MXSRV_MX_SERVER_SOCKET mxsrv_tcp_server_socket_struct;

#if HAVE_UNIX_DOMAIN_SOCKETS
MXSRV_MX_SERVER_SOCKET mxsrv_unix_server_socket_struct;
#endif

MXSRV_MX_SERVER_SOCKET mxsrv_ascii_server_socket_struct;

uint32_t
mxsrv_get_returning_message_type( uint32_t message_type )
{
	uint32_t i, returning_message_type;

	static uint32_t message_pairs[][2] = {

{MX_NETMSG_GET_ARRAY_BY_NAME, mx_server_response(MX_NETMSG_GET_ARRAY_BY_NAME)},
{MX_NETMSG_PUT_ARRAY_BY_NAME, mx_server_response(MX_NETMSG_PUT_ARRAY_BY_NAME)},
{MX_NETMSG_GET_ARRAY_BY_HANDLE,
			mx_server_response(MX_NETMSG_GET_ARRAY_BY_HANDLE)},
{MX_NETMSG_PUT_ARRAY_BY_HANDLE,
			mx_server_response(MX_NETMSG_PUT_ARRAY_BY_HANDLE)},
{MX_NETMSG_GET_FIELD_TYPE,    mx_server_response(MX_NETMSG_GET_FIELD_TYPE)},
{MX_NETMSG_GET_ATTRIBUTE,     mx_server_response(MX_NETMSG_GET_ATTRIBUTE)},
{MX_NETMSG_SET_ATTRIBUTE,     mx_server_response(MX_NETMSG_SET_ATTRIBUTE)},
{MX_NETMSG_SET_CLIENT_INFO,   mx_server_response(MX_NETMSG_SET_CLIENT_INFO)},
{MX_NETMSG_GET_OPTION,        mx_server_response(MX_NETMSG_GET_OPTION)},
{MX_NETMSG_SET_OPTION,        mx_server_response(MX_NETMSG_SET_OPTION)},
{MX_NETMSG_ADD_CALLBACK,      mx_server_response(MX_NETMSG_ADD_CALLBACK)},
{MX_NETMSG_DELETE_CALLBACK,   mx_server_response(MX_NETMSG_DELETE_CALLBACK)},

	};

	static long num_message_pairs = sizeof( message_pairs )
					/ sizeof( message_pairs[0] );

	for ( i = 0; i < num_message_pairs; i++ ) {
		if ( message_type == message_pairs[i][0] ) {
			break;		/* Exit the for() loop. */
		}
	}
	if ( i >= num_message_pairs ) {
		returning_message_type = MX_NETMSG_UNEXPECTED_ERROR;
	} else {
		returning_message_type = message_pairs[i][1];
	}
	return returning_message_type;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_free_client_socket_handler( MX_SOCKET_HANDLER *socket_handler,
			MX_SOCKET_HANDLER_LIST *socket_handler_list )
{
	static const char fname[] = "mxsrv_free_client_socket_handler()";

	MX_LIST_HEAD *list_head;
	MX_HANDLE_TABLE *callback_handle_table;
	MX_HANDLE_STRUCT *callback_handle_struct, *callback_handle_struct_array;
	long n;
	unsigned long i, array_size;
	signed long callback_handle;
	MX_CALLBACK *callback_ptr;
	MX_LIST *callback_socket_handler_list;
	MX_LIST_ENTRY *callback_socket_handler_list_entry;
	MX_LIST_ENTRY *list_start;
	unsigned long num_list_entries;
	MX_CALLBACK_SOCKET_HANDLER_INFO *csh_info;
	MX_LIST *field_callback_list;
	MX_LIST_ENTRY *field_callback_list_entry;
	MX_RECORD_FIELD *field;
	mx_status_type mx_status, mx_status1, mx_status2;

	n = socket_handler->handler_array_index;

	if ( socket_handler_list != NULL ) {
		socket_handler_list->array[n] = NULL;

		/* Update the list of fds to check in select(). */

		mxsrv_update_select_fds( socket_handler_list );
	}

	/* Announce that the client socket has gone away. */

	if ( n >= 0 ) {
		mx_info("Client %ld (socket %d) disconnected.",
			n, socket_handler->synchronous_socket->socket_fd);
	}

	list_head = socket_handler->list_head;

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the current database is NULL." );
	}

	/* If this handler had any callbacks, find them and delete them.
	 *
	 * FIXME: Find a simpler and/or higher performance way of managing
	 *        the callbacks.  There must be a better way.
	 *
	 *---------------------------------------------------------------
	 *
	 * In doing this, keep the following in mind:
	 *
	 * 1.  The server has an MX_SOCKET_HANDLER_LIST, which is not
	 *     the same thing as an MX_LIST!
	 *
	 * 2.  The server may have an MX_HANDLE_TABLE of callbacks.
	 *
	 * 3.  Every record field may have an MX_LIST of callbacks. Each
	 *     of the callbacks _must_ be of callback class MXCBC_FIELD.
	 *
	 * 4.  Every record field callback _must_ have an MX_LIST of
	 *     socket handlers which is stored as the callback argument.
	 *
	 *---------------------------------------------------------------
	 *
	 * The procedure we follow is the following:
	 *
	 * A.  Look at each handle in the server's callback MX_HANDLE_TABLE.
	 *
	 * B.  If the handle is valid and contains a callback pointer,
	 *     then examine the callback further.
	 *
	 * C.  If the callback_class for the callback is MXCBC_FIELD,
	 *     then examine the callback further.  In this case, the
	 *     "->u.record_field" member of the MX_CALLBACK structure
	 *     will contain a pointer to the associated MX_RECORD_FIELD.
	 *
	 * D.  The "->callback_argument" member of the MX_CALLBACK will
	 *     be a pointer to an MX_LIST of MX_SOCKET_HANDLERs for this
	 *     record field.  Look to see if the socket handler we are
	 *     looking for is present in this list.
	 *
	 * E.  If we find the MX_SOCKET_HANDLER in the socket handler
	 *     MX_LIST, then delete it from that list.
	 *
	 *     If we do not find it, then we are done with this handle
	 *     from the server's callback MX_HANDLE_TABLE and should go
	 *     on to the next one.
	 *
	 * F.  If this leaves the callback's socket handler MX_LIST empty,
	 *     then:
	 *     1.  Remove the callback from the server's callback handle
	 *         table.
	 *     2.  Remove the callback from the record field's MX_LIST
	 *         of callbacks.
	 *     3.  Delete the callback itself.
	 *
	 * G.  If this leaves the record field's MX_LIST of callbacks
	 *     empty, then delete the MX_LIST.
	 *
	 */

	callback_handle_table = list_head->server_callback_handle_table;

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s: callback_handle_table = %p",
		fname, callback_handle_table));
#endif

	if ( callback_handle_table != (MX_HANDLE_TABLE *) NULL ) {

		/* Look through the handle table for handles used
		 * by this socket handler.
		 */

		callback_handle_struct_array =
			callback_handle_table->handle_struct_array;

#if NETWORK_DEBUG_CALLBACKS
		MX_DEBUG(-2,("%s: callback_handle_struct_array = %p",
			fname, callback_handle_struct_array));
#endif

		if (callback_handle_struct_array == (MX_HANDLE_STRUCT *) NULL)
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The handle_struct_array table for the current "
			"database is NULL." );
		}

		array_size = callback_handle_table->block_size
				* callback_handle_table->num_blocks;

#if NETWORK_DEBUG_CALLBACKS
		MX_DEBUG(-2,("%s: array_size = %lu", fname, array_size));
#endif

		for ( i = 0; i < array_size; i++ ) {

			/* Step A */

			callback_handle_struct =
				&(callback_handle_struct_array[i]);

#if NETWORK_DEBUG_CALLBACKS
			MX_DEBUG(-2,("%s: i = %lu, callback_handle_struct = %p",
				fname, i, callback_handle_struct));
#endif

#if NETWORK_PROTECT_VM_HANDLE_TABLE
			mx_callback_handle_table_change_permissions(
					callback_handle_table, R_OK );
#endif

			callback_handle = callback_handle_struct->handle;
			callback_ptr    = callback_handle_struct->pointer;

#if NETWORK_PROTECT_VM_HANDLE_TABLE
			mx_callback_handle_table_change_permissions(
					callback_handle_table, 0 );
#endif

#if NETWORK_DEBUG_CALLBACKS
			MX_DEBUG(-2,
			("%s: callback_handle = %#lx, callback_ptr = %p",
				fname, callback_handle, callback_ptr));
#endif

			/* Step B */

			if ( (callback_handle != MX_ILLEGAL_HANDLE)
			  && (callback_ptr != NULL) )
			{
			    /* Step C */

#if NETWORK_DEBUG_CALLBACKS
			    MX_DEBUG(-2,
			    ("%s: callback_class = %lu, callback_type = %lu",
			    	fname, callback_ptr->callback_class,
				callback_ptr->callback_type));
#endif

			    /* Is this a record field callback? */

			    if ( callback_ptr->callback_class != MXCBC_FIELD )
			    {
#if NETWORK_DEBUG_CALLBACKS
				MX_DEBUG(-2,
				("%s: Callback %p is of class %lu.  Skipping.",
					fname, callback_ptr,
					callback_ptr->callback_class));
#endif
			    	/* This is not a record field callback,
				 * so skip on to the next callback.
				 */

				continue;
			    }

			    /* The callback must have a pointer to the
			     * record field.
			     */

			    field = callback_ptr->u.record_field;

#if NETWORK_DEBUG_CALLBACKS
			    MX_DEBUG(-2,("%s: record field = %p",
			    	fname, field));
#endif

			    if ( field == (MX_RECORD_FIELD *) NULL ) {
			    	(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				fname, "Record field callback %p does not "
				"have a pointer to the corresponding "
				"record field.", callback_ptr );

				continue;    /* Skip this broken callback. */
			    }

#if NETWORK_DEBUG_CALLBACKS
			    MX_DEBUG(-2,
		        ("%s: Callback %p is for record field '%s.%s', ID %#lx",
			    	fname, callback_ptr,
				field->record->name, field->name,
				(unsigned long) callback_ptr->callback_id));
#endif
			    /* Step D */

			    /* Get the socket handler MX_LIST for this
			     * record field callback.
			     */

			    callback_socket_handler_list =
			    		callback_ptr->callback_argument;

#if NETWORK_DEBUG_CALLBACKS
			    MX_DEBUG(-2,
			    ("%s: callback_socket_handler_list = %p",
			    	fname, callback_socket_handler_list));
#endif

			    if (callback_socket_handler_list == (MX_LIST *)NULL)
			    {
#if NETWORK_DEBUG_CALLBACKS
				MX_DEBUG(-2,
				("%s: The socket handler list for callback "
				"%p used by record field '%s.%s' is NULL.",
					fname, callback_ptr,
					field->record->name, field->name ));
#endif

				continue;    /* Skip this callback. */
			    }

			    num_list_entries =
			    	callback_socket_handler_list->num_list_entries;

			    list_start =
			    	callback_socket_handler_list->list_start;

#if NETWORK_DEBUG_CALLBACKS
			    MX_DEBUG(-2,("%s: #1 Callback %p, "
			    "num_list_entries = %lu, list_start = %p",
			    fname, callback_ptr, num_list_entries, list_start));
#endif

			    if ((num_list_entries == 0) || (list_start == NULL))
			    {
#if NETWORK_DEBUG_CALLBACKS
				MX_DEBUG(-2,
				("%s: callback_socket_handler_list %p "
				"for callback %p is empty.  Skipping...",
					fname, callback_socket_handler_list,
					callback_ptr ));
#endif

				continue;	/* Skip this callback. */
			    }

			    /* Step E */

			    /* See if the socket handler MX_LIST contains
			     * the socket handler that we are looking for.
			     */

#if NETWORK_DEBUG_CALLBACKS
			    MX_DEBUG(-2,("%s: looking for socket_handler %p "
			    	"on callback_socket_handler_list %p",
					fname, socket_handler,
					callback_socket_handler_list));
#endif

			    /* Walk through the list looking for the socket
			     * handler.
			     */

			    mx_status =
			        mxp_local_field_find_socket_handler_in_list(
				    callback_socket_handler_list,
				    socket_handler,
				    &callback_socket_handler_list_entry );

			    if ( mx_status.code == MXE_NOT_FOUND ) {

			    	/* This callback does not use the socket
				 * handler, so skip over it.
				 */

				continue;
			    } else
			    if ( mx_status.code == MXE_SUCCESS ) {

			    	/* The list entry for the socket handler was
				 * found and a pointer to it can be found in
				 * the callback_socket_handler_list_entry
				 * variable.
				 */

			    } else {
			    	/* Something went wrong when we looked for
				 * the socket handler, so print an error.
				 */

				(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				fname, "An unexpected error occurred while "
				"searching the socket handler list %p "
				"for callback %p used by record field '%s.%s'.",
					callback_socket_handler_list,
					callback_ptr,
					field->record->name,
					field->name );

				continue;    /* Skip this broken callback. */
			    }

#if NETWORK_DEBUG_CALLBACKS
			    MX_DEBUG(-2,
			    ("%s: MX_SOCKET_HANDLER %p _is_ on the "
			    "socket handler list %p for callback %p, "
			    "so we will remove it from the list.",
			    fname, socket_handler,
			    callback_socket_handler_list, callback_ptr));
#endif

#if NETWORK_DEBUG_MESSAGES
			    if ( socket_handler->network_debug_flags
					& MXF_NETDBG_VERBOSE)
			    {
				mxsrv_print_timestamp();

				fprintf( stderr,
				"MX (socket %d) deleting callback %#lx "
				"for newly disconnected client.\n",
			    (int) socket_handler->synchronous_socket->socket_fd,
				(unsigned long) callback_ptr->callback_id );
			    }
#endif
			    /* If we get here, then the socket handler _IS_
			     * on this callback's socket handler list.
			     */

			    /* Delete the MX_CALLBACK_SOCKET_HANDLER_INFO
			     * structure.
			     */

			    csh_info =
			callback_socket_handler_list_entry->list_entry_data;

			    mx_free( csh_info );

			    /* Delete the list entry from the callback's
			     * socket handler list.
			     */

			    mx_status = mx_list_delete_entry(
			    			callback_socket_handler_list,
					callback_socket_handler_list_entry );

			    mx_list_entry_destroy(
			    		callback_socket_handler_list_entry );

			    if ( mx_status.code != MXE_SUCCESS ) {
			    	/* Something unexpected happened when
				 * removing the list entry from the list.
				 */

				continue;    /* Skip this broken callback. */
			    }

#if NETWORK_DEBUG_CALLBACKS
			    MX_DEBUG(-2,
			    ("%s: #2 Callback %p, num_list_entries = %lu",
			    	fname, callback_ptr,
			      callback_socket_handler_list->num_list_entries));
#endif

			    /* Does callback_socket_handler_list still have
			     * any remaining socket handlers?
			     */

			    if ( callback_socket_handler_list->list_start
			    		!= NULL )
			    {
#if NETWORK_DEBUG_CALLBACKS
			    	MX_DEBUG(-2,
				("%s: callback socket handler list %p for "
				"record field '%s.%s' is NOT empty.",
				    fname, callback_socket_handler_list,
				    field->record->name, field->name));
#endif

				/* We are done with this callback, so skip
				 * on to the next one.
				 */

				continue;
			    }

			    /* Step F */

			    /**************************************************
			     * If we get here, the callback's socket handler  *
			     * list is empty.  If so, then we start to delete *
			     * all references to the callback.                *
			     * ************************************************/
#if NETWORK_DEBUG_CALLBACKS
			    MX_DEBUG(-2,
			    ("%s: callback_socket_handler_list %p is empty.",
			    	fname, callback_socket_handler_list));
#endif


			    /* Step F, part 1 */

			    /* Remove the callback from the server's
			     * callback handle table.
			     */

#if NETWORK_PROTECT_VM_HANDLE_TABLE
			    mx_callback_handle_table_change_permissions(
					callback_handle_table,
					R_OK | W_OK );
#endif

			    mx_status1 = mx_delete_handle( callback_handle,
			    				callback_handle_table );

#if NETWORK_PROTECT_VM_HANDLE_TABLE
			    mx_callback_handle_table_change_permissions(
					callback_handle_table, 0 );
#endif

			    /* Step F, part 2 */

			    /* Remove the callback from the record field's
			     * MX list of callbacks.
			     */

			    field_callback_list = field->callback_list;

			    mx_status2 = mx_list_find_list_entry(
			    			field_callback_list,
						callback_ptr,
						&field_callback_list_entry );

			    if ( mx_status2.code != MXE_SUCCESS ) {

			        if ( mx_status2.code == MXE_NOT_FOUND ) {
					/* Make sure that the MXE_NOT_FOUND
					 * message is seen.
					 */

			    		(void) mx_error( mx_status2.code,
						mx_status2.location,
						"%s", mx_status2.message );
				}
				continue;    /* Skip this broken callback. */
			    }

			    mx_status2 = mx_list_delete_entry(
			    			field_callback_list,
						field_callback_list_entry );

			    if ( mx_status2.code != MXE_SUCCESS ) {
				continue;    /* Skip this broken callback. */
			    }

			    mx_list_entry_destroy( field_callback_list_entry );

			    /* Step F, part 3 */

			    /* If either the deletion of the callback from
			     * the server's callback handle table failed
			     * or the deletion of the callback from the
			     * record field's callback list failed, then
			     * it is not safe to delete this callback.
			     *
			     * If so, then the only thing we can do is
			     * move on to the next callback, after generating
			     * an error message.
			     */

			    if ( (mx_status1.code != MXE_SUCCESS)
			      || (mx_status2.code != MXE_SUCCESS) )
			    {
			    	(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				fname, "Deletion of all references to "
				"callback %p for record field '%s.%s' failed.  "
				"Server callback handle deletion status = %lu, "
				"Record field callback list deletion status "
				"= %lu.", callback_ptr,
					field->record->name, field->name,
					mx_status1.code, mx_status2.code );

				continue;    /* Skip this broken callback. */
			    }

			    /* If we get here, then all references to the
			     * callback should have been deleted, so it is
			     * now safe to delete the callback structure
			     * itself.
			     */

			    mx_free( callback_ptr );

			    /* Step G */

			    /* Does the record field's callback list still
			     * have any entries in it?
			     */

			    if ( field_callback_list->list_start != NULL ) {

			    	/* The field's callback list still has
				 * entries in it, so we skip on to the
				 * next callback.
				 */

				continue;
			    }

			    /* The field's callback list does _not_ still
			     * have any entries in it, so delete the
			     * callback list itself.
			     */

			    mx_list_destroy( field_callback_list );

			    field->callback_list = NULL;

			    /* We are done with this callback, so proceed
			     * on to the next callback.
			     */
			}
		}
	}

	/* At this point, we have finished dealing with callbacks,
	 * so prepare to delete the socket handler itself.
	 */

	/* Close our end of the synchronous socket. */

	(void) mx_socket_close( socket_handler->synchronous_socket );

	if ( socket_handler_list != NULL ) {
		socket_handler_list->num_sockets_in_use--;
	}

#if 0
	mx_free( socket_handler->synchronous_socket );
#endif

	/* Free the message buffer. */

	if ( socket_handler->message_buffer != NULL ) {
		mx_free_network_buffer( socket_handler->message_buffer );
	}

	/* Invalidate the contents of the socket handler just in case
	 * someone has a pointer to it.
	 */

	socket_handler->synchronous_socket = NULL;
	socket_handler->handler_array_index = -1;
	socket_handler->event_handler = NULL;
	socket_handler->message_buffer = NULL;

	mx_free( socket_handler );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_mx_server_socket_init( MX_RECORD *list_head_record,
				MX_SOCKET_HANDLER_LIST *socket_handler_list,
				MX_EVENT_HANDLER *event_handler )
{
	static const char fname[] = "mxsrv_mx_server_socket_init()";

	MXSRV_MX_SERVER_SOCKET *server_socket_struct;
	MX_SOCKET_HANDLER *socket_handler;
	MX_SOCKET *server_socket;
	MX_LIST_HEAD *list_head;
	int i, socket_type, max_sockets, handler_array_size;
	mx_status_type mx_status;

	MX_DEBUG( 1,("%s invoked.", fname));

	if ( list_head_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The list_head_record pointer passed was NULL." );
	}
	if ( socket_handler_list == (MX_SOCKET_HANDLER_LIST *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET_HANDLER_LIST pointer passed was NULL." );
	}
	if ( event_handler == (MX_EVENT_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EVENT_HANDLER pointer passed was NULL." );
	}

	list_head = list_head_record->record_superclass_struct;

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the current database is NULL." );
	}

	max_sockets = socket_handler_list->max_sockets;
	handler_array_size = socket_handler_list->handler_array_size;

	/* Get the port number of the server socket we are supposed to open. */

	server_socket_struct = (MXSRV_MX_SERVER_SOCKET *)
				( event_handler->type_struct );

	if ( server_socket_struct == (MXSRV_MX_SERVER_SOCKET *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MXSRV_MX_SERVER_SOCKET pointer for server event handler '%s' is NULL.",
			event_handler->name );
	}

	/*********** Try to create the server socket. ************/

	socket_type = server_socket_struct->type;

	if ( socket_type == MXF_SRV_TCP_SERVER_TYPE ) {

		int port_number;

		port_number = server_socket_struct->u.tcp.port;

		if ( port_number < 0 ) {
			return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
		"No TCP/IP socket number was specified for the server." );
		}

		if ( port_number > 65535 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal server port number %d for TCP server event handler '%s'",
				port_number, event_handler->name );
		}

		mx_status = mx_tcp_socket_open_as_server(
						&server_socket, port_number, 0,
						MX_SOCKET_DEFAULT_BUFFER_SIZE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_info( "TCP server socket %d was opened.", port_number );

#if HAVE_UNIX_DOMAIN_SOCKETS
	} else if ( socket_type == MXF_SRV_UNIX_SERVER_TYPE ) {

		char *pathname;

		pathname = server_socket_struct->u.unix_domain.pathname;

		if ( strlen( pathname ) == 0 ) {
			return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
		"No Unix domain socket name was specified for the server." );
		}

		mx_status = mx_unix_socket_open_as_server(
						&server_socket, pathname, 0,
						MX_SOCKET_DEFAULT_BUFFER_SIZE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_info( "Unix domain server socket '%s' was opened.",
				pathname );
#endif
	} else if ( socket_type == MXF_SRV_ASCII_SERVER_TYPE ) {

		int port_number;

		port_number = server_socket_struct->u.ascii.port;

		if ( port_number < 0 ) {
			return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
		"No ASCII socket number was specified for the server." );
		}

		if ( port_number > 65535 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal server port number %d for ASCII server event handler '%s'",
				port_number, event_handler->name );
		}

		mx_status = mx_tcp_socket_open_as_server(
						&server_socket, port_number, 0,
						MX_SOCKET_DEFAULT_BUFFER_SIZE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_info( "ASCII server socket %d was opened.", port_number );

	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal MX server socket type %d requested.",
			socket_type );
	}

#ifndef OS_WIN32
	if ( server_socket->socket_fd < 0 ||
			server_socket->socket_fd >= max_sockets )
	{
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Server socket value %d returned by mx_socket_open_as_server() is outside "
"the allowed range of 0 to %d.", server_socket->socket_fd, max_sockets - 1 );
	}
#endif /* OS_WIN32 */

	/* Add the socket to the list of sockets that we handle. */

	for ( i = 0; i < handler_array_size; i++ ) {

		/* Look for an empty slot in the array. */

		if ( socket_handler_list->array[i] == NULL ) {
			break;                     /* Exit the for() loop. */
		}
	}

	if ( i >= handler_array_size ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Attempted to use more than the maximum number (%d) "
			"of socket handlers allowed.", handler_array_size );
	}

	socket_handler = (MX_SOCKET_HANDLER *)
				calloc( 1, sizeof(MX_SOCKET_HANDLER) );

	if ( socket_handler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate MX_SOCKET_HANDLER for socket %d.",
			server_socket->socket_fd );
	}

	socket_handler_list->array[i] = socket_handler;

	socket_handler->synchronous_socket = server_socket;
	socket_handler->list_head = list_head;
	socket_handler->handler_array_index = i;
	socket_handler->event_handler = event_handler;
	socket_handler->network_debug_flags = FALSE;

	strlcpy( socket_handler->client_address_string, "",
			sizeof(socket_handler->client_address_string ) );

	strlcpy( socket_handler->program_name, "mxserver",
			sizeof(socket_handler->program_name) );

	mx_username( socket_handler->username, MXU_USERNAME_LENGTH );

	socket_handler->process_id = mx_process_id();

	/* The server socket does not read anything from the socket,
	 * so it does not need a message buffer.
	 */

	socket_handler->message_buffer = NULL;

	socket_handler_list->num_sockets_in_use++;

	return MX_SUCCESSFUL_RESULT;
}

/* For some reason, inet_ntoa() on Irix does not always work,
 * so we provide our own copy here.
 */

#if defined(OS_IRIX)

#  define inet_ntoa(x)	my_inet_ntoa(x)

#  define MY_INET_NTOA_LENGTH	(12+3)

static char *
my_inet_ntoa( struct in_addr in )
{
	static char address[MY_INET_NTOA_LENGTH+1];
	unsigned long address_value;
	unsigned long nibble1, nibble2, nibble3, nibble4;

	address_value = in.s_addr;

	nibble4 = address_value % 256L;

	address_value /= 256L;

	nibble3 = address_value % 256L;

	address_value /= 256L;

	nibble2 = address_value % 256L;

	address_value /= 256L;

	nibble1 = address_value % 256L;

	snprintf( address, sizeof(address), "%lu.%lu.%lu.%lu",
				nibble1, nibble2, nibble3, nibble4 );

	return &(address[0]);
}

#endif  /* OS_IRIX */

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_mx_server_socket_process_event( MX_RECORD *record_list,
				MX_SOCKET_HANDLER *socket_handler,
                                MX_SOCKET_HANDLER_LIST *socket_handler_list,
				MX_EVENT_HANDLER *event_handler )
{
	static const char fname[] = "mxsrv_mx_server_socket_process_event()";

	MX_LIST_HEAD *list_head;
	MXSRV_MX_SERVER_SOCKET *server_socket_struct;
	MX_SOCKET_HANDLER *new_socket_handler;
	MX_SOCKET *client_socket;
	int i, socket_type, max_sockets, handler_array_size;
	int saved_errno;
	mx_bool_type connection_allowed;
	char *error_string;
	mx_status_type mx_status;

	void *client_address_ptr;
	char *client_address_string_ptr;

	struct sockaddr_in tcp_client_address;

	mx_socklen_t tcp_client_address_size = sizeof( struct sockaddr_in );
	mx_socklen_t client_address_size;

#if HAVE_UNIX_DOMAIN_SOCKETS
	struct sockaddr_un unix_client_address;

	mx_socklen_t unix_client_address_size = sizeof( struct sockaddr_un );
#endif

	MX_DEBUG( 1,("%s invoked.", fname));

	if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SOCKET_HANDLER pointer passed was NULL." );
	}
	if ( socket_handler_list == (MX_SOCKET_HANDLER_LIST *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_SOCKET_HANDLER_LIST pointer passed was NULL." );
	}
	if ( event_handler == (MX_EVENT_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_EVENT_HANDLER pointer passed was NULL." );
	}

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == NULL ) {
		mx_info( "Exiting due to corrupt list head record." );
		exit( MXE_CORRUPT_DATA_STRUCTURE );
	}

	max_sockets = socket_handler_list->max_sockets;
	handler_array_size = socket_handler_list->handler_array_size;

	if ( socket_handler_list->num_sockets_in_use >= (max_sockets - 1) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Maximum number (%d) of sockets allowed reached.  "
			"Cannot handle client request for a new connection.",
			max_sockets );
	}

	server_socket_struct = (MXSRV_MX_SERVER_SOCKET *)
					(event_handler->type_struct);

	if ( server_socket_struct == (MXSRV_MX_SERVER_SOCKET *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MXSRV_MX_SERVER_SOCKET pointer for event handler type '%s' is NULL.",
			event_handler->name );
	}

	if ( server_socket_struct->client_event_handler == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"client_event_handler pointer for event handler type '%s' is NULL.",
			event_handler->name );
	}

	socket_type = server_socket_struct->type;

	switch( socket_type ) {
	case MXF_SRV_TCP_SERVER_TYPE:
	case MXF_SRV_ASCII_SERVER_TYPE:
		client_address_ptr = &tcp_client_address;
		client_address_size = tcp_client_address_size;
		break;

#if HAVE_UNIX_DOMAIN_SOCKETS
	case MXF_SRV_UNIX_SERVER_TYPE:
		client_address_ptr = &unix_client_address;
		client_address_size = unix_client_address_size;
		break;
#endif
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The server_socket_struct passed says that is of illegal type %d.",
			socket_type );
	}

	/* Allocate memory for the client MX_SOCKET structure. */

	client_socket = (MX_SOCKET *) calloc( 1, sizeof(MX_SOCKET) );

	if ( client_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a client socket." );
	}

	/* The server socket should be ready for an accept() operation,
	 * so let's try it.
	 */

	client_socket->socket_fd = accept(
				socket_handler->synchronous_socket->socket_fd,
				( struct sockaddr *) client_address_ptr,
				&client_address_size );

        saved_errno = mx_socket_check_error_status( &(client_socket->socket_fd),
					MXF_SOCKCHK_INVALID, &error_string );

        if ( saved_errno != 0 ) {
		mx_free( client_socket );

                return mx_error( MXE_NETWORK_IO_ERROR, fname,
               "Attempt to perform accept() on MX server port failed.  "
                        "Errno = %d.  Error string = '%s'.",
                        saved_errno, error_string );
        }

	/* Allocate a socket handler for the new socket. */

	new_socket_handler = (MX_SOCKET_HANDLER *)
				calloc( 1, sizeof(MX_SOCKET_HANDLER) );

	if ( new_socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		mx_free( client_socket );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Attempt to allocate a new socket handler for new client socket %d failed.",
			client_socket->socket_fd );
	}

	new_socket_handler->list_head = list_head;

	new_socket_handler->network_debug_flags
			= list_head->network_debug_flags;

	new_socket_handler->handler_array_index = -1;

	new_socket_handler->username[0] = '\0';

	new_socket_handler->program_name[0] = '\0';

	new_socket_handler->process_id = (unsigned long) -1;

	new_socket_handler->data_format = list_head->default_data_format;

	new_socket_handler->use_64bit_network_longs = FALSE;

	new_socket_handler->remote_header_length = 0;

	new_socket_handler->remote_mx_version = 0;

	new_socket_handler->last_rpc_message_id = 0;

	new_socket_handler->authentication_type = MXF_SRVAUTH_NONE;

	/* Allocate memory for the message buffer. */

	mx_status = mx_allocate_network_buffer(
			&(new_socket_handler->message_buffer),
			MXU_NETWORK_INITIAL_MESSAGE_BUFFER_LENGTH );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( client_socket );
		mx_free( new_socket_handler );

		return mx_status;
	}

	new_socket_handler->message_buffer->data_format
					= new_socket_handler->data_format;

	new_socket_handler->synchronous_socket = client_socket;
	new_socket_handler->event_handler
			= server_socket_struct->client_event_handler;

	/* Perform initial checks on the new socket. */

	switch( socket_type ) {
	case MXF_SRV_TCP_SERVER_TYPE:
	case MXF_SRV_ASCII_SERVER_TYPE:

		/* Did accept() return a valid address? */


#if ( defined(OS_HPUX) && defined(__LP64__) )

		if ( tcp_client_address.sin_addr.s_addr == 0 ) {
			mx_warning(
		"accept() returned a host address of 0 for the new client "
		"socket.  This problem seems to be specific to 64-bit compiled "
		"code on HP-UX.\nAt present the only 'solutions' are:\n\n"
		"  1. Use the 32-bit compiler for your server (recommended).\n"
		"  2. Add 0.0.0.0 to your server ACL file (I really "
		"do _not_ recommend this).\n" );
		}

#endif

		/* Convert the host address of the socket to standard
		 * numbers-and-dots notation.
		 */

		client_address_string_ptr =
			inet_ntoa( tcp_client_address.sin_addr );

		strlcpy( new_socket_handler->client_address_string,
			client_address_string_ptr,
			MXU_ADDRESS_STRING_LENGTH );

		/* Check the connection access control list to see if the
		 * remote client is permitted to connect to us.
		 */

		if ( list_head->connection_acl == NULL ) {
			connection_allowed = TRUE;
		} else {
			mx_status =
		mx_check_socket_connection_acl_permissions( record_list,
				new_socket_handler->client_address_string,
				TRUE, &connection_allowed );

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) mxsrv_free_client_socket_handler(
						new_socket_handler, NULL );

				return mx_status;
			}
		}

		if ( connection_allowed == FALSE ) {
			(void) mxsrv_free_client_socket_handler(
						new_socket_handler, NULL );

			return mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
			"Network client '%s' is not allowed to connect to us.",
				client_address_string_ptr );
		}
		break;

#if HAVE_UNIX_DOMAIN_SOCKETS
	case MXF_SRV_UNIX_SERVER_TYPE:
		strlcpy( new_socket_handler->client_address_string,
				server_socket_struct->u.unix_domain.pathname,
				MXU_ADDRESS_STRING_LENGTH );

		/* MX Unix domain clients transmit a single null byte when
		 * when they connect.  User credentials are passed along with
		 * this byte for some versions of Unix.
		 */

		mx_status = mxsrv_get_unix_domain_socket_credentials(
				new_socket_handler );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mxsrv_free_client_socket_handler(
						new_socket_handler, NULL );

			return mx_status;
		}
		break;
#endif
	}

	/* Now that we seem to have a working client socket, let's put
	 * it into the socket handler list.
	 */

	for ( i = 0; i < handler_array_size; i++ ) {

		/* Look for an empty slot in the array. */

		if ( socket_handler_list->array[i] == NULL ) {
			break;                     /* Exit the for() loop. */
		}
	}

	if ( i >= handler_array_size ) {

		(void) mxsrv_free_client_socket_handler(
				new_socket_handler, NULL );

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"Attempted to use more than the maximum number (%d) "
			"of socket handlers allowed.", handler_array_size );
	}

	new_socket_handler->handler_array_index = i;

	socket_handler_list->array[i] = new_socket_handler;

	socket_handler_list->num_sockets_in_use++;

	/* Update the list of fds to check in select(). */

	mxsrv_update_select_fds( socket_handler_list );

	/* Announce that a new client has connected. */

	mx_info("Client %d (socket %d) connected from '%s'.",
		i, client_socket->socket_fd,
		new_socket_handler->client_address_string );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_mx_client_socket_init( MX_RECORD *record_list,
				MX_SOCKET_HANDLER_LIST *socket_handler_list,
				MX_EVENT_HANDLER *event_handler )
{
	/* Nothing to do here.  Client sockets are actually initialized
	 * in mxsrv_mx_server_socket_process_event().
	 */

	return MX_SUCCESSFUL_RESULT;
}

#define MXS_START_NOTHING		0
#define MXS_START_RECORD_FIELD_NAME	1
#define MXS_START_RECORD_FIELD_HANDLE	2

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_mx_client_socket_process_event( MX_RECORD *record_list,
				MX_SOCKET_HANDLER *socket_handler,
                                MX_SOCKET_HANDLER_LIST *socket_handler_list,
				MX_EVENT_HANDLER *event_handler )
{
	static const char fname[] = "mxsrv_mx_client_socket_process_event()";

	uint32_t *header;
	MX_NETWORK_MESSAGE_BUFFER *received_message;

	int update_next_event_time;
	int queue_a_message, value_at_message_start;
	char *record_name, *field_name;
	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_LIST_HEAD *list_head;
	MX_HANDLE_TABLE *handle_table;
	long record_handle, field_handle;
	MX_SOCKET *client_socket;
	void *record_ptr;

	char *ptr, *message_ptr, *value_ptr;
	uint32_t *uint32_message_body;
	int saved_errno;
	int bytes_left, bytes_received, initial_recv_length;
	uint32_t magic_value, header_length, message_length, total_length;
	uint32_t message_type, returned_message_type, message_id;
	mx_status_type mx_status, mx_status2;

	char separators[] = MX_RECORD_FIELD_SEPARATORS;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING recv_measurement, parse_measurement;
	MX_HRT_TIMING immediate_measurement, total_measurement;

	MX_HRT_START( total_measurement );
	MX_HRT_START( recv_measurement );
#endif

        if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
                return mx_error( MXE_NULL_ARGUMENT, fname,
                        "MX_SOCKET_HANDLER pointer passed was NULL." );
        }

	record = NULL;

	client_socket = socket_handler->synchronous_socket;

#if NETWORK_DEBUG_VERBOSE
	MX_DEBUG(-2,
	  ("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
	MX_DEBUG(-2,("%s invoked for socket handler %ld.", fname,
				socket_handler->handler_array_index));
	MX_DEBUG(-2,("socket_handler->synchronous_socket = %p",
				socket_handler->synchronous_socket));
	MX_DEBUG(-2,("socket_handler->handler_array_index = %ld",
				socket_handler->handler_array_index));
	MX_DEBUG(-2,("socket_handler->event_handler = %p",
				socket_handler->event_handler));
#endif

        if ( socket_handler_list == (MX_SOCKET_HANDLER_LIST *) NULL ) {
                return mx_error( MXE_NULL_ARGUMENT, fname,
                "MX_SOCKET_HANDLER_LIST pointer passed was NULL." );
        }
        if ( event_handler == (MX_EVENT_HANDLER *) NULL ) {
                return mx_error( MXE_NULL_ARGUMENT, fname,
                        "MX_EVENT_HANDLER pointer passed was NULL." );
        }

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_LIST_HEAD pointer for the MX database is NULL." );
	}

	/* Try to read the beginning of the header of the incoming message. */

	received_message = socket_handler->message_buffer;

	header = received_message->u.uint32_buffer;
	ptr    = received_message->u.char_buffer;

	initial_recv_length = 4 * sizeof( uint32_t );

	bytes_left = initial_recv_length;

	while( bytes_left > 0 ) {
		bytes_received = recv( client_socket->socket_fd,
						ptr, bytes_left, 0 );

		switch( bytes_received ) {
		case MX_SOCKET_ERROR:
			saved_errno = mx_socket_get_last_error();

			switch( saved_errno ) {
			case ECONNRESET:
			case ECONNABORTED:
				break;
			default:
				return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Error receiving header from remote host.  Errno = %d, error text = '%s'",
			saved_errno, mx_socket_strerror( saved_errno ) );
				break;
			}
			mx_status = mxsrv_free_client_socket_handler(
					socket_handler, socket_handler_list );

			return mx_status;
			break;
		case 0:
			/* The remote socket has closed so remove the
			 * socket handler from the socket handler list.
			 */

			mx_status = mxsrv_free_client_socket_handler(
					socket_handler, socket_handler_list );

			return mx_status;
			break;
		default:
			bytes_left -= bytes_received;
			ptr += bytes_received;
			break;
		}
	}

#if 0 && NETWORK_DEBUG_VERBOSE
	{
		long i;
		unsigned char c;

		for ( i = 0; i < initial_recv_length; i++ ) {
			c = (received_message->buffer)[i];

			if ( isprint(c) ) {
				MX_DEBUG(-2,("buffer[%ld] = %02x (%c)",
						i, c, c));
			} else {
				MX_DEBUG(-2,("buffer[%ld] = %02x", i, c));
			}
		}
	}
#endif

	magic_value    = mx_ntohl( header[ MX_NETWORK_MAGIC ] );
        header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
        message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );

#if NETWORK_DEBUG_VERBOSE
        MX_DEBUG(-2,("%s: magic_value    = %lx",
			fname, (unsigned long) magic_value));

        MX_DEBUG(-2,("%s: header_length  = %ld",
			fname, (unsigned long) header_length));

        MX_DEBUG(-2,("%s: message_length = %ld",
			fname, (unsigned long) message_length));

        MX_DEBUG(-2,("%s: message_type   = %#lx",
		fname, (unsigned long) message_type));
#endif

        if ( magic_value != MX_NETWORK_MAGIC_VALUE ) {
                return mx_error( MXE_NETWORK_IO_ERROR, fname,
                "Wrong magic number %lx in received message.",
		(unsigned long) magic_value );
        }

	/* Save a copy of the client's header length on the first pass
	 * through this function.
	 */

	if ( socket_handler->remote_header_length == 0 ) {
		socket_handler->remote_header_length = header_length;

#if NETWORK_DEBUG_HEADER_LENGTH
		MX_DEBUG(-2,("%s: client header length = %lu",
			fname, socket_handler->remote_header_length));
#endif
	}

	/* If the message is too long to fit into the current buffer,
	 * increase the size of the buffer.
	 */

	total_length = header_length + message_length;

	if ( total_length > received_message->buffer_length ) {

		MX_DEBUG( 2,
		("%s: Increasing buffer length from %lu to %lu", fname,
		  (unsigned long) received_message->buffer_length,
	   		(unsigned long) total_length ));

		mx_status = mx_reallocate_network_buffer(
					socket_handler->message_buffer,
					total_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Update local pointers. */

		received_message = socket_handler->message_buffer;

		header = received_message->u.uint32_buffer;
	}

        /* Receive the rest of the data. */

	ptr  = received_message->u.char_buffer;

	ptr += initial_recv_length;

        bytes_left = total_length - initial_recv_length;

        while( bytes_left > 0 ) {
                bytes_received = recv( client_socket->socket_fd,
						ptr, bytes_left, 0 );

                switch( bytes_received ) {
		case MX_SOCKET_ERROR:
			saved_errno = mx_socket_get_last_error();

			(void) mxsrv_free_client_socket_handler(
					socket_handler, socket_handler_list );

                        return mx_error( MXE_NETWORK_IO_ERROR, fname,
                        "Error receiving message body from remote host.  "
                        "Errno = %d, error text = '%s'",
			saved_errno, mx_socket_strerror( saved_errno ) );

                        break;
                case 0:
			(void) mxsrv_free_client_socket_handler(
					socket_handler, socket_handler_list );

                        return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"Network connection closed unexpectedly." );
                        break;
                default:
                        bytes_left -= bytes_received;
                        ptr += bytes_received;
                        break;
                }
        }

	message_ptr  = received_message->u.char_buffer;

	message_ptr += header_length;

	/* Ensure that a '\0' byte is placed after the end of the
	 * message field.  It is OK to write to this location since
	 * the network message buffer allocation and reallocation
	 * routines actually add one extra byte to the buffer.
	 * This is to make sure that string fields are always
	 * null terminated even if the driver did not allocate
	 * space for the null byte.
	 */

	message_ptr[ message_length ] = '\0';

	if ( mx_client_supports_message_ids(socket_handler) == FALSE ) {

		message_id = 0;     /* This client is using MX 1.4 or before. */
	} else {
		message_id = mx_ntohl( header[ MX_NETWORK_MESSAGE_ID ] );
	}

	socket_handler->last_rpc_message_id = message_id;

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE) {
		fprintf( stderr, "\nMX NET: CLIENT (socket %d) -> SERVER\n",
				(int) client_socket->socket_fd );

		mx_network_display_message( received_message, NULL,
				socket_handler->use_64bit_network_longs );
	}
#endif

#if NETWORK_DEBUG
	{
		char message_type_string[40];
		char text_buffer[200];
		size_t buffer_left;

		switch( message_type ) {
		case MX_NETMSG_GET_ARRAY_BY_NAME:
			strlcpy( message_type_string, "GET_ARRAY_BY_NAME",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_PUT_ARRAY_BY_NAME:
			strlcpy( message_type_string, "PUT_ARRAY_BY_NAME",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_GET_ARRAY_BY_HANDLE:
			strlcpy( message_type_string, "GET_ARRAY_BY_HANDLE",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_PUT_ARRAY_BY_HANDLE:
			strlcpy( message_type_string, "PUT_ARRAY_BY_HANDLE",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_GET_NETWORK_HANDLE:
			strlcpy( message_type_string, "GET_NETWORK_HANDLE",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_GET_FIELD_TYPE:
			strlcpy( message_type_string, "GET_FIELD_TYPE",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_GET_ATTRIBUTE:
			strlcpy( message_type_string, "GET_ATTRIBUTE",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_SET_ATTRIBUTE:
			strlcpy( message_type_string, "SET_ATTRIBUTE",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_SET_CLIENT_INFO:
			strlcpy( message_type_string, "SET_CLIENT_INFO",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_GET_OPTION:
			strlcpy( message_type_string, "GET_OPTION",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_SET_OPTION:
			strlcpy( message_type_string, "SET_OPTION",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_ADD_CALLBACK:
			strlcpy( message_type_string, "ADD_CALLBACK",
						sizeof(message_type_string) );
			break;
		case MX_NETMSG_DELETE_CALLBACK:
			strlcpy( message_type_string, "DELETE_CALLBACK",
						sizeof(message_type_string) );
			break;
		default:
			snprintf( message_type_string,
				sizeof(message_type_string),
				"Unknown message type %#lx",
				(unsigned long) message_type );
		}

		buffer_left = sizeof(text_buffer) - 6;
#if 1
		snprintf( text_buffer, buffer_left, "%s: (%s)",
			fname, message_type_string );
#else
		snprintf( text_buffer, buffer_left, "%s: (%s) message = '",
			fname, message_type_string );

		buffer_left = sizeof(text_buffer) - strlen(text_buffer) - 6;

		strlcat( text_buffer, message, buffer_left );

		strlcat( text_buffer, "'", buffer_left );
#endif

		MX_DEBUG( 2,("%s", text_buffer));
	}
#endif

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( recv_measurement );

	MX_HRT_START( parse_measurement );
#endif

	/* Set some flags depending on the message type. */

	switch( message_type ) {
	case MX_NETMSG_GET_ARRAY_BY_NAME:
	case MX_NETMSG_PUT_ARRAY_BY_NAME:
	case MX_NETMSG_GET_NETWORK_HANDLE:
	case MX_NETMSG_GET_FIELD_TYPE:
		value_at_message_start = MXS_START_RECORD_FIELD_NAME;
		break;

	case MX_NETMSG_GET_ARRAY_BY_HANDLE:
	case MX_NETMSG_PUT_ARRAY_BY_HANDLE:
	case MX_NETMSG_GET_ATTRIBUTE:
	case MX_NETMSG_SET_ATTRIBUTE:
	case MX_NETMSG_ADD_CALLBACK:
		value_at_message_start = MXS_START_RECORD_FIELD_HANDLE;
		break;

	case MX_NETMSG_SET_CLIENT_INFO:
	case MX_NETMSG_GET_OPTION:
	case MX_NETMSG_SET_OPTION:
	case MX_NETMSG_DELETE_CALLBACK:
		value_at_message_start = MXS_START_NOTHING;
		break;
	default:
		mx_status = mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"MX message type %#lx is not yet implemented.",
			(unsigned long) message_type );

		(void) mx_network_socket_send_error_message(
					client_socket,
					message_id, 
					socket_handler->remote_header_length,
					socket_handler->network_debug_flags,
					MX_NETMSG_UNEXPECTED_ERROR,
					mx_status );

		return mx_status;
	}

	/* If this message type requires it, we parse the beginning of the
	 * message to look for a record name and a field name.  If found,
	 * we then get the corresponding MX_RECORD and MX_RECORD_FIELD
	 * pointers.
	 */

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( value_at_message_start ) {
	case MXS_START_NOTHING:
		record = NULL;
		record_field = NULL;
		break;

	case MXS_START_RECORD_FIELD_NAME:
		/* The do...while(0) loop below is just a trick to make
		 * it easy to jump to the end of this block of code.
		 */

		do {
			/* Which record and field does this correspond to? */

			record_name = message_ptr;

			ptr = strchr( message_ptr, '.' );

			if ( ptr == NULL ) {
				mx_status = mx_error(
				MXE_ILLEGAL_ARGUMENT, fname,
				"No period found in record field name '%s'.",
					message_ptr );

				break;	/* Exit the do...while(0) loop. */
			}

			*ptr = '\0';

			field_name = ++ptr;

		        ptr = field_name + strcspn( field_name, separators );

		        *ptr = '\0';

			record = mx_get_record( record_list, record_name );

			if ( record == (MX_RECORD *) NULL ) {
				mx_status = mx_error( MXE_NOT_FOUND, fname,
				"Server '%s' has no record named '%s'.",
					list_head->hostname, record_name );

				break;	/* Exit the do...while(0) loop. */
			}

			mx_status = mx_find_record_field( record, field_name,
							&record_field );
		} while (0);

		break;

	case MXS_START_RECORD_FIELD_HANDLE:
		handle_table = (MX_HANDLE_TABLE *) list_head->handle_table;

		if ( handle_table == (MX_HANDLE_TABLE *) NULL ) {
			mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The 'handle_table' pointer for the MX database is NULL." );

			break;	/* Exit the switch() statement. */
		}

		uint32_message_body = header +
		  (mx_remote_header_length(socket_handler) / sizeof(uint32_t));

		record_handle = (long) mx_ntohl( uint32_message_body[0] );

		field_handle = (long) mx_ntohl( uint32_message_body[1] );

#if NETWORK_DEBUG_HANDLES
		MX_DEBUG(-2,("%s: requested network field (%ld,%ld)",
			fname, record_handle, field_handle));
#endif

		if ( (record_handle < 0) || (field_handle < 0) ) {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal network field handle (%ld,%ld) requested.  "
			"The two values in a network field handle must both "
			"be non-negative.", record_handle, field_handle );

			break;	/* Exit the switch() statement. */
		}

		/* First find the record pointer. */

		mx_status = mx_get_pointer_from_handle( &record_ptr,
						handle_table, record_handle );

		if ( mx_status.code != MXE_SUCCESS ) {

			break;	/* Exit the switch() statement. */
		}

		record = (MX_RECORD *) record_ptr;

		if ( record == (MX_RECORD *) NULL ) {
			mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer returned for network field "
			"(%ld,%ld) is NULL.", record_handle, field_handle );

			break;	/* Exit the switch() statement. */
		}

		/* Then find the field pointer. */

		if ( field_handle >= record->num_record_fields ) {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested field handle %ld for network field "
			"(%ld,%ld) is greater than or equal to the "
			"number of record fields %ld for record '%s'.",
				field_handle, record_handle, field_handle,
				record->num_record_fields, record->name );

			break;	/* Exit the switch() statement. */
		}

		record_field = &(record->record_field_array[ field_handle ]);

		if ( record_field == (MX_RECORD_FIELD *) NULL ) {

			mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD_FIELD pointer returned for "
			"network field (%ld,%ld) is NULL.",
				record_handle, field_handle );

			break;	/* Exit the switch() statement. */
		}

#if NETWORK_DEBUG_HANDLES
		MX_DEBUG(-2,
	("%s: network field handle = (%ld,%ld), record = '%s', field = '%s'",
	 		fname, record_handle, field_handle,
			record->name, record_field->name ));
#endif

		break;

	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal 'value_at_message_start' value (%d).  "
		"This should not be able to happen.", value_at_message_start );

		break;
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		returned_message_type = mxsrv_get_returning_message_type(
						message_type );

		/* Send back the error message. */

		(void) mx_network_socket_send_error_message(
					client_socket,
					message_id, 
					socket_handler->remote_header_length,
					socket_handler->network_debug_flags,
					MX_NETMSG_UNEXPECTED_ERROR,
					mx_status );

		return mx_status;
	}

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( parse_measurement );
#endif

	/* Do we finish handling this event now or do we queue an event
	 * to be processed later?
	 */

	queue_a_message = FALSE;

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( immediate_measurement );
#endif

	/* Here we handle messages that are to be dealt with immediately. */

	update_next_event_time = FALSE;

	switch ( message_type ) {
	case MX_NETMSG_GET_ARRAY_BY_NAME:
	case MX_NETMSG_GET_ARRAY_BY_HANDLE:
		mx_status = mxsrv_handle_get_array( record_list, socket_handler,
						record, record_field,
						received_message );

		update_next_event_time = TRUE;
		break;
	case MX_NETMSG_PUT_ARRAY_BY_NAME:
	case MX_NETMSG_PUT_ARRAY_BY_HANDLE:
		value_ptr  = received_message->u.char_buffer;

		value_ptr += header_length;

		if ( message_type == MX_NETMSG_PUT_ARRAY_BY_HANDLE ) {

			value_ptr += ( 2 * sizeof( uint32_t ) );
		} else {
			if ( socket_handler->remote_mx_version < 1005005L ) {
				value_ptr += 49;
			} else {
				value_ptr += MXU_RECORD_FIELD_NAME_LENGTH;
			}
		}

		mx_status = mxsrv_handle_put_array( record_list, socket_handler,
						record, record_field,
						received_message, value_ptr );

		update_next_event_time = TRUE;
		break;
	case MX_NETMSG_GET_NETWORK_HANDLE:
		mx_status = mxsrv_handle_get_network_handle( record_list,
						socket_handler,
						record, record_field );
		break;
	case MX_NETMSG_GET_FIELD_TYPE:
		mx_status = mxsrv_handle_get_field_type( record_list,
						socket_handler, client_socket,
						record_field, received_message);
		break;
	case MX_NETMSG_GET_ATTRIBUTE:
		mx_status = mxsrv_handle_get_attribute( record_list,
						socket_handler,
						record, record_field,
						received_message );
		break;
	case MX_NETMSG_SET_ATTRIBUTE:
		mx_status = mxsrv_handle_set_attribute( record_list,
						socket_handler,
						record, record_field,
						received_message );
		break;
	case MX_NETMSG_SET_CLIENT_INFO:
		mx_status = mxsrv_handle_set_client_info( record_list,
						socket_handler,
						received_message );
		break;
	case MX_NETMSG_GET_OPTION:
		mx_status = mxsrv_handle_get_option( record_list,
						socket_handler,
						received_message );
		break;
	case MX_NETMSG_SET_OPTION:
		mx_status = mxsrv_handle_set_option( record_list,
						socket_handler,
						received_message );
		break;
	case MX_NETMSG_ADD_CALLBACK:
		mx_status = mxsrv_handle_add_callback( record_list,
						socket_handler,
						record, record_field,
						received_message );
		break;
	case MX_NETMSG_DELETE_CALLBACK:
		mx_status = mxsrv_handle_delete_callback( record_list,
						socket_handler,
						received_message );
		break;
	default:
		mx_status = mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"MX message type %#lx is not yet implemented!",
			(unsigned long) message_type );

		(void) mx_network_socket_send_error_message(
					client_socket,
					message_id, 
					socket_handler->remote_header_length,
					socket_handler->network_debug_flags,
					MX_NETMSG_UNEXPECTED_ERROR,
					mx_status );
		break;
	}

#if NETWORK_DEBUG_VERBOSE
	MX_DEBUG(-2,("socket_handler->synchronous_socket = %p",
				socket_handler->synchronous_socket));
	MX_DEBUG(-2,("socket_handler->handler_array_index = %ld",
				socket_handler->handler_array_index));
	MX_DEBUG(-2,("socket_handler->event_handler = %p",
				socket_handler->event_handler));
#endif

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( immediate_measurement );
	MX_HRT_END( total_measurement );

	if ( ( record != NULL ) && ( record_field != NULL ) ) {
		MX_HRT_RESULTS( recv_measurement, fname,
			"receiving for '%s.%s'",
			record->name, record_field->name );

		MX_HRT_RESULTS( parse_measurement, fname,
			"parsing for '%s.%s'",
			record->name, record_field->name );

		MX_HRT_RESULTS( immediate_measurement, fname,
			"immediate processing for '%s.%s'",
			record->name, record_field->name );

		MX_HRT_RESULTS( total_measurement, fname,
			"total processing for '%s.%s'",
			record->name, record_field->name );
	} else {
		MX_HRT_RESULTS( recv_measurement, fname, "receiving" );
		MX_HRT_RESULTS( parse_measurement, fname, "parsing" );
		MX_HRT_RESULTS( immediate_measurement, fname, "immediate" );
		MX_HRT_RESULTS( total_measurement, fname, "total" );
	}
#endif

	/* If an event time manager is in use, compute the time
	 * of the next allowed event.
	 */

	if ( (record == (MX_RECORD *) NULL)
	  || ( record->event_time_manager == (MX_EVENT_TIME_MANAGER *) NULL ) )
	{
		update_next_event_time = FALSE;
	}

	if ( update_next_event_time ) {

#if NETWORK_DEBUG_VERBOSE
		MX_DEBUG(-2,
		("%s: updating next_allowed_event_time for '%s'",
		 	fname, record->name));
#endif

		mx_status2 = mx_update_next_allowed_event_time( record,
								record_field );

		if ( mx_status2.code != MXE_SUCCESS )
			return mx_status2;
	}

#if NETWORK_DEBUG_VERBOSE
	MX_DEBUG(-2,("%s exiting.", fname));
	MX_DEBUG(-2,
	  ("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"));
#endif
	MXW_UNUSED( queue_a_message );
	MXW_UNUSED( returned_message_type );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_mx_client_socket_proc_queued_event( MX_RECORD *record_list,
					MX_QUEUED_EVENT *queued_event )
{
	static const char fname[] =
			"mxsrv_mx_client_socket_proc_queued_event()";

	MX_NETWORK_MESSAGE_BUFFER *queued_message;
	MX_SOCKET_HANDLER *socket_handler;
	char *value_ptr;
	uint32_t *header;
	uint32_t message_type, header_length;
	mx_status_type mx_status;

	if ( queued_event == (MX_QUEUED_EVENT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_QUEUED_EVENT structure pointer passed is NULL." );
	}

	socket_handler = queued_event->socket_handler;

#if NETWORK_DEBUG_VERBOSE
	MX_DEBUG(-2,
	  ("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"));
	MX_DEBUG(-2,("%s invoked.", fname));
	MX_DEBUG(-2,("socket_handler = %p", socket_handler));
	MX_DEBUG(-2,("socket_handler->synchronous_socket = %p",
				socket_handler->synchronous_socket));
	MX_DEBUG(-2,("socket_handler->handler_array_index = %ld",
				socket_handler->handler_array_index));
	MX_DEBUG(-2,("socket_handler->event_handler = %p",
				socket_handler->event_handler));
#endif
	if ( queued_event->event_type != MXQ_NETWORK_MESSAGE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Queued event type %ld is not supported by this function.",
			queued_event->event_type );
	}

	value_ptr = NULL;

	queued_message = 
		(MX_NETWORK_MESSAGE_BUFFER *) queued_event->event_data;

	header = queued_message->u.uint32_buffer;

	message_type = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );

	/* Handle the request. */

#if NETWORK_DEBUG_VERBOSE
	MX_DEBUG( 2,
	("%s: queued_event->record = %p, queued_event->record_field = %p",
		fname, queued_event->record, queued_event->record_field));

	if ( (queued_event->record != NULL)
	  && (queued_event->record_field != NULL ) )
	{
		MX_DEBUG(-2,("%s: record field = '%s.%s", fname,
		queued_event->record->name, queued_event->record_field->name));
	}

	MX_DEBUG(-2,("%s: message_type = %#lx",
			fname, (unsigned long) message_type));
#endif

	mx_status = MX_SUCCESSFUL_RESULT;

	switch ( message_type ) {
	case MX_NETMSG_GET_ARRAY_BY_NAME:
	case MX_NETMSG_GET_ARRAY_BY_HANDLE:
		mx_status = mxsrv_handle_get_array( record_list,
						socket_handler,
						queued_event->record,
						queued_event->record_field,
						queued_event->event_data );

		break;
	case MX_NETMSG_PUT_ARRAY_BY_NAME:
	case MX_NETMSG_PUT_ARRAY_BY_HANDLE:
		header_length = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );

		value_ptr  = queued_message->u.char_buffer;

		value_ptr += header_length;

		if ( message_type == MX_NETMSG_PUT_ARRAY_BY_HANDLE ) {

			value_ptr += ( 2 * sizeof( uint32_t ) );
		} else {
			value_ptr += MXU_RECORD_FIELD_NAME_LENGTH;
		}

		mx_status = mxsrv_handle_put_array( record_list,
						socket_handler,
						queued_event->record,
						queued_event->record_field,
						queued_message,
						value_ptr );

		break;
	default:
		mx_status = mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Illegal client message type %#lx",
			(long) message_type );
	}

#if NETWORK_DEBUG_VERBOSE
	MX_DEBUG(-2,("socket_handler->synchronous_socket = %p",
				socket_handler->synchronous_socket));
	MX_DEBUG(-2,("socket_handler->handler_array_index = %ld",
				socket_handler->handler_array_index));
	MX_DEBUG(-2,("socket_handler->event_handler = %p",
				socket_handler->event_handler));

	MX_DEBUG(-2,("%s exiting.", fname));
	MX_DEBUG(-2,
	  ("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"));
#endif

	return mx_status;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_get_array( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_get_array()";

	uint32_t *receive_buffer_header;
	uint32_t receive_buffer_header_length;
	uint32_t receive_buffer_message_type = 0;
	uint32_t receive_buffer_message_id = 0;
	uint32_t send_buffer_message_type = 0;
	long receive_datatype;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	/* If the socket handler pointer is NULL, then we have no way
	 * to return a response to the (hypothetical?) client.
	 */

	if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"the MX_SOCKET_HANDLER pointer passed was NULL." );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	/* The do...while(0) loop below is just a trick to make it easy
	 * to jump to the end of this block of code, since we need to send
	 * a message to the client even if an error occurred.
	 */

	do {
		if ( record == (MX_RECORD *) NULL ) {
			mx_status = mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );

			break;		/* Exit the do...while(0) loop. */
		}
		if ( record_field == (MX_RECORD_FIELD *) NULL ) {
			mx_status = mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD_FIELD pointer passed was NULL." );

			break;		/* Exit the do...while(0) loop. */
		}
		if ( network_message == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
			mx_status = mx_error( MXE_NULL_ARGUMENT, fname,
		    "The MX_NETWORK_MESSAGE_BUFFER pointer passed was NULL." );

			break;		/* Exit the do...while(0) loop. */
		}

		/* Save the message type and message id for later. */

		receive_buffer_header = network_message->u.uint32_buffer;

		receive_buffer_header_length =
		   mx_ntohl(receive_buffer_header[ MX_NETWORK_HEADER_LENGTH ]);

		receive_buffer_message_type =
		   mx_ntohl(receive_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]);

		if ( mx_client_supports_message_ids(socket_handler) == FALSE ) {
			/* Handle clients from MX 1.4 or before. */

			receive_datatype = record_field->datatype;
			receive_buffer_message_id = 0;
		} else {
			receive_datatype = (long)
		   mx_ntohl(receive_buffer_header[ MX_NETWORK_DATA_TYPE ]);

			receive_buffer_message_id =
		   mx_ntohl(receive_buffer_header[ MX_NETWORK_MESSAGE_ID ]);
		}

		/* Bug compatibility for old MX clients. */

		if ( ( mx_client_supports_message_ids(socket_handler) == FALSE )
	  && ( receive_buffer_message_type == MX_NETMSG_GET_ARRAY_BY_HANDLE ) )
		{
			/* MX clients prior to MX 1.5 expected the wrong server
			 * response type for 'get array by handle' messages.
			 */

			send_buffer_message_type =
				mx_server_response(MX_NETMSG_GET_ARRAY_BY_NAME);
		} else {
			send_buffer_message_type =
			    mx_server_response( receive_buffer_message_type );
		}

		/* Check the record field's permissions. */

		if ( record_field->flags & MXFF_NO_ACCESS ) {
			mx_status = mx_error( MXE_PERMISSION_DENIED, fname,
			"MX record field '%s.%s' can not be accessed by "
			"an MX client program.",
				record->name, record_field->name );
		}

		/* Get the data from the hardware for this get_array request. */

#if 1
		mx_numbered_breakpoint( 0 );
#endif

		mx_status = mx_process_record_field( record, record_field,
							MX_PROCESS_GET, NULL );

	} while (0);

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_status = mx_network_socket_send_error_message(
					socket_handler->synchronous_socket,
					receive_buffer_message_id,
					socket_handler->remote_header_length,
					socket_handler->network_debug_flags,
					MX_NETMSG_UNEXPECTED_ERROR,
					mx_status );
	} else {
		mx_status = mxsrv_send_field_value_to_client(
					socket_handler,
					record, record_field,
					network_message,
					send_buffer_message_type,
					receive_buffer_message_id );
	}

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"final for '%s.%s'", record->name, record_field->name );
#endif
	MXW_UNUSED( receive_datatype );
	MXW_UNUSED( receive_buffer_header_length );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_send_field_value_to_client( 
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *network_message,
			uint32_t message_type_for_client,
			uint32_t message_id_for_client )
{
	static const char fname[] = "mxsrv_send_field_value_to_client()";

	char location[ sizeof(fname) + 40 ];
	uint32_t *send_buffer_header;
	char *send_buffer_message;
	long send_buffer_header_length, send_buffer_message_length;
	long send_buffer_message_actual_length;
	size_t num_network_bytes;

	MX_SOCKET *mx_socket;
	void *pointer_to_value;
	int array_is_dynamically_allocated;
	unsigned long data_format;
	mx_status_type ( *token_constructor )
		(void *, char *, size_t, MX_RECORD *, MX_RECORD_FIELD *);
	mx_status_type mx_status;

#if 0
	if ( record_field->datatype == MXFT_RECORD ) {
		mx_breakpoint();
	}
#endif

	mx_status = MX_SUCCESSFUL_RESULT;

	MX_DEBUG( 2,("%s: socket_handler = %p, network_message = %p",
			fname, socket_handler, network_message ));

	if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET_HANDLER pointer passed was NULL." );
	}
	if ( network_message == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_MESSAGE_BUFFER pointer passed was NULL." );
	}

	mx_socket = socket_handler->synchronous_socket;

	MX_DEBUG( 1,("***** %s invoked for socket %d *****",
					fname, mx_socket->socket_fd));

	if ( record_field->flags & MXFF_VARARGS ) {
		array_is_dynamically_allocated = TRUE;
	} else {
		array_is_dynamically_allocated = FALSE;
	}

	/* Get a pointer to the data to be returned to the remote client. */

	pointer_to_value = mx_get_field_value_pointer( record_field );

	/* Construct the response header. */

	send_buffer_header_length =
		(long) mx_remote_header_length(socket_handler);

	send_buffer_message_length = (long)
		( network_message->buffer_length - send_buffer_header_length );

	send_buffer_message_actual_length = 0;

	send_buffer_header = network_message->u.uint32_buffer;;

	send_buffer_message  = network_message->u.char_buffer;
	send_buffer_message += send_buffer_header_length;

	/* What data format do we use to send the response? */

	data_format = socket_handler->data_format;

	if ( mx_status.code == MXE_SUCCESS ) {

	    int i, max_attempts;

	    /* Loop until the output buffer is large enough for the data
	     * that we want to send or until some other error occurs.
	     */

	    max_attempts = 10;

	    for ( i = 0; i < max_attempts; i++ ) {

		size_t current_length, new_length;

		switch( data_format ) {
		case MX_NETWORK_DATAFMT_ASCII:

		        /* Use ASCII MX database format. */

		        mx_status = mx_get_token_constructor(
				record_field->datatype, &token_constructor );

			if ( mx_status.code == MXE_SUCCESS ) {
				strlcpy( send_buffer_message, "",
					send_buffer_message_length );

			        if ( (record_field->num_dimensions == 0)
			          || ((record_field->datatype == MXFT_STRING)
			             && (record_field->num_dimensions == 1)) ) {

			                mx_status = (*token_constructor)(
						pointer_to_value,
						send_buffer_message,
						send_buffer_message_length,
						record, record_field );
				} else {
					mx_status = mx_create_array_description(
						pointer_to_value,
						record_field->num_dimensions-1,
						send_buffer_message,
						send_buffer_message_length,
						record, record_field,
						token_constructor );
				}
		        }

			send_buffer_message_actual_length
				= (long) strlen( send_buffer_message ) + 1;

			/* ASCII data transfers do not currently support
			 * dynamically resizing network buffers, so we return
			 * instead if we get an MXE_WOULD_EXCEED_LIMIT status
			 * code.
			 */

			num_network_bytes = 0;

			if ( mx_status.code == MXE_WOULD_EXCEED_LIMIT )
				return mx_status;
			break;

		case MX_NETWORK_DATAFMT_RAW:

			mx_status = mx_copy_array_to_buffer(
					pointer_to_value,
					array_is_dynamically_allocated,
					record_field->datatype,
					record_field->num_dimensions,
					record_field->dimension,
					record_field->data_element_size,
					send_buffer_message,
					send_buffer_message_length,
					&num_network_bytes,
				    socket_handler->use_64bit_network_longs );

			send_buffer_message_actual_length
				= (long) num_network_bytes;
			break;

		case MX_NETWORK_DATAFMT_XDR:
#if HAVE_XDR
			mx_status = mx_xdr_data_transfer(
					MX_XDR_ENCODE,
					pointer_to_value,
					array_is_dynamically_allocated,
					record_field->datatype,
					record_field->num_dimensions,
					record_field->dimension,
					record_field->data_element_size,
					send_buffer_message,
					send_buffer_message_length,
					&num_network_bytes );

			send_buffer_message_actual_length
				= (long) num_network_bytes;
#else
			mx_status = mx_error( MXE_UNSUPPORTED, fname,
				"XDR network data format is not supported "
				"on this system." );
#endif
			break;

		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "Unrecognized network data format type %lu was requested.",
		    		data_format );
		}

		/* If we succeeded or if some error other than
		 * MXE_WOULD_EXCEED_LIMIT occurred, break out
		 * of the for(i) loop.
		 */

		if ( mx_status.code != MXE_WOULD_EXCEED_LIMIT ){
			break;
		}

		/* The data does not fit into our existing buffer, so we must
		 * try to make the buffer larger.
		 *
		 * NOTE: In this situation, the variable 'num_network_bytes'
		 * actually tells you how many bytes would not fit in the
		 * existing buffer.
		 */

		current_length = network_message->buffer_length;

		new_length = current_length + num_network_bytes;

		mx_status = mx_reallocate_network_buffer(
						network_message, new_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Update some values. */

		send_buffer_header = network_message->u.uint32_buffer;;

		send_buffer_message = network_message->u.char_buffer
						+ send_buffer_header_length;

		send_buffer_message_length += num_network_bytes;
	    }

	    if ( i >= max_attempts ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"%d attempts to increase the network buffer size "
			"for record field '%s.%s' failed.  "
			"You should never see this error.",
			    max_attempts, record->name, record_field->name );
	    }
	}

	/* Make sure these pointers are up to date. */

	send_buffer_header = network_message->u.uint32_buffer;;

	send_buffer_message  = network_message->u.char_buffer;
	send_buffer_message += send_buffer_header_length;

	/* Fill in the header of the message to be sent back to the client. */

	send_buffer_header[ MX_NETWORK_STATUS_CODE ] = mx_htonl(mx_status.code);

        if ( mx_status.code != MXE_SUCCESS ) {
		*send_buffer_message = '\0';

		strlcat( send_buffer_message,
			mx_status.message, send_buffer_message_length );

		send_buffer_message_actual_length
			= (long) strlen( send_buffer_message ) + 1;
	}

	send_buffer_header[ MX_NETWORK_MAGIC ]
				= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
		= mx_htonl( message_type_for_client );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
				= mx_htonl( send_buffer_header_length );

	send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
				= mx_htonl( send_buffer_message_actual_length );

	if ( mx_client_supports_message_ids(socket_handler) ) {

		send_buffer_header[ MX_NETWORK_DATA_TYPE ]
				= mx_htonl( record_field->datatype );

		send_buffer_header[ MX_NETWORK_MESSAGE_ID ]
				= mx_htonl( message_id_for_client );
	}

	/* Send the string description back to the client. */

#if NETWORK_DEBUG
	{
		char text_buffer[200];
		char *ptr;
		long i, length, bytes_left;
		long max_length = 20;

		switch( data_format ) {
		case MX_NETWORK_DATAFMT_ASCII:
			snprintf( text_buffer, sizeof(text_buffer),
				"%s: sending response = '", fname );

			strlcat( text_buffer, send_buffer_message,
						sizeof(text_buffer) );

			strlcat( text_buffer, "'", sizeof(text_buffer) );
			break;

		case MX_NETWORK_DATAFMT_RAW:
			snprintf( text_buffer, sizeof(text_buffer),
				"%s: sending response = ", fname );

			ptr = text_buffer + strlen( text_buffer );

			length = send_buffer_message_length;

			if ( length > max_length )
				length = max_length;

			for ( i = 0; i < length; i++ ) {
				bytes_left = sizeof(text_buffer)
						- strlen(text_buffer) - 1;

				snprintf( ptr, bytes_left,
					"%#02x ", send_buffer_message[i] );

				ptr += 5;
			}
			break;

		default:
			snprintf( text_buffer, sizeof(text_buffer),
				"%s: sending response in data format %lu.",
				fname, data_format );
			break;
		}

		MX_DEBUG(-2,("%s", text_buffer));
	}
#endif

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
				(int) mx_socket->socket_fd );

		mx_network_display_message( network_message, record_field,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {

		mxsrv_print_timestamp();

		if ( message_id_for_client & MX_NETWORK_MESSAGE_IS_CALLBACK ) {
		    fprintf( stderr,
			"MX (socket %d) VC_CALLBACK('%s.%s') = ",
			(int) socket_handler->synchronous_socket->socket_fd,
			record->name,
			record_field->name );
		} else {
		    fprintf( stderr,
			"MX (socket %d) GET_ARRAY('%s.%s') = ",
			(int) socket_handler->synchronous_socket->socket_fd,
			record->name,
			record_field->name );
		}

		mx_network_buffer_show_value(
				send_buffer_message,
				socket_handler->data_format,
				record_field->datatype,
				message_type_for_client,
				send_buffer_message_actual_length,
				socket_handler->use_64bit_network_longs );

		fprintf( stderr, "\n" );
	}
#endif

	mx_status = mx_network_socket_send_message( mx_socket,
						-1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		if ( record != NULL ) {
			snprintf( location, sizeof(location),
			"%s to client socket %d for record field '%s.%s'",
					fname, mx_socket->socket_fd,
					record->name, record_field->name );
		} else {
			snprintf( location, sizeof(location),
			"%s to client socket %d for field '%s'",
					fname, mx_socket->socket_fd,
					record_field->name );
		}

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_put_array( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *network_message,
			void *value_buffer_ptr )
{
	static const char fname[] = "mxsrv_handle_put_array()";

	char location[ sizeof(fname) + 40 ];
	char *receive_buffer_message;
	uint32_t *receive_buffer_header;
	uint32_t receive_buffer_header_length;
	uint32_t receive_buffer_message_length;
	uint32_t receive_buffer_message_type;
	uint32_t receive_buffer_message_id;
	uint32_t send_buffer_message_type;
	long receive_datatype;

	char *send_buffer_message;
	char *value_buffer = NULL;
	uint32_t *send_buffer_header;
	uint32_t send_buffer_header_length, send_buffer_message_length;
	size_t num_value_bytes;

	MX_SOCKET *mx_socket;
	MX_RECORD_FIELD_PARSE_STATUS parse_status;
	char token_buffer[500];
	void *pointer_to_value;
	long message_buffer_used, buffer_left;

#if defined(_WIN64)
	uint64_t i, xdr_ptr_address, xdr_remainder, xdr_gap_size;
#else
	unsigned long i, xdr_ptr_address, xdr_remainder, xdr_gap_size;
#endif
	int array_is_dynamically_allocated;
	mx_status_type ( *token_parser )
		( void *, char *, MX_RECORD *, MX_RECORD_FIELD *,
		MX_RECORD_FIELD_PARSE_STATUS * );
	mx_status_type mx_status;

	char separators[] = MX_RECORD_FIELD_SEPARATORS;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	/* If the socket handler pointer is NULL, then we have no way
	 * to return a response to the (hypothetical?) client.
	 */

	if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET_HANDLER pointer passed was NULL." );
	}

	receive_buffer_message_type = 0;
	receive_buffer_message_id = 0;

	mx_status = MX_SUCCESSFUL_RESULT;

	mx_socket = socket_handler->synchronous_socket;

	MX_DEBUG( 1,("***** %s invoked for socket %d *****",
		fname, mx_socket->socket_fd));

	/* The do...while(0) loop below is just a trick to make it easy
	 * to jump to the end of this block of code, since we need to send
	 * a message to the client even if an error occurred.
	 */

	do {
		if ( record == (MX_RECORD *) NULL ) {
			mx_status =  mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );

			break;		/* Exit the do...while(0) loop. */
		}
		if ( record_field == (MX_RECORD_FIELD *) NULL ) {
			mx_status =  mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD_FIELD pointer passed was NULL." );

			break;		/* Exit the do...while(0) loop. */
		}
		if ( network_message == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
			mx_status = mx_error( MXE_NULL_ARGUMENT, fname,
		    "The MX_NETWORK_MESSAGE_BUFFER pointer passed was NULL." );

			break;		/* Exit the do...while(0) loop. */
		}
		if ( value_buffer_ptr == NULL ) {
			mx_status = mx_error( MXE_NULL_ARGUMENT, fname,
			"The value buffer pointer passed was NULL." );

			break;		/* Exit the do...while(0) loop. */
		}

		/* Save the message type and message id for later. */

		receive_buffer_header = network_message->u.uint32_buffer;

		receive_buffer_header_length =
		   mx_ntohl(receive_buffer_header[ MX_NETWORK_HEADER_LENGTH ]);

		receive_buffer_message_type =
		   mx_ntohl(receive_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]);

		if ( mx_client_supports_message_ids(socket_handler) == FALSE ) {
			/* Handle clients from MX 1.4 or before. */

			receive_datatype = record_field->datatype;
			receive_buffer_message_id = 0;
		} else {
			receive_datatype = (long)
		      mx_ntohl(receive_buffer_header[ MX_NETWORK_DATA_TYPE ]);

			receive_buffer_message_id =
		      mx_ntohl(receive_buffer_header[ MX_NETWORK_MESSAGE_ID ]);
		}

		/* Check the record field's permissions. */

		if ( record_field->flags & MXFF_NO_ACCESS ) {
			mx_status = mx_error( MXE_PERMISSION_DENIED, fname,
			"MX record field '%s.%s' can not be accessed by "
			"an MX client program.",
				record->name, record_field->name );

			break;		/* Exit the do...while(0) loop. */
		}
		if ( record_field->flags & MXFF_READ_ONLY ) {
			mx_status = mx_error( MXE_READ_ONLY, fname,
			"MX record field '%s.%s' is read-only.",
				record->name, record_field->name );

			break;		/* Exit the do...while(0) loop. */
		}

		/* Get some parameters from the field. */

		if ( record_field->flags & MXFF_VARARGS ) {
			array_is_dynamically_allocated = TRUE;
		} else {
			array_is_dynamically_allocated = FALSE;
		}

#if NETWORK_DEBUG_DEBUG_FIELD_NAMES
	        MX_DEBUG(-2,("%s: record_name = '%s'", fname, record->name));
		MX_DEBUG(-2,("%s: field_name = '%s'", fname,
						record_field->name));
#endif

		pointer_to_value = mx_get_field_value_pointer( record_field );

	        mx_status = mx_get_token_parser(
                        record_field->datatype, &token_parser );

		if ( mx_status.code != MXE_SUCCESS )
			break;		/* Exit the do...while(0) loop. */

		/* Get a pointer to the start of the value string. */

		receive_buffer_message  = network_message->u.char_buffer;
		receive_buffer_message += receive_buffer_header_length;

		receive_buffer_message_length = 
		   mx_ntohl(receive_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]);

		value_buffer = (char *) value_buffer_ptr;

		message_buffer_used = value_buffer - receive_buffer_message;

		if ( message_buffer_used < 0 ) {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The 'value_buffer_ptr' (%p) passed is "
				"located before the 'receive_buffer_message' "
				"pointer (%p).  This should not happen.",
				value_buffer_ptr, receive_buffer_message );

			break;		/* Exit the do...while(0) loop. */
		}

		buffer_left = ((long) receive_buffer_message_length)
					- ((long) message_buffer_used);

		if ( buffer_left <= 0 ) {
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The amount of buffer used (%ld) by the field "
				"name or handle was greater than the total "
				"amount of buffer available (%ld).",
					message_buffer_used,
					(long) receive_buffer_message_length );

			break;		/* Exit the do...while(0) loop. */
		}

		switch( socket_handler->data_format ) {
		case MX_NETWORK_DATAFMT_ASCII:

			/* The data were sent in ASCII MX database format. */

#if 0 && NETWORK_DEBUG_VERBOSE
			MX_DEBUG(-2,("%s: value_string = '%s'",
						fname, value_string));
#endif

			mx_initialize_parse_status( &parse_status,
						value_buffer, separators );

			/* If this is a string field, get the maximum length of
			 * a string token for use by the parser.
			 */

			if ( record_field->datatype == MXFT_STRING ) {
				parse_status.max_string_token_length =
				 mx_get_max_string_token_length( record_field );
			} else {
				parse_status.max_string_token_length = 0L;
			}

			/* Parse the tokens. */

			if ( (record_field->num_dimensions == 0)
	                  || ((record_field->datatype == MXFT_STRING)
			   && (record_field->num_dimensions == 1)) ) {

	                        mx_status = mx_get_next_record_token(
					&parse_status,
                                        token_buffer, sizeof(token_buffer) );

	                        if ( mx_status.code == MXE_SUCCESS ) {
#if NETWORK_DEBUG_VERBOSE
					MX_DEBUG(-2,
				    ("%s: calling *token_parser() for '%s.%s'",
				    fname, record->name, record_field->name));
#endif

		                        mx_status = ( *token_parser ) (
						pointer_to_value,
						token_buffer,
						record, record_field,
						&parse_status );
				}
	                } else {

#if NETWORK_DEBUG_VERBOSE
				MX_DEBUG(-2,
			("%s: calling mx_parse_array_description for '%s.%s'",
				fname, record->name, record_field->name));
#endif
				mx_status = mx_parse_array_description(
                                        pointer_to_value,
                                        (record_field->num_dimensions - 1),
                                        record, record_field,
                                        &parse_status, token_parser );
                	}

			num_value_bytes = strlen( value_buffer );
			break;

		case MX_NETWORK_DATAFMT_RAW:
			mx_status = mx_copy_buffer_to_array(
					value_buffer,
					buffer_left,
					pointer_to_value,
					array_is_dynamically_allocated,
					record_field->datatype,
					record_field->num_dimensions,
					record_field->dimension,
					record_field->data_element_size,
					&num_value_bytes,
				    socket_handler->use_64bit_network_longs );
			break;

		case MX_NETWORK_DATAFMT_XDR:
#if HAVE_XDR
			/* The XDR data pointer 'ptr' must be aligned on a
			 * 4 byte address boundary for XDR data conversion
			 * to work correctly on all architectures.  If the
			 * pointer does not point to an address that is a
			 * multiple of 4 bytes, we move it to the next address
			 * that _is_ and fill the bytes inbetween with zeros.
		 	 */

#if defined(_WIN64)
			xdr_ptr_address = (uint64_t) value_buffer;
#else
			xdr_ptr_address = (unsigned long) value_buffer;
#endif

			xdr_remainder = xdr_ptr_address % 4;

			if ( xdr_remainder != 0 ) {
				xdr_gap_size = 4 - xdr_remainder;

				for ( i = 0; i < xdr_gap_size; i++ ) {
					value_buffer[i] = '\0';
				}

				value_buffer += xdr_gap_size;

				buffer_left -= xdr_gap_size;
			}

			MX_DEBUG( 2,
			("%s: ptr_address = %#lx, value_buffer = %p",
				fname, xdr_ptr_address, value_buffer));

			/* Now we are ready to do the XDR data conversion. */

			mx_status = mx_xdr_data_transfer(
					MX_XDR_DECODE,
					pointer_to_value,
					array_is_dynamically_allocated,
					record_field->datatype,
					record_field->num_dimensions,
					record_field->dimension,
					record_field->data_element_size,
					value_buffer,
					buffer_left,
					&num_value_bytes );
#else
			mx_status = mx_error( MXE_UNSUPPORTED, fname,
				"XDR network data format is not supported "
				"on this system." );
#endif
			break;

		default:
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "Unrecognized network data format type %lu was requested.",
		    		socket_handler->data_format );
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			break;		/* Exit the do...while(0) loop. */

		/* Send the client request just received to the hardware. */

		mx_status = mx_process_record_field( record, record_field,
							MX_PROCESS_PUT, NULL );
	} while (0);

	/* Bug compatibility for old MX clients. */

	if ( ( mx_client_supports_message_ids(socket_handler) == FALSE )
	  && ( receive_buffer_message_type == MX_NETMSG_PUT_ARRAY_BY_HANDLE ) )
	{
		/* MX clients prior to MX 1.5 expected the wrong server
		 * response type for 'put array by handle' messages.
		 */

		send_buffer_message_type =
			mx_server_response(MX_NETMSG_PUT_ARRAY_BY_NAME);
	} else {
		send_buffer_message_type =
			mx_server_response( receive_buffer_message_type );
	}

	/* Allocate a buffer to construct the message to be sent
	 * back to the client.
	 */

	send_buffer_header_length = mx_remote_header_length(socket_handler);

        if ( mx_status.code == MXE_SUCCESS ) {
		send_buffer_message_length = 1;
	} else {
		send_buffer_message_length = (uint32_t)
			( 1 + strlen(mx_status.message) );
	}

	send_buffer_header = network_message->u.uint32_buffer;

	send_buffer_message  = network_message->u.char_buffer;
	send_buffer_message += send_buffer_header_length;

	send_buffer_header[ MX_NETWORK_STATUS_CODE ] = mx_htonl(mx_status.code);

        if ( mx_status.code == MXE_SUCCESS ) {
		strlcpy( send_buffer_message, "", send_buffer_message_length );
	} else {
		strlcpy( send_buffer_message, mx_status.message,
						send_buffer_message_length );
	}

	send_buffer_header[ MX_NETWORK_MAGIC ]
				= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
				= mx_htonl( send_buffer_message_type );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
				= mx_htonl( send_buffer_header_length );

	send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
				= mx_htonl( send_buffer_message_length );

	if ( mx_client_supports_message_ids(socket_handler) ) {

		send_buffer_header[ MX_NETWORK_DATA_TYPE ]
				= mx_htonl( record_field->datatype );

		send_buffer_header[ MX_NETWORK_MESSAGE_ID ]
				= mx_htonl( receive_buffer_message_id );
	}

	/* Send the string description back to the client. */

#if NETWORK_DEBUG
	{
		char text_buffer[200];

		snprintf( text_buffer, sizeof(text_buffer),
			"%s: sending response = '", fname );

		buffer_left = sizeof(text_buffer) - strlen(text_buffer) - 6;

		strlcat( text_buffer, send_buffer_message, sizeof(text_buffer));

		if ( buffer_left < send_buffer_message_length ) {
			strlcat( text_buffer, "***'", sizeof(text_buffer) );
		} else {
			strlcat( text_buffer, "'", sizeof(text_buffer) );
		}

		MX_DEBUG(-2,("%s", text_buffer));
	}
#endif

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
				(int) mx_socket->socket_fd );

		mx_network_display_message( network_message, record_field,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
			"MX (socket %d) PUT_ARRAY('%s.%s') = ",
			(int) socket_handler->synchronous_socket->socket_fd,
			record->name,
			record_field->name );

		mx_network_buffer_show_value(
				value_buffer,
				socket_handler->data_format,
				record_field->datatype,
				receive_buffer_message_type,
				num_value_bytes,
				socket_handler->use_64bit_network_longs );

		fprintf( stderr, "\n" );
	}
#endif

	mx_status = mx_network_socket_send_message( mx_socket,
						-1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		snprintf( location, sizeof(location),
				"%s to client socket %d",
				fname, mx_socket->socket_fd );

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for '%s.%s'", record->name, record_field->name );
#endif

	MXW_UNUSED( receive_datatype );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_get_network_handle( MX_RECORD *record_list,
				MX_SOCKET_HANDLER *socket_handler,
				MX_RECORD *record,
				MX_RECORD_FIELD *record_field )
{
	static const char fname[] = "mxsrv_handle_get_network_handle()";

	char location[ sizeof(fname) + 40 ];
	MX_LIST_HEAD *list_head_struct;
	MX_HANDLE_TABLE *handle_table;
	MX_NETWORK_MESSAGE_BUFFER *network_message;
	uint32_t *send_buffer_header, *send_buffer_message;

	long i, send_buffer_header_length;
	long record_handle;
	long field_handle;
	mx_status_type mx_status;

	record_handle = MX_ILLEGAL_HANDLE;
	field_handle = MX_ILLEGAL_HANDLE;

	mx_status = MX_SUCCESSFUL_RESULT;

	/* Get the record handle for the record. */

	if ( record->handle >= 0 ) {
		record_handle = record->handle;
	} else {
		/* No record handle has been created for this record yet,
		 * so we must make a new one.
		 */

		/* The do...while(0) loop below is just a trick to make
		 * it easy to jump to the end of this block of code.
		 */

		do {
			list_head_struct =
				mx_get_record_list_head_struct( record );

			if ( list_head_struct == (MX_LIST_HEAD *) NULL ) {
				mx_status = mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD structure pointer for record '%s' is NULL.",
				record->name );

				break;	/* Exit the do...while(0) loop. */
			}

			handle_table = (MX_HANDLE_TABLE *)
						list_head_struct->handle_table;

			if ( handle_table == (MX_HANDLE_TABLE *) NULL ) {
				mx_status = mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The handle_table pointer for the current record list is NULL.");

				break;	/* Exit the do...while(0) loop. */
			}

			mx_status = mx_create_handle( &record_handle,
						handle_table, record );

			if ( mx_status.code != MXE_SUCCESS ) {

				break;	/* Exit the do...while(0) loop. */
			}

			record->handle = record_handle;

		} while (0);
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		/* Send back the error message. */

		(void) mx_network_socket_send_error_message(
					socket_handler->synchronous_socket,
					socket_handler->last_rpc_message_id,
					socket_handler->remote_header_length,
					socket_handler->network_debug_flags,
					MX_NETMSG_UNEXPECTED_ERROR,
					mx_status );

		return mx_status;
	}

	/* The field handle is just the array index in the MX_RECORD_FIELD
	 * array for the given record.  After the database has finished
	 * its initialization, this value is stored in the field at
	 * record_field->field_number.
	 */

	field_handle = record_field->field_number;

#if NETWORK_DEBUG_HANDLES
	MX_DEBUG(-2,("%s: record '%s', field '%s', handle = (%ld,%ld)",
		fname, record->name, record_field->name,
		record_handle, field_handle ));
#endif
	/* Check for 0 in data_element_size array elements.  This is
	 * a common error in setting up new record field definitions.
	 */

	for ( i = 0; i < record_field->num_dimensions; i++ ) {
		if ( record_field->data_element_size[i] == 0 ) {
			mx_warning("data_element_size[%ld] for record field "
			"'%s.%s' is 0.  Fixing this will require "
			"recompiling MX.", i, record->name, record_field->name);
		}
	}

	/* Construct the message to be sent back to the client. */

	network_message = socket_handler->message_buffer;

	send_buffer_header = network_message->u.uint32_buffer;

	send_buffer_header_length =
		(long) mx_remote_header_length(socket_handler);

	send_buffer_message = send_buffer_header
			+ ( send_buffer_header_length / sizeof(uint32_t) );

	/* Construct the message header. */

	send_buffer_header[ MX_NETWORK_MAGIC ]
		= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
		= mx_htonl( mx_server_response(MX_NETMSG_GET_NETWORK_HANDLE) );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
		= mx_htonl( send_buffer_header_length );

	send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
		= mx_htonl( 2 * sizeof( uint32_t ) );

	send_buffer_header[ MX_NETWORK_STATUS_CODE ] = mx_htonl( MXE_SUCCESS );

	if ( mx_client_supports_message_ids(socket_handler) ) {

		send_buffer_header[ MX_NETWORK_DATA_TYPE ]
			= mx_htonl( MXFT_ULONG );

		send_buffer_header[ MX_NETWORK_MESSAGE_ID ]
			= mx_htonl( socket_handler->last_rpc_message_id );
	}

	/* Construct the message body. */

	send_buffer_message[0] = mx_htonl( record_handle );
	send_buffer_message[1] = mx_htonl( field_handle );

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
			(int) socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message, NULL,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
		    "MX (socket %d) GET_NETWORK_HANDLE('%s.%s') = (%lu,%lu)\n",
			(int) socket_handler->synchronous_socket->socket_fd,
			record->name,
			record_field->name,
			record_handle,
			field_handle );
	}
#endif

	/* Send the record field handle back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		snprintf( location, sizeof(location),
			"%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_get_field_type( MX_RECORD *record_list,
				MX_SOCKET_HANDLER *socket_handler,
				MX_SOCKET *mx_socket,
				MX_RECORD_FIELD *field,
				MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_get_field_type()";

	char location[ sizeof(fname) + 40 ];
	uint32_t *send_buffer_header, *send_buffer_message;
	uint32_t i, send_buffer_header_length, send_buffer_message_length;
	mx_status_type mx_status;

	MX_DEBUG( 1,("%s for %p, message_length = %lu",
			fname, network_message,
			(unsigned long) network_message->buffer_length ));
	MX_DEBUG( 1,("%s: field->datatype = %ld, field->num_dimensions = %ld",
			fname, field->datatype, field->num_dimensions));
	MX_DEBUG( 1,("%s: field->dimension[0] = %ld",
			fname, field->dimension[0]));

	/* Allocate a buffer to construct the message to be sent
	 * back to the client.
	 */

	send_buffer_header_length = mx_remote_header_length(socket_handler);

	send_buffer_message_length = (uint32_t)
		( sizeof(uint32_t) * ( 2 + field->num_dimensions ) );

	/* Ensure that we can always assign a value to the first element
	 * of the dimension array.
	 */

	if ( send_buffer_message_length <= (2 * sizeof(uint32_t)) ) {
		send_buffer_message_length = 3 * sizeof(uint32_t);
	}

	send_buffer_header = network_message->u.uint32_buffer;

	send_buffer_message = send_buffer_header
		+ ( send_buffer_header_length / sizeof(uint32_t) );

	/* Construct the header of the message. */

	send_buffer_header[ MX_NETWORK_MAGIC ]
				= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
		= mx_htonl( mx_server_response(MX_NETMSG_GET_FIELD_TYPE) );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
				= mx_htonl( send_buffer_header_length );

	send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
				= mx_htonl( send_buffer_message_length );

	send_buffer_header[ MX_NETWORK_STATUS_CODE ] = mx_htonl( MXE_SUCCESS );

	if ( mx_client_supports_message_ids(socket_handler) ) {

		send_buffer_header[ MX_NETWORK_DATA_TYPE ]
			= mx_htonl( MXFT_ULONG );

		send_buffer_header[ MX_NETWORK_MESSAGE_ID ]
			= mx_htonl( socket_handler->last_rpc_message_id );
	}

	/* Construct the body of the message. */

	send_buffer_message[0] = mx_htonl( field->datatype );
	send_buffer_message[1] = mx_htonl( field->num_dimensions );

	if ( field->num_dimensions <= 0 ) {
		send_buffer_message[2] = mx_htonl( 0L );
	} else {
		for ( i = 0; i < field->num_dimensions; i++ ) {
		    send_buffer_message[i+2] = mx_htonl(field->dimension[i]);
		}
	}

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
				(int) mx_socket->socket_fd );

		mx_network_display_message( network_message, NULL,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
			"MX (socket %d) GET_FIELD_TYPE('%s.%s') = "
			"( 'datatype' = %lu, 'num_dimensions' = %lu )\n",
			(int) socket_handler->synchronous_socket->socket_fd,
			field->record->name,
			field->name,
			field->datatype,
			field->num_dimensions );
	}
#endif

	/* Send the field type information back to the client. */

	mx_status = mx_network_socket_send_message( mx_socket,
						-1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		snprintf( location, sizeof(location),
			"%s to client socket %d",
			fname, mx_socket->socket_fd );

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_get_attribute( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_get_attribute()";

	char location[ sizeof(fname) + 40 ];
	uint32_t *send_buffer_header, *send_buffer_message;
	uint32_t send_buffer_header_length, send_buffer_message_length;
	uint32_t *header, *message;
	uint32_t attribute_number;
	double attribute_value;

	union {
		double double_value;
		uint32_t uint32_value[2];
	} u;

	XDR xdrs;
	int xdr_status;
	mx_bool_type illegal_attribute_number;
	mx_status_type mx_status;

	header = network_message->u.uint32_buffer;
	message = header +
		(mx_remote_header_length(socket_handler) / sizeof(uint32_t));

	attribute_number = mx_ntohl( message[2] );

	MX_DEBUG( 2,("%s: attribute_number = %#lx",
		fname, (unsigned long) attribute_number));

	/* Get the requested attribute value. */

	illegal_attribute_number = FALSE;

	switch( attribute_number ) {
	case MXNA_VALUE_CHANGE_THRESHOLD:
		attribute_value = record_field->value_change_threshold;
		break;
	case MXNA_POLL:
		if ( record_field->flags & MXFF_POLL ) {
			attribute_value = 1;
		} else {
			attribute_value = 0;
		}
		break;
	case MXNA_READ_ONLY:
		if ( record_field->flags & MXFF_READ_ONLY ) {
			attribute_value = 1;
		} else {
			attribute_value = 0;
		}
		break;
	case MXNA_NO_ACCESS:
		if ( record_field->flags & MXFF_NO_ACCESS ) {
			attribute_value = 1;
		} else {
			attribute_value = 0;
		}
		break;
	default:
		attribute_value = 0;
		illegal_attribute_number = TRUE;
		break;
	}

	/* Allocate a buffer to construct the message to be sent
	 * back to the client.
	 */

	send_buffer_header_length = mx_remote_header_length(socket_handler);

	send_buffer_header = network_message->u.uint32_buffer;

	send_buffer_message = send_buffer_header
			+ ( send_buffer_header_length / sizeof(uint32_t) );

	send_buffer_message_length = (long)
		( network_message->buffer_length - send_buffer_header_length );

	/* Construct the header of the message. */

	send_buffer_header[ MX_NETWORK_MAGIC ]
				= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
		= mx_htonl( mx_server_response(MX_NETMSG_GET_ATTRIBUTE) );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
				= mx_htonl( send_buffer_header_length );

	/* Construct the body of the message. */

	if ( illegal_attribute_number == FALSE ) {

		switch( socket_handler->data_format ) {
		case MX_NETWORK_DATAFMT_ASCII:
			return mx_error( MXE_UNSUPPORTED, fname,
			    "Message type MX_NETMSG_GET_ATTRIBUTE is not "
			    "supported for ASCII format network connections." );
			break;

		case MX_NETWORK_DATAFMT_RAW:
			u.double_value = attribute_value;

			send_buffer_message[0] = u.uint32_value[0];
			send_buffer_message[1] = u.uint32_value[1];

			send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
						= mx_htonl( sizeof(double) );
			break;

#if HAVE_XDR
		case MX_NETWORK_DATAFMT_XDR:
			xdrmem_create(&xdrs,
				(void *)send_buffer_message, 8, XDR_ENCODE);

			xdr_status = xdr_double( &xdrs, &attribute_value );

			xdr_destroy( &xdrs );

			send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
						= mx_htonl( 8 );
			break;
#endif /* HAVE_XDR */

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
        "Unsupported data format %ld requested by MX client '%s', process %lu.",
				socket_handler->data_format,
				socket_handler->client_address_string,
				socket_handler->process_id );
			break;
		}

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
				= mx_htonl( MXE_SUCCESS );
	} else {
		char *error_message;

		error_message = (char *) send_buffer_message;

		snprintf( error_message,
			send_buffer_message_length,
			"Illegal attribute number %#lx",
			(unsigned long) attribute_number );

		send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
				= mx_htonl( strlen(error_message) + 1 );

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
				= mx_htonl( MXE_ILLEGAL_ARGUMENT );
	}

	if ( mx_client_supports_message_ids(socket_handler) ) {

		send_buffer_header[ MX_NETWORK_DATA_TYPE ]
			= mx_htonl( MXFT_ULONG );

		send_buffer_header[ MX_NETWORK_MESSAGE_ID ]
			= mx_htonl( socket_handler->last_rpc_message_id );
	}

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
			(int) socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message, NULL,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
			"MX (socket %d) GET_ATTRIBUTE('%s.%s', %lu) = %g\n",
			(int) socket_handler->synchronous_socket->socket_fd,
			record->name,
			record_field->name,
			(unsigned long) attribute_number,
			attribute_value );
	}
#endif

	/* Send the attribute information back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		snprintf( location, sizeof(location),
			"%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	MXW_UNUSED( xdr_status );

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_set_attribute( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_set_attribute()";

	char location[ sizeof(fname) + 40 ];
	char *send_buffer_char_message;
	uint32_t *send_buffer_header;
	uint32_t send_buffer_header_length, send_buffer_message_length;
	uint32_t *header, *message;
	uint32_t attribute_number;
	double attribute_value;

	union {
		double double_value;
		uint32_t uint32_value[2];
	} u;

	uint32_t *uint32_value_ptr;
	XDR xdrs;
	int xdr_status;
	mx_bool_type illegal_attribute_number, permission_denied;
	mx_status_type mx_status;

	header = network_message->u.uint32_buffer;
	message = header +
		(mx_remote_header_length(socket_handler) / sizeof(uint32_t));

	attribute_number = mx_ntohl( message[2] );

	uint32_value_ptr = &(message[3]);

	switch( socket_handler->data_format ) {
	case MX_NETWORK_DATAFMT_ASCII:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Message type MX_NETMSG_SET_ATTRIBUTE is not supported for "
		"ASCII format network connections." );
		break;

	case MX_NETWORK_DATAFMT_RAW:
		u.uint32_value[0] = uint32_value_ptr[0];
		u.uint32_value[1] = uint32_value_ptr[1];

		attribute_value = u.double_value;
		break;

#if HAVE_XDR
	case MX_NETWORK_DATAFMT_XDR:
		xdrmem_create( &xdrs, (void *)uint32_value_ptr, 8, XDR_DECODE );

		xdr_status = xdr_double( &xdrs, &attribute_value );

		xdr_destroy( &xdrs );
		break;
#endif /* HAVE_XDR */

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Unsupported data format %ld requested by MX client '%s', process %lu.",
			socket_handler->data_format,
			socket_handler->client_address_string,
			socket_handler->process_id );
		break;
	}

	MXW_UNUSED( xdr_status );

	MX_DEBUG( 2,
	("%s: attribute_number = %#lx, attribute_value = %g", fname,
		(unsigned long) attribute_number, attribute_value));

	/* Set the requested attribute value. */

	illegal_attribute_number = FALSE;
	permission_denied = FALSE;

	switch( attribute_number ) {
	case MXNA_VALUE_CHANGE_THRESHOLD:
		record_field->value_change_threshold = attribute_value;
		break;

	case MXNA_POLL:
		if ( attribute_value >= 0.001 ) {
			record_field->flags |= MXFF_POLL;
		} else {
			record_field->flags &= (~MXFF_POLL);
		}
		break;

	case MXNA_READ_ONLY:
	case MXNA_NO_ACCESS:
		permission_denied = TRUE;
		break;

	default:
		illegal_attribute_number = TRUE;
		break;
	}

	/* Allocate a buffer to construct the message to be sent
	 * back to the client.
	 */

	send_buffer_header_length = mx_remote_header_length(socket_handler);

	send_buffer_header = network_message->u.uint32_buffer;

	send_buffer_char_message  = network_message->u.char_buffer;
	send_buffer_char_message += send_buffer_header_length;

	send_buffer_message_length = (long)
		( network_message->buffer_length - send_buffer_header_length );

	/* Construct the header of the message. */

	send_buffer_header[ MX_NETWORK_MAGIC ]
				= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
		= mx_htonl( mx_server_response(MX_NETMSG_SET_ATTRIBUTE) );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
				= mx_htonl( send_buffer_header_length );

	/* Construct the body of the message. */

	if ( permission_denied ) {
		snprintf( send_buffer_char_message,
		send_buffer_message_length,
		"Cannot change the read only status of record field '%s'.",
				record_field->name );

		send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
			= mx_htonl( strlen(send_buffer_char_message) + 1 );

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
				= mx_htonl( MXE_PERMISSION_DENIED );
	} else
	if ( illegal_attribute_number ) {
		snprintf( send_buffer_char_message,
				send_buffer_message_length,
				"Illegal attribute number %#lx",
				(unsigned long) attribute_number );

		send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
			= mx_htonl( strlen(send_buffer_char_message) + 1 );

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
				= mx_htonl( MXE_ILLEGAL_ARGUMENT );
	} else {
		send_buffer_char_message[0] = '\0';

		send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ] = mx_htonl( 1 );

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
				= mx_htonl( MXE_SUCCESS );
	}

	if ( mx_client_supports_message_ids(socket_handler) ) {

		send_buffer_header[ MX_NETWORK_DATA_TYPE ]
			= mx_htonl( MXFT_ULONG );

		send_buffer_header[ MX_NETWORK_MESSAGE_ID ]
			= mx_htonl( socket_handler->last_rpc_message_id );
	}

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
			(int) socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message, NULL,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
			"MX (socket %d) SET_ATTRIBUTE('%s.%s', %lu) = %g\n",
			(int) socket_handler->synchronous_socket->socket_fd,
			record->name,
			record_field->name,
			(unsigned long) attribute_number,
			attribute_value );
	}
#endif

	/* Send the attribute information back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		snprintf( location, sizeof(location),
			"%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	MX_DEBUG(-1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_set_client_info( MX_RECORD *record_list,
				MX_SOCKET_HANDLER *socket_handler, 
				MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_set_client_info()";

	char location[ sizeof(fname) + 40 ];

	uint32_t *send_buffer_header;
	uint32_t send_buffer_header_length, send_buffer_message_length;
	char *send_buffer_message;
	char *message_string;
	char *ptr, *ptr2, *endptr;
	size_t length;
	long header_length;
	mx_status_type mx_status;

	header_length = (long) mx_remote_header_length(socket_handler);

	message_string  = network_message->u.char_buffer;
	message_string += header_length;

	MX_DEBUG( 1,("%s for '%s', message_length = %lu",
		fname, message_string,
		(unsigned long) network_message->buffer_length ));

	/* The do...while(0) loop below is just a trick to make it easy
	 * to jump to the end of this block of code.
	 */

	do {
		/* How many characters at the start of the string
		 * are whitespace?
		 */

		length = strspn( message_string, MX_WHITESPACE );

		ptr = message_string + length;

		/* The username should be the first string in the message
		 * buffer.  Find out how long it is.
		 */

		length = strcspn( ptr, MX_WHITESPACE );

		if ( ((long) length) <= 0 )
			break;	/* Exit the do...while(0) loop. */

		ptr2 = ptr + length;

		*ptr2 = '\0';

		if ( length > MXU_USERNAME_LENGTH )
			length = MXU_USERNAME_LENGTH;

		/* The username is only stored if it has not yet been set. */

		if ( strlen( socket_handler->username ) == 0 ) {
			strlcpy( socket_handler->username, ptr, length+1 );
		}

		ptr = ptr2 + 1;

		/* The next string should be the program name.  How many
		 * whitespace characters separate us from it?
		 */

		length = strspn( ptr, MX_WHITESPACE );

		ptr += length;

		/* How long is the program name? */

		length = strcspn( ptr, MX_WHITESPACE );

		if ( ((long) length) <= 0 )
			break;	/* Exit the do...while(0) loop. */

		ptr2 = ptr + length;

		*ptr2 = '\0';

		if ( length > MXU_PROGRAM_NAME_LENGTH )
			length = MXU_PROGRAM_NAME_LENGTH;

		strlcpy( socket_handler->program_name, ptr, length+1 );

		ptr = ptr2 + 1;

		/* The next string should be the process id.  How many
		 * whitespace characters separate us from it?
		 */

		length = strspn( ptr, MX_WHITESPACE );

		ptr += length;

		/* How long is the process id? */

		length = strcspn( ptr, MX_WHITESPACE );

		if ( ((long) length) <= 0 )
			break;	/* Exit the do...while(0) loop. */

		ptr2 = ptr + length;

		*ptr2 = '\0';

		socket_handler->process_id = strtoul( ptr, &endptr, 0 );

		if ( *endptr != '\0' ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The string '%s' sent for the process id is not a legal number.", ptr );
		}

		/* Ignore the rest of the message buffer. */

	} while (0);

	MX_DEBUG( 2,("%s: username = '%s'", fname, socket_handler->username));
	MX_DEBUG( 2,("%s: program_name = '%s'",
				fname, socket_handler->program_name));

	/* Allocate a buffer to construct the success message to be sent
	 * back to the client.
	 */

	send_buffer_header_length = mx_remote_header_length(socket_handler);

	send_buffer_message_length = 1;

	send_buffer_header = network_message->u.uint32_buffer;

	send_buffer_message  = network_message->u.char_buffer;
	send_buffer_message += send_buffer_header_length;

	/* Construct the header of the message. */

	send_buffer_header[ MX_NETWORK_MAGIC ]
				= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
		= mx_htonl( mx_server_response(MX_NETMSG_SET_CLIENT_INFO) );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
				= mx_htonl( send_buffer_header_length );

	send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
				= mx_htonl( send_buffer_message_length );

	send_buffer_header[ MX_NETWORK_STATUS_CODE ] = mx_htonl( MXE_SUCCESS );

	if ( mx_client_supports_message_ids(socket_handler) ) {

		send_buffer_header[ MX_NETWORK_DATA_TYPE ]
			= mx_htonl( MXFT_STRING );

		send_buffer_header[ MX_NETWORK_MESSAGE_ID ]
			= mx_htonl( socket_handler->last_rpc_message_id );
	}

	/* Construct an empty body for the message. */

	send_buffer_message[0] = '\0';

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
			(int) socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message, NULL,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
			"MX (socket %d) SET_CLIENT_INFO(user = '%s', "
				"program = '%s')\n",
			(int) socket_handler->synchronous_socket->socket_fd,
			socket_handler->username,
			socket_handler->program_name );
	}
#endif

	/* Send the success message back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		snprintf( location, sizeof(location),
			"%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_get_option( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_get_option()";

	char location[ sizeof(fname) + 40 ];
	uint32_t *send_buffer_header, *send_buffer_message;
	uint32_t send_buffer_header_length, send_buffer_message_length;
	uint32_t *option_array;
	uint32_t option_number, option_value;
	int illegal_option_number;
	mx_status_type mx_status;

	option_array  = network_message->u.uint32_buffer;
	option_array +=
		(mx_remote_header_length(socket_handler) / sizeof(uint32_t));

	option_number = mx_ntohl( option_array[0] );

	MX_DEBUG( 2,("%s: option_number = %#lx",
		fname, (unsigned long) option_number));

	/* Get the requested option value. */

	illegal_option_number = FALSE;

	switch( option_number ) {
	case MX_NETWORK_OPTION_DATAFMT:
		option_value = (uint32_t) socket_handler->data_format;
		break;
	case MX_NETWORK_OPTION_NATIVE_DATAFMT:
		option_value = (uint32_t) mx_native_data_format();
		break;
	case MX_NETWORK_OPTION_64BIT_LONG:

#if ( MX_WORDSIZE != 64 )
		option_value = FALSE;
#else  /* MX_WORDSIZE == 64 */
		if ( socket_handler->use_64bit_network_longs ) {
			option_value = FALSE;
		} else {
			option_value = TRUE;
		}
#endif /* MX_WORDSIZE == 64 */

		break;
	case MX_NETWORK_OPTION_WORDSIZE:
		option_value = MX_WORDSIZE;
		break;
	default:
		option_value = 0;
		illegal_option_number = TRUE;
		break;
	}

	/* Allocate a buffer to construct the message to be sent
	 * back to the client.
	 */

	send_buffer_header_length = mx_remote_header_length(socket_handler);

	send_buffer_header = network_message->u.uint32_buffer;

	send_buffer_message = send_buffer_header
			+ ( send_buffer_header_length / sizeof(uint32_t) );

	send_buffer_message_length = (long)
		( network_message->buffer_length - send_buffer_header_length );

	/* Construct the header of the message. */

	send_buffer_header[ MX_NETWORK_MAGIC ]
				= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
		= mx_htonl( mx_server_response(MX_NETMSG_GET_OPTION) );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
				= mx_htonl( send_buffer_header_length );

	/* Construct the body of the message. */

	if ( illegal_option_number == FALSE ) {
		send_buffer_message[0] = mx_htonl( option_value );

		send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
				= mx_htonl( sizeof(uint32_t) );

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
				= mx_htonl( MXE_SUCCESS );
	} else {
		char *error_message;

		error_message = (char *) send_buffer_message;

		snprintf( error_message,
			send_buffer_message_length,
			"Illegal option number %#lx",
			(unsigned long) option_number );

		send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
				= mx_htonl( strlen(error_message) + 1 );

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
				= mx_htonl( MXE_ILLEGAL_ARGUMENT );
	}

	if ( mx_client_supports_message_ids(socket_handler) ) {

		send_buffer_header[ MX_NETWORK_DATA_TYPE ]
			= mx_htonl( MXFT_ULONG );

		send_buffer_header[ MX_NETWORK_MESSAGE_ID ]
			= mx_htonl( socket_handler->last_rpc_message_id );
	}

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
			(int) socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message, NULL,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
			"MX (socket %d) GET_OPTION( %lu ) = %lu\n",
			(int) socket_handler->synchronous_socket->socket_fd,
			(unsigned long) option_number,
			(unsigned long) option_value );
	}
#endif

	/* Send the option information back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		snprintf( location, sizeof(location),
			"%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_set_option( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_set_option()";

	MX_LIST_HEAD *list_head;
	char location[ sizeof(fname) + 40 ];
	char *send_buffer_char_message;
	uint32_t *send_buffer_header;
	uint32_t send_buffer_header_length;
	uint32_t send_buffer_message_length;
	uint32_t *option_array;
	uint32_t option_number, option_value;
	int illegal_option_number, illegal_option_value;
	mx_status_type mx_status;

	option_array  = network_message->u.uint32_buffer;
	option_array +=
		(mx_remote_header_length(socket_handler) / sizeof(uint32_t));

	option_number = mx_ntohl( option_array[0] );
	option_value = mx_ntohl( option_array[1] );

	MX_DEBUG( 2,("%s: option_number = %#lx, option_value = %#lx", fname,
		(unsigned long) option_number,
		(unsigned long) option_value));

	/* Set the requested option value. */

	illegal_option_number = FALSE;
	illegal_option_value = FALSE;

	switch( option_number ) {
	case MX_NETWORK_OPTION_DATAFMT:
		socket_handler->data_format = option_value;
		socket_handler->message_buffer->data_format = option_value;
		break;

	case MX_NETWORK_OPTION_64BIT_LONG:

#if ( MX_WORDSIZE != 64 )
		if ( option_value != FALSE ) {
			illegal_option_value = TRUE;
		}
#else  /* MX_WORDSIZE == 64 */
		if ( option_value == FALSE ) {
			socket_handler->use_64bit_network_longs = FALSE;
		} else {
			socket_handler->use_64bit_network_longs = TRUE;
		}
#endif /* MX_WORDSIZE == 64 */

		MX_DEBUG( 2,
		("%s: option_value = %d, use_64bit_network_longs = %d",
			fname, (int) option_value,
			(int) socket_handler->use_64bit_network_longs ));
		break;

	case MX_NETWORK_OPTION_CLIENT_VERSION:
		socket_handler->remote_mx_version = option_value;
#if 0
		MX_DEBUG(-2,("%s: remote_mx_version = %lu",
			fname, socket_handler->remote_mx_version));
#endif
		break;

		/* FIXME: The following case is not Y2038 safe, since
		 * years after 2038 will need 64-bit timestamps.
		 */

	case MX_NETWORK_OPTION_CLIENT_VERSION_TIME:
		socket_handler->remote_mx_version_time = option_value;

#if 0
		MX_DEBUG(-2,("%s: remote_mx_version_time = %" PRIu64,
			fname, socket_handler->remote_mx_version_time));
#endif
		break;

	default:
		illegal_option_number = TRUE;
		break;
	}

	/* Allocate a buffer to construct the message to be sent
	 * back to the client.
	 */

	send_buffer_header_length = mx_remote_header_length(socket_handler);

	send_buffer_header = network_message->u.uint32_buffer;

	send_buffer_char_message  = network_message->u.char_buffer;
	send_buffer_char_message += send_buffer_header_length;

	send_buffer_message_length = (long)
		( network_message->buffer_length - send_buffer_header_length );

	/* Construct the header of the message. */

	send_buffer_header[ MX_NETWORK_MAGIC ]
				= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
		= mx_htonl( mx_server_response(MX_NETMSG_SET_OPTION) );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
				= mx_htonl( send_buffer_header_length );

	/* Construct the body of the message. */

	if ( illegal_option_number ) {
		snprintf( send_buffer_char_message,
			send_buffer_message_length,
			"Illegal option number %#lx",
			(unsigned long) option_number );

		send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
			= mx_htonl( strlen(send_buffer_char_message) + 1 );

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
				= mx_htonl( MXE_ILLEGAL_ARGUMENT );
	} else
	if ( illegal_option_value ) {
		switch( option_number ) {
		case MX_NETWORK_OPTION_64BIT_LONG:
			list_head = mx_get_record_list_head_struct(
						record_list );

			if ( list_head == NULL ) {
				mx_info(
				"Exiting due to corrupt list head record." );

				exit( MXE_CORRUPT_DATA_STRUCTURE );
			}

			snprintf( send_buffer_char_message,
				send_buffer_message_length,
				"Server '%s' does not support the 64-bit longs "
				"option, since it is not a 64-bit computer.",
					list_head->hostname );
			break;
		default:
			snprintf( send_buffer_char_message,
				send_buffer_message_length,
				"Illegal option value %#lx for option %#lx",
					(unsigned long) option_value,
					(unsigned long) option_number );
		}

		send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ]
			= mx_htonl( strlen(send_buffer_char_message) + 1 );

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
			= mx_htonl( MXE_ILLEGAL_ARGUMENT );
	} else {
		send_buffer_char_message[0] = '\0';

		send_buffer_header[ MX_NETWORK_MESSAGE_LENGTH ] = mx_htonl( 1 );

		send_buffer_header[ MX_NETWORK_STATUS_CODE ]
				= mx_htonl( MXE_SUCCESS );
	}

	if ( mx_client_supports_message_ids(socket_handler) ) {

		send_buffer_header[ MX_NETWORK_DATA_TYPE ]
			= mx_htonl( MXFT_ULONG );

		send_buffer_header[ MX_NETWORK_MESSAGE_ID ]
			= mx_htonl( socket_handler->last_rpc_message_id );
	}

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
			(int) socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message, NULL,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
			"MX (socket %d) SET_OPTION: Set option %lu to %lu\n",
			(int) socket_handler->synchronous_socket->socket_fd,
			(unsigned long) option_number,
			(unsigned long) option_value );
	}
#endif

	/* Send the option information back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		snprintf( location, sizeof(location),
			"%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxsrv_record_field_callback( MX_CALLBACK *callback, void *argument )
{
	static const char fname[] = "mxsrv_record_field_callback()";

	MX_LIST *callback_socket_handler_list;
	MX_LIST_ENTRY *list_start, *list_entry;
	MX_CALLBACK_SOCKET_HANDLER_INFO *csh_info;
	MX_SOCKET_HANDLER *socket_handler;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	mx_status_type mx_status;

	if ( callback == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CALLBACK pointer passed was NULL." );
	}

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s (%p): callback = %p, id = %#lx, argument = %p",
		fname, mxsrv_record_field_callback,
		callback, (unsigned long) callback->callback_id, argument));
#endif

	callback_socket_handler_list = (MX_LIST *) argument;

	if ( callback_socket_handler_list == (MX_LIST *) NULL ) {
		return mx_error( MXE_INVALID_CALLBACK, fname,
		"The MX_LIST pointer passed was NULL." );
	}

	if ( callback->callback_class != MXCBC_FIELD ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal callback class %lu used.  "
		"Only MXCBC_FIELD (%d) callbacks are allowed here.",
			callback->callback_class, MXCBC_FIELD );
	}

	record_field = callback->u.record_field;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"No MX_RECORD_FIELD pointer was specified for callback %p",
			callback );
	}

	record = record_field->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Callbacks are not supported for temporary record fields.  "
		"Field name = '%s'", record_field->name );
	}

	switch( callback->callback_type ) {
	case MXCBT_VALUE_CHANGED:

		/* Send value changed callbacks to the clients. */

		list_start = callback_socket_handler_list->list_start;

		if ( list_start == (MX_LIST_ENTRY *) NULL ) {
			(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The list_start pointer for socket handler list %p "
			"used by callback %p for record field '%s.%s' is NULL.",
				callback_socket_handler_list, callback,
				record_field->record->name, record_field->name);

			return MX_SUCCESSFUL_RESULT;
		}

		list_entry = list_start;

		do {
			csh_info = list_entry->list_entry_data;

			if ( csh_info == NULL ) {
				return mx_error(
				MXE_CORRUPT_DATA_STRUCTURE, fname,
				"An MX_CALLBACK_SOCKET_HANDLER_INFO pointer "
				"for record field '%s.%s' is NULL.",
					record->name, record_field->name );
			}

			socket_handler = csh_info->socket_handler;

			if ( socket_handler == NULL ) {
				return mx_error(
				MXE_CORRUPT_DATA_STRUCTURE, fname,
	      "An MX_SOCKET_HANDLER pointer for record field '%s.%s' is NULL.",
					record->name, record_field->name );
			}

#if NETWORK_DEBUG_CALLBACKS
			MX_DEBUG(-2,
			("%s: sending field '%s.%s' to '%s', pid %lu",
				fname, record->name, record_field->name,
				socket_handler->client_address_string,
				socket_handler->process_id));
#endif
			message_buffer = socket_handler->message_buffer;

			if ( message_buffer ==  NULL ) {
				return mx_error( MXE_INVALID_CALLBACK, fname,
				"The MX_NETWORK_MESSAGE_BUFFER corresponding "
				"to the socket handler passed is NULL." );
			}

			mx_status = mxsrv_send_field_value_to_client(
						socket_handler,
						record, record_field,
						message_buffer,
					mx_server_response(MX_NETMSG_CALLBACK),
						callback->callback_id );
#if NETWORK_DEBUG_CALLBACKS
			MX_DEBUG(-2,
			("%s: mxsrv_send_field_value_to_client status = %ld",
				fname, mx_status.code));
#endif
			list_entry = list_entry->next_list_entry;

		} while ( list_entry != list_start );

		break;
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_add_callback( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *field,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_add_callback()";

	uint32_t *uint32_header, *uint32_message;
	uint32_t message_id, message_type;
	unsigned long header_length, message_length;
	unsigned long record_handle, field_handle, callback_type;
	MX_CALLBACK *callback_object;
	mx_status_type mx_status;

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s: record = '%s', field = '%s'",
		fname, record->name, field->name ));
#endif

	uint32_header = network_message->u.uint32_buffer;

	header_length  = mx_ntohl( uint32_header[MX_NETWORK_HEADER_LENGTH] );

	if ( mx_client_supports_message_ids(socket_handler) == FALSE ) {

		return mx_error( MXE_UNSUPPORTED, fname,
		"This server cannot support callbacks from clients using "
		"MX 1.4 or before." );
	}

	message_length = mx_ntohl( uint32_header[MX_NETWORK_MESSAGE_LENGTH] );

	if ( message_length < (3 * sizeof(uint32_t)) ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"The ADD_CALLBACK message of length %lu sent by "
		"client socket %d is shorter than the required length of %lu.",
			message_length,
			socket_handler->synchronous_socket->socket_fd,
			(unsigned long) (3L * sizeof(uint32_t)) );
	}

	message_type = mx_ntohl( uint32_header[MX_NETWORK_MESSAGE_TYPE] );

	message_id = mx_ntohl( uint32_header[MX_NETWORK_MESSAGE_ID] );

	uint32_message = uint32_header + ( header_length / sizeof(uint32_t) );

	record_handle = mx_ntohl( uint32_message[0] );
	field_handle  = mx_ntohl( uint32_message[1] );
	callback_type = mx_ntohl( uint32_message[2] );

	MXW_UNUSED( record_handle );
	MXW_UNUSED( field_handle );

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s: (%lu,%lu) callback_type = %lu",
		fname, record_handle, field_handle, callback_type ));
#endif
	/* If callbacks are not currently enabled for this server,
	 * send an error message to the client that tells it this.
	 */

	if ( socket_handler->list_head->master_timer == NULL ) {

		mx_status = mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		"Callbacks are not currently enabled for this MX server.  "
		"Check the configuration file 'mxserver.opt' to see "
		"if the '-c' option is present there." );

		(void) mx_network_socket_send_error_message(
					socket_handler->synchronous_socket,
					message_id,
					socket_handler->remote_header_length,
					socket_handler->network_debug_flags,
				(long) mx_server_response( message_type ),
					mx_status );

		return mx_status;
	}

	/*------------------------------------------------------------*/

	mx_status = mx_local_field_add_socket_handler_to_callback(
					field,
					callback_type,
					mxsrv_record_field_callback,
					socket_handler,
					&callback_object );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( callback_object == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"For some reason, mx_record_field_add_callback() or "
		"mx_record_field_find_callback() returned a NULL"
		"MX_CALLBACK object in spite of the fact that it returned "
		"an MXE_SUCCESS status code.  This should not happen." );
	}

	/* Construct a message to send the callback id back to the client. */

	uint32_header = network_message->u.uint32_buffer;

	uint32_header[MX_NETWORK_MAGIC] = mx_htonl(MX_NETWORK_MAGIC_VALUE);

	uint32_header[MX_NETWORK_HEADER_LENGTH] =
			mx_htonl(socket_handler->remote_header_length);

	uint32_header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl(sizeof(uint32_t));

	uint32_header[MX_NETWORK_MESSAGE_TYPE] =
			mx_htonl( mx_server_response(MX_NETMSG_ADD_CALLBACK) );

	uint32_header[MX_NETWORK_STATUS_CODE] = mx_htonl(MXE_SUCCESS);

	if ( mx_client_supports_message_ids(socket_handler) ) {

		uint32_header[MX_NETWORK_DATA_TYPE] = mx_htonl(MXFT_HEX);

		uint32_header[MX_NETWORK_MESSAGE_ID] = mx_htonl(message_id);
	}

	uint32_message = uint32_header +
		(mx_remote_header_length(socket_handler) / sizeof(uint32_t));

	uint32_message[0] = mx_htonl( callback_object->callback_id );

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
			(int) socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message, NULL,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
			"MX (socket %d) ADD_CALLBACK('%s.%s') = %#lx\n",
			(int) socket_handler->synchronous_socket->socket_fd,
			record->name,
			field->name,
			(unsigned long) callback_object->callback_id );
	}
#endif

	/* Send the message to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mxsrv_handle_delete_callback( MX_RECORD *record,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_delete_callback()";

	MX_LIST_HEAD *list_head;
	MX_HANDLE_TABLE *callback_handle_table;
	MX_HANDLE_STRUCT *handle_struct, *handle_struct_array;
	MX_CALLBACK *callback, *callback_ptr;
	signed long callback_handle;
	MX_LIST *callback_socket_handler_list;
	MX_LIST_ENTRY *callback_socket_handler_list_entry;
	MX_CALLBACK_SOCKET_HANDLER_INFO *csh_info;
	MX_RECORD_FIELD *record_field = NULL;
	MX_LIST *rf_callback_list;
	MX_LIST_ENTRY *rf_callback_list_entry;
	uint32_t *uint32_header, *uint32_message;
	uint32_t message_id, message_type, callback_id;
	char *char_message;
	unsigned long header_length, message_length;
	unsigned long i, num_handles;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed to this function was NULL." );
	}
	if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_SOCKET_HANDLER pointer passed to this function was NULL." );
	}
	if ( network_message == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_MESSAGE_BUFFER pointer passed "
		"to this function was NULL." );
	}

	/* Get a pointer to the list_head object. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Cannot get the MX_LIST_HEAD pointer for the record list "
		"containing the record '%s'.", record->name );
	}

	/* Get a pointer to the callback handle table. */

	callback_handle_table = list_head->server_callback_handle_table;

	if ( callback_handle_table == (MX_HANDLE_TABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The server_callback_handle_table pointer for "
		"the record list is NULL." );
	}

	/* Look in the message buffer for the callback id. */

	uint32_header = network_message->u.uint32_buffer;

	header_length = mx_ntohl( uint32_header[MX_NETWORK_HEADER_LENGTH] );

	message_length = mx_ntohl( uint32_header[MX_NETWORK_MESSAGE_LENGTH] );

	message_type = mx_ntohl( uint32_header[MX_NETWORK_MESSAGE_TYPE] );

	message_id = mx_ntohl( uint32_header[MX_NETWORK_MESSAGE_ID] );

	uint32_message = uint32_header + ( header_length / sizeof(uint32_t) );

	callback_id = mx_ntohl( uint32_message[0] );

	MXW_UNUSED( message_length );

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s invoked for callback id %#lx",
		fname, (unsigned long) callback_id ));
#endif

	/* Look through the handle table for a callback with
	 * the requested callback ID.
	 */

	num_handles = callback_handle_table->num_blocks
			* callback_handle_table->block_size;

	handle_struct_array = callback_handle_table->handle_struct_array;

	if ( handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The handle_struct_array for the server's callback handle "
		"table is NULL." );
	}

	callback = NULL;

	for ( i = 0; i < num_handles; i++ ) {
		handle_struct = &handle_struct_array[i];

		if ( handle_struct->pointer != NULL ) {
			callback_ptr    = handle_struct->pointer;
			callback_handle = handle_struct->handle;

			if ( callback_ptr->callback_id == callback_id ) {
				callback = callback_ptr;
				break;		/* Exit the for() loop. */
			}
		}
	}

	if ( callback == NULL ) {
		mx_status = mx_error( MXE_NOT_FOUND, fname,
			"Callback id %#lx was not found in the "
			"server's callback handle table",
				(unsigned long) callback_id );

		return mx_network_socket_send_error_message(
					socket_handler->synchronous_socket,
					message_id,
					socket_handler->remote_header_length,
					socket_handler->network_debug_flags,
				(long) mx_server_response( message_type ),
					mx_status );
	}

	if ( callback->callback_class != MXCBC_FIELD ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The callback class for callback %p, id %#lx is not "
		"MXCBC_FIELD.  Instead it is %ld",
			callback,
			(unsigned long) callback->callback_id,
			callback->callback_class );
	}

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s: callback %p has callback id %#lx",
		fname, callback, (unsigned long) callback_id ));
#endif
	/* Record the fact that the number of users of this callback
	 * has now been reduced by 1.
	 */

	callback->usage_count--;

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s: callback %p, new usage count = %lu",
		fname, callback, callback->usage_count));
#endif

	/* See if the callback still has users for this socket handler. */

	/* Find the list of socket handlers attached to this callback. */

	callback_socket_handler_list = callback->callback_argument;

	if ( callback_socket_handler_list == (MX_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The socket handler list for callback %p, id %#lx is NULL.",
			callback, (unsigned long) callback->callback_id );
	}

	/* Find this socket handler in the callback socket handler list. */

	mx_status = mxp_local_field_find_socket_handler_in_list(
				callback_socket_handler_list,
				socket_handler,
				&callback_socket_handler_list_entry );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	csh_info = callback_socket_handler_list_entry->list_entry_data;

	/* Record the fact that the number of users of this callback
	 * _via_ _this_ _socket_ _handler_ has now been reduced by 1.
	 */

	csh_info->usage_count--;

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,
	("%s: callback %p, socket handler %p, csh_info->usage_count = %lu",
		fname, callback, csh_info->socket_handler,
		csh_info->usage_count));
#endif

	if ( csh_info->usage_count == 0 ) {

		/* If this combination of socket handler and callback does
		 * not have any more users, then delete the callback socket
		 * handler list entry.
		 */

		mx_free( csh_info );

		mx_status = mx_list_delete_entry( callback_socket_handler_list,
					callback_socket_handler_list_entry );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_list_entry_destroy( callback_socket_handler_list_entry );

		/* This socket handler's reference to the callback
		 * is now deleted.
		 */

#if NETWORK_DEBUG_CALLBACKS
		MX_DEBUG(-2,
	    ("%s: socket handler %p is no longer using callback %p, ID %#lx",
		    	fname, socket_handler, callback,
			(unsigned long) callback->callback_id));
#endif
	}

	/* Does this callback still have users? */

	if ( callback->usage_count == 0 ) {

		/********************************************************
		 * If we get here, then the callback has no more users. *
		 ********************************************************/

#if NETWORK_DEBUG_CALLBACKS
		MX_DEBUG(-2,
		("%s: No clients are still using callback %p, id %#lx",
		    fname, callback, (unsigned long) callback->callback_id));
#endif
		/* If the number of users is zero, then the variable
		 * callback_socket_handler_list->num_list_entries
		 * should also be zero.
		 */

		if ( callback_socket_handler_list->num_list_entries != 0 ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The usage count for callback %p, ID %#lx is zero, "
			"but the matching value for "
			"callback_socket_handler_list->num_list_entries "
			"is _NOT_ zero.  Instead, it has a value of %lu.",
				callback, (unsigned long) callback->callback_id,
				callback_socket_handler_list->num_list_entries);
		}

		/* Delete the callback from the server's
		 * callback handle table.
		 */

		callback_handle = (signed long)
			( callback->callback_id & MX_NETWORK_MESSAGE_ID_MASK );

		mx_status = mx_delete_handle( callback_handle,
						callback_handle_table );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Find the local record field's list of callbacks. */

		record_field = callback->u.record_field;

		if ( record_field == (MX_RECORD_FIELD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD_FIELD pointer for callback %p, id %#lx is NULL",
			callback, (unsigned long) callback->callback_id );
		}

		rf_callback_list = record_field->callback_list;

		if ( rf_callback_list == (MX_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The callback_list pointer for record field '%s' is NULL.",
			record_field->name );
		}

		/* Delete the callback from the record field's callback list. */

		mx_status = mx_list_find_list_entry( rf_callback_list,
						callback, 
						&rf_callback_list_entry );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_list_delete_entry( rf_callback_list,
						rf_callback_list_entry );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_list_entry_destroy( rf_callback_list_entry );

		/*
		 * We have now deleted all of the references to the callback
		 * that should exist.  Just in case somebody still has a
		 * pointer to the callback, we zero out the contents of the
		 * callback structure and then free it.  The contents are
		 * zeroed out to make it easier to detect bugs in the
		 * handling of callback pointers.
		 */

		memset( callback, 0, sizeof(MX_CALLBACK) );

		mx_free( callback );

		/* The callback is fully deleted now. */

#if NETWORK_DEBUG_CALLBACKS
		MX_DEBUG(-2,("%s: The callback is now fully deleted.", fname));
#endif
	}

	/* Send a message back to the client to tell it that the
	 * deletion operation is complete.
	 */

	/* Construct the message. */

	uint32_header = network_message->u.uint32_buffer;

	uint32_header[MX_NETWORK_MAGIC] = mx_htonl(MX_NETWORK_MAGIC_VALUE);

	uint32_header[MX_NETWORK_HEADER_LENGTH] =
			mx_htonl(socket_handler->remote_header_length);

	uint32_header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( 1 );

	uint32_header[MX_NETWORK_MESSAGE_TYPE] =
		mx_htonl( mx_server_response(MX_NETMSG_DELETE_CALLBACK) );

	uint32_header[MX_NETWORK_STATUS_CODE] = mx_htonl(MXE_SUCCESS);

	if ( mx_client_supports_message_ids(socket_handler) ) {

		uint32_header[MX_NETWORK_DATA_TYPE] = mx_htonl(MXFT_CHAR);

		uint32_header[MX_NETWORK_MESSAGE_ID] = mx_htonl(message_id);
	}

	uint32_message = uint32_header +
		(mx_remote_header_length(socket_handler) / sizeof(uint32_t));

	char_message = (char *) uint32_message;

	char_message[0] = '\0';

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
			(int) socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message, NULL,
				socket_handler->use_64bit_network_longs );
	}

	if ( socket_handler->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mxsrv_print_timestamp();

		fprintf( stderr,
			"MX (socket %d) DELETE_CALLBACK('%s.%s') = %#lx\n",
			(int) socket_handler->synchronous_socket->socket_fd,
			record->name,
			record_field->name,
			(unsigned long) callback_id );
	}
#endif

	/* Send the message to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

#if HAVE_UNIX_DOMAIN_SOCKETS

mx_status_type
mxsrv_get_unix_domain_socket_credentials( MX_SOCKET_HANDLER *socket_handler )
{
	static const char fname[] =
		"mxsrv_get_unix_domain_socket_credentials()";

	char null_byte;
	mx_status_type mx_status;

	null_byte = 1;

	if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET_HANDLER pointer passed was NULL." );
	}

#if 0
#else
	/* If we cannot get user credentials on this platform, then just
	 * read and discard the null byte.
	 */

	socket_handler->username[0] = '\0';

	mx_status = mx_socket_receive( socket_handler->synchronous_socket,
					&null_byte, 1,
					NULL, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif

/*--------------------------------------------------------------------------*/

/* For now we use CR-LF line terminators. */

static char ascii_line_terminators[] = "\r\n";

mx_status_type
mxsrv_ascii_client_socket_process_event( MX_RECORD *record_list,
				MX_SOCKET_HANDLER *socket_handler,
				MX_SOCKET_HANDLER_LIST *socket_handler_list,
				MX_EVENT_HANDLER *event_handler )
{
	static const char fname[] = "mxsrv_ascii_client_socket_process_event()";

	MX_SOCKET *client_socket = NULL;
	MX_LIST_HEAD *list_head = NULL;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer = NULL;
	char *message_ptr = NULL;
	char *receive_ptr = NULL;
	size_t num_bytes_already_received, remaining_buffer_length;
	mx_status_type mx_status, mx_status2;

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX record list pointer passed was NULL." );
	}

	if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_SOCKET_HANDLER pointer passed was NULL." );
	}

	client_socket = socket_handler->synchronous_socket;

	MX_DEBUG(-2,("%s: event from client socket %d.",
		fname, (int) client_socket->socket_fd ));

	if ( socket_handler_list == (MX_SOCKET_HANDLER_LIST *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_SOCKET_HANDLER_LIST pointer passed was NULL." );
	}
	if ( event_handler == (MX_EVENT_HANDLER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_EVENT_HANDLER pointer passed was NULL." );
	}

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the MX database is NULL." );
	}

	/* We look to see if a complete message has been sent by the client.
	 * A complete message includes the line terminators at the end.
	 *
	 * Note the client may be a user that has used 'telnet' to connect to
	 * the port, so we _must_ _not_ block waiting for a complete message.
	 */

	message_buffer = socket_handler->message_buffer;

	message_ptr = message_buffer->u.char_buffer;

	num_bytes_already_received = strlen( message_ptr );

	receive_ptr = message_ptr + num_bytes_already_received;

	remaining_buffer_length =
		message_buffer->buffer_length - num_bytes_already_received;

	/* If strlen( message_ptr ) is greater than zero, then we have 
	 * already received a partial message and must try to append
	 * the rest of the message at the end of the existing partial
	 * message.
	 */

	mx_status = mx_socket_getline( client_socket,
					receive_ptr,
					remaining_buffer_length,
					ascii_line_terminators );

	MX_DEBUG(-2,("%s: mx_status.code = %lu, message_ptr = '%s'",
		fname, mx_status.code, message_ptr));

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		/* We have received a complete line from the socket,
		 * so now we can go on to parsing that line.
		 */ 
		break;

	case MXE_NETWORK_CONNECTION_LOST:
		/* If the client has closed the connection, then remove
		 * the socket handler from the socket handler list.
		 */

		mx_status2 = mxsrv_free_client_socket_handler(
				socket_handler, socket_handler_list );

		return mx_status;
		break;

	default:
		return mx_status;
		break;
	}

	MX_DEBUG(-2,("%s: Received command = '%s'", fname, message_ptr ));

	return mx_status;
}

/*--------------------------------------------------------------------------*/

