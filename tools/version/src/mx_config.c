/*
 * Name:    mx-config.c
 *
 * Purpose: This program can be used to get information about the
 *          installed version of MX.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2011, 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_osdef.h"
#include "mx_private_version.h"

#if defined(OS_UNIX)
#  include <sys/utsname.h>
#endif

void
print_usage( void )
{
	fprintf( stderr,
	"Usage: mx_config [option]\n"
	"\n"
	"  The available options (including all build targets) are:\n"
	"\n"
	"  gcc\n"
	"  glibc\n"
	"  gnuc\n"
	"  library\n"
	"  major\n"
	"  minor\n"
	"  musl\n"
	"  os\n"
	"  update\n"
	"  version\n"
	"\n"
	);

	return;
}

int
main( int argc, char **argv )
{
	if ( argc < 2 ) {
		print_usage();
		exit(1);
	}

	if ( strcmp( argv[1], "version" ) == 0 ) {
		printf( "%ld\n", MX_VERSION );
		exit(0);
	}
	if ( strcmp( argv[1], "major" ) == 0 ) {
		printf( "%d\n", MX_MAJOR_VERSION );
		exit(0);
	}
	if ( strcmp( argv[1], "minor" ) == 0 ) {
		printf( "%d\n", MX_MINOR_VERSION );
		exit(0);
	}
	if ( strcmp( argv[1], "update" ) == 0 ) {
		printf( "%d\n", MX_UPDATE_VERSION );
		exit(0);
	}

#if defined(MX_GNUC_VERSION)
	if ( strcmp( argv[1], "gnuc" ) == 0 ) {
		printf( "%ld\n", MX_GNUC_VERSION );
		exit(0);
	}
	if ( strcmp( argv[1], "gcc" ) == 0 ) {
		printf( "%ld\n", MX_GNUC_VERSION );
		exit(0);
	}
#endif

#if defined(MX_GLIBC_VERSION)
	if ( strcmp( argv[1], "glibc" ) == 0 ) {
		printf( "%ld\n", MX_GLIBC_VERSION );
		exit(0);
	}
#endif

#if defined(MX_MUSL_VERSION)
	if ( strcmp( argv[1], "musl" ) == 0 ) {
		printf( "%ld\n", MX_MUSL_VERSION );
		exit(0);
	}
#endif

	if ( strcmp( argv[1], "library" ) == 0 ) {

#if defined(MX_GLIBC_VERSION)
		printf( "glibc\n" );
#elif defined(MX_MUSL_VERSION)
		printf( "musl\n" );
#else
		printf( "unknown\n" );
#endif
		exit(0);
	}

/*------------------------------------------------------------------------*/

	if ( strcmp( argv[1], "os" ) == 0 ) {

#if defined(MX_CYGWIN_VERSION)
		printf( "%ld\n", MX_CYGWIN_VERSION );
#elif defined(MX_DARWIN_VERSION)
		printf( "%d\n", MX_DARWIN_VERSION );
#elif defined(MX_RTEMS_VERSION)
		printf( "%d\n", MX_RTEMS_VERSION );
#elif defined(MX_SOLARIS_VERSION)
		printf( "%d\n", MX_SOLARIS_VERSION );
#elif defined(MX_UNIXWARE_VERSION)
		printf( "%d\n", MX_UNIXWARE_VERSION );
#elif defined(OS_UNIX)
		{
			struct utsname uname_struct;
			int status, num_items, saved_errno;
			long os_version, os_major, os_minor, os_update;

			status = uname( &uname_struct );

			if ( status < 0 ) {
				saved_errno = errno;

				fprintf( stderr,
		"Error: uname() failed with errno = %d, error message = '%s'\n",
					saved_errno, strerror( saved_errno ) );
				exit(1);
			}

			num_items = sscanf( uname_struct.release, "%lu.%lu.%lu",
					&os_major, &os_minor, &os_update );

			if ( num_items == 0 ) {
				os_version = -1;
			} else {
				os_version = os_major * 1000000L;

				if ( num_items >= 2 ) {
					os_version += os_minor * 1000L;

					if ( num_items >= 3 ) {
						os_version += os_update;
					}
				}
			}

			printf( "%ld\n", os_version );
			exit(0);
		}
#else
		fprintf( stderr, "Error: Unsupported operating system.\n" );
#endif
		exit(0);
	}

	fprintf( stderr, "Error: Unsupported option '%s'.\n\n", argv[1] );

	print_usage();

	exit(1);
}
