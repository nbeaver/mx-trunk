#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_unistd.h"

#include "mx_multi.h"

MX_API char *optarg;
MX_API int optind;

int
main( int argc, char *argv[] )
{
#if 0
	static const char fname[] = "main()";
#endif

	MX_MULTI_NETWORK_VARIABLE *mnv;
	MX_RECORD *mx_record_list;
	MX_RECORD *mx_server_record;
	int c;
	mx_bool_type network_debug, start_debugger;
	mx_status_type mx_status;

	network_debug = FALSE;
	start_debugger = FALSE;

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

	mx_record_list = NULL;

	mx_status = mx_create_empty_database( &mx_record_list );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	if ( network_debug ) {
		mx_multi_set_debug_flag( mx_record_list, TRUE );
	}

	mx_set_debug_level(0);

	mx_server_record = mx_record_list;

	mx_status = mx_multi_create( &mnv, argv[optind],
					(void **) &mx_server_record );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	return 0;
}

