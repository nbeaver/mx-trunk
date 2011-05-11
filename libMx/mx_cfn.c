/*
 * Name:     mx_cfn.c
 *
 * Purpose:  Functions for constructing MX control system filenames.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006, 2009, 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_CFN_DEBUG			FALSE

#define MX_DEBUG_SETUP_HOME_VARIABLE	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_cfn.h"
#include "mx_cfn_defaults.h"

#if defined(OS_WIN32) || defined(OS_DJGPP)
#  define SETUP_HOME_VARIABLE	TRUE
#else
#  define SETUP_HOME_VARIABLE	FALSE
#endif

#if defined(OS_WIN32) && defined(_MSC_VER)
#  define putenv	_putenv
#endif

/* Some platforms do not automatically set up a HOME variable. */

#if SETUP_HOME_VARIABLE

static mx_bool_type mxp_home_variable_set = FALSE;

#define MXP_SETUP_HOME_USING_ROOT_DIR	1
#define MXP_SETUP_HOME_USING_HOMEPATH	2

static mx_status_type
mxp_setup_home_variable( void )
{
	static const char fname[] = "mxp_setup_home_variable()";

	char zero_length_string[] = "";
	char home[MXU_FILENAME_LENGTH+10];
	char os_version_string[40];
	char *home_ptr, *homepath_ptr, *homedrive_ptr, *homeshare_ptr;
	size_t length;
	int setup_type, status;
	mx_status_type mx_status;

	mxp_home_variable_set = TRUE;

	/* First, see if the HOME variable already exists. */

	home_ptr = getenv( "HOME" );

	if ( home_ptr != NULL ) {
		/* If the HOME variable already exists,
		 * we do not need to do anything further.
		 */

#if MX_DEBUG_SETUP_HOME_VARIABLE
		MX_DEBUG(-2,("%s: HOME variable already set to value '%s'",
			fname, home_ptr));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* We do different things depending on the operating system version. */

	mx_status = mx_get_os_version_string( os_version_string,
						sizeof(os_version_string) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_DEBUG_SETUP_HOME_VARIABLE
	MX_DEBUG(-2,("%s: os_version_string = '%s'", fname, os_version_string));
#endif

	length = strlen( os_version_string );

	if ( strncmp( os_version_string, "Windows 7", length ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_HOMEPATH;
	} else
	if ( strncmp( os_version_string, "Windows Vista", length ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_HOMEPATH;
	} else
	if ( strncmp( os_version_string, "Windows Server 2003", length ) == 0 ){
		setup_type = MXP_SETUP_HOME_USING_HOMEPATH;
	} else
	if ( strncmp( os_version_string, "Windows XP", length ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_HOMEPATH;
	} else
	if ( strncmp( os_version_string, "Windows 2000", length ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_HOMEPATH;
	} else
	if ( strncmp( os_version_string, "Windows NT 4.0", length ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_HOMEPATH;
	} else
	if ( strncmp( os_version_string, "Windows NT 3.51", length ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_HOMEPATH;
	} else
	if ( strncmp( os_version_string, "Windows Me", length ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_ROOT_DIR;
	} else
	if ( strncmp( os_version_string, "Windows 98", length ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_ROOT_DIR;
	} else
	if ( strncmp( os_version_string, "Windows 95", length ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_ROOT_DIR;
	} else
	if ( strncmp( os_version_string, "MS-DOS", 6 ) == 0 ) {
		setup_type = MXP_SETUP_HOME_USING_ROOT_DIR;
	} else {
		setup_type = -1;
	}

#if MX_DEBUG_SETUP_HOME_VARIABLE
	MX_DEBUG(-2,("%s: setup_type = %d", fname, setup_type ));
#endif

	switch( setup_type ) {
	case MXP_SETUP_HOME_USING_ROOT_DIR:
		strlcpy( home, "HOME=c:/", sizeof(home) );
		break;
	case MXP_SETUP_HOME_USING_HOMEPATH:
		homeshare_ptr = getenv( "HOMESHARE" );
		homedrive_ptr = getenv( "HOMEDRIVE" );
		homepath_ptr = getenv( "HOMEPATH" );

		if ( homeshare_ptr == NULL ) {
			homeshare_ptr = zero_length_string;

			if ( homedrive_ptr == NULL ) {
				homedrive_ptr = zero_length_string;
			}
		} else {
			/* Do not use HOMEDRIVE if HOMESHARE is specified. */

			homedrive_ptr = zero_length_string;
		}

		if ( homepath_ptr == NULL ) {
			homepath_ptr = zero_length_string;
		}

		snprintf( home, sizeof(home), "HOME=%s%s%s",
			homeshare_ptr, homedrive_ptr, homepath_ptr );
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Finding the user's home directory is not yet implemented "
		"for '%s'.", os_version_string );
	}

#if MX_DEBUG_SETUP_HOME_VARIABLE
	MX_DEBUG(-2,("%s: '%s'", fname, home));
#endif

	status = putenv( home );

	if ( status != 0 ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to set the environment variable HOME to '%s'", home);
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* SETUP_HOME_VARIABLE */

/*--------------------------------------------------------------------------*/

/**** Platform dependent versions of mx_is_absolute_filename() ****/

#if defined(OS_WIN32)

/* Win32 filenames may or may not begin with a drive letter. */

MX_EXPORT mx_bool_type
mx_is_absolute_filename( const char *filename )
{
	static const char fname[] = "mx_is_absolute_filename()";

	size_t num_chars;

	if ( filename == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename argument passed was NULL." );

		return FALSE;
	}

	/* Does the filename begin with a drive letter? */

	if ( filename[1] == ':' ) {

		/* Is the first character a valid drive letter? */

		num_chars = strcspn( filename,
		    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" );

		if ( num_chars != 0 ) {
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "Drive '%c:' in filename '%s' is not a valid drive letter.",
				filename[0], filename );

			return FALSE;
		}

		if ( ( filename[2] == '\\' ) || ( filename[2] == '/' ) ) {
			return TRUE;
		} else {
			return FALSE;
		}
	} else {
		if ( ( filename[0] == '\\' ) || ( filename[0] == '/' ) ) {
			return TRUE;
		} else {
			return FALSE;
		}
	}

	return FALSE;
}

#elif defined(OS_VMS)

/* Ignore the issue and pretend that all filenames are absolute. */

MX_EXPORT mx_bool_type
mx_is_absolute_filename( const char *filename )
{
	return TRUE;
}

#else

/* On Posix-like systems, any filename beginning with '/' is assumed
 * to be an absolute filename.
 */

MX_EXPORT mx_bool_type
mx_is_absolute_filename( const char *filename )
{
	static const char fname[] = "mx_is_absolute_filename()";

	if ( filename == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename argument passed was NULL." );

		return FALSE;
	}

	if ( filename[0] == '/' ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#endif

/*--------------------------------------------------------------------------*/

/**** Platform dependent versions of mx_normalize_filename() ****/

#if defined(OS_VMS)

/* Currently, for this platform we do not perform any
 * real normalization.  This probably means that using
 * the default file locations will not work here.
 */

MX_EXPORT char *
mx_normalize_filename( const char *original_filename,
			char *new_filename,
			size_t max_filename_length )
{
	static const char fname[] = "mx_normalize_filename()";

	if ( original_filename == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"'original_filename' argument is NULL." );

		return NULL;
	}
	if ( new_filename == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"'new_filename' argument is NULL." );

		return NULL;
	}
	if ( max_filename_length == 0 ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified maximum filename length of %ld is too short "
		"to fit even a 1 byte string into.",
			(long) max_filename_length );
	}

	strlcpy( new_filename, original_filename, max_filename_length );

	return new_filename;
}

#else

/* Posix and Win32 style normalization. */

MX_EXPORT char *
mx_normalize_filename( const char *original_filename,
			char *new_filename,
			size_t max_filename_length )
{
	static const char fname[] = "mx_normalize_filename()";

	mx_bool_type slash_seen;
	size_t i, j, original_length;
	char c;

	if ( original_filename == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"'original_filename' argument is NULL." );

		return NULL;
	}
	if ( new_filename == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"'new_filename' argument is NULL." );

		return NULL;
	}
	if ( max_filename_length <= 0 ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified maximum filename length of %ld is too short "
		"to fit even a 1 byte string into.",
			(long) max_filename_length );
	}

	/* Convert multiple '/' characters in a row
	 * to just a single '/' character.
	 */
	
	original_length = strlen(original_filename);

	slash_seen = FALSE;

	for( i = 0, j = 0; i < original_length; i++ ) {
		c = original_filename[i];

#if defined(OS_WIN32)
		/* Convert backslashes to forward slashes. */

		if ( c == '\\' ) {
			c = '/';
		}
#endif

		if ( slash_seen && ( c == '/' ) ) {
			/* Skip this character. */
		} else {
			if ( j < max_filename_length ) {
				new_filename[j] = c;
				new_filename[j+1] = '\0';

				j++;
			} else {
				(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Original filename '%s' is too long to fit "
				"into the new filename buffer of %ld bytes "
				"after extra '/' characters are removed.",
					original_filename,
					(long) max_filename_length);

				return NULL;
			}
		}

		if ( c == '/' ) {
			slash_seen = TRUE;
		} else {
			slash_seen = FALSE;
		}
	}

	return new_filename;
}

#endif

/*--------------------------------------------------------------------------*/

static char *
mxp_search_path( const char *original_filename,
		const char *extension,
		int argc, char **argv )
{
	char temp_filename[ MXU_FILENAME_LENGTH+1 ];
	long i;
	int os_status;
	char *found_pathname = NULL;

	for ( i = 0; i < argc; i++ ) {

		if ( extension != NULL ) {
			snprintf( temp_filename, sizeof(temp_filename),
			"%s/%s%s", argv[i], original_filename, extension );
		} else {
			snprintf( temp_filename, sizeof(temp_filename),
			"%s/%s", argv[i], original_filename );
		}

		os_status = access( temp_filename, R_OK );

		if ( os_status == 0 ) {
			found_pathname = strdup( temp_filename );

			return found_pathname;
		}
	}

	return NULL;
}

/*-------*/

MX_EXPORT char *
mx_find_file_in_path( const char *original_filename,
			const char *extension,
			const char *path_variable_name,
			unsigned long flags )
{
	static const char fname[] = "mx_find_file_in_path()";

	size_t original_length, extension_length;
	const char *extension_ptr = NULL;
	int status, path_argc;
	char **path_argv = NULL;
	char *path_variable = NULL;
	mx_bool_type already_has_extension, have_path_variable_array;
	mx_bool_type try_without_extension, look_in_current_directory;
	mx_bool_type contains_path_separator;
	char *modified_filename = NULL;
	char *found_filename = NULL;
	int os_status;

	if ( original_filename == NULL )
		return NULL;

	/*---*/

	if ( flags & MXF_FPATH_TRY_WITHOUT_EXTENSION ) {
		try_without_extension = TRUE;
	} else {
		try_without_extension = FALSE;
	}

	if ( flags & MXF_FPATH_LOOK_IN_CURRENT_DIRECTORY ) {
		look_in_current_directory = TRUE;
	} else {
		look_in_current_directory = FALSE;
	}

	/* Does the filename already have the extension attached? */

	original_length = strlen( original_filename );

	already_has_extension = FALSE;

	if ( extension == NULL ) {
		already_has_extension = TRUE;
	} else {
		extension_length = strlen( extension );

		if ( extension_length > original_length ) {
			already_has_extension = FALSE;
		} else {
			extension_ptr = original_filename
				+ (original_length - extension_length);

#if defined(OS_WIN32)
			if ( mx_strcasecmp( extension_ptr, extension ) == 0 ) {
#else
			if ( strcmp( extension_ptr, extension ) == 0 ) {
#endif
				already_has_extension = TRUE;
			}
		}
	}

	/* Does the original filename contain a path separator character? */

	contains_path_separator = FALSE;

	if ( strchr( original_filename, '/' ) != NULL ) {
		contains_path_separator = TRUE;
	}

#if defined( OS_WIN32 )
	if ( strchr( original_filename, '\\' ) != NULL ) {
		contains_path_separator = TRUE;
	}
#endif

	if ( contains_path_separator ) {
		/* If the original filename contains a path separator, then
		 * this filename is either an absolute pathname or a relative
		 * pathname.  In either case, we ignore the path_variable_name
		 * variable passed to us and try looking for the original
		 * filename as is.
		 */

		if ( already_has_extension == FALSE ) {

			modified_filename = malloc( MXU_FILENAME_LENGTH + 1 );

			if ( modified_filename == NULL ) {
				mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate memory for a "
			"temporary filename buffer." );

				return NULL;
			}

			snprintf( modified_filename, MXU_FILENAME_LENGTH,
				"%s%s", original_filename, extension );

			os_status = access( modified_filename, R_OK );

			if ( os_status == 0 ) {
				return modified_filename;
			}

			mx_free( modified_filename );
		}

		os_status = access( original_filename, R_OK );

		if ( os_status == 0 ) {
			/* The file _does_ exist.  We strdup() a copy of the
			 * original filename, since the caller assumes that
			 * it will be returned a malloc-ed buffer that needs
			 * to be freed.
			 */

			found_filename = strdup( original_filename );

			return found_filename;
		} else {
			return NULL;
		}
	}

	/*-----------------------------------------------------------------*/

	/* If we get here, then we need to search the path, if available,
	 * and/or look in the current directory.
	 */

	/* If possible, construct the path variable array. */

	have_path_variable_array = FALSE;

	if ( path_variable_name != NULL ) {
		path_variable = getenv( path_variable_name );

		if ( path_variable != NULL ) {
			/* mx_path_variable_split() will write to the
			 * contents of the 'path_variable' passed to it.
			 * Thus, we need to make a copy of the path
			 * variable before calling mx_path_variable_split().
			 */

			path_variable = strdup( path_variable );

			if ( path_variable == NULL ) {
				mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to duplicate the "
				"contents of the '%s' environment variable.",
					path_variable_name );

				return NULL;
			}

			status = mx_path_variable_split( path_variable,
						&path_argc, &path_argv );

			if ( status == 0 ) {
				have_path_variable_array = TRUE;
			} else {
				mx_free( path_variable );
			}
		}
	}

	if ( have_path_variable_array ) {
		if ( already_has_extension ) {
			/* Search the path using the already built-in
			 * extension.
			 */

			found_filename = mxp_search_path( original_filename,
							NULL,
							path_argc, path_argv );
		} else {
			/* Search the path using an extension. */

			found_filename = mxp_search_path( original_filename,
							extension,
							path_argc, path_argv );

			if ( found_filename != NULL ) {
				mx_free( path_argv );
				mx_free( path_variable );
				return found_filename;
			}

			/* If we get here, an attempt to search for the file
			 * with the caller-provided extension failed.
			 *
			 * If requested and the caller-provided extension
			 * was _not_ NULL, then we try again _without_ the
			 * caller-provided extension.
			 */

			if ( ( try_without_extension )
			  && ( extension != NULL ) )
			{
				found_filename = mxp_search_path(
							original_filename,
							NULL,
							path_argc, path_argv );

				if ( found_filename != NULL ) {
					mx_free( path_argv );
					mx_free( path_variable );
					return found_filename;
				}
			}
		}
		mx_free( path_argv );
		mx_free( path_variable );
	}

	/* If we get here, any attempt to search the path has failed.
	 * If requested, we also search in the current directory.
	 */

	if ( look_in_current_directory ) {
		if ( already_has_extension == FALSE ) {

			modified_filename = malloc( MXU_FILENAME_LENGTH + 1 );

			if ( modified_filename == NULL ) {
				mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate memory for a "
			"temporary filename buffer." );

				return NULL;
			}

			snprintf( modified_filename, MXU_FILENAME_LENGTH,
				"%s%s", original_filename, extension );

			os_status = access( modified_filename, R_OK );

			if ( os_status == 0 ) {
				return modified_filename;
			}

			mx_free( modified_filename );
		}

		if ( already_has_extension || try_without_extension ) {

			os_status = access( original_filename, R_OK );

			if ( os_status == 0 ) {
				/* Duplicate the original filename, since the
				 * caller will assume that any non-NULL pointer
				 * returned is a malloc-ed buffer that it will
				 * need to free.
				 */

				found_filename = strdup( original_filename );

				return found_filename;
			}
		}
	}
		
	return NULL;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT int
mx_path_variable_split( char *path_variable,
			int *argc, char ***argv )
{
	int status;

	if ( ( path_variable== NULL )
	  || ( argc == NULL )
	  || ( argv == NULL ) )
	{
		errno = EINVAL;
		return -1;
	}

	/* Windows separates path variables using a semicolon ';'
	 * while everybody else uses a colon ':'.
	 */

#if defined(OS_WIN32)
	status = mx_string_split( path_variable, ";", argc, argv );
#else
	status = mx_string_split( path_variable, ":", argc, argv );
#endif

	return status;
}

/*--------------------------------------------------------------------------*/

#define MS_NOT_IN_MACRO		1
#define MS_STARTING_MACRO	2
#define MS_IN_MACRO		3

MX_EXPORT char *
mx_expand_filename_macros( const char *original_filename,
			char *new_filename,
			size_t max_filename_length )
{
	static const char fname[] = "mx_expand_filename_macros()";

	char macro_name[MXU_FILENAME_LENGTH+1];
	char macro_contents[MXU_FILENAME_LENGTH+1];
	char *macro_ptr;
	size_t i, j, original_length, name_length, macro_length;
	int macro_state;
	char c;

	if ( original_filename == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"'original_filename' argument is NULL." );

		return NULL;
	}
	if ( new_filename == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"'new_filename' argument is NULL." );

		return NULL;
	}
	if ( max_filename_length == 0 ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified maximum filename length of %ld is too short "
		"to fit even a 1 byte string into.",
			(long) max_filename_length );
	}

#if SETUP_HOME_VARIABLE
	{
		mx_status_type mx_status;

		if ( mxp_home_variable_set == FALSE ) {
			mx_status = mxp_setup_home_variable();

			if ( mx_status.code != MXE_SUCCESS )
				return NULL;
		}
	}
#endif /* SETUP_HOME_VARIABLE */

	macro_state = MS_NOT_IN_MACRO;

	original_length = strlen( original_filename );

	new_filename[0] = '\0';

	for ( i = 0, j = 0; i < original_length; i++ ) {
		c = original_filename[i];

		switch( macro_state ) {
		case MS_STARTING_MACRO:
			if ( c != '{' ) {
				(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Character '$' in original filename '%s' is "
				"not immediately followed by a '{' character.",
					original_filename );

				return NULL;
			}
			macro_state = MS_IN_MACRO;
			macro_name[0] = '\0';
			break;
		case MS_IN_MACRO:
			if ( c != '}' ) {
				/* Add another letter to the macro name. */
				name_length = strlen(macro_name);

				if ( name_length >= MXU_FILENAME_LENGTH ) {
					(void) mx_error(
						MXE_WOULD_EXCEED_LIMIT, fname,
					"The macro name starting with '%s' "
					"in unexpanded filename '%s' "
					"is longer than the longest allowed "
					"macro name of %d characters.",
						macro_name,
						original_filename,
						MXU_FILENAME_LENGTH );

					return NULL;
				}
				macro_name[name_length] = c;
				macro_name[name_length+1] = '\0';
			} else {
				/* Found the terminating '}' at the end
				 * of the macro name.
				 */

#if MX_CFN_DEBUG
				MX_DEBUG(-2,("%s: Detected macro '%s'",
					fname, macro_name));
#endif

				macro_ptr = getenv( macro_name );

				if ( macro_ptr != NULL ) {
					strlcpy( macro_contents, macro_ptr,
						sizeof(macro_contents) );
				} else {
					/* The environment variable was not
					 * found.  Supply a default value
					 * for MXDIR if it was not found.
					 * Otherwise, we return an error.
					 */

					if (strcmp(macro_name, "MXDIR") == 0) {
						strlcpy( macro_contents,
							MX_CFN_DEFAULT_MXDIR,
							sizeof(macro_contents));
#if MX_CFN_DEBUG
						MX_DEBUG(-2,
					("%s: Providing default MXDIR = '%s'.",
						fname, MX_CFN_DEFAULT_MXDIR));
#endif
					} else {
						(void) mx_error(
							MXE_NOT_FOUND, fname,
						"Environment variable '%s' in "
						"unexpanded filename '%s' "
						"was not found.", macro_name,
							original_filename );

						return NULL;
					}
				}

				macro_length = strlen( macro_contents );

				strlcat( new_filename, macro_contents,
						max_filename_length );

				j += macro_length;

#if MX_CFN_DEBUG
				MX_DEBUG(-2,("%s: Macro '%s' = '%s'",
					fname, macro_name, macro_contents));
#endif

				macro_state = MS_NOT_IN_MACRO;
			}
			break;
		case MS_NOT_IN_MACRO:
			if ( c == '$' ) {
				macro_state = MS_STARTING_MACRO;
			} else {
				if ( j < max_filename_length ) {
					new_filename[j] = c;
					new_filename[j+1] = '\0';

					j++;
				} else {
					(void) mx_error(
						MXE_WOULD_EXCEED_LIMIT, fname,
					"The expanded filename corresponding "
					"to original filename '%s' is too long "
					"to fit into the new filename buffer "
					"of %ld bytes.", original_filename,
						(long) max_filename_length );

					return NULL;
				}
			}
			break;
		default:
			(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"Macro state %d for original filename '%s' is "
				"illegal at i = %ld, j = %ld.",
					macro_state, original_filename,
					(long) i, (long) j );
			break;
		}
	}

#if MX_CFN_DEBUG
	MX_DEBUG(-2,("%s: new_filename = '%s'", fname, new_filename));
#endif

	return new_filename;
}

MX_EXPORT FILE *
mx_cfn_fopen( int filename_type,
		const char *filename,
		const char *file_mode )
{
	mx_status_type mx_status;

	char cfn_filename[ MXU_FILENAME_LENGTH + 50 ];
	FILE *file;

	if ( filename == NULL ) {
		errno = EINVAL;
		return NULL;
	}
	if ( file_mode == NULL ) {
		errno = EINVAL;
		return NULL;
	}

	mx_status = mx_cfn_construct_filename( filename_type, filename,
					cfn_filename, sizeof(cfn_filename) );

	if ( mx_status.code != MXE_SUCCESS ) {
		errno = EINVAL;
		return NULL;
	}

	file = fopen( cfn_filename, file_mode );

	return file;
}

MX_EXPORT mx_status_type
mx_cfn_construct_filename( int filename_type,
			const char *original_filename,
			char *new_filename,
			size_t max_filename_length )
{
	static const char fname[] = "mx_cfn_construct_filename()";

	char filename_buffer1[MXU_FILENAME_LENGTH+20];
	char filename_buffer2[MXU_FILENAME_LENGTH+20];
	char *prefix;

	if ( original_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'original_filename' argument is NULL." );
	}
	if ( new_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'new_filename' argument is NULL." );
	}
	if ( max_filename_length == 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified maximum filename length of %ld is too short "
		"to fit even a 1 byte string into.",
			(long) max_filename_length );
	}

#if MX_CFN_DEBUG
	MX_DEBUG(-2,
	("%s invoked for filename_type = %d, original_filename = '%s'",
		fname, filename_type, original_filename));
#endif

	/* If the original filename already contains an absolute pathname,
	 * then just return the original filename.
	 */

	if ( mx_is_absolute_filename( original_filename ) ) {
		strlcpy( new_filename, original_filename, max_filename_length );

		return MX_SUCCESSFUL_RESULT;
	}

	/* If the original filename starts with a '.' character, then we
	 * assume that it is relative to the current directory of the process
	 * and leave it alone.
	 */

	if ( original_filename[0] == '.' ) {
		strlcpy( new_filename, original_filename, max_filename_length );

		return MX_SUCCESSFUL_RESULT;
	}

	/* Construct the new filename. */

	switch( filename_type ) {
	case MX_CFN_PROGRAM:
		prefix = MX_CFN_PROGRAM_DIR;
		break;
	case MX_CFN_CONFIG:
		prefix = MX_CFN_CONFIG_DIR;
		break;
	case MX_CFN_INCLUDE:
		prefix = MX_CFN_INCLUDE_DIR;
		break;
	case MX_CFN_LIBRARY:
		prefix = MX_CFN_LIBRARY_DIR;
		break;
	case MX_CFN_LOGFILE:
		prefix = MX_CFN_LOGFILE_DIR;
		break;
	case MX_CFN_RUNFILE:
		prefix = MX_CFN_RUNFILE_DIR;
		break;
	case MX_CFN_SYSTEM:
		prefix = MX_CFN_SYSTEM_DIR;
		break;
	case MX_CFN_STATE:
		prefix = MX_CFN_STATE_DIR;
		break;
	case MX_CFN_SCAN:
		prefix = MX_CFN_SCAN_DIR;
		break;
	case MX_CFN_USER:
		prefix = MX_CFN_USER_DIR;
		break;
	case MX_CFN_CWD:
		prefix = MX_CFN_CWD_DIR;
		break;
	case MX_CFN_MODULE:
		prefix = MX_CFN_MODULE_DIR;
		break;
	case MX_CFN_ABSOLUTE:
	default:
		prefix = "";
		break;
	}

	if ( filename_type == MX_CFN_ABSOLUTE ) {
		strlcpy( filename_buffer1, original_filename,
				sizeof(filename_buffer1) );
	} else {
		snprintf( filename_buffer1, sizeof(filename_buffer1),
			"%s/%s", prefix, original_filename );
	}

#if MX_CFN_DEBUG
	MX_DEBUG(-2,("%s:   raw filename = '%s'", fname, filename_buffer1));
#endif

	mx_expand_filename_macros( filename_buffer1,
			filename_buffer2, sizeof(filename_buffer2) );

#if MX_CFN_DEBUG
	MX_DEBUG(-2,("%s:   expanded filename = '%s'",
			fname, filename_buffer2));
#endif

#if 0
	mx_normalize_filename( filename_buffer2,
			filename_buffer1, sizeof(filename_buffer1) );
#else
	mx_canonicalize_filename( filename_buffer2,
			filename_buffer1, sizeof(filename_buffer1) );
#endif

#if MX_CFN_DEBUG
	MX_DEBUG(-2,("%s:   normalized filename = '%s'",
			fname, filename_buffer1));
#endif

	strlcpy( new_filename, filename_buffer1, max_filename_length );

	return MX_SUCCESSFUL_RESULT;
}

