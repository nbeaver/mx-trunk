/*
 * Name:     n_tcpip.c
 *
 * Purpose:  Client interface to an MX TCP/IP server.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "mx_osdef.h"

#if HAVE_TCPIP

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_net_socket.h"
#include "n_tcpip.h"

MX_RECORD_FUNCTION_LIST mxn_tcpip_server_record_function_list = {
	NULL,
	mxn_tcpip_server_create_record_structures,
	mxn_tcpip_server_finish_record_initialization,
	mxn_tcpip_server_delete_record,
	NULL,
	NULL,
	NULL,
	mxn_tcpip_server_open,
	mxn_tcpip_server_close,
	NULL,
	mxn_tcpip_server_resynchronize
};

MX_NETWORK_SERVER_FUNCTION_LIST
		mxn_tcpip_server_network_server_function_list = {
	mxn_tcpip_server_receive_message,
	mxn_tcpip_server_send_message,
	mxn_tcpip_server_connection_is_up,
	mxn_tcpip_server_reconnect_if_down
};

MX_RECORD_FIELD_DEFAULTS mxn_tcpip_server_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_SERVER_STANDARD_FIELDS,
	MXN_TCPIP_SERVER_STANDARD_FIELDS
};

long mxn_tcpip_server_num_record_fields
		= sizeof( mxn_tcpip_server_record_field_defaults )
			/ sizeof( mxn_tcpip_server_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxn_tcpip_server_rfield_def_ptr
			= &mxn_tcpip_server_record_field_defaults[0];

MX_EXPORT mx_status_type
mxn_tcpip_server_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxn_tcpip_server_create_record_structures()";

	MX_NETWORK_SERVER *network_server;
	MX_TCPIP_SERVER *tcpip_server;
	mx_status_type mx_status;

	/* Allocate memory for the necessary structures. */

	network_server = (MX_NETWORK_SERVER *)
				malloc( sizeof(MX_NETWORK_SERVER) );

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_SERVER structure." );
	}

	tcpip_server = (MX_TCPIP_SERVER *) malloc( sizeof(MX_TCPIP_SERVER) );

	if ( tcpip_server == (MX_TCPIP_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TCPIP_SERVER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = network_server;
	record->record_type_struct = tcpip_server;
	record->class_specific_function_list
			= &mxn_tcpip_server_network_server_function_list;

	network_server->server_supports_network_handles = TRUE;
	network_server->network_handles_are_valid = TRUE;

	network_server->record = record;

	mx_status = mx_allocate_network_buffer(
			&(network_server->message_buffer),
			MXU_NETWORK_INITIAL_MESSAGE_BUFFER_LENGTH );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	network_server->network_field_array_block_size = 100L;
	network_server->num_network_fields = 0;
	network_server->network_field_array = NULL;

	MX_DEBUG( 2,("%s: MX_WORDSIZE = %d, MX_PROGRAM_MODEL = %#x",
		fname, MX_WORDSIZE, MX_PROGRAM_MODEL));

#if ( MX_WORDSIZE == 64 )
	network_server->truncate_64bit_longs = TRUE;
#else
	network_server->truncate_64bit_longs = FALSE;
#endif

	network_server->last_rpc_message_id = 0;

	network_server->last_callback_message_id
			= MX_NETWORK_MESSAGE_IS_CALLBACK;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_tcpip_server_finish_record_initialization( MX_RECORD *record )
{
	MX_TCPIP_SERVER *tcpip_server;

	tcpip_server = (MX_TCPIP_SERVER *) record->record_type_struct;

	tcpip_server->socket = NULL;

	tcpip_server->first_attempt = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_tcpip_server_delete_record( MX_RECORD *record )
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
mxn_tcpip_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_tcpip_server_open()";

	MX_LIST_HEAD *list_head;
	MX_NETWORK_SERVER *network_server;
	MX_TCPIP_SERVER *tcpip_server;
	MX_SOCKET *server_socket;
	unsigned long version, flags, requested_data_format;
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

	tcpip_server = (MX_TCPIP_SERVER *) record->record_type_struct;

	if ( tcpip_server == (MX_TCPIP_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TCPIP_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	flags = network_server->server_flags;

	if ( flags & MXF_NETWORK_SERVER_DISABLED ) {
		return mx_error( MXE_RECORD_DISABLED_BY_USER, fname,
	"Connections to MX server '%s' have been disabled in the MX database.",
			record->name );
	}

	mx_status = mx_tcp_socket_open_as_client( &server_socket,
			tcpip_server->hostname, tcpip_server->port, 0,
			MX_SOCKET_DEFAULT_BUFFER_SIZE );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		tcpip_server->socket = server_socket;
		break;

	case MXE_NETWORK_IO_ERROR:
		tcpip_server->socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"The MX server at port %ld on the computer '%s' is either not running "
"or is not working correctly.  "
"You can try to fix this by restarting the MX server.",
			tcpip_server->port, tcpip_server->hostname );

	default:
		tcpip_server->socket = NULL;

		return mx_error( mx_status.code, fname,
"An unexpected error occurred while trying to connect to the MX server "
"at port %ld on the computer '%s'.",
			tcpip_server->port, tcpip_server->hostname);

	}

	/* Warn about the use of an obsolete test flag. */

	if ( flags & 0x10000000 ) {
		mx_warning(
		"Flag 0x10000000 for setting non-blocking I/O for server '%s' "
		"is obsolete.  Non-blocking I/O is now the default.",
			network_server->record->name );
	}

	/* Set the socket to non-blocking mode if desired. */

	if ( flags & MXF_NETWORK_SERVER_BLOCKING_IO ) {

		network_server->timeout = -1.0;
	} else {
		mx_status = mx_socket_set_non_blocking_mode( server_socket,
								TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		network_server->timeout = 5.0;
	}

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

	mx_status = mx_set_client_info( record,
				list_head->username, list_head->program_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the user has requested that 64-bit long integers be
	 * sent and received using the full 64-bit resolution.
	 */

	if ( flags & MXF_NETWORK_SERVER_USE_64BIT_LONGS ) {
		mx_status = mx_network_request_64bit_longs( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_tcpip_server_close( MX_RECORD *record )
{
	MX_TCPIP_SERVER *tcpip_server;
	MX_SOCKET *server_socket;
	mx_status_type mx_status;

	tcpip_server = (MX_TCPIP_SERVER *) record->record_type_struct;

	server_socket = tcpip_server->socket;

	if ( server_socket != NULL ) {
		(void) mx_network_mark_handles_as_invalid( record );

		mx_status = mx_socket_close( server_socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	tcpip_server->socket = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_tcpip_server_resynchronize( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mxn_tcpip_server_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxn_tcpip_server_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxn_tcpip_server_receive_message( MX_NETWORK_SERVER *network_server,
				MX_NETWORK_MESSAGE_BUFFER *message_buffer )
{
	static const char fname[] = "mxn_tcpip_server_receive_message()";

	MX_TCPIP_SERVER *tcpip_server;
	char location[ sizeof(fname) + MXU_HOSTNAME_LENGTH + 20 ];
	mx_status_type mx_status;

	tcpip_server = (MX_TCPIP_SERVER *)
				network_server->record->record_type_struct;

	if ( tcpip_server == (MX_TCPIP_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TCPIP_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	mx_status = mx_network_socket_receive_message( tcpip_server->socket,
				      network_server->timeout, message_buffer );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s from server '%s'",
			fname, network_server->record->name );

		if ( ( mx_status.code == MXE_NETWORK_CONNECTION_LOST )
		  || ( mx_status.code == MXE_NETWORK_IO_ERROR )
		  || ( mx_status.code == MXE_TIMED_OUT ) )
		{
			if ( tcpip_server->socket != NULL ) {
				(void) mx_network_mark_handles_as_invalid(
						network_server->record );

				(void) mx_socket_close( tcpip_server->socket );
			}
			tcpip_server->socket = NULL;
		}

		return mx_error( mx_status.code, location, mx_status.message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_tcpip_server_send_message( MX_NETWORK_SERVER *network_server,
				MX_NETWORK_MESSAGE_BUFFER *message_buffer )
{
	static const char fname[] = "mxn_tcpip_server_send_message()";

	MX_TCPIP_SERVER *tcpip_server;
	char location[ sizeof(fname) + MXU_HOSTNAME_LENGTH + 20 ];
	mx_status_type mx_status;

	tcpip_server = (MX_TCPIP_SERVER *)
				network_server->record->record_type_struct;

	if ( tcpip_server == (MX_TCPIP_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TCPIP_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	mx_status = mx_network_socket_send_message( tcpip_server->socket,
						network_server->timeout,
						message_buffer );

	if ( mx_status.code != MXE_SUCCESS ) {
		sprintf( location, "%s to server '%s'",
			fname, network_server->record->name );

		if ( ( mx_status.code == MXE_NETWORK_CONNECTION_LOST )
		  || ( mx_status.code == MXE_NETWORK_IO_ERROR )
		  || ( mx_status.code == MXE_TIMED_OUT ) )
		{
			if ( tcpip_server->socket != NULL ) {
				(void) mx_network_mark_handles_as_invalid(
						network_server->record );

				(void) mx_socket_close( tcpip_server->socket );
			}
			tcpip_server->socket = NULL;
		}

		return mx_error( mx_status.code, location, mx_status.message );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_tcpip_server_connection_is_up( MX_NETWORK_SERVER *network_server,
					int *connection_is_up )
{
	static const char fname[] = "mxn_tcpip_server_connection_is_up()";

	MX_TCPIP_SERVER *tcpip_server;

	tcpip_server = (MX_TCPIP_SERVER *)
			network_server->record->record_type_struct;

	if ( tcpip_server == (MX_TCPIP_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TCPIP_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	if ( tcpip_server->socket == NULL ) {
		*connection_is_up = FALSE;
	} else {
		*connection_is_up = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_tcpip_server_reconnect_if_down( MX_NETWORK_SERVER *network_server )
{
	static const char fname[] = "mxn_tcpip_server_reconnect_if_down()";

	MX_TCPIP_SERVER *tcpip_server;
	unsigned long flags;
	mx_status_type mx_status;

	tcpip_server = (MX_TCPIP_SERVER *)
			network_server->record->record_type_struct;

	if ( tcpip_server == (MX_TCPIP_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TCPIP_SERVER pointer for server '%s' is NULL.",
			network_server->record->name );
	}

	flags = network_server->server_flags;

	if ( flags & MXF_NETWORK_SERVER_DISABLED ) {
		return mx_error( MXE_RECORD_DISABLED_BY_USER, fname,
	"Connections to MX server '%s' have been disabled in the MX database.",
			network_server->record->name );
	}

	if ( tcpip_server->socket == NULL ) {

		/* The connection went away for some reason, so should we
		 * try to reconnect?
		 */

		network_server->network_handles_are_valid = TRUE;

		if ( flags & MXF_NETWORK_SERVER_NO_AUTO_RECONNECT ) {
			return mx_error_quiet( MXE_NETWORK_IO_ERROR, fname,
			"Server '%s' at '%s', port %ld is not connected.",
				network_server->record->name,
				tcpip_server->hostname,
				tcpip_server->port );
		} else {
			mx_info(
			"*** Reconnecting to server '%s' at '%s', port %ld.",
				network_server->record->name,
				tcpip_server->hostname,
				tcpip_server->port );

			mx_status = mxn_tcpip_server_open(
					network_server->record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_TCPIP */
