/*
 * Name:    mscan.c
 *
 * Purpose: Scan routine for the simple motor scan program.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004-2006, 2010, 2015-2016, 2018, 2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_TIMING	FALSE

#include <stdio.h>

#include "motor.h"
#include "mdialog.h"
#include "mx_hrt_debug.h"

int
motor_scan_fn( int argc, char *argv[] )
{
	static const char cname[] = "scan";

	MX_RECORD *record;
	MX_SCAN *scan;
	char old_filename[ MXU_FILENAME_LENGTH + 1 ];
	char buffer[ 2*MXU_FILENAME_LENGTH + 20 ];
	double estimated_scan_duration;
	int status;
	mx_status_type mx_status;

#if DEBUG_TIMING
	MX_HRT_TIMING scan_measurement;
	MX_HRT_TIMING update_measurement;
#endif

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

	strlcpy( old_filename, scan->datafile.filename, MXU_FILENAME_LENGTH );

	if ( motor_estimate_on ) {
		/* Show the estimated scan duration */

		mx_status = mx_scan_get_estimated_scan_duration( record,
						&estimated_scan_duration );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		if ( estimated_scan_duration < 1.0e-9 ) {
			fprintf( output,
		"*** WARNING ***: The estimated scan duration was reported "
			"to be %f seconds, which is probably incorrect.\n\n",
						estimated_scan_duration );
		} else {
			fprintf( output,
			"Estimated scan duration = %f seconds.\n\n",
						estimated_scan_duration );
		}
	}

	/* Invoke the scan function. */

	fprintf( output, "*** Scan '%s' starting. ***\n", record->name );
	fprintf( output,
	"Press 'p' to pause the scan or any other key to abort the scan.\n" );

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s: Invoking mx_perform_scan()", cname));
	MX_HRT_START( scan_measurement );
#endif

	mx_status = mx_perform_scan( record );

#if DEBUG_TIMING
	MX_HRT_END( scan_measurement );
	MX_HRT_RESULTS( scan_measurement, cname, "End of mx_perform_scan()" );

	MX_DEBUG(-2,("%s: Updating scan database.", cname));
	MX_HRT_START( update_measurement );
#endif

	/* Check to see if the datafile name for the scan has changed.
	 * If it has, then we will need to update the scan database
	 * at some point.
	 */

	if ( strcmp( old_filename, scan->datafile.filename ) != 0 ) {
		MX_DEBUG( 2,
		("scan: The datafile name has changed from '%s' to '%s'.",
		 	old_filename, scan->datafile.filename ));

		if ( motor_autosave_on ) {
			snprintf( buffer, sizeof(buffer),
				"save scan \"%s\"", scan_savefile );

			cmd_execute_command_line( command_list_length,
							command_list, buffer );

			motor_has_unsaved_scans = FALSE;
		} else {
			motor_has_unsaved_scans = TRUE;
		}
	}

#if DEBUG_TIMING
	MX_HRT_END( update_measurement );
	MX_HRT_RESULTS( update_measurement, cname,
		"Finished updating scan database." );
#endif

	if ( mx_status.code != MXE_SUCCESS ) {
		return FAILURE;
	} else {
		return SUCCESS;
	}
}

mx_status_type
motor_scan_pause_request_handler( MX_SCAN *scan )
{
	static const char fname[] = "motor_scan_pause_request_handler()";

	char buffer[80];
	int status, string_length;

	fprintf( output, "\n*** The scan '%s' is paused. ***\n\n",
			scan->record->name );

	for (;;) {
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

#if ( defined(OS_HPUX) && !defined(__ia64) )
	return MX_SUCCESSFUL_RESULT;
#endif
}

