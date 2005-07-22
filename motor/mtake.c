/*
 * Name:    mtake.c
 *
 * Purpose: Take motor commands from an external process.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"
#include "command.h"

int
motor_take_fn( int argc, char *argv[] )
{
	fprintf(output, "The '&' or 'take' command is no longer supported.\n");
	return FAILURE;
}

