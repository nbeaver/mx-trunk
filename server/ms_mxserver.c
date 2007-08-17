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
 * Copyright 1999-2007 Illinois Institute of Technology
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

#define NETWORK_DEBUG_TIMING_VERBOSE	FALSE

#define NETWORK_DEBUG_FIELD_NAMES	FALSE

#define NETWORK_DEBUG_HANDLES		FALSE

#define NETWORK_DEBUG_HEADER_LENGTH	FALSE

#define NETWORK_DEBUG_CALLBACKS		FALSE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>

#include "mxconfig.h"
#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_handle.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_net_socket.h"
#include "mx_pipe.h"
#include "mx_array.h"
#include "mx_bit.h"

#include "mx_process.h"
#include "mx_callback.h"
#include "ms_mxserver.h"
#include "ms_security.h"

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

/*
 * If NETWORK_DEBUG_TIMING_VERBOSE is TRUE, force NETWORK_DEBUG_TIMING to TRUE.
 */

#if NETWORK_DEBUG_TIMING_VERBOSE
#  undef NETWORK_DEBUG_TIMING

#  define NETWORK_DEBUG_TIMING  FALSE
#endif

MXSRV_MX_SERVER_SOCKET mxsrv_tcp_server_socket_struct;

#if HAVE_UNIX_DOMAIN_SOCKETS
MXSRV_MX_SERVER_SOCKET mxsrv_unix_server_socket_struct;
#endif

static mx_status_type mxsrv_delete_field_callback( MX_CALLBACK *callback );

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

mx_status_type
mxsrv_free_client_socket_handler( MX_SOCKET_HANDLER *socket_handler,
			MX_SOCKET_HANDLER_LIST *socket_handler_list )
{
	static const char fname[] = "mxsrv_free_client_socket_handler()";

	MX_LIST_HEAD *list_head;
	MX_HANDLE_TABLE *handle_table;
	MX_HANDLE_STRUCT *handle_struct, *handle_struct_array;
	long n;
	unsigned long i, array_size;
	signed long handle;
	MX_CALLBACK *callback;

	n = socket_handler->handler_array_index;

	if ( socket_handler_list != NULL ) {
		socket_handler_list->array[n] = NULL;
	}

	if ( n >= 0 ) {
		mx_info("Client %ld (socket %d) disconnected.",
			n, socket_handler->synchronous_socket->socket_fd);
	}

	list_head = socket_handler->list_head;

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the current database is NULL." );
	}

	/* If this handler had any callbacks, find them and delete them. */

	handle_table = list_head->server_callback_handle_table;

	if ( handle_table != (MX_HANDLE_TABLE *) NULL ) {

		/* Look through the handle table for handles used
		 * by this socket handler.
		 */

		handle_struct_array = handle_table->handle_struct_array;

		if ( handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The handle_struct_array table for the current "
			"database is NULL." );
		}

		array_size = 
			handle_table->block_size * handle_table->num_blocks;

		for ( i = 0; i < array_size; i++ ) {
			handle_struct = &(handle_struct_array[i]);

			handle = handle_struct->handle;
			callback = handle_struct->pointer;

			if ((handle != MX_ILLEGAL_HANDLE) && (callback != NULL))
			{
			    /* Does this callback use the socket handler
			     * that we are in the process of deleting?
			     */

			    if (callback->callback_argument != socket_handler) 
			    {
			    	/* The callback does not use this socket
				 * handler, so skip on to the next callback.
				 */

				continue;
			    }

			    /* If we get here, the callback _does_ use this
			     * socket handler.
			     */

			    /* Delete the handle from the server's callback
			     * handle table.
			     */

			    (void) mx_delete_handle( handle, handle_table );

			    /* If this callback was attached to a record field,
			     * we must delete the callback there as well.
			     */

			    if ( callback->callback_class == MXCBC_FIELD ) {
				(void) mxsrv_delete_field_callback ( callback );
			    } else {
			    	mx_free( callback );
			    }
			}
		}
	}

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
		mx_free( socket_handler->message_buffer );
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

static mx_status_type
mxsrv_delete_field_callback( MX_CALLBACK *callback )
{
	static const char fname[] = "mxsrv_delete_field_callback()";

	MX_RECORD_FIELD *field;
	MX_LIST *callback_list;
	MX_LIST_ENTRY *list_start, *list_entry, *next_list_entry;
	MX_CALLBACK *callback_entry;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for callback %p", fname, callback));

	if ( callback == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CALLBACK pointer passed was NULL.");
	}

	field = callback->u.record_field;

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD_FIELD pointer for callback %p was NULL.",
			callback );
	}

	MX_DEBUG(-2,("%s: field = '%s.%s'",
		fname, field->record->name, field->name));

	/* Find this callback in the record field's callback list. */

	callback_list = field->callback_list;

	if ( callback_list == (MX_LIST *) NULL ) {
		MX_DEBUG(-2,("%s: No callback list installed for field '%s.%s'",
			fname, field->record->name, field->name));

		return MX_SUCCESSFUL_RESULT;
	}

	list_start = callback_list->list_start;

	if ( list_start == (MX_LIST_ENTRY *) NULL ) {
		MX_DEBUG(-2,
	    ("%s: No callbacks installed in callback list %p for field '%s.%s'",
		fname, callback_list, field->record->name, field->name));

		return MX_SUCCESSFUL_RESULT;
	}

	list_entry = list_start;

	do {
		next_list_entry = list_entry->next_list_entry;

		callback_entry = list_entry->list_entry_data;

		if ( callback_entry == callback ) {
			/* We have found the callback entry, so delete it. */

			MX_DEBUG(-2,("%s: Found callback entry.", fname));

			mx_status = mx_list_delete_entry( callback_list,
								list_entry );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_list_entry_destroy( list_entry );

#if 0
			if ( callback_list->list_start == NULL ) {
				mx_list_destroy( callback_list );

				field->callback_list = NULL;
			}
#endif

			return MX_SUCCESSFUL_RESULT;
		}

		list_entry = next_list_entry;

	} while ( list_entry != list_start );

	MX_DEBUG(-2,("%s: Did not find callback entry.", fname));

	return MX_SUCCESSFUL_RESULT;
}

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
		"Illegal server port number %d for server event handler '%s'",
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
				malloc( sizeof(MX_SOCKET_HANDLER) );

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
	socket_handler->network_debug = FALSE;

	strcpy( socket_handler->client_address_string, "" );
	strlcpy( socket_handler->program_name, "mxserver",
					MXU_PROGRAM_NAME_LENGTH );

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
	int saved_errno, connection_allowed;
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

	client_socket = (MX_SOCKET *) malloc( sizeof(MX_SOCKET) );

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
				malloc( sizeof(MX_SOCKET_HANDLER) );

	if ( new_socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		mx_free( client_socket );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Attempt to allocate a new socket handler for new client socket %d failed.",
			client_socket->socket_fd );
	}

	new_socket_handler->list_head = list_head;

	new_socket_handler->network_debug = list_head->network_debug;

	new_socket_handler->handler_array_index = -1;

	new_socket_handler->username[0] = '\0';

	new_socket_handler->program_name[0] = '\0';

	new_socket_handler->process_id = (unsigned long) -1;

	new_socket_handler->data_format = list_head->default_data_format;

#if ( MX_WORDSIZE == 64 )
	new_socket_handler->truncate_64bit_longs = TRUE;
#else
	new_socket_handler->truncate_64bit_longs = FALSE;
#endif
	new_socket_handler->remote_header_length = 0;

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
		mxsrv_check_socket_connection_acl_permissions( record_list,
				new_socket_handler->client_address_string,
				&connection_allowed );

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

	mx_info("Client %d (socket %d) connected from '%s'.",
		i, client_socket->socket_fd,
		new_socket_handler->client_address_string );

	return MX_SUCCESSFUL_RESULT;
}

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
	MX_HRT_TIMING queue_measurement, immediate_measurement;

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
	MX_DEBUG(-2,("socket_handler->callback_socket = %p",
				socket_handler->callback_socket));
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
	 * message field.
	 */

	message_ptr[ message_length ] = '\0';

	if ( mx_client_supports_message_ids(socket_handler) == FALSE ) {

		message_id = 0;     /* This client is using MX 1.4 or before. */
	} else {
		message_id = mx_ntohl( header[ MX_NETWORK_MESSAGE_ID ] );
	}

	socket_handler->last_rpc_message_id = message_id;

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug ) {
		fprintf( stderr, "\nMX NET: CLIENT (socket %d) -> SERVER\n",
				(int) client_socket->socket_fd );

		mx_network_display_message( received_message );
	}
#endif

#if NETWORK_DEBUG
	{
		char message_type_string[40];
		char text_buffer[200];
		size_t buffer_left;

		switch( message_type ) {
		case MX_NETMSG_GET_ARRAY_BY_NAME:
			strcpy( message_type_string, "GET_ARRAY_BY_NAME" );
			break;
		case MX_NETMSG_PUT_ARRAY_BY_NAME:
			strcpy( message_type_string, "PUT_ARRAY_BY_NAME" );
			break;
		case MX_NETMSG_GET_ARRAY_BY_HANDLE:
			strcpy( message_type_string, "GET_ARRAY_BY_HANDLE" );
			break;
		case MX_NETMSG_PUT_ARRAY_BY_HANDLE:
			strcpy( message_type_string, "PUT_ARRAY_BY_HANDLE" );
			break;
		case MX_NETMSG_GET_NETWORK_HANDLE:
			strcpy( message_type_string, "GET_NETWORK_HANDLE" );
			break;
		case MX_NETMSG_GET_FIELD_TYPE:
			strcpy( message_type_string, "GET_FIELD_TYPE" );
			break;
		case MX_NETMSG_SET_CLIENT_INFO:
			strcpy( message_type_string, "SET_CLIENT_INFO" );
			break;
		case MX_NETMSG_GET_OPTION:
			strcpy( message_type_string, "GET_OPTION" );
			break;
		case MX_NETMSG_SET_OPTION:
			strcpy( message_type_string, "SET_OPTION" );
			break;
		case MX_NETMSG_ADD_CALLBACK:
			strcpy( message_type_string, "ADD_CALLBACK" );
			break;
		case MX_NETMSG_DELETE_CALLBACK:
			strcpy( message_type_string, "DELETE_CALLBACK" );
			break;
		default:
			sprintf( message_type_string,
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

		if ( message_length > buffer_left ) {
			strcat( text_buffer, "***'" );
		} else {
			strcat( text_buffer, "'" );
		}
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
					socket_handler->network_debug,
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
					socket_handler->network_debug,
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
			value_ptr += MXU_RECORD_FIELD_NAME_LENGTH;
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
					socket_handler->network_debug,
					MX_NETMSG_UNEXPECTED_ERROR,
					mx_status );
		break;
	}

#if NETWORK_DEBUG_VERBOSE
	MX_DEBUG(-2,("socket_handler->synchronous_socket = %p",
				socket_handler->synchronous_socket));
	MX_DEBUG(-2,("socket_handler->callback_socket = %p",
				socket_handler->callback_socket));
	MX_DEBUG(-2,("socket_handler->handler_array_index = %ld",
				socket_handler->handler_array_index));
	MX_DEBUG(-2,("socket_handler->event_handler = %p",
				socket_handler->event_handler));
#endif

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( immediate_measurement );

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
	} else {
		MX_HRT_RESULTS( recv_measurement, fname, "receiving" );
		MX_HRT_RESULTS( parse_measurement, fname, "parsing" );
		MX_HRT_RESULTS( immediate_measurement, fname, "immediate" );
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
	return mx_status;
}

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
	MX_DEBUG(-2,("socket_handler->callback_socket = %p",
				socket_handler->callback_socket));
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
	MX_DEBUG(-2,("socket_handler->callback_socket = %p",
				socket_handler->callback_socket));
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

mx_status_type
mxsrv_handle_get_array( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	uint32_t *receive_buffer_header;
	uint32_t receive_buffer_header_length;
	uint32_t receive_buffer_message_type, receive_buffer_message_id;
	uint32_t send_buffer_message_type;
	long receive_datatype;
	mx_status_type mx_status, process_mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

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
		receive_datatype = 
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

	/* Get the data from the hardware for this get_array request. */

	process_mx_status = mx_process_record_field( record, record_field,
						MX_PROCESS_GET, NULL );

	mx_status = mxsrv_send_field_value_to_client( socket_handler,
						record, record_field,
						network_message,
						send_buffer_message_type,
						receive_buffer_message_id,
						process_mx_status );

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"final for '%s.%s'", record->name, record_field->name );
#endif

	return mx_status;
}

mx_status_type
mxsrv_send_field_value_to_client( 
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *network_message,
			uint32_t message_type_for_client,
			uint32_t message_id_for_client,
			mx_status_type caller_mx_status )
{
	static const char fname[] = "mxsrv_send_field_value_to_client()";

	char location[ sizeof(fname) + 40 ];
	uint32_t *send_buffer_header;
	char *send_buffer_message;
	long send_buffer_header_length, send_buffer_message_length;
	long send_buffer_message_actual_length;
	size_t num_bytes;

	MX_SOCKET *mx_socket;
	void *pointer_to_value;
	int array_is_dynamically_allocated;
	unsigned long data_format;
	mx_status_type ( *token_constructor )
		(void *, char *, size_t, MX_RECORD *, MX_RECORD_FIELD *);
	mx_status_type mx_status;

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

	send_buffer_header_length = mx_remote_header_length(socket_handler);

	send_buffer_message_length = network_message->buffer_length
					- send_buffer_header_length;

	send_buffer_message_actual_length = 0;

	send_buffer_header = network_message->u.uint32_buffer;;

	send_buffer_message  = network_message->u.char_buffer;
	send_buffer_message += send_buffer_header_length;

	/* What data format do we use to send the response? */

	data_format = socket_handler->data_format;

	/* For MXFT_RECORD and MXFT_INTERFACE fields, the raw value is
	 * a pointer to the data structure, while for MXFT_RECORDTYPE 
	 * fields, the raw value is an MX version dependent 'long'
	 * value that represents the driver type.  None of these are
	 * directly useable by the client, so instead we send back
	 * the ASCII name for the field value.  This is most easily done
	 * by telling the code below to use MX_NETWORK_DATAFMT_ASCII
	 * in its response.
	 */

	if ( (record_field->datatype == MXFT_RECORD)
	  || (record_field->datatype == MXFT_RECORDTYPE)
	  || (record_field->datatype == MXFT_INTERFACE) )
	{
		data_format = MX_NETWORK_DATAFMT_ASCII;
	}

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
				strcpy( send_buffer_message, "" );

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

			num_bytes = 0;

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
					&num_bytes,
					socket_handler->truncate_64bit_longs );

			send_buffer_message_actual_length = (long) num_bytes;
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
					&num_bytes );

			send_buffer_message_actual_length = (long) num_bytes;
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
		 * try to make the buffer larger.  In this case, the variable
		 * 'num_bytes' actually tells you how many bytes would not
		 * fit in the existing buffer.
		 */

		current_length = network_message->buffer_length;

		new_length = current_length + num_bytes;

		mx_status = mx_reallocate_network_buffer(
						network_message, new_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Update some values. */

		send_buffer_header = network_message->u.uint32_buffer;;

		send_buffer_message = network_message->u.char_buffer
						+ send_buffer_header_length;

		send_buffer_message_length += num_bytes;
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

		strncat( send_buffer_message,
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
		size_t buffer_left;
		char *ptr;
		long i, length;
		long max_length = 20;

		switch( data_format ) {
		case MX_NETWORK_DATAFMT_ASCII:
			sprintf( text_buffer,
				"%s: sending response = '", fname );

			buffer_left = sizeof(text_buffer)
					- strlen(text_buffer) - 6;

			strncat( text_buffer,
					send_buffer_message, buffer_left );

			if ( buffer_left < send_buffer_message_length ) {
				strcat( text_buffer, "***'" );
			} else {
				strcat( text_buffer, "'" );
			}
			break;

		case MX_NETWORK_DATAFMT_RAW:
			sprintf( text_buffer,
				"%s: sending response = ", fname );

			ptr = text_buffer + strlen( text_buffer );

			length = send_buffer_message_length;

			if ( length > max_length )
				length = max_length;

			for ( i = 0; i < length; i++ ) {
				sprintf(ptr, "%#02x ", send_buffer_message[i]);

				ptr += 5;
			}
			break;

		default:
			sprintf( text_buffer,
				"%s: sending response in data format %lu.",
				fname, data_format );
			break;
		}

		MX_DEBUG(-2,("%s", text_buffer));
	}
#endif

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
						mx_socket->socket_fd );

		mx_network_display_message( network_message );
	}
#endif

	mx_status = mx_network_socket_send_message( mx_socket,
						-1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		if ( record != NULL ) {
			sprintf( location,
			"%s to client socket %d for record field '%s.%s'",
					fname, mx_socket->socket_fd,
					record->name, record_field->name );
		} else {
			sprintf( location,
			"%s to client socket %d for field '%s'",
					fname, mx_socket->socket_fd,
					record_field->name );
		}

		return mx_error( mx_status.code, location, mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

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

	char *send_buffer_message, *value_buffer;
	uint32_t *send_buffer_header;
	uint32_t send_buffer_header_length, send_buffer_message_length;

	MX_SOCKET *mx_socket;
	MX_RECORD_FIELD_PARSE_STATUS parse_status;
	char token_buffer[500];
	void *pointer_to_value;
	long message_buffer_used, buffer_left;
	unsigned long i, ptr_address, remainder, gap_size;
	int array_is_dynamically_allocated;
	mx_status_type ( *token_parser )
		( void *, char *, MX_RECORD *, MX_RECORD_FIELD *,
		MX_RECORD_FIELD_PARSE_STATUS * );
	mx_status_type mx_status;

	char separators[] = MX_RECORD_FIELD_SEPARATORS;

	receive_buffer_message_type = 0;
	receive_buffer_message_id = 0;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	mx_status = MX_SUCCESSFUL_RESULT;

	mx_socket = socket_handler->synchronous_socket;

	MX_DEBUG( 1,("***** %s invoked for socket %d *****",
		fname, mx_socket->socket_fd));

	if ( record_field->flags & MXFF_VARARGS ) {
		array_is_dynamically_allocated = TRUE;
	} else {
		array_is_dynamically_allocated = FALSE;
	}

	/* The do...while(0) loop below is just a trick to make it easy
	 * to jump to the end of this block of code.
	 */

	do {
		if ( record == (MX_RECORD *) NULL ) {
			mx_status =  mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );

			break;		/* Exit the do...while(0) loop. */
		}

		if ( record_field == (MX_RECORD_FIELD *) NULL ) {
			mx_status =  mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD_FIELD pointer passed was NULL." );

			break;		/* Exit the do...while(0) loop. */
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
			receive_datatype = 
		      mx_ntohl(receive_buffer_header[ MX_NETWORK_DATA_TYPE ]);

			receive_buffer_message_id =
		      mx_ntohl(receive_buffer_header[ MX_NETWORK_MESSAGE_ID ]);
		}

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
					NULL,
					socket_handler->truncate_64bit_longs );
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

			ptr_address = (unsigned long) value_buffer;

			remainder = ptr_address % 4;

			if ( remainder != 0 ) {
				gap_size = 4 - remainder;

				for ( i = 0; i < gap_size; i++ ) {
					value_buffer[i] = '\0';
				}

				value_buffer += gap_size;

				buffer_left -= gap_size;
			}

			MX_DEBUG( 2,
			("%s: ptr_address = %#lx, value_buffer = %p",
				fname, ptr_address, value_buffer));

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
					NULL );
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
		strcpy( send_buffer_message, "" );
	} else {
		strcpy( send_buffer_message, mx_status.message );
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

		sprintf( text_buffer, "%s: sending response = '", fname );

		buffer_left = sizeof(text_buffer) - strlen(text_buffer) - 6;

		strncat( text_buffer, send_buffer_message, buffer_left );

		if ( buffer_left < send_buffer_message_length ) {
			strcat( text_buffer, "***'" );
		} else {
			strcat( text_buffer, "'" );
		}

		MX_DEBUG(-2,("%s", text_buffer));
	}
#endif

#if NETWORK_DEBUG_MESSAGES
	if ( socket_handler->network_debug ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
						mx_socket->socket_fd );

		mx_network_display_message( network_message );
	}
#endif

	mx_status = mx_network_socket_send_message( mx_socket,
						-1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s to client socket %d",
				fname, mx_socket->socket_fd );

		return mx_error( mx_status.code, location, mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for '%s.%s'", record->name, record_field->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

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
					socket_handler->network_debug,
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

	send_buffer_header_length = mx_remote_header_length(socket_handler);

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
	if ( socket_handler->network_debug ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
				socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message );
	}
#endif

	/* Send the record field handle back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location, mx_status.message );
	}

	return MX_SUCCESSFUL_RESULT;
}

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
	if ( socket_handler->network_debug ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
						mx_socket->socket_fd );

		mx_network_display_message( network_message );
	}
#endif

	/* Send the field type information back to the client. */

	mx_status = mx_network_socket_send_message( mx_socket,
						-1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s to client socket %d",
				fname, mx_socket->socket_fd );

		return mx_error( mx_status.code, location, mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

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

	header_length = mx_remote_header_length(socket_handler);

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
	if ( socket_handler->network_debug ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
				socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message );
	}
#endif

	/* Send the success message back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location, mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mxsrv_handle_get_option( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_get_option()";

	char location[ sizeof(fname) + 40 ];
	uint32_t *send_buffer_header, *send_buffer_message;
	uint32_t send_buffer_header_length;
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
		if ( socket_handler->truncate_64bit_longs ) {
			option_value = FALSE;
		} else {
			option_value = TRUE;
		}
#endif /* MX_WORDSIZE == 64 */

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

		sprintf( error_message, "Illegal option number %#lx",
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
	if ( socket_handler->network_debug ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
				socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message );
	}
#endif

	/* Send the option information back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location, mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

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
			socket_handler->truncate_64bit_longs = TRUE;
		} else {
			socket_handler->truncate_64bit_longs = FALSE;
		}
#endif /* MX_WORDSIZE == 64 */

		MX_DEBUG( 2,("%s: option_value = %d, truncate_64bit_longs = %d",
			fname, (int) option_value,
			(int) socket_handler->truncate_64bit_longs ));
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

	/* Construct the header of the message. */

	send_buffer_header[ MX_NETWORK_MAGIC ]
				= mx_htonl( MX_NETWORK_MAGIC_VALUE );

	send_buffer_header[ MX_NETWORK_MESSAGE_TYPE ]
		= mx_htonl( mx_server_response(MX_NETMSG_SET_OPTION) );

	send_buffer_header[ MX_NETWORK_HEADER_LENGTH ]
				= mx_htonl( send_buffer_header_length );

	/* Construct the body of the message. */

	if ( illegal_option_number ) {
		sprintf( send_buffer_char_message,
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

			sprintf( send_buffer_char_message,
				"Server '%s' does not support the 64-bit longs "
				"option, since it is not a 64-bit computer.",
					list_head->hostname );
			break;
		default:
			sprintf( send_buffer_char_message,
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
	if ( socket_handler->network_debug ) {
		fprintf( stderr, "\nMX NET: SERVER -> CLIENT (socket %d)\n",
				socket_handler->synchronous_socket->socket_fd );

		mx_network_display_message( network_message );
	}
#endif

	/* Send the option information back to the client. */

	mx_status = mx_network_socket_send_message(
		socket_handler->synchronous_socket, -1.0, network_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s to client socket %d",
			fname, socket_handler->synchronous_socket->socket_fd );

		return mx_error( mx_status.code, location, mx_status.message );
	}

	MX_DEBUG( 1,("***** %s successful *****", fname));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxsrv_record_field_callback( MX_CALLBACK *callback, void *argument )
{
	static const char fname[] = "mxsrv_record_field_callback()";

	MX_SOCKET_HANDLER *socket_handler;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	mx_bool_type send_value_changed_callback;
	mx_status_type mx_status;

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s (%p): callback = %p, argument = %p",
		fname, mxsrv_record_field_callback, callback, argument));
#endif

	if ( callback == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CALLBACK pointer passed was NULL." );
	}

	socket_handler = (MX_SOCKET_HANDLER *) argument;

	if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		return mx_error( MXE_INVALID_CALLBACK, fname,
		"The MX_SOCKET_HANDLER pointer passed was NULL." );
	}

	message_buffer = socket_handler->message_buffer;

	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_INVALID_CALLBACK, fname,
		"The MX_NETWORK_MESSAGE_BUFFER corresponding to "
		"the socket handler passed is NULL." );
	}

	if ( callback->callback_class != MXCBC_FIELD ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal callback class %lu used.  "
		"Only MXCBC_FIELD (%d) callbacks are allowed here.",
			callback->callback_class, MXCBC_FIELD );
	}

	if ( callback->callback_type != MXCBT_VALUE_CHANGED ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal callback type %lu used.  Currently, "
		"MXCBT_VALUE_CHANGED (%d) callbacks are allowed here.",
			callback->callback_class, MXCBT_VALUE_CHANGED );
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

	/* Do we need to get a new field value? */

	if ( callback->get_new_value ) {
		/* Process the record field to get the new value. */

#if NETWORK_DEBUG_CALLBACKS
		MX_DEBUG(-2,("%s: Processing '%s.%s'",
			fname, record->name, record_field->name));
#endif

		mx_status = mx_process_record_field( record, record_field,
						MX_PROCESS_GET,
						&send_value_changed_callback );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		/* Send the current value of the field. */

		send_value_changed_callback = TRUE;
	}

	/* If we get here, see if we should send a value changed callback
	 * to the client.
	 */

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s: send_value_changed_callback = %d",
		fname, send_value_changed_callback));
#endif

	if ( send_value_changed_callback ) {

#if NETWORK_DEBUG_CALLBACKS
		MX_DEBUG(-2,
		("%s: invoking mxsrv_send_field_value_to_client for '%s.%s'",
			fname, record->name, record_field->name ));
#endif

		mx_status = mxsrv_send_field_value_to_client( socket_handler,
						record, record_field,
						message_buffer,
					mx_server_response(MX_NETMSG_CALLBACK),
						callback->callback_id,
						mx_status );
#if NETWORK_DEBUG_CALLBACKS
		MX_DEBUG(-2,
		("%s: mxsrv_send_field_value_to_client status = %ld",
			fname, mx_status.code));
#endif
	}

	return mx_status;
}

mx_status_type
mxsrv_handle_add_callback( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *field,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_add_callback()";

	uint32_t *uint32_header, *uint32_message;
	uint32_t message_id;
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

	message_id = mx_ntohl( uint32_header[MX_NETWORK_MESSAGE_ID] );

	uint32_message = uint32_header + ( header_length / sizeof(uint32_t) );

	record_handle = mx_ntohl( uint32_message[0] );
	field_handle  = mx_ntohl( uint32_message[1] );
	callback_type = mx_ntohl( uint32_message[2] );

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s: (%lu,%lu) callback_type = %lu",
		fname, record_handle, field_handle, callback_type ));
#endif

	/* Add the callback to the list of callbacks. */

	mx_status = mx_local_field_add_callback( field,
					callback_type,
					mxsrv_record_field_callback,
					socket_handler,
					&callback_object );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( callback_object == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"For some reason, mx_field_add_callback() returned a NULL"
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

mx_status_type
mxsrv_handle_delete_callback( MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *network_message )
{
	static const char fname[] = "mxsrv_handle_delete_callback()";

	return mx_error(MXE_NOT_YET_IMPLEMENTED, fname, "Not yet implemented.");
}

mx_status_type
mxsrv_process_callbacks( MX_LIST_HEAD *list_head,
				MX_PIPE *callback_pipe )
{
	static const char fname[] = "mxsrv_process_callbacks()";

	MX_HANDLE_TABLE *handle_table;
	MX_HANDLE_STRUCT *handle_struct;
	signed long handle;
	MX_CALLBACK *callback;
	MX_CALLBACK_MESSAGE *callback_message;
	mx_status_type (*cb_function)( void * );
	unsigned long i, array_size;
	size_t bytes_read;
	mx_status_type mx_status;

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s (%p)invoked for list head %p",
		fname, mxsrv_process_callbacks, list_head));
#endif

	/* Read the next message from the callback pipe. */

	mx_status = mx_pipe_read( callback_pipe,
				(char *) &callback_message,
				sizeof(MX_CALLBACK_MESSAGE *),
				&bytes_read );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_read != sizeof(MX_CALLBACK_MESSAGE *) ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to read an MX_CALLBACK_MESSAGE pointer from "
		"MX callback pipe %p failed, since the number of bytes "
		"read (%ld) is less than the expected size of %ld.",
			callback_pipe, (long) bytes_read,
			(long) sizeof(MX_CALLBACK_MESSAGE *) );
	}

#if NETWORK_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s: callback_message = %p", fname, callback_message));

	MX_DEBUG(-2,("%s: callback_type = %ld",
		fname, callback_message->callback_type));
#endif

	/* We do different things depending on the type of callback message. */

	switch( callback_message->callback_type ) {
	case MXCBT_POLL:
		/* Poll all value changed callback handlers. */

#if NETWORK_DEBUG_CALLBACKS
		MX_DEBUG(-2,
		("%s: Poll all value changed callback handlers.", fname));
#endif

		if ( list_head != callback_message->list_head ) {
			return mx_error( MXE_IPC_IO_ERROR, fname,
			"The callback has a different address %p for the "
			"list head than the server's list head address %p.",
				callback_message->list_head,
				list_head );
		}

		handle_table = list_head->server_callback_handle_table;

		if ( handle_table == (MX_HANDLE_TABLE *) NULL ) {
#if NETWORK_DEBUG_CALLBACKS
			MX_DEBUG(-2,
			("%s: No callback handle table installed.", fname));
#endif

			return MX_SUCCESSFUL_RESULT;
		}

		if ( handle_table->handle_struct_array
				== (MX_HANDLE_STRUCT *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"handle_struct_array pointer for callback handle "
			"table is NULL." );
		}

		array_size = handle_table->block_size
				* handle_table->num_blocks;

		for ( i = 0; i < array_size; i++ ) {
			handle_struct = &(handle_table->handle_struct_array[i]);

			handle   = handle_struct->handle;
			callback = handle_struct->pointer;

#if 0
			MX_DEBUG(-2,
	    ("%s: handle_struct_array[%lu] = %p, handle = %ld, callback = %p",
				fname, i, handle_struct, handle, callback));
#endif

			if ( ( handle == MX_ILLEGAL_HANDLE )
			  || ( callback == NULL ) )
			{
				/* Skip unused handles. */

				continue;
			}

#if 0
			MX_DEBUG(-2,("%s: callback->callback_function = %p",
				fname, callback->callback_function));
#endif

			if ( callback->callback_function != NULL ) {
#if NETWORK_DEBUG_CALLBACKS
				MX_DEBUG(-2,
	("%s: calling mx_invoke_callback(): i = %lu, handle = %ld, callback = %p, callback->callback_function = %p",
					fname, i, handle, callback,
					callback->callback_function));
#endif

				mx_status = mx_invoke_callback(callback, TRUE);
			}
		}
		break;
	case MXCBT_MOTOR_BACKLASH:
		mx_status = mx_motor_backlash_callback( callback_message );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXCBT_FUNCTION:
		cb_function = callback_message->u.function.callback_function;

		if ( cb_function == NULL ) {
			mx_warning(
		  "Function callback seen by %s with a NULL function pointer.",
				fname );
		} else {
			mx_status = (*cb_function)( callback_message );
		}
		break;
	default:
		mx_warning( "Unrecognized callback type %ld seen in %s",
			callback_message->callback_type, fname );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

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
