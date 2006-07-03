/*
 * Name:     mx_cfn.c
 *
 * Purpose:  Functions for constructing MX control system filenames.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_CFN_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_cfn.h"
#include "mx_cfn_defaults.h"

/*---*/

#if defined(OS_WIN32)

#error Win32 support not yet implemented.

#elif defined(OS_VMS)

MX_EXPORT mx_bool_type
mx_is_absolute_filename( char *filename )
{
	return TRUE;
}

MX_EXPORT char *
mx_normalize_filename( char *original_filename,
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

	/* Currently, for this platform we do not perform any
	 * real normalization.  This probably means that using
	 * the default file locations will not work here.
	 */

	strlcpy( new_filename, original_filename, max_filename_length );

	return new_filename;
}

#else

/* Most other platforms can handle Posix style filenames. */

MX_EXPORT mx_bool_type
mx_is_absolute_filename( char *filename )
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

MX_EXPORT char *
mx_normalize_filename( char *original_filename,
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

	/* For Posix filenames, all we do here is convert multiple '/'
	 * characters in a row to just a single '/' character.
	 */
	
	original_length = strlen(original_filename);

	slash_seen = FALSE;

	for( i = 0, j = 0; i < original_length; i++ ) {
		c = original_filename[i];

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

#define MS_NOT_IN_MACRO		1
#define MS_STARTING_MACRO	2
#define MS_IN_MACRO		3

MX_EXPORT char *
mx_expand_filename_macros( char *original_filename,
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

MX_EXPORT char *
mx_construct_control_system_filename( int filename_type,
					char *original_filename,
					char *new_filename,
					size_t max_filename_length )
{
	static const char fname[] = "mx_construct_control_system_filename()";

	char filename_buffer1[MXU_FILENAME_LENGTH+20];
	char filename_buffer2[MXU_FILENAME_LENGTH+20];
	char *prefix;

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

		return &new_filename[0];
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

	mx_normalize_filename( filename_buffer2,
			filename_buffer1, sizeof(filename_buffer1) );

#if MX_CFN_DEBUG
	MX_DEBUG(-2,("%s:   normalized filename = '%s'",
			fname, filename_buffer1));
#endif

	strlcpy( new_filename, filename_buffer1, max_filename_length );

	return new_filename;
}

