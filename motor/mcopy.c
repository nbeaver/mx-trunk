/*
 * Name:    mcopy.c
 *
 * Purpose: Motor scan copy function.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "motor.h"

static int
motor_copy_scan_record( MX_RECORD *old_scan_record,
		MX_RECORD **new_scan_record, char *new_scan_name );


int
motor_copy_fn( int argc, char *argv[] )
{
	const char cname[] = "copy";

	MX_RECORD *old_scan_record, *new_scan_record;
	MX_SCAN *old_scan;
	char *old_scan_name, *new_scan_name;
	char buffer[MXU_FILENAME_LENGTH + 12];
	size_t length;
	int status;
	mx_status_type mx_status;

	static char usage[]
		= "Usage:  copy scan 'scan_name' 'new_scan_name'\n";

	if ( argc <= 2 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	length = strlen( argv[2] );

	if ( length == 0 )
		length = 1;

	if ( strncmp( argv[2], "scan", length ) == 0 )
	{
		switch( argc ) {
		case 3:
			fprintf( output,
			    "%s: No source or destination scan name given\n",
				cname );
			fprintf( output, "%s\n", usage );
			return FAILURE;
		case 4:
			fprintf( output,"%s: No destination scan name given\n",
				cname );
			fprintf( output, "%s\n", usage );
			return FAILURE;
		case 5:
			old_scan_name = argv[3];
			new_scan_name = argv[4];
			break;
		default:
			fprintf( output,
			"%s: Too many arguments on command line.\n", cname );
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		/* Does an old scan record with this name exist? */

		old_scan_record = mx_get_record(
					motor_record_list, old_scan_name );

		if ( old_scan_record == (MX_RECORD *) NULL ) {
			fprintf( output,
			"%s: Old scan record '%s' does not exist.\n",
				cname, old_scan_name );
			return FAILURE;
		}

		if ( old_scan_record->mx_superclass != MXR_SCAN ) {
			fprintf( output,
			"%s: The old record '%s' is not a scan record.\n",
				cname, old_scan_name );
			return FAILURE;
		}

		old_scan = (MX_SCAN *)
				(old_scan_record->record_superclass_struct);

		if ( old_scan == (MX_SCAN *) NULL ) {
			fprintf( output,
				"MX_SCAN structure for scan '%s' is NULL.\n",
				old_scan_name );
			return FAILURE;
		}

		/* Abort if the scan uses missing devices. */

		if ( old_scan->missing_record_array != NULL ) {
			int i;

			fprintf( output,
	"%s: Scan '%s' cannot be copied since it uses missing devices \"",
				cname, old_scan_record->name );

			for ( i = 0; i < old_scan->num_missing_records; i++ ) {
				if ( i > 0 ) {
					fprintf( output, "," );
				}
				fprintf( output, "%s",
				    old_scan->missing_record_array[i]->name );
			}
			fprintf( output, "\"\n" );

			return FAILURE;
		}

		status = motor_copy_scan_record( old_scan_record,
					&new_scan_record, new_scan_name );

		if ( status != SUCCESS )
			return status;

		mx_status = mx_finish_record_initialization( new_scan_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else {
		fprintf( output,
		"%s: unrecognized argument '%s'\n\n", cname, argv[2]);
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	if ( motor_autosave_on ) {
		sprintf(buffer, "save scan \"%s\"", scan_savefile);

		cmd_execute_command_line( command_list_length,
						command_list, buffer );

		motor_has_unsaved_scans = FALSE;
	} else {
		motor_has_unsaved_scans = TRUE;
	}

	return SUCCESS;
}

static int
motor_copy_scan_record( MX_RECORD *old_scan_record,
			MX_RECORD **new_scan_record, char *new_scan_name )
{
	const char fname[] = "motor_copy_scan_record()";

	MX_RECORD *record;
	static char old_scan_description[MXU_RECORD_DESCRIPTION_LENGTH+1];
	static char new_scan_description[MXU_RECORD_DESCRIPTION_LENGTH+1];
	int i, new_scan_name_length;
	mx_status_type mx_status;

	*new_scan_record = NULL;

	/* Does a record with the new scan's name already exist? */

	record = mx_get_record( motor_record_list, new_scan_name );

	if ( record != (MX_RECORD *) NULL ) {
		/* If the record that previously had the same
		 * name as 'new_scan_name', is a scan record,
		 * then delete it.  Otherwise, give an error
		 * message.
		 */

		if ( record->mx_superclass != MXR_SCAN ) {
			fprintf( output,
	"%s: The record '%s' already exists and is not a scan record.\n",
					fname, new_scan_name );
			fprintf( output,
"Cannot overwrite a previously existing record unless it is a scan record.\n");
			return FAILURE;
		}

		mx_status = mx_delete_record( record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	}

	/* Get the record description for this old record. */

	mx_status = mx_create_description_from_record(
		old_scan_record,
		old_scan_description,
		sizeof(old_scan_description)-1 );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Create the new scan description from the old one. */

	strcpy( new_scan_description, old_scan_description );

	new_scan_name_length = strlen( new_scan_name );

	if ( new_scan_name_length > MXU_RECORD_NAME_LENGTH ) {
		new_scan_name_length = MXU_RECORD_NAME_LENGTH;
	}

	for ( i = 0; i < new_scan_name_length; i++ ) {
		new_scan_description[i] = new_scan_name[i];
	}
	for (i = new_scan_name_length; i<MXU_RECORD_NAME_LENGTH; i++) {
		new_scan_description[i] = ' ';
	}

	/* Create a scan record from the newly formatted scan 
	 * description string.
	 */

	mx_status = mx_create_record_from_description(
		motor_record_list,
		new_scan_description,
		new_scan_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

