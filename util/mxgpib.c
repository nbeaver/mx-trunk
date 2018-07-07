/*
 * Name:    mxgpib.c
 *
 * Purpose: This program allows the user to communicate with the requested
 *          GPIB address on a GPIB bus controlled by an MX server.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_signal.h"
#include "mx_record.h"
#include "mx_multi.h"
#include "mx_net.h"
#include "mx_gpib.h"

static char DEFAULT_HOST[] = "localhost";

#define DEFAULT_PORT	9727

mx_status_type
mxgpib_connect_to_mx_server( MX_RECORD **record_list,
				MX_RECORD **server_record,
				char *server_name,
				int server_port,
				int default_display_precision,
				mx_bool_type network_debugging );

mx_status_type
mxgpib_find_port_record( MX_RECORD **port_record,
				MX_RECORD *record_list,
				MX_RECORD *server_record,
				char *port_name );

static void
mxgpib_termination_handler( int signal_number )
{
	mx_info("*** mxgpib shutting down ***");

	exit(0);
}

static void
mxgpib_install_signal_and_exit_handlers( void )
{
	mx_setup_standard_signal_error_handlers();

#ifdef SIGINT
	signal( SIGINT, mxgpib_termination_handler );
#endif
#ifdef SIGHUP
	signal( SIGHUP, mxgpib_termination_handler );
#endif
#ifdef SIGTERM
	signal( SIGTERM, mxgpib_termination_handler );
#endif
	return;
}

static unsigned long
mxgpib_parse_hex( char *buffer )
{
	char *endptr;
	unsigned long value;

	value = strtoul( buffer, &endptr, 16 );

	if ( *endptr != '\0' ) {
		value = 0;
	}

	return value;
}

int
main( int argc, char *argv[] )
{
	static const char fname[] = "mxgpib";

	MX_RECORD *record_list, *server_record, *gpib_record;
	char *ptr, *server_name;
	int server_port;
	char *gpib_record_name;
	int gpib_address;
	char gpib_buffer[1000];

	unsigned long newline;
	char newline_chars[4];

	int i, debug_level, num_non_option_arguments;
	int default_display_precision;
	mx_bool_type start_debugger;
	mx_bool_type network_debugging;
	mx_status_type mx_status;

	int argc_addr;
	char **argv_addr;

	static char usage[] =
"\n"
"Usage: %s { options } port_address\n"
"\n"
"    where port_address is\n"
"\n"
"          gpib_name:port_address\n"
"      or  hostname:gpib_name:port_address\n"
"      or  hostname@port:gpib_name:port_address\n"
"\n"
"    and options are\n"
"\n"
"      -d debug_level\n"
"      -n newline_chars      (in hex; default = 0x0d0a)\n"
"\n"
"Example: %s localhost@9727:gpib0:2\n"
"\n";

	int c, error_flag;

	mx_status = mx_initialize_runtime();

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr,
		"%s: Unable to initialize the MX runtime environment.\n",
			argv[0] );

		exit(1);
	}

#if ! defined( OS_WIN32 )
	mxgpib_install_signal_and_exit_handlers();
#endif

	debug_level = 0;

	default_display_precision = 8;

	newline = mxgpib_parse_hex( "0x0d0a" );

	start_debugger = FALSE;

	network_debugging = FALSE;

	/* Process command line arguments via getopt, if any. */

	error_flag = FALSE;

	while ((c = getopt(argc, argv, "ad:Dn:x")) != -1 ) {
		switch(c) {
		case 'a':
			network_debugging = TRUE;
			break;
		case 'd':
			debug_level = atoi( optarg );
			break;
		case 'D':
			start_debugger = TRUE;
			break;
		case 'n':
			newline = mxgpib_parse_hex( optarg );
			break;
		case 'x':
			putenv("MX_DEBUGGER=xterm -e gdbtui -p %lu");
			break;
		case '?':
			error_flag = TRUE;
			break;
		}
	}

	i = optind;

	num_non_option_arguments = argc - optind;

	if ( start_debugger ) {
		mx_breakpoint();
	}

	newline_chars[0] = (char) (( newline >> 24 ) & 0xff);
	newline_chars[1] = (char) (( newline >> 16 ) & 0xff);
	newline_chars[2] = (char) (( newline >> 8 ) & 0xff);
	newline_chars[3] = (char) (newline & 0xff);

	if ( num_non_option_arguments != 1 ) {
		error_flag = TRUE;
	}

	if ( error_flag ) {
		fprintf( stderr, usage, fname, fname );
		exit(1);
	}

	/* Parse the record address. */

	mx_string_split( argv[i], ":", &argc_addr, &argv_addr );

	if ( argc_addr == 2 ) {
		server_name = DEFAULT_HOST;
		server_port = DEFAULT_PORT;
		gpib_record_name = argv_addr[0];
		gpib_address = atoi( argv_addr[1] );
	} else
	if ( argc_addr == 3 ) {
		server_name = argv_addr[0];
		gpib_record_name = argv_addr[1];
		gpib_address = atoi( argv_addr[2] );

		ptr = strchr( server_name, '@' );

		if ( ptr == NULL ) {
			server_port = DEFAULT_PORT;
		} else {
			*ptr = '\0';
			ptr++;

			server_port = atoi( ptr );
		}
	} else {
		fprintf( stderr, "%s", usage );
		exit(1);
	}

	mx_set_debug_level( debug_level );

	/* Connect to the remote MX server */

	mx_status = mxgpib_connect_to_mx_server( &record_list, &server_record,
						server_name, server_port,
						default_display_precision,
						network_debugging );

	if ( mx_status.code != MXE_SUCCESS ) {
		exit( (int) mx_status.code );
	}

	/* Find the GPIB port record. */

	mx_status = mxgpib_find_port_record( &gpib_record,
					record_list, server_record,
					gpib_record_name );

	if ( mx_status.code != MXE_SUCCESS ) {
		exit( (int) mx_status.code );
	}

	fprintf( stderr,
	"mxgpib starting for server %s, port %d, GPIB '%s', address %d\n",
		server_name, server_port, gpib_record_name, gpib_address );
	fprintf( stderr, "press ctrl-C to exit\n" );

	while (TRUE) {
		printf( "> " );

		mx_fgets( gpib_buffer, sizeof(gpib_buffer), stdin );

		mx_gpib_putline( gpib_record, gpib_address,
					gpib_buffer, NULL, 0x0 );
		mx_msleep(250);
		mx_gpib_getline( gpib_record, gpib_address,
					gpib_buffer, sizeof(gpib_buffer),
					NULL, 0x0 );

		fprintf( stderr, "  %s\n", gpib_buffer );
	}

	return 0;
}

mx_status_type
mxgpib_connect_to_mx_server( MX_RECORD **record_list,
				MX_RECORD **server_record,
				char *server_name,
				int server_port,
				int default_display_precision,
				mx_bool_type network_debugging )
{
	static const char fname[] = "mxgpib_connect_to_mx_server()";

	MX_LIST_HEAD *list_head_struct;
	static char description[MXU_RECORD_DESCRIPTION_LENGTH + 1];
	mx_status_type mx_status;

	/* Initialize MX device drivers. */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set up a record list to put this server record in. */

	*record_list = mx_initialize_database();

	if ( *record_list == NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to setup an MX record list." );
	}

	/* If requested, turn on network debugging. */

	if ( network_debugging ) {
		mx_multi_set_debug_flags( *record_list, MXF_NETDBG_SUMMARY );
	}

	/* Set the default floating point display precision. */

	list_head_struct = mx_get_record_list_head_struct( *record_list );

	if ( list_head_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX record list head structure is NULL." );
	}

	list_head_struct->default_precision = default_display_precision;

	/* Set the program name. */

	strlcpy( list_head_struct->program_name, "mxgpib",
				MXU_PROGRAM_NAME_LENGTH );

	/* Create a record description for this server. */

	snprintf( description, sizeof(description),
		"remote_server server network tcpip_server \"\" \"\" 0x0 %s %d",
			server_name, server_port );

	mx_status = mx_create_record_from_description(
			*record_list, description, server_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_finish_record_initialization( *server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the record list as ready to be used. */

	list_head_struct->list_is_active = TRUE;
	list_head_struct->fixup_records_in_use = FALSE;

	/* Try to connect to the MX server. */

	mx_status = mx_open_hardware( *server_record );

	if ( mx_status.code != MXE_SUCCESS )
		exit( (int) mx_status.code );

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mxgpib_find_port_record( MX_RECORD **gpib_record,
			MX_RECORD *record_list,
			MX_RECORD *server_record,
			char *gpib_record_name )
{
#if 0
	static const char fname[] = "mxgpib_find_port_record()";
#endif

	static char description[MXU_RECORD_DESCRIPTION_LENGTH + 1];

	mx_status_type mx_status;

	/* Create a record description for the port. */

	snprintf( description, sizeof(description),
    "%s interface gpib network_gpib \"\" \"\" 0 0 0x0 0x0 0x1 %s %s",
		gpib_record_name, server_record->name, gpib_record_name );

	mx_status = mx_create_record_from_description( record_list, description,
						gpib_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_finish_record_initialization( *gpib_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Try to connect to the serial port. */

	mx_status = mx_open_hardware( *gpib_record );

	return mx_status;
}

