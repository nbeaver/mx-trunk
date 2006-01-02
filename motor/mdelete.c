/*
 * Name:    mdelete.c
 *
 * Purpose: Delete scan function.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "motor.h"

int
motor_delete_fn( int argc, char *argv[] )
{
	const char cname[] = "delete";

	MX_RECORD *record;
	char buffer[MXU_FILENAME_LENGTH + 12];
	size_t length;
	mx_status_type mx_status;

	static char usage[] = "Usage:  delete scan 'scan_name'\n";

	if ( argc == 2 ) {
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
		}

		/* Get the record for this scan. */

		record = mx_get_record( motor_record_list, argv[3] );

		if ( record == (MX_RECORD *) NULL ) {
			fprintf(output,
			"%s: There is no scan named '%s'\n", cname, argv[3]);
			return FAILURE;
		}

		if ( record->mx_superclass != MXR_SCAN ) {
			fprintf(output,
			"%s: The record '%s' is not a scan record.\n",
			cname, argv[3]);

			return FAILURE;
		}

		/* Delete the scan record. */

		mx_status = mx_delete_record( record );

		if ( mx_status.code != MXE_SUCCESS ) {
			fprintf(output,
			"%s: The scan record '%s' is not deleted.\n",
			cname, argv[3]);

			return FAILURE;
		}
	} else {
		fprintf(output,
			"%s: unrecognized argument '%s'\n\n",cname,argv[2]);
		fprintf(output,"%s\n", usage);
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

