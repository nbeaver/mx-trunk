/*
 * Name:    mxput.c
 *
 * Purpose: Mxget reads the value of a network field from an MX server.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2013-2017, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_multi.h"
#include "mx_net.h"

static void
print_usage( void )
{
	fprintf( stderr,
"\n"
"Usage: mxput <options> record_name.field <value> ...\n"
"       mxput <options> hostname:record_name.field <value> ...\n"
"       mxput <options> hostname@port:record_name.field <value> ...\n"
"\n"
"where the options are:\n"
"   -a   Enable network debugging.\n"
"   -D   Start the source code debugger.\n"
"   -e   Enable escape sequences.\n"
"\n"
	);
}

int
main( int argc, char *argv[] )
{
	char server_name[ MXU_HOSTNAME_LENGTH+1 ];
	char server_arguments[ MXU_SERVER_ARGUMENTS_LENGTH+1 ];
	char record_name[ MXU_RECORD_NAME_LENGTH+1 ];
	char field_name[ MXU_FIELD_NAME_LENGTH+1 ];

	MX_RECORD *server_record = NULL;
	int server_port;
	unsigned long server_flags;

	MX_RECORD *mx_record_list = NULL;
	MX_LIST_HEAD *list_head_struct = NULL;
	MX_NETWORK_FIELD *nf = NULL;
	MX_RECORD_FIELD *local_field = NULL;
	void *value_ptr;

	char write_buffer[10000];
	unsigned long i, j, length;
	unsigned char character, next_character;

	int c;
	mx_bool_type start_debugger, use_escape_sequences;
	unsigned long network_debug_flags;
	unsigned long max_network_dump_bytes;
	mx_status_type mx_status;

	/* For MX network fields that use signed integer types, it must
	 * be possible for the user to send a negative value to the field.
	 * However, by default getopt() assumes that _anything_ that starts
	 * with a '-' character is an option unless the special character
	 * sequence '--' has previously appeared in the argv list.  Users
	 * will hate having to prefix negative numbers with '--' arguments,
	 * so we have to insert the '--' for them so that the users do not
	 * have to deal with the issue.
	 *
	 * The way we deal with this is to create a new array of strings
	 * to take the place of the 'argv' array passed to us.  We walk
	 * through the 'argv' array copying strings to the 'fixed_argv'.
	 * If along the way, we find an array element in 'argv' that
	 * consists of '-' followed by a number or '-' followed by a '.'
	 * character (for cases like -.325), we copy to the 'fixed_argv'
	 * array the special string '--'.  We then continue on copying
	 * from 'argv' to 'fixed_argv' using 'i' as the index of 'argv'
	 * and 'i+1' as the index of 'fixed_argv'.
	 *
	 * Comments that this is incompatible with the normal behavior
	 * of getopt() will be ignored.  (W. Lavender, 2014-10-09)
	 */

	int fixed_argc;
	char **fixed_argv;
	mx_bool_type double_hyphen_inserted;

	double_hyphen_inserted = FALSE;

	fixed_argc = argc;

	fixed_argv = calloc( argc+1, sizeof(char *) );

	if ( fixed_argv == (char **) NULL ) {
		fprintf( stderr, "The attempt to allocate the fixed_argv "
			"array with %d elements failed.\n", argc+1 );
		exit(1);
	}

	for ( i = 0; i < argc; i++ ) {

#if 0
		fprintf(stderr, "argv[%lu] = '%s'\n", i, argv[i]);
#endif
		if ( double_hyphen_inserted ) {
			fixed_argv[i+1] = strdup( argv[i] );
		} else {
			if ( argv[i][0] == '-' ) {
				if ( (argv[i][1] == '.')
				  || isdigit( (int) (argv[i][1]) ) )
				{
					fixed_argv[i] = strdup( "--" );

					fixed_argc = argc + 1;

					double_hyphen_inserted = TRUE;

					fixed_argv[i+1] = strdup( argv[i] );
				} else {
					fixed_argv[i] = strdup( argv[i] );
				}
			} else {
				fixed_argv[i] = strdup( argv[i] );
			}
		}
	}

	/* Verify that all of the strdup() calls succeeded. */

	for ( i = 0; i < fixed_argc; i++ ) {
		if ( fixed_argv[i] == (char *) NULL ) {
			fprintf( stderr, "fixed_argv[%lu] is NULL.\n", i );
			exit(1);
		}
#if 0
		fprintf(stderr, "fixed_argv[%lu] = '%s'\n", i, fixed_argv[i] );
#endif
	}

	/* Now that we have fixed the 'argv' array, we can continue
	 * on to the getopt() parsing stage.
	 */

	network_debug_flags = 0;
	max_network_dump_bytes = 0;
	start_debugger = FALSE;
	use_escape_sequences = FALSE;

	if ( argc < 3 ) {
		print_usage();
		exit(1);
	}

	/* See if any command line arguments were specified. */

	while ( (c = getopt(fixed_argc, fixed_argv, "aA:Deq:")) != -1 )
	{
		switch(c) {
		case 'a':
			network_debug_flags = MXF_NETDBG_SUMMARY;
			break;
		case 'A':
			network_debug_flags =
				mx_hex_string_to_unsigned_long( optarg );
			break;
		case 'D':
			start_debugger = TRUE;
			break;
		case 'e':
			use_escape_sequences = TRUE;
			break;
		case 'q':
			max_network_dump_bytes = atol( optarg );
			break;
		default:
			print_usage();
			break;
		}
	}

	if ( start_debugger ) {
		mx_breakpoint();
	}

	/* If we create an MX database right now, then we can turn on
	 * network debugging right away.  The alternative would be to allow
	 * mx_connect_to_mx_server() to automatically create an MX database
	 * for us, but that would cause us to miss the first few network
	 * messages that are sent and received.
	 */

	mx_status = mx_initialize_drivers();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	mx_record_list = mx_initialize_database();

	if ( mx_record_list == NULL ) {
		mx_status = mx_error( MXE_FUNCTION_FAILED, "mxget",
			"Unable to setup an MX record list." );

		return mx_status.code;
	}

	if ( network_debug_flags ) {
		mx_multi_set_debug_flags( mx_record_list, network_debug_flags );
	}

	list_head_struct = mx_get_record_list_head_struct( mx_record_list );

	if ( list_head_struct == (MX_LIST_HEAD *) NULL ) {
		mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, "mxget",
		"The MX_LIST_HEAD pointer is NULL." );

		return mx_status.code;
	}

	list_head_struct->max_network_dump_bytes = max_network_dump_bytes;

	/* Parse fixed_argv[optind] to get the network field arguments. */

	mx_status = mx_parse_network_field_id( fixed_argv[optind],
				server_name, sizeof(server_name) - 1,
				server_arguments, sizeof(server_arguments) - 1,
				record_name, sizeof(record_name) - 1,
				field_name, sizeof(field_name) - 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	server_port = atoi( server_arguments );

	/* Connect to the specified server.
	 *
	 * Assigning mx_record_list to server_record gives the function
	 * mx_connect_to_mx_server() a way to find the MX record list.
	 * After returning, server_record should contain a pointer to
	 * the newly created MX server record.
	 */

	server_record = mx_record_list;

	server_flags = MXF_NETWORK_SERVER_NO_AUTO_RECONNECT
			| MXF_NETWORK_SERVER_BLOCKING_IO;

	mx_status = mx_connect_to_mx_server( &server_record,
				server_name, server_port, 0.1, server_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	mx_status = mx_set_program_name( mx_record_list, "mxput" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	/* Create an MX_NETWORK_FIELD and a local MX_RECORD_FIELD
	 * for the requested network field name.
	 */

	mx_status = mx_create_network_field( &nf, server_record,
					record_name, field_name, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	/* Connect the MX_NETWORK_FIELD to the server. */

	mx_status = mx_network_field_connect( nf );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	/* Get pointers to the local MX_RECORD_FIELD. */

	local_field = nf->local_field;

	value_ptr = mx_get_field_value_pointer( local_field );

	/* Concatenate the strings passed via the command line
	 * into one big string with quotes around each item.
	 *
	 * We skip over fixed_argv[optind] since that contains the
	 * name of the network field we are writing to.  The
	 * values we want to send to that network field start
	 * at fixed_argv[optind+1].

	 */

	strlcpy( write_buffer, "", sizeof(write_buffer) );

	for ( i = optind+1; i < fixed_argc; i++ ) {
		strlcat( write_buffer, "\"", sizeof(write_buffer) );
		strlcat( write_buffer, fixed_argv[i], sizeof(write_buffer) );
		strlcat( write_buffer, "\" ", sizeof(write_buffer) );
	}

	/* If requested process escape sequences in the incoming text. */

	if ( use_escape_sequences ) {
		length = strlen( write_buffer );

		for ( i = 0, j = 0; i < length; i++, j++ ) {
			character = write_buffer[i];

			if ( character == '\\' ) {
				next_character = write_buffer[i+1];

				switch( next_character ) {
				case 'n':
					write_buffer[j] = '\n'; i++;
					break;
				default:
					write_buffer[j] = '\\';
					break;
				}
			} else {
				write_buffer[j] = character;
			}
		}
	}

	mx_status = mx_create_field_from_description( local_field->record,
							local_field,
							NULL, write_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	/* Send the field value to the remote MX server. */

	mx_status = mx_put_field_array( NULL, NULL,
					nf, local_field, value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status.code;

	exit(0);

	MXW_NOT_REACHED( return 0 );
}

