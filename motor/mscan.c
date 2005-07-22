/*
 * Name:    mscan.c
 *
 * Purpose: Scan routine for the simple motor scan program.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"
#include "mdialog.h"

int
motor_scan_fn( int argc, char *argv[] )
{
	static const char cname[] = "scan";

	MX_RECORD *record;
	MX_SCAN *scan;
	char old_filename[ MXU_FILENAME_LENGTH + 1 ];
	char buffer[ MXU_FILENAME_LENGTH + 12 ];
	int status;
	mx_status_type mx_status;

	if ( argc != 3 ) {
		fprintf(output, "Usage: scan 'scan_name'.\n");
		return FAILURE;
	}

	/* Get the record for this scan name. */

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf( output,
			"No scan record named '%s' was found.\n", argv[2] );
		return FAILURE;
	}

	/* Is this a scan record? */

	if ( record->mx_superclass != MXR_SCAN ) {
		fprintf( output,
			"'%s' is not a scan record.  Record class = %ld\n",
			argv[2], record->mx_superclass );
		return FAILURE;
	}

	scan = (MX_SCAN *) (record->record_superclass_struct);

	if ( scan == (MX_SCAN *) NULL ) {
		fprintf( output,
			"MX_SCAN structure for scan '%s' is NULL.\n",
			argv[2] );
		return FAILURE;
	}

	/* Abort if the scan uses missing devices. */

	if ( scan->missing_record_array != NULL ) {
		int i;

		fprintf( output,
	"%s: Scan '%s' cannot be executed since it uses missing devices \"",
				cname, record->name );

		for ( i = 0; i < scan->num_missing_records; i++ ) {
			if ( i > 0 ) {
				fprintf( output, "," );
			}
			fprintf( output, "%s",
				scan->missing_record_array[i]->name );
		}
		fprintf( output, "\"\n" );

		return FAILURE;
	}

	/* Does the data file specified for this scan already exist? */

	status = motor_check_for_datafile_name_collision( scan );

	if ( status != SUCCESS )
		return status;

	/* Prompt for changes to the scan header. */

	status = motor_prompt_for_scan_header( scan );

	if ( status != SUCCESS )
		return status;

	/* Save a copy of the current datafile name. */

	mx_strncpy( old_filename,
		scan->datafile.filename, MXU_FILENAME_LENGTH + 1 );

	/* Invoke the scan function. */

	fprintf( output, "*** Scan '%s' starting. ***\n", record->name );
	fprintf( output,
	"Press 'p' to pause the scan or any other key to abort the scan.\n" );

	mx_status = mx_perform_scan( record );

	/* Check to see if the datafile name for the scan has changed.
	 * If it has, then we will need to update the scan database
	 * at some point.
	 */

	if ( strcmp( old_filename, scan->datafile.filename ) != 0 ) {
		MX_DEBUG( 2,
		("scan: The datafile name has changed from '%s' to '%s'.",
		 	old_filename, scan->datafile.filename ));

		if ( motor_autosave_on ) {
			sprintf( buffer, "save scan %s", scan_savefile );

			cmd_execute_command_line( command_list_length,
							command_list, buffer );

			motor_has_unsaved_scans = FALSE;
		} else {
			motor_has_unsaved_scans = TRUE;
		}
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		return FAILURE;
	} else {
		return SUCCESS;
	}
}

mx_status_type
motor_scan_pause_request_handler( MX_SCAN *scan )
{
	const char fname[] = "motor_scan_pause_request_handler()";

	char buffer[80];
	int status, string_length;
	char command;

	fprintf( output, "\n*** The scan '%s' is paused. ***\n\n",
			scan->record->name );

	while (1) {
		fprintf( output,
		"Your choices are: (C)ontinue scan\n"
		"                  (S)top motors at end of current step\n"
		"                  (I)mmediate abort\n" );

		string_length = sizeof(buffer);

		status = motor_get_string( output, "---> ", "C",
					&string_length, buffer );

		if ( status != SUCCESS ) {
			fprintf( output,
		"Unexpected error reading command string.  Trying again.\n" );

			continue;
		}

		command = '\0';

		switch( buffer[0] ) {
		case 'c':
		case 'C':
			return MX_SUCCESSFUL_RESULT;
			break;
		case 's':
		case 'S':
			return mx_error( MXE_STOP_REQUESTED, fname,
		"Scan motors will be stopped at the end of the current step.");
			break;
		case 'i':
		case 'I':
			return mx_error( MXE_INTERRUPTED, fname,
						"Scan aborted." );
		default:
			fprintf( output,
				"Command '%s' not understood.  Try again.\n",
				buffer );
			break;
		}
	}
}

