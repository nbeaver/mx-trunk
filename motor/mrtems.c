/*
 * Name:    mrtems.c
 *
 * Purpose: RTEMS startup routines for "motor".
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005-2006, 2010, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#if defined( OS_RTEMS )

#include "os_rtems.h"
#include "motor.h"

#define NUM_ARGUMENTS	7
#define MAX_ARGV_LENGTH	80

rtems_task
Init( rtems_task_argument ignored )
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

	rtems_bsdnet_initialize_network();

#if ( MX_RTEMS_VERSION > 4009099 )
	mount_and_make_target_path( NULL,
				"/TFTP",
				RTEMS_FILESYSTEM_TYPE_TFTPFS,
				RTEMS_FILESYSTEM_READ_WRITE,
				NULL );
#else
	rtems_bsdnet_initialize_tftp_filesystem();
#endif

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
	argv[6] = argv6;

	snprintf( argv2, sizeof(argv2),
			"TFTP/%s/mxmotor.dat", MX_RTEMS_SERVER_ADDRESS );

	snprintf( argv4, sizeof(argv4),
			"/TFTP/%s/mxscan.dat", MX_RTEMS_SERVER_ADDRESS );

	motor_main( argc, argv );

	fprintf(stderr,
	"*** motor_main() returned even though it should not. ***\n");

	exit( 0 );
}

void
motor_rtems_reboot( void )
{

#if defined(MX_RTEMS_BSP_PC386) || defined(MX_RTEMS_BSP_MVME2307)

	fprintf(stderr,
"RTEMS does not have a command line to return to, so we will reboot now.\n");
	fflush(stderr);

	mx_msleep(1000);

	rtemsReboot();

#else

	fprintf(stderr, "RTEMS does not have a command line to return to, "
			"so we will call exit(0) now.\n");

	exit( 0 );
#endif

}

#endif /* OS_RTEMS */

