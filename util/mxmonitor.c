/*
 * Name:    mxmonitor.c
 *
 * Purpose: MX utility for monitoring MX server value changed callbacks.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007-2014, 2017, 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_unistd.h"
#include "mx_array.h"
#include "mx_net.h"
#include "mx_callback.h"
#include "mx_key.h"
#include "mx_debugger.h"
#include "n_tcpip.h"
#include "n_unix.h"

/*-------------------------------------------------------------------------*/

static int client_callback_num_beeps = 0;

static mx_status_type
client_callback_function( MX_CALLBACK *callback, void *argument )
{
	MX_RECORD *server_record;
	MX_NETWORK_FIELD *nf;
	MX_RECORD_FIELD *local_field;
	MX_TCPIP_SERVER *tcpip_server;
	char server_id[500];
	char value_string[200];
	int i;
	mx_status_type mx_status;

	if ( callback->callback_class != MXCBC_NETWORK ) {
		/* Skip all callbacks that are not network callbacks. */

		return MX_SUCCESSFUL_RESULT;
	}

	/* If you replaced the NULL in the mx_remote_field_add_callback()
	 * call made in the main routine with a pointer to a custom value,
	 * then that pointer is returned to the callback routine via
	 * the void pointer called 'argument' above.
	 */

	nf = callback->u.network_field;

	local_field = nf->local_field;

	server_record = nf->server_record;

	tcpip_server = server_record->record_type_struct;

	snprintf( server_id, sizeof(server_id),
			"%s@%ld", tcpip_server->hostname, tcpip_server->port );

	mx_status = mx_create_description_from_field( NULL, local_field,
				value_string, sizeof(value_string) );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	for( i = 0; i < client_callback_num_beeps; i++ ) {

#if defined(OS_WIN32)
		Beep( 440, 200 );
#else
		fprintf( stderr, "\a" );
#endif

		mx_msleep( 200 );
	}

	mx_info( "callback %#lx: '%s:%s' = '%s'",
		(unsigned long) callback->callback_id,
		server_id, nf->nfname, value_string );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
add_network_field( MX_RECORD **server_record,
			char *network_field_id,
			unsigned long network_debug_flags )
{
#if 0
	static const char fname[] = "add_network_field()";
#endif

	char display_buffer[200];

	char server_name[ MXU_HOSTNAME_LENGTH+1 ];
	char server_arguments[ MXU_SERVER_ARGUMENTS_LENGTH+1 ];
	char record_name[ MXU_RECORD_NAME_LENGTH+1 ];
	char field_name[ MXU_FIELD_NAME_LENGTH+1 ];
	MX_CALLBACK *callback;
	MX_RECORD *record_list;
	MX_LIST_HEAD *list_head_struct;
	MX_NETWORK_FIELD *nf;
	int num_items, server_port;
	unsigned long server_flags;
	MX_RECORD_FIELD *local_field;
	void *value_ptr;
	mx_status_type mx_status;

	mx_status = mx_parse_network_field_id( network_field_id,
			server_name, MXU_HOSTNAME_LENGTH,
			server_arguments, MXU_SERVER_ARGUMENTS_LENGTH,
			record_name, sizeof(record_name) - 1,
			field_name, sizeof(field_name) - 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: server_name = '%s', server_arguments = '%s'",
		fname, server_name, server_arguments));

	MX_DEBUG(-2,("%s: record_name = '%s', field_name = '%s'",
		fname, record_name, field_name));
#endif

	num_items = sscanf( server_arguments, "%d", &server_port );

	if ( num_items != 1 ) {
		server_port = 9727;
	}

	server_flags = MXF_NETWORK_SERVER_BLOCKING_IO
			| MXF_NETWORK_SERVER_QUIET_RECONNECTION;

	if ( network_debug_flags & MXF_NETDBG_SUMMARY ) {
		server_flags |= MXF_NETWORK_SERVER_DEBUG_SUMMARY;
	}
	if ( network_debug_flags & MXF_NETDBG_MSG_IDS ) {
		server_flags |= MXF_NETWORK_SERVER_DEBUG_MESSAGE_IDS;
	}
	if ( network_debug_flags & MXF_NETDBG_VERBOSE ) {
		server_flags |= MXF_NETWORK_SERVER_DEBUG_VERBOSE;
	}

	/* Connect to the MX server. */

	mx_status = mx_connect_to_mx_server( server_record,
				server_name, server_port, 60.0, server_flags);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	record_list = (*server_record)->list_head;

	list_head_struct = mx_get_record_list_head_struct( record_list );

	list_head_struct->network_debug_flags = network_debug_flags;

	mx_status = mx_set_program_name( record_list, "mxmonitor" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Create a new MX_NETWORK_FIELD. */

	mx_status = mx_create_network_field( &nf, *server_record,
					record_name, field_name, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the starting value of the network field. */

	local_field = nf->local_field;

	value_ptr = mx_get_field_value_pointer( local_field );

	mx_status = mx_get_array( nf, local_field->datatype,
				local_field->num_dimensions,
				local_field->dimension,
				value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Display the starting value of the network field. */

	mx_status = mx_create_description_from_field( NULL, local_field,
			display_buffer, sizeof(display_buffer) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,
	("%s: Starting value = '%s'", fname, display_buffer));
#endif

	/* Create a callback handler for this network field.
	 *
	 * If you want to pass a custom value to the callback,
	 * replace the NULL argument below with a pointer
	 * to the custom value.
	 */

	mx_status = mx_remote_field_add_callback( nf,
				MXCBT_VALUE_CHANGED,
				client_callback_function,
				NULL,
				&callback );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: Created new callback %#lx",
		fname, (unsigned long) callback->callback_id ));
#endif

	mx_info( "New callback %#lx: '%s@%s:%s' = '%s'",
		(unsigned long) callback->callback_id,
		server_name, server_arguments,
		nf->nfname, display_buffer );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
callback_list_traverse( MX_RECORD *record_list,
			mx_bool_type show_server_label,
			void *argument,
			mx_status_type (*traverse_fn)
				(MX_LIST *, unsigned long *, void *) )
{
	static const char fname[] = "callback_list_traverse()";

	MX_LIST_HEAD *list_head;
	MX_RECORD *current_record;
	MX_NETWORK_SERVER *server;
	MX_TCPIP_SERVER *tcpip_server;
	MX_UNIX_SERVER *unix_server;
	MX_LIST *cb_list;
	unsigned long callback_number;
	mx_status_type mx_status;

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer is NULL!" );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	callback_number = 0;

	current_record = record_list->next_record;

	while ( current_record != record_list ) {
		if ( current_record->mx_class == MXN_NETWORK_SERVER ) {
			if ( show_server_label ) {
				switch( current_record->mx_type ) {
				case MXN_NET_TCPIP:
					tcpip_server =
					    current_record->record_type_struct;

					mx_info(
					"Server '%s' at host '%s' port %lu:",
						current_record->name,
						tcpip_server->hostname,
						tcpip_server->port );
					break;
				case MXN_NET_UNIX:
					unix_server =
					    current_record->record_type_struct;

					mx_info( "Server '%s' at path '%s':",
						current_record->name,
						unix_server->pathname );
					break;
				}
			}

			server = current_record->record_class_struct;

			cb_list = server->callback_list;

			if ( cb_list->num_list_entries > 0 ) {
				mx_status = traverse_fn( cb_list,
							&callback_number,
							argument );
			}
		}

		current_record = current_record->next_record;
	}

	/* Suppress GCC set but not used error. */

#if ( defined(__GNUC__) && ( ! defined(__clang__) ) )
	mx_status = mx_status;
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static void
add_new_callback( MX_RECORD *record_list )
{
	char network_field_id[200];
	MX_LIST_HEAD *list_head;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	list_head = mx_get_record_list_head_struct( record_list );

	mx_info_entry_dialog( "Enter new callback name --> ",
			NULL, TRUE, network_field_id, sizeof(network_field_id));

	mx_status = add_network_field( &record_list,
					network_field_id,
					list_head->network_debug_flags );

	/* Suppress GCC set but not used error. */

#if ( defined(__GNUC__) && ( ! defined(__clang__) ) )
	mx_status = mx_status;
#endif

	return;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
scod_traverse_fn( MX_LIST *callback_list,
		unsigned long *callback_number,
		void *argument )
{
	static const char fname[] = "scod_traverse_fn()";

	MX_LIST_ENTRY *list_entry, *next_list_entry;
	MX_CALLBACK *callback;
	unsigned long callback_identifier;
	mx_bool_type delete_callback, deleted_last_entry;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	callback_identifier = *((unsigned long *) argument );

	if ( callback_list->list_start == (MX_LIST_ENTRY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"callback_list %p has a NULL list_start member.",
			callback_list );
	}

	list_entry = callback_list->list_start;

	do {
		callback = list_entry->list_entry_data;

		if ( callback->callback_class == MXCBC_NETWORK ) {

			delete_callback = FALSE;

			deleted_last_entry = FALSE;

			/* See if the callback identifier matches. */

			/* If callback_identifier >= 0x80000000, then the
			 * callback identifier is a callback id.  Otherwise,
			 * it is a callback number from this program's 'l'
			 * or list command.
			 */

			if ( callback_identifier
					>= MX_NETWORK_MESSAGE_IS_CALLBACK )
			{
				if ( callback_identifier
						== callback->callback_id )
				{
					delete_callback = TRUE;

					mx_info( "Deleting callback ID %#lx",
						callback_identifier );
				}
			} else {
				if ( callback_identifier == (*callback_number) )				{
					delete_callback = TRUE;

					mx_info( "Deleting callback number %lu "
					"(callback ID %#lx)",
						callback_identifier,
					(unsigned long) callback->callback_id );
				}

			}

			(*callback_number)++;

			next_list_entry = list_entry->next_list_entry;

			if ( delete_callback ) {
				/* We are going to delete this callback.
				 * This will also remove this list entry
				 * from the callback list, so we must 
				 * handle the finding of the next list
				 * entry with care.
				 */

				if ( list_entry == next_list_entry ) {
					deleted_last_entry = TRUE;
				}

				mx_status = mx_remote_field_delete_callback(
								callback );
			}

			if ( deleted_last_entry ) {
				return MX_SUCCESSFUL_RESULT;
			}

			list_entry = next_list_entry;
		} else {
			list_entry = list_entry->next_list_entry;
		}

	} while ( list_entry != callback_list->list_start );

	/* Suppress GCC set but not used error. */

#if ( defined(__GNUC__) && ( ! defined(__clang__) ) )
	mx_status = mx_status;
#endif

	return MX_SUCCESSFUL_RESULT;
}

static void
select_callback_to_delete( MX_RECORD *record_list )
{
	char buffer[40];
	char *endptr;
	unsigned long selected_callback;

	mx_info_entry_dialog( "Select callback to delete --> ",
				NULL, TRUE, buffer, sizeof(buffer) );

	selected_callback = strtoul( buffer, &endptr, 0 );

	if ( *endptr != '\0' ) {
		mx_info( "Illegal callback number '%s' requested.", buffer );
	} else {
		callback_list_traverse( record_list, FALSE,
				&selected_callback, scod_traverse_fn );
	}

	return;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
lc_traverse_fn( MX_LIST *callback_list,
		unsigned long *callback_number,
		void *argument )
{
	static const char fname[] = "lc_traverse_fn()";

	MX_LIST_ENTRY *list_start, *list_entry;
	MX_CALLBACK *callback;
	MX_NETWORK_FIELD *nf;

	list_start = callback_list->list_start;

	if ( list_start == (MX_LIST_ENTRY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"callback_list %p has a NULL list_start member.",
			callback_list );
	}

	list_entry = list_start;

	do {
		callback = list_entry->list_entry_data;

		if ( callback->callback_class == MXCBC_NETWORK ) {
			nf = callback->u.network_field;

			mx_info( "%3lu - Callback %#lx for nf '%s'",
					*callback_number,
					(unsigned long) callback->callback_id,
					nf->nfname );

			(*callback_number)++;
		}

		list_entry = list_entry->next_list_entry;

	} while ( list_entry != list_start );

	return MX_SUCCESSFUL_RESULT;
}

static void
list_callbacks_function( MX_RECORD *record_list )
{
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	mx_info( "==== Current Callbacks ====" );

	mx_status = callback_list_traverse( record_list, TRUE,
						NULL, lc_traverse_fn );

	mx_info( "===========================" );

	/* Suppress GCC set but not used error. */

#if ( defined(__GNUC__) && ( ! defined(__clang__) ) )
	mx_status = mx_status;
#endif

	return;
}

/*-------------------------------------------------------------------------*/

static void
check_for_interactive_command( MX_RECORD *record_list )
{
	char c;

	if ( mx_kbhit() ) {
		c = mx_getch();

		switch( c ) {
		case 'a':
			add_new_callback( record_list );
			break;
		case 'd':
			select_callback_to_delete( record_list );
			break;
		case 'l':
			list_callbacks_function( record_list );
			break;
		case 'q':
			mx_info( "Exiting ..." );
			exit(0);
			break;
		}
	}

	return;
}

/*-------------------------------------------------------------------------*/

static void
timestamp_output_function( char *string )
{
	time_t time_struct;
	struct tm *current_time;
	char time_buffer[80];

	time( &time_struct );

	current_time = localtime( &time_struct );

	strftime( time_buffer, sizeof(time_buffer),
			"%b %d %H:%M:%S ", current_time );

	fprintf( stderr, "%s%s\n", time_buffer, string );
}

/*-------------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
	static const char fname[] = "main()";

	MX_RECORD *record_list;
	MX_RECORD *server_record;
	int c;
	mx_bool_type start_debugger, interactive, show_timestamp;
	unsigned long i, network_debug_flags;
	double timeout;
	mx_status_type mx_status;

	if ( argc < 2 ) {
		fprintf( stderr, "\nUsage: %s network_field_name\n\n", argv[0]);
		exit(1);
	}

	record_list = NULL;

	network_debug_flags = 0;
	start_debugger = FALSE;
	interactive = TRUE;
	show_timestamp = FALSE;

	while ( (c = getopt(argc, argv, "aA:b:BDitx")) != -1 ) {
		switch (c) {
		case 'a':
			network_debug_flags |= MXF_NETDBG_SUMMARY;
			break;
		case 'A':
			network_debug_flags =
				mx_hex_string_to_unsigned_long( optarg );
			break;

			/* Note: client_callback_num_beeps is a global variable
			 * declared just before the callback function at the
			 * start of this file.
			 */
		case 'b':
			client_callback_num_beeps = atoi( optarg );
			break;
		case 'B':
			client_callback_num_beeps = 1;
			break;
		case 'D':
			start_debugger = TRUE;
			break;
		case 'i':
			interactive = FALSE;
			break;
		case 't':
			show_timestamp = TRUE;
			break;
		case 'x':
			putenv("MX_DEBUGGER=xterm -e gdbtui -p %lu");
			break;
		}
	}

	if ( start_debugger ) {
		mx_start_debugger(NULL);
	}

	mx_set_debug_level(0);

	if ( show_timestamp ) {
		mx_set_info_output_function( timestamp_output_function );
		mx_set_warning_output_function( timestamp_output_function );
		mx_set_error_output_function( timestamp_output_function );
		mx_set_debug_output_function( timestamp_output_function );
	}

	server_record = NULL;

	for ( i = optind; i < argc; i++ ) {

		mx_status = add_network_field( &server_record, argv[i],
							network_debug_flags );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );
	}

	if ( server_record == (MX_RECORD *) NULL ) {
		mx_status = mx_error( MXE_NOT_FOUND, fname,
		"No server records were created, so we are exiting." );

		exit( mx_status.code );
	}

	record_list = server_record->list_head;

	/* Wait indefinitely for callbacks. */

	timeout = 1.0;		/* in seconds */

	for(;;) {
		mx_status = mx_network_wait_for_messages(record_list, timeout);

		switch( mx_status.code ) {
		case MXE_SUCCESS:
		case MXE_TIMED_OUT:
			break;
		default:
			exit( mx_status.code );
		}

		if ( interactive ) {
			check_for_interactive_command( record_list );
		}

		mx_usleep(1);
	}

#if !defined(OS_SOLARIS)
	return 0;
#endif
}

