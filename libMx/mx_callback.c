/*
 * Name:    mx_callback.c
 *
 * Purpose: MX callback handling functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2007-2015, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_CALLBACK_DEBUG				FALSE

#define MX_CALLBACK_DEBUG_PROCESS			FALSE

#define MX_CALLBACK_FIXUP_CORRUPTED_POLL_MESSAGE	FALSE

#define MX_CALLBACK_DEBUG_INVOKE_CALLBACK		FALSE

#define MX_CALLBACK_DEBUG_CALLBACK_POINTERS		FALSE

#define MX_CALLBACK_DEBUG_PROCESS_CALLBACKS_TIMING	FALSE

#define MX_CALLBACK_DEBUG_VALUE_CHANGED_POLL		FALSE

/* Note: If you protect the VM version of the handle table here, then
 *       you must also set NETWORK_PROTECT_VM_HANDLE_TABLE in the 
 *       file server/ms_mxserver.c
 */

#define MX_CALLBACK_USE_VM_HANDLE_TABLE			FALSE

#define MX_CALLBACK_PROTECT_VM_HANDLE_TABLE		FALSE

#if MX_CALLBACK_PROTECT_VM_HANDLE_TABLE

#  if ( MX_CALLBACK_USE_VM_HANDLE_TABLE == FALSE )
#    error If MX_CALLBACK_PROTECT_VM_HANDLE_TABLE is set, then MX_CALLBACK_USE_VM_HANDLE_TABLE must be set as well.
#  endif
#endif

/*
 * WARNING: The macro MX_CALLBACK_DEBUG_WITHOUT_TIMER should only be defined
 * if we intend to run this program from within a debugger.  If the macro is
 * defined, the poll callback code will _only_ be invoked when the programmer
 * manually invokes it from a C debugger prompt.  If code compiled with this
 * macro is run directly (not from a debugger), then the poll callback will
 * never be invoked.
 */

#define MX_CALLBACK_DEBUG_WITHOUT_TIMER			FALSE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_interval_timer.h"
#include "mx_virtual_timer.h"
#include "mx_signal_alloc.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_pipe.h"
#include "mx_unistd.h"
#include "mx_vm_alloc.h"
#include "mx_process.h"
#include "mx_callback.h"

#if MX_CALLBACK_DEBUG_PROCESS_CALLBACKS_TIMING
#include "mx_hrt_debug.h"
#endif

/*------------------------------------------------------------------------*/

#if MX_CALLBACK_DEBUG_WITHOUT_TIMER

/* WARNING: Compiling with MX_CALLBACK_DEBUG_WITHOUT_TIMER is only meant
 * for testing the callback timer logic.  You should NOT use this code
 * during normal operations.  If you do, then none of your callback
 * functions will get invoked.
 */

/*
 * If MX_CALLBACK_DEBUG_WITHOUT_TIMER is defined, then we assume that we
 * are trying to debug the operation of value changed poll messages.
 * In that case, we do not actually start a poll callback timer.  Instead,
 * we provide a function called mxp_poll_callback() that can be called from
 * the debugger to trigger a manual poll callback.  In addition, you can
 * also invoke mxp_poll_callback() by sending a SIGUSR2 signal to the
 * server process.
 */

void mxp_poll_callback( int );
void mxp_stop_master_timer( MX_INTERVAL_TIMER * );

static MX_CALLBACK_MESSAGE *mxp_poll_callback_message = NULL;
static MX_PIPE *mxp_callback_pipe = NULL;
static MX_LIST_HEAD *mxp_list_head = NULL;

MX_EXPORT mx_status_type
mx_initialize_callback_support( MX_RECORD *record )
{
	static const char fname[] = "mx_initialize_callback_support()";

	MX_LIST_HEAD *list_head;
	MX_HANDLE_TABLE *callback_handle_table;
	mx_status_type mx_status;

	mx_warning("The macro MX_CALLBACK_DEBUG_WITHOUT_TIMER has been "
	"set to TRUE for this process!  This means that poll callbacks "
	"will _not_ automatically take place.  Instead, you must send "
	"SIGUSR2 signals to the process for a poll callback to take place.");

	/* Return if callback support is already initialized. */

	if ( mxp_list_head != NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));

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

	mxp_list_head = list_head;

	/* Initialize data structures for the manual poll callback. */

	mxp_callback_pipe = list_head->callback_pipe;

	mxp_poll_callback_message = malloc( sizeof(MX_CALLBACK_MESSAGE) );

	if ( mxp_poll_callback_message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt to allocate memory for an "
		"MX_CALLBACK_MESSAGE structure failed." );
	}

	mxp_poll_callback_message->callback_type = MXCBT_POLL;
	mxp_poll_callback_message->list_head = list_head;

	/* Attempt to allocate SIGUSR2 for our private use. */

	MX_DEBUG(-2,("%s: Attempting to allocate SIGUSR2.", fname));

	mx_status = mx_signal_allocate( SIGUSR2, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return MX_SUCCESSFUL_RESULT;

	MX_DEBUG(-2,("%s: Successfully allocated SIGUSR2.", fname));

	/* Configure mxp_poll_callback() as the signal handler for SIGUSR2.
	 * This allows us to manually trigger the value changed poll using
	 * the Unix 'kill' command.
	 */

	signal( SIGUSR2, mxp_poll_callback );

	return MX_SUCCESSFUL_RESULT;
}

void
mxp_poll_callback( int signal_number )
{
	(void) mx_pipe_write( mxp_callback_pipe,
				(char *) &mxp_poll_callback_message,
				sizeof(MX_CALLBACK_MESSAGE *) );
	return;
}

void
mxp_stop_master_timer( MX_INTERVAL_TIMER *itimer )
{
	(void) mx_interval_timer_stop( itimer, NULL );

	fprintf(stderr, "Master timer stopped.\n");

	return;
}

/*------------------------------------------------------------------------*/

#else /* not MX_CALLBACK_DEBUG_WITHOUT_TIMER */

/*** This is the normal callback setup that uses virtual timers. ***/

/* NOTE: The only job of mx_request_value_changed_poll() is to send a message
 *       through the callback pipe to the main thread to ask for the value
 *       changed handlers to be polled.
 *
 *       On some platforms, this function will be invoked in a signal handler
 *       context, so the function must only do things that are signal handler
 *       safe.  In particular, you cannot allocate memory with malloc()
 *       here or write a pointer for a structure on this function's stack
 *       to the pipe.
 *
 *       The solution, in this case, is to malloc() the necessary data
 *       structure in advance in the mx_initialize_callback_support()
 *       function.
 */

MX_EXPORT void
mx_request_value_changed_poll( MX_VIRTUAL_TIMER *callback_timer,
				void *callback_args )
{
	static const char fname[] = "mx_request_value_changed_poll()";

	MX_CALLBACK_MESSAGE *poll_callback_message;
	MX_LIST_HEAD *list_head;
	MX_PIPE *mx_pipe;

#if MX_CALLBACK_DEBUG_VALUE_CHANGED_POLL
	MX_CLOCK_TICK current_clock_tick;

	current_clock_tick = mx_current_clock_tick();

	MX_DEBUG(-2,
	("%s: *************************************************", fname));

	MX_DEBUG(-2,("%s: clock tick = (%lu,%lu)",
	fname, current_clock_tick.high_order, current_clock_tick.low_order));
#endif

	if ( callback_args == NULL ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The callback_args pointer passed was NULL." );
		return;
	}

	poll_callback_message = callback_args;

	list_head = poll_callback_message->list_head;

	mx_pipe = list_head->callback_pipe;

	if ( mx_pipe == NULL ) {
#if MX_CALLBACK_DEBUG_VALUE_CHANGED_POLL
		MX_DEBUG(-2,
		("%s: callback_pipe == NULL.  Returning...", fname));
#endif
		return;
	}

	/* Write the address of the callback message to the callback pipe.
	 * The callback message will be read and handled by the server
	 * main loop.
	 *
	 * Please note that the only system calls used by normal execution
	 * of mx_pipe_write() are the write() call on Linux/Unix and
	 * WriteFile() on Win32.  Thus, mx_pipe_write() should be safe
	 * to invoke from a signal handler.
	 */

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: poll_callback_message = %p",
		fname, poll_callback_message));
#endif

	(void) mx_pipe_write( mx_pipe,
				(char *) &poll_callback_message,
				sizeof(MX_CALLBACK_MESSAGE *) );

	return;
}

#if MX_CALLBACK_FIXUP_CORRUPTED_POLL_MESSAGE
static MX_LIST_HEAD *mxp_saved_mx_list_head = NULL;
#endif

MX_EXPORT mx_status_type
mx_initialize_callback_support( MX_RECORD *record )
{
	static const char fname[] = "mx_initialize_callback_support()";

	MX_LIST_HEAD *list_head;
	MX_HANDLE_TABLE *callback_handle_table;
	MX_INTERVAL_TIMER *master_timer;
	MX_VIRTUAL_TIMER *callback_timer;
	double callback_timer_interval;
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

#if MX_CALLBACK_FIXUP_CORRUPTED_POLL_MESSAGE
	mxp_saved_mx_list_head = list_head;
#endif

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

#if MX_CALLBACK_USE_VM_HANDLE_TABLE
		{
			MX_HANDLE_STRUCT *handle_struct_array;
			unsigned long i, handle_table_size;
			size_t handle_table_size_in_bytes;

			/* Replace the handle_struct_array allocated by
			 * mx_create_handle_table() with another array
			 * allocated by mx_vm_alloc().  Initially, the
			 * new array is configured to be protected
			 * against all access.
			 */

			mx_free( callback_handle_table->handle_struct_array );

			handle_table_size = callback_handle_table->num_blocks
				* callback_handle_table->block_size;

			handle_table_size_in_bytes = handle_table_size
						* sizeof(MX_HANDLE_STRUCT);

			handle_struct_array = mx_vm_alloc( NULL,
						handle_table_size_in_bytes,
						R_OK | W_OK );

			if ( handle_struct_array == NULL ) {
				return mx_error( MXE_OPERATING_SYSTEM_ERROR,
				fname, "The attempt to swap in a protected "
				"handle_struct_array for the callback handle "
				"table failed." );
			}

			callback_handle_table->handle_struct_array
						= handle_struct_array;

			for ( i = 0; i < handle_table_size; i++ ) {
			    handle_struct_array[i].handle = MX_ILLEGAL_HANDLE;
			    handle_struct_array[i].pointer = NULL;
			}

			MX_DEBUG(-2,("%s: callback_handle_table = %p, "
			"handle_struct_array = %p",
				fname, callback_handle_table,
				callback_handle_table->handle_struct_array ));

#if MX_CALLBACK_PROTECT_VM_HANDLE_TABLE
			mx_vm_set_protection(
				callback_handle_table->handle_struct_array,
				handle_table_size_in_bytes, 0 );
#endif
		}
#endif /* MX_CALLBACK_USE_VM_HANDLE_TABLE */

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

		MX_CALLBACK_MESSAGE *poll_callback_message;

		/* The callback timer will need a structure pointer
		 * to write to the callback pipe telling the main
		 * thread that it is time to poll the value changed
		 * handlers.  We must malloc() that structure here,
		 * since it is unsafe to do it in the context of the
		 * mx_request_value_changed_poll() function.
		 */

		poll_callback_message = malloc( sizeof(MX_CALLBACK_MESSAGE) );

		if ( poll_callback_message == (MX_CALLBACK_MESSAGE *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"The attempt to allocate memory for an "
			"MX_CALLBACK_MESSAGE structure failed." );
		}

		poll_callback_message->callback_type = MXCBT_POLL;
		poll_callback_message->list_head = list_head;

		list_head->poll_callback_message = poll_callback_message;

		/* Now create the callback virtual timer.  The timer
		 * is intentionally a one-shot timer, so the poll
		 * callback function is expected to restart the timer
		 * at the beginning of that function.  This ensures
		 * that there is, at most, one poll callback message
		 * in the callback pipe at any given time and prevents
		 * the callback pipe from filling up with a multitude
		 * of poll callbacks if the callback function is slow.
		 */

		mx_status = mx_virtual_timer_create( &callback_timer,
						list_head->master_timer,
						MXIT_ONE_SHOT_TIMER,
						mx_request_value_changed_poll,
						poll_callback_message );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		poll_callback_message->u.poll.oneshot_timer = callback_timer;

		list_head->callback_timer = callback_timer;

		/* Set the poll callback timer interval (in seconds). */

		if ( list_head->poll_callback_interval >= 0.0 ) {
			callback_timer_interval
				= list_head->poll_callback_interval;
		} else {
			callback_timer_interval = 0.1;
		}

		/* Start the callback virtual timer. */

		mx_status = mx_virtual_timer_start( list_head->callback_timer,
						callback_timer_interval );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MX_CALLBACK_DEBUG
		MX_DEBUG(-2,
		("%s: callback timer started with timer interval %g seconds.",
			fname, callback_timer_interval));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* not MX_CALLBACK_DEBUG_WITHOUT_TIMER */

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_setup_callback_pipe( MX_RECORD *record_list, MX_PIPE **callback_pipe )
{
	static const char fname[] = "mx_setup_callback_pipe()";

	MX_LIST_HEAD *list_head;
	MX_PIPE *callback_pipe_ptr;
	MX_INTERVAL_TIMER *master_timer;
	mx_status_type mx_status;

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the record list %p is NULL.",
			record_list );
	}

	mx_status = mx_pipe_open( &callback_pipe_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	list_head->callback_pipe = callback_pipe_ptr;

	mx_status = mx_pipe_set_blocking_mode( list_head->callback_pipe,
					MXF_PIPE_READ | MXF_PIPE_WRITE, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If necessary, create a master timer with a timer period
	 * of 100 milliseconds.
	 */

	if ( list_head->master_timer == (MX_INTERVAL_TIMER *) NULL ) {
		mx_status = mx_virtual_timer_create_master(
						&master_timer, 0.1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		list_head->master_timer = master_timer;
	}

	if ( callback_pipe != (MX_PIPE **) NULL ) {
		*callback_pipe = callback_pipe_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_remote_field_add_callback( MX_NETWORK_FIELD *nf,
			unsigned long supported_callback_types,
			mx_status_type ( *callback_function )(
						MX_CALLBACK *, void * ),
			void *callback_argument,
			MX_CALLBACK **callback_object )
{
	static const char fname[] = "mx_remote_field_add_callback()";

	MX_CALLBACK *callback_ptr;
	MX_LIST_ENTRY *list_entry;
	MX_RECORD *server_record;
	MX_LIST_HEAD *list_head;
	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	uint32_t *header, *uint32_message;
	char *char_message;
	unsigned long header_length, message_length, message_type, status_code;
	unsigned long data_type, message_id, callback_id;
	char nf_label[80];
	mx_bool_type connected;
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

#if MX_CALLBACK_DEBUG_CALLBACK_POINTERS
	{
		int is_valid;

		is_valid = mx_pointer_is_valid( callback_function,
						sizeof(callback_function),
						R_OK );

		if ( is_valid == FALSE ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Callback function pointer %p is not valid.",
				callback_function );
		}

		if ( callback_argument != NULL ) {
			is_valid = mx_pointer_is_valid( callback_argument,
						sizeof(callback_argument),
						R_OK | W_OK );

			if ( is_valid == FALSE ) {
				return mx_error(
				MXE_CORRUPT_DATA_STRUCTURE, fname,
				"Callback argument pointer %p is not valid.",
					callback_argument );
			}
		}
	}
#endif

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

	/* If the server is not new enough to support message ids,
	 * then there is no point in proceding any further.
	 */

	if ( mx_server_supports_message_ids(server) == FALSE ) {
		return mx_error( MXE_UNSUPPORTED | MXE_QUIET, fname,
		"MX server '%s' does not support message callbacks.",
			server_record->name );
	}

	/* Make sure the network field is connected. */

	mx_status = mx_network_field_is_connected( nf, &connected );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( connected == FALSE ) {
		mx_status = mx_network_field_connect( nf );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,("\n*** ADD CALLBACK for '%s'",
			mx_network_get_nf_label(
				server_record, nf->nfname,
				nf_label, sizeof(nf_label) )
			));
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
	header_length = server->remote_header_length;

	header[MX_NETWORK_MAGIC]          = mx_htonl(MX_NETWORK_MAGIC_VALUE);
	header[MX_NETWORK_HEADER_LENGTH]  = mx_htonl(header_length);
	header[MX_NETWORK_MESSAGE_TYPE]   = mx_htonl(MX_NETMSG_ADD_CALLBACK);
	header[MX_NETWORK_STATUS_CODE]    = mx_htonl(MXE_SUCCESS);
	header[MX_NETWORK_DATA_TYPE]      = mx_htonl(MXFT_ULONG);


	mx_network_update_message_id( &(server->last_rpc_message_id) );

	header[MX_NETWORK_MESSAGE_ID] = mx_htonl(server->last_rpc_message_id);

	/*--- Then, fill in the message body. ---*/

	uint32_message = header
		+ ( mx_remote_header_length(server) / sizeof(uint32_t) );

	uint32_message[0] = mx_htonl( nf->record_handle );
	uint32_message[1] = mx_htonl( nf->field_handle );
	uint32_message[2] = mx_htonl( supported_callback_types );

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

	uint32_message = header
		+ ( mx_remote_header_length(server) / sizeof(uint32_t) );

	char_message = message_buffer->u.char_buffer
				+ mx_remote_header_length(server);

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

	MXW_UNUSED( message_length );

	/* If we got an error status code from the server, return the
	 * error message to the caller.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long) status_code, fname,
				"%s", char_message );
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

	MXW_UNUSED( data_type );
	MXW_UNUSED( message_id );

	/* Get the callback ID from the returned message. */

	callback_id = mx_ntohl( uint32_message[0] );

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,
	("%s: The callback ID of type %lu for handle (%lu,%lu) is %#lx",
		fname, supported_callback_types,
		nf->record_handle, nf->field_handle,
		callback_id ));
#endif

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mx_network_get_nf_label( server_record, nf->nfname,
					nf_label, sizeof(nf_label) );

		fprintf( stderr, "MX ADD_CALLBACK('%s') = %#lx\n",
				nf_label, callback_id );
	}

	/* Create a new client-side callback structure. */

	callback_ptr = malloc( sizeof(MX_CALLBACK) );

	if ( callback_ptr == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_CALLBACK structure" );
	}

	callback_ptr->callback_class    = MXCBC_NETWORK;
	callback_ptr->supported_callback_types
					= supported_callback_types;
	callback_ptr->callback_id       = callback_id;
	callback_ptr->active            = FALSE;
	callback_ptr->usage_count	= 0;	/* Not used for remote cb. */
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

	/* If requested, return the new callback object to the caller. */

	if ( callback_object != (MX_CALLBACK **) NULL ) {
		*callback_object = callback_ptr;
	}

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_remote_field_delete_callback( MX_CALLBACK *callback )
{
	static const char fname[] = "mx_remote_field_delete_callback()";

	MX_NETWORK_FIELD *nf;
	MX_RECORD *server_record;
	MX_NETWORK_SERVER *server;
	MX_LIST_HEAD *list_head;
	char nf_label[80];
	MX_LIST *callback_list;
	MX_LIST_ENTRY *callback_list_entry;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	uint32_t *header, *uint32_message;
	char *char_message;
	unsigned long header_length, message_length, message_type, status_code;
	mx_status_type mx_status;

	if ( callback == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CALLBACK pointer passed was NULL." );
	}

	if ( callback->callback_class != MXCBC_NETWORK ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"MX_CALLBACK %p is not a remote field callback.  "
		"Instead, it is of callback class %lu",
			callback, callback->callback_class );
	}

	nf = callback->u.network_field;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_FIELD pointer for callback %p is NULL.",
			callback );
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

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,
		("\n*** DELETE CALLBACK for callback id %#lx, field '%s'.",
			(unsigned long) callback->callback_id,
			mx_network_get_nf_label( nf->server_record,
						nf->nfname,
						nf_label, sizeof(nf_label) )
		));
	}

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mx_network_get_nf_label( nf->server_record, nf->nfname,
					nf_label, sizeof(nf_label) );

		fprintf( stderr, "MX DELETE_CALLBACK('%s', %#lx)\n",
			nf_label, (unsigned long) callback->callback_id );
	}

	/* Find the message buffer for this server. */

	message_buffer = server->message_buffer;

	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_MESSAGE_BUFFER for server '%s' is NULL.",
			server_record->name );
	}

	/* Construct a message to the server asking it to delete a callback. */

	/*--- First, construct the header. ---*/

	header = message_buffer->u.uint32_buffer;
	header_length = server->remote_header_length;

	header[MX_NETWORK_MAGIC]          = mx_htonl(MX_NETWORK_MAGIC_VALUE);
	header[MX_NETWORK_HEADER_LENGTH]  = mx_htonl(header_length);
	header[MX_NETWORK_MESSAGE_TYPE]   = mx_htonl(MX_NETMSG_DELETE_CALLBACK);
	header[MX_NETWORK_STATUS_CODE]    = mx_htonl(MXE_SUCCESS);
	header[MX_NETWORK_DATA_TYPE]      = mx_htonl(MXFT_ULONG);


	mx_network_update_message_id( &(server->last_rpc_message_id) );

	header[MX_NETWORK_MESSAGE_ID] = mx_htonl(server->last_rpc_message_id);

	/*--- Then, fill in the message body. ---*/

	uint32_message = header
		+ ( mx_remote_header_length(server) / sizeof(uint32_t) );

	uint32_message[0] = mx_htonl( callback->callback_id );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( 1 * sizeof(uint32_t) );

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

	uint32_message = header
		+ ( mx_remote_header_length(server) / sizeof(uint32_t) );

	char_message = message_buffer->u.char_buffer
				+ mx_remote_header_length(server);

	/* Read the header of the returned message. */

	header_length  = mx_ntohl( header[MX_NETWORK_HEADER_LENGTH] );
	message_length = mx_ntohl( header[MX_NETWORK_MESSAGE_LENGTH] );
	message_type   = mx_ntohl( header[MX_NETWORK_MESSAGE_TYPE] );
	status_code    = mx_ntohl( header[MX_NETWORK_STATUS_CODE] );

	if ( message_type != mx_server_response(MX_NETMSG_DELETE_CALLBACK) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Message type for response was not %#lx.  "
		"Instead it was of type = %#lx.",
	        (unsigned long) mx_server_response(MX_NETMSG_DELETE_CALLBACK),
		    (unsigned long) message_type );
	}

	MXW_UNUSED( message_length );

	/* If we got an error status code from the server, return the
	 * error message to the caller.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long) status_code, fname,
				"%s", char_message );
	}

	/* If we get here, then we have successfully deleted the callback
	 * in the server.  We must now delete all references to it here
	 * on the client side.
	 */

	/* Find the callback list for the local MX_NETWORK_SERVER object
	 * that corresponds to this server.
	 */

	callback_list = server->callback_list;

	if ( callback_list == (MX_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The callback_list pointer for server '%s' is NULL.",
			server_record->name );
	}

	/* Find the callback in the callback list and delete it. */

	mx_status = mx_list_find_list_entry( callback_list, callback,
						&callback_list_entry );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_list_delete_entry( callback_list, callback_list_entry );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_list_entry_destroy( callback_list_entry );

	/* Null out the contents of the MX_CALLBACK structure to make sure
	 * that anybody that still has a pointer to it will crash when they
	 * dereference the pointer.  This makes it easier to detect the
	 * presence of bugs in memory allocation and management.
	 */

	memset( callback, 0, sizeof(MX_CALLBACK) );

	/* Finish by freeing the MX_CALLBACK object itself. */

	mx_free( callback );

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxp_local_field_traverse_function( MX_LIST_ENTRY *list_entry,
					void *input_argument,
					void **output_argument )
{
	static const char fname[] = "mxp_local_field_traverse_function()";

	MX_CALLBACK_SOCKET_HANDLER_INFO *csh_info;
	MX_SOCKET_HANDLER *socket_handler;
	long mx_status_code;

	if ( list_entry == (MX_LIST_ENTRY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIST_ENTRY pointer passed was NULL." );
	}
	if ( input_argument == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET_HANDLER pointer passed was NULL.");
	}
	if ( output_argument == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pointer for the MX_LIST_ENTRY that matches "
		"the MX_SOCKET_HANDLER is NULL." );
	}

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,
	("%s: list_entry = %p, input_argument = %p, output_argument = %p",
		fname, list_entry, input_argument, output_argument));
#endif

	socket_handler = (MX_SOCKET_HANDLER *) input_argument;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: socket_handler = %p", fname, socket_handler));
#endif

	csh_info = (MX_CALLBACK_SOCKET_HANDLER_INFO *)
			list_entry->list_entry_data;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: csh_info = %p", fname, csh_info));

	MX_DEBUG(-2, ("%s: csh_info->socket_handler = %p, socket_handler = %p",
		fname, csh_info->socket_handler, socket_handler));
#endif

	if ( csh_info->socket_handler == socket_handler ) {
		*output_argument = list_entry;

#if MX_CALLBACK_DEBUG
		mx_status_code = MXE_EARLY_EXIT;
#else
		mx_status_code = MXE_EARLY_EXIT | MXE_QUIET;
#endif
		return mx_error( mx_status_code, fname,
		"Found the list entry for the requested socket handler." );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxp_local_field_find_socket_handler_in_list(
			MX_LIST *callback_socket_handler_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_LIST_ENTRY **callback_socket_handler_list_entry )
{
	static const char fname[] =
		"mxp_local_field_find_socket_handler_in_list()";

	void *list_entry_ptr;
	mx_status_type mx_status;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: list_entry_ptr = %p", fname, list_entry_ptr));
	MX_DEBUG(-2,("%s: callback_socket_handler_list_entry = %p",
		fname, callback_socket_handler_list_entry));
#endif

	mx_status = mx_list_traverse( callback_socket_handler_list,
				mxp_local_field_traverse_function,
				socket_handler,
				&list_entry_ptr );

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: mx_status.code = %ld", fname, mx_status.code));
#endif

	if ( mx_status.code == MXE_EARLY_EXIT ) {

		/* The structure containing the socket handler was found. */

		*callback_socket_handler_list_entry = list_entry_ptr;

#if MX_CALLBACK_DEBUG
		MX_DEBUG(-2,
	("%s: list_entry_ptr = %p, *callback_socket_handler_list_entry = %p",
			fname, list_entry_ptr,
			*callback_socket_handler_list_entry ));
#endif

		return MX_SUCCESSFUL_RESULT;

	}

	if ( mx_status.code == MXE_SUCCESS ) {
		return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
		"Socket handler %p was not found "
		"in callback socket handler list %p",
			socket_handler, callback_socket_handler_list );
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_local_field_add_socket_handler_to_callback(
			MX_RECORD_FIELD *record_field,
			unsigned long supported_callback_types,
			mx_status_type ( *callback_function )(
						MX_CALLBACK *, void * ),
			MX_SOCKET_HANDLER *socket_handler,
			MX_CALLBACK **callback_object )
{
	static const char fname[] =
		"mx_local_field_add_socket_handler_to_callback()";

	MX_LIST *callback_socket_handler_list;
	MX_LIST_ENTRY *callback_socket_handler_list_entry;
	MX_CALLBACK_SOCKET_HANDLER_INFO *csh_info;
	unsigned long old_supported_callback_types;
	mx_status_type mx_status;

	/* See if a callback of this type already exists. */

	mx_status = mx_local_field_find_old_callback( record_field,
						&old_supported_callback_types,
						NULL,
						callback_function,
						NULL,
						callback_object );

	if ( mx_status.code == MXE_SUCCESS ) {

		/* mx_local_field_find_old_callback() successfully
		 * found an existing callback.
		 */

		if ( socket_handler == NULL ) {

			/* If the socket handler is a NULL pointer, then
			 * this means that this reference of the callback
			 * is not associated with an MX socket.
			 *
			 * If that is the case, then there is no point
			 * in trying to add a nonexistent socket handler
			 * to the socket handler list, so we return
			 * without doing anything further.
			 */

			return MX_SUCCESSFUL_RESULT;
			
		} else {
			callback_socket_handler_list =
				(*callback_object)->callback_argument;

			/* Is this socket handler already in the list? */

			mx_status = mxp_local_field_find_socket_handler_in_list(
					callback_socket_handler_list,
					socket_handler,
					&callback_socket_handler_list_entry );

			if ( mx_status.code == MXE_SUCCESS ) {

#if 0
				MX_DEBUG(-2,
		("%s: Found matching callback socket handler list entry = %p",
				    fname, callback_socket_handler_list_entry));
#endif
				/* The socket handler info struct is
				 * already in the list, so we must
				 * increment its usage count.
				 */

				csh_info =
			    callback_socket_handler_list_entry->list_entry_data;

			    	csh_info->usage_count++;

			} else
			if ( mx_status.code == MXE_NOT_FOUND ) {

				/* If the socket_handler is not already in
				 * the list, then we allocate memory for an
				 * MX_CALLBACK_SOCKET_HANDLER_INFO structure
				 * and then add that to the list.
				 */

				csh_info =
			    malloc( sizeof(MX_CALLBACK_SOCKET_HANDLER_INFO) );

			    	if ( csh_info == NULL ) {
					return mx_error(
					MXE_OUT_OF_MEMORY, fname,
					"Ran out of memory trying to allocate "
					"an MX_CALLBACK_SOCKET_HANDLER_INFO "
					"structure." );
				}

				csh_info->callback = *callback_object;
				csh_info->socket_handler = socket_handler;
				csh_info->usage_count = 1;

				/* Add the csh_info structure to the list. */

				mx_status = mx_list_entry_create(
					&callback_socket_handler_list_entry,
					csh_info, NULL );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				mx_status = mx_list_add_entry(
					callback_socket_handler_list,
					callback_socket_handler_list_entry );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			} else {
				/* An unexpected error occurred. */

				return mx_status;
			}
		}
	} else
	if ( mx_status.code == MXE_NOT_FOUND ) {

		/* If we get here, then we did _not_ find an already
		 * existing callback that matches.  Thus, we must
		 * create a new callback.
		 */
							
		/* Create a new list of socket handlers for this callback. */

		mx_status = mx_list_create( &callback_socket_handler_list );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		csh_info = NULL;

		if ( socket_handler == NULL ) {

			/* If the socket handler that was passed to us
			 * was NULL, then there is nothing to add to the
			 * new callback socket handler list, so we leave
			 * the list empty.
			 */

		} else {
			/* If the socket handler that was passed to us
			 * was not NULL, then we must allocate memory
			 * for an MX_CALLBACK_SOCKET_HANDLER_INFO
			 * structure and then add that to the list.
			 */

			csh_info =
			    malloc( sizeof(MX_CALLBACK_SOCKET_HANDLER_INFO) );

			if ( csh_info == NULL ) {
				return mx_error(
				MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to allocate "
				"an MX_CALLBACK_SOCKET_HANDLER_INFO "
				"structure." );
			}

			csh_info->socket_handler = socket_handler;
			csh_info->usage_count = 1;

			/* Add the csh_info structure to the list. */

			mx_status = mx_list_entry_create(
					&callback_socket_handler_list_entry,
						csh_info, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_list_add_entry(
					callback_socket_handler_list,
					callback_socket_handler_list_entry );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Create the callback. */

		mx_status = mx_local_field_add_new_callback( record_field,
						supported_callback_types,
						callback_function,
						callback_socket_handler_list,
						callback_object );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( csh_info != NULL ) {
			csh_info->callback = *callback_object;
		}
	} else {
		/* We got an unexpected error, so return that error
		 * to the caller.
		 */

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_local_field_add_new_callback( MX_RECORD_FIELD *record_field,
			unsigned long supported_callback_types,
			mx_status_type ( *callback_function )(
						MX_CALLBACK *, void * ),
			void *callback_argument,
			MX_CALLBACK **callback_object )
{
	static const char fname[] = "mx_local_field_add_new_callback()";

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

#if MX_CALLBACK_DEBUG_CALLBACK_POINTERS
	{
		int is_valid;

		is_valid = mx_pointer_is_valid( callback_function,
						sizeof(callback_function),
						R_OK );

		if ( is_valid == FALSE ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Callback function pointer %p is not valid.",
				callback_function );
		}

		if ( callback_argument != NULL ) {
			is_valid = mx_pointer_is_valid( callback_argument,
						sizeof(callback_argument),
						R_OK | W_OK );

			if ( is_valid == FALSE ) {
				return mx_error(
				MXE_CORRUPT_DATA_STRUCTURE, fname,
				"Callback argument pointer %p is not valid.",
					callback_argument );
			}
		}
	}
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
		mx_status = mx_initialize_callback_support( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	callback_handle_table = list_head->server_callback_handle_table;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,
	("%s: callback_handle_table = %p, record_field->callback_list = %p",
		fname, callback_handle_table, record_field->callback_list));
#endif

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

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: callback_list = %p", fname, callback_list));
#endif

	/* Allocate an MX_CALLBACK structure to contain the callback info. */

	callback_ptr = malloc( sizeof(MX_CALLBACK) );

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: callback_ptr = %p", fname, callback_ptr));
#endif

	if ( callback_ptr == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to allocate an MX_CALLBACK structure.");
	}

	callback_ptr->callback_class    = MXCBC_FIELD;
	callback_ptr->supported_callback_types
					= supported_callback_types;
	callback_ptr->active            = FALSE;
	callback_ptr->usage_count	= 1;	/* New cb has only 1 user. */
	callback_ptr->get_new_value	= FALSE;
	callback_ptr->first_callback    = TRUE;
	callback_ptr->timer_interval	= record_field->timer_interval;
	callback_ptr->callback_function = callback_function;
	callback_ptr->callback_argument = callback_argument;
	callback_ptr->u.record_field    = record_field;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: callback_ptr = %p, usage_count = %lu",
		fname, callback_ptr, callback_ptr->usage_count));
#endif

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: callback_ptr->callback_function = %p",
		fname, callback_ptr->callback_function));
	MX_DEBUG(-2,("%s: callback_ptr->callback_argument = %p",
		fname, callback_ptr->callback_argument));
#endif

	/* Add this callback to the server's callback handle table. */

#if MX_CALLBACK_PROTECT_VM_HANDLE_TABLE
	mx_callback_handle_table_change_permissions( callback_handle_table,
							R_OK | W_OK );
#endif

	mx_status = mx_create_handle( &callback_handle,
					callback_handle_table, callback_ptr );

#if MX_CALLBACK_PROTECT_VM_HANDLE_TABLE
	mx_callback_handle_table_change_permissions( callback_handle_table, 0 );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: callback_ptr %p was added to "
		"callback_handle_table %p with handle %lu",
		fname, callback_ptr, callback_handle_table, callback_handle));
#endif

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

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: callback_ptr %p was added to callback_list %p "
		"of record field '%s.%s' with list_entry = %p",
		fname, callback_ptr, callback_list,
		record->name, record_field->name, list_entry ));
#endif

	/* If requested, return the new callback object to the caller. */

	if ( callback_object != (MX_CALLBACK **) NULL ) {
		*callback_object = callback_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_local_field_delete_callback( MX_CALLBACK *callback )
{
	static const char fname[] = "mx_local_field_delete_callback()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"This function is not yet implemented for callback %p", callback );
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_local_field_find_old_callback( MX_RECORD_FIELD *record_field,
			unsigned long *supported_callback_types,
			uint32_t      *callback_id,
			mx_status_type ( *callback_function )(
						MX_CALLBACK *, void * ),
			void          *callback_argument,
			MX_CALLBACK **callback_object )
{
	static const char fname[] = "mx_local_field_find_old_callback()";

	MX_CALLBACK *callback_ptr;
	MX_LIST *record_field_callback_list;
	MX_LIST_ENTRY *list_start, *list_entry;
	MX_RECORD *record;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}
	if ( callback_object == (MX_CALLBACK **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CALLBACK pointer passed was NULL." );
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

	record_field_callback_list = record_field->callback_list;

	if ( record_field_callback_list == (MX_LIST *) NULL ) {
		*callback_object = NULL;

		return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
		"Record field '%s.%s' does not have any callbacks.",
			record->name, record_field->name );
	}

	list_start = record_field_callback_list->list_start;

	/* If the list is empty, then return a "not found" status. */

	if ( list_start == NULL ) {
		return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
			"Did not find the requested callback." );
	}

	/* Go through the list looking for a matching list entry.
	 *
	 * Jump to the end of the loop using 'continue' if the
	 * entry does not match.
	 */

	list_entry = list_start;

	do {
		callback_ptr = list_entry->list_entry_data;

		if ( callback_ptr == NULL )
			continue;

		if ( supported_callback_types != NULL ) {
		    if ( (*supported_callback_types)
		    		!= callback_ptr->supported_callback_types )
		    {
		    	continue;
		    }
		}

		if ( callback_id != NULL ) {
		    if ( (*callback_id) != callback_ptr->callback_id )
		    	continue;
		}

		if ( callback_function != NULL ) {
		    if ( callback_function != callback_ptr->callback_function )
			continue;
		}

		if ( callback_argument != NULL ) {
		    if ( callback_argument != callback_ptr->callback_argument )
			continue;
		}

		/* If we get here, we have a match, so increment
		 * the usage count.
		 */

		callback_ptr->usage_count++;

#if MX_CALLBACK_DEBUG
		MX_DEBUG(-2,("%s: callback_ptr = %p, usage_count = %lu",
			fname, callback_ptr, callback_ptr->usage_count));
#endif

		/* Return the callback object to the caller. */

		*callback_object = callback_ptr;

		return MX_SUCCESSFUL_RESULT;

	} while ( (list_entry = list_entry->next_list_entry) != list_start );

	return mx_error( MXE_NOT_FOUND | MXE_QUIET, fname,
		"Did not find the requested callback." );
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_local_field_invoke_callback_list( MX_RECORD_FIELD *record_field,
				unsigned long callback_type )
{
	static const char fname[] = "mx_local_field_invoke_callback_list()";

	MX_LIST *record_field_callback_list;
	MX_LIST_ENTRY *list_start, *list_entry, *next_list_entry;
	MX_CALLBACK *callback;
	mx_bool_type get_new_value;
	mx_status_type mx_status;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	if ( callback_type == MXCBT_POLL ) {
		get_new_value = TRUE;
	} else {
		get_new_value = FALSE;
	}

#if MX_CALLBACK_DEBUG_INVOKE_CALLBACK
	MX_DEBUG(-2,
	("%s invoked for field '%s', callback_type = %lu, get_new_value = %d",
		fname, record_field->name, callback_type, get_new_value));
#endif

	record_field_callback_list = record_field->callback_list;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: get_new_value = %d",
				fname, (int) get_new_value));
	MX_DEBUG(-2,("%s: record_field_callback_list = %p",
				fname, record_field_callback_list));
#endif

	if ( record_field_callback_list == (MX_LIST *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Walk through the list of callbacks. */

	list_start = record_field_callback_list->list_start;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: list_start = %p", fname, list_start));
#endif

	if ( list_start == (MX_LIST_ENTRY *) NULL ) {
		mx_warning("%s: The list_start entry for callback list %p "
		"used by record field '%s.%s' is NULL.", fname,
			record_field_callback_list,
			record_field->record->name, record_field->name );

		return MX_SUCCESSFUL_RESULT;
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
				list_entry, record_field_callback_list,
				record_field->record->name,
				record_field->name );
		}

		callback = list_entry->list_entry_data;

		if ( callback != (MX_CALLBACK *) NULL ) {
			
			if ( callback->callback_function != NULL ) {
				/* Invoke the callback. */

				mx_status = mx_invoke_callback( callback,
								callback_type,
								get_new_value );

				/* If the callback is invalid, remove the
				 * callback from the callback list.
				 */

				if ( mx_status.code == MXE_INVALID_CALLBACK ) {

					mx_status = mx_list_delete_entry(
						record_field_callback_list,
						list_entry );

					if ( mx_status.code == MXE_SUCCESS ) {
						mx_list_entry_destroy(
							list_entry );
					}

					/* If the callback we just deleted
					 * was the list_start callback, then
					 * we must load the new value of the
					 * list_start callback.
					 */

					list_start =
				    record_field_callback_list->list_start;

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

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_function_add_callback( MX_RECORD *record_list,
			mx_status_type ( *callback_function )(
					MX_CALLBACK_MESSAGE * ),
			mx_status_type ( *callback_destructor )(
					MX_CALLBACK_MESSAGE * ),
			void *callback_argument,
			double callback_interval,
			MX_CALLBACK_MESSAGE **callback_message )
{
	static const char fname[] = "mx_function_add_callback()";

	MX_CALLBACK_MESSAGE *callback_message_ptr;
	MX_LIST_HEAD *list_head;
	MX_VIRTUAL_TIMER *oneshot_timer;
	mx_status_type mx_status;

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The record list pointer passed was NULL." );
	}
	if ( callback_function == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The callback function pointer passed was NULL." );
	}

#if MX_CALLBACK_DEBUG_CALLBACK_POINTERS
	{
		int is_valid;

		is_valid = mx_pointer_is_valid( callback_function,
						sizeof(callback_function),
						R_OK );

		if ( is_valid == FALSE ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Callback function pointer %p is not valid.",
				callback_function );
		}

		if ( callback_argument != NULL ) {
			is_valid = mx_pointer_is_valid( callback_argument,
						sizeof(callback_argument),
						R_OK | W_OK );

			if ( is_valid == FALSE ) {
				return mx_error(
				MXE_CORRUPT_DATA_STRUCTURE, fname,
				"Callback argument pointer %p is not valid.",
					callback_argument );
			}
		}
	}
#endif

	if ( callback_interval <= 0.0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The callback interval %g is not a legal callback interval.  "
		"The callback interval must be a positive number.",
			callback_interval );
	}

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record list %p is NULL.  "
		"This should not be able to happen.", record_list );
	}

	if ( list_head->callback_pipe == NULL ) {
		/* We are not configured to handle callbacks, so we
		 * must return an error.
		 */

		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Virtual timer callbacks are not enabled for this MX process.");
	}

	callback_message_ptr = malloc( sizeof(MX_CALLBACK_MESSAGE) );

	if ( callback_message_ptr == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_CALLBACK_MESSAGE object.");
	}

	callback_message_ptr->callback_type = MXCBT_FUNCTION;
	callback_message_ptr->list_head = list_head;

	callback_message_ptr->u.function.callback_function = callback_function;

	callback_message_ptr->u.function.callback_destructor =
							callback_destructor;

	callback_message_ptr->u.function.callback_args = callback_argument;

	/* Specify the callback interval in seconds. */

	callback_message_ptr->u.function.callback_interval = callback_interval;

	/* Create a one-shot interval timer that will arrange for the
	 * callback function to be called later.
	 */

	mx_status = mx_virtual_timer_create( &oneshot_timer,
					list_head->master_timer,
					MXIT_ONE_SHOT_TIMER,
					mx_callback_standard_vtimer_handler,
					callback_message_ptr );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( callback_message_ptr );

		return mx_status;
	}

	callback_message_ptr->u.function.oneshot_timer = oneshot_timer;

	/* Start the callback virtual timer. */

	mx_status = mx_virtual_timer_start( oneshot_timer,
			callback_message_ptr->u.function.callback_interval );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_virtual_timer_destroy( oneshot_timer );
		mx_free( callback_message_ptr );

		return mx_status;
	}

	/* If requested, return the new callback message to the caller. */

	if ( callback_message != (MX_CALLBACK_MESSAGE **) NULL ) {
		*callback_message = callback_message_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_function_delete_callback( MX_CALLBACK_MESSAGE *callback_message )
{
	static const char fname[] = "mx_function_delete_callback()";

	MX_VIRTUAL_TIMER *oneshot_timer;
	mx_status_type (*callback_destructor)(MX_CALLBACK_MESSAGE *);
	mx_status_type mx_status;

#if 0
	MX_DEBUG(-2,("%s invoked for callback message %p",
		fname, callback_message));
#endif

	if ( callback_message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CALLBACK_MESSAGE pointer passed was NULL." );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	oneshot_timer = callback_message->u.function.oneshot_timer;

	if ( oneshot_timer != (MX_VIRTUAL_TIMER *) NULL ) {
		mx_status = mx_virtual_timer_destroy( oneshot_timer );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	callback_destructor = callback_message->u.function.callback_destructor;

	if ( callback_destructor != NULL ) {
		mx_status = (*callback_destructor)( callback_message );
	}

	mx_free( callback_message );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_invoke_callback( MX_CALLBACK *callback,
		unsigned long callback_type,
		mx_bool_type get_new_value )
{
	mx_status_type (*function)( MX_CALLBACK *, void * );
	void *argument;
	mx_status_type mx_status;

#if MX_CALLBACK_DEBUG
	static const char fname[] = "mx_invoke_callback()";

	MX_DEBUG(-2,("%s invoked for callback %p, ID = %#lx, "
		"callback_type = %lu, get_new_value = %d",
		fname, callback, (unsigned long) callback->callback_id,
		callback_type, get_new_value));
#endif

	if ( callback == (MX_CALLBACK *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If this is a network value changed callback, then copy the values
	 * from the callback's network message to the local record field
	 * (if any).
	 */

	if ( (callback->callback_class == MXCBC_NETWORK)
	  && (callback->supported_callback_types & MXCBT_VALUE_CHANGED)
	  && (callback_type == MXCBT_VALUE_CHANGED) )
	{
		MX_NETWORK_FIELD *nf;

		nf = callback->u.network_field;

		if ( nf != (MX_NETWORK_FIELD *) NULL ) {

			if ( ( nf->local_field != (MX_RECORD_FIELD *) NULL )
			  && ( nf->do_not_copy_buffer_on_callback == FALSE ) )
			{
#if MX_CALLBACK_DEBUG
				MX_DEBUG(-2,
			    ("%s: Copying message for network field '%s:%s'",
			    		fname, nf->server_record->name,
					nf->nfname));
#endif
				mx_status = mx_network_copy_message_to_field(
					nf->server_record, nf->local_field );
			}
		}
	}

	callback->callback_type = callback_type;
	callback->get_new_value = get_new_value;

	function = callback->callback_function;
	argument = callback->callback_argument;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: function = %p, argument = %p",
		fname, function, argument));
#endif

	if ( function == NULL ) {

#if MX_CALLBACK_DEBUG
		MX_DEBUG(-2,("%s: Aborting since function == NULL.", fname));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: callback->active = %d",
			fname, (int)callback->active));
#endif

	/* If get_new_value is TRUE, then mx_process_record_field()
	 * will be invoked for the field.  Then, if the new field
	 * value triggers the value_changed threshold, then the
	 * function mx_local_field_invoke_callback_list() will be
	 * called to forward the change to all interested client
	 * programs.  However, invoking the callback list leads us
	 * back here to mx_invoke_callback(), so we must set an
	 * 'active' flag to make sure that we do not cause an
	 * infinite recursion.
	 */

	if ( callback->active ) {

#if MX_CALLBACK_DEBUG
		MX_DEBUG(-2,
		("%s: Aborting since callback->active != 0", fname));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,
	("%s: REALLY invoking callback function %p", fname, function));
#endif
	callback->active = TRUE;

	mx_status = (*function)( callback, argument );

	callback->active = FALSE;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,
	("%s: Returned from callback function %p, mx_status.code = %ld",
		fname, function, mx_status.code));
#endif
	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_delete_callback( MX_CALLBACK *callback )
{
	static const char fname[] = "mx_delete_callback()";

	mx_status_type mx_status;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s invoked for callback = %p", fname, callback ));
#endif

	if ( callback == (MX_CALLBACK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CALLBACK pointer passed was NULL.");
	}

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: callback_id = %#lx",
			fname, (unsigned long) callback->callback_id));
#endif

	switch( callback->callback_class ) {
	case MXCBC_NETWORK:
		mx_status = mx_remote_field_delete_callback( callback );
		break;
	case MXCBC_FIELD:
		mx_status = mx_local_field_delete_callback( callback );
		break;
	default:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"MX_CALLBACK %p has an unsupported callback class of %lu",
			callback, callback->callback_class );
		break;
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

mx_status_type
mx_process_callbacks( MX_RECORD *record_list, MX_PIPE *callback_pipe )
{
	static const char fname[] = "mx_process_callbacks()";

	MX_LIST_HEAD *list_head;
	MX_CALLBACK_MESSAGE *callback_message;
	mx_status_type (*cb_function)( MX_CALLBACK_MESSAGE *);
	size_t bytes_read;
	mx_status_type mx_status;

#if MX_CALLBACK_DEBUG_PROCESS_CALLBACKS_TIMING
	MX_HRT_TIMING total_processing_time_measurement;

	MX_HRT_START( total_processing_time_measurement );
#endif

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s invoked for %p.", fname, record_list));
#endif
	/* You can actually pass any record to this function and not just
	 * the list head record, since mx_get_record_list_head_struct()
	 * will do the work of finding the real list head record.
	 */

	list_head = mx_get_record_list_head_struct( record_list );

	MXW_UNUSED( list_head );

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

#if MX_CALLBACK_DEBUG_PROCESS
	MX_DEBUG(-2,("%s: type = %ld, message = %p",
		fname, callback_message->callback_type, callback_message ));
#endif

	/* We do different things depending on the type of callback message. */

	switch( callback_message->callback_type ) {
	case MXCBT_POLL:
		/* Poll all value changed callback handlers. */

		mx_status = mx_poll_callback_handler( callback_message );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
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

#if MX_CALLBACK_DEBUG_PROCESS_CALLBACKS_TIMING
	MX_HRT_END( total_processing_time_measurement );

	MX_HRT_RESULTS( total_processing_time_measurement,
		fname, "total processing time" );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT unsigned long
mx_callback_handle_table_change_permissions(
				MX_HANDLE_TABLE *callback_handle_table,
				unsigned long new_permission_flags )
{
	MX_HANDLE_STRUCT *handle_struct_array;
	size_t region_size_in_bytes;
	unsigned long old_permission_flags;

	if ( callback_handle_table == (MX_HANDLE_TABLE *) NULL ) {
		return (-1);
	}

	handle_struct_array = callback_handle_table->handle_struct_array;

	if ( handle_struct_array == (MX_HANDLE_STRUCT *) NULL ) {
		return (-1);
	}

	region_size_in_bytes = callback_handle_table->num_blocks
				* callback_handle_table->block_size
				* sizeof(MX_HANDLE_STRUCT);

	mx_vm_get_protection( handle_struct_array, 0,
				NULL, &old_permission_flags );

	mx_vm_set_protection( handle_struct_array,
				region_size_in_bytes,
				new_permission_flags );

	return old_permission_flags;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT void
mx_callback_standard_vtimer_handler( MX_VIRTUAL_TIMER *vtimer, void *args )
{
	static const char fname[] = "mx_callback_standard_vtimer_handler()";

	MX_CALLBACK_MESSAGE *callback_message;
	MX_LIST_HEAD *list_head;

	callback_message = args;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s invoked for vtimer = %p, args = %p",
		fname, vtimer, args));
#endif

	list_head = callback_message->list_head;

	if ( list_head->callback_pipe == (MX_PIPE *) NULL ) {
		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"The callback pipe for this process has not been created." );

		return;
	}

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: writing vtimer %p callback_message %p to MX pipe %p",
		fname, vtimer, callback_message, list_head->callback_pipe));
#endif

	(void) mx_pipe_write( list_head->callback_pipe,
				(char *) &callback_message,
				sizeof(MX_CALLBACK_MESSAGE *) );

	return;
}

/*--------------------------------------------------------------------------*/

/* mx_poll_callback_handler() polls all record fields that
 * currently have value changed callback handlers.
 */

MX_EXPORT mx_status_type
mx_poll_callback_handler( MX_CALLBACK_MESSAGE *callback_message )
{
	static const char fname[] = "mx_poll_callback_handler()";

	MX_LIST_HEAD *list_head;
	MX_HANDLE_TABLE *handle_table;
	MX_HANDLE_STRUCT *handle_struct;
	signed long handle;
	MX_CALLBACK *callback;
	MX_RECORD_FIELD *record_field;
	MX_RECORD *record;
	mx_bool_type get_new_value, send_value_changed_callback;
	unsigned long i, array_size;
	long interval;
	mx_status_type mx_status;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s invoked for callback message %p.",
		fname, callback_message));
#endif
	if ( callback_message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CALLBACK_MESSAGE pointer passed was NULL." );
	}

#if MX_CALLBACK_FIXUP_CORRUPTED_POLL_MESSAGE
	/*-------------------------------------------------------------------*/
	/* FIXME!!! - This is a kludge and should be replaced by a real fix. */
	/*                                                                   */
	/*            The problem with this technique is that it is not      */
	/*            reentrant or thread safe, since there can only be      */
	/*            one mxp_saved_mx_list_head pointer in a given process. */
	{
		MX_CALLBACK_MESSAGE *saved_callback_message;
		MX_VIRTUAL_TIMER *vtimer;

		if ( mxp_saved_mx_list_head == (MX_LIST_HEAD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The mxp_saved_mx_list_head pointer is NULL." );
		}

#if 0
		/* Hulk smash! */

		callback_message = (MX_CALLBACK_MESSAGE *) 0xdeadbeef;
#endif

		/* If the callback message pointer passed to us has been
		 * corrupted, then replace it with a copy of the pointer
		 * that was saved in the list head.
		 */

		saved_callback_message = (MX_CALLBACK_MESSAGE *)
				mxp_saved_mx_list_head->poll_callback_message;

		if ( callback_message != saved_callback_message ) {
			vtimer = saved_callback_message->u.poll.oneshot_timer;

			if ( vtimer == (MX_VIRTUAL_TIMER *) NULL ) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				fname, "The virtual timer pointer for the "
				"saved poll callback %p is NULL.",
				saved_callback_message );
			}

			mx_warning( "Replacing the corrupted MX poll callback "
			"message pointer %p with a copy %p saved in the "
			"MX list head.",
				callback_message, saved_callback_message );

			callback_message = saved_callback_message;

			vtimer->callback_args = saved_callback_message;
		}
	}
	/*-------------------------------------------------------------------*/
#endif /*MX_CALLBACK_FIXUP_CORRUPTED_POLL_MESSAGE */

	list_head = callback_message->list_head;

	list_head->num_poll_callbacks++;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: list_head->num_poll_callbacks = %lu",
		fname, list_head->num_poll_callbacks));
#endif

	/* Restart the callback virtual timer.  This is done here to make
	 * sure that the timer is restarted, since a failure later on in
	 * this callback function might otherwise prevent the timer from
	 * being restarted.  Since this poll callback is executed in the
	 * main thread and the callback pipe is read in the main thread,
	 * there is no danger of multiple copies of this callback being
	 * run at the same time.  In addition, there is no danger of the
	 * callback pipe filling up with poll callback messages.
	 */

	mx_status = mx_virtual_timer_restart( list_head->callback_timer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_CALLBACK_DEBUG
	MX_DEBUG(-2,("%s: virtual timer %p restarted.",
		fname, list_head->callback_timer));
#endif

	/*---*/

	handle_table = list_head->server_callback_handle_table;

	if ( handle_table == (MX_HANDLE_TABLE *) NULL ) {
#if MX_CALLBACK_DEBUG
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

#if MX_CALLBACK_PROTECT_VM_HANDLE_TABLE
	    mx_callback_handle_table_change_permissions( handle_table, R_OK );
#endif

	    handle   = handle_struct->handle;
	    callback = handle_struct->pointer;

#if MX_CALLBACK_PROTECT_VM_HANDLE_TABLE
	    mx_callback_handle_table_change_permissions( handle_table, 0 );
#endif

	    if ( ( handle == MX_ILLEGAL_HANDLE )
	      || ( callback == NULL ) )
	    {
		/* Skip unused handles. */

		continue;
	    }

#if 0
	    MX_DEBUG(-2,
    ("%s: handle_struct_array[%lu] = %p, handle = %ld, callback = %p",
			fname, i, handle_struct, handle, callback));
#endif

	    /* If this callback has a timer interval,
	     * check to see if this is the right time
	     * to invoke the callback.
	     */

	    interval = callback->timer_interval;

	    if ( interval > 0 ) {
		unsigned long num_polls;

		num_polls = list_head->num_poll_callbacks;
#if 1
		MX_DEBUG(-2,
		("%s: callback = %p, interval = %ld, num_polls = %lu",
				fname, callback, interval, num_polls));
#endif
		if ( (num_polls % interval) != 0 ) {
		    /* This is the wrong time,
		     * so skip this callback.
		     */

		    continue;
		}
#if 1
		MX_DEBUG(-2,("%s: will perform callback %p",
				fname, callback));
#endif
	    }
#if 0
	    MX_DEBUG(-2,("%s: callback->callback_function = %p",
			fname, callback->callback_function));
#endif
	    /* If a callback function is defined, then we invoke it
	     * via mx_invoke_callback().
	     *
	     * We must decide whether or not a new value will be
	     * fetched from the hardware and store that decision
	     * in the variable get_new_value.
	     */

	    if ( callback->callback_function != NULL ) {

	    	record_field = callback->u.record_field;

		if ( record_field == (MX_RECORD_FIELD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"No MX_RECORD_FIELD pointer was specified for "
			"callback %p", callback );
		}

		record = record_field->record;

		if ( record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Callbacks are not supported for temporary "
			"record fields.  Field name = '%s'.",
				record_field->name );
		}

		/* If this is the first time that this callback
		 * has been invoked, then we unconditionally 
		 * get a new value from the hardware.
		 */

		if ( callback->first_callback ) {
		    callback->first_callback = FALSE;

		    get_new_value = TRUE;

		/* Else, if this is a local field callback, we
		 * check to see if the MXFF_POLL flag is set.
		 * If MXFF_POLL is set, then we fetch a new
		 * value from the hardware.  Otherwise, we
		 * do not.
		 *
		 * MX client programs can modify the state
		 * of the MXFF_POLL flag by setting the
		 * MX_NETWORK_ATTRIBUTE_POLL (2) attribute
		 * via the mx_network_field_set_attribute()
		 * function.
		 */
		 
		} else
		if ( callback->callback_class == MXCBC_FIELD ) {
		    if ( record_field->flags & MXFF_POLL ) {
		    	get_new_value = TRUE;
		    } else {
		    	get_new_value = FALSE;
		    }

		/* If none of the above conditions apply, then
		 * we will not get a new value from the hardware.
		 */

		} else {
		    get_new_value = FALSE;
		}

		/* Do we need to get a new field value? */

		if ( get_new_value ) {

			/* Process the record field to get the new value.
			 *
			 * mx_process_record_field() will automatically
			 * send out any value changed messages that are
			 * necessary.
			 */

			record = record_field->record;

#if MX_CALLBACK_DEBUG
			MX_DEBUG(-2,("%s: Processing '%s.%s'",
			fname, record->name, record_field->name));
#endif
			mx_status = mx_process_record_field(
						record, record_field,
						MX_PROCESS_GET, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else {
			/* We do _not_ process the record field, but we _do_ 
			 * check to see if the contents of the field have 
			 * changed since the last time that we looked.
			 */
					
			mx_status = mx_test_for_value_changed( record_field,
						MX_PROCESS_GET,
						&send_value_changed_callback );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( send_value_changed_callback ) {

				/* mx_test_for_value_changed() does _not_
				 * automatically send out any value changed
				 * messages, so we must explicitly do that
				 * here.
				 */

				mx_status = mx_invoke_callback( callback,
						MXCBT_VALUE_CHANGED, FALSE );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		}
	    }
	}

	return MX_SUCCESSFUL_RESULT;
}

