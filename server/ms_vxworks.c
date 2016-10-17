/*
 * Name:    ms_vxworks.c
 *
 * Purpose: VxWorks startup routines for "mxserver".
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#if defined( OS_VXWORKS )

#include <stdio.h>
#include <stdlib.h>

#include <taskLib.h>
#include <ioLib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "ms_mxserver.h"

#define NUM_ARGUMENTS	7
#define MAX_ARGV_LENGTH	80

extern void mxserver( int port_number,
		char *device_database_file,
		char *access_control_list_file );

extern void mxserver_task( int port_number,
		char *device_database_file,
		char *access_control_list_file );

void mxserver_task( int port_number,
		char *device_database_file,
		char *access_control_list_file )
{
	int argc;
	char **argv;
	int vfd_stdin, vfd_stdout, vfd_stderr;

	/* You cannot count on stdin, stdout, and stderr automatically
	 * being available for a task created by taskSpawn(), so we have
	 * to manually set them up.
	 */

	vfd_stderr = open("/vio/0",2,0);
	ioGlobalStdSet(2,vfd_stderr);

	vfd_stdout = open("/vio/0",2,0);
	ioGlobalStdSet(1,vfd_stdout);

	vfd_stdin = open("/vio/0",2,0);
	ioGlobalStdSet(0,vfd_stdin);

	/* Set up argc and argv for mxserver_main(). */

	char argv0[MAX_ARGV_LENGTH+1] = "mxserver";
	char argv1[MAX_ARGV_LENGTH+1] = "-p";
	char argv2[MAX_ARGV_LENGTH+1] = "9727";
	char argv3[MAX_ARGV_LENGTH+1] = "-f";
	char argv4[MAX_ARGV_LENGTH+1] = "mxserver.dat";
	char argv5[MAX_ARGV_LENGTH+1] = "-C";
	char argv6[MAX_ARGV_LENGTH+1] = "mxserver.acl";

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
	argv[5] = argv5;
	argv[6] = argv6;

	if ( port_number > 0 ) {
		snprintf( argv[2], MAX_ARGV_LENGTH, "%d", port_number );
	}

	if ( strlen( device_database_file ) > 0 ) {
		strlcpy( argv[4], device_database_file, MAX_ARGV_LENGTH );
	}

	if ( strlen( access_control_list_file ) > 0 ) {
		strlcpy( argv[6], access_control_list_file, MAX_ARGV_LENGTH);
	}

	mxserver_main( argc, argv );

	fprintf( stderr, "mxserver_task is exiting...\n" );
}

/* mxserver() is the command executed at the VxWorks shell prompt.  Its job
 * is to start mxserver_task() with the VX_FP_TASK option set.
 */

void mxserver( int port_number,
		char *device_database_file,
		char *access_control_list_file )
{
	int task_id, priority, stack_size;

	priority = 0;
	stack_size = 65536;

	task_id = taskSpawn( "mxserver_task",
				priority,
				VX_FP_TASK,
				stack_size,
				(FUNCPTR) mxserver_task,
				port_number,
				(int) device_database_file,
				(int) access_control_list_file,
                                0, 0, 0, 0, 0, 0, 0 );

#if 1
	fprintf( stderr, "mxserver_task() started.  task_id = %#lx\n",
		(unsigned long) task_id );
#endif

	return;
}

#endif /* OS_VXWORKS */

