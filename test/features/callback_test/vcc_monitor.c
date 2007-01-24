#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_net.h"
#include "mx_callback.h"

extern char *optarg;
extern int optind;

int
main( int argc, char *argv[] )
{
	static const char fname[] = "vcc_monitor";

	char server_name[ MXU_HOSTNAME_LENGTH+1 ];
	char server_arguments[ MXU_SERVER_ARGUMENTS_LENGTH+1 ];
	char record_name[ MXU_RECORD_NAME_LENGTH+1 ];
	char field_name[ MXU_FIELD_NAME_LENGTH+1 ];
	MX_RECORD *server_record;
	MX_NETWORK_SERVER *server;
	MX_NETWORK_FIELD nf;
	int c, server_port, num_items;
	mx_bool_type network_debug, start_debugger;
	mx_status_type mx_status;

	if ( argc < 2 ) {
		fprintf( stderr, "\nUsage: %s network_field_name\n\n", argv[0]);
		exit(1);
	}

	network_debug = FALSE;
	start_debugger = FALSE;

	fprintf( stderr, "optind = %d\n", optind );

	while ( (c = getopt(argc, argv, "AD")) != -1 )
	{
		switch (c) {
		case 'A':
			network_debug = TRUE;
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

	MX_DEBUG(-2,("%s: server_name = '%s', server_arguments = '%s'",
		fname, server_name, server_arguments));

	MX_DEBUG(-2,("%s: record_name = '%s', field_name = '%s'",
		fname, record_name, field_name));

	num_items = sscanf( server_arguments, "%d", &server_port );

	if ( num_items != 1 ) {
		server_port = 9727;
	}

	mx_status = mx_connect_to_mx_server( &server_record,
				server_name, server_port, 8 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( network_debug ) {
		server->server_flags |= MXF_NETWORK_SERVER_DEBUG;
	}

	mx_status = mx_network_field_init( &nf, server_record, "%s.%s",
						record_name, field_name );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	exit(0);
}

