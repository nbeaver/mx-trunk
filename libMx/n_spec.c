/*
 * Name:     n_spec.c
 *
 * Purpose:  Client interface to a Spec server.  Spec is a product of
 *           Certified Scientific Software. (www.certif.com)
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004-2006, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXN_SPEC_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_spec.h"
#include "n_spec.h"

MX_RECORD_FUNCTION_LIST mxn_spec_server_record_function_list = {
	NULL,
	mxn_spec_server_create_record_structures,
	mxn_spec_server_finish_record_initialization,
	NULL,
	NULL,
	mxn_spec_server_open,
	mxn_spec_server_close,
	NULL,
	mxn_spec_server_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxn_spec_server_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXN_SPEC_SERVER_STANDARD_FIELDS
};

long mxn_spec_server_num_record_fields
		= sizeof( mxn_spec_server_record_field_defaults )
			/ sizeof( mxn_spec_server_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxn_spec_server_rfield_def_ptr
			= &mxn_spec_server_record_field_defaults[0];

MX_EXPORT mx_status_type
mxn_spec_server_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxn_spec_server_create_record_structures()";

	MX_SPEC_SERVER *spec_server;

	/* Allocate memory for the necessary structures. */

	spec_server = (MX_SPEC_SERVER *) malloc( sizeof(MX_SPEC_SERVER) );

	if ( spec_server == (MX_SPEC_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SPEC_SERVER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = spec_server;
	record->class_specific_function_list = NULL;

	spec_server->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_spec_server_finish_record_initialization( MX_RECORD *record )
{
	MX_SPEC_SERVER *spec_server;

	spec_server = (MX_SPEC_SERVER *) record->record_type_struct;

	spec_server->socket = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_spec_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_spec_server_open()";

	MX_LIST_HEAD *list_head;
	MX_SPEC_SERVER *spec_server;
	MX_SOCKET *server_socket;
	char hostname[ MXU_HOSTNAME_LENGTH + 1 ];
	char *program_id, *ptr;
	int i, c, program_id_is_a_port_number;
	int port, first_port_number, last_port_number;
	long spec_command_code;
	unsigned long process_id;
	size_t hostname_length, program_id_length;
	char client_id[ MXU_PROGRAM_NAME_LENGTH + 1 ];
	char server_id[ MXU_PROGRAM_NAME_LENGTH + 1 ];
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			record->name );
	}

	spec_server = (MX_SPEC_SERVER *) record->record_type_struct;

	if ( spec_server == (MX_SPEC_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SPEC_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	/* A colon ':' character should separate the hostname from
	 * the program id.
	 */

	ptr = strrchr( spec_server->server_id, ':' );

	if ( ptr == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Spec server id '%s' is not valid since it does not contain "
		"the ':' character that should separate the hostname from "
		"the program identifier.", spec_server->server_id );
	}

	hostname_length = ptr - spec_server->server_id + 1;

	if ( hostname_length > MXU_HOSTNAME_LENGTH ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The spec hostname in the server id '%s' is longer than "
		"the maximum allowed value of %d",
			spec_server->server_id, MXU_HOSTNAME_LENGTH );
	}

	strlcpy( hostname, spec_server->server_id, hostname_length );

	/*---*/

	program_id = ptr + 1;

	program_id_length = strlen( program_id );

	/* If the program_id consists only of numerical characters, we
	 * can assume that it is a socket number.  Otherwise, it is the
	 * name of the remote spec process.
	 */

	program_id_is_a_port_number = TRUE;

	for ( i = 0; i < program_id_length; i++ ) {
		c = program_id[i];

		if ( isdigit(c) == 0 ) {
			program_id_is_a_port_number = FALSE;

			break;		/* Exit the for() loop. */
		}
	}

	if ( program_id_is_a_port_number ) {
		first_port_number = atoi( program_id );

		last_port_number = first_port_number;

#if MXN_SPEC_DEBUG
		MX_DEBUG(-2,("%s: hostname = '%s', port = %d",
			fname, hostname, first_port_number));
#endif
	} else {
		first_port_number = SV_PORT_FIRST;

		last_port_number = SV_PORT_LAST;

#if MXN_SPEC_DEBUG
		MX_DEBUG(-2,("%s: hostname = '%s', program_id = '%s'",
			fname, hostname, program_id));
#endif
	}

	/* Construct the ID string to send to spec. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"mx_get_record_list_head() returned a NULL pointer.  "
		"This should not be able to happen." );
	}

	process_id = mx_process_id();

#if MXN_SPEC_DEBUG
	MX_DEBUG(-2,("%s: program_name = '%s', process_id = %lu",
		fname, list_head->program_name, process_id ));
#endif

	sprintf( client_id, "MX %s %lu", list_head->program_name, process_id );

	spec_server->socket = NULL;

	for ( port = first_port_number; port <= last_port_number; port++ ) {

#if MXN_SPEC_DEBUG
		MX_DEBUG(-2,("%s: port = %d", fname, port));
#endif

		/* Try to open the socket. */

		mx_status = mx_tcp_socket_open_as_client(
			&server_socket, hostname, port,
			MXF_SOCKET_QUIET_CONNECTION,
			MX_SOCKET_DEFAULT_BUFFER_SIZE );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			spec_server->socket = server_socket;
			break;

		case MXE_NETWORK_IO_ERROR:
			spec_server->socket = NULL;

			if ( program_id_is_a_port_number ) {
				return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"The Spec server at port %d on the computer '%s' "
			"is either not running or is not working correctly.  "
			"You can try to fix this by restarting "
			"the Spec server.",
					port, hostname );
			}

			break;

		default:
			spec_server->socket = NULL;

			return mx_error( mx_status.code, fname,
			"An unexpected error occurred while trying to connect "
			"to the Spec server at port %d on the computer '%s'.",
				port, hostname);

			break;
		}

		/* If we are searching over multiple ports, we must check
		 * to see if the process we have connected to is the 
		 * spec server we want.
		 */

		if ( ( program_id_is_a_port_number == FALSE )
		  && ( spec_server->socket != NULL ) )
		{
			/* Use the 'hello' protocol to see if this is
			 * the correct server.
			 */

#if MXN_SPEC_DEBUG
			MX_DEBUG(-2,
			("%s: SV_HELLO from '%s'", fname, client_id));
#endif

			mx_status = mx_spec_send_message( record,
							SV_HELLO,
							SV_STRING,
							0, 0,
						(long) strlen(client_id) + 1,
							"", client_id );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_spec_receive_message( record,
							&spec_command_code,
							NULL, NULL, NULL,
							sizeof(server_id),
							NULL, NULL,
							server_id );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( spec_command_code != SV_HELLO_REPLY ) {
				/* We probably have found something that
				 * is not a Spec server, so we close the 
				 * connection.
				 */

				(void) mx_socket_close( spec_server->socket );


				/* Go back to the top of the for() loop
				 * for the next port number.
				 */

				continue;
			} else {
#if MXN_SPEC_DEBUG
				MX_DEBUG(-2,(
				"%s: received SV_HELLO_REPLY, server_id = '%s'",
				fname, server_id));
#endif
			}

			/* Does the server id match the program id we are
			 * looking for.
			 */

			if ( strcmp( server_id, program_id ) == 0 ) {
				/* We have found a match! */

#if MXN_SPEC_DEBUG
				MX_DEBUG(-2,("%s: We have found a match!",
					fname));
#endif

				break;		/* Exit the for() loop. */
			} else {
				/* This is the wrong server, so close
				 * the socket.
				 */

				(void) mx_socket_close( spec_server->socket );

				spec_server->socket = NULL;
			}
		}
	}

	if ( spec_server->socket == NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The spec server '%s' could not be found.",
			spec_server->server_id );
	}

	/* Check to see if the Spec server is working by asking for
	 * something innocuous, namely, whether or not the Spec server
	 * is in simulate mode.
	 */

#if 0
	{
		long dimension_array[1];
		char buffer[20];

		dimension_array[0] = sizeof(buffer);

		mx_status = mx_spec_get_array( record, "status/simulate",
					SV_STRING, 1, dimension_array,
					buffer );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG(-2,("%s: status/simulate buffer = '%s'",
			fname, buffer ));
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxn_spec_server_close( MX_RECORD *record )
{
	MX_SPEC_SERVER *spec_server;
	MX_SOCKET *server_socket;
	mx_status_type mx_status;

	spec_server = (MX_SPEC_SERVER *) record->record_type_struct;

	server_socket = spec_server->socket;

	if ( server_socket != NULL ) {
		mx_status = mx_socket_close( server_socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	spec_server->socket = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_spec_server_resynchronize( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mxn_spec_server_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxn_spec_server_open( record );

	return mx_status;
}

