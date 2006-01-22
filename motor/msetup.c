/*
 * Name:    msetup.c
 *
 * Purpose: Motor scan setup functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005-2006 Illinois Institute of Technology
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
motor_setup_fn( int argc, char *argv[] )
{
	static const char cname[] = "setup";

	MX_RECORD *record;
	char buffer[MXU_FILENAME_LENGTH + 12];
	char *scan_name;
	static char scan_record_description[MXU_RECORD_DESCRIPTION_LENGTH+1];
	long scan_class;
	size_t length;
	int status;
	mx_status_type mx_status;
	static char usage[] = "Usage:  setup scan 'scan_name'\n";


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

		if ( record != NULL ) {
			/* If the old record is a scan record, delete it. */

			if ( record->mx_superclass == MXR_SCAN ) {

				mx_status = mx_delete_record( record );

				if ( mx_status.code != MXE_SUCCESS ) {
					fprintf(output,
		"An error occurred while trying to delete the old scan '%s'\n",
						scan_name);
					return FAILURE;
				}

			/* If the old record is not a scan record, print
			 * an error message and abort.
			 */

			} else {
				switch( record->mx_superclass ) {
				case MXR_LIST_HEAD:
					fprintf(output, "A record list head ");
					break;

				case MXR_INTERFACE:
					fprintf(output, "An interface ");
					break;

				case MXR_DEVICE:
					fprintf(output, "A device ");
					break;

				default:
					fprintf(output,
				"A record of unknown record superclass %ld ",
						record->mx_superclass);
					break;
				}

				fprintf(output,
				"with the name '%s' already exists.\n",
					scan_name);
				fprintf(output,
					"Try another name for the scan.\n");

				return FAILURE;
			}
		}

		/* What kind of scan are we going to set up? */

		fprintf(output,
"\n"
"Press ctrl-D to abort scan setup and throw away the scan.\n"
"\n"
"Enter scan class:\n"
"    1.  Linear scan - scaler scan, motor scan, etc.\n"
"    2.  List scan - read list of motor positions\n"
"    3.  XAFS scan\n"
"    4.  Quick scan\n"
"\n" );

		status = motor_get_long( output,
				"--> ", TRUE, 1, &scan_class, 1, 4 );

		scan_class += (MXS_LINEAR_SCAN - 1);

		/* Now call a class-dependent function to prompt for the
		 * scan parameters.
		 */

		scan_record_description[0] = '\0';

		switch( scan_class ) {
		case MXS_LINEAR_SCAN:
			status = motor_setup_linear_scan_parameters( 
				scan_name, NULL,
				scan_record_description,
				MXU_RECORD_DESCRIPTION_LENGTH,
				NULL, NULL, NULL );
			break;

		case MXS_LIST_SCAN:
			status = motor_setup_list_scan_parameters(
				scan_name, NULL,
				scan_record_description,
				MXU_RECORD_DESCRIPTION_LENGTH,
				NULL, NULL, NULL );
			break;
		case MXS_XAFS_SCAN:
			status = motor_setup_xafs_scan_parameters(
				scan_name, NULL,
				scan_record_description,
				MXU_RECORD_DESCRIPTION_LENGTH,
				NULL, NULL, NULL );
			break;
		case MXS_QUICK_SCAN:
			status = motor_setup_quick_scan_parameters(
				scan_name, NULL,
				scan_record_description,
				MXU_RECORD_DESCRIPTION_LENGTH,
				NULL, NULL, NULL );
			break;
		default:
			fprintf(output, "%s: unrecognized scan class %ld.\n",
				cname, scan_class);
			return FAILURE;
			break;
		}

		if ( status == FAILURE ) {
			fprintf( output, "Scan setup aborted.\n");
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

