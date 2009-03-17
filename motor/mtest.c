/*
 * Name:    mtest.c
 *
 * Purpose: For MX motor test routines.  Not for normal use.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
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
	mx_stack_traceback();

	return FAILURE;
}

