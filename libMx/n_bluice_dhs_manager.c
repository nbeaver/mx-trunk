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
	mx_status_type mx_status;

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

#if 0
	snprintf( message_type_format, sizeof(message_type_format),
			"%%%lus", (unsigned long) sizeof(message_type_name) );

	for (;;) {
		mx_status = mx_bluice_receive_message( dhs_manager_record,
					NULL, 0, &actual_data_length, -1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 1
		MX_DEBUG(-2,("%s: received '%s' from Blu-Ice server '%s'.",
			fname, bluice_server->receive_buffer,
			dhs_manager_record->name ));
#endif

		/* Figure out what kind of message this is. */

		num_items = sscanf( bluice_server->receive_buffer,
				message_type_format,
				message_type_name );

		if ( num_items != 1 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Message type name not found in message '%s' "
			"received from DHS server '%s'.",
				bluice_server->receive_buffer,
				dhs_manager_record->name );
		}

		for ( i = 0; i < num_dhs_handlers; i++ ) {
			if ( strcmp( message_type_name, 
				dhs_handler_list[i].message_name ) == 0 )
			{
				break;		/* Exit the for(i) loop. */
			}
		}

		if ( i >= num_dhs_handlers ) {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Message type '%s' received in message '%s' from "
			"DHS server '%s' does not currently have a handler.",
				message_type_name,
				bluice_server->receive_buffer,
				dhs_manager_record->name );
		} else {
			message_fn = dhs_handler_list[i].message_handler;

			if ( message_fn == NULL ) {
				MX_DEBUG( 2,("%s: Message type '%s' SKIPPED.",
					fname, message_type_name ));
			} else {
				MX_DEBUG( 2,("%s: Invoking handler for '%s'",
						fname, message_type_name));

				mx_status = (*message_fn)( thread,
						dhs_manager_record,
						bluice_server,
						bluice_dhs_manager );
			}
		}

		mx_status = mx_thread_check_for_stop_request( thread );
	}
#endif

#if 1 || ( defined(OS_HPUX) && !defined(__ia64) )
	return MX_SUCCESSFUL_RESULT;
#endif

}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxn_bluice_dhs_manager_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxn_bluice_dhs_manager_create_record_structures()";

	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DHS_MANAGER *bluice_dhs_manager;
	mx_status_type mx_status;

	/* Allocate memory for the necessary structures. */

	bluice_server = (MX_BLUICE_SERVER *)
				malloc( sizeof(MX_BLUICE_SERVER) );

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_SERVER structure." );
	}

	bluice_dhs_manager = (MX_BLUICE_DHS_MANAGER *)
				malloc( sizeof(MX_BLUICE_DHS_MANAGER) );

	if ( bluice_dhs_manager == (MX_BLUICE_DHS_MANAGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_DHS_MANAGER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = bluice_server;;
	record->record_type_struct = bluice_dhs_manager;
	record->class_specific_function_list = NULL;

	bluice_server->record = record;
	bluice_server->socket = NULL;

	bluice_dhs_manager->record = record;
	bluice_dhs_manager->dhs_manager_thread = NULL;

	mx_status = mx_mutex_create( &(bluice_server->socket_send_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mutex_create( &(bluice_server->foreign_data_mutex) );

	return mx_status;
}

#define CLIENT_TYPE_RESPONSE_LENGTH \
	((2*MXU_HOSTNAME_LENGTH) + MXU_USERNAME_LENGTH + 100)

MX_EXPORT mx_status_type
mxn_bluice_dhs_manager_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_bluice_dhs_manager_open()";

	MX_LIST_HEAD *list_head;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DHS_MANAGER *bluice_dhs_manager;
	mx_status_type mx_status;

#if BLUICE_DHS_MANAGER_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

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

	bluice_dhs_manager = (MX_BLUICE_DHS_MANAGER *)
					record->record_type_struct;

	if ( bluice_dhs_manager == (MX_BLUICE_DHS_MANAGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BLUICE_DHS_MANAGER pointer for record '%s' is NULL.",
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

	/* Listen for connections from the DHS on the server port. */

	mx_status = mx_tcp_socket_open_as_server( &(bluice_server->socket),
				bluice_dhs_manager->port_number,
				MXF_SOCKET_DISABLE_NAGLE_ALGORITHM,
				MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DHS_MANAGER *bluice_dhs_manager;
	MX_SOCKET *server_socket;
	long thread_exit_status;
	mx_status_type mx_status;
	long mx_status_code;

	bluice_server = (MX_BLUICE_SERVER *) record->record_class_struct;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

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

	/* Lock the bluice_server mutexes so that other threads cannot
	 * use the data structures while we are destroying them.  Get
	 * copies of the pointers to the mutexes so that we can continue
	 * to hold them while tearing down the bluice_server structure.
	 */

#if BLUICE_DHS_MANAGER_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Locking the socket send mutex.", fname));
#endif

	mx_status_code = mx_mutex_lock( bluice_server->socket_send_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the socket send mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

#if BLUICE_DHS_MANAGER_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Locking the foreign data mutex.", fname));
#endif

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

#if BLUICE_DHS_MANAGER_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Shutting down the server socket.", fname));
#endif

	server_socket = bluice_server->socket;

	if ( server_socket != NULL ) {
		mx_status = mx_socket_close( server_socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	bluice_server->socket = NULL;

	/* FIXME: Successfully destroying all of the interrelated Blu-Ice
	 * data structures in a thread-safe manner is difficult.  For example,
	 * the MX_BLUICE_SERVER has a foreign data mutex that could be used
	 * to prevent access.  But there is a window for a race condition
	 * between the time another thread gets a pointer to the
	 * MX_BLUICE_SERVER structure and the time that thread locks the
	 * foreign data mutex in that structure.  During that window of
	 * opportunity, it would be possible for MX_BLUICE_SERVER structure
	 * to be free()-ed by the thread running this close() routine.
	 * If that happened, then the other thread would core dump when
	 * it tried to access the mutex through the free()-ed 
	 * MX_BLUICE_SERVER pointer.
	 *
	 * For now, all that is important is that we make sure the monitor
	 * thread is shut down before destroying the socket so that the
	 * user does not get an error message from the socket routines.
	 * This means that a lost connection to the Blu-Ice server currently
	 * can only be dealt with by exiting the application program and then
	 * reconnecting.
	 */

	/* FIXME: Return with the mutexes still locked.  We assume for now
	 * that the program is shutting down.
	 */

	return mx_status;

#if 0 /* Disabled for now. */

	/* FIXME: If we did attempt to free all of the data structures,
	 * it would resemble stuff like the following code.
	 */

#if BLUICE_DHS_MANAGER_DEBUG_SHUTDOWN
	MX_DEBUG(-2,("%s: Destroying ion chamber data structures.", fname));
#endif

	if ( bluice_server->ion_chamber_array != NULL ) {
	    for ( i = 0; i < bluice_server->num_ion_chambers; i++ ) {
		foreign_ion_chamber = bluice_server->ion_chamber_array[i];

		    if ( foreign_ion_chamber != NULL ) {

			/* Reach through to the timer record and delete the
			 * entry in its array of foreign ion chambers.  This
			 * is thread-safe since the 'bluice_timer' record is
			 * expected to lock the Blu-Ice foreign data mutex
			 * before using these data structures.
			 */

			mx_timer = foreign_ion_chamber->mx_timer;

			bluice_timer = (MX_BLUICE_TIMER *)
					mx_timer->record->record_type_struct;

			for ( j = 0; j < bluice_timer->num_ion_chambers; j++ ) {
			    if ( foreign_ion_chamber ==
			    	bluice_timer->foreign_ion_chamber_array[j] )
			    {
			    	bluice_timer->foreign_ion_chamber_array[j]
					= NULL;

				break;    /* Exit the for(j) loop. */
			    }
			}
		    }
		}
		mx_free( foreign_ion_chamber );
	    }
	    mx_free( bluice_server->ion_chamber_array );
	}

#endif /* Disabled for now. */

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

