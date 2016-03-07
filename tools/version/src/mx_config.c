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
 * Copyright 2011, 2014-2016 Illinois Institute of Technology
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
#include "mx_program_model.h"
#include "mx_util.h"

#if defined(OS_UNIX)
#  include <sys/utsname.h>
#endif

#if defined(OS_WIN32)
#  include <windows.h>
#endif

static void
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
	"  musl\n"
	"  mx_arch\n"
	"  mx_install_dir\n"
	"  mx_major\n"
	"  mx_minor\n"
	"  mx_update\n"
	"  mx_branch_label\n"
	"  mx_version\n"
	"  os_version\n"
	"  python\n"
	"  wordsize\n"
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

	if ( strcmp( argv[1], "mx_arch" ) == 0 ) {
		printf( "%s\n", MX_ARCH );
		exit(0);
	}
	if ( strcmp( argv[1], "mx_version" ) == 0 ) {
		printf( "%ld\n", MX_VERSION );
		exit(0);
	}
	if ( strcmp( argv[1], "mx_major" ) == 0 ) {
		printf( "%d\n", MX_MAJOR_VERSION );
		exit(0);
	}
	if ( strcmp( argv[1], "mx_minor" ) == 0 ) {
		printf( "%d\n", MX_MINOR_VERSION );
		exit(0);
	}
	if ( strcmp( argv[1], "mx_update" ) == 0 ) {
		printf( "%d\n", MX_UPDATE_VERSION );
		exit(0);
	}
	if ( strcmp( argv[1], "mx_branch_label" ) == 0 ) {
		printf( "%s\n", MX_BRANCH_LABEL );
		exit(0);
	}
	if ( strcmp( argv[1], "wordsize" ) == 0 ) {
		printf( "%d\n", MX_POINTER_SIZE );
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
#elif defined(_MSC_VER)
		printf( "msvcrt\n" );
#else
		printf( "unknown\n" );
#endif
		exit(0);
	}

/*------------------------------------------------------------------------*/

	if ( strcmp( argv[1], "os_version" ) == 0 ) {

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

		HKEY python_core_hkey, python_item_hkey;
		TCHAR python_version_keyname[80];
		char python_item_keyname[200];
		char python_item_keydata[2000];
		char python_item_doublequotes[2000];
		char original_char;
		DWORD buffer_size, keydata_size, dwType;
		long i, j, length, win32_status;
		int is_versions_command, items;
		unsigned long python_major, python_minor;
		char libname[40];

		if ( argc < 3 ) {
			fprintf( stdout,
			"ERR -\n"
			"Usage:  mx_config python [option] [version_number]\n"
			"\n"
			"    Where option may be\n"
			"        include\n"
			"        install_path\n"
			"        library\n"
			"        python_path\n"
			"        versions\n"
			"\n"
			);
			exit(1);
		}

		if ( strcmp( argv[2], "versions" ) == 0 ) {
			is_versions_command = TRUE;
		} else {
			is_versions_command = FALSE;

			if ( argc < 4 ) {
				fprintf( stdout,
			 "ERR - '%s %s %s' needs a version number argument.\n",
					argv[0], argv[1], argv[2] );
				exit(1);
			}
		}

		/*--------------------------------------------------------*/

		/* We need to figure out from the Windows registry where
		 * Python is installed and what version it is.
		 */

		win32_status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
					TEXT("Software\\Python\\PythonCore"),
					0, KEY_READ, &python_core_hkey );

		if ( win32_status != ERROR_SUCCESS ) {
			fprintf( stdout,
			"ERR - RegOpenKeyEx() failed with status %ld\n",
				win32_status );
			exit(1);
		}

		/*-----*/

		/* Enumerate and display the subkeys of PythonCore. */

		for ( i = 0; ; i++ ) {

			buffer_size = sizeof(python_version_keyname);

			win32_status = RegEnumKeyEx( python_core_hkey, i,
					(LPTSTR) python_version_keyname,
					&buffer_size, NULL, NULL, NULL, NULL );

			if ( win32_status == ERROR_NO_MORE_ITEMS ) {
				if ( is_versions_command == FALSE ) {
				    fprintf( stdout,
					"ERR - Version '%s' not found\n",
						argv[3] );
				} else {
				    if ( i > 0 ) {
					fprintf( stdout, "\n" );
					exit(0);
				    } else {
					fprintf( stdout,
						"ERR - Python not found\n" );
				    }
				}
				exit(1);
			}

			if ( win32_status != ERROR_SUCCESS ) {
				fprintf( stdout,
				"ERR - RegEnumKeyEx() failed with status %ld\n",
					win32_status );
				exit(1);
			}

			if ( is_versions_command ) {
				if ( i == 0 ) {
					fprintf( stdout, "%s",
						python_version_keyname );
				} else {
					fprintf( stdout, " %s",
						python_version_keyname );
				}
			} else {
				if ( strcmp( argv[3], python_version_keyname )
				    == 0 )
				{
					break;
				}
			}
		}

		/*-----*/

		_snprintf( python_item_keyname, sizeof(python_item_keyname),
			"Software\\Python\\PythonCore\\%s\\",
				python_version_keyname );

		length = sizeof( python_item_keyname )
			- strlen( python_item_keyname ) - 1;

		if ( strcmp( argv[2], "install_path" ) == 0 ) {
			strncat( python_item_keyname, "InstallPath", length );
		} else
		if ( strcmp( argv[2], "include" ) == 0 ) {
			strncat( python_item_keyname, "InstallPath", length );
		} else
		if ( strcmp( argv[2], "library" ) == 0 ) {
			strncat( python_item_keyname, "InstallPath", length );
		} else
		if ( strcmp( argv[2], "modules" ) == 0 ) {
			strncat( python_item_keyname, "Modules", length );
		} else
		if ( strcmp( argv[2], "python_path" ) == 0 ) {
			strncat( python_item_keyname, "PythonPath", length );
		} else {
			fprintf( stdout, "ERR - command '%s' not recognized\n",
				argv[2] );
			exit(1);
		}

		/*-----*/

		buffer_size = sizeof(python_item_keyname);

		win32_status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
					python_item_keyname,
					0, KEY_READ, &python_item_hkey );

		if ( win32_status != ERROR_SUCCESS ) {
			fprintf( stdout,
			"ERR - RegOpenKeyEx() failed with status %ld\n",
				win32_status );
			exit(1);
		}

		/*-----*/

		dwType = REG_SZ;

		keydata_size = sizeof(python_item_keydata);

		win32_status = RegQueryValueEx( python_item_hkey,
					TEXT(""),
					NULL, &dwType,
					(LPBYTE) python_item_keydata,
					&keydata_size );

		if ( win32_status != ERROR_SUCCESS ) {
			fprintf( stdout,
			"ERR - RegQueryValueEx() failed with status %ld\n",
				win32_status );
			exit(1);
		}

		length = sizeof( python_item_keydata )
			- strlen( python_item_keydata ) - 1;

		if ( strcmp( argv[2], "include" ) == 0 ) {
			strncat( python_item_keydata, "include", length );
		} else
		if ( strcmp( argv[2], "library" ) == 0 ) {
			items = sscanf( python_version_keyname, "%lu.%lu",
					&python_major, &python_minor );

			if ( items != 2 ) {
				fprintf( stdout,
				"ERR - Unparseable version keyname '%s'\n",
					python_version_keyname );
				exit(1);
			}

			_snprintf( libname, sizeof(libname),
				"libs\\python%lu%lu.lib",
				python_major, python_minor );

			strncat( python_item_keydata, libname, length );
		}

#if 1
		for ( i = 0, j = 0;
			i < sizeof(python_item_keydata),
			j < sizeof(python_item_doublequotes);
			i++, j++ )
		{
			original_char = python_item_keydata[i];

			python_item_doublequotes[j] = original_char;

			if ( original_char == '\\' ) {
				j++;
				python_item_doublequotes[j] = original_char;
			}

			if ( original_char == '\0' )
				break;
		}
#endif

		fprintf( stdout, "%s\n", python_item_doublequotes );

		exit(0);
	}
#endif

/*------------------------------------------------------------------------*/

	fprintf( stderr, "Error: Unsupported option '%s'.\n\n", argv[1] );

	print_usage();

	exit(1);
}

