/*
 * Name:     n_bluice_dhs_manager.c
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
 * Copyright 2008, 2010, 2012, 2015 Illinois Institute of Technology
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
#include "mx_driver.h"
#include "mx_bluice.h"
#include "d_bluice_ion_chamber.h"
#include "d_bluice_motor.h"
#include "d_bluice_shutter.h"
#include "d_bluice_timer.h"
#include "v_bluice_operation.h"
#include "v_bluice_string.h"
#include "n_bluice_dhs.h"
#include "n_bluice_dhs_manager.h"

MX_RECORD_FUNCTION_LIST mxn_bluice_dhs_manager_record_function_list = {
	NULL,
	mxn_bluice_dhs_manager_create_record_structures,
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

static void
mxn_bluice_dhs_manager_register_devices( MX_RECORD *dhs_manager_record,
						MX_RECORD *dhs_record )
{
	MX_RECORD *list_head_record, *current_record;
	MX_RELAY *relay;
	MX_BLUICE_FOREIGN_DEVICE *fdev;
	MX_BLUICE_ION_CHAMBER *bluice_ion_chamber;
	MX_BLUICE_MOTOR *bluice_motor;
	MX_BLUICE_OPERATION *bluice_operation;
	MX_BLUICE_SHUTTER *bluice_shutter;
	MX_BLUICE_STRING *bluice_string;
	char command[200];

	list_head_record = dhs_manager_record->list_head;

	current_record = list_head_record->next_record;

	while( current_record != list_head_record ) {

	    strlcpy( command, "", sizeof(command) );

	    /* FIXME: Not yet implemented are registration
	     * of encoders, and objects.
	     */

	    switch( current_record->mx_type ) {
	    case MXT_AIN_BLUICE_DHS_ION_CHAMBER:
		bluice_ion_chamber = current_record->record_type_struct;

		if ( bluice_ion_chamber->bluice_server_record == dhs_record ) {

		    fdev = bluice_ion_chamber->foreign_device;

		    snprintf( command, sizeof(command),
			"stoh_register_ion_chamber %s %s %ld %s %s",
					current_record->name,
					fdev->u.ion_chamber.counter_name,
					fdev->u.ion_chamber.channel_number,
					fdev->u.ion_chamber.timer_name,
					fdev->u.ion_chamber.timer_type );
		}
		break;

	    case MXT_MTR_BLUICE_DHS:
		bluice_motor = current_record->record_type_struct;

		if ( bluice_motor->bluice_server_record == dhs_record ) {

		    snprintf( command, sizeof(command),
			"stoh_register_real_motor %s %s",
					current_record->name,
					bluice_motor->bluice_name );
		}
		break;

	    case MXT_RLY_BLUICE_DHS_SHUTTER:
		bluice_shutter = current_record->record_type_struct;

		if ( bluice_shutter->bluice_server_record == dhs_record ) {

		    if ( strlen(bluice_shutter->bluice_name) > 0 ) {
			snprintf( command, sizeof(command),
			    "stoh_register_shutter %s %s",
						current_record->name,
						bluice_shutter->bluice_name );
		    } else {
			snprintf( command, sizeof(command),
			    "stoh_register_shutter %s ", current_record->name );

			relay = current_record->record_class_struct;

			switch( relay->relay_status ) {
			case MXF_RELAY_IS_CLOSED:
			    strlcat( command, "closed", sizeof(command) );
			    break;

			case MXF_RELAY_IS_OPEN:
			    strlcat( command, "open", sizeof(command) );
			    break;

			default:
			    strlcat( command, "closed", sizeof(command) );
			    break;
			}
		    }
		}
		break;

	    case MXV_BLUICE_DHS_OPERATION:
	        bluice_operation = current_record->record_type_struct;

		if ( bluice_operation->bluice_server_record == dhs_record ) {
		
		    snprintf( command, sizeof(command),
		    	"stoh_register_operation %s %s",
					bluice_operation->bluice_name,
					bluice_operation->bluice_name );
		}
	    	break;

	    case MXV_BLUICE_DHS_STRING:
		bluice_string = current_record->record_type_struct;

		if ( bluice_string->bluice_server_record == dhs_record ) {

		    snprintf( command, sizeof(command),
			"stoh_register_string %s %s",
					bluice_string->bluice_name,
					bluice_string->bluice_name );
					
		}
		break;

	    default:
		break;
	    }

	    if ( strlen(command) > 0 ) {
		mx_status_type mx_status;

		mx_status = mx_bluice_send_message( dhs_record,
							command, NULL, 0 );

		MXW_UNUSED( mx_status );
	    }

	    current_record = current_record->next_record;
	}

	return;
}

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

	MX_RECORD **dhs_record_array;
	MX_RECORD *dhs_record = NULL;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
	unsigned long i, num_dhs_records;
	long mx_status_code;

	double timeout_in_seconds;
	int manager_socket_fd, saved_errno;
	char *error_string;
	void *client_address_ptr;
	struct sockaddr_in client_address;
	mx_socklen_t client_address_size = sizeof( struct sockaddr_in );
	mx_status_type mx_status;

	char message[500];
	char client_type[40];
	char dhs_name[40];
	char protocol_name[40];
	int num_items;
	char format[40];

	size_t text_length, null_bytes_to_send;
	char *ptr;
	long num_bytes_available;

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

#if 0 && BLUICE_DHS_MANAGER_DEBUG
		if ( mx_status.code == MXE_TIMED_OUT ) {
			if ( mx_get_debug_level() >= -2 ) {
				(void) mx_error( mx_status.code,
				mx_status.location, mx_status.message );
			}

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
					(mx_socklen_t *) &client_address_size );

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
		/* Send an stoc_send_client_type message to the DHS.
		 *
		 * We do not use mx_bluice_send_message() here, since the
		 * DHS manager record does not have an MX_BLUICE_SERVER
		 * structure.
		 */

		/* The message starts with the string: stoc_send_client_type */

		strlcpy( message, "stoc_send_client_type", sizeof(message) );

		/* The message must be padded with NULs out to 
		 * MX_BLUICE_OLD_MESSAGE_LENGTH (200) bytes.
		 */

		text_length = strlen( message );

		null_bytes_to_send = MX_BLUICE_OLD_MESSAGE_LENGTH - text_length;

		ptr = message + text_length;

		memset( ptr, 0, null_bytes_to_send );

		/* Now send the message. */

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: sending '%s' command to DHS socket fd %d",
			  fname, message, dhs_socket->socket_fd));
#endif
		mx_status = mx_socket_send( dhs_socket, &message,
					MX_BLUICE_OLD_MESSAGE_LENGTH );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"The attempt by DHS manager record '%s' "
			"to send '%s' to DHS socket %d failed.",
				dhs_manager_record->name,
				message, dhs_socket->socket_fd );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: '%s' successfully sent.",
				fname, message));
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
		/* How many bytes are there to read? */

		mx_status = mx_socket_num_input_bytes_available( dhs_socket,
							&num_bytes_available );

		if ( mx_status.code != MXE_SUCCESS ) {

			mx_socket_close( dhs_socket );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: socket %d, num_bytes_available = %ld",
			fname, dhs_socket->socket_fd, num_bytes_available ));
#endif

		if ( num_bytes_available > (sizeof(message) - 1) ) {
			(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The number of bytes (%ld) available to read "
			"from DHS socket %d is greater than the maximum "
			"length of the message buffer %ld.",
				num_bytes_available,
				dhs_socket->socket_fd,
				(long) (sizeof(message) - 1) );
		}

		/* Read the available number of bytes. */

		mx_status = mx_socket_receive( dhs_socket,
						message,
						num_bytes_available,
						NULL, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS ) {

			mx_socket_close( dhs_socket );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

		/* Make sure that the response is null terminated. */

		message[num_bytes_available] = '\0';

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: DHS socket %d, response = '%s'",
			fname, dhs_socket->socket_fd, message));
#endif
		/* Parse the returned string. */

		snprintf( format, sizeof(format), "%%%ds %%%ds %%%ds",
			(int) sizeof(client_type) - 1,
			(int) sizeof(dhs_name) - 1,
			(int) sizeof(protocol_name) - 1 );

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: format = '%s'", fname, format));
#endif
		num_items = sscanf( message, format,
				client_type, dhs_name, protocol_name );

		if ( num_items == 3 ) {
			/* Everything is fine. */
		} else
		if ( num_items == 2 ) {
			/* We must specify a protocol name. */

			strlcpy( protocol_name, "", sizeof(protocol_name) );
		} else {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Could not parse DHS socket %d message response = '%s'",
				dhs_socket->socket_fd, message );

			mx_socket_close( dhs_socket );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: client_type = '%s'", fname, client_type));

		MX_DEBUG(-2,("%s: dhs_name = '%s'", fname, dhs_name));

		MX_DEBUG(-2,("%s: protocol_name = '%s'", fname, protocol_name));
#endif

		if ( (strcmp(client_type, "htos_client_type_is_hardware") != 0)
		  && (strcmp(client_type, "htos_client_is_hardware") != 0) )
		{
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Received unexpected message type '%s' from "
			"DHS socket %d in response to an "
			"'stoc_send_client_type' message.",
				client_type, dhs_socket->socket_fd );
		}

		/* Look for the DHS record that corresponds to
		 * the supplied DHS name.
		 */

		dhs_record_array = bluice_dhs_manager->dhs_record_array;

		num_dhs_records = bluice_dhs_manager->num_dhs_records;

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: num_dhs_records = %lu",
			fname, num_dhs_records));
#endif

		for ( i = 0; i < num_dhs_records; i++ ) {
			dhs_record = dhs_record_array[i];

			bluice_dhs_server = dhs_record->record_type_struct;

#if BLUICE_DHS_MANAGER_DEBUG
			MX_DEBUG(-2,("%s: Comparing '%s' to '%s' for "
			"DHS record '%s', i = %lu",
				fname, dhs_name, bluice_dhs_server->dhs_name,
				dhs_record->name, i ));
#endif
			if ( strcmp( dhs_name,
				bluice_dhs_server->dhs_name ) == 0 )
			{
				/* We have found a match, so
				 * break out of the for() loop.
				 */

				break;
			}
		}

		if ( i >= num_dhs_records ) {
			(void) mx_error( MXE_NOT_FOUND, fname,
			"DHS name '%s' sent by DHS socket %d does not match "
			"any of the DHS names managed by DHS manager '%s'.  "
			"The DHS connection will be closed.",
				dhs_name, dhs_socket->socket_fd,
				dhs_manager_record->name );

			mx_socket_close( dhs_socket );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

		/* Discard any unread input such as trailing null bytes
		 * from the DHS socket.
		 */

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,
		("%s: FIXME: 1 second delay kludge for DHS socket %d",
			fname, dhs_socket->socket_fd ));
#endif

		mx_msleep(1000);  /* FIXME: This should not be necessary. */

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: Discarding unread input from DHS socket %d",
			fname, dhs_socket->socket_fd ));
#endif
		mx_status = mx_socket_discard_unread_input( dhs_socket );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_socket_close( dhs_socket );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

		/* Assign this socket to the DHS record that we have found. */

		bluice_server = dhs_record->record_class_struct;

		mx_status_code = mx_mutex_lock(
					bluice_server->foreign_data_mutex );

		if ( mx_status_code != MXE_SUCCESS ) {
			(void) mx_error( mx_status_code, fname,
			"Unable to lock the foreign data mutex "
			"for DHS server '%s'.", dhs_record->name );

			mx_socket_close( dhs_socket );

			/* Go back to the top of the for(;;) loop. */

			continue;
		}

		bluice_server->socket = dhs_socket;

		/* FIXME: It should not be necessary to hard code this. */

		bluice_server->protocol_version = MX_BLUICE_PROTOCOL_2;

#if BLUICE_DHS_MANAGER_DEBUG
		MX_DEBUG(-2,("%s: DHS socket %d assigned to DHS record '%s'.",
			fname, dhs_socket->socket_fd, dhs_record->name ));
#endif
		/* The last step is to send a series of stoh_register_...
		 * commands to the DHS.  This is necessary to prod the
		 * DHS into sending configuration parameter requests.
		 */

		mxn_bluice_dhs_manager_register_devices(
				dhs_manager_record, dhs_record );

		mx_mutex_unlock( bluice_server->foreign_data_mutex );
	}

	/* Should never get here. */

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
			"Ran out of memory trying to allocate "
			"an MX_BLUICE_DHS_MANAGER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;;
	record->record_type_struct = bluice_dhs_manager;
	record->class_specific_function_list = NULL;

	bluice_dhs_manager->record = record;
	bluice_dhs_manager->dhs_manager_thread = NULL;
	bluice_dhs_manager->operation_counter = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_bluice_dhs_manager_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dhs_manager_open()";

	MX_LIST_HEAD *list_head;
	MX_BLUICE_DHS_MANAGER *bluice_dhs_manager;
	MX_RECORD *current_record;
	MX_RECORD **record_array;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
	unsigned long i, num_dhs_records;
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

	/* Find out how many DHS records are in the MX database for
	 * this DHS manager.
	 */

	num_dhs_records = 0;

	current_record = record->next_record;

	while( current_record != record ) {
		if ( current_record->mx_type == MXN_BLUICE_DHS_SERVER ) {
			bluice_dhs_server = current_record->record_type_struct;

			if ( record == bluice_dhs_server->dhs_manager_record ) {
				num_dhs_records++;
			}
		}

		current_record = current_record->next_record;
	}

#if BLUICE_DHS_MANAGER_DEBUG
	MX_DEBUG(-2,("%s: num_dhs_records = %lu", fname, num_dhs_records));
#endif
	/* Create an array to contain pointers to the DHS records. */

	record_array = (MX_RECORD **)
			malloc( num_dhs_records * sizeof(MX_RECORD *) );

	if ( record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu-element array "
		"of DHS record pointers for DHS manager '%s'.",
			num_dhs_records, record->name );
	}

	bluice_dhs_manager->dhs_record_array = record_array;

	/* Now add the DHS records to the array. */

	i = 0;

	current_record = record->next_record;

	while( current_record != record ) {

		if ( i >= num_dhs_records ) {
			break;
		}

		if ( current_record->mx_type == MXN_BLUICE_DHS_SERVER ) {
			bluice_dhs_server = current_record->record_type_struct;

			if ( record == bluice_dhs_server->dhs_manager_record ) {
				record_array[i] = current_record;

#if BLUICE_DHS_MANAGER_DEBUG
				MX_DEBUG(-2,("%s: Added DHS '%s' to DHS "
				"manager '%s' record array, i = %lu",
					fname, current_record->name,
					record->name, i ));
#endif
				i++;
			}
		}

		current_record = current_record->next_record;
	}

	bluice_dhs_manager->num_dhs_records = num_dhs_records;

	/* Listen for connections from DHS processes on the server port. */

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
	MX_DEBUG(-2,("%s: DHS manager thread for record '%s' stopped "
		"with exit status = %ld",
		fname, record->name, thread_exit_status ));
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

