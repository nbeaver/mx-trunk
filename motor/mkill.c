/*
 * Name:    mkill.c
 *
 * Purpose: Device kill function.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "motor.h"

int
motor_kill_fn( int argc, char *argv[] )
{
	MX_RECORD *record;
	mx_status_type mx_status;

	if ( argc != 3 ) {
		fprintf(output,
"Usage:  kill 'devicename' - kills moving or counting for 'devicename'\n"
		);
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf( output, "Record '%s' does not exist.\n",
			argv[2] );

		return FAILURE;
	}

	/* Find out what kind of device this is. */

	if ( record->mx_superclass == MXR_INTERFACE ) {
		fprintf(output,"Kill is only supported for device records.\n");
		return FAILURE;
	}

	switch( record->mx_class ) {
	case MXC_MOTOR:
		mx_status = mx_motor_immediate_abort( record );
		break;
	default:
		fprintf(output, "Kill is not supported for '%s' records.\n",
				mx_get_driver_name( record ) );

		return FAILURE;
		break;
	}

	if ( mx_status.code == MXE_SUCCESS ) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

