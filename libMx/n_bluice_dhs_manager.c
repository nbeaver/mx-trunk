/*
 * Name:     n_bluice_dhs.c
 *
 * Purpose:  Blu-Ice DCSS-like manager of DHS connection attempts.
 *
 *           This driver opens a server socket and then waits for connection
 *           attempts by DHS processes.  Individual bluice_dhs_server records
 *           register themselves with the manager record.  If a connection
 *           attempt succeeds, then the monitor thread for the corresponding
 *           DHS record is started.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_DHS_MANAGER_DEBUG		TRUE

#define BLUICE_DHS_MANAGER_DEBUG_SHUTDOWN	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bluice.h"
#include "d_bluice_timer.h"
#include "n_bluice_dhs_manager.h"

MX_RECORD_FUNCTION_LIST mxn_bluice_dhs_manager_record_function_list = {
	NULL,
	mxn_bluice_dhs_manager_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxn_bluice_dhs_manager_open,
	mxn_bluice_dhs_manager_close,
	NULL,
	mxn_bluice_dhs_manager_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxn_bluice_dhs_manager_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXN_BLUICE_DHS_MANAGER_STANDARD_FIELDS
};

long mxn_bluice_dhs_manager_num_record_fields
		= sizeof( mxn_bluice_dhs_manager_rf_defaults )
		    / sizeof( mxn_bluice_dhs_manager_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxn_bluice_dhs_manager_rfield_def_ptr
			= &mxn_bluice_dhs_manager_rf_defaults[0];

/*-------------------------------------------------------------------------*/

/* mxn_bluice_dhs_manager_thread() waits for connection attempts by
 * registered DHS servers.  If the DHS that is connecting is not
 * recognized or is already connected, then the connection attempt
 * is rejected.  If the connection attempt succeeds, then the
 * manager thread starts the monitor thread that was registered
 * for that DHS.
 */

static mx_status_type
mxn_bluice_dhs_manager_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxn_bluice_dhs_manager_thread()";

	MX_RECORD *dhs_manager_record;
	MX_BLUICE_DHS_MANAGER *bluice_dhs_manager;
	MX_SOCKET *dhs_socket;
	double timeout_in_seconds;
	int manager_socket_fd, saved_errno;
	char *error_string;
	void *client_address_ptr;
	struct sockaddr_in client_address;
	mx_socklen_t client_address_size = sizeof( struct sockaddr_in );
	mx_status_type mx_status;

	char message_header[MX_BLUICE_MSGHDR_LENGTH+1];

	static char stoc_send_client_type[] = "stoc_send_client_type";
	size_t stoc_length;

	stoc_length = strlen( stoc_send_client_type ) + 1;

	client_address_ptr = &client_address;

	/*-------------------------------------------------------*/

	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	dhs_manager_record = (MX_RECORD *) args;

#if BLUICE_DHS_MANAGER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, dhs_manager_record->name));
#endif

	bluice_dhs_manager = (MX_BLUICE_DHS_MANAGER *)
				dhs_manager_record->record_type_struct;

	if ( bluice_dhs_manager == (MX_BLUICE_DHS_MANAGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_DHS_MANAGER pointer for record '%s' is NULL.",
			dhs_manager_record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	manager_socket_fd = bluice_dhs_manager->socket->socket_fd;

	for (;;) {
		mx_status = mx_thread_check_for_stop_request( thread );

		/* See if a DHS process is attempting to connect. */

		timeout_in_seconds = 1.0;

		mx_status = mx_socket_wait_for_event(
					bluice_dhs_manager->socket,
					timeout_in_seconds );

#if BLUICE_DHS_MANAGER_DEBUG
		if ( mx_status.code == MXE_TIMED_OUT ) {
			(void) mx_error( mx_status.code,
				mx_status.location, mx_status.message );

			continue;  /* Go back to the top of the for(;;) loop. */
		} else
#endif
		if ( mx_status.code != MXE_SUCCESS ) {
			continue;  /* Go back to the top of the for(;;) loop. */
		}

		/* If we get here, then bluice_dhs_manager->socket
		 * is readable.
		 */

		/* Allocate memory for a new MX_SOCKET structure. */

		dhs_socket = (MX_SOCKET *) malloc( sizeof(MX_SOCKET) );

		if ( dhs_socket == (MX_SOCKET *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to allocate an "
				"MX_SOCKET structure." );
		}

		/* Accept the connection. */

		dhs_socket->socket_fd = accept( manager_socket_fd,
					(struct sockaddr *) client_address_ptr,
					&client_address_size );

		saved_errno = mx_socket_check_error_status(
					&(dhs_socket->socket_fd),
					MXF_SOCKCHK_INVALID,
					&error_string );

		if ( saved_errno != 0 ) {

			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"An attempt to perform an accept() on "
			"DHS manager socket %d failed.  "
			"Errno = %d.  Error string = '%s'.",
				manager_socket_fd,
				saved_errno, error_string );
				
			/* Go back to the top of the for(;;) loop. */

			continue;
		}
#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: DHS socket fd = %d",
				fname, dhs_socket->socket_fd));
#endif
		/* Send an stoc_send_client_type message to the DHS. */

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: sending '%s' command to DHS socket fd %d",
			  fname, stoc_send_client_type, dhs_socket->socket_fd));
#endif
		/* First send the header. */

		snprintf( message_header, sizeof(message_header),
				"%*lu0",
				MX_BLUICE_MSGHDR_TEXT_LENGTH,
				(unsigned long) stoc_length );

		mx_status = mx_socket_send( dhs_socket,
						&message_header,
						MX_BLUICE_MSGHDR_LENGTH );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"The attempt by DHS manager record '%s' "
			"to send a message header to "
			"DHS socket %d failed.",
				dhs_manager_record->name,
				dhs_socket->socket_fd );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

		/* Then send the message body. */

		mx_status = mx_socket_send( dhs_socket,
						&stoc_send_client_type,
						stoc_length );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"The attempt by DHS manager record '%s' "
			"to send a message header to "
			"DHS socket %d failed.",
				dhs_manager_record->name,
				dhs_socket->socket_fd );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: '%s' successfully sent.",
				fname, stoc_send_client_type));
#endif

		/* Wait up to 1 second for a message reply. */

		timeout_in_seconds = 1.0;

		mx_status = mx_socket_wait_for_event( dhs_socket,
						timeout_in_seconds );

		if ( mx_status.code == MXE_TIMED_OUT ) {
			(void) mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %g seconds for DHS socket %d "
			"to respond to an stoc_send_client_type message.",
				timeout_in_seconds,
				dhs_socket->socket_fd );

			mx_socket_close( dhs_socket );

			/* Go back to the top of the for(;;) loop. */

			continue;
		} else
		if ( mx_status.code != MXE_SUCCESS ) {

			mx_socket_close( dhs_socket );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: Reading response header from DHS socket %d",
				fname, dhs_socket->socket_fd ));
#endif
	}

#if ( defined(OS_HPUX) && !defined(__ia64) )
	return MX_SUCCESSFUL_RESULT;
#endif

}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxn_bluice_dhs_manager_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxn_bluice_dhs_manager_create_record_structures()";

	MX_BLUICE_DHS_MANAGER *bluice_dhs_manager;

	/* Allocate memory for the necessary structures. */

	bluice_dhs_manager = (MX_BLUICE_DHS_MANAGER *)
				malloc( sizeof(MX_BLUICE_DHS_MANAGER) );

	if ( bluice_dhs_manager == (MX_BLUICE_DHS_MANAGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_DHS_MANAGER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;;
	record->record_type_struct = bluice_dhs_manager;
	record->class_specific_function_list = NULL;

	bluice_dhs_manager->record = record;
	bluice_dhs_manager->dhs_manager_thread = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_bluice_dhs_manager_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dhs_manager_open()";

	MX_LIST_HEAD *list_head;
	MX_BLUICE_DHS_MANAGER *bluice_dhs_manager;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if BLUICE_DHS_MANAGER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, record->name));
#endif

	mx_status = MX_SUCCESSFUL_RESULT;

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			record->name );
	}

	bluice_dhs_manager = (MX_BLUICE_DHS_MANAGER *)
					record->record_type_struct;

	if ( bluice_dhs_manager == (MX_BLUICE_DHS_MANAGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BLUICE_DHS_MANAGER pointer for record '%s' is NULL.",
			record->name );
	}

	/* Allocate memory for the receive buffer. */

	bluice_dhs_manager->receive_buffer =
		(char *) malloc( MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH );

	if ( bluice_dhs_manager->receive_buffer == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a Blu-Ice message receive "
		"buffer of length %lu.",
		    (unsigned long) MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH  );
	}

	bluice_dhs_manager->receive_buffer_length
		= MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH;

	/* Listen for connections from the DHS on the server port. */

	mx_status = mx_tcp_socket_open_as_server(
				&(bluice_dhs_manager->socket),
				bluice_dhs_manager->port_number,
				MXF_SOCKET_DISABLE_NAGLE_ALGORITHM,
				MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if BLUICE_DHS_MANAGER_DEBUG
	MX_DEBUG(-2,("%s: Opened DHS manager port %ld, socket %d",
		fname, bluice_dhs_manager->port_number,
		bluice_dhs_manager->socket->socket_fd));
#endif

	/* At this point we need to create a thread that monitors
	 * messages sent by the DHS server.  From this point on,
	 * only mxn_bluice_dhs_manager_thread() is allowed to
	 * receive messages from the DHS server.
	 */

	mx_status = mx_thread_create(
			&(bluice_dhs_manager->dhs_manager_thread),
			mxn_bluice_dhs_manager_thread,
			record );

	return mx_status;
}

#define MX_BLUICE_DHS_CLOSE_WAIT_TIME	(5.0)		/* in seconds */

MX_EXPORT mx_status_type
mxn_bluice_dhs_manager_close( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dhs_manager_close()";

	MX_BLUICE_DHS_MANAGER *bluice_dhs_manager;
	long thread_exit_status;
	mx_status_type mx_status;

	bluice_dhs_manager = 
		(MX_BLUICE_DHS_MANAGER *) record->record_type_struct;

	if ( bluice_dhs_manager == (MX_BLUICE_DHS_MANAGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_DHS_MANAGER pointer for record '%s' is NULL.",
			record->name );
	}

	/* First, tell the monitor thread to exit. */

#if BLUICE_DHS_MANAGER_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Stopping DHS monitor thread.", fname));
#endif

	mx_status = mx_thread_stop( bluice_dhs_manager->dhs_manager_thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_thread_wait( bluice_dhs_manager->dhs_manager_thread,
					&thread_exit_status,
					MX_BLUICE_DHS_CLOSE_WAIT_TIME );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if BLUICE_DHS_MANAGER_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: DHS monitor thread stopped with exit status = %ld",
		fname, thread_exit_status ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_bluice_dhs_manager_resynchronize( MX_RECORD *record )
{
#if 1
	static const char fname[] = "mxn_bluice_dhs_manager_resynchronize()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Reconnection to Blu-Ice server '%s' is not currently supported.  "
	"The only way to reconnect is to exit your application program "
	"and then start your application again.", record->name );
#else
	mx_status_type mx_status;

	mx_status = mxn_bluice_dhs_manager_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxn_bluice_dhs_manager_open( record );

	return mx_status;
#endif
}

