#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_net.h"

MX_API char *optarg;
MX_API int optind;

int
main( int argc, char *argv[] )
{
	static const char fname[] = "mxattr_set";

	char description[ MXU_RECORD_DESCRIPTION_LENGTH+1 ];
	char *description_ptr;

	char server_name[ MXU_HOSTNAME_LENGTH+1 ];
	char server_arguments[ MXU_SERVER_ARGUMENTS_LENGTH+1 ];
	char record_name[ MXU_RECORD_NAME_LENGTH+1 ];
	char field_name[ MXU_FIELD_NAME_LENGTH+1 ];
	char record_field_name[ MXU_RECORD_FIELD_NAME_LENGTH+1 ];
	MX_RECORD *record_list;
	MX_RECORD *server_record;
	MX_LIST_HEAD *list_head;
	char server_record_name[] = "mxserver";
	MX_NETWORK_FIELD nf;
	int c, server_port, num_items;
	mx_bool_type start_debugger;
	unsigned long network_debug_flags;
	unsigned long server_flags;
	char *attribute_name;
	unsigned long attribute_number;
	double attribute_value;
	mx_status_type mx_status;

	if ( argc < 4 ) {
		fprintf( stderr,
"\nUsage: %s network_field_name attribute_name attribute_value\n\n", fname );
		exit(1);
	}

	network_debug_flags = 0;
	start_debugger = FALSE;

	while ( (c = getopt(argc, argv, "aAD")) != -1 )
	{
		switch (c) {
		case 'a':
			network_debug_flags |= MXF_NETWORK_SERVER_DEBUG_SUMMARY;
			break;
		case 'A':
			network_debug_flags |= MXF_NETWORK_SERVER_DEBUG_VERBOSE;
			break;
		case 'D':
			start_debugger = TRUE;
			break;
		}
	}

	if ( start_debugger ) {
		mx_start_debugger(NULL);
	}

	mx_set_debug_level(0);

	mx_status = mx_parse_network_field_id( argv[optind],
				server_name, MXU_HOSTNAME_LENGTH,
				server_arguments, MXU_SERVER_ARGUMENTS_LENGTH,
				record_name, sizeof(record_name) - 1,
				field_name, sizeof(field_name) - 1 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

#if 0
	MX_DEBUG(-2,("%s: server_name = '%s', server_arguments = '%s'",
		fname, server_name, server_arguments));

	MX_DEBUG(-2,("%s: record_name = '%s', field_name = '%s'",
		fname, record_name, field_name));

	MX_DEBUG(-2,("%s: value_change_threshold = %g",
		fname, value_change_threshold));
#endif

	num_items = sscanf( server_arguments, "%d", &server_port );

	if ( num_items != 1 ) {
		server_port = 9727;
	}

	server_flags = MXF_NETWORK_SERVER_BLOCKING_IO;

	server_flags |= network_debug_flags;

	/* Create the MX runtime database. */

	snprintf( description, sizeof(description),
		"%s server network tcpip_server \"\" \"\" %#lx %s %d",
		server_record_name, server_flags, server_name, server_port );

	description_ptr = description;

	mx_status = mx_setup_database_from_array(
				&record_list, 1, &description_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	list_head = mx_get_record_list_head_struct( record_list );

	list_head->network_debug_flags = network_debug_flags;

	mx_status = mx_set_program_name( record_list, "mxattr_set" );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	server_record = mx_get_record( record_list, server_record_name );

	if ( server_record == (MX_RECORD *) NULL ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
			"Record '%s' was not found in the MX database.",
			server_record_name );

		exit( MXE_NOT_FOUND );
	}

	/* Initialize the network field data structure. */

	snprintf( record_field_name, sizeof(record_field_name),
			"%s.%s", record_name, field_name );

	mx_status = mx_network_field_init( &nf, server_record,
						record_field_name );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Set a new value for the requested attribute. */

	attribute_name = argv[optind+1];

	attribute_value = atof( argv[optind+2] );

	num_items = sscanf( attribute_name, "%lu", &attribute_number );

	if ( num_items > 0 ) {
		mx_status = mx_network_field_set_attribute(
				&nf, attribute_number, attribute_value );
	} else {
		mx_status = mx_network_field_set_attribute_by_name(
				&nf, attribute_name, attribute_value );
	}

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	exit(0);
}

