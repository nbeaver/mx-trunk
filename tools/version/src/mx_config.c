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
#include "mx_util.h"

#if defined(OS_UNIX)
#  include <sys/utsname.h>
#endif

#if defined(OS_WIN32)
#  include <windows.h>
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
	"  python\n"
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

/*------------------------------------------------------------------------*/

#if defined(OS_WIN32)
	if ( strcmp( argv[1], "python" ) == 0 ) {

		HKEY python_core_hkey;
		TCHAR python_version_keyname[80];
		char install_dir_keyname[200];
		char install_dir_pathname[MXU_FILENAME_LENGTH+80];
		DWORD buffer_size;
		DWORD dwType = REG_SZ;
		long win32_status;

		/*--------------------------------------------------------*/

		/* We need to figure out from the Windows registry where
		 * Python is installed and what version it is.
		 */

		win32_status = RegOpenKeyEx( HKEY_CURRENT_USER,
					TEXT("Software\\Python\\PythonCore"),
					0, KEY_QUERY_VALUE, &python_core_hkey );

		fprintf( stderr, "RegOpenKeyEx() status = %ld\n",
				win32_status );

		if ( win32_status != ERROR_SUCCESS ) {
			fprintf( stderr,
			"RegOpenKeyEx() failed with status %ld\n",
				win32_status );
			exit(1);
		}

		/*-----*/

		/* Use RegQueryInfoKey().  See "Enumerating Registry Subkeys"
		 * example in MSDN.
		 */

		/*-----*/

		/* Now get the first subkey of PythonCore, which should
		 * be the name of the Python version.  We _assume_ that 
		 * there is only one subkey.  If there are more than one,
		 * we pay attention to only the first one.
		 */

		buffer_size = sizeof(python_version_keyname);

		win32_status = RegEnumKeyEx( python_core_hkey, 0,
					(LPSTR) python_version_keyname,
					&buffer_size, NULL, NULL, NULL, NULL );

		fprintf( stderr, "RegEnumKeyEx() status = %ld\n",
				win32_status );

		if ( win32_status != ERROR_SUCCESS ) {
			fprintf( stderr,
			"RegEnumKeyEx() failed with status %ld\n",
				win32_status );
			exit(1);
		}

		fprintf( stderr, "python_version_keyname = '%s'\n",
			python_version_keyname );

		/*-----*/

		buffer_size = sizeof(python_version_keyname);

		win32_status = RegQueryValueEx( python_core_hkey,
					TEXT(""),
					NULL, &dwType,
					(LPBYTE) python_version_keyname,
					&buffer_size );

		fprintf( stderr, "RegQueryValueEx() status = %ld\n",
				win32_status );

		if ( win32_status != ERROR_SUCCESS ) {
			fprintf( stderr,
			"RegQueryValueEx() failed with status %ld\n",
				win32_status );
			exit(1);
		}

		fprintf( stderr, "python_version_keyname = '%s'\n",
				python_version_keyname );

		if ( argc < 3 ) {
			fprintf( stderr,
			"\n"
			"Usage:  mx_config python [option]\n"
			"\n"
			"    Where option may be\n"
			"        includes\n"
			"        installed\n"
			"        install_dir\n"
			"        library\n"
			"\n"
			);
			exit(1);
		}
	}
#endif

/*------------------------------------------------------------------------*/

	fprintf( stderr, "Error: Unsupported option '%s'.\n\n", argv[1] );

	print_usage();

	exit(1);
}

