/*
 * Name:    mvxworks.c
 *
 * Purpose: VxWorks startup routines for "motor".
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005-2006, 2013, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#if defined( OS_VXWORKS )

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"

#define NUM_ARGUMENTS	6
#define MAX_ARGV_LENGTH	80

extern void mxmotor_shell( char *device_database_file,
			char *scan_database_file );

void mxmotor_shell( char *device_database_file,
		char *scan_database_file )
{
	int argc;
	char **argv;

	char argv0[MAX_ARGV_LENGTH+1] = "mxmotor_shell";
	char argv1[MAX_ARGV_LENGTH+1] = "-F";
	char argv2[MAX_ARGV_LENGTH+1] = "mxmotor.dat";
	char argv3[MAX_ARGV_LENGTH+1] = "-s";
	char argv4[MAX_ARGV_LENGTH+1] = "mxscan.dat";
	char argv5[MAX_ARGV_LENGTH+1] = "-N";

	fprintf( stderr, "mxmotor_shell( '%s', '%s' ) starting...\n",
		device_database_file, scan_database_file );

	argc = NUM_ARGUMENTS;

	argv = (char **)
		malloc( NUM_ARGUMENTS * sizeof(char *[MAX_ARGV_LENGTH+1]) );

	if ( argv == NULL ) {
		fprintf(stderr,
		"motor: Cannot allocate memory for argv array.\n");

		return;
	}

	argv[0] = argv0;
	argv[1] = argv1;
	argv[2] = argv2;
	argv[3] = argv3;
	argv[4] = argv4;
	argv[5] = argv5;

	if ( strlen( device_database_file ) > 0 ) {
		strlcpy( argv[2], device_database_file, MAX_ARGV_LENGTH );
	}

	if ( strlen( scan_database_file ) > 0 ) {
		strlcpy( argv[4], scan_database_file, MAX_ARGV_LENGTH );
	}

	fprintf( stderr, "Calling motor_main( %d, %s, %s, %s, %s, %s, %s )\n",
		argc, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5] );

	motor_main( argc, argv );

	fprintf( stderr, "motor_shell() is exiting...\n" );
}

#endif /* OS_VXWORKS */

