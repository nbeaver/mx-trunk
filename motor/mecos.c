/*
 * Name:    mecos.c
 *
 * Purpose: eCos startup routines for "motor".
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#if defined( OS_ECOS )

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"

#define NUM_ARGUMENTS	7
#define MAX_ARGV_LENGTH	80

int
main( int ecos_argc, char *ecos_argv[] )
{
	int argc;
	char **argv;

	char argv0[MAX_ARGV_LENGTH+1] = "motor.exe";
	char argv1[MAX_ARGV_LENGTH+1] = "-F";
	char argv2[MAX_ARGV_LENGTH+1] = "NotInitialized";
	char argv3[MAX_ARGV_LENGTH+1] = "-s";
	char argv4[MAX_ARGV_LENGTH+1] = "NotInitialized";
	char argv5[MAX_ARGV_LENGTH+1] = "-N";
	char argv6[MAX_ARGV_LENGTH+1] = "-t";

	argc = NUM_ARGUMENTS;

	mx_msleep(5);

	fprintf(stderr,"\n*** MX motor.exe is starting ***\n");

	argv = (char **)
		malloc( NUM_ARGUMENTS * sizeof(char *[MAX_ARGV_LENGTH+1]) );

	if ( argv == NULL ) {
		fprintf(stderr,
		"motor: Cannot allocate memory for argv array.\n");

		exit(0);
	}

	argv[0] = argv0;
	argv[1] = argv1;
	argv[2] = argv2;
	argv[3] = argv3;
	argv[4] = argv4;
	argv[5] = argv5;
	argv[6] = argv6;

	snprintf( argv2, sizeof(argv2),
			"TFTP/xxx/motor.dat" );

	snprintf( argv4, sizeof(argv4),
			"/TFTP/xxx/scan.dat" );

	motor_main( argc, argv );

	fprintf(stderr,
	"*** motor_main() returned even though it should not. ***\n");

	exit(0);
}

#endif /* OS_ECOS */

