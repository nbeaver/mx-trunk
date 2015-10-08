/*
 * Name:    ms_main.c
 *
 * Purpose: main() routine for the MX server.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MS_MAIN_DEBUG_SIGNALS		FALSE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_time.h"
#include "mx_unistd.h"
#include "mx_version.h"
#include "mx_stdint.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_handle.h"
#include "mx_memory.h"
#include "mx_socket.h"
#include "mx_select.h"
#include "mx_net.h"
#include "mx_pipe.h"
#include "mx_io.h"
#include "mx_key.h"
#include "mx_callback.h"
#include "mx_clock.h"
#include "mx_hrt.h"
#include "mx_cfn.h"
#include "mx_log.h"
#include "mx_syslog.h"
#include "mx_multi.h"
#include "mx_thread.h"
#include "mx_virtual_timer.h"
#include "mx_process.h"
#include "mx_security.h"

#include "ms_mxserver.h"

MX_EVENT_HANDLER mxsrv_event_handler_list[] = {
	{ MXF_SRV_TCP_SERVER_TYPE,
	  "MX server",
	  &mxsrv_tcp_server_socket_struct,
	  mxsrv_mx_server_socket_init,
	  mxsrv_mx_server_socket_process_event,
	  NULL },

#if HAVE_UNIX_DOMAIN_SOCKETS
	{ MXF_SRV_UNIX_SERVER_TYPE,
	  "MX server",
	  &mxsrv_unix_server_socket_struct,
	  mxsrv_mx_server_socket_init,
	  mxsrv_mx_server_socket_process_event,
	  NULL },
#endif

	{ MXF_SRV_MX_CLIENT_TYPE,
	  "MX client",
	  NULL,
	  mxsrv_mx_client_socket_init,
	  mxsrv_mx_client_socket_process_event,
	  mxsrv_mx_client_socket_proc_queued_event },

};

int mxsrv_num_event_handlers = sizeof( mxsrv_event_handler_list )
				/ sizeof( mxsrv_event_handler_list[0] );

/*--------*/

#if defined(DEBUG_DMALLOC)
   unsigned long mainloop_mark;
#endif

/*--------*/

#if defined(DEBUG_MPATROL)
#  include <mpatrol/heapdiff.h>

   static heapdiff mainloop_heapdiff;
#endif

/*--------*/

static void
mxsrv_exit_handler( void )
{
	mx_info( "*** MX server process exiting. ***" );

#if defined(DEBUG_DMALLOC)
	dmalloc_log_changed( mainloop_mark, 1, 1, 1 );
#endif

#if defined(DEBUG_MPATROL)
	heapdiffend( mainloop_heapdiff );
#endif
}

/*--------*/

static int
mxsrv_user_interrupt_function( void )
{
	/* The MX server never has direct user interrupts. */

	return MXF_USER_INT_NONE;
}

static void
mxsrv_sigterm_handler( int signal_number )
{
	mx_info( "Received a request to shutdown via a SIGTERM signal." );

	exit(0);
}

static void
mxsrv_sigint_traceback_handler( int signal_number )
{
	mx_stack_traceback();

	signal( SIGINT, mxsrv_sigint_traceback_handler );
}

static void
mxsrv_sigint_termination_handler( int signal_number )
{
	mx_info( "Received a request to shutdown via a SIGINT signal." );

	exit(0);
}

static void
mxsrv_install_signal_and_exit_handlers( int sigint_displays_traceback )
{
	atexit( mxsrv_exit_handler );

	signal( SIGTERM, mxsrv_sigterm_handler );

	if ( sigint_displays_traceback ) {
		signal( SIGINT, mxsrv_sigint_traceback_handler );
	} else {
		signal( SIGINT, mxsrv_sigint_termination_handler );
	}

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

	return;
}

/*------------------------------------------------------------------*/

/* FIXME: For some reason, with Visual C++ 6.0 under Windows 2000,
 * output to stderr via 'output functions' seems to disappear without
 * a trace if you run the Visual C++ program from a Cygwin session.
 * On the other hand, output to stderr seems to work from high level
 * routines such as main().  However, write()s to file descriptor 2
 * seem to work in both cases.  I have no idea why it is working
 * this way.  For reference, this occurred with Cygwin 1.5.20.
 * 
 * (WML - July 24, 2006 - at APS Sector 10-ID)
 */

#if defined(_MSC_VER)
#  define MXSRV_USE_STDIO_OUTPUT	FALSE
#else
#  define MXSRV_USE_STDIO_OUTPUT	TRUE
#endif

void
mxsrv_print_timestamp( void )
{
	time_t time_struct;
	struct tm current_time;
	char buffer[80];

	time( &time_struct );

	(void) localtime_r( &time_struct, &current_time );

	strftime( buffer, sizeof(buffer),
			"%b %d %H:%M:%S ", &current_time );

#if MXSRV_USE_STDIO_OUTPUT
	fputs( buffer, stderr );
#else
	write( 2, buffer, strlen(buffer) );
#endif

	/* Enable the following if you want to log the ID of this thread. */

#if 0	/* Show thread ID */
	*buffer = '\0';

	mx_thread_id_string( buffer, sizeof(buffer) );

#if MXSRV_USE_STDIO_OUTPUT
	fputs( buffer, stderr );
#else
	write( 2, buffer, strlen(buffer) );
#endif

#endif	/* Show thread ID */

	return;
}

static void
mxsrv_output_function( char *string )
{
	mxsrv_print_timestamp();

#if MXSRV_USE_STDIO_OUTPUT
	fprintf( stderr, "%s\n", string );
#else
	{
		static char n = '\n';

		write( 2, string, strlen(string) );
		write( 2, &n, 1 );
	}
#endif
}

static void
mxsrv_warning_output_function( char *string )
{
	mxsrv_print_timestamp();

#if MXSRV_USE_STDIO_OUTPUT
	fprintf( stderr, "Warning: %s\n", string );
#else
	{
		static char warning[] = "Warning: ";
		static char n = '\n';

		write( 2, warning, strlen(warning) );
		write( 2, string, strlen(string) );
		write( 2, &n, 1 );
	}
#endif
}

static void
mxsrv_setup_output_functions( void )
{
	mx_set_info_output_function( mxsrv_output_function );
	mx_set_warning_output_function( mxsrv_warning_output_function );
	mx_set_error_output_function( mxsrv_output_function );
	mx_set_debug_output_function( mxsrv_output_function );
}

/*------------------------------------------------------------------*/

#if 0

static mx_status_type
mxsrv_poll_all( void )
{
	MX_DRIVER **mx_list_of_types;
	MX_DRIVER *driver_list_ptr;
	long i, num_record_fields;
	MX_RECORD_FIELD_DEFAULTS *defaults_array;
	MX_RECORD_FIELD_DEFAULTS *array_element;

	/* mx_get_driver_lists() returns a 3-element array containing
	 * the superclass list, the class list, and the type list.
	 */

	mx_list_of_types = mx_get_driver_lists();

	/* We are interested in the third element of this array, since
	 * that one is an array that contains all of the drivers found
	 * in mx_driver.c.
	 */

	driver_list_ptr = mx_list_of_types[2];

	/* Walk through all of the device drivers until we get to
	 * the end of the array.
	 */

	while ( driver_list_ptr->mx_type != 0 ) {

		MX_DEBUG( 2,("%s: ", driver_list_ptr->name ));

		num_record_fields = *(driver_list_ptr->num_record_fields);

		defaults_array = *(driver_list_ptr->record_field_defaults_ptr);

		for ( i = 0; i < num_record_fields; i++ ) {
			array_element = &defaults_array[i];

			MX_DEBUG( 2,("   %s", array_element->name));

			array_element->flags |= MXFF_POLL;
		}

		driver_list_ptr++;
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

static mx_status_type
mxsrv_poll_all( void )
{
	MX_DRIVER *current_driver;
	long i, num_record_fields;
	MX_RECORD_FIELD_DEFAULTS *defaults_array;
	MX_RECORD_FIELD_DEFAULTS *array_element;

	/* We are interested in the third element of this array, since
	 * that one is an array that contains all of the drivers found
	 * in mx_driver.c.
	 */

	current_driver = mx_get_driver_by_name( NULL );

	/* Walk through all of the device drivers until we get to
	 * the end of the array.
	 */

	while ( current_driver != (MX_DRIVER *) NULL ) {

		MX_DEBUG(-2,("%s: ", current_driver->name ));

		num_record_fields = *(current_driver->num_record_fields);

		defaults_array = *(current_driver->record_field_defaults_ptr);

		for ( i = 0; i < num_record_fields; i++ ) {
			array_element = &defaults_array[i];

			MX_DEBUG(-2,("   %s", array_element->name));

			array_element->flags |= MXFF_POLL;
		}

		current_driver = current_driver->next_driver;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif

/*------------------------------------------------------------------*/

static void
mxsrv_display_resource_usage( int initialize_flag, double event_interval )
{
	static MX_CLOCK_TICK next_event_tick, event_interval_in_ticks;

	MX_CLOCK_TICK current_tick;
	MX_SYSTEM_MEMINFO system_meminfo;
	MX_PROCESS_MEMINFO process_meminfo;
	mx_status_type mx_status;

	if ( initialize_flag ) {
		event_interval_in_ticks =
			mx_convert_seconds_to_clock_ticks( event_interval );

		next_event_tick = mx_current_clock_tick();

		return;
	}

	current_tick = mx_current_clock_tick();

	if ( mx_compare_clock_ticks( current_tick, next_event_tick ) >= 0 ) {

		next_event_tick = mx_add_clock_ticks( current_tick,
					event_interval_in_ticks );

		mx_status = mx_get_process_meminfo( MXF_PROCESS_ID_SELF,
					       &process_meminfo );

		if ( mx_status.code == MXE_SUCCESS ) {

			mx_status = mx_get_system_meminfo( &system_meminfo );

			if ( mx_status.code == MXE_SUCCESS ) {

				MX_DEBUG(-2,
		    ("MEM: Free = %lu, process = %lu, allocated = %lu",
				(unsigned long) system_meminfo.free_bytes,
				(unsigned long) process_meminfo.total_bytes,
			    (unsigned long) process_meminfo.allocated_bytes ));
			}
		}
	}
	return;
}

/*------------------------------------------------------------------*/

static void
wait_at_exit_fn( void )
{
	fprintf( stderr, "Press any key to exit...\n" );
	mx_getch();
}

/*------------------------------------------------------------------*/

#if HAVE_MAIN_ROUTINE

int
main( int argc, char *argv[] )

#else /* HAVE_MAIN_ROUTINE */

int
mxserver_main( int argc, char *argv[] )

#endif /* HAVE_MAIN_ROUTINE */
{
	static const char fname[] = "main()";

	MX_RECORD *mx_record_list;
	MX_LIST_HEAD *list_head_struct;
	MX_HANDLE_TABLE *handle_table;
	unsigned long handle_table_block_size, handle_table_num_blocks;
	char mx_database_filename[MXU_FILENAME_LENGTH+1];
	char mx_connection_acl_filename[MXU_FILENAME_LENGTH+1];
	char mx_stderr_destination_filename[MXU_FILENAME_LENGTH+1];
	char server_pathname[MXU_FILENAME_LENGTH+1];
	char os_version_string[40];
	char ident_string[80];
	mx_bool_type enable_callbacks;
	int i, debug_level, start_debugger, saved_errno;
	int server_port, default_display_precision, init_hw_flags;
	int install_syslog_handler, syslog_number, syslog_options;
	int display_stack_traceback, redirect_stderr, destination_unbuffered;
	int bypass_signal_handlers, poll_all;
	unsigned long network_debug_flags;
	mx_bool_type enable_remote_breakpoint;
	mx_bool_type wait_for_debugger, just_in_time_debugging;
	mx_bool_type wait_at_exit;
	mx_bool_type monitor_resources;
	double resource_monitor_interval;
	double master_timer_period;
	double vc_poll_callback_interval;
	long delay_microseconds;
	unsigned long default_data_format;
	FILE *new_stderr;

	int max_sockets, handler_array_size;
	MX_SOCKET_HANDLER_LIST socket_handler_list;

	mx_status_type ( *init_fn ) (MX_RECORD *,
					MX_SOCKET_HANDLER_LIST *,
					MX_EVENT_HANDLER * );
	mx_status_type mx_status;

#if HAVE_GETOPT
	int c, error_flag;
#endif

	/* Initialize the parts of the MX runtime environment that do not
	 * depend on the presence of an MX database.
	 */

	mx_status = mx_initialize_runtime();

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr,
		"%s: Unable to initialize MX runtime environment.\n",
			argv[0] );

		exit(1);
	}

	mxsrv_setup_output_functions();

	mx_cfn_construct_config_filename( "mxserver.dat",
			mx_database_filename, MXU_FILENAME_LENGTH );
	mx_cfn_construct_config_filename( "mxserver.acl",
			mx_connection_acl_filename, MXU_FILENAME_LENGTH );

	strlcpy( mx_stderr_destination_filename,
					"", MXU_FILENAME_LENGTH );

#if defined(OS_DJGPP)
	enable_callbacks = FALSE;
#else
	enable_callbacks = TRUE;
#endif

	debug_level = 0;

	start_debugger = FALSE;

	default_display_precision = 8;

	server_port = -1;

	strlcpy( server_pathname, "", sizeof(server_pathname) );

	install_syslog_handler = FALSE;

	syslog_number = -1;
	syslog_options = 0;

	init_hw_flags = 0;

	delay_microseconds = -1L;

	redirect_stderr = FALSE;
	destination_unbuffered = FALSE;

	default_data_format = MX_NETWORK_DATAFMT_ASCII;

	display_stack_traceback = FALSE;

	bypass_signal_handlers = FALSE;

	network_debug_flags = 0;

	enable_remote_breakpoint = FALSE;

	wait_for_debugger = FALSE;
	just_in_time_debugging = FALSE;
	wait_at_exit = FALSE;

	monitor_resources = FALSE;
	resource_monitor_interval = -1.0;	/* in seconds */

	master_timer_period = 0.1;		/* in seconds */

	vc_poll_callback_interval = -1.0;	/* in seconds */

	poll_all = FALSE;

#if HAVE_GETOPT
        /* Process command line arguments, if any. */

        error_flag = FALSE;

        while ((c = getopt(argc, argv,
		"aAab:BcC:d:De:E:f:Jkl:L:m:M:n:p:P:rsStu:v:wxY:Z")) != -1)
	{
                switch (c) {
		case 'a':
			network_debug_flags |= MXF_NETDBG_SUMMARY;
			break;
		case 'A':
			network_debug_flags |= MXF_NETDBG_VERBOSE;
			break;
		case 'b':
			if ( strcmp( optarg, "raw" ) == 0 ) {
				default_data_format = MX_NETWORK_DATAFMT_RAW;
			} else
			if ( strcmp( optarg, "xdr" ) == 0 ) {
				default_data_format = MX_NETWORK_DATAFMT_XDR;
			} else
			if ( strcmp( optarg, "ascii" ) == 0 ) {
				default_data_format = MX_NETWORK_DATAFMT_ASCII;
			} else {
				fprintf( stderr,
	"mxserver: Error: unrecognized data format '%s'.  The allowed values\n"
	"  are raw, xdr, and ascii.\n", optarg );
				exit(1);
			}
			break;
		case 'B':
			poll_all = TRUE;
			break;
		case 'c':
			enable_callbacks = TRUE;
			break;
		case 'C':
			strlcpy( mx_connection_acl_filename,
					optarg, MXU_FILENAME_LENGTH );
			break;
                case 'd':
                        debug_level = atoi( optarg );
                        break;
		case 'D':
			start_debugger = TRUE;
			break;
		case 'e':
			redirect_stderr = TRUE;
			destination_unbuffered = FALSE;
			strlcpy( mx_stderr_destination_filename,
					optarg, MXU_FILENAME_LENGTH );
			break;
		case 'E':
			redirect_stderr = TRUE;
			destination_unbuffered = TRUE;
			strlcpy( mx_stderr_destination_filename,
					optarg, MXU_FILENAME_LENGTH );
			break;
                case 'f':
                        strlcpy( mx_database_filename,
					optarg, MXU_FILENAME_LENGTH );
                        break;
		case 'J':
			just_in_time_debugging = TRUE;
                        break;
		case 'k':
			enable_callbacks = FALSE;
			break;
		case 'l':
			install_syslog_handler = TRUE;
			syslog_options = 0;

			syslog_number = atoi( optarg );
			break;
		case 'L':
			install_syslog_handler = TRUE;
			syslog_options = MXF_SYSLOG_USE_STDERR;

			syslog_number = atoi( optarg );
			break;
		case 'm':
			monitor_resources = TRUE;
			resource_monitor_interval = atof( optarg );
			break;
		case 'M':
			master_timer_period = atof( optarg );
			break;
		case 'n':
			delay_microseconds = atoi( optarg);
			break;
                case 'p':
                        server_port = atoi( optarg );
                        break;
		case 'P':
			default_display_precision = atoi( optarg );
			break;
		case 'r':
			enable_remote_breakpoint = TRUE;
			break;
		case 's':
			display_stack_traceback = TRUE;
			break;
		case 'S':
			display_stack_traceback = FALSE;
			break;
		case 't':
			init_hw_flags |= MXF_INITHW_TRACE_OPENS;
			break;
		case 'u':
#if HAVE_UNIX_DOMAIN_SOCKETS
			strlcpy( server_pathname,
					optarg, MXU_FILENAME_LENGTH );
#else
			fprintf( stderr,
"Error: Cannot use option '-u' since Unix domain sockets are not supported\n"
"       on this system.\n" );
			exit(1);
#endif
			break;
		case 'v':
			vc_poll_callback_interval = atof( optarg );
			break;
		case 'w':
			wait_for_debugger = TRUE;
			break;
		case 'W':
			wait_at_exit = TRUE;
			break;
		case 'x':
			putenv("MX_DEBUGGER=xterm -e gdb -tui -p %lu");
			break;
		case 'Y':
			/* Directly set the value of MXDIR. */

			mx_setenv( "MXDIR", optarg );
			break;
		case 'Z':
			bypass_signal_handlers = TRUE;
			break;
                case '?':
                        error_flag = TRUE;
                        break;
                }

                if ( error_flag == TRUE ) {
                        fprintf( stderr,
"Usage: mxserver [-d debug_level] [-f mx_database_file] [-l log_number]\n"
"  [-L log_number ] [-p server_port] [-P display_precision] \n"
"  [-C connection_acl_filename]\n" );
                        exit(1);
                }
        }

	if ( start_debugger ) {
		mx_prepare_for_debugging( NULL, just_in_time_debugging );

		mx_start_debugger( NULL );
	} else
	if ( just_in_time_debugging ) {
		mx_prepare_for_debugging( NULL, just_in_time_debugging );
	}

	if ( wait_for_debugger ) {
		mx_wait_for_debugger();
	}

	if ( wait_at_exit ) {
		fprintf( stderr,
			"This program will wait at exit to be closed.\n" );
		fflush( stderr );

		atexit( wait_at_exit_fn );
	}

#endif /* HAVE_GETOPT */

	if ( redirect_stderr ) {
		new_stderr = freopen( mx_stderr_destination_filename,
						"a", stderr );

		if ( new_stderr == NULL ) {
			saved_errno = errno;

			fprintf( stdout,
"Error: cannot redirect mxserver's output to the file '%s'\n"
"Reason = '%s'\n", mx_stderr_destination_filename, strerror( saved_errno ) );
			exit(1);
		}

		if ( destination_unbuffered ) {
			setvbuf( new_stderr, (char *) NULL, _IONBF, 0 );
		}
	}

	if ( bypass_signal_handlers == FALSE ) {
		mxsrv_install_signal_and_exit_handlers(display_stack_traceback);
	}

	mx_set_user_interrupt_function( mxsrv_user_interrupt_function );

	mx_set_debug_level( debug_level );

	if ( install_syslog_handler ) {
		snprintf( ident_string, sizeof(ident_string),
				"mxserver@%d", server_port );

		mx_status = mx_install_syslog_handler( ident_string,
					syslog_number, syslog_options );

		if ( mx_status.code != MXE_SUCCESS )
			exit(1);
	}

	mx_info( "***** MX server %s started *****",
		mx_get_version_full_string() );

	for ( i = 0; i < mxsrv_num_event_handlers; i++ ) {
		if ( mxsrv_event_handler_list[i].type
				== MXF_SRV_MX_CLIENT_TYPE ) {
			break;   /* Exit the for() loop. */
		}
	}

	if ( i >= mxsrv_num_event_handlers ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX client event handler is not found "
			"in the mxsrv_event_handler_list array." );

		exit( MXE_CORRUPT_DATA_STRUCTURE );
	}

	mxsrv_tcp_server_socket_struct.type = MXF_SRV_TCP_SERVER_TYPE;

	mxsrv_tcp_server_socket_struct.client_event_handler
		= &mxsrv_event_handler_list[i];

	mxsrv_tcp_server_socket_struct.u.tcp.port = server_port;

#if HAVE_UNIX_DOMAIN_SOCKETS
	mxsrv_unix_server_socket_struct.type = MXF_SRV_UNIX_SERVER_TYPE;

	mxsrv_unix_server_socket_struct.client_event_handler
		= &mxsrv_event_handler_list[i];

	strlcpy( mxsrv_unix_server_socket_struct.u.unix_domain.pathname,
			server_pathname, MXU_FILENAME_LENGTH );
#endif

	/* Initialize the list of socket handlers. */

	max_sockets = mx_get_max_file_descriptors();

	if ( max_sockets <= 1 ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Not enough file descriptors for the MX server to run." );

		exit( MXE_WOULD_EXCEED_LIMIT );
	}

	handler_array_size = max_sockets;

	socket_handler_list.max_sockets = max_sockets;
	socket_handler_list.num_sockets_in_use = 0;
	socket_handler_list.handler_array_size = handler_array_size;

	socket_handler_list.array = (MX_SOCKET_HANDLER **)
		malloc( handler_array_size * sizeof(MX_SOCKET_HANDLER *) );

	if ( socket_handler_list.array == (MX_SOCKET_HANDLER **) NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
		"Couldn't allocate socket handler array for %d handlers.",
			handler_array_size );

		exit( MXE_OUT_OF_MEMORY );
	}

	for ( i = 0; i < handler_array_size; i++ ) {
		socket_handler_list.array[i] = NULL;
	}

	mxsrv_update_select_fds( &socket_handler_list );

	/* Initialize the MX device drivers. */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* If requested, turn on the MXFF_POLL bit for all MX fields. */

	if ( poll_all ) {
		mx_status = mxsrv_poll_all();

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );
	}

	/* Initialize the MX database. */

	mx_record_list = mx_initialize_record_list();

	if ( mx_record_list == (MX_RECORD *) NULL ) {
		(void) mx_error( MXE_FUNCTION_FAILED, fname,
			"Attempt to initialize the record list failed." );

		exit( MXE_FUNCTION_FAILED );
	}

	/* Mark this record list as belonging to a server. */

	list_head_struct = (MX_LIST_HEAD *)
				(mx_record_list->record_superclass_struct);

	if ( list_head_struct == (MX_LIST_HEAD *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LIST_HEAD pointer for list head record is NULL." );

		exit( MXE_CORRUPT_DATA_STRUCTURE );
	}

	list_head_struct->is_server = TRUE;

	strlcpy( list_head_struct->program_name,
			"mxserver", MXU_PROGRAM_NAME_LENGTH );

	/* Set the default floating point display precision. */

	list_head_struct->default_precision = default_display_precision;
 
	/* Set the time interval for value changed polls. */

	list_head_struct->poll_callback_interval = vc_poll_callback_interval;

	/* Save the 'enable_remote_breakpoint' flag in the list head. */

	if ( enable_remote_breakpoint ) {
		list_head_struct->remote_breakpoint_enabled = TRUE;
	} else {
		list_head_struct->remote_breakpoint_enabled = FALSE;
	}

	/* Set server_protocols_active to reflect the protocols that will
	 * be used.
	 */

	list_head_struct->server_protocols_active = MXPROT_MX;

	/* Set the default network data format. */

	list_head_struct->default_data_format = default_data_format;

	/* Use the application_ptr to point to the socket handler list. */

	list_head_struct->application_ptr = &socket_handler_list;

	/* Set the default network debugging flag. */

#if 0
	list_head_struct->network_debug_flags = network_debug_flags;
#else
	mx_multi_set_debug_flags( mx_record_list, network_debug_flags );
#endif

#if 0
	fprintf(stderr, "%s: list_head_struct->network_debug = %d\n",
		fname, list_head_struct->network_debug );
#endif

	/* Make sure that the socket infrastructure has been initialized.
	 * On Win32, it is necessary to do this before invoking the 
	 * mx_gethostname() function.
	 */

	mx_status = mx_socket_initialize();

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Print out the hostname, operating system, and process id
	 * for the server.
	 */

	mx_status = mx_gethostname( list_head_struct->hostname,
					MXU_HOSTNAME_LENGTH );

	if ( mx_status.code != MXE_SUCCESS ) {
		strlcpy( list_head_struct->hostname,
			"unknown hostname", MXU_HOSTNAME_LENGTH );
	}

	mx_status = mx_get_os_version_string( os_version_string,
					sizeof(os_version_string) );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	mx_info("Server '%s' (%s), process id %lu",
	    list_head_struct->hostname, os_version_string, mx_process_id() );

#if 0
	MX_DEBUG(-2,("%s: MX_WORDSIZE = %d, MX_PROGRAM_MODEL = %#x",
		fname, MX_WORDSIZE, MX_PROGRAM_MODEL));
#endif

	/* If requested, enable the callback support. */

	if ( enable_callbacks ) {
		MX_PIPE *callback_pipe;
		MX_INTERVAL_TIMER *master_timer;

		mx_info("Enabling callbacks.");

		mx_status = mx_pipe_open( &callback_pipe );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		list_head_struct->callback_pipe = callback_pipe;

		mx_status = mx_pipe_set_blocking_mode(
				list_head_struct->callback_pipe,
				MXF_PIPE_READ | MXF_PIPE_WRITE,
				FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		/* Create a master timer with a timer period
		 * of 100 milliseconds.
		 */

		mx_status = mx_virtual_timer_create_master( &master_timer,
							master_timer_period );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );

		list_head_struct->master_timer = master_timer;

		list_head_struct->callbacks_enabled = TRUE;
	}

	/* Read the database description file and add the records therein
	 * to the record list.
	 */

	mx_info("Loading database file '%s'.", mx_database_filename);

	mx_status = mx_read_database_file( mx_record_list,
					mx_database_filename, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Perform database initialization steps that cannot be done until
	 * all the records have been defined.
	 */

	mx_status = mx_finish_database_initialization( mx_record_list );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Initialize the MX log file if the log file control record
	 * "mx_log" has been defined.
	 */

	mx_status = mx_log_open( mx_record_list );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Initialize all the hardware described by the record list. */

	mx_status = mx_initialize_hardware( mx_record_list, init_hw_flags );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

#if 0
	/* List all of the records that have been defined. */

	{
		MX_RECORD *current_record;

		current_record = mx_record_list;

		do {
			MX_DEBUG(-2,(
	"%s: Record '%s' defined.  Superclass = %ld, class = %ld, type = %ld",
				fname,
				current_record->name,
				current_record->mx_superclass,
				current_record->mx_class,
				current_record->mx_type ));

			current_record = current_record->next_record;

		} while ( current_record != mx_record_list );
	}
#endif

	/* Initialize the record handle table. */

	handle_table_block_size = 100;

	handle_table_num_blocks = list_head_struct->num_records
					/ handle_table_block_size;

	if ( list_head_struct->num_records % handle_table_block_size != 0 ) {
		handle_table_num_blocks++;
	}

	mx_status = mx_create_handle_table( &handle_table,
						handle_table_block_size,
						handle_table_num_blocks );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	list_head_struct->handle_table = handle_table;

	/* Initialize the record processing functions for the database. */

	mx_status = mx_initialize_database_processing( mx_record_list );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Initialize the socket connection access control list. */

	if ( strlen( mx_connection_acl_filename ) > 0 ) {

		mx_status = mx_setup_connection_acl( mx_record_list,
						mx_connection_acl_filename );

		if ( mx_status.code != MXE_SUCCESS )
			exit( mx_status.code );
	}

	/* Initialize all the event handler interfaces.  As part of this
	 * process, the network sockets that the server listens to will
	 * be opened.
	 */

	for ( i = 0; i < mxsrv_num_event_handlers; i++ ) {
		init_fn = mxsrv_event_handler_list[i].init;

		if ( init_fn == NULL ) {
			(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"init function pointer for server type ? is NULL." );

			exit( MXE_CORRUPT_DATA_STRUCTURE );
		}

		mx_status = ( *init_fn ) ( mx_record_list,
					&socket_handler_list,
					&mxsrv_event_handler_list[i] );

		if ( ( mx_status.code != MXE_SUCCESS )
		  && ( mx_status.code != MXE_NOT_FOUND ) )
		{
			exit( mx_status.code );
		}
	}

	/* Update the select_readfds value in socket_handler_list. */

	mxsrv_update_select_fds( &socket_handler_list );

	/* We must have at least one socket in use to act as a server. */

	if ( socket_handler_list.num_sockets_in_use <= 0 ) {
		(void) mx_error( MXE_INITIALIZATION_ERROR, fname,
	"No server sockets were opened during event handler initialization.  "
	"We cannot act as a server if we do not have any sockets open." );
		exit(1);
	}

	if ( delay_microseconds > 0 ) {
		mx_warning( "The MX server has been manually throttled "
		"using the -n option to a minimum of %ld microseconds "
		"per operation.", delay_microseconds );
	}

	if ( monitor_resources ) {
		mxsrv_display_resource_usage( TRUE, resource_monitor_interval );
	}

	/************ Primary event loop *************/

	mx_info("mxserver: Ready to accept client connections.");

#if defined(DEBUG_DMALLOC)
	mainloop_mark = dmalloc_mark();
#endif

#if defined(DEBUG_MPATROL)
	heapdiffstart( mainloop_heapdiff, HD_UNFREED | HD_FULL );
#endif

	for (;;) {

		if ( monitor_resources ) {
			mxsrv_display_resource_usage( FALSE, -1.0 );
		}

#if 0
		/* If compiled in, this code acts as an intentional
		 * memory leak.  It is used for testing memory leak
		 * detection code.
		 */

		MX_DEBUG(-2,("%s: BEEP!", fname));

		(void) malloc( 21 );

		mx_sleep(1);
#endif

		/* If for some reason we need to slow down the event loop,
		 * specify a positive number for delay_microseconds via 
		 * the -n command line argument for the MX server.
		 * Under _most_ circumstances, you should not want to do
		 * this.  However, if for some reason you temporarily
		 * need to throttle the rate at which client requests
		 * are serviced, then this is a way to do it.
		 */

		if ( delay_microseconds > 0 ) {
			mx_usleep( (unsigned long) delay_microseconds );
		}

		/* Process incoming MX events. */

		mxsrv_process_sockets_with_select( mx_record_list,
						&socket_handler_list );

		/* Check for callbacks. */

		if ( list_head_struct->callback_pipe != NULL ) {
			size_t num_bytes_available;

			mx_status = mx_pipe_num_bytes_available(
						list_head_struct->callback_pipe,
						&num_bytes_available );

			if ( ( mx_status.code == MXE_SUCCESS )
			  && ( num_bytes_available > 0 ) )
			{
				mx_status = mx_process_callbacks(
					mx_record_list,
					list_head_struct->callback_pipe );
			}
		}
	}

#if ( defined(OS_HPUX) && !defined(__ia64) )
	return 0;
#endif
}

