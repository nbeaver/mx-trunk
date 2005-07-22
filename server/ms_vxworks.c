/*
 * Name:    ms_vxworks.c
 *
 * Purpose: VxWorks startup routines for "mxserver".
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#if defined( OS_VXWORKS )

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "ms_mxserver.h"

#define NUM_ARGUMENTS	5
#define MAX_ARGV_LENGTH	80

extern void mxserver( char *device_database_file,
		char *access_control_list_file );

void mxserver( char *device_database_file,
		char *access_control_list_file )
{
	int argc;
	char **argv;

	char argv0[MAX_ARGV_LENGTH+1] = "mxserver";
	char argv1[MAX_ARGV_LENGTH+1] = "-f";
	char argv2[MAX_ARGV_LENGTH+1] = "mxserver.dat";
	char argv3[MAX_ARGV_LENGTH+1] = "-C";
	char argv4[MAX_ARGV_LENGTH+1] = "mxserver.acl";

	argc = NUM_ARGUMENTS;

	argv = ( char ** )
		malloc( NUM_ARGUMENTS * sizeof(char *[MAX_ARGV_LENGTH+1]) );

	if ( argv == NULL ) {
		fprintf(stderr,
		"mxserver: Cannot allocate memory for argv array.\n");

		return;
	}

	argv[0] = argv0;
	argv[1] = argv1;
	argv[2] = argv2;
	argv[3] = argv3;
	argv[4] = argv4;

	if ( strlen( device_database_file ) > 0 ) {
		mx_strncpy( argv[2], device_database_file, MAX_ARGV_LENGTH );
	}

	if ( strlen( access_control_list_file ) > 0 ) {
		mx_strncpy( argv[4], access_control_list_file, MAX_ARGV_LENGTH);
	}

	mxserver_main( argc, argv );

	fprintf( stderr, "mxserver task is exiting...\n" );
}

#endif /* OS_VXWORKS */

