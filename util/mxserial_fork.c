/*
 * Name:    mxserial_fork.c
 *
 * Purpose: This program allows the user to communicate with an RS-232
 *          port controlled by an MX server.
 *
 * Note:    Since this program uses fork(), it can only be used as is
 *          on Unix-like systems.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>

#include "mx_unistd.h"
#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_key.h"

static char DEFAULT_HOST[] = "localhost";

#ifndef USE_GETOPT
#define USE_GETOPT	TRUE
#endif

#define DEFAULT_PORT	9727

static FILE *to_user = NULL;
static FILE *from_user = NULL;

static pid_t original_pid = 0;

static pid_t other_pid = 0;

mx_status_type
mxser_connect_to_mx_server( MX_RECORD **record_list,
				MX_RECORD **server_record,
				char *server_name,
				int server_port,
				int default_display_precision );

mx_status_type
mxser_find_port_record( MX_RECORD **port_record,
				MX_RECORD *record_list,
				MX_RECORD *server_record,
				char *port_name );

mx_status_type
mxser_send_characters_to_the_server( MX_RECORD *port_record,
				char *newline_chars, FILE *file );

mx_status_type
mxser_receive_characters_from_the_server( MX_RECORD *port_record,
					int max_receive_speed );

static void
mxser_termination_handler( int signal_number )
{
	if ( original_pid == getpid() ) {
		mx_info("*** mxserial shutting down ***");
	}

	if ( other_pid != 0 ) {
		(void) kill( other_pid, SIGTERM );
	}
	exit(0);
}

static void
mxser_install_signal_and_exit_handlers( void )
{
#ifdef SIGILL
	signal( SIGILL, mx_standard_signal_error_handler );
#endif
#ifdef SIGTRAP
	signal( SIGTRAP, mx_standard_signal_error_handler );
#endif
#ifdef SIGIOT
	signal( SIGIOT, mx_standard_signal_error_handler );
#endif
#ifdef SIGBUS
	signal( SIGBUS, mx_standard_signal_error_handler );
#endif
#ifdef SIGFPE
	signal( SIGFPE, mx_standard_signal_error_handler );
#endif
#ifdef SIGSEGV
	signal( SIGSEGV, mx_standard_signal_error_handler );
#endif


#ifdef SIGINT
	signal( SIGINT, mxser_termination_handler );
#endif
#ifdef SIGHUP
	signal( SIGHUP, mxser_termination_handler );
#endif
#ifdef SIGTERM
	signal( SIGTERM, mxser_termination_handler );
#endif
	return;
}

static unsigned long
mxser_parse_hex( char *buffer )
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
	static const char fname[] = "mxserial";

	MX_RECORD *record_list, *server_record, *port_record;
	FILE *file;
	char *ptr, *server_name, *port_name;
	int server_port;
	pid_t child_pid;

	unsigned long newline;
	char newline_chars[4];

	int i, debug_level, num_non_option_arguments;
	int default_display_precision;
	int max_receive_speed;
	mx_bool_type start_debugger;
	mx_status_type mx_status;

	static char usage[] =
"\n"
"Usage: %s { options } port_address\n"
"\n"
"    where port_address is\n"
"\n"
"          port_name\n"
"      or  hostname:port_name\n"
"      or  hostname@port:port_name\n"
"\n"
"    and options are\n"
"\n"
"      -d debug_level\n"
"      -f file_to_send\n"
"      -n newline_chars      (in hex; default = 0x0d0a)\n"
"      -s max_receive_speed  (in bits per second; default = 10000)\n"
"\n"
"Example: %s localhost@9727:theta_rs232\n"
"\n";

#if USE_GETOPT
	int c, error_flag;
#endif

	mx_status = mx_initialize_runtime();

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr,
		"%s: Unable to initialize the MX runtime environment.\n",
			argv[0] );

		exit(1);
	}

	original_pid = getpid();

#if ! defined( OS_WIN32 )
	mxser_install_signal_and_exit_handlers();
#endif

	to_user = stdout;
	from_user = stdin;

	debug_level = 0;

	default_display_precision = 8;

	newline = mxser_parse_hex( "0x0d0a" );

	max_receive_speed = 10000;

	file = NULL;

	start_debugger = FALSE;

#if USE_GETOPT
	/* Process command line arguments via getopt, if any. */

	error_flag = FALSE;

	while ((c = getopt(argc, argv, "d:Df:n:s:")) != -1 ) {
		switch(c) {
		case 'd':
			debug_level = atoi( optarg );
			break;
		case 'D':
			start_debugger = TRUE;
			break;
		case 'f':
			file = fopen( optarg, "r" );

			if ( file == NULL ) {
				fprintf( stderr, "%s: cannot open file '%s'.\n",
						fname, optarg );
				exit(1);
			}
			break;
		case 'n':
			newline = mxser_parse_hex( optarg );
			break;
		case 's':
			max_receive_speed = atoi( optarg );
			break;
		case '?':
			error_flag = TRUE;
			break;
		}
	}

	i = optind;

	num_non_option_arguments = argc - optind;

#else  /* USE_GETOPT */

	i = 1;

	num_non_option_arguments = argc - 1;

#endif /* USE_GETOPT */

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

	ptr = strchr( argv[i], ':' );

	if ( ptr == NULL ) {
		server_name = DEFAULT_HOST;
		server_port = DEFAULT_PORT;
		port_name = argv[i];
	} else {
		*ptr = '\0';
		ptr++;

		port_name = ptr;

		ptr = strchr( argv[1], '@' );

		if ( ptr == NULL ) {
			server_port = DEFAULT_PORT;
		} else {
			*ptr = '\0';
			ptr++;

			server_port = atoi( ptr );
		}
		server_name = argv[i];
	}

	mx_set_debug_level( debug_level );

	/* Fork into two separate processes.  The child process writes
	 * to the server while the parent process reads from the server.
	 */

	child_pid = fork();

	if ( child_pid == 0 ) {
		other_pid = original_pid;
	} else {
		other_pid = child_pid;
	}

	/* Connect to the remote MX server */

	mx_status = mxser_connect_to_mx_server( &record_list, &server_record,
						server_name, server_port,
						default_display_precision );

	if ( mx_status.code != MXE_SUCCESS )
		exit( (int) mx_status.code );

	/* Find the serial port record. */

	mx_status = mxser_find_port_record( &port_record, record_list,
						server_record, port_name );

	if ( mx_status.code != MXE_SUCCESS )
		exit( (int) mx_status.code );

	/* From now on, the child sends characters to the serial port,
	 * while the parent receives characters from the serial port.
	 */

	if ( child_pid == 0 ) {

		mxser_send_characters_to_the_server( port_record,
						newline_chars, file );
	} else {
		mx_info("*** mxserial connected ***");

		mxser_receive_characters_from_the_server( port_record,
							max_receive_speed );
	}

	exit(0);

	return 0;
}

mx_status_type
mxser_connect_to_mx_server( MX_RECORD **record_list,
				MX_RECORD **server_record,
				char *server_name,
				int server_port,
				int default_display_precision )
{
	static const char fname[] = "mxser_connect_to_mx_server()";

	MX_LIST_HEAD *list_head_struct;
	static char description[MXU_RECORD_DESCRIPTION_LENGTH + 1];
	mx_status_type mx_status;

	/* Initialize MX device drivers. */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set up a record list to put this server record in. */

	*record_list = mx_initialize_record_list();

	if ( *record_list == NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to setup an MX record list." );
	}

	/* Set the default floating point display precision. */

	list_head_struct = mx_get_record_list_head_struct( *record_list );

	if ( list_head_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX record list head structure is NULL." );
	}

	list_head_struct->default_precision = default_display_precision;

	/* Set the program name. */

	strlcpy( list_head_struct->program_name, "mxserial",
				MXU_PROGRAM_NAME_LENGTH );

	/* Create a record description for this server. */

	sprintf( description,
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
mxser_find_port_record( MX_RECORD **port_record,
			MX_RECORD *record_list,
			MX_RECORD *server_record,
			char *port_name )
{
#if 0
	static const char fname[] = "mxser_find_port_record()";
#endif

	static char description[MXU_RECORD_DESCRIPTION_LENGTH + 1];

	mx_status_type mx_status;

	/* Create a record description for the port. */

	sprintf( description,
	"%s interface rs232 network_rs232 \"\" \"\" 9600 8 N 1 N 0 0 -1 0 %s %s",
		port_name, server_record->name, port_name );

	mx_status = mx_create_record_from_description( record_list, description,
							port_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_finish_record_initialization( *port_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Try to connect to the serial port. */

	mx_status = mx_open_hardware( *port_record );

	return mx_status;
}

mx_status_type
mxser_receive_characters_from_the_server( MX_RECORD *port_record,
						int max_receive_speed )
{
	char c;
	double sleep_seconds;
	unsigned long sleep_us, num_bytes_available;
	mx_status_type mx_status;

	sleep_seconds = mx_divide_safely( 10.0, (double) max_receive_speed );

	sleep_us = mx_round( 1000000.0 * sleep_seconds );

	for (;;) {
		num_bytes_available = 0;

		while ( num_bytes_available == 0 ) {
			mx_status = mx_rs232_num_input_bytes_available(
					port_record, &num_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_usleep( sleep_us );
		}

		mx_status = mx_rs232_getchar( port_record, &c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		fputc( (int) c, to_user );

		fflush( to_user );
	}

#if ( defined(OS_HPUX) && !defined(__ia64) )
	return MX_SUCCESSFUL_RESULT;
#endif
}

#define MXSERIAL_BUFFER_SIZE	1000

mx_status_type
mxser_send_characters_to_the_server( MX_RECORD *port_record,
					char *newline_chars,
					FILE *file )
{
	char buffer[MXSERIAL_BUFFER_SIZE+1];
	size_t bytes_read, bytes_written;
	char c;
	int i;
	mx_status_type mx_status;

	if ( file != NULL ) {
		for (;;) {
			bytes_read = fread( buffer,
					1, MXSERIAL_BUFFER_SIZE, file );

			if ( bytes_read <= 0 ) {
				fclose( file );
				break;		/* Exit the while() loop. */
			}

			mx_status = mx_rs232_write( port_record, buffer,
						bytes_read, &bytes_written, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( bytes_written != bytes_read ) {
				mx_warning(
	"I/O error: Only %ld bytes of a %ld byte block were written!",
				(long) bytes_written, (long) bytes_read );
			}
		}
	}

	for (;;) {
		if ( mx_kbhit() ) {
			c = mx_getch();

			if ( c != 0xa ) {
				mx_status = mx_rs232_putchar( port_record,
						c, MXF_232_WAIT );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			} else {
				for ( i = 0; i < 4; i++ ) {
					if ( newline_chars[i] != '\0' ) {

					    mx_status = mx_rs232_putchar(
							port_record,
							newline_chars[i],
							MXF_232_WAIT );

					    if (mx_status.code != MXE_SUCCESS)
						return mx_status;
					}
				}
			}
		}
		mx_msleep(1);
	}

#if ( defined(OS_HPUX) && !defined(__ia64) )
	return MX_SUCCESSFUL_RESULT;
#endif
}

