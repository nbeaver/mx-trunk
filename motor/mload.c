/*
 * Name:    mload.c
 *
 * Purpose: Motor load motor and scan files.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "motor.h"

int
motor_load_fn( int argc, char *argv[] )
{
	const char cname[] = "load";

	MX_RECORD *old_record, *new_record;
	static char record_description[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	int i, buffer_full;
	unsigned long flags;
	long length, string_length, buffer_left;
	mx_status_type mx_status;

	static char usage[] =
			"Usage:  load scans 'scan_savefile'\n"
			"  or    load record 'record_description'\n";

	if ( argc == 2 ) {
		fprintf(output,"%s\n",usage);
		return FAILURE;
	}

	length = (long) strlen( argv[2] );

	if ( length <= 0 )
		length = 1;

	/* LOAD SCANS function. */

	if ( strncmp( argv[2], "scans", length ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf(output, "%s\n", usage);
			return FAILURE;
		}

		flags =
		    MXFC_ALLOW_SCAN_REPLACEMENT | MXFC_DELETE_BROKEN_RECORDS;

		mx_status = mx_read_database_file(
				motor_record_list, argv[3], flags );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	/* LOAD RECORD function. */

	} else if ( strncmp( argv[2], "record", length ) == 0 ) {

		if ( argc < 7 ) {
			fprintf(output,
		"%s: A record description must have at least 4 fields.  "
		"This one has only %d.\n", cname, argc - 3);
			return FAILURE;
		}
		if ( strcmp( argv[4], "scan" ) != 0 ) {
			fprintf(output,
	"%s: Only the loading of scan records is currently supported.\n",
				cname);
			return FAILURE;
		}

		/* Does a record by this name already exist? */

		old_record = mx_get_record( motor_record_list, argv[3] );

		if ( old_record != (MX_RECORD *) NULL ) {
			if ( old_record->mx_superclass == MXR_SCAN ) {
				mx_status = mx_delete_record( old_record );

				if ( mx_status.code != MXE_SUCCESS ) {
					fprintf( output,
		"%s: Attempt to delete old scan record '%s' failed.\n",
						cname, argv[3] );
					return FAILURE;
				}
			} else {
				fprintf( output,
			"%s: A non-scan record named '%s' already exists.\n",
					cname, argv[3] );
				return FAILURE;
			}
		}

		/* Create a record description string from the argv array. */

		strcpy( record_description, "" );

		buffer_full = FALSE;

		for ( i = 3; i < argc; i++ ) {
			string_length = (long) strlen( record_description );
			buffer_left = (long) sizeof( record_description ) 
						- string_length - 1;

			if ( buffer_left <= 0 ) {
				buffer_full = TRUE;
				break;
			}

			strncat( record_description, argv[i], buffer_left );

			string_length = (long) strlen( record_description );
			buffer_left = (long) sizeof( record_description ) 
						- string_length - 1;

			if ( buffer_left <= 0 ) {
				buffer_full = TRUE;
				break;
			}

			strncat( record_description, " ", buffer_left );
		}

		if ( buffer_full ) {
			fprintf( output,
"%s: The generated record description is longer than the internal buffer "
"'record_description' in function motor_load_fn() which is of length %lu.  "
"Increase the size of this buffer in the source code and recompile.\n",
		cname, (unsigned long) (sizeof(record_description) - 1) );
			return FAILURE;
		}

		/* Create the new scan record. */

		mx_status = mx_create_record_from_description(
		    motor_record_list, record_description, &new_record, 0 );

		if ( mx_status.code != MXE_SUCCESS ) {
			fprintf( output,
			"%s: Attempt to create new scan record '%s' failed.\n",
				cname, argv[3] );
			return FAILURE;
		}

		mx_status = mx_finish_record_initialization( new_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			fprintf( output,
"%s: Attempt to finish record initialization for new record '%s' failed.\n",
				cname, argv[3] );

			(void) mx_delete_record( new_record );

			return FAILURE;
		}

	} else {
		fprintf(output,"%s: unrecognized argument '%s'\n\n",
						cname, argv[2]);
		fprintf(output,"%s\n", usage);
		return FAILURE;
	}

	return SUCCESS;
}

