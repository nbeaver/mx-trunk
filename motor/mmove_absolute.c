/*
 * Name:    mmove_absolute.c
 *
 * Purpose: Motor move absolute function.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
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
motor_mabs_fn( int argc, char *argv[] )
{
	const char cname[] = "mabs";

	MX_RECORD *record;
	int i, steps;
	double position;
	char buffer[100];
	size_t length;
	int status;
	mx_status_type mx_status;

	if ( argc <= 2 ) {
		fprintf(output,
"Usage:  mabs 'motorname' 'position'    - Move 'motorname' to the absolute\n"
"                                         position 'position' expressed\n"
"                                         in engineering units.\n"
"\n"
"        mabs 'motorname' steps 'steps' - Move 'motorname' to the absolute\n"
"                                         position 'steps' expressed\n"
"                                         in low level units.\n"
		);
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf(output,"%s: invalid motor name '%s'\n", cname,argv[2]);
		return FAILURE;
	}

	/* Is this a motor? */

	if ( record->mx_class != MXC_MOTOR ) {
		fprintf(output,"%s: '%s' is not a motor.\n", cname, argv[2]);
		return FAILURE;
	}

	if ( argc == 3 ) {
		fprintf(output, 
		"%s: must specify a move distance for motor %s.\n",
			cname, argv[2]);

		return FAILURE;
	} else {
		length = strlen( argv[3] );

		if ( length == 0 )
			length = 1;

		if ( strncmp( argv[3], "steps", length ) == 0 ) {
			if ( argc == 4 ) {
				fprintf(output,
				"%s: must specify step count.\n", cname);
				return FAILURE;
			}

			i = sscanf(argv[4], "%d", &steps);

			if ( i == 0 ) {
				fprintf(output,
					"%s: invalid value '%s'\n",
					cname, argv[4]);
				return FAILURE;
			}

			mx_status = mx_motor_move_absolute_steps_with_report(
					record, steps,
					motor_move_report_function,
					MXF_MTR_SHOW_MOVE );
		} else {
			i = sscanf(argv[3], "%lg", &position);

			if ( i == 0 ) {
				fprintf(output,
					"%s: invalid value '%s'\n",
					cname, argv[3]);
				return FAILURE;
			}

			mx_status = mx_motor_move_absolute_with_report(
					record, position,
					motor_move_report_function,
					MXF_MTR_SHOW_MOVE );
		}

		if ( (mx_status.code == MXE_SUCCESS)
			  && allow_motor_database_updates )
		{
			sprintf(buffer,
				"save motor %s", motor_savefile);
			cmd_execute_command_line( command_list_length,
				command_list, buffer );
		}
	}

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		status = SUCCESS;
		break;

	case MXE_INTERRUPTED:
		status = INTERRUPTED;
		break;

	default:
		status = FAILURE;
		break;
	}

	return status;
}

