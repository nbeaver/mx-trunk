#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_array.h"
#include "mx_net.h"
#include "mx_callback.h"

extern char *optarg;
extern int optind;

int
main( int argc, char *argv[] )
{
	static const char fname[] = "vcc_monitor";

	char description[ MXU_RECORD_DESCRIPTION_LENGTH+1 ];
	char *description_ptr;
	char display_buffer[200];

	char server_name[ MXU_HOSTNAME_LENGTH+1 ];
	char server_arguments[ MXU_SERVER_ARGUMENTS_LENGTH+1 ];
	char record_name[ MXU_RECORD_NAME_LENGTH+1 ];
	char field_name[ MXU_FIELD_NAME_LENGTH+1 ];
	char record_field_name[ MXU_RECORD_FIELD_NAME_LENGTH+1 ];
	MX_CALLBACK *callback;
	MX_RECORD *record_list;
	MX_RECORD *server_record;
	char server_record_name[] = "mxserver";
	MX_NETWORK_FIELD nf;
	long i, datatype, num_dimensions;
	long dimension_array[8];
	size_t *sizeof_array;
	int c, server_port, num_items;
	mx_bool_type network_debug, start_debugger;
	unsigned long server_flags;
	MX_RECORD_FIELD temp_field;
	void *value_ptr;
	mx_status_type mx_status;

	if ( argc < 2 ) {
		fprintf( stderr, "\nUsage: %s network_field_name\n\n", argv[0]);
		exit(1);
	}

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

	if ( network_debug ) {
		server_flags = 0x60000000;
	} else {
		server_flags = 0x20000000;
	}

	/* Create the MX runtime database. */

	snprintf( description, sizeof(description),
		"%s server network tcpip_server \"\" \"\" %#lx %s %d",
		server_record_name, server_flags, server_name, server_port );

	description_ptr = description;

	mx_status = mx_setup_database_from_array(
				&record_list, 1, &description_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	mx_status = mx_set_program_name( record_list, "vcc_monitor" );

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

	/* Find out the dimensions of the record field. */

	/* FIXME! - This should be done using a network field handle. */

	mx_status = mx_get_field_type( server_record, record_field_name,
					1L, &datatype, &num_dimensions,
					dimension_array );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	MX_DEBUG(-2,("%s: datatype = %s, num_dimensions = %ld", fname,
		mx_get_field_type_string(datatype), num_dimensions));

	if ( mx_get_debug_level() >= 2 ) {
		fprintf( stderr, "%s: ", fname );
		for ( i = 0; i < num_dimensions; i++ ) {
			fprintf( stderr, "%ld ", dimension_array[i] );
		}
		fprintf( stderr, "\n\n" );
	}

	/* Allocate local memory for the network field value. */

	mx_status = mx_get_datatype_sizeof_array( datatype, &sizeof_array );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	if ( num_dimensions == 0 ) {
		value_ptr = malloc( sizeof_array[0] );
	} else {
		value_ptr = mx_allocate_array( num_dimensions,
					dimension_array, sizeof_array );
	}

	if ( value_ptr == NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Unable to allocate a local %ld-dimensional "
			"array for record field '%s'.",
				num_dimensions, record_field_name );

		exit( MXE_OUT_OF_MEMORY );
	}

	/* Create a local 'temporary' record field to describe the
	 * network field value.  We must use &value_ptr for any value
	 * pointer given to mx_initialize_temp_record_field(), since
	 * the temporary field created has the MXFF_VARARGS bit set.
	 */

	mx_status = mx_initialize_temp_record_field( &temp_field,
							datatype,
							num_dimensions,
							dimension_array,
							sizeof_array,
							&value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Get the starting value of the network field. */

	mx_status = mx_get_array( &nf, datatype,
			num_dimensions, dimension_array, value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Display the starting value of the network field. */

	mx_status = mx_create_description_from_field( NULL, &temp_field,
				display_buffer, sizeof(display_buffer) );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	MX_DEBUG(-2,("%s: Starting value = '%s'", fname, display_buffer));

	/* Create a callback handler for this network field. */

	mx_status = mx_network_add_callback( &nf,
					MXCB_VALUE_CHANGED, NULL, NULL,
					&callback );

	exit(0);
}

