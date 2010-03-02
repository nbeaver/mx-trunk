/*
 * Name:    mtest.c
 *
 * Purpose: For MX motor test routines.  Not for normal use.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"
#include "command.h"

int
motor_test_fn( int argc, char *argv[] )
{
	if ( argc >= 3 ) {
		if ( strcmp( argv[2], "stack" ) == 0 ) {
			mx_stack_traceback();
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "zero" ) == 0 ) {
			int i = 42;
			int j = 0;
			int k;

			k = i / j;
			
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "openfds" ) == 0 ) {
			int max_fds, num_open_fds;

			max_fds = mx_get_max_file_descriptors();

			num_open_fds = mx_get_number_of_open_file_descriptors();

			mx_info( "openfds: max_fds = %d, num_open_fds = %d",
				max_fds, num_open_fds );

			return SUCCESS;
		}
	}

	mx_info( "Nothing happens." );

	return FAILURE;
}

