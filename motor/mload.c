/*
 * Name:    mload.c
 *
 * Purpose: Motor load motor and scan files.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MOTOR_DEBUG_LOAD_SCAN	FALSE

#include <stdio.h>
#include <string.h>

#include "motor.h"

int
motor_load_fn( int argc, char *argv[] )
{
	static const char cname[] = "load";

	MX_RECORD *old_record, *new_record;
	MX_RECORD *list_head_record, *last_old_record, *current_record;
	static char record_description[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	char buffer[MXU_FILENAME_LENGTH + 12];
	int i;
	unsigned long flags;
	long length;
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

		/* Running mx_finish_database_initialization() multiple
		 * times on the same records is not permitted, so we must
		 * execute mx_finish_record_initialization() on just the
		 * records that we are loading now.  We work around this
		 * by using a feature of the implementation of the MX
		 * record list.
		 *
		 * The MX database is a circular doubly-linked list of
		 * MX_RECORD objects, with a list head record named
		 * 'mx_database'.  Since records are added to the database
		 * in the order that they were found in the database file,
		 * the "first" record can be found via the pointer
		 * 'list_head_record->next_record', while the "last" record
		 * can be found via 'list_head_record->previous_record'.
		 *
		 * The new scan records will be added after the "last"
		 * record, so we need to get a pointer to that record now,
		 * so that we can walk the new part of the record list
		 * resulting from the upcoming call to the function
		 * mx_read_database_file().
		 *
		 * Note: There is an abundance of code that depends on
		 * the MX database being a circular doubly-linked list,
		 * so it is exceedingly unlikely that this implementation
		 * detail will ever change.  (WML)
		 */

		list_head_record = motor_record_list;

		/* Make sure this is _really_ the list head record. */

		list_head_record = list_head_record->list_head;

		last_old_record = list_head_record->previous_record;

#if MOTOR_DEBUG_LOAD_SCAN
		MX_DEBUG(-2,("%s: last_old_record = '%s'",
			cname, last_old_record->name));
#endif
		/* Now we can load the new scans. */

		flags =
		    MXFC_ALLOW_SCAN_REPLACEMENT | MXFC_DELETE_BROKEN_RECORDS;

		mx_status = mx_read_database_file(
				motor_record_list, argv[3], flags );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		/* Walk through the newly loaded scan records and run
		 * mx_finish_record_initialization() on them.
		 */

		current_record = last_old_record->next_record;

		while ( current_record != list_head_record ) {

#if MOTOR_DEBUG_LOAD_SCAN
			MX_DEBUG(-2,("%s: current_record = '%s'",
				cname, current_record->name ));
#endif
			mx_status = mx_finish_record_initialization(
						current_record );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_open_hardware( current_record );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_finish_delayed_initialization(
						current_record );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			current_record = current_record->next_record;
		}

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

		strlcpy( record_description, "", sizeof(record_description) );

		for ( i = 3; i < argc; i++ ) {
			strlcat( record_description, argv[i],
						sizeof(record_description) );

			strlcat( record_description, " ",
						sizeof(record_description) );
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

		mx_status = mx_open_hardware( new_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			fprintf( output,
"%s: Attempt to open hardware for new record '%s' failed.\n",
				cname, argv[3] );

			(void) mx_delete_record( new_record );

			return FAILURE;
		}

		mx_status = mx_finish_delayed_initialization( new_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			fprintf( output,
"%s: Attempt to finish delayed initialization for new record '%s' failed.\n",
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

	if ( motor_autosave_on ) {
		snprintf( buffer, sizeof(buffer),
			"save scan \"%s\"", scan_savefile );

		cmd_execute_command_line( command_list_length,
						command_list, buffer );

		motor_has_unsaved_scans = FALSE;
	} else {
		motor_has_unsaved_scans = TRUE;
	}

	return SUCCESS;
}

