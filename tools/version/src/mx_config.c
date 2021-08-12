/*
 * Name:    mx_config.c
 *
 * Purpose: This program can be used to get information about the
 *          installed version of MX.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2011, 2014-2018, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "mx_osdef.h"
#include "mx_private_version.h"
#include "mx_program_model.h"
#include "mx_util.h"
#include "mx_unistd.h"

#if defined(OS_UNIX)
#  include <sys/utsname.h>
#  include <dirent.h>
#endif

#if defined(OS_WIN32)
#  include <windows.h>
#endif

static void
print_usage( void )
{
	printf(
	"Usage: mx_config [option]\n"
	"\n"
	"  The known options (not available on all build targets) are:\n"
	"\n"
	"  clang\n"
	"  gcc\n"
	"  glibc\n"
	"  gnuc\n"
	"  id\n"
	"  id_class\n"
	"  id_like\n"
	"  library\n"
	"  library_found\n"
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

/* mx_config cannot use the version of strlcpy() bundled with libMx,
 * so we define a simple version of it here for build targets that
 * need it.
 */

#if !defined(OS_MACOSX)

size_t
strlcpy( char *dest, const char *src, size_t dest_size )
{
	size_t bytes_in_source, bytes_to_copy;

	memset( dest, 0, dest_size );

	bytes_in_source = strlen( src );

	if ( bytes_in_source < dest_size ) {
		bytes_to_copy = bytes_in_source;
	} else {
		bytes_to_copy = dest_size - 1;
	}

	memmove( dest, src, bytes_to_copy );

	return bytes_in_source;
}

#endif

#if defined(OS_LINUX)

/* This is a copy of the mx_match() function from libMx.  The mx_config
 * program _MUST_ _NOT_ depend on libMx, so we make a copy here.
 */

static int
mxp_match( const char *pattern, const char *string )
{
	switch( *pattern ) {
	case '\0':	return ! *string;

	case '*':	return mxp_match( pattern+1, string ) ||
				( *string && mxp_match( pattern, string+1 ) );

	case '?':	return *string && mxp_match( pattern+1, string+1 );

	default:	return *pattern == *string &&
					mxp_match( pattern+1, string+1 );
	}
}

#endif

#if defined(OS_LINUX)

static int linux_parameters_saved = FALSE;

static char linux_name[100] = "Linux";
static char linux_id[100] = "linux";
static char linux_id_like[200] = "";

void
get_linux_parameters( void )
{
	FILE *osfile = NULL;
	char parameter_buffer[200];
	char *parameter_name = NULL;
	char *parameter_value = NULL;
	size_t parameter_length;

	if ( linux_parameters_saved ) {
		return;
	}

	linux_parameters_saved = TRUE;

	osfile = fopen( "/etc/os-release", "r" );

	if ( osfile == (FILE *) NULL ) {
		return;
	}

	while (1) {
		fgets( parameter_buffer, sizeof(parameter_buffer), osfile );

		if ( feof(osfile) || ferror(osfile) ) {
			fclose( osfile );
			return;
		}

		/* Split buffer line into parameter name and value. */

		parameter_value = strchr( parameter_buffer, '=' );

		if ( parameter_value == NULL ) {

			/* No parameter found here. Go to the next line. */
			continue;
		}

		parameter_name = parameter_buffer;

		*parameter_value = '\0';

		parameter_value++;

		if ( *parameter_value == '"' ) {
			parameter_value++;
		}

		parameter_length = strlen( parameter_value );

		if ( parameter_value[ parameter_length - 1 ] == '\n' ) {
			parameter_value[ parameter_length - 1 ] = '\0';

			parameter_length = strlen( parameter_value );
		}


		if ( parameter_value[ parameter_length - 1 ] == '"' ) {
			parameter_value[ parameter_length - 1 ] = '\0';

			parameter_length = strlen( parameter_value );
		}

#if 0
		printf( "parameter '%s' = '%s'\n",
			parameter_name, parameter_value );
#endif

		if ( strcmp( parameter_name, "NAME" ) == 0 ) {
		    strlcpy( linux_name, parameter_value, sizeof(linux_name) );
		} else
		if ( strcmp( parameter_name, "ID" ) == 0 ) {
		    strlcpy( linux_id, parameter_value, sizeof(linux_id) );
		} else
		if ( strcmp( parameter_name, "ID_LIKE" ) == 0 ) {
		    strlcpy( linux_id_like, parameter_value,
				    sizeof(linux_id_like) );
		}
	}

	fclose( osfile );

	return;
}

#endif

#if defined(OS_WIN32)

static void
mxp_insert_double_backslashes( char *dest_buffer,
			size_t dest_buffer_length,
			char *src_buffer,
			size_t src_buffer_length )
{
	size_t i, j;
	char original_char;

	for ( i = 0, j = 0;
		i < src_buffer_length, j < dest_buffer_length;
		i++, j++ )
	{
		original_char = src_buffer[i];

		if ( original_char == '/' ) {
			original_char = '\\';
		}

		dest_buffer[j] = original_char;

		if ( original_char == '\\' ) {
			j++;

			if ( j >= dest_buffer_length ) {
				dest_buffer[j-1] = '\0';
				return;
			}

			dest_buffer[j] = original_char;
		}

		if ( original_char == '\0' ) {
			return;
		}
	}
}

#endif

int
main( int argc, char **argv )
{
	if ( argc < 2 ) {
		print_usage();
		exit(1);
	}

#if ( defined(OS_WIN32) && defined(__GNUC__) )
	if ( strcmp( argv[1], "mx_arch" ) == 0 ) {
		printf( "%s\n", "win32-mingw" );
		exit(0);
	}
#else
	if ( strcmp( argv[1], "mx_arch" ) == 0 ) {
		printf( "%s\n", MX_ARCH );
		exit(0);
	}
#endif
	if ( strcmp( argv[1], "mx_install_dir" ) == 0 ) {
		printf( "%s\n", MX_INSTALL_DIR );
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

#if defined(MX_CLANG_VERSION)
	if ( strcmp( argv[1], "clang" ) == 0 ) {
		printf( "%ld\n", MX_CLANG_VERSION );
		exit(0);
	}
#endif

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
		printf( "%ld\n", MX_DARWIN_VERSION );
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

				printf( 
		"ERR - uname() failed with errno = %d, error message = '%s'\n",
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
		printf( "ERR - Unsupported operating system.\n" );
#endif
		exit(0);
	}

/*------------------------------------------------------------------------*/

#if defined(OS_LINUX)

	/* This is for reading values from /etc/os-release on Linux*/

	if ( strcmp( argv[1], "name" ) == 0 ) {
		get_linux_parameters();

		printf( "%s\n", linux_name );
		exit(0);
	}

	if ( strcmp( argv[1], "id" ) == 0 ) {
		get_linux_parameters();

		printf( "%s\n", linux_id );
		exit(0);
	}

	if ( strcmp( argv[1], "id_like" ) == 0 ) {
		get_linux_parameters();

		printf( "%s\n", linux_id_like );
		exit(0);
	}

	if ( strcmp( argv[1], "id_class" ) == 0 ) {
		int i, c;

		get_linux_parameters();

		/* Display only the first word in the id_like parameter. */

		for ( i = 0; ; i++ ) {
			c = linux_id_like[i];

			if ( ( c == '\0' ) || ( c == ' ' ) ) {
				break;	/* Exit the for() loop. */
			}
			putchar( c );
		}

		putchar( '\n' );
		exit(0);
	}

#endif

/*------------------------------------------------------------------------*/

#if defined(OS_LINUX)

	/* library_found */

#endif

/*------------------------------------------------------------------------*/

#if defined(MX_PYTHON)

	if ( strcmp( argv[1], "python" ) == 0 ) {
		fprintf( stderr, "Python is here.\n" );
	}

/*------------------------------------------------------------------------*/

#elif defined(OS_WIN32)
	if ( strcmp( argv[1], "python" ) == 0 ) {

		HKEY hive_key;
		HKEY python_core_hkey, python_item_hkey;
		TCHAR python_version_keyname[80];
		char python_item_keyname[200];
		char python_item_keydata[2000];
		char python_item_double_backslash[2000];
		char mxdir[2000];
		DWORD buffer_size, keydata_size, dwType;
		long i, length, win32_status;
		int is_versions_command, items;
		unsigned long python_major, python_minor;
		char libname[40];

		if ( argc < 3 ) {
			printf( 
			"ERR -\n"
			"Usage:  mx_config python [option] [version_number]\n"
			"\n"
			"    Where option may be\n"
			"        batch\n"
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
				printf( 
			 "ERR - '%s %s %s' needs a version number argument.\n",
					argv[0], argv[1], argv[2] );
				exit(1);
			}
		}

		/*--------------------------------------------------------*/

		/* We need to figure out from the Windows registry where
		 * Python is installed and what version it is.
		 */

		hive_key = HKEY_LOCAL_MACHINE;

		win32_status = RegOpenKeyEx( hive_key,
					TEXT("Software\\Python\\PythonCore"),
					0, KEY_READ, &python_core_hkey );

		if ( win32_status == ERROR_FILE_NOT_FOUND ) {
			/* We did not find it in the HKEY_LOCAL_MACHINE hive,
			 * so let's look for it in HKEY_CURRENT_USER instead.
			 */

			hive_key = HKEY_CURRENT_USER;

			win32_status = RegOpenKeyEx( hive_key,
					TEXT("Software\\Python\\PythonCore"),
					0, KEY_READ, &python_core_hkey );

			if ( win32_status == ERROR_FILE_NOT_FOUND ) {

				/* The MX makefile for the tools directory
				 * assumes that an exit code of 2 means
				 * 'Python not found'.  So make sure that
				 * this code path always returns an exit
				 * code of 2.  Also make sure that none
				 * of the other branches of the 'python'
				 * code tree do _not_ an exit code of 2.
				 * Also make sure that the error message
				 * below is written to stderr rather than
				 * stdout, so that it can be separately
				 * discarded.
				 */

				fprintf( stderr, "ERR - "
				"Python does not appear to be installed "
				"on this computer.\n" );
				exit(2);
			}
		}

		if ( win32_status != ERROR_SUCCESS ) {
			printf( "ERR - RegOpenKeyEx() failed with status %ld\n",
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
				    printf( "ERR - Version '%s' not found\n",
						argv[3] );
				} else {
				    if ( i > 0 ) {
					printf( "\n" );
					exit(0);
				    } else {
					printf( "ERR - Python not found\n" );
				    }
				}
				exit(1);
			}

			if ( win32_status != ERROR_SUCCESS ) {
				printf( 
				"ERR - RegEnumKeyEx() failed with status %ld\n",
					win32_status );
				exit(1);
			}

			if ( is_versions_command ) {
				if ( i == 0 ) {
					printf( "%s", python_version_keyname );
				} else {
					printf( " %s", python_version_keyname );
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

		if ( strcmp( argv[2], "batch" ) == 0 ) {
			strncat( python_item_keyname, "InstallPath", length );
		} else
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
			printf( "ERR - command '%s' not recognized\n",
				argv[2] );
			exit(1);
		}

		/*-----*/

		buffer_size = sizeof(python_item_keyname);

		win32_status = RegOpenKeyEx( hive_key,
					python_item_keyname,
					0, KEY_READ, &python_item_hkey );

		if ( win32_status != ERROR_SUCCESS ) {
			printf(
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
			printf(
			"ERR - RegQueryValueEx() failed with status %ld\n",
				win32_status );
			exit(1);
		}

		length = sizeof( python_item_keydata )
			- strlen( python_item_keydata ) - 1;

		if ( python_item_keydata[length-1] != '\\' ) {
			/* If the key data does not end in a backslash,
			 * then add a backslash ourselves.
			 */

			strncat( python_item_keydata, "\\", length );

			length = sizeof( python_item_keydata )
				- strlen( python_item_keydata ) - 1;
		}

		if ( strcmp( argv[2], "include" ) == 0 ) {
			strncat( python_item_keydata, "include", length );
		} else
		if ( strcmp( argv[2], "library" ) == 0 ) {
			items = sscanf( python_version_keyname, "%lu.%lu",
					&python_major, &python_minor );

			if ( items != 2 ) {
				printf( 
				"ERR - Unparseable version keyname '%s'\n",
					python_version_keyname );
				exit(1);
			}

			_snprintf( libname, sizeof(libname),
				"libs\\python%lu%lu.lib",
				python_major, python_minor );

			strncat( python_item_keydata, libname, length );
		}

		mxp_insert_double_backslashes( python_item_double_backslash,
					sizeof(python_item_double_backslash),
					python_item_keydata,
					sizeof(python_item_keydata) );

		if ( strcmp( argv[2], "batch" ) == 0 ) {

			printf(
"@rem This batch file makes MX use Win32 Python rather than Cygwin Python.\n" );

			if ( argc >= 5 ) {
				mxp_insert_double_backslashes( mxdir,
							sizeof(mxdir),
							argv[4],
							strlen(argv[4]) - 1 );

				printf( "@set path=%s\\\\bin;%s;%%path%%\n",
					mxdir, python_item_double_backslash );
			} else {
				printf( "@set path=%s;%%path%%\n",
					python_item_double_backslash );
			}
		} else {
			/* include, library, install_path */

			printf( "%s\n", python_item_double_backslash );
		}

		exit(0);
	}

#elif defined(OS_LINUX)
	if ( strcmp( argv[1], "python" ) == 0 ) {

#define MXCFG_DEFAULT_OPTION		1
#define MXCFG_INCLUDE_OPTION		2
#define MXCFG_LIBRARY_OPTION		3
#define MXCFG_VERSIONS_OPTION		4
#define MXCFG_VERSION_PATHS_OPTION	5

		int i, option_type, file_status, match_status;
		char *path_string, *path_string_copy, *path_element;
		char *search_ptr, *mx_python_env_ptr;
		char filename_match[PATH_MAX+1];
		DIR *dir;
		struct dirent *dirent_ptr;
		char *name_ptr;

		if ( argc < 3 ) {
			printf(
			"ERR -\n"
			"Usage:  mx_config python [option] [version_number]\n"
			"\n"
			"    Where option may be\n"
			"        default\n"
			"        include\n"
			"        library\n"
			"        versions\n"
			"        version_paths\n"
			"\n"
			);
			exit(1);
		}

		if ( strcmp( argv[2], "default" ) == 0 ) {
			option_type = MXCFG_DEFAULT_OPTION;
		} else
		if ( strcmp( argv[2], "include" ) == 0 ) {
			option_type = MXCFG_INCLUDE_OPTION;
		} else
		if ( strcmp( argv[2], "library" ) == 0 ) {
			option_type = MXCFG_LIBRARY_OPTION;
		} else
		if ( strcmp( argv[2], "versions" ) == 0 ) {
			option_type = MXCFG_VERSIONS_OPTION;
		} else
		if ( strcmp( argv[2], "version_paths" ) == 0 ) {
			option_type = MXCFG_VERSION_PATHS_OPTION;
		} else {
			fprintf( stderr,
			"mx_config: Unrecognized option '%s'\n", argv[2] );
			exit(1);
		}

		if ( ( option_type != MXCFG_DEFAULT_OPTION )
		  && ( option_type != MXCFG_VERSIONS_OPTION )
		  && ( option_type != MXCFG_VERSION_PATHS_OPTION ) )
		{
			if ( argc < 4 ) {
				printf(
			 "ERR - '%s %s %s' needs a version number argument.\n",
					argv[0], argv[1], argv[2] );
				exit(1);
			}
		}

		/* Walk through the path looking for Python versions. */

		path_string = getenv( "PATH" );

		if ( path_string == NULL ) {
			fprintf( stderr,
		    "ERR - The 'PATH' environment variable is not defined.\n" );
			exit(1);
		}

		path_string_copy = strdup( path_string );

		if ( path_string_copy == NULL ) {
			fprintf( stderr,
			"ERR - Copying the PATH variable failed.\n" );
			exit(1);
		}

		mx_python_env_ptr = getenv("MX_PYTHON");

#if 0
		fprintf( stderr, "mx_python_env_ptr = '%s'\n",
				mx_python_env_ptr );
#endif

		search_ptr = path_string_copy;

		for ( i = 0; ; i++ ) {
			path_element = strtok( search_ptr, ":" );

			if ( path_element == NULL ) {
				break;
			}

			search_ptr = NULL;

#if 0
			fprintf( stderr, "path_element[%d] = '%s'\n",
				i, path_element );
#endif

			switch( option_type ) {
			case MXCFG_DEFAULT_OPTION:
				if ( mx_python_env_ptr == NULL ) {
					snprintf(filename_match,
					sizeof(filename_match),
					"%s/python3", path_element );
				} else
				if ( mx_python_env_ptr[0] == '2' ) {
					snprintf(filename_match,
					sizeof(filename_match),
					"%s/python", path_element );
				} else {
					snprintf(filename_match,
					sizeof(filename_match),
					"%s/python%s", path_element,
					mx_python_env_ptr );
				}

				errno = 0;

				file_status = access( filename_match, X_OK );

#if 0
				fprintf( stderr,
				"filename_match = '%s' [%d] [errno = %d]\n",
					filename_match, file_status, errno );
#endif

				if ( file_status == 0 ) {
					printf( "%s\n", filename_match );
					exit(0);
				}
				break;
			case MXCFG_VERSIONS_OPTION:
			case MXCFG_VERSION_PATHS_OPTION:
				dir = opendir( path_element );

				if ( dir == (DIR *) NULL ) {
					/* Skip unavailable PATH elements. */
					break;
				}

				while (1) {
					errno = 0;

					dirent_ptr = readdir(dir);

					if ( dirent_ptr != NULL ) {
						name_ptr = dirent_ptr->d_name;

						match_status = mxp_match(
						  "python*", name_ptr );

						if ( match_status == 1 ) {
						    printf( "%s ", name_ptr );
						}

						fprintf( stderr,
						"name_ptr = '%s' [%d]\n",
						    name_ptr, match_status );
					} else {
						if ( errno == 0 ) {
							break; /* Exit while. */
						} else {
							fprintf( stderr,
					    "An error occurred while examining "
					    "the files in the PATH directory "
					    "'%s'.  Errno = %d, "
					    "error message = '%s'\n",
						path_element,
						errno, strerror(errno) );
							exit(1);
						}
					}
				}

				closedir(dir);

				break;
			}
		}

		free( path_string_copy );
		exit(0);
	}
#endif

/*------------------------------------------------------------------------*/

	printf( "ERR -: Unsupported option '%s'.\n\n", argv[1] );

	print_usage();

	exit(1);
}

