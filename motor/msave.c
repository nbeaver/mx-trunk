/*
 * Name:    msave.c
 *
 * Purpose: Motor save motor/save file function.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "motor.h"

int
motor_save_fn( int argc, char *argv[] )
{
	static const char cname[] = "save";

	static const char usage[] = "Usage:  save motor 'motor_file'\n"
				    " or     save scan  'scan_file'";

	MX_RECORD *record;
	int superclass_is_a_match;
	long i, num_record_superclasses;
	long *record_superclass_list;
	size_t length;
	mx_status_type mx_status;

	if ( argc == 2 ) {
		fprintf(output,"%s\n",usage);
		return FAILURE;
	}

	if ( argc == 3 ) {
		fprintf(output,"%s: No data file specified.\n\n", cname);
		fprintf(output,"%s\n",usage);
		return FAILURE;
	}

	num_record_superclasses = 0;
	record_superclass_list = NULL;

	length = strlen( argv[2] );

	if ( length == 0 )
		length = 1;

	/* SAVE MOTOR function. */

	if ( strncmp( argv[2], "motors", length ) == 0 ) {

		/* Save interface, device, variable, and server records. */

		num_record_superclasses = 4;

		record_superclass_list = (long *)
			malloc( num_record_superclasses * sizeof(long) );

		record_superclass_list[0] = MXR_INTERFACE;
		record_superclass_list[1] = MXR_DEVICE;
		record_superclass_list[2] = MXR_VARIABLE;
		record_superclass_list[3] = MXR_SERVER;

	} else		 /* SAVE SCAN function. */

	if ( strncmp( argv[2], "scans", length ) == 0 ) {

		/* Only save scan records. */

		num_record_superclasses = 1;

		record_superclass_list = (long *)
			malloc( num_record_superclasses * sizeof(long) );

		record_superclass_list[0] = MXR_SCAN;

		fprintf( output, "Saving scan database to '%s'.\n", argv[3] );
	}

	if ( record_superclass_list == (long *) NULL ) {
		fprintf( output,
"%s: Ran out of memory trying to allocate record_superclass_list.\n", cname );
		return FAILURE;
	}

	/* Read hardware values into the software records.
	 *
	 * Skip over the first record since it is just the record list head.
	 */

	record = motor_record_list->next_record;

	/* The list is actually circular, so we step through the list 
	 * until we get to the beginning again.
	 */

	while ( record != motor_record_list ) {

		/* Check to see if this is the type of record we want. */

		superclass_is_a_match = FALSE;

		for ( i = 0; i < num_record_superclasses; i++ ) {
			if (record->mx_superclass == record_superclass_list[i]){

				superclass_is_a_match = TRUE;

				break;      /* Exit the for() loop. */
			}
		}

		if ( superclass_is_a_match == TRUE ) {

			mx_status = mx_read_parms_from_hardware( record );

			if ( mx_status.code != MXE_SUCCESS ) {
				fprintf( output,
		"%s: Can't read hardware parameters for device '%s'\n",
					cname, record->name );

				free( record_superclass_list );
				return FAILURE;
			}
		}

		record = record->next_record;
	}

	mx_status = mx_write_database_file( motor_record_list, argv[3],
			num_record_superclasses, record_superclass_list );

	free( record_superclass_list );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
		"%s: Can't save database file '%s'\n", cname, argv[3] );

		return FAILURE;
	}

	return SUCCESS;
}

