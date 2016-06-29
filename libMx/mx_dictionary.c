/*
 * Name:    mx_dictionary.c
 *
 * Purpose: MX dictionaries are a key-value database where the keys are
 *          found in an array of string names.  The values are pointed
 *          to by an array of void pointers, where the actual values are
 *          MX arrays which are each allocated by the mx_allocate_array()
 *          functions.
 *
 * Note:    The current implementation of MX dictionaries has not been
 *          optimized for speed.  It anticipates that most key-value
 *          pairs will be initialized at program startup time.
 *
 * Warning: The values (including 1-dimensional values) _must_ have been
 *          allocated by mx_allocate_array().  If you put an array that
 *          you allocated directly using malloc() or calloc(), then at
 *          best you will get an error message.  At worst, it can crash
 *          your program, since MX dictionaries make use of the metadata
 *          created by mx_allocate_array() that describes the datatype and
 *          dimensions of the array.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_DICTIONARY_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_cfn.h"
#include "mx_dictionary.h"

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_dictionary_create( MX_DICTIONARY **new_dictionary,
			const char *dictionary_name )
{
	static const char fname[] = "mx_dictionary_create()";

	if ( new_dictionary == (MX_DICTIONARY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY pointer passed was NULL." );
	}

	*new_dictionary = (MX_DICTIONARY *) malloc( sizeof(MX_DICTIONARY) );

	if ( *new_dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt to create a new MX_DICTIONARY structure failed." );
	}

	strlcpy( (*new_dictionary)->name,
		dictionary_name,
		sizeof( (*new_dictionary)->name ) );

	(*new_dictionary)->num_allocated_keys = 0;
	(*new_dictionary)->key_array = NULL;
	(*new_dictionary)->value_array = NULL;

	(*new_dictionary)->application_ptr = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_dictionary_get_num_keys_in_use( MX_DICTIONARY *dictionary,
					long *num_keys_in_use )
{
	static const char fname[] = "mx_dictionary_get_num_keys_in_use()";

	int i;

	if ( dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY pointer passed was NULL." );
	}
	if ( num_keys_in_use == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_keys_in_use pointer passed was NULL." );
	}

	*num_keys_in_use = 0;

	for ( i = 0; i < dictionary->num_allocated_keys; i++ ) {
		if ( dictionary->key_array[i] != NULL ) {
			if ( strlen(dictionary->key_array[i]) > 0 ) {
				(*num_keys_in_use)++;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_dictionary_read_file( MX_DICTIONARY *dictionary,
			const char *dictionary_filename )
{
	static const char fname[] = "mx_dictionary_read_file()";

	FILE *file;
	char cfn_filename[MXU_FILENAME_LENGTH+1];
	char buffer[1000];
	char *ptr = NULL;
	size_t new_num_lines_in_file;
	long num_keys_in_use;
	mx_status_type mx_status;

	if ( dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY pointer passed was NULL." );
	}
	if ( dictionary_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dictionary_filename pointer passed was NULL." );
	}

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: dictionary_filename = '%s'",
		fname, dictionary_filename));
#endif
	/* Figure out how many lines of text are in the file
	 * after we find out the full name of the file.
	 *
	 * A key specified in the file we are reading may match an
	 * existing key in the dictionary.  If so, we replace the
	 * existing key value with the new key value.  If the new
	 * key value has a different datatype and dimensions than
	 * the existing key value, then we just ignore the new
	 * version.
	 */

	mx_status = mx_cfn_construct_filename( MX_CFN_CONFIG,
						dictionary_filename,
						cfn_filename,
						sizeof(cfn_filename) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: cfn_filename = '%s'", fname, cfn_filename));
#endif

	/* The net result is that the number of lines in the new file
	 * reported by mx_get_num_lines_in_file() may be an overestimate
	 * of how many lines we need to add to the dictionary.
	 */

	mx_status = mx_get_num_lines_in_file( cfn_filename,
						&new_num_lines_in_file );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_error( mx_status.code, mx_status.location,
				mx_status.message );

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: new_num_lines_in_file = %ld",
		fname, (long) new_num_lines_in_file));
#endif

	/* We need to figure out how many entries are actually being
	 * used by the existing key table.
	 */

	mx_status = mx_dictionary_get_num_keys_in_use( dictionary,
							&num_keys_in_use );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: num_keys_in_use = %ld", fname, num_keys_in_use));
#endif

	/* Now try to open the file and read in the dictionary definitions. */

	file = fopen( cfn_filename, "r" );

	if ( file == (FILE *) NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to open the file '%s'.", dictionary_filename );
	}

	while (1) {
		ptr = mx_fgets( buffer, sizeof(buffer), file );

		if ( ptr == NULL ) {
			if ( feof(file) ) {
				(void) fclose(file);
				return MX_SUCCESSFUL_RESULT;
			}
			if ( ferror(file) ) {
				(void) fclose(file);
				return mx_error( MXE_FILE_IO_ERROR, fname,
				"An error occurred while reading from "
				"file '%s'.", cfn_filename );
			} else {
				return mx_error( MXE_UNKNOWN_ERROR, fname,
				"A NULL pointer was returned by mx_fgets() "
				"for file '%s', but EOF did not occur "
				"and ferror() reported no errors.",
					cfn_filename );
			}
		}
#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

