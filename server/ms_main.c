/*
 * Name:    ms_main.c
 *
 * Purpose: main() routine for the MX server.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MS_MAIN_DEBUG_MEMORY_LEAK	FALSE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>

#include "mx_osdef.h"
#include "mx_util.h"
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
#include "mx_clock.h"
#include "mx_hrt.h"
#include "mx_log.h"
#include "mx_syslog.h"

#include "mx_process.h"
#include "ms_mxserver.h"
#include "ms_security.h"

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

static int
mxsrv_user_interrupt_function( void )
{
	/* The MX server never has direct user interrupts. */

	return MXF_USER_INT_NONE;
}

static void
mxsrv_exit_handler( void )
{
	mx_info( "*** MX server process exiting. ***" );
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

static void
mxsrv_print_timestamp( void )
{
	time_t time_struct;
	struct tm *current_time;
	char buffer[80];

	time( &time_struct );

	current_time = localtime( &time_struct );

	strftime( buffer, sizeof(buffer),
			"%b %d %H:%M:%S ", current_time );

	fputs( buffer, stderr );
}

static void
mxsrv_info_output_function( char *string )
{
	mxsrv_print_timestamp();
	fprintf( stderr, "%s\n", string );
}

static void
mxsrv_warning_output_function( char *string )
{
	mxsrv_print_timestamp();
	fprintf( stderr, "Warning: %s\n", string );
}

static void
mxsrv_error_output_function( char *string )
{
	mxsrv_print_timestamp();
	fprintf( stderr, "%s\n", string );
}

static void
mxsrv_debug_output_function( char *string )
{
	mxsrv_print_timestamp();
	fprintf( stderr, "%s\n", string );
}

static void
mxsrv_setup_output_functions( void )
{
	mx_set_info_output_function( mxsrv_info_output_function );
	mx_set_warning_output_function( mxsrv_warning_output_function );
	mx_set_error_output_function( mxsrv_error_output_function );
	mx_set_debug_output_function( mxsrv_debug_output_function );
}

/*------------------------------------------------------------------*/

#if MS_MAIN_DEBUG_MEMORY_LEAK

static void
mxsrv_display_meminfo( int initialize_flag )
{
	static MX_CLOCK_TICK next_event_tick, event_interval_in_ticks;

	MX_CLOCK_TICK current_tick;
	MX_SYSTEM_MEMINFO system_meminfo;
	MX_PROCESS_MEMINFO process_meminfo;
	mx_status_type mx_status;

	if ( initialize_flag ) {
		event_interval_in_ticks =
			mx_convert_seconds_to_clock_ticks( 1.0 );

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

#endif

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

	MX_RECORD *mx_record_list, *current_record, *next_record;
	MX_LIST_HEAD *list_head_struct;
	MX_CLOCK_TICK current_clock_tick;
	MX_HANDLE_TABLE *handle_table;
	unsigned long handle_table_block_size, handle_table_num_blocks;
	char mx_database_filename[MXU_FILENAME_LENGTH+1];
	char mx_connection_acl_filename[MXU_FILENAME_LENGTH+1];
	char mx_stderr_destination_filename[MXU_FILENAME_LENGTH+1];
	char server_pathname[MXU_FILENAME_LENGTH+1];
	char server_hostname[MXU_HOSTNAME_LENGTH+1];
	char os_version_string[40];
	char ident_string[80];
	int i, debug_level, start_debugger, saved_errno;
	int num_fds, max_fd;
	int server_port, default_display_precision, init_hw_flags;
	int install_syslog_handler, syslog_number, syslog_options;
	int display_stack_traceback, redirect_stderr, destination_unbuffered;
	int bypass_signal_handlers;
	long delay_microseconds;
	unsigned long default_data_format;
	FILE *new_stderr;
	MX_SOCKET *current_socket;

	MX_EVENT_HANDLER *event_handler;
	int max_sockets, handler_array_size;
	MX_SOCKET_HANDLER_LIST socket_handler_list;
	fd_set readfds;
	struct timeval timeout;

	mx_status_type ( *init_fn ) (MX_RECORD *,
					MX_SOCKET_HANDLER_LIST *,
					MX_EVENT_HANDLER * );
	mx_status_type ( *process_event_fn ) ( MX_RECORD *,
					MX_SOCKET_HANDLER *,
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

	strcpy( mx_database_filename, "mxserver.dat" );
	strcpy( mx_connection_acl_filename, "mxserver.acl" );

	debug_level = 0;

	start_debugger = FALSE;

	default_display_precision = 8;

	server_port = -1;

	strcpy( server_pathname, "" );

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

#if HAVE_GETOPT
        /* Process command line arguments, if any. */

        error_flag = FALSE;

        while ((c = getopt(argc, argv, "b:C:d:De:E:f:l:L:n:p:P:sStu:Z")) != -1)
	{
                switch (c) {
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
		case 'n':
			delay_microseconds = atoi( optarg);
			break;
                case 'p':
                        server_port = atoi( optarg );
                        break;
		case 'P':
			default_display_precision = atoi( optarg );
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
		mx_start_debugger( NULL );
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
		sprintf( ident_string, "mxserver@%d", server_port );

		mx_status = mx_install_syslog_handler( ident_string,
					syslog_number, syslog_options );

		if ( mx_status.code != MXE_SUCCESS )
			exit(1);
	}

	mx_info( "***** MX server %s started *****", mx_get_version_string() );

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

	/* Initialize the MX device drivers. */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

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

	/* Set server_protocols_active to reflect the protocols that will
	 * be used.
	 */

	list_head_struct->server_protocols_active = MXPROT_MX;

	/* Set the default network data format. */

	list_head_struct->default_data_format = default_data_format;

	/* Use the application_ptr to point to the socket handler list. */

	list_head_struct->application_ptr = &socket_handler_list;

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

	mx_status = mx_gethostname(server_hostname, sizeof(server_hostname)-1);

	if ( mx_status.code != MXE_SUCCESS ) {
		strlcpy( server_hostname,
			"unknown hostname", MXU_HOSTNAME_LENGTH );
	}

	mx_status = mx_get_os_version_string( os_version_string,
					sizeof(os_version_string) );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	mx_info("Server '%s' (%s), process id %lu",
		server_hostname, os_version_string, mx_process_id() );

#if 1
	MX_DEBUG(-2,("%s: MX_WORDSIZE = %d, MX_PROGRAM_MODEL = %#x",
		fname, MX_WORDSIZE, MX_PROGRAM_MODEL));
#endif

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

		mx_status = mxsrv_setup_connection_acl( mx_record_list,
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

#if MS_MAIN_DEBUG_MEMORY_LEAK
	mxsrv_display_meminfo( TRUE );
#endif

	/************ Primary event loop *************/

	mx_info("mxserver: Ready to accept client connections.");

	for (;;) {

#if MS_MAIN_DEBUG_MEMORY_LEAK
		mxsrv_display_meminfo( FALSE );
#endif

#if 0
		/* If compiled in, this code acts as an intentional
		 * memory leak.  It is used for testing memory leak
		 * detection code.
		 */

		MX_DEBUG(-2,("%s: BEEP!", fname));

		(void) malloc( 21 );

		mx_sleep(1);
#endif

#if 0
		/* Some test code for memory leak checking. */

		while(1) {
			mxsrv_display_meminfo( FALSE );
#if defined(OS_WIN32)
			Sleep(1000);
#else
			mx_sleep(1);
#endif
		}

		/* End of memory leak checking test code. */
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
			mx_udelay( (unsigned long) delay_microseconds );
		}

#if 0
		MX_DEBUG(-2,("%s: Top of event loop.", fname));
#endif

		/* Initialize the arguments to select(). */

		FD_ZERO(&readfds);

		max_fd = -1;

		for ( i = 0; i < handler_array_size; i++ ) {
			if ( socket_handler_list.array[i] != NULL ) {
				current_socket
			    = socket_handler_list.array[i]->synchronous_socket;

				if ( current_socket->socket_fd < 0 ) {
					MX_DEBUG(0,
("main #1: socket_handler_list.array[%d] = %p, current_socket fd = %d",
		i, socket_handler_list.array[i], current_socket->socket_fd));
				}

				FD_SET( current_socket->socket_fd, &readfds );

				if ( current_socket->socket_fd > max_fd ) {
					max_fd = current_socket->socket_fd;
				}
			}
		}

#ifdef OS_WIN32
		max_fd = -1;
#else
		max_fd++;
#endif

		timeout.tv_sec = 0;
		timeout.tv_usec = 1;     /* Wait 1 microsecond. */
		
		/* Use select() to look for events. */

/*		MX_MEMORY_DISPLAY_COUNTER( allocated_bytes, "MARKER #2" ); */

		num_fds = select( max_fd, &readfds, NULL, NULL, &timeout );

		saved_errno = errno;

		if ( num_fds < 0 ) {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Error in select() while waiting for events.  "
			"Errno = %d.  Error string = '%s'.",
			saved_errno, strerror( saved_errno ) );

		} else if ( num_fds == 0 ) {

			/* Didn't get any events, so do nothing here. */

		} else {
			MX_DEBUG( 2,("%s: select() returned.  num_fds = %d",
					fname, num_fds));

			/* Figure out which sockets had events and
			 * then process the events.
			 */

			for ( i = 0; i < handler_array_size; i++ ) {
			    if ( socket_handler_list.array[i] != NULL ) {

				current_socket
			    = socket_handler_list.array[i]->synchronous_socket;

				if ( current_socket->socket_fd < 0 ) {
					MX_DEBUG(0,
("main #2: socket_handler_list.array[%d] = %p, current_socket fd = %d",
		i, socket_handler_list.array[i], current_socket->socket_fd));
				}

				if ( FD_ISSET(current_socket->socket_fd,
							&readfds) )
				{

					event_handler =
				socket_handler_list.array[i]->event_handler;

					if ( event_handler == NULL ) {
						(void) mx_error(
						MXE_NETWORK_IO_ERROR, fname,
		"Event handler pointer for socket handler %d is NULL.", i);

						continue;
					}

					process_event_fn
					    = event_handler->process_event;

					if ( process_event_fn == NULL ) {
						(void) mx_error(
						MXE_NETWORK_IO_ERROR, fname,
	"process_event function pointer for socket handler %d is NULL.", i);

						continue;
					}

					(void) ( *process_event_fn )
						( mx_record_list,
						  socket_handler_list.array[i],
						  &socket_handler_list,
						  event_handler );
				}
			    }
			}
		}

		/* Are there any queued events that are ready to process? */

		current_clock_tick = mx_current_clock_tick();

		current_record = mx_record_list;

		do {
			if ( current_record->event_queue != NULL ) {

				mx_status = mx_process_queued_event(
						current_record,
						current_clock_tick );
			}
			next_record = current_record->next_record;

			if ( next_record == (MX_RECORD *) NULL ) {
				(void) mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The next record pointer for record '%s' was found to be NULL "
	"while checking for queued events.  This implies massive memory "
	"corruption in the MX server, so the MX server will abort "
	"with a core dump now.",
			current_record->name );

				mx_force_core_dump();

				/* mx_force_core_dump() does not return. */
			}

			current_record = next_record;

		} while ( current_record != mx_record_list );
	}

#if defined(OS_HPUX)
	return 0;
#endif
}

