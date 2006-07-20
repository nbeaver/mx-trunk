/*
 * Name:    example3.c
 *
 * Purpose: This example is intended as guidance on how to use the MX driver
 *          library from another control system such as EPICS.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_unistd.h"

#include "mx_record.h"
#include "mx_motor.h"

#define DATABASE_FILENAME	"example.dat"

int
main( int argc, char **argv )
{
	MX_RECORD *record_list;
	MX_RECORD *motor_record;
	char *motor_name;
	double destination, position;
	unsigned long status;
	int busy;

	mx_status_type mx_status;

	if ( argc != 3 ) {
		fprintf(stderr,"Usage: example3 motor_name destination\n");
		exit(-1);
	}

	motor_name = argv[1];

	destination = atof( argv[2] );

	mx_status = mx_setup_database( &record_list, DATABASE_FILENAME );

	if ( mx_status.code != MXE_SUCCESS )
		exit(-1);

	motor_record = mx_get_record( record_list, motor_name );

	if ( motor_record == NULL ) {
		fprintf(stderr,"Motor '%s' does not exist.\n",
				motor_name );
		exit(-1);
	}

	mx_status = mx_motor_internal_move_absolute(motor_record, destination);

	if ( mx_status.code != MXE_SUCCESS )
		exit(-1);

	busy = TRUE;

	while ( busy ) {
		mx_status = mx_motor_get_status( motor_record, &status);

		if ( mx_status.code != MXE_SUCCESS )
			exit(-1);

		if ( status & MXSF_MTR_IS_BUSY ) {
			busy = TRUE;
		} else {
			busy = FALSE;
		}

		mx_status = mx_motor_get_position( motor_record, &position );

		if ( mx_status.code != MXE_SUCCESS )
			exit(-1);

		fprintf( stderr, "%g\n", position );

		sleep(1);
	}

	fprintf(stderr, "move complete.\n");

	exit(0);
}
