/*
 * Name:    ms_autosave.c
 *
 * Purpose: main() routine for the MX automatic save/restore and
 *          record polling utility.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2007, 2009-2012, 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_time.h"
#include "mx_unistd.h"
#include "mx_version.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_variable.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_syslog.h"

#include "ms_autosave.h"

int msauto_quiet_exit;

#if ! defined( OS_WIN32 )

static void
msauto_exit_handler( void )
{
	if ( msauto_quiet_exit == FALSE ) {
		mx_info( "*** MX autosave process exiting. ***" );
	}
}

static void
msauto_sigterm_handler( int signal_number )
{
	mx_info( "Received a request to shutdown via a SIGTERM signal." );

	exit(0);
}

static void
msauto_install_signal_and_exit_handlers( void )
{
	atexit( msauto_exit_handler );

	signal( SIGTERM, msauto_sigterm_handler );

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

#endif  /* ! OS_WIN32 */

/*------------------------------------------------------------------*/

static void
msauto_print_timestamp( void )
{
	time_t time_struct;
	struct tm current_time;
	char buffer[80];

	time( &time_struct );

	(void) localtime_r( &time_struct, &current_time );

	strftime( buffer, sizeof(buffer),
			"%b %d %H:%M:%S ", &current_time );

	fputs( buffer, stderr );
}

static void
msauto_info_output_function( char *string )
{
	msauto_print_timestamp();
	fprintf( stderr, "%s\n", string );
}

static void
msauto_warning_output_function( char *string )
{
	msauto_print_timestamp();
	fprintf( stderr, "Warning: %s\n", string );
}

static void
msauto_error_output_function( char *string )
{
	msauto_print_timestamp();
	fprintf( stderr, "%s\n", string );
}

static void
msauto_debug_output_function( char *string )
{
	msauto_print_timestamp();
	fprintf( stderr, "%s\n", string );
}

static void
msauto_setup_output_functions( void )
{
	mx_set_info_output_function( msauto_info_output_function );
	mx_set_warning_output_function( msauto_warning_output_function );
	mx_set_error_output_function( msauto_error_output_function );
	mx_set_debug_output_function( msauto_debug_output_function );
}

/*------------------------------------------------------------------*/

static void
wait_at_exit_fn( void )
{
	fprintf( stderr, "Press any key to exit...\n" );
	mx_getch();
}

/*------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
	static const char fname[] = "main()";

	MX_AUTOSAVE_LIST autosave_list;

	MX_RECORD *record_list;
	MX_LIST_HEAD *list_head;

	MX_CLOCK_TICK event_interval;
	MX_CLOCK_TICK next_event_time;
	MX_CLOCK_TICK current_time;

	double update_interval_in_seconds = 30.0;

	FILE *autosave_list_file;
	char autosave_list_filename[MXU_FILENAME_LENGTH+1];
	char autosave1_filename[MXU_FILENAME_LENGTH+1];
	char autosave2_filename[MXU_FILENAME_LENGTH+1];
	char *autosave_filename;
	char hostname[80];
	char ident_string[100];
	int i, debug_level, num_non_option_arguments;
	int syslog_number, syslog_options;
	int default_display_precision;
	mx_bool_type start_debugger, install_syslog_handler;
	mx_bool_type wait_for_debugger, just_in_time_debugging;
	mx_bool_type wait_at_exit;
	mx_bool_type no_restore, restore_only, save_only;
	unsigned long network_debug_flags;
	mx_status_type mx_status;

	static char usage[] =
"*          Usage: mxautosave [options] autosave_list_filename autosave_fn1 [ autosave_fn2 ]\n"
"\n"
"The available options are:\n"
"       -a                   (enable network debugging summary)\n"
"       -A                   (enable verbose network debugging)\n"
"       -d debug_level\n"
"       -l log_number        (log to syslog)\n"
"       -L log_number        (log to syslog and stderr)\n"
"       -P display_precision\n"
"       -R                   (never restore parameters)\n"
"       -r                   (restore a set of parameters)\n"
"       -s                   (save a set of parameters)\n"
"       -u update_interval_in_seconds\n"
"\n";

#if HAVE_GETOPT
	int c, error_flag;
#endif

	mx_status = mx_initialize_runtime();

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( stderr,
		"%s: Unable to initialize the MX runtime environment.\n",
			argv[0] );

		exit(1);
	}

	msauto_quiet_exit = FALSE;

#if ! defined( OS_WIN32 )
	msauto_install_signal_and_exit_handlers();
#endif
	network_debug_flags = 0;

	start_debugger = FALSE;
	wait_for_debugger = FALSE;
	wait_at_exit = FALSE;
	just_in_time_debugging = FALSE;

	debug_level = 0;

	default_display_precision = 8;

	install_syslog_handler = FALSE;

	syslog_number = -1;
	syslog_options = 0;

	no_restore = FALSE;
	restore_only = FALSE;
	save_only = FALSE;

#if HAVE_GETOPT
	/* Process command line arguments via getopt, if any. */

	error_flag = FALSE;

	while ((c = getopt(argc, argv, "aAd:DJl:L:P:Rrsu:x")) != -1 ) {
		switch(c) {
		case 'a':
			network_debug_flags |= MXF_NETDBG_SUMMARY;
			break;
		case 'A':
			network_debug_flags |= MXF_NETDBG_VERBOSE;
			break;
		case 'd':
			debug_level = atoi( optarg );
			break;
		case 'D':
			start_debugger = TRUE;
			break;
		case 'J':
			just_in_time_debugging = TRUE;
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
		case 'P':
			default_display_precision = atoi( optarg );
			break;
		case 'R':
			no_restore = TRUE;
			break;
		case 'r':
			restore_only = TRUE;
			break;
		case 's':
			save_only = TRUE;
			break;
		case 'u':
			update_interval_in_seconds = atof( optarg );
			break;
		case 'x':
			putenv("MX_DEBUGGER=xterm -e gdb -tui -p %lu");
			break;
		case 'w':
			wait_for_debugger = TRUE;
			break;
		case 'W':
			wait_at_exit = TRUE;
			break;
		case 'Y':
			/* Directly set the value of MXDIR. */

			mx_setenv( "MXDIR", optarg );
			break;
		case '?':
			error_flag = TRUE;
			break;
		}

		if ( error_flag == TRUE ) {
			fputs( usage, stderr );
			msauto_quiet_exit = TRUE;
			exit(1);
		}
	}

	i = optind;

	num_non_option_arguments = argc - optind;

#else  /* HAVE_GETOPT */

	i = 1;

	num_non_option_arguments = argc - 1;

#endif /* HAVE_GETOPT */

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

	if ( save_only && restore_only ) {
		fprintf(stderr,
			"Error: You must not specify both the save '-s'\n"
			"and restore '-r' command switches.\n" );
		fputs( usage, stderr );
		msauto_quiet_exit = TRUE;
		exit(1);
	}

	if ( restore_only && no_restore ) {
		fprintf(stderr,
			"Error: You must not specify both the restore '-r'\n"
			"and no restore '-R' command switches.\n" );
		fputs( usage, stderr );
		msauto_quiet_exit = TRUE;
		exit(1);
	}

	if ( save_only || restore_only ) {
		if ( num_non_option_arguments != 2 ) {
			fprintf(stderr,
				"Error: incorrect number of arguments.\n\n");
			fputs( usage, stderr );
			msauto_quiet_exit = TRUE;
			exit(1);
		}
	} else {
		if ( num_non_option_arguments != 3 ) {
			fprintf(stderr,
				"Error: incorrect number of arguments.\n\n");
			fputs( usage, stderr );
			msauto_quiet_exit = TRUE;
			exit(1);
		}
	}

	if ( (save_only == FALSE) && (restore_only == FALSE) ) {
		msauto_setup_output_functions();
	}

	/* Copy the other command line arguments. */

	strlcpy( autosave_list_filename, argv[i], sizeof(autosave_list_filename) );

	strlcpy( autosave1_filename, argv[i+1], sizeof(autosave1_filename) );

	if ( save_only || restore_only ) {
		/* Set the second filename to an empty string. */

		strlcpy( autosave2_filename, "", sizeof(autosave2_filename) );
	} else {
		strlcpy( autosave2_filename, argv[i+2],
					sizeof(autosave2_filename) );
	}

	mx_set_debug_level( debug_level );

	if ( install_syslog_handler ) {
		mx_gethostname( hostname, sizeof(hostname) - 1 );

		snprintf( ident_string, sizeof(ident_string),
			"mxautosave@%s", hostname );

		mx_status = mx_install_syslog_handler( ident_string,
					syslog_number, syslog_options );

		if ( mx_status.code != MXE_SUCCESS )
			exit(1);
	}

	current_time = mx_current_clock_tick();

	event_interval = mx_convert_seconds_to_clock_ticks(
					update_interval_in_seconds );

	if ( (save_only == FALSE) && (restore_only == FALSE) ) {
		mx_info( "*** MX autosave utility %s started ***",
				mx_get_version_full_string() );

		mx_info(
		"%g clock ticks per second, current time = (%lu,%lu)",
			mx_clock_ticks_per_second(),
			current_time.high_order,
			current_time.low_order);

		mx_info("Event interval:  %g seconds, (%lu,%lu) clock ticks.",
			update_interval_in_seconds,
			event_interval.high_order,
			event_interval.low_order);
	}

	next_event_time = current_time;

	/* Does the file containing the list of record fields exist and
	 * is it readable?
	 */

	autosave_list_file = fopen( autosave_list_filename, "r" );

	if ( autosave_list_file == NULL ) {
		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Could not open the autosave list file '%s' for reading.",
			autosave_list_filename );

		exit( MXE_FILE_IO_ERROR );
	}

	/* Create an empty MX database. */

	mx_status = msauto_create_empty_mx_database( &record_list,
						default_display_precision );

	if ( mx_status.code != MXE_SUCCESS )
		exit( (int) mx_status.code );

	/* Set the 'network_debug' flag appropriately. */

	list_head = mx_get_record_list_head_struct( record_list );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the current database is NULL." );

		exit( MXE_CORRUPT_DATA_STRUCTURE );
	}

	list_head->network_debug_flags = network_debug_flags;

	/* Construct the autosave list data structure. */

	mx_status = msauto_construct_autosave_list( &autosave_list,
				autosave_list_file, autosave_list_filename,
				record_list );

	if ( mx_status.code != MXE_SUCCESS )
		exit( (int) mx_status.code );

	fclose( autosave_list_file );

#if 0
	/* Print out the list of records in the generated record list. */

	{
		MX_RECORD *current_record, *list_head_record;

		MX_DEBUG(-2,("%s: List of local database records:", fname));

		list_head_record = record_list->list_head;

		current_record = list_head_record->next_record;

		while ( current_record != list_head_record ) {
			MX_DEBUG(-2,("    %s", current_record->name));

			current_record = current_record->next_record;
		}
	}
#endif

#if 0
	{
		/* Print out the contents of each autosave list. */

		MX_AUTOSAVE_LIST_ENTRY *autosave_list_entry;
		MX_AUTOSAVE_LIST_ENTRY *autosave_list_entry_array;

		MX_DEBUG(-2,("*** Autosave list: (%ld entries)",
				autosave_list.num_entries ));

		autosave_list_entry_array = autosave_list.entry_array;

		for ( i = 0; i < autosave_list.num_entries; i++ ) {
			autosave_list_entry = &autosave_list_entry_array[i];

			MX_DEBUG(-2,("        %s",
			  autosave_list_entry->read_record->name));
		}
	}
#endif

	/* Check to see if the autosave list is empty. */

	if ( autosave_list.num_entries == 0 ) {
		mx_warning( "The MX autosave list is empty." );

		mx_info("mxautosave has nothing to do, so it will exit now.");

		exit(0);
	}

	/* Restore the values in the most current autosave file to
	 * the MX database.
	 */

	if ( ( save_only || no_restore ) == FALSE ) {

		mx_status = msauto_restore_fields_from_autosave_files(
			autosave1_filename, autosave2_filename,
			&autosave_list );

		if ( mx_status.code != MXE_SUCCESS )
			exit( (int) mx_status.code );
	}

	/* If we are _only_ restoring, then we are done. */

	if ( restore_only ) {
		msauto_quiet_exit = TRUE;
		exit(0);
	}

	/* If we are only saving, then fetch the current values and then
	 * save them.
	 */

	if ( save_only ) {
		mx_status = msauto_poll_autosave_list( &autosave_list );

		if ( mx_status.code != MXE_SUCCESS )
			exit( (int) mx_status.code );

		mx_status = msauto_save_fields_to_autosave_file(
					autosave1_filename, &autosave_list );

		if ( mx_status.code != MXE_SUCCESS )
			exit( (int) mx_status.code );

		msauto_quiet_exit = TRUE;
		exit(0);
	}

	/*********************** Loop forever *************************/

	autosave_filename = autosave1_filename;

	mx_info("%s: Starting update loop.", fname);

	for(;;) {
		current_time = mx_current_clock_tick();

		/* Check to see if it is time to rewrite an autosave file. */

		if ( mx_compare_clock_ticks( current_time,
					next_event_time ) > 0 ) {

			MX_DEBUG( 1,
	("***** Time to rewrite autosave file.  current_time = (%lu,%lu)",
				current_time.high_order,
				current_time.low_order));

			mx_status = msauto_save_fields_to_autosave_file(
					autosave_filename, &autosave_list );

			if ( mx_status.code == MXE_NETWORK_CONNECTION_LOST )
				exit( (int) mx_status.code );

			if ( autosave_filename == autosave1_filename ) {
				autosave_filename = autosave2_filename;
			} else {
				autosave_filename = autosave1_filename;
			}

			next_event_time = mx_add_clock_ticks(
						next_event_time,
						event_interval );
		}

		mx_msleep(10);

		/* Check to see if the next event time for the given
		 * polling interval has arrived.
		 */

		if ( mx_compare_clock_ticks( current_time,
					next_event_time ) > 0 ) {

			MX_DEBUG( 1,
			("===== Time to update.  current_time = (%lu,%lu)",
				current_time.high_order,
				current_time.low_order));

			mx_status = msauto_poll_autosave_list( &autosave_list );

			if ( mx_status.code == MXE_NETWORK_CONNECTION_LOST )
				exit( (int) mx_status.code );

			next_event_time = mx_add_clock_ticks(
						next_event_time,
						event_interval );
		}
	}

#if ( defined(OS_HPUX) && !defined(__ia64) )
	return 0;
#endif
}

mx_status_type
msauto_create_empty_mx_database( MX_RECORD **record_list,
				int default_display_precision )
{
	static const char fname[] = "msauto_create_empty_mx_database()";

	MX_LIST_HEAD *list_head_struct;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* Initialize MX device drivers. */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set up the record list. */

	*record_list = mx_initialize_record_list();

	if ( (*record_list) == NULL ) {
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

	/* Set the MX program name. */

	strlcpy( list_head_struct->program_name, "mxautosave",
				MXU_PROGRAM_NAME_LENGTH );

	/* Mark the record list as ready to be used. */

	list_head_struct->list_is_active = TRUE;
	list_head_struct->fixup_records_in_use = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
msauto_add_mx_variable_to_database( MX_RECORD *record_list,
				char *network_field_id,
				long record_number,
				MX_RECORD **created_record,
				char *record_name,
				size_t max_record_name_length,
				char *field_name,
				size_t max_field_name_length )
{
	static const char fname[] = "msauto_add_mx_variable_to_database()";

	MX_RECORD *server_record;
	long i, num_elements;
	long datatype, num_dimensions;
	long dimension_array[MXU_FIELD_MAX_DIMENSIONS];
	char server_name[MXU_HOSTNAME_LENGTH+1];
	char server_arguments[MXU_SERVER_ARGUMENTS_LENGTH+1];
	char record_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char *ptr;
	size_t string_length;
	char description[MXU_RECORD_DESCRIPTION_LENGTH+1];
	mx_status_type mx_status;

	MX_DEBUG( 2,("=== %s invoked for network_field_id = '%s' ===",
		fname, network_field_id));

	/* Break up the network_field_id string passed to us
	 * into its component parts.
	 */

	mx_status = mx_parse_network_field_id( network_field_id,
			server_name, sizeof(server_name) - 1,
			server_arguments, sizeof(server_arguments) - 1,
			record_name, max_record_name_length,
			field_name, max_field_name_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: server_name = '%s', server_arguments = '%s'",
			fname, server_name, server_arguments));
	MX_DEBUG( 2,("%s: record_name = '%s', field_name = '%s'",
			fname, record_name, field_name));

	strlcpy( record_field_name, record_name, sizeof(record_field_name) );

	strlcat( record_field_name, ".", sizeof(record_field_name) );

	strlcat( record_field_name, field_name, sizeof(record_field_name) );

	MX_DEBUG( 2,("%s: record_field_name = '%s'", fname, record_field_name));

	/* If the returned server_name or server_arguments strings are
	 * of zero length, set them to default values.
	 */

	if ( strlen( server_name ) == 0 ) {
		strlcpy( server_name, "localhost", sizeof(server_name) );
	}
	if ( strlen( server_arguments ) == 0 ) {
		strlcpy( server_arguments, "9727", sizeof(server_arguments) );
	}

	/* Find the requested server record in the database, or
	 * create a new one.  Wait up to a maximum of 5 seconds
	 * to connect to this MX server.
	 */

	mx_status = mx_get_mx_server_record( record_list,
			server_name, server_arguments,
			&server_record, 5.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Ask the server for information about this record field. */

	MX_DEBUG( 2,("%s: record_field_name = '%s'",
			fname, record_field_name));

	mx_status = mx_get_field_type( server_record,
			record_field_name,
			MXU_FIELD_MAX_DIMENSIONS,
			&datatype,
			&num_dimensions,
			dimension_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Variable records cannot use num_dimensions = 0. */

	if ( num_dimensions == 0 ) {
		num_dimensions = 1;
		dimension_array[0] = 1;
	}

	MX_DEBUG( 2,
	("datatype = %ld, num_dimensions = %ld, dimension_array[0] = %ld",
			datatype, num_dimensions, dimension_array[0] ));

	/* Create an MX database record of class 'net_variable' */

	snprintf( description, sizeof(description),
		"record%ld variable net_variable ", record_number );

	switch( datatype ) {
	case MXFT_STRING:
		strlcat( description, "net_string ", sizeof(description) );
		break;
	case MXFT_BOOL:
		strlcat( description, "net_bool ", sizeof(description) );
		break;
	case MXFT_CHAR:
		strlcat( description, "net_char ", sizeof(description) );
		break;
	case MXFT_UCHAR:
		strlcat( description, "net_uchar ", sizeof(description) );
		break;
	case MXFT_SHORT:
		strlcat( description, "net_short ", sizeof(description) );
		break;
	case MXFT_USHORT:
		strlcat( description, "net_ushort ", sizeof(description) );
		break;
	case MXFT_LONG:
		strlcat( description, "net_long ", sizeof(description) );
		break;
	case MXFT_ULONG:
	case MXFT_HEX:
		strlcat( description, "net_ulong ", sizeof(description) );
		break;
	case MXFT_INT64:
		strlcat( description, "net_int64 ", sizeof(description) );
		break;
	case MXFT_UINT64:
		strlcat( description, "net_uint64 ", sizeof(description) );
		break;
	case MXFT_FLOAT:
		strlcat( description, "net_float ", sizeof(description) );
		break;
	case MXFT_DOUBLE:
		strlcat( description, "net_double ", sizeof(description) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Remote record field '%s.%s' is of type %ld "
			"which is not supported by this program.",
			record_name, field_name, datatype );
	}

	string_length = strlen(description);

	ptr = description + string_length;

	snprintf( ptr, sizeof(description) - string_length,
			"\"\" \"\" %s %s.%s %ld %ld ",
			server_record->name, record_name, field_name,
			num_dimensions, dimension_array[0] );

	if ( datatype == MXFT_STRING ) {
		num_elements = 1;
	} else {
		num_elements = dimension_array[0];
	}

	for ( i = 1; i < num_dimensions; i++ ) {
		string_length = strlen(description);

		ptr = description + string_length;

		snprintf( ptr, sizeof(description) - string_length,
			"%ld ", dimension_array[i] );

		num_elements *= dimension_array[i];
	}
	switch( datatype ) {
	case MXFT_STRING:
		for ( i = 0; i < num_elements; i++ ) {
			strlcat( description, "\"\" ", sizeof(description) );
		}
		break;
	default:
		for ( i = 0; i < num_elements; i++ ) {
			strlcat( description, "0 ", sizeof(description) );
		}
		break;
	}

	/* Now that we have a record description for the variable,
	 * add it to the MX database.
	 */

	MX_DEBUG( 2,("%s: description = '%s'", fname, description));

	mx_status = mx_create_record_from_description( server_record->list_head,
					description, created_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_finish_record_initialization( *created_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_open_hardware( *created_record );

	return mx_status;
}

#if ( HAVE_EPICS == 0 )

static mx_status_type
msauto_add_epics_variable_to_database( MX_RECORD *record_list,
				char *epics_pv_name,
				long record_number,
				MX_RECORD **created_record,
				char *record_name,
				size_t max_record_name_length,
				char *field_name,
				size_t max_field_name_length )
{
	static const char fname[] = "msauto_add_epics_variable_to_database()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Support for EPICS PVs is not compiled into this copy of mxautosave." );
}

#else /* HAVE_EPICS */

#include "mx_epics.h"

static mx_status_type
msauto_add_epics_variable_to_database( MX_RECORD *record_list,
				char *epics_pv_name,
				long record_number,
				MX_RECORD **created_record,
				char *record_name,
				size_t max_record_name_length,
				char *field_name,
				size_t max_field_name_length )
{
	static const char fname[] = "msauto_add_epics_variable_to_database()";

	long i, length, string_length;
	long epics_datatype, epics_array_length;
	char record_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char *ptr, *r_ptr;
	char description[MXU_RECORD_DESCRIPTION_LENGTH+1];
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for epics_pv_name = '%s'",
		fname, epics_pv_name));

	/* Convert the EPICS PV name to an MX variable record field name.
	 *
	 * We do this by changing the last period in the PV name to an 
	 * underscore and then appending .value to the end.
	 */

	r_ptr = NULL;

	ptr = strrchr( epics_pv_name, '.' );

	if ( ptr == NULL ) {
		snprintf( record_name, max_record_name_length,
			"%s_VAL", epics_pv_name );
	} else {
		length = ptr - epics_pv_name;

		MX_DEBUG( 2,("%s: length = %ld", fname, length));

		memcpy( record_name, epics_pv_name, length );

		r_ptr = record_name + length;

		ptr++;

		snprintf( r_ptr, max_record_name_length - length, "_%s", ptr );
	}

	if ( max_record_name_length >= MXU_RECORD_NAME_LENGTH ) {
		record_name[MXU_RECORD_NAME_LENGTH - 1] = '\0';
	}

	strlcpy( field_name, "value", max_field_name_length );

	snprintf( record_field_name, sizeof(record_field_name),
		"%s.%s", record_name, field_name );

	MX_DEBUG( 2,("%s: record_field_name = '%s'", fname, record_field_name));

	/* Ask the server for information about this record field. */

	mx_status = mx_epics_get_pv_type( epics_pv_name,
					&epics_datatype,
					&epics_array_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("epics_datatype = %ld, epics_array_length = %ld",
			epics_datatype, epics_array_length));

	/* Create an MX database record of class 'epics_variable' */

	snprintf(description, sizeof(description),
		"record%ld variable epics_variable ", record_number);

	switch( epics_datatype ) {
	case MX_CA_STRING:
		strlcat( description, "epics_string ", sizeof(description) );
		break;
	case MX_CA_CHAR:
		strlcat( description, "epics_char ", sizeof(description) );
		break;
	case MX_CA_SHORT:
		strlcat( description, "epics_short ", sizeof(description) );
		break;
	case MX_CA_LONG:
		strlcat( description, "epics_long ", sizeof(description) );
		break;
	case MX_CA_FLOAT:
		strlcat( description, "epics_float ", sizeof(description) );
		break;
	case MX_CA_DOUBLE:
		strlcat( description, "epics_double ", sizeof(description) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Remote EPICS PV '%s' is of type %ld "
			"which is not supported by this program.",
			epics_pv_name, epics_datatype );
		break;
	}

	string_length = strlen(description);

	ptr = description + string_length;

	snprintf( ptr, sizeof(description) - string_length,
			"\"\" \"\" %s 1 %ld ",
			epics_pv_name, epics_array_length );

	switch( epics_datatype ) {
	case MX_CA_STRING:
		strlcat( description, "\"\" ", sizeof(description) );
		break;
	default:
		for ( i = 0; i < epics_array_length; i++ ) {
			strlcat( description, "0 ", sizeof(description) );
		}
		break;
	}

	/* Now that we have a record description for the variable,
	 * add it to the MX database.
	 */

	MX_DEBUG( 2,("%s: description = '%s'", fname, description));

	mx_status = mx_create_record_from_description( record_list->list_head,
					description, created_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_finish_record_initialization( *created_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_open_hardware( *created_record );

	return mx_status;
}

static mx_status_type
msauto_epics_motor_position_write_function( void *list_entry_ptr )
{
	static const char fname[] =
		"msauto_epics_motor_position_write_function()";

	MX_AUTOSAVE_LIST_ENTRY *list_entry;
	double motor_set_position;
	char epics_motor_record_name[ MXAUTO_FIELD_ID_NAME_LENGTH+1 ];
	char *ptr;
	int set_flag;
	MX_EPICS_PV set_pv;
	MX_EPICS_PV val_pv;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( list_entry_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The list_entry_ptr passed was NULL." );
	}

	list_entry = (MX_AUTOSAVE_LIST_ENTRY *) list_entry_ptr;

	motor_set_position = *(double *) list_entry->read_value_pointer;

	MX_DEBUG( 2,("%s: motor_set_position = %g", fname, motor_set_position));

	MX_DEBUG( 2,("%s: record_field_id = '%s'",
				fname, list_entry->record_field_id));

	/* Construct the EPICS motor record name without .RBV on the end. */

	strlcpy( epics_motor_record_name, list_entry->record_field_id,
					sizeof(epics_motor_record_name) );

	/* Find the last '.' in the string. */

	ptr = strrchr( epics_motor_record_name, '.' );

	if ( ptr == NULL ) {
		/* If there are no '.' characters in the string, then the
		 * entire string is assumed to be the motor record name.
		 */
	} else {
		/* Null terminate at the end of the motor name. */

		*ptr = '\0';
	}

	mx_info("Restoring position of EPICS motor '%s'.",
			epics_motor_record_name);

	mx_epics_pvname_init( &set_pv, "%s.SET", epics_motor_record_name );
	mx_epics_pvname_init( &val_pv, "%s.VAL", epics_motor_record_name );

	/* Now perform the intricate minuet required to restore the
	 * motor position.
	 */

	/* Change the motor record from 'Use' to 'Set' mode. */

	set_flag = 1;

	mx_status = mx_caput( &set_pv, MX_CA_SHORT, 1, &set_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Change the user position in the EPICS database. */

	mx_status = mx_caput( &val_pv, MX_CA_DOUBLE, 1, &motor_set_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Put the motor record back into 'Use' mode. */

	set_flag = 0;

	mx_status = mx_caput( &set_pv, MX_CA_SHORT, 1, &set_flag );

	return mx_status;
}

#endif /* HAVE_EPICS */

static mx_status_type
msauto_add_to_autosave_list( MX_AUTOSAVE_LIST *autosave_list,
				char *protocol_id,
				char *record_field_id,
				char *extra_arguments,
				unsigned long autosave_flags,
				char *read_record_name,
				char *read_field_name,
				MX_RECORD *read_record,
				char *write_record_name,
				char *write_field_name,
				MX_RECORD *write_record,
				mx_status_type (*write_function)(void *) )
{
	static const char fname[] = "msauto_add_to_autosave_list()";

	MX_AUTOSAVE_LIST_ENTRY *autosave_list_entry_array;
	MX_AUTOSAVE_LIST_ENTRY *autosave_list_entry, *ptr;
	MX_RECORD_FIELD *field;
	mx_status_type (*constructor)( void *, char *, size_t,
					MX_RECORD *, MX_RECORD_FIELD * );
	mx_status_type (*parser)( void *, char *,
				MX_RECORD *, MX_RECORD_FIELD *,
				MX_RECORD_FIELD_PARSE_STATUS * );

	unsigned long j, new_num_entries;
	size_t new_entry_array_size;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s: Adding record '%s' to autosave list.",
		fname, read_record->name));

	j = autosave_list->num_entries;

	if ( (j + 1) % MX_AUTOSAVE_ARRAY_BLOCK_SIZE == 0 ) {
		/* Need to increase the size of the entry_array. */

		new_num_entries = (j + 1 + MX_AUTOSAVE_ARRAY_BLOCK_SIZE);

		MX_DEBUG( 2,
		("%s: Increasing size of autosave list to have %lu entries.",
			fname, new_num_entries));

		new_entry_array_size = new_num_entries
					* sizeof( MX_AUTOSAVE_LIST_ENTRY );

		ptr = (MX_AUTOSAVE_LIST_ENTRY *) realloc(
			autosave_list->entry_array, new_entry_array_size);

		if ( ptr == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory reallocating the entry array "
			"for the autosave list to have %lu elements.",
				new_num_entries );
		}

		autosave_list->entry_array = ptr;
	}

	autosave_list_entry_array = autosave_list->entry_array;

	autosave_list_entry = &autosave_list_entry_array[j];

	/* Initialize the items in the autosave_list_entry structure. */

	strlcpy( autosave_list_entry->protocol_id, protocol_id,
				MXAUTO_PROTOCOL_ID_NAME_LENGTH );
	strlcpy( autosave_list_entry->record_field_id, record_field_id,
				MXAUTO_FIELD_ID_NAME_LENGTH );
	strlcpy( autosave_list_entry->extra_arguments, extra_arguments,
				MXAUTO_FIELD_ID_NAME_LENGTH );

	autosave_list_entry->autosave_flags = autosave_flags;

	strlcpy( autosave_list_entry->read_record_name,
				read_record_name, MXU_RECORD_NAME_LENGTH );

	strlcpy( autosave_list_entry->read_field_name,
				read_field_name, MXU_FIELD_NAME_LENGTH );

	autosave_list_entry->read_record = read_record;

	MX_DEBUG( 2,("%s: read_record_name = '%s'", fname, read_record_name));
	MX_DEBUG( 2,("%s: read_field_name = '%s'", fname, read_field_name));
	MX_DEBUG( 2,("%s: read_record = '%s'", fname, read_record->name));

	strlcpy( autosave_list_entry->write_record_name,
				write_record_name, MXU_RECORD_NAME_LENGTH );

	strlcpy( autosave_list_entry->write_field_name,
				write_field_name, MXU_FIELD_NAME_LENGTH );

	autosave_list_entry->write_record = write_record;

	MX_DEBUG( 2,("%s: write_record_name = '%s'", fname, write_record_name));
	MX_DEBUG( 2,("%s: write_field_name = '%s'", fname, write_field_name));

	if ( write_record == NULL ) {
		MX_DEBUG( 2,("%s: write_record = (null)", fname));
	} else {
		MX_DEBUG( 2,
		("%s: write_record = '%s'", fname, write_record->name));
	}

	mx_status = mx_find_record_field( read_record, "value", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autosave_list_entry->read_value_field = field;

	autosave_list_entry->read_value_pointer
					= mx_get_field_value_pointer( field );

	mx_status = mx_get_token_constructor( field->datatype, &constructor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autosave_list_entry->token_constructor = constructor;

	mx_status = mx_get_token_parser( field->datatype, &parser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autosave_list_entry->token_parser = parser;

	/*---*/

	autosave_list_entry->write_function = write_function;

	if ( write_function != NULL ) {
		autosave_list_entry->write_value_field = NULL;
		autosave_list_entry->write_value_pointer = NULL;
	} else {
		mx_status = mx_find_record_field( write_record,
						"value", &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		autosave_list_entry->write_value_field = field;

		autosave_list_entry->write_value_pointer
					= mx_get_field_value_pointer( field );

		mx_status = mx_get_token_constructor( field->datatype,
							&constructor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( constructor != autosave_list_entry->token_constructor ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The write field '%s.%s' is not the same data type "
			"as the read field '%s.%s'.",
				write_record_name, write_field_name,
				read_record_name, read_field_name );
		}

		mx_status = mx_get_token_parser( field->datatype, &parser );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( parser != autosave_list_entry->token_parser ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The write field '%s.%s' is not the same data type "
			"as the read field '%s.%s'.",
				write_record_name, write_field_name,
				read_record_name, read_field_name );
		}
	}

	/* We're done so increment the number of entries in the list. */

	autosave_list->num_entries = j + 1;

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
msauto_construct_autosave_list( MX_AUTOSAVE_LIST *autosave_list,
                		FILE *autosave_list_file,
				char *autosave_list_filename,
				MX_RECORD *record_list )
{
	static const char fname[] = "msauto_construct_autosave_list()";

	MX_RECORD *created_read_record, *created_write_record;
	char read_record_name[MXU_RECORD_NAME_LENGTH+1];
	char read_field_name[MXU_FIELD_NAME_LENGTH+1];
	char write_record_name[MXU_RECORD_NAME_LENGTH+1];
	char write_field_name[MXU_FIELD_NAME_LENGTH+1];
	char format[50];
	char buffer[350];
	char protocol_id[MXAUTO_PROTOCOL_ID_NAME_LENGTH+1];
	char record_field_id[MXAUTO_FIELD_ID_NAME_LENGTH+1];
	char extra_arguments[MXAUTO_FIELD_ID_NAME_LENGTH+1];
	char *ptr;
	long i, num_items_read;
	unsigned long autosave_flags;
	mx_status_type mx_status;
	mx_status_type (*write_function)(void *);

	MX_DEBUG( 2,("%s: autosave_list_filename = '%s'",
			fname, autosave_list_filename));

	/*
	 * Allocate initial blocks of space to store the autosave list entries.
	 */

	autosave_list->num_entries = 0;

	autosave_list->entry_array = (MX_AUTOSAVE_LIST_ENTRY *)
			malloc( MX_AUTOSAVE_ARRAY_BLOCK_SIZE
				* sizeof(MX_AUTOSAVE_LIST_ENTRY) );

	if ( autosave_list->entry_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate %d autosave list entries",
				MX_AUTOSAVE_ARRAY_BLOCK_SIZE );
	}

	/* Now read in all of the items in the autosave list. */

	/* The format of each line is:
	 *
	 * protocol_id record_field_id flags extra_arguments
	 *
	 * An example on a local server would be:
	 *
	 *    mx localhost:edge_energy.value 0x0
	 *
	 * Note that the extra arguments are optional and that the
	 * default port number is 9727.
	 *
	 * For a motor position, we want to restore the position to
	 * different field from where we read it.
	 *
	 *    mx 192.168.1.2:theta.position 0x1 192.168.1.2:theta.set_position
	 *
	 * where the flag value 0x1 is what tells mxautosave to restore
	 * to a different variable name.
	 *
	 * If the server is on nonstandard port 7890, you would use
	 * something like:
	 *
	 *    mx example.com@7890:d_spacing.value 0x0
	 */

	snprintf( format, sizeof(format),
			"%%%ds %%%ds %%lx %%%ds",
			(int) (sizeof(protocol_id) - 1),
			(int) (sizeof(record_field_id) - 1),
			(int) (sizeof(extra_arguments) - 1) );

	MX_DEBUG( 2,("%s: format = '%s'", fname, format));

	for ( i = 0; ; i++ ) {
		mx_fgets(buffer, sizeof buffer, autosave_list_file);

		if ( feof(autosave_list_file) ) {
			break;		/* Exit the for loop. */
		}

		if ( ferror(autosave_list_file) ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
	"Unexpected error reading line %ld of the autosave list file '%s'.",
				i, autosave_list_filename );
		}

		MX_DEBUG( 2,("%s: Line %ld = '%s'",fname,i,buffer));

		num_items_read = sscanf( buffer, format, protocol_id,
			record_field_id, &autosave_flags, extra_arguments );

		if ( num_items_read < 3 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Line %ld of autosave list file '%s' is incorrectly "
			"formatted.  Contents = '%s'",
				i, autosave_list_filename, buffer );
		}
		if ( num_items_read == 3 ) {
			strlcpy( extra_arguments, "", sizeof(extra_arguments) );
		}

		MX_DEBUG( 2,("protocol_id = '%s', record_field_id = '%s', "
				"autosave_flags = %#lx, extra_arguments = '%s'",
			protocol_id, record_field_id, autosave_flags,
			extra_arguments ));

		write_function = NULL;

		/* Add the record to read from. */

		if ( strcmp( protocol_id, "mx" ) == 0 ) {
			MX_DEBUG( 2,
			("%s: *** add mx read variable for '%s' ***",
			 	fname, record_field_id));

			mx_status = msauto_add_mx_variable_to_database(
				record_list,
				record_field_id, i, &created_read_record,
				read_record_name, sizeof(read_record_name) - 1,
				read_field_name, sizeof(read_field_name) - 1 );
		} else
		if ( strcmp( protocol_id, "epics" ) == 0 ) {
			MX_DEBUG( 2,
			("%s: *** add epics read variable for '%s' ***",
			 	fname, record_field_id));

			mx_status = msauto_add_epics_variable_to_database(
				record_list,
				record_field_id, i, &created_read_record,
				read_record_name, sizeof(read_record_name) - 1,
				read_field_name, sizeof(read_field_name) - 1 );
		} else
		if ( strcmp( protocol_id, "epics_motor_position" ) == 0 ) {
			MX_DEBUG( 2,
		("%s: *** add epics motor position read variable for '%s' ***",
			 	fname, record_field_id));

			mx_status = msauto_add_epics_variable_to_database(
				record_list,
				record_field_id, i, &created_read_record,
				read_record_name, sizeof(read_record_name) - 1,
				read_field_name, sizeof(read_field_name) - 1 );
		} else {
			ptr = strrchr( protocol_id, '.' );

			if ( ptr && (strcmp( ptr, ".value" ) == 0) ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		    "Autosave list file '%s' appears to be using the file "
		    "format used by MX 1.1 and before.  You must convert "
		    "this file to the new format used by MX 1.2 and above.",
					autosave_list_filename );
			} else {
				return mx_error( MXE_UNSUPPORTED, fname,
		    "Unsupported protocol type '%s' requested for line '%s'",
			    		protocol_id, buffer );
			}
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If requested, add the record to write to. */

		MX_DEBUG( 2,("%s: autosave_flags = %#lx", fname, autosave_flags));

		if ( ( autosave_flags & 0x1 ) == 0 ) {
			MX_DEBUG( 2,
		("%s: *** created_write_record == created_read_record ***",
		 		fname));

			created_write_record = created_read_record;

			strlcpy( write_record_name, read_record_name,
					MXU_RECORD_NAME_LENGTH );
			strlcpy( write_field_name, read_field_name,
					MXU_FIELD_NAME_LENGTH );
		} else {
			i++;

			if ( strcmp( protocol_id, "mx" ) == 0 ) {
				MX_DEBUG( 2,
			("%s: *** add mx write variable for '%s' ***",
			 	fname, extra_arguments));

			    mx_status = msauto_add_mx_variable_to_database(
					record_list, extra_arguments,
					i, &created_write_record,
				write_record_name, sizeof(write_record_name)-1,
				write_field_name, sizeof(write_field_name)-1 );
			} else
			if ( strcmp( protocol_id, "epics" ) == 0 ) {
				MX_DEBUG( 2,
			("%s: *** add epics write variable for '%s' ***",
			 	fname, extra_arguments));

			    mx_status = msauto_add_epics_variable_to_database(
					record_list, extra_arguments,
					i, &created_write_record,
				write_record_name, sizeof(write_record_name)-1,
				write_field_name, sizeof(write_field_name)-1 );
			} else
			if (strcmp( protocol_id, "epics_motor_position" ) == 0)
			{
				created_write_record = NULL;

				strlcpy( write_record_name, "",
					sizeof(write_record_name) );

				strlcpy( write_field_name, "",
					sizeof(write_field_name) );

#if HAVE_EPICS
				write_function =
				    msauto_epics_motor_position_write_function;
#else /* HAVE_EPICS */
				return mx_error( MXE_UNSUPPORTED, fname,
	    "Support for EPICS is not compiled into this copy of mxautosave." );
#endif /* HAVE_EPICS */

			} else {
				return mx_error( MXE_UNSUPPORTED, fname,
		    "Unsupported protocol type '%s' requested for line '%s'",
		    		protocol_id, buffer );
			}
		}

		/* Add the record(s) to the autosave list. */

		MX_DEBUG( 2,
		    ("%s: about to call msauto_add_to_autosave_list()", fname));

		MX_DEBUG( 2,("%s: read_record_name = '%s'",
			fname, read_record_name));
		MX_DEBUG( 2,("%s: read_field_name = '%s'",
			fname, read_field_name));
		MX_DEBUG( 2,("%s: created_read_record = '%s'",
			fname, created_read_record->name));

		MX_DEBUG( 2,("%s: write_record_name = '%s'",
			fname, write_record_name));
		MX_DEBUG( 2,("%s: write_field_name = '%s'",
			fname, write_field_name));

		if ( created_write_record == NULL ) {
			MX_DEBUG( 2,
				("%s: created_write_record = (null)", fname));
		} else {
			MX_DEBUG( 2,("%s: created_write_record = '%s'",
				fname, created_write_record->name));
		}

		mx_status = msauto_add_to_autosave_list( autosave_list,
				protocol_id, record_field_id, extra_arguments,
				autosave_flags,
				read_record_name, read_field_name,
				created_read_record,
				write_record_name, write_field_name,
				created_write_record,
				write_function );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
msauto_poll_autosave_list( MX_AUTOSAVE_LIST *autosave_list )
{
	MX_AUTOSAVE_LIST_ENTRY *entry_array, *entry;
	long i;
	mx_status_type mx_status;

	entry_array = autosave_list->entry_array;

	for ( i = 0; i < autosave_list->num_entries; i++ ) {

		entry = &entry_array[i];

		mx_status = mx_receive_variable( entry->read_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
msauto_save_fields_to_autosave_file( char *autosave_filename,
			MX_AUTOSAVE_LIST *autosave_list )
{
	static const char fname[] = "msauto_save_fields_to_autosave_file()";

	MX_AUTOSAVE_LIST_ENTRY *autosave_list_entry;
	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	FILE *autosave_file;
	char buffer[500];
	int saved_errno, result;
	long i;
	mx_status_type mx_status;

	MX_DEBUG( 1,("%s: saving fields to autosave file '%s'",
		fname, autosave_filename));

	mx_status = msauto_poll_autosave_list( autosave_list );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Delete the previous version of the autosave file, if it exists. */

	result = remove( autosave_filename );

	if ( result != 0 ) {
		saved_errno = errno;

		if ( saved_errno != ENOENT ) {

			(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Error during attempt to delete old autosave file '%s'.  "
		"Errno = %d, error string = '%s'",
			autosave_filename, saved_errno,
			strerror(saved_errno));
		}
	}

	/* Open the autosave file for writing. */

	autosave_file = fopen( autosave_filename, "w" );

	if ( autosave_file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Could not open the autosave file '%s' for writing.  "
		"Errno = %d, error string = '%s'",
			autosave_filename, saved_errno,
			strerror(saved_errno));
	}

	/* Write out the new contents of the autosave file. */

	for ( i = 0; i < autosave_list->num_entries; i++ ) {
		autosave_list_entry = &(autosave_list->entry_array)[i];

		snprintf( buffer, sizeof(buffer),
				"%s.%s", autosave_list_entry->read_record_name,
				autosave_list_entry->read_field_name );

		MX_DEBUG( 2,("%s: Saving value of '%s.%s'", fname,
				autosave_list_entry->read_record_name,
				autosave_list_entry->read_field_name ));

		fprintf( autosave_file, "%-s  ", buffer );

		buffer[0] = '\0';

		value_field = autosave_list_entry->read_value_field;

		value_ptr = autosave_list_entry->read_value_pointer;

		if ( (value_field->num_dimensions == 0)
		  || ((value_field->datatype == MXFT_STRING)
		    && (value_field->num_dimensions == 1))) {

			/* Single token */

			mx_status = ( autosave_list_entry->token_constructor ) (
					value_ptr,
					buffer,
					sizeof(buffer),
					autosave_list_entry->read_record,
					value_field );
		} else {
			mx_status = mx_create_array_description(
					value_ptr,
					value_field->num_dimensions - 1,
					buffer,
					sizeof(buffer),
					autosave_list_entry->read_record,
					value_field,
					autosave_list_entry->token_constructor );
		}

		if ( mx_status.code != MXE_SUCCESS ) {
			fclose( autosave_file );

			return mx_status;
		}

		MX_DEBUG( 2,("%s: buffer = '%s'", fname, buffer));

		fprintf( autosave_file, "%s\n", buffer );
	}

	/* Write out a marker to indicate that this is the end of the file. */

	fprintf( autosave_file, "********\n" );
	fclose( autosave_file );

	return mx_status;
}

#define CLOSE_AUTOSAVE_FILES \
		do {                                           \
			if ( autosave1 != NULL ) {             \
				(void) fclose( autosave1 );    \
			}                                      \
			if ( autosave2 != NULL ) {             \
				(void) fclose( autosave2 );    \
			}                                      \
		} while(0)

static mx_status_type
msauto_choose_autosave_file_to_use(
		char *autosave1_filename,
		char *autosave2_filename,
		char **filename_to_use )
{
	static const char fname[] = "msauto_choose_autosave_file_to_use()";

	FILE *autosave1, *autosave2, *older_file, *newer_file;
	FILE *autosave_to_use;
	char *older_filename, *newer_filename;
	struct stat stat_struct1, stat_struct2;
	char buffer[500];
	int saved_errno, savefile_is_complete;

	autosave_to_use = NULL;
	*filename_to_use = NULL;

	/* Do the autosave files exist and are they readable? */

	if ( strlen(autosave1_filename) == 0 ) {
		autosave1 = NULL;
	} else {
		autosave1 = fopen( autosave1_filename, "r" );

		if ( autosave1 == NULL ) {
			saved_errno = errno;
			mx_info(
"Warning: Could not open autosave file '%s' for reading.  Reason = '%s'",
				autosave1_filename, strerror(saved_errno) );
		}
	}

	if ( strlen(autosave2_filename) == 0 ) {
		autosave2 = NULL;
	} else {
		autosave2 = fopen( autosave2_filename, "r" );

		if ( autosave2 == NULL ) {
			saved_errno = errno;
			mx_info( 
"Warning: Could not open autosave file '%s' for reading.  Reason = '%s'",
				autosave2_filename, strerror(saved_errno) );
		}
	}

	if ( autosave1 == NULL && autosave2 == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	if ( autosave1 == NULL && autosave2 != NULL ) {
		autosave_to_use = autosave2;
		*filename_to_use = autosave2_filename;
	} else
	if ( autosave1 != NULL && autosave2 == NULL ) {
		autosave_to_use = autosave1;
		*filename_to_use = autosave1_filename;
	} else {
		autosave_to_use = NULL;         /* Both files exist. */
		*filename_to_use = NULL;
	}

	savefile_is_complete = FALSE;

	if ( autosave_to_use == NULL ) {

		/* If both autosave files exist, check to see which file
		 * is newer.
		 */

		if ( fstat( fileno(autosave1), &stat_struct1 ) != 0 ) {
			saved_errno = errno;

			CLOSE_AUTOSAVE_FILES;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Could not fstat() autosave file 1.  Reason = '%s'",
				strerror( saved_errno ) );
		}
		if ( fstat( fileno(autosave2), &stat_struct2 ) != 0 ) {
			saved_errno = errno;

			CLOSE_AUTOSAVE_FILES;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Could not fstat() autosave file 2.  Reason = '%s'",
				strerror( saved_errno ) );
		}
		if ( stat_struct1.st_mtime >= stat_struct2.st_mtime ) {
			newer_file = autosave1;
			older_file = autosave2;
			newer_filename = autosave1_filename;
			older_filename = autosave2_filename;
		} else {
			newer_file = autosave2;
			older_file = autosave1;
			newer_filename = autosave2_filename;
			older_filename = autosave1_filename;
		}

		/* Is the newer file a complete savefile?  Look for a line
		 * in the file starting with an asterisk.
		 */

		mx_fgets(buffer, sizeof buffer, newer_file);

		while ( ! feof( newer_file ) && ! ferror( newer_file ) ) {

			if ( buffer[0] == '*' ) {
				savefile_is_complete = TRUE;
				break;     /* Exit the while() loop. */
			}
			mx_fgets(buffer, sizeof buffer, newer_file);
		}

		if ( savefile_is_complete ) {
			autosave_to_use = newer_file;
			*filename_to_use = newer_filename;
		} else {
			autosave_to_use = older_file;
			*filename_to_use = older_filename;
		}
	}

	if ( savefile_is_complete == FALSE ) {

		/* Is the file 'autosave_to_use' a complete savefile?
		 * Look for a line in the file starting with an asterisk.
		 */

		mx_fgets(buffer, sizeof buffer, autosave_to_use);

		while (!feof(autosave_to_use) && !ferror(autosave_to_use)){

			if ( buffer[0] == '*' ) {
				savefile_is_complete = TRUE;
				break;     /* Exit the while() loop. */
			}
			mx_fgets(buffer, sizeof buffer, autosave_to_use);
		}

		if ( savefile_is_complete == FALSE ) {
			*filename_to_use = NULL;
		}
	}

	CLOSE_AUTOSAVE_FILES;

	return MX_SUCCESSFUL_RESULT;
}

#define SEPARATORS " \t"

mx_status_type
msauto_restore_fields_from_autosave_files(
			char *autosave1_filename, char * autosave2_filename,
			MX_AUTOSAVE_LIST *autosave_list )
{
	static const char fname[] =
			"msauto_restore_fields_from_autosave_files()";

	MX_AUTOSAVE_LIST_ENTRY *autosave_list_entry;
	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	MX_RECORD_FIELD_PARSE_STATUS parse_status;
	FILE *autosave_to_use;
	char *filename_to_use;
	char token_buffer[500];
	char buffer[500];
	char *buffer_ptr;
	char autosave_record_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char autosave_list_read_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char autosave_list_write_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char separators[] = MX_RECORD_FIELD_SEPARATORS;
	char autosave_backup_filename[ MXU_FILENAME_LENGTH + 1 ];
	int saved_errno;
	long i;
	unsigned long autosave_flags;
	mx_status_type mx_status;

	mx_status = msauto_choose_autosave_file_to_use(
				autosave1_filename, autosave2_filename,
				&filename_to_use );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( filename_to_use == NULL ) {
		mx_info(
"Warning: No complete autosave file was found.  No parameters will be restored."
			);

		return MX_SUCCESSFUL_RESULT;
	}

	snprintf( autosave_backup_filename,
		sizeof(autosave_backup_filename),
		"%s_bak", filename_to_use );

	MX_DEBUG( 2,("%s: autosave_backup_filename = '%s'",
		fname, autosave_backup_filename));

	mx_status = mx_copy_file( filename_to_use,
				autosave_backup_filename, 0644,
				MXF_CP_OVERWRITE );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_warning( "The attempt to backup autosave file '%s' failed.",
				filename_to_use );
	} else {
		mx_info( "Autosave file '%s' copied to backup file '%s'",
			filename_to_use, autosave_backup_filename );
	}

	autosave_to_use = fopen( filename_to_use, "r" );

	if ( autosave_to_use == NULL ) {
		saved_errno = errno;
		mx_warning(
"Could not open autosave file '%s' for reading, so no parameters will be "
"restored.  Reason = '%s'.  This is odd since we successfully opened that "
"file just a moment ago.",
			filename_to_use, strerror(saved_errno) );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_info("Restoring parameters from autosave file '%s'",
			filename_to_use);

	/* Step through the lines in the autosave file and restore
	 * parameters as we go.  Do _not_ return with an error status
	 * code from here on.  To do so would cause the autosave process
	 * to exit.
	 */

	for ( i = 0; i < autosave_list->num_entries; i++ ) {
		autosave_list_entry = &(autosave_list->entry_array)[i];

		mx_fgets( buffer, sizeof buffer, autosave_to_use );

		if ( feof( autosave_to_use ) || ferror( autosave_to_use )
			  || (buffer[0] == '*') ) {

			fclose( autosave_to_use );

			(void) mx_error( MXE_FILE_IO_ERROR, fname,
	"Only %ld autosave entries were read from autosave file '%s'.  "
	"%lu entries were expected.",
			i, filename_to_use, autosave_list->num_entries );

			return MX_SUCCESSFUL_RESULT;
		}

		buffer_ptr = buffer + strspn( buffer, SEPARATORS );

		sscanf( buffer_ptr, "%s", autosave_record_field_name );

		buffer_ptr += strlen( autosave_record_field_name );

		buffer_ptr += strspn( buffer_ptr, SEPARATORS );

		snprintf( autosave_list_read_field_name,
			sizeof(autosave_list_read_field_name),
			"%s.%s",
			autosave_list_entry->read_record_name,
			autosave_list_entry->read_field_name );

		snprintf( autosave_list_write_field_name,
			sizeof(autosave_list_write_field_name),
			"%s.%s",
			autosave_list_entry->write_record_name,
			autosave_list_entry->write_field_name );

		MX_DEBUG( 2,("%s: autosave_record_field_name = '%s'",
			fname, autosave_record_field_name));
		MX_DEBUG( 2,("%s: autosave_list_read_field_name = '%s'",
			fname, autosave_list_read_field_name));
		MX_DEBUG( 2,("%s: autosave_list_write_field_name = '%s'",
			fname, autosave_list_write_field_name));

		if ( strcmp( autosave_record_field_name,
				autosave_list_read_field_name ) != 0 ) {

			fclose( autosave_to_use );

			(void) mx_error( MXE_FILE_IO_ERROR, fname,
	"Autosave file '%s' and the autosave list are out of "
	"synchronization at line %ld.  Autosave record field name = '%s', "
	"Autosave list read field name = '%s'", filename_to_use, i, 
				autosave_record_field_name,
				autosave_list_read_field_name );

			return MX_SUCCESSFUL_RESULT;
		}
		
		if ( autosave_list_entry->write_value_field == NULL ) {
			value_field = autosave_list_entry->read_value_field;
			value_ptr = autosave_list_entry->read_value_pointer;
		} else {
			value_field = autosave_list_entry->write_value_field;
			value_ptr = autosave_list_entry->write_value_pointer;
		}

		/* Initialize the token parser. */

		mx_initialize_parse_status( &parse_status,
						buffer_ptr, separators );

		/* If this is a string field, find out what the maximum
		 * length of a string token is for user by the token parser.
		 */

		if ( value_field->datatype == MXFT_STRING ) {
			parse_status.max_string_token_length =
				mx_get_max_string_token_length( value_field );
		} else {
			parse_status.max_string_token_length = 0L;
		}

		/* Parse the tokens. */

		if ( (value_field->num_dimensions == 0)
		  || ((value_field->datatype == MXFT_STRING)
		    && (value_field->num_dimensions == 1))) {

			/* Single token */

			mx_status = mx_get_next_record_token( &parse_status,
					token_buffer, sizeof(token_buffer) );

			if ( mx_status.code != MXE_SUCCESS ) {
				fclose( autosave_to_use );

				return MX_SUCCESSFUL_RESULT;
			}

			mx_status = (autosave_list_entry->token_parser) (
					value_ptr,
					token_buffer,
					autosave_list_entry->write_record,
					value_field,
					&parse_status );
		} else {
			/* Array of tokens. */

			mx_status = mx_parse_array_description(
					value_ptr,
					value_field->num_dimensions - 1,
					autosave_list_entry->write_record,
					value_field,
					&parse_status,
					autosave_list_entry->token_parser );
		}
		if ( mx_status.code != MXE_SUCCESS ) {
			fclose( autosave_to_use );

			return MX_SUCCESSFUL_RESULT;
		}

		if ( autosave_list_entry->write_function == NULL ) {
			autosave_flags = autosave_list_entry->autosave_flags;

			if ( autosave_flags & 0x1 ) {
				mx_info( "Restoring value of '%s' to '%s'.",
					autosave_record_field_name,
					autosave_list_write_field_name );
			} else {
				mx_info( "Restoring value of '%s'.",
					autosave_record_field_name );
			}

			(void) mx_send_variable(
					autosave_list_entry->write_record );
		} else {
			mx_status = autosave_list_entry->write_function(
						autosave_list_entry );
		}
	}

	fclose( autosave_to_use );

	return MX_SUCCESSFUL_RESULT;
}

