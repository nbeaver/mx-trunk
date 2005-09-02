/*
 * Name:     n_bluice_dcss.c
 *
 * Purpose:  Client interface to a Blu-Ice DCSS server.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXN_BLUICE_DCSS_DEBUG		TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_bluice.h"
#include "n_bluice_dcss.h"

MX_RECORD_FUNCTION_LIST mxn_bluice_dcss_server_record_function_list = {
	NULL,
	mxn_bluice_dcss_server_create_record_structures,
	mxn_bluice_dcss_server_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxn_bluice_dcss_server_open,
	mxn_bluice_dcss_server_close,
	NULL,
	mxn_bluice_dcss_server_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxn_bluice_dcss_server_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXN_BLUICE_DCSS_SERVER_STANDARD_FIELDS
};

long mxn_bluice_dcss_server_num_record_fields
		= sizeof( mxn_bluice_dcss_server_record_field_defaults )
		    / sizeof( mxn_bluice_dcss_server_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxn_bluice_dcss_server_rfield_def_ptr
			= &mxn_bluice_dcss_server_record_field_defaults[0];

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxn_bluice_dcss_server_create_record_structures()";

	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;

	/* Allocate memory for the necessary structures. */

	bluice_server = (MX_BLUICE_SERVER *)
				malloc( sizeof(MX_BLUICE_SERVER) );

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_SERVER structure." );
	}

	bluice_dcss_server = (MX_BLUICE_DCSS_SERVER *)
				malloc( sizeof(MX_BLUICE_DCSS_SERVER) );

	if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_DCSS_SERVER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = bluice_server;;
	record->record_type_struct = bluice_dcss_server;
	record->class_specific_function_list = NULL;

	bluice_dcss_server->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_finish_record_initialization( MX_RECORD *record )
{
	MX_BLUICE_SERVER *bluice_server;

	bluice_server = (MX_BLUICE_SERVER *) record->record_class_struct;

	bluice_server->socket = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dcss_server_open()";

	MX_LIST_HEAD *list_head;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	int i, num_retries, num_bytes_available;
	unsigned long wait_ms;
	size_t actual_data_length;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = MX_SUCCESSFUL_RESULT;

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			record->name );
	}

	bluice_server = (MX_BLUICE_SERVER *) record->record_class_struct;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BLUICE_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	bluice_dcss_server = (MX_BLUICE_DCSS_SERVER *)
					record->record_type_struct;

	if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BLUICE_DCSS_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	/* Allocate memory for the receive buffer. */

	bluice_server->receive_buffer =
		(char *) malloc( MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH );

	if ( bluice_server->receive_buffer == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a Blu-Ice message receive "
		"buffer of length %lu.",
		    (unsigned long) MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH  );
	}

	bluice_server->receive_buffer_length
		= MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH;

	/* Connect to the DCSS. */

	mx_status = mx_tcp_socket_open_as_client( &(bluice_server->socket),
				bluice_dcss_server->hostname,
				bluice_dcss_server->port_number, 0,
				MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: socket opened.", fname));

	MX_DEBUG(-2,("%s: socket = %p", fname, bluice_server->socket));
	MX_DEBUG(-2,("%s: socket->socket_fd = %d",
			fname, (int) bluice_server->socket->socket_fd));
	MX_DEBUG(-2,("%s: socket->socket_flags = %#lx",
			fname, bluice_server->socket->socket_flags));

	/* The DCSS should send us a 'stoc_send_client_type' message
	 * almost immediately.  Wait for this to happen.
	 */

	wait_ms = 100;
	num_retries = 50;

	for ( i = 0; i < num_retries; i++ ) {
		mx_status = mx_socket_num_input_bytes_available(
						bluice_server->socket,
						&num_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_available > 0 )
			break;			/* Exit the for() loop. */

		mx_msleep( wait_ms );
	}

	if ( i >= num_retries ) {
		return mx_error( MXE_TIMED_OUT, fname,
			"Timed out waiting for the initial message from "
			"Blu-Ice DCSS server '%s' for over %g seconds.",
				record->name,
				0.001 * (double) ( num_retries * wait_ms ) );
	}

	/* The first message has arrived.  Read it in. */

	mx_status = mx_bluice_receive_message( record,
					&actual_data_length, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is the message a 'stoc_send_client_type' message? */

	if (strcmp(bluice_server->text_pointer, "stoc_send_client_type") != 0) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not receive the 'stoc_send_client_type' message from "
		"Blu-Ice DCSS server '%s' that we were expecting.  "
		"Instead, we received '%s'.  Perhaps the server you have "
		"specified is not a DCSS server?",
			record->name, bluice_server->text_pointer );
	}

	MX_DEBUG(-2,("%s: received '%s'.", fname, bluice_server->text_pointer));

	return mx_status;
}

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_close( MX_RECORD *record )
{
	MX_BLUICE_SERVER *bluice_server;
	MX_SOCKET *server_socket;
	mx_status_type mx_status;

	bluice_server = (MX_BLUICE_SERVER *) record->record_class_struct;

	server_socket = bluice_server->socket;

	if ( server_socket != NULL ) {
		mx_status = mx_socket_close( server_socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	bluice_server->socket = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_bluice_dcss_server_resynchronize( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mxn_bluice_dcss_server_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxn_bluice_dcss_server_open( record );

	return mx_status;
}

