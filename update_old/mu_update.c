/*
 * Name:    mu_update.c
 *
 * Purpose: main() routine for the MX auto save/restore and record polling
 *          utility.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
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
#include <sys/stat.h>

#include "mx_unistd.h"

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_version.h"
#include "mx_record.h"
#include "mx_variable.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_syslog.h"

#include "mu_update.h"

#if ! defined( OS_WIN32 )

static void
mxupd_exit_handler( void )
{
	mx_info( "*** MX update process exiting. ***" );
}

static void
mxupd_sigterm_handler( int signal_number )
{
	mx_info( "Received a request to shutdown via a SIGTERM signal." );

	exit(0);
}

static void
mxupd_install_signal_and_exit_handlers( void )
{
	atexit( mxupd_exit_handler );

	signal( SIGTERM, mxupd_sigterm_handler );

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
mxupd_print_timestamp( void )
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
mxupd_info_output_function( char *string )
{
	mxupd_print_timestamp();
	fprintf( stderr, "%s\n", string );
}

static void
mxupd_warning_output_function( char *string )
{
	mxupd_print_timestamp();
	fprintf( stderr, "Warning: %s\n", string );
}

static void
mxupd_error_output_function( char *string )
{
	mxupd_print_timestamp();
	fprintf( stderr, "%s\n", string );
}

static void
mxupd_debug_output_function( char *string )
{
	mxupd_print_timestamp();
	fprintf( stderr, "%s\n", string );
}

static void
mxupd_setup_output_functions( void )
{
	mx_set_info_output_function( mxupd_info_output_function );
	mx_set_warning_output_function( mxupd_warning_output_function );
	mx_set_error_output_function( mxupd_error_output_function );
	mx_set_debug_output_function( mxupd_debug_output_function );
}

/*------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
	const char fname[] = "main()";

	static MXUPD_UPDATE_LIST update_list_array[MXUPD_NUM_UPDATE_LISTS];

	MXUPD_UPDATE_LIST *update_list;

	MX_RECORD *server_record;
	char server_name[MXU_HOSTNAME_LENGTH+1];
	int server_port;

	MX_CLOCK_TICK event_interval[MXUPD_NUM_UPDATE_LISTS];
	MX_CLOCK_TICK next_event_time[MXUPD_NUM_UPDATE_LISTS];
	MX_CLOCK_TICK current_time;

	double event_interval_in_seconds[MXUPD_NUM_UPDATE_LISTS]
		= { 30.0, 1.0 };

	FILE *update_list_file;
	char update_list_filename[MXU_FILENAME_LENGTH+1];
	char autosave1_filename[MXU_FILENAME_LENGTH+1];
	char autosave2_filename[MXU_FILENAME_LENGTH+1];
	char *autosave_filename;
	char ident_string[80];
	int i, debug_level, num_non_option_arguments, update_lists_empty;
	int install_syslog_handler, syslog_number, syslog_options;
	int default_display_precision;
	mx_status_type status;

	static char usage[] =
"Usage: mxupdate [-d debug_level] [-l log_number] [-L log_number]\n"
"  [-P display_precision] server_name server_port update_list_filename\n"
"  autosave_filename_1 autosave_filename_2\n\n";

#if HAVE_GETOPT
	int c, error_flag;
#endif

	mxupd_setup_output_functions();

#if ! defined( OS_WIN32 )
	mxupd_install_signal_and_exit_handlers();
#endif

	debug_level = 0;

	default_display_precision = 8;

	install_syslog_handler = FALSE;

	syslog_number = -1;
	syslog_options = 0;

#if HAVE_GETOPT
	/* Process command line arguments via getopt, if any. */

	error_flag = FALSE;

	while ((c = getopt(argc, argv, "d:l:L:P:")) != -1 ) {
		switch(c) {
		case 'd':
			debug_level = atoi( optarg );
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
		case '?':
			error_flag = TRUE;
			break;
		}

		if ( error_flag == TRUE ) {
			fputs( usage, stderr );
			exit(1);
		}
	}

	i = optind;

	num_non_option_arguments = argc - optind;

#else  /* HAVE_GETOPT */

	i = 1;

	num_non_option_arguments = argc - 1;

#endif /* HAVE_GETOPT */

	if ( num_non_option_arguments != 5 ) {
		fprintf(stderr, "Error: incorrect number of arguments.\n\n");
		fputs( usage, stderr );
		exit(1);
	}

	/* Copy the other command line arguments. */

	mx_strncpy( server_name, argv[i], MXU_HOSTNAME_LENGTH + 1);

	server_port = atoi( argv[i+1] );

	mx_strncpy( update_list_filename, argv[i+2], MXU_FILENAME_LENGTH + 1);
	mx_strncpy( autosave1_filename, argv[i+3], MXU_FILENAME_LENGTH + 1);
	mx_strncpy( autosave2_filename, argv[i+4], MXU_FILENAME_LENGTH + 1);

	mx_set_debug_level( debug_level );

	if ( install_syslog_handler ) {
		sprintf( ident_string, "mxupdate@%d", server_port );

		status = mx_install_syslog_handler( ident_string,
					syslog_number, syslog_options );

		if ( status.code != MXE_SUCCESS )
			exit(1);
	}

	mx_info( "*** MX update utility %s started at %s ***",
			mx_get_version_string(),
			mx_current_time_string( NULL, 0 ) );

	/* Initialize the MX time keeping functions. */

	mx_initialize_clock_ticks();

	current_time = mx_current_clock_tick();

	MX_DEBUG(-2,
	("%g clock ticks per second, current time = (%ld,%ld)",
		mx_clock_ticks_per_second(),
		current_time.high_order,
		(long) current_time.low_order));

	for ( i = 0; i < MXUPD_NUM_UPDATE_LISTS; i++ ) {
		event_interval[i] = mx_convert_seconds_to_clock_ticks(
					event_interval_in_seconds[i] );

		MX_DEBUG(-2,
		("Event interval %d:  %g seconds, (%ld,%ld) clock ticks.",
			i, event_interval_in_seconds[i],
			event_interval[i].high_order,
			(long) event_interval[i].low_order));

		next_event_time[i] = current_time;
	}

	/* Initialize the subsecond sleep functions (if they need it). */

	mx_msleep(1);

	/* Does the file containing the list of record fields exist and
	 * is it readable?
	 */

	update_list_file = fopen( update_list_filename, "r" );

	if ( update_list_file == NULL ) {
		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Couldn't open the update list file '%s' for reading.",
			update_list_filename );

		exit( MXE_FILE_IO_ERROR );
	}

	/* Connect to the remote MX server */

	status = mxupd_connect_to_mx_server( &server_record,
						server_name, server_port,
						default_display_precision );

	if ( status.code != MXE_SUCCESS )
		exit( status.code );

	/* Construct the update list data structure. */

	status = mxupd_construct_update_list( MXUPD_NUM_UPDATE_LISTS,
				update_list_array,
				update_list_file, update_list_filename,
				server_record );

	if ( status.code != MXE_SUCCESS )
		exit( status.code );

#if 0
	/* Print out the list of records in the generated record list. */

	{
		MX_RECORD *current_record, *list_head_record;

		MX_DEBUG(-2,("%s: List of local database records:", fname));

		list_head_record = server_record->list_head;

		current_record = list_head_record->next_record;

		while ( current_record != list_head_record ) {
			MX_DEBUG(-2,("    %s", current_record->name));

			current_record = current_record->next_record;
		}
	}
#endif

#if 0
	/* Print out the contents of each update list. */

	MX_DEBUG(-2,("%s: Number of update lists = %d",
				fname, MXUPD_NUM_UPDATE_LISTS ));

	for ( i = 0; i < MXUPD_NUM_UPDATE_LISTS; i++ ) {

		MXUPD_UPDATE_LIST_ENTRY *update_list_entry;
		MXUPD_UPDATE_LIST_ENTRY *update_list_entry_array;
		int j;

		update_list = &update_list_array[i];

		MX_DEBUG(-2,("*** Update list %d: (%ld entries)",
			i, update_list->num_entries ));

		update_list_entry_array = update_list->entry_array;

		for ( j = 0; j < update_list->num_entries; j++ ) {
			update_list_entry = &update_list_entry_array[j];

			MX_DEBUG(-2,("        %s",
			  update_list_entry->local_record->name));
		}
	}
#endif

	/* Check to see if all of the update lists are empty. */

	update_lists_empty = TRUE;

	for ( i = 0; i < MXUPD_NUM_UPDATE_LISTS; i++ ) {
		update_list = &update_list_array[i];

		if ( update_list->num_entries != 0 ) {
			update_lists_empty = FALSE;
			break;
		}
	}

	if ( update_lists_empty ) {
		mx_warning( "All of the MX update lists are empty." );

		mx_info("Mxupdate has nothing to do, so it will exit now.");

		exit(0);
	}

	/* Restore the values in the most current autosave file to
	 * the MX database.
	 */

	update_list = &update_list_array[0];

	status = mxupd_restore_fields_from_autosave_files(
			autosave1_filename, autosave2_filename,
			update_list );

	if ( status.code != MXE_SUCCESS )
		exit(status.code);

	/*********************** Loop forever. *************************/

	autosave_filename = autosave1_filename;

	MX_DEBUG(-2,("%s: Starting update loop.", fname));

	while(1) {
		current_time = mx_current_clock_tick();

		/* Check to see if it is time to rewrite an autosave file. */

		if ( mx_compare_clock_ticks( current_time,
					next_event_time[0] ) > 0 ) {

			MX_DEBUG(1,
	("***** Time to rewrite autosave file.  current_time = (%ld,%ld)",
				current_time.high_order,
				(long) current_time.low_order));

			update_list = &update_list_array[0];

			status = mxupd_save_fields_to_autosave_file(
					autosave_filename, update_list );

			if ( status.code == MXE_NETWORK_CONNECTION_LOST )
				exit( status.code );

			if ( autosave_filename == autosave1_filename ) {
				autosave_filename = autosave2_filename;
			} else {
				autosave_filename = autosave1_filename;
			}

			next_event_time[0] = mx_add_clock_ticks(
						next_event_time[0],
						event_interval[0] );
		}

		/* Check to see if the next event time for a given
		 * polling interval has arrived.
		 */

		for ( i = 1; i < MXUPD_NUM_UPDATE_LISTS; i++ ) {
			if ( mx_compare_clock_ticks( current_time,
					next_event_time[i] ) > 0 ) {

				MX_DEBUG(1,
	("===== Time to update poll group %d.  current_time = (%ld,%ld)",
				i, current_time.high_order,
				(long) current_time.low_order));

				update_list = &update_list_array[i];

				status = mxupd_poll_update_list( update_list );

				if (status.code == MXE_NETWORK_CONNECTION_LOST)
					exit( status.code );

				next_event_time[i] = mx_add_clock_ticks(
						next_event_time[i],
						event_interval[i] );
			}
		}
		mx_msleep(10);
	}
}

mx_status_type
mxupd_connect_to_mx_server( MX_RECORD **server_record,
				char *server_name, int server_port,
				int default_display_precision )
{
	const char fname[] = "mxupd_connect_to_mx_server()";

	MX_RECORD *record_list;
	MX_LIST_HEAD *list_head_struct;
	static char description[MXU_RECORD_DESCRIPTION_LENGTH + 1];
	int i, max_retries;
	mx_status_type status;

	MX_DEBUG(-2,("%s: server_name = '%s', server_port = %d",
			fname, server_name, server_port ));

	/* Initialize MX device drivers. */

	status = mx_initialize_drivers();

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Set up a record list to put this server record in. */

	record_list = mx_initialize_record_list();

	if ( record_list == NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to setup an MX record list." );
	}

	/* Set the default floating point display precision. */

	list_head_struct = mx_get_record_list_head_struct( record_list );

	if ( list_head_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX record list head structure is NULL." );
	}

	list_head_struct->default_precision = default_display_precision;

	/* Set the MX program name. */

	mx_strncpy( list_head_struct->program_name, "mxupdate",
				MXU_PROGRAM_NAME_LENGTH );

	/* Create a record description for this server. */

	sprintf( description,
		"remote_server server network tcpip_server \"\" \"\" 0x0 %s %d",
			server_name, server_port );

	MX_DEBUG(-2,("%s: description = '%s'", fname, description));

	status = mx_create_record_from_description( record_list, description,
							server_record, 0 );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_finish_record_initialization( *server_record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the record list as ready to be used. */

	list_head_struct->list_is_active = TRUE;
	list_head_struct->fixup_records_in_use = FALSE;

	/* Try to connect to the MX server. */

	max_retries = 100;

	for ( i = 0; i < max_retries; i++ ) {
		status = mx_open_hardware( *server_record );

		if ( status.code == MXE_SUCCESS )
			break;                 /* Exit the for() loop. */

		switch( status.code ) {
		case MXE_NETWORK_IO_ERROR:
			break;                 /* Try again. */
		default:
			return status;
			break;
		}
		mx_msleep(1000);
	}

	if ( status.code != MXE_SUCCESS ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"%d attempts to connect to the MX server '%s' at port %d have failed.  "
"Update process aborting...", max_retries, server_name, server_port );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_format_variable_description( MX_RECORD *server_record,
		char *record_name, char *field_name, int record_number,
		char *description, long description_length,
		long datatype, long num_dimensions, long *dimension_array )
{
	const char fname[] = "mx_format_variable_description()";

	size_t buffer_left;
	long i, num_elements;
	char *ptr;

	if ( num_dimensions <= 0 ) {
		num_dimensions = 1;
		dimension_array[0] = 1;
	}

	/* Create an MX database record of class 'net_variable' */

	sprintf(description, "record%d variable net_variable ", record_number);

	buffer_left = description_length - strlen(description) - 1;

	switch( datatype ) {
	case MXFT_STRING:
		strncat( description, "net_string ", buffer_left );
		break;
	case MXFT_CHAR:
		strncat( description, "net_char ", buffer_left );
		break;
	case MXFT_UCHAR:
		strncat( description, "net_uchar ", buffer_left );
		break;
	case MXFT_SHORT:
		strncat( description, "net_short ", buffer_left );
		break;
	case MXFT_USHORT:
		strncat( description, "net_ushort ", buffer_left );
		break;
	case MXFT_INT:
		strncat( description, "net_int ", buffer_left );
		break;
	case MXFT_UINT:
		strncat( description, "net_uint ", buffer_left );
		break;
	case MXFT_LONG:
		strncat( description, "net_long ", buffer_left );
		break;
	case MXFT_ULONG:
		strncat( description, "net_ulong ", buffer_left );
		break;
	case MXFT_FLOAT:
		strncat( description, "net_float ", buffer_left );
		break;
	case MXFT_DOUBLE:
		strncat( description, "net_double ", buffer_left );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Remote record field '%s.%s' is of type %ld "
			"which is not supported by this program.",
			record_name, field_name, datatype );
		break;
	}

	ptr = description + strlen(description);

	sprintf( ptr, "\"\" \"\" %s %s.%s %ld %ld ",
			server_record->name, record_name, field_name,
			num_dimensions, dimension_array[0] );

	if ( datatype == MXFT_STRING ) {
		num_elements = 1;
	} else {
		num_elements = dimension_array[0];
	}

	for ( i = 1; i < num_dimensions; i++ ) {
		ptr = description + strlen(description);

		sprintf( ptr, "%ld ", dimension_array[i] );

		num_elements *= dimension_array[i];
	}
	switch( datatype ) {
	case MXFT_STRING:
		for ( i = 0; i < num_elements; i++ ) {
			buffer_left = description_length
					- strlen(description) - 1;

			strncat( description, "\"\" ", buffer_left );
		}
		break;
	default:
		for ( i = 0; i < num_elements; i++ ) {
			buffer_left = description_length
					- strlen(description) - 1;

			strncat( description, "0 ", buffer_left );
		}
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxupd_add_to_update_list( long update_list_number,
		long num_update_lists,
		MXUPD_UPDATE_LIST *update_list_array,
		char *record_name,
		char *field_name,
		MX_RECORD *local_record )
{
	const char fname[] = "mxupd_add_to_update_list()";

	MXUPD_UPDATE_LIST *update_list;
	MXUPD_UPDATE_LIST_ENTRY *update_list_entry_array;
	MXUPD_UPDATE_LIST_ENTRY *update_list_entry, *ptr;
	MX_RECORD_FIELD *field;
	mx_status_type (*constructor)( void *, char *, size_t,
					MX_RECORD *, MX_RECORD_FIELD * );
	mx_status_type (*parser)( void *, char *,
				MX_RECORD *, MX_RECORD_FIELD *,
				MX_RECORD_FIELD_PARSE_STATUS * );

	unsigned long j, new_num_entries;
	size_t new_entry_array_size;
	mx_status_type status;

	if ( update_list_number >= num_update_lists ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Update list number %ld is outside the allowed range 0 to %ld.",
			update_list_number, num_update_lists-1 );
	}

	update_list = &update_list_array[ update_list_number ];

	MX_DEBUG( 2,("%s: Adding record '%s' to update list %ld.",
		fname, local_record->name, update_list_number));

	j = update_list->num_entries;

	if ( (j + 1) % MXUPD_UPDATE_ARRAY_BLOCK_SIZE == 0 ) {
		/* Need to increase the size of the entry_array. */

		new_num_entries = (j + 1 + MXUPD_UPDATE_ARRAY_BLOCK_SIZE);

		MX_DEBUG(-2,
		("%s: Increasing size of update list %ld to have %ld entries.",
			fname, update_list_number, new_num_entries));

		new_entry_array_size = new_num_entries
					* sizeof( MXUPD_UPDATE_LIST_ENTRY );

		ptr = (MXUPD_UPDATE_LIST_ENTRY *) realloc(
			update_list->entry_array, new_entry_array_size);

		if ( ptr == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory reallocating the entry array "
			"for update list %ld to have %ld elements.",
				update_list_number, new_num_entries );
		}

		update_list->entry_array = ptr;
	}

	update_list_entry_array = update_list->entry_array;

	update_list_entry = &update_list_entry_array[j];

	/* Initialize the items in the update_list_entry structure. */

	mx_strncpy( update_list_entry->record_name,
				record_name, MXU_RECORD_NAME_LENGTH + 1);

	mx_strncpy( update_list_entry->field_name,
				field_name, MXU_FIELD_NAME_LENGTH + 1);

	update_list_entry->local_record = local_record;

	status = mx_find_record_field( local_record, "value", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	update_list_entry->local_value_field = field;

	update_list_entry->local_value_pointer
					= mx_get_field_value_pointer( field );

	status = mx_get_token_constructor( field->datatype, &constructor );

	if ( status.code != MXE_SUCCESS )
		return status;

	update_list_entry->token_constructor = constructor;

	status = mx_get_token_parser( field->datatype, &parser );

	if ( status.code != MXE_SUCCESS )
		return status;

	update_list_entry->token_parser = parser;

	/* We're done so increment the number of entries in the list. */

	update_list->num_entries = j + 1;

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mxupd_construct_update_list( long num_update_lists,
		MXUPD_UPDATE_LIST *update_list_array,
                FILE *update_list_file, char *update_list_filename,
                MX_RECORD *server_record )
{
	const char fname[] = "mxupd_construct_update_list()";

	MX_RECORD *created_record;
	static char description[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	char record_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char record_name[MXU_RECORD_NAME_LENGTH+1];
	char field_name[MXU_FIELD_NAME_LENGTH+1];
	char format[80];
	char buffer[80];
	long datatype, num_dimensions;
	long dimension_array[MXU_FIELD_MAX_DIMENSIONS];
	char *ptr;
	long write_to_savefile, poll_group;
	long i, num_items_read;
	mx_status_type status;

	MX_DEBUG(-2,("%s: update_list_filename = '%s', server_record = '%s'",
			fname, update_list_filename, server_record->name ));

	/* Allocate initial blocks of space to store the update list entries.*/

	for ( i = 0; i < num_update_lists; i++ ) {
		update_list_array[i].num_entries = 0;

		update_list_array[i].entry_array = (MXUPD_UPDATE_LIST_ENTRY *)
			malloc( MXUPD_UPDATE_ARRAY_BLOCK_SIZE
				* sizeof(MXUPD_UPDATE_LIST_ENTRY) );

		if ( update_list_array[i].entry_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate %d update list entries",
				MXUPD_UPDATE_ARRAY_BLOCK_SIZE );
		}
	}

	/* Now read in all of the items in the update list. */

	sprintf( format, "%%%ds %%ld %%ld",
			MXU_RECORD_FIELD_NAME_LENGTH );

	MX_DEBUG( 2,("%s: format = '%s'", fname, format));

	for ( i = 0; ; i++ ) {
		fgets(buffer, sizeof buffer, update_list_file);

		if ( feof(update_list_file) ) {
			break;		/* Exit the for loop. */
		}

		if ( ferror(update_list_file) ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
	"Unexpected error reading line %ld of the update list file '%s'.",
				i, update_list_filename );
		}

		MX_DEBUG( 2,("%s: Line %ld = '%s'",fname,i,buffer));

		num_items_read = sscanf( buffer, format, record_field_name,
					&write_to_savefile, &poll_group );

		if ( num_items_read != 3 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Line %ld of update list file '%s' is unparseable.  Contents = '%s'",
				i, update_list_filename, buffer );
		}

		MX_DEBUG( 2,
	("record_field_name = '%s', write_to_savefile = %ld, poll_group = %ld",
			record_field_name, write_to_savefile, poll_group));

		ptr = strchr( record_field_name, '.' );

		if ( ptr == NULL ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Record field name '%s' from line %ld of update list file '%s' does not "
"have a period character in it.", record_field_name, i, update_list_filename );
		}

		*ptr = '\0';

		mx_strncpy( record_name, record_field_name,
						MXU_RECORD_NAME_LENGTH + 1);

		*ptr = '.';

		ptr++;

		mx_strncpy( field_name, ptr, MXU_FIELD_NAME_LENGTH + 1);

		MX_DEBUG( 2,("Line %ld: record_name = '%s', field_name = '%s'",
			i, record_name, field_name));

		/* Ask the server for information about this record field. */

		status = mx_get_field_type( server_record,
				record_field_name,
				MXU_FIELD_MAX_DIMENSIONS,
				&datatype,
				&num_dimensions,
				dimension_array );

		if ( status.code != MXE_SUCCESS )
			return status;

		MX_DEBUG( 2,
	("datatype = %ld, num_dimensions = %ld, dimension_array[0] = %ld",
			datatype, num_dimensions, dimension_array[0] ));

		/* Create a record in the local database that corresponds
		 * to the remote record field.
		 */

		status = mx_format_variable_description( server_record,
			record_name, field_name, i,
			description, sizeof(description),
			datatype, num_dimensions,
			dimension_array );

		if ( status.code != MXE_SUCCESS )
			return status;

		MX_DEBUG( 2,("description = '%s'", description));

		status = mx_create_record_from_description(
				server_record->list_head,
				description, &created_record, 0 );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_finish_record_initialization( created_record );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_open_hardware( created_record );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* If write_to_savefile is TRUE, add this record to list 0. */

		if ( write_to_savefile ) {
			status = mxupd_add_to_update_list( 0,
				num_update_lists, update_list_array,
				record_name, field_name, created_record );

			if ( status.code != MXE_SUCCESS )
				return status;
		}

		/* If poll_group is non-zero, add this record to the 
		 * specified poll group.
		 */

		if ( poll_group > 0 ) {
			if ( poll_group >= num_update_lists ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Poll group number %ld is outside the allowed range of 1 to %ld.",
					poll_group, num_update_lists - 1 );
			}

			status = mxupd_add_to_update_list( poll_group,
				num_update_lists, update_list_array,
				record_name, field_name, created_record );

			if ( status.code != MXE_SUCCESS )
				return status;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mxupd_poll_update_list( MXUPD_UPDATE_LIST *update_list )
{
	MXUPD_UPDATE_LIST_ENTRY *entry_array, *entry;
	long i;
	mx_status_type status;

	entry_array = update_list->entry_array;

	for ( i = 0; i < update_list->num_entries; i++ ) {

		entry = &entry_array[i];

		status = mx_receive_variable( entry->local_record );

		if ( status.code != MXE_SUCCESS )
			return status;
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mxupd_save_fields_to_autosave_file( char *autosave_filename,
			MXUPD_UPDATE_LIST *update_list )
{
	const char fname[] = "mxupd_save_fields_to_autosave_file()";

	MXUPD_UPDATE_LIST_ENTRY *update_list_entry;
	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	FILE *autosave_file;
	char buffer[500];
	int saved_errno, result;
	long i;
	mx_status_type status;

	MX_DEBUG(1,("%s: saving fields to autosave file '%s'",
		fname, autosave_filename));

	status = mxupd_poll_update_list( update_list );

	if ( status.code != MXE_SUCCESS )
		return status;

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
		"Couldn't open the autosave file '%s' for writing.  "
		"Errno = %d, error string = '%s'",
			autosave_filename, saved_errno,
			strerror(saved_errno));
	}

	/* Write out the new contents of the autosave file. */

	for ( i = 0; i < update_list->num_entries; i++ ) {
		update_list_entry = &(update_list->entry_array)[i];

		sprintf( buffer, "%s.%s", update_list_entry->record_name,
				update_list_entry->field_name );

		MX_DEBUG( 2,("%s: Saving value of '%s'", fname, buffer ));

		fprintf( autosave_file, "%-s  ", buffer );

		buffer[0] = '\0';

		value_field = update_list_entry->local_value_field;

		value_ptr = update_list_entry->local_value_pointer;

		if ( (value_field->num_dimensions == 0)
		  || ((value_field->datatype == MXFT_STRING)
		    && (value_field->num_dimensions == 1))) {

			/* Single token */

			status = ( update_list_entry->token_constructor ) (
					value_ptr,
					buffer,
					sizeof(buffer),
					update_list_entry->local_record,
					value_field );
		} else {
			status = mx_create_array_description(
					value_ptr,
					value_field->num_dimensions - 1,
					buffer,
					sizeof(buffer),
					update_list_entry->local_record,
					value_field,
					update_list_entry->token_constructor );
		}
		if ( status.code != MXE_SUCCESS )
			return status;

		MX_DEBUG( 2,("%s: buffer = '%s'", fname, buffer));

		fprintf( autosave_file, "%s\n", buffer );
	}

	/* Write out a marker to indicate that this is the end of the file. */

	fprintf( autosave_file, "********\n" );
	fclose( autosave_file );

	return status;
}

static mx_status_type
mxupd_choose_autosave_file_to_use(
		char *autosave1_filename,
		char *autosave2_filename,
		FILE **autosave_to_use,
		char **filename_to_use )
{
	const char fname[] = "mxupd_choose_autosave_file_to_use()";

	FILE *autosave1, *autosave2, *older_file, *newer_file;
	char *older_filename, *newer_filename;
	struct stat stat_struct1, stat_struct2;
	char buffer[500];
	int saved_errno, savefile_is_complete;

	*autosave_to_use = NULL;
	*filename_to_use = NULL;

	/* Do the autosave files exist and are they readable? */

	autosave1 = fopen( autosave1_filename, "r" );

	if ( autosave1 == NULL ) {
		saved_errno = errno;
		mx_info(
"Warning: Couldn't open autosave file '%s' for reading.  Reason = '%s'",
			autosave1_filename, strerror(saved_errno) );
	}

	autosave2 = fopen( autosave2_filename, "r" );

	if ( autosave2 == NULL ) {
		saved_errno = errno;
		mx_info( 
"Warning: Couldn't open autosave file '%s' for reading.  Reason = '%s'",
			autosave2_filename, strerror(saved_errno) );
	}

	if ( autosave1 == NULL && autosave2 == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	if ( autosave1 == NULL && autosave2 != NULL ) {
		*autosave_to_use = autosave2;
		*filename_to_use = autosave2_filename;
	} else
	if ( autosave1 != NULL && autosave2 == NULL ) {
		*autosave_to_use = autosave1;
		*filename_to_use = autosave1_filename;
	} else {
		*autosave_to_use = NULL;         /* Both files exist. */
		*filename_to_use = NULL;
	}

	savefile_is_complete = FALSE;

	if ( *autosave_to_use == NULL ) {

		/* If both autosave files exist, check to see which file
		 * is newer.
		 */

		if ( fstat( fileno(autosave1), &stat_struct1 ) != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Couldn't fstat() autosave file 1.  Reason = '%s'",
				strerror( saved_errno ) );
		}
		if ( fstat( fileno(autosave2), &stat_struct2 ) != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Couldn't fstat() autosave file 2.  Reason = '%s'",
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

		fgets(buffer, sizeof buffer, newer_file);

		while ( ! feof( newer_file ) && ! ferror( newer_file ) ) {

			if ( buffer[0] == '*' ) {
				savefile_is_complete = TRUE;
				break;     /* Exit the while() loop. */
			}
			fgets(buffer, sizeof buffer, newer_file);
		}

		if ( savefile_is_complete ) {
			*autosave_to_use = newer_file;
			*filename_to_use = newer_filename;

			fclose( older_file );

			rewind( *autosave_to_use );
		} else {
			*autosave_to_use = older_file;
			*filename_to_use = older_filename;

			fclose( newer_file );
		}
	}

	if ( savefile_is_complete == FALSE ) {

		/* Is the file '*autosave_to_use' a complete savefile?
		 * Look for a line in the file starting with an asterisk.
		 */

		fgets(buffer, sizeof buffer, *autosave_to_use);

		while (!feof(*autosave_to_use) && !ferror(*autosave_to_use)){

			if ( buffer[0] == '*' ) {
				savefile_is_complete = TRUE;
				break;     /* Exit the while() loop. */
			}
			fgets(buffer, sizeof buffer, *autosave_to_use);
		}

		if ( savefile_is_complete ) {
			rewind( *autosave_to_use );
		} else {
			fclose( *autosave_to_use );

			*autosave_to_use = NULL;
			*filename_to_use = NULL;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

#define SEPARATORS " \t"

mx_status_type
mxupd_restore_fields_from_autosave_files(
			char *autosave1_filename, char * autosave2_filename,
			MXUPD_UPDATE_LIST *update_list )
{
	const char fname[] = "mxupd_restore_fields_from_autosave_files()";

	MXUPD_UPDATE_LIST_ENTRY *update_list_entry;
	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	MX_RECORD_FIELD_PARSE_STATUS parse_status;
	FILE *autosave_to_use;
	char *filename_to_use;
	char token_buffer[500];
	char buffer[500];
	char *buffer_ptr;
	char autosave_record_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char update_list_record_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];
	char separators[] = MX_RECORD_FIELD_SEPARATORS;
	long i;
	size_t length;
	mx_status_type status;

	status = mxupd_choose_autosave_file_to_use(
				autosave1_filename, autosave2_filename,
				&autosave_to_use, &filename_to_use );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( autosave_to_use == NULL ) {
		mx_info(
"Warning: No complete autosave file was found.  No parameters will be restored."
			);

		return MX_SUCCESSFUL_RESULT;
	}

	mx_info("Restoring parameters from autosave file '%s'",
			filename_to_use);

	/* Step through the lines in the autosave file and restore
	 * parameters as we go.  Do _not_ return with an error status
	 * code from here on.  To do so would cause the update process
	 * to exit.
	 */

	for ( i = 0; i < update_list->num_entries; i++ ) {
		update_list_entry = &(update_list->entry_array)[i];

		fgets( buffer, sizeof buffer, autosave_to_use );

		if ( feof( autosave_to_use ) || ferror( autosave_to_use )
			  || (buffer[0] == '*') ) {

			fclose( autosave_to_use );

			(void) mx_error( MXE_FILE_IO_ERROR, fname,
	"Only %ld autosave entries were read from autosave file '%s'.  "
	"%ld entries were expected.",
			i, filename_to_use, update_list->num_entries );

			return MX_SUCCESSFUL_RESULT;
		}

		/* Zap any trailing newline. */

		length = strlen( buffer );

		if ( buffer[ length - 1 ] == '\n' ) {
			buffer[ length - 1 ] = '\0';
		}

		buffer_ptr = buffer + strspn( buffer, SEPARATORS );

		sscanf( buffer_ptr, "%s", autosave_record_field_name );

		buffer_ptr += strlen( autosave_record_field_name );

		buffer_ptr += strspn( buffer_ptr, SEPARATORS );

		sprintf( update_list_record_field_name, "%s.%s",
			update_list_entry->record_name,
			update_list_entry->field_name );

		if ( strcmp( autosave_record_field_name,
				update_list_record_field_name ) != 0 ) {

			(void) mx_error( MXE_FILE_IO_ERROR, fname,
	"Autosave file '%s' and the autosave update list are out of "
	"synchronization at line %ld.  Autosave record field name = '%s', "
	"Update list record field name = '%s'", filename_to_use, i, 
				autosave_record_field_name,
				update_list_record_field_name );

			return MX_SUCCESSFUL_RESULT;
		}

		
		value_field = update_list_entry->local_value_field;

		value_ptr = update_list_entry->local_value_pointer;

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

			status = mx_get_next_record_token( &parse_status,
					token_buffer, sizeof(token_buffer) );

			if ( status.code != MXE_SUCCESS )
				return MX_SUCCESSFUL_RESULT;

			status = (update_list_entry->token_parser) (
					value_ptr,
					token_buffer,
					update_list_entry->local_record,
					value_field,
					&parse_status );
		} else {
			/* Array of tokens. */

			status = mx_parse_array_description(
					value_ptr,
					value_field->num_dimensions - 1,
					update_list_entry->local_record,
					value_field,
					&parse_status,
					update_list_entry->token_parser );
		}
		if ( status.code != MXE_SUCCESS )
			return MX_SUCCESSFUL_RESULT;

		mx_info("Restoring value of '%s'.",autosave_record_field_name);

		(void) mx_send_variable( update_list_entry->local_record );
	}
	return MX_SUCCESSFUL_RESULT;
}
