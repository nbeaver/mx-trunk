#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_unistd.h"
#include "mx_array.h"
#include "mx_net.h"
#include "mx_callback.h"
#include "n_tcpip.h"
#include "n_unix.h"

MX_API char *optarg;
MX_API int optind;

static mx_status_type
client_callback_function( MX_CALLBACK *callback, void *argument )
{
	MX_RECORD *server_record;
	MX_NETWORK_FIELD *nf;
	MX_RECORD_FIELD *local_field;
	MX_TCPIP_SERVER *tcpip_server;
	char server_id[500];
	char value_string[200];
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

	mx_info( "callback %#lx: '%s:%s' = '%s'",
		(unsigned long) callback->callback_id,
		server_id, nf->nfname, value_string );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
list_callbacks_function( MX_RECORD *record )
{
	static const char fname[] = "list_callbacks_function()";

	MX_LIST_HEAD *list_head;
	MX_RECORD *record_list, *current_record;
	MX_NETWORK_SERVER *server;
	MX_TCPIP_SERVER *tcpip_server;
	MX_UNIX_SERVER *unix_server;
	MX_LIST *cb_list;
	MX_LIST_ENTRY *list_start, *list_entry;
	MX_CALLBACK *callback;
	MX_NETWORK_FIELD *nf;

	mx_info( "==== Current Callbacks ====" );

	record_list = record->list_head;

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer is NULL!" );
	}

	current_record = record_list->next_record;

	while ( current_record != record_list ) {
		if ( current_record->mx_class == MXN_NETWORK_SERVER ) {
			switch( current_record->mx_type ) {
			case MXN_NET_TCPIP:
				tcpip_server =
					current_record->record_type_struct;

				mx_info( "Server '%s' at host '%s' port %lu:",
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

			server = current_record->record_class_struct;

			cb_list = server->callback_list;

			list_start = cb_list->list_start;

			list_entry = list_start;

			do {
				callback = list_entry->list_entry_data;

				switch( callback->callback_class ) {
				case MXCBC_NETWORK:
					nf = callback->u.network_field;

					mx_info( "  Callback %#lx for nf '%s'",
					  (unsigned long) callback->callback_id,
						nf->nfname );
					break;
				}
				list_entry = list_entry->next_list_entry;

			} while ( list_entry != list_start );
		}

		current_record = current_record->next_record;
	}

	mx_info( "===========================" );

	return MX_SUCCESSFUL_RESULT;
}

int
main( int argc, char *argv[] )
{
	static const char fname[] = "vcc_monitor";

	char display_buffer[200];

	char server_name[ MXU_HOSTNAME_LENGTH+1 ];
	char server_arguments[ MXU_SERVER_ARGUMENTS_LENGTH+1 ];
	char record_name[ MXU_RECORD_NAME_LENGTH+1 ];
	char field_name[ MXU_FIELD_NAME_LENGTH+1 ];
	MX_CALLBACK *callback;
	MX_RECORD *record_list;
	MX_RECORD *server_record;
	MX_NETWORK_FIELD *nf;
	int c, server_port, num_items;
	mx_bool_type network_debug, start_debugger, list_callbacks;
	unsigned long i, server_flags;
	MX_RECORD_FIELD *local_field;
	void *value_ptr;
	double timeout;
	mx_status_type mx_status;

	if ( argc < 2 ) {
		fprintf( stderr, "\nUsage: %s network_field_name\n\n", argv[0]);
		exit(1);
	}

	record_list = NULL;

	network_debug = FALSE;
	start_debugger = FALSE;
	list_callbacks = FALSE;

	while ( (c = getopt(argc, argv, "ADl")) != -1 )
	{
		switch (c) {
		case 'A':
			network_debug = TRUE;
			break;
		case 'D':
			start_debugger = TRUE;
			break;
		case 'l':
			list_callbacks = TRUE;
			break;
		}
	}

	if ( start_debugger ) {
		mx_start_debugger(NULL);
	}

	mx_set_debug_level(0);

	server_record = NULL;

	for ( i = optind; i < argc; i++ ) {

		mx_status = mx_parse_network_field_id( argv[i],
				server_name, MXU_HOSTNAME_LENGTH,
				server_arguments, MXU_SERVER_ARGUMENTS_LENGTH,
				record_name, sizeof(record_name) - 1,
				field_name, sizeof(field_name) - 1 );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		MX_DEBUG(-2,("%s: server_name = '%s', server_arguments = '%s'",
			fname, server_name, server_arguments));

		MX_DEBUG(-2,("%s: record_name = '%s', field_name = '%s'",
			fname, record_name, field_name));

		num_items = sscanf( server_arguments, "%d", &server_port );

		if ( num_items != 1 ) {
			server_port = 9727;
		}

		if ( network_debug ) {
			server_flags = 0x70000000;
		} else {
			server_flags = 0x30000000;
		}

		/* Connect to the MX server. */

		mx_status = mx_connect_to_mx_server( &server_record,
					server_name, server_port, server_flags);

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		record_list = server_record->list_head;

		mx_status = mx_set_program_name( record_list, "vcc_monitor" );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		/* Create a new MX_NETWORK_FIELD. */

		mx_status = mx_create_network_field( &nf, server_record,
						record_name, field_name, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		/* Get the starting value of the network field. */

		local_field = nf->local_field;

		value_ptr = mx_get_field_value_pointer( local_field );

		mx_status = mx_get_array( nf, local_field->datatype,
					local_field->num_dimensions,
					local_field->dimension,
					value_ptr );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		/* Display the starting value of the network field. */

		mx_status = mx_create_description_from_field( NULL, local_field,
				display_buffer, sizeof(display_buffer) );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		MX_DEBUG(-2,
		("%s: Starting value = '%s'", fname, display_buffer));

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
			exit( mx_status.code );

		MX_DEBUG(-2,("%s: Created new callback %#lx",
			fname, (unsigned long) callback->callback_id ));
	}

	if ( list_callbacks ) {
		list_callbacks_function( record_list );
	}

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
	}

	return 0;
}

