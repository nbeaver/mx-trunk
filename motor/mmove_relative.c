/*
 * Name:    mmove_relative.c
 *
 * Purpose: Motor move relative function.
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

#include "motor.h"

int
motor_mrel_fn( int argc, char *argv[] )
{
	static const char cname[] = "mrel";

	MX_RECORD *record;
	int i, relative_steps;
	double relative_position;
	char buffer[100];
	size_t length;
	int status;
	mx_status_type mx_status;

	if ( argc == 2 ) {
		fprintf(output,
"Usage:  mrel 'motorname' 'distance'    - Move 'motorname' by the relative\n"
"                                         amount 'distance' expressed in\n"
"                                         engineering units.\n"
"\n"
"        mrel 'motorname' steps 'steps' - Move 'motorname' by the relative\n"
"                                         amount 'steps' expressed in\n"
"                                         low level units.\n"
		);
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf(output,"%s: invalid motor name '%s'\n",cname,argv[2]);
		return FAILURE;
	}

	/* Is this a motor? */

	if ( record->mx_class != MXC_MOTOR ) {
		fprintf(output,"%s: '%s' is not a motor.\n",cname,argv[2]);
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

		if ( strncmp(argv[3], "steps", length) == 0 ) {
			if ( argc == 4 ) {
				fprintf(output,
				"%s: must specify step count.\n", cname);
				return FAILURE;
			}

			i = sscanf(argv[4], "%d", &relative_steps);

			if ( i == 0 ) {
				fprintf(output,
				"%s: invalid value '%s'\n", cname, argv[4]);
				return FAILURE;
			}

			mx_status = mx_motor_move_relative_steps_with_report(
				record, relative_steps,
				motor_move_report_function, MXF_MTR_SHOW_MOVE);
		} else {
			i = sscanf(argv[3], "%lg", &relative_position);

			if ( i == 0 ) {
				fprintf(output,
				"%s: invalid value '%s'\n", cname, argv[3]);
				return FAILURE;
			}

			mx_status = mx_motor_move_relative_with_report(
				record, relative_position,
				motor_move_report_function, MXF_MTR_SHOW_MOVE);
		}

		if ( (mx_status.code == MXE_SUCCESS)
			  && allow_motor_database_updates )
		{
			snprintf( buffer, sizeof(buffer),
				"save motor %s", motor_savefile );
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

