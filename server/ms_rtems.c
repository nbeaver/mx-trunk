/*
 * Name:    ms_rtems.c
 *
 * Purpose: RTEMS startup routines for "mxserver".
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

#if defined( OS_RTEMS )

#include "os_rtems.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "ms_mxserver.h"

#define NUM_ARGUMENTS	5
#define MAX_ARGV_LENGTH	80

rtems_task
Init( rtems_task_argument ignored )
{
	int argc;
	char **argv;

	char argv0[MAX_ARGV_LENGTH+1] = "mxserver.exe";
	char argv1[MAX_ARGV_LENGTH+1] = "-f";
	char argv2[MAX_ARGV_LENGTH+1] = "NotInitialized";
	char argv3[MAX_ARGV_LENGTH+1] = "-C";
	char argv4[MAX_ARGV_LENGTH+1] = "NotInitialized";

	argc = NUM_ARGUMENTS;

	mx_msleep(1);

	fprintf(stderr, "\n*** mxserver.exe is starting ***\n");

	rtems_bsdnet_initialize_network();

	rtems_bsdnet_initialize_tftp_filesystem();

	argv = (char **)
		malloc( NUM_ARGUMENTS * sizeof(char *[MAX_ARGV_LENGTH+1]) );

	if ( argv == NULL ) {
		fprintf(stderr,
		"mxserver: Cannot allocate memory for argv array.\n" );

		return;
	}

	argv[0] = argv0;
	argv[1] = argv1;
	argv[2] = argv2;
	argv[3] = argv3;
	argv[4] = argv4;

	sprintf( argv2, "/TFTP/%s/mxserver.dat", MX_RTEMS_SERVER_ADDRESS );

	sprintf( argv4, "/TFTP/%s/mxserver.acl", MX_RTEMS_SERVER_ADDRESS );

	mxserver_main( argc, argv );

	fprintf(stderr,
	"*** mxserver_main() returned even though it should not. ***\n");

	exit( 0 );
}

#endif /* OS_RTEMS */

