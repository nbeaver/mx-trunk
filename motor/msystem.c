/*
 * Name:    msystem.c
 *
 * Purpose: Motor command to run an external program.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* This routine is just a fancy wrapper around the system() function call. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "motor.h"

int
motor_system_fn( int argc, char *argv[] )
{
	static char system_buffer[255];
	size_t buffer_left;
	int i, status;

	if ( argc <= 2 ) {
		fprintf(output, "Usage: system 'external command line'\n");
		return FAILURE;
	}

	strcpy( system_buffer, "" );

	for ( i = 2; i < argc; i++ ) {
		buffer_left
		    = sizeof(system_buffer) - strlen(system_buffer) - 1;

		strncat( system_buffer, argv[i], buffer_left );

		buffer_left
		    = sizeof(system_buffer) - strlen(system_buffer) - 1;

		strncat( system_buffer, " ", buffer_left );
	}

	status = system( system_buffer );

	if ( status != 0 ) {
		fprintf(output,
		"system: Command '%s' failed with error code = %d\n",
			system_buffer, status);

		return FAILURE;
	}

	return SUCCESS;
}

