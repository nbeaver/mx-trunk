/*
 * Name:     n_unix.c
 *
 * Purpose:  Client interface to an MX Unix domain socket server.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2008, 2010-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "mx_osdef.h"

#if HAVE_UNIX_DOMAIN_SOCKETS

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_net_socket.h"
#include "n_unix.h"

MX_RECORD_FUNCTION_LIST mxn_unix_server_record_function_list = {
	NULL,
	mxn_unix_server_create_record_structures,
	mxn_unix_server_finish_record_initialization,
	mxn_unix_server_delete_record,
	NULL,
	mxn_unix_server_open,
	mxn_unix_server_close,
	NULL,
	mxn_unix_server_resynchronize
};

MX_NETWORK_SERVER_FUNCTION_LIST
		mxn_unix_server_network_server_function_list = {
	mxn_unix_server_receive_message,
	mxn_unix_server_send_message,
	mxn_unix_server_connection_is_up,
	mxn_unix_server_reconnect_if_down,
	mxn_unix_server_message_is_available
};

MX_RECORD_FIELD_DEFAULTS mxn_unix_server_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_SERVER_STANDARD_FIELDS,
	MXN_UNIX_SERVER_STANDARD_FIELDS
};

long mxn_unix_server_num_record_fields
		= sizeof( mxn_unix_server_record_field_defaults )
			/ sizeof( mxn_unix_server_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxn_unix_server_rfield_def_ptr
			= &mxn_unix_server_record_field_defaults[0];

MX_EXPORT mx_status_type
mxn_unix_server_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxn_unix_server_create_record_structures()";

	MX_NETWORK_SERVER *network_server;
	MX_UNIX_SERVER *unix_server;
	mx_status_type mx_status;

	/* Allocate memory for the necessary structures. */

	network_server = (MX_NETWORK_SERVER *)
				malloc( sizeof(MX_NETWORK_SERVER) );

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_SERVER structure." );
	}

	unix_server = (MX_UNIX_SERVER *) malloc( sizeof(MX_UNIX_SERVER) );

	if ( unix_server == (MX_UNIX_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_UNIX_SERVER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = network_server;
	record->record_type_struct = unix_server;
	record->class_specific_function_list
			= &mxn_unix_server_network_server_function_list;

	network_server->server_supports_network_handles = TRUE;
	network_server->network_handles_are_valid = TRUE;

	network_server->remote_header_length = 0;
	network_server->last_data_type = 0;

	network_server->record = record;

	mx_status = mx_allocate_network_buffer(
			&(network_server->message_buffer),
			MXU_NETWORK_INITIAL_MESSAGE_BUFFER_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	network_server->network_field_array_block_size = 100L;
	network_server->num_network_fields = 0;
	network_server->network_field_array = NULL;

	network_server->callback_list = NULL;

	MX_DEBUG( 2,("%s: MX_WORDSIZE = %d, MX_PROGRAM_MODEL = %#x",
		fname, MX_WORDSIZE, MX_PROGRAM_MODEL));

	network_server->use_64bit_network_longs = FALSE;

	network_server->connection_status = 0;

	network_server->last_rpc_message_id = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_unix_server_finish_record_initialization( MX_RECORD *record )
{
	MX_UNIX_SERVER *unix_server;

	unix_server = (MX_UNIX_SERVER *) record->record_type_struct;

	unix_server->socket = NULL;

	unix_server->first_attempt = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_unix_server_delete_record( MX_RECORD *record )
{
	MX_NETWORK_SERVER *network_server;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	network_server = (MX_NETWORK_SERVER *) record->record_class_struct;

	if ( network_server != NULL ) {
		if ( network_server->message_buffer != NULL ) {
			mx_free_network_buffer(network_server->message_buffer);

			network_server->message_buffer = NULL;
		}

		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_unix_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_unix_server_open()";

	MX_LIST_HEAD *list_head;
	MX_NETWORK_SERVER *network_server;
	MX_UNIX_SERVER *unix_server;
	MX_SOCKET *server_socket;
	unsigned long version, flags, requested_data_format, socket_flags;
	long mx_status_code;
	char null_byte;
	mx_bool_type quiet_open;
	mx_status_type mx_status;

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			record->name );
	}

	network_server = (MX_NETWORK_SERVER *) record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	unix_server = (MX_UNIX_SERVER *) record->record_type_struct;

	if ( unix_server == (MX_UNIX_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UNIX_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
	    network_server->server_flags |= MXF_NETWORK_SERVER_DEBUG_SUMMARY;
	}
	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
	    network_server->server_flags |= MXF_NETWORK_SERVER_DEBUG_VERBOSE;
	}


	flags = network_server->server_flags;

	if ( flags & MXF_NETWORK_SERVER_DISABLED ) {
		return mx_error( MXE_RECORD_DISABLED_BY_USER, fname,
	"Connections to MX server '%s' have been disabled in the MX database.",
			record->name );
	}

	if ( flags & MXF_NETWORK_SERVER_QUIET_RECONNECTION ) {
		if ( network_server->connection_status & MXCS_CONNECTION_LOST )
		{
			quiet_open = TRUE;
		} else {
			quiet_open = FALSE;
		}
	} else {
		quiet_open = FALSE;
	}

	if ( quiet_open ) {
		socket_flags = MXF_SOCKET_QUIET_CONNECTION;
	} else {
		socket_flags = 0;
	}

	mx_status = mx_unix_socket_open_as_client( &server_socket,
					unix_server->pathname, socket_flags,
					MX_SOCKET_DEFAULT_BUFFER_SIZE );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		unix_server->socket = server_socket;
		network_server->connection_status |= MXCS_CONNECTED;

		if ( network_server->connection_status & MXCS_CONNECTION_LOST )
		{
			network_server->connection_status |= MXCS_RECONNECTED;
		}

		network_server->connection_status &= (~MXCS_CONNECTION_LOST);
		break;

	case MXE_NETWORK_IO_ERROR:
		unix_server->socket = NULL;
		network_server->connection_status &= (~MXCS_CONNECTED);

		if ( quiet_open ) {
			mx_status_code = MXE_NETWORK_IO_ERROR | MXE_QUIET;
		} else {
			mx_status_code = MXE_NETWORK_IO_ERROR;
		}

		return mx_error( mx_status_code, fname,
"The MX server at socket '%s' on the local computer is either not running "
"or is not working correctly.  "
"You can try to fix this by restarting the MX server.",
			unix_server->pathname );

	default:
		unix_server->socket = NULL;
		network_server->connection_status &= (~MXCS_CONNECTED);

		return mx_error( mx_status.code, fname,
"An unexpected error occurred while trying to connect to the MX server "
"at socket '%s' on the local computer.", unix_server->pathname);

	}

	/* Set the socket to non-blocking mode if requested. */

	if ( flags & MXF_NETWORK_SERVER_BLOCKING_IO ) {

		network_server->timeout = -1.0;
	} else {
		mx_status = mx_socket_set_non_blocking_mode( server_socket,
								TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		network_server->timeout = 5.0;
	}

	/* Transmit a single null byte to the remote server.  For Unix domain
	 * sockets, this has the side effect of sending unforgeable user
	 * credentials to the remote server.
	 */

	null_byte = '\0';

	mx_status = mx_socket_send( unix_server->socket, &null_byte, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out whether or not the server supports message IDs. */

	mx_status = mx_network_server_discover_header_length( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the user has requested a particular data format. */

	if ( flags & MXF_NETWORK_SERVER_USE_ASCII_FORMAT ) {
		requested_data_format = MX_NETWORK_DATAFMT_ASCII;
	} else
	if ( flags & MXF_NETWORK_SERVER_USE_RAW_FORMAT ) {
		requested_data_format = MX_NETWORK_DATAFMT_RAW;
	} else
	if ( flags & MXF_NETWORK_SERVER_USE_XDR_FORMAT ) {
		requested_data_format = MX_NETWORK_DATAFMT_XDR;
	} else {
		/* The default is automatic configuration. */

		requested_data_format = MX_NETWORK_DATAFMT_NEGOTIATE;
	}

	mx_status = mx_network_request_data_format( record,
						requested_data_format );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the MX version number for the remote server. */

	network_server->remote_mx_version = 0UL;

	mx_status = mx_get_by_name( record, "mx_database.mx_version",
					MXFT_ULONG, &version );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	network_server->remote_mx_version = version;

	/* If the remote MX server is version 1.5.5 or newer, then tell
	 * it what our version is.
	 */

	if ( network_server->remote_mx_version >= 1005005L ) {
		mx_status = mx_network_send_client_version( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Tell the server who we are in an insecure manner. */

	mx_status = mx_set_client_info( record,
				list_head->username, list_head->program_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the user has requested that 64-bit long integers be
	 * sent and received using the full 64-bit resolution.
	 */

	if ( flags & MXF_NETWORK_SERVER_USE_64BIT_LONGS ) {
		mx_status = mx_network_request_64bit_longs( record, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_unix_server_close( MX_RECORD *record )
{
	static const char fname[] = "mxn_unix_server_close()";

	MX_NETWORK_SERVER *network_server;
	MX_UNIX_SERVER *unix_server;
	MX_SOCKET *server_socket;
	mx_status_type mx_status;

	network_server = (MX_NETWORK_SERVER *) record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	unix_server = (MX_UNIX_SERVER *) record->record_type_struct;

	if ( unix_server == (MX_UNIX_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UNIX_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	server_socket = unix_server->socket;

	if ( server_socket != NULL ) {
		(void) mx_network_mark_handles_as_invalid( record );

		mx_status = mx_socket_close( server_socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Force off the "connected" and "reconnected" bits. */

	network_server->connection_status
				&= ~(MXCS_CONNECTED | MXCS_RECONNECTED);

	unix_server->socket = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_unix_server_resynchronize( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mxn_unix_server_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxn_unix_server_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxn_unix_server_receive_message( MX_NETWORK_SERVER *network_server,
				MX_NETWORK_MESSAGE_BUFFER *message_buffer )
{
	static const char fname[] = "mxn_unix_server_receive_message()";

	MX_UNIX_SERVER *unix_server;
	char location[ sizeof(fname) + MXU_HOSTNAME_LENGTH + 20 ];
	mx_status_type mx_status;

	unix_server = (MX_UNIX_SERVER *)
				network_server->record->record_type_struct;

	if ( unix_server == (MX_UNIX_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UNIX_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	mx_status = mx_network_socket_receive_message( unix_server->socket,
				      network_server->timeout, message_buffer );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s from server '%s'",
			fname, network_server->record->name );

		if ( ( mx_status.code == MXE_NETWORK_CONNECTION_LOST )
		  || ( mx_status.code == MXE_NETWORK_IO_ERROR )
		  || ( mx_status.code == MXE_TIMED_OUT ) )
		{
			if ( unix_server->socket != NULL ) {
				(void) mx_network_mark_handles_as_invalid(
						network_server->record );

				(void) mx_socket_close( unix_server->socket );
			}
			unix_server->socket = NULL;

			network_server->connection_status
						= MXCS_CONNECTION_LOST;
		}

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_unix_server_send_message(MX_NETWORK_SERVER *network_server,
				MX_NETWORK_MESSAGE_BUFFER *message_buffer )
{
	static const char fname[] = "mxn_unix_server_send_message()";

	MX_UNIX_SERVER *unix_server;
	char location[ sizeof(fname) + MXU_HOSTNAME_LENGTH + 20 ];
	mx_status_type mx_status;

	unix_server = (MX_UNIX_SERVER *)
				network_server->record->record_type_struct;

	if ( unix_server == (MX_UNIX_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UNIX_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	mx_status = mx_network_socket_send_message( unix_server->socket,
							network_server->timeout,
							message_buffer );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s to server '%s'",
			fname, network_server->record->name );

		if ( ( mx_status.code == MXE_NETWORK_CONNECTION_LOST )
		  || ( mx_status.code == MXE_NETWORK_IO_ERROR )
		  || ( mx_status.code == MXE_TIMED_OUT ) )
		{
			if ( unix_server->socket != NULL ) {
				(void) mx_network_mark_handles_as_invalid(
						network_server->record );

				(void) mx_socket_close( unix_server->socket );
			}
			unix_server->socket = NULL;

			network_server->connection_status
						= MXCS_CONNECTION_LOST;
		}

		return mx_error( mx_status.code, location,
					"%s", mx_status.message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_unix_server_connection_is_up( MX_NETWORK_SERVER *network_server,
					mx_bool_type *connection_is_up )
{
	static const char fname[] = "mxn_unix_server_connection_is_up()";

	MX_UNIX_SERVER *unix_server;

	unix_server = (MX_UNIX_SERVER *)
			network_server->record->record_type_struct;

	if ( unix_server == (MX_UNIX_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UNIX_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	if ( unix_server->socket == NULL ) {
		network_server->connection_status &= (~MXCS_CONNECTED);

		*connection_is_up = FALSE;
	} else {
		network_server->connection_status |= MXCS_CONNECTED;

		*connection_is_up = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_unix_server_reconnect_if_down( MX_NETWORK_SERVER *network_server )
{
	static const char fname[] = "mxn_unix_server_reconnect_if_down()";

	MX_UNIX_SERVER *unix_server;
	unsigned long flags;
	mx_bool_type quiet_reconnection;
	mx_status_type mx_status;

	unix_server = (MX_UNIX_SERVER *)
			network_server->record->record_type_struct;

	if ( unix_server == (MX_UNIX_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UNIX_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	flags = network_server->server_flags;

	if ( flags & MXF_NETWORK_SERVER_DISABLED ) {
		return mx_error( MXE_RECORD_DISABLED_BY_USER, fname,
	"Connections to MX server '%s' have been disabled in the MX database.",
			network_server->record->name );
	}

	if ( flags & MXF_NETWORK_SERVER_QUIET_RECONNECTION ) {
		quiet_reconnection = TRUE;
	} else {
		quiet_reconnection = FALSE;
	}

	if ( unix_server->socket != NULL ) {
		network_server->connection_status |= MXCS_CONNECTED;
	} else {
		network_server->connection_status &= (~MXCS_CONNECTED);

		network_server->connection_status |= MXCS_CONNECTION_LOST;

		/* The connection went away for some reason, so should we
		 * try to reconnect?
		 */

		network_server->network_handles_are_valid = TRUE;

		if ( flags & MXF_NETWORK_SERVER_NO_AUTO_RECONNECT ) {
			return mx_error(
			(MXE_NETWORK_IO_ERROR | MXE_QUIET), fname,
			"Server '%s' at '%s' is not connected.",
				network_server->record->name,
				unix_server->pathname );
		} else
		if ( quiet_reconnection == FALSE ) {
			mx_info(
			"*** Reconnecting to server '%s' at '%s'.",
				network_server->record->name,
				unix_server->pathname );
		}

		mx_status = mxn_unix_server_open( network_server->record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		network_server->connection_status =
					MXCS_CONNECTED | MXCS_RECONNECTED;

		if ( quiet_reconnection ) {
			mx_info( "*** Reconnected to server '%s' at '%s'.",
				network_server->record->name,
				unix_server->pathname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_unix_server_message_is_available( MX_NETWORK_SERVER *network_server,
					mx_bool_type *message_is_available )
{
	static const char fname[] = "mxn_unix_server_message_is_available()";

	MX_UNIX_SERVER *unix_server;
	long num_bytes_available;
	mx_status_type mx_status;

	unix_server = (MX_UNIX_SERVER *)
			network_server->record->record_type_struct;

	if ( unix_server == (MX_UNIX_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UNIX_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	mx_status = mx_socket_num_input_bytes_available( unix_server->socket,
							&num_bytes_available );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NETWORK_CONNECTION_LOST:
		*message_is_available = FALSE;

		mx_info("*** Connection lost to server '%s' at '%s'.",
			network_server->record->name,
			unix_server->pathname );

		return mx_status;
		break;
	default:
		*message_is_available = FALSE;

		return mx_status;
		break;
	}

	if ( num_bytes_available == 0 ) {
		*message_is_available = FALSE;
	} else {
		*message_is_available = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_UNIX */

