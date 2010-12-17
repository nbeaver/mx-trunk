/*
 * Name:    mmodify.c
 *
 * Purpose: Motor scan modify functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "motor.h"
#include "mdialog.h"

int
motor_modify_fn( int argc, char *argv[] )
{
	static const char cname[] = "modify";

	MX_RECORD *record;
	MX_SCAN *scan;
	char *scan_name;
	static char scan_record_description[MXU_RECORD_DESCRIPTION_LENGTH+1];
	char buffer[MXU_FILENAME_LENGTH + 12];
	size_t length;
	int status;
	static char usage[] = "Usage:  modify scan 'scan_name'\n";


	if ( argc <= 2 ) {
		fprintf(output,"%s\n",usage);
		return FAILURE;
	}

	length = strlen( argv[2] );

	if ( length == 0 )
		length = 1;

	if ( strncmp( argv[2], "scan", length ) == 0 )
	{
		if ( argc == 3 ) {
			fprintf(output,
				"%s: No scan name specified.\n\n", cname);
			fprintf(output,"%s\n",usage);
			return FAILURE;
		} else if ( argc >= 5 ) {
			fprintf(output,
			"%s: Too many arguments on command line.\n\n", cname);
			fprintf(output,"%s\n",usage);
			return FAILURE;
		}

		scan_name = &(argv[3][0]);

		/* Does a record with this name already exist? */

		record = mx_get_record( motor_record_list, scan_name );

		if ( record == (MX_RECORD *) NULL ) {
			fprintf( output, "%s: Record '%s' does not exist.\n",
				cname, scan_name );
			return FAILURE;
		}

		/* If the old record is not a scan record, print
		 * an error message and abort.
		 */

		if ( record->mx_superclass != MXR_SCAN ) {
			fprintf( output,
			"%s: Record '%s' is not a scan record.\n",
				cname, record->name );
			return FAILURE;
		}

		/* Get a pointer to the MX_SCAN structure. */

		scan = (MX_SCAN *)(record->record_superclass_struct);

		if ( scan == (MX_SCAN *) NULL ) {
			fprintf( output,
"%s: MX_SCAN pointer for record '%s' is NULL.  This shouldn't happen.\n",
				cname, scan_name );
			return FAILURE;
		}

		/* Abort if the scan uses missing devices. */

		if ( scan->missing_record_array != NULL ) {
			int i;

			fprintf( output,
	"%s: Scan '%s' cannot be modified since it uses missing devices \"",
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

		/* Now call a class-dependent function to prompt for the
		 * scan parameters.
		 */

		fprintf(output,
"\n"
"Press ctrl-D to discard the scan modifications and revert to the old scan.\n"
"\n");

		scan_record_description[0] = '\0';

		switch( record->mx_class ) {
		case MXS_LINEAR_SCAN:
			status = motor_setup_linear_scan_parameters(
				scan_name, scan,
				scan_record_description,
				MXU_RECORD_DESCRIPTION_LENGTH,
				NULL, NULL, NULL );
			break;

		case MXS_LIST_SCAN:
			status = motor_setup_list_scan_parameters(
				scan_name, scan,
				scan_record_description,
				MXU_RECORD_DESCRIPTION_LENGTH,
				NULL, NULL, NULL );
			break;
		case MXS_XAFS_SCAN:
			status = motor_setup_xafs_scan_parameters(
				scan_name, scan,
				scan_record_description,
				MXU_RECORD_DESCRIPTION_LENGTH,
				NULL, NULL, NULL );
			break;
		case MXS_QUICK_SCAN:
			status = motor_setup_quick_scan_parameters(
				scan_name, scan,
				scan_record_description,
				MXU_RECORD_DESCRIPTION_LENGTH,
				NULL, NULL, NULL );
			break;
		case MXS_AREA_DETECTOR_SCAN:
			status = motor_setup_area_detector_scan_parameters(
				scan_name, scan,
				scan_record_description,
				MXU_RECORD_DESCRIPTION_LENGTH,
				NULL, NULL, NULL );
			break;
		default:
#if 0
			fprintf(output, "%s: unrecognized scan class %ld.\n",
				cname, record->mx_class);
#else
			fprintf(output, "\n");
#endif
			return FAILURE;
		}

		if ( status == FAILURE ) {
			fprintf( output, "Scan modify aborted.\n");
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
	} else {
		fprintf(output,
			"%s: unrecognized argument '%s'\n\n",cname,argv[2]);
		fprintf(output,"%s\n", usage);
		return FAILURE;
	}
	return status;
}

