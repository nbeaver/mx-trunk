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

#define MX_DICTIONARY_DEBUG			FALSE

#define MX_DICTIONARY_DEBUG_VALUE_ARRAY		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_cfn.h"
#include "mx_array.h"
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
mx_dictionary_show_dictionary( MX_DICTIONARY *dictionary )
{
	static const char fname[] = "mx_dictionary_show_dictionary()";

	long i, j, num_allocated_keys;
	long mx_datatype, num_dimensions, array_index;
	char **key_array;
	void **value_array;
	MX_ARRAY_HEADER_WORD_TYPE *header;

	if ( dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dictionary pointer passed was NULL." );
	}

	num_allocated_keys = dictionary->num_allocated_keys;

	fprintf( stderr, "Dictionary: '%s', num_allocated_keys = %ld\n",
		dictionary->name, num_allocated_keys );

	key_array = dictionary->key_array;

	if ( key_array == (char **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The key_array pointer for dictionary '%s' is NULL.",
			dictionary->name );
	}

	value_array = dictionary->value_array;

	if ( value_array == (void **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The value_array pointer for dictionary '%s' is NULL.",
			dictionary->name );
	}

	for ( i = 0; i < num_allocated_keys; i++ ) {
		if ( value_array[i] == NULL ) {
			fprintf( stderr, "key[%ld] = '%s', value = NULL\n",
				i, key_array[i] );
		} else {
			header = (MX_ARRAY_HEADER_WORD_TYPE *) value_array[i];

			mx_datatype = header[MX_ARRAY_OFFSET_MX_DATATYPE];
			num_dimensions = header[MX_ARRAY_OFFSET_NUM_DIMENSIONS];

			fprintf( stderr,
	    "key[%ld] = '%s', mx_datatype = '%s' (%ld), num_dimensions = %ld",
			    i, key_array[i],
			    mx_get_datatype_name_from_datatype( mx_datatype ),
		    	    mx_datatype,
			    num_dimensions );

			if ( num_dimensions > 0 ) {
			    array_index = MX_ARRAY_OFFSET_NUM_DIMENSIONS - 1;

			    fprintf( stderr, ", array = (%ld",
				(long) header[array_index] );

			    for ( j = 1; j < num_dimensions; j++ ) {
				array_index--;

				fprintf( stderr, ",%ld",
					(long) header[array_index] );
			    }
			    fprintf( stderr, ")" );
			}
			fprintf( stderr, "\n" );
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
	char *buffer_copy = NULL;
	char *ptr = NULL;
	char *string_ptr = NULL;
	char *key_ptr = NULL;
	char *datatype_ptr = NULL;
	char *dimension_ptr = NULL;
	char *value_ptr = NULL;
	int saved_errno, split_status;
	int dimension_argc;
	char **dimension_argv;
	long new_datatype;
	size_t new_num_lines_in_file;
	long i, num_keys_in_use, new_num_keys;
	long new_key_dimension[2];
	size_t new_key_sizeof[2];
	char **new_key_array;
	void **new_value_array;
	long n, value_num_dimensions;
	long *value_dimension_array;
	size_t *value_sizeof_array;
	uint32_t first_uint32;
	mx_status_type mx_status;

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s invoked", fname));
#endif

	if ( dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY pointer passed was NULL." );
	}
	if ( dictionary_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dictionary_filename pointer passed was NULL." );
	}

	/* FIXME: Some, but not all, of the work has been done to make
	 * it possible to load a new dictionary file into a dictionary
	 * that already has been created.  But this is not yet complete.
	 */

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
	/* Allocate memory for the new key and value arrays. */

	new_num_keys = num_keys_in_use + new_num_lines_in_file;

	new_key_dimension[0] = new_num_keys;
	new_key_dimension[1] = MXU_DICTIONARY_KEY_LENGTH+1;

	new_key_sizeof[0] = sizeof(char);
	new_key_sizeof[1] = sizeof(char *);

	new_key_array = mx_allocate_array( MXFT_STRING,
					2, new_key_dimension,
					new_key_sizeof );

	if ( new_key_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of dictionary keys.", new_num_keys );
	}

	new_value_array = calloc( new_num_keys, sizeof(void *) );

	if ( new_value_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of value pointers.", new_num_keys );
	}

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: new_key_array = %p", fname, new_key_array));
#endif

#if MX_DICTIONARY_DEBUG_VALUE_ARRAY
	MX_DEBUG(-2,("%s: new_value_array = %p", fname, new_value_array));
#endif

	/* Now try to open the file and read in the dictionary definitions. */

	file = fopen( cfn_filename, "r" );

	if ( file == (FILE *) NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to open the file '%s'.", dictionary_filename );
	}

	/* We do not directly increment i in the for() loop before, since
	 * we may want to skip over comment lines, etc.
	 */

	for ( i = 0; i < new_num_keys;  ) {

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: i = %ld", fname, i));
#endif
		/*----------------------------------------------------------*/

		/* Read in a key-value definition from the file. */

		memset( buffer, 0, sizeof(buffer) );

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
		buffer_copy = strdup( buffer );

		if ( buffer_copy == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to make a duplicate of "
			"the buffer '%s'.", buffer );
		}

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: buffer_copy = '%s'", fname, buffer_copy));
#endif
		/*----------------------------------------------------------*/

		string_ptr = buffer_copy;

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: #1 string_ptr = '%s'", fname, string_ptr));
#endif

		/* The 'key' name should be the first thing in the buffer. */

		key_ptr = mx_string_token( &string_ptr, ":" );

		if ( key_ptr == NULL ) {
			mx_status = mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Could not find a key name in buffer '%s'.", buffer );

			mx_free(buffer_copy);
			return mx_status;
		}

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: key_ptr = '%s'", fname, key_ptr));
#endif

		if ( string_ptr == NULL ) {
			mx_status = mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Key name '%s' in buffer '%s' is not followed "
			"by a datatype name.", key_ptr, buffer );

			mx_free(buffer_copy);
			return mx_status;
		}

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: #2 string_ptr = '%s'", fname, string_ptr));
#endif
		/* The next field is the datatype, like 'float' or 'string'. */

		datatype_ptr = mx_string_token( &string_ptr, ":" );

		if ( datatype_ptr == NULL ) {
			mx_status = mx_error( MXE_UNPARSEABLE_STRING, fname,
		    "Could not find a datatype name in buffer '%s'.", buffer );

			mx_free(buffer_copy);
			return mx_status;
		}

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: datatype_ptr = '%s'", fname, datatype_ptr));
#endif

		if ( string_ptr == NULL ) {
			mx_status = mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Datatype name '%s' in buffer '%s' is not followed "
			"by a dimension description.", datatype_ptr, buffer );

			mx_free(buffer_copy);
			return mx_status;
		}

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: #3 string_ptr = '%s'", fname, string_ptr));
#endif
		/* The next field contains the dimensionality of the field
		 * that we are creating.
		 */

		dimension_ptr = mx_string_token( &string_ptr, ":" );

		if ( dimension_ptr == NULL ) {
			mx_status = mx_error( MXE_UNPARSEABLE_STRING, fname,
		    "Could not find a dimension name in buffer '%s'.", buffer );

			mx_free(buffer_copy);
			return mx_status;
		}

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: dimension_ptr = '%s'", fname, dimension_ptr));
#endif

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: #4 string_ptr = '%s'", fname, string_ptr));
#endif
		/* The last field is the value field, which may not be
		 * present at all (i.e. set to NULL).  If it is not
		 * present, the value is set some datatype-dependent
		 * version of 0.
		 */

		value_ptr = string_ptr;

#if MX_DICTIONARY_DEBUG
		MX_DEBUG(-2,("%s: value_ptr = '%s'", fname, value_ptr));
#endif
		/*----------------------------------------------------------*/

		/* Put the new key name into the dictionary. */

		strlcpy( new_key_array[i], key_ptr, MXU_DICTIONARY_KEY_LENGTH );

		/* Get the MX datatype of this key. */

		new_datatype =
			mx_get_datatype_from_datatype_name( datatype_ptr );

		if ( new_datatype < 0 ) {
			mx_free(buffer_copy);

			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Key '%s' specified by line '%s' has an "
			"illegal datatype.", key_ptr, buffer );
		}

		/* Figure out the dimensionality of this new key. */

		split_status = mx_string_split( dimension_ptr, " ",
					&dimension_argc, &dimension_argv );

		if ( split_status < 0 ) {
			saved_errno = errno;
			mx_free(buffer_copy);

			return mx_error( MXE_FUNCTION_FAILED, fname,
			"The attempt to split the dimension string in "
			"'%s' failed.  errno = %d, error message = '%s'.",
				buffer, saved_errno, strerror(saved_errno) );
		}

		value_num_dimensions = dimension_argc;

		value_dimension_array = (long *)
				malloc( value_num_dimensions * sizeof(long) );

		if ( value_dimension_array == (long *) NULL ) {
			mx_free(dimension_argv);
			mx_free(buffer_copy);

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld element "
			"dimension array for the value array in the "
			"dictionary file line '%s'.",
				value_num_dimensions, buffer );
		}

		for ( n = 0; n < value_num_dimensions; n++ ) {

#if MX_DICTIONARY_DEBUG
			MX_DEBUG(-2,("%s: dimension_argv[%ld] = '%s'",
				fname, n, dimension_argv[n] ));
#endif
			value_dimension_array[n] = atol( dimension_argv[n] );
		}

		mx_status = mx_get_datatype_sizeof_array( new_datatype,
							&value_sizeof_array );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free(value_dimension_array);
			mx_free(dimension_argv);
			mx_free(buffer_copy);
			return mx_status;
		}

		/* Allocate memory for the value array. */

		new_value_array[i] = mx_allocate_array( new_datatype,
							value_num_dimensions,
							value_dimension_array,
							value_sizeof_array );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free(value_dimension_array);
			mx_free(dimension_argv);
			mx_free(buffer_copy);
			return mx_status;
		}

		first_uint32 = ((uint32_t *) new_value_array[i])[0];

#if MX_DICTIONARY_DEBUG_VALUE_ARRAY
		MX_DEBUG(-2,("%s: new_value_array[%ld] = %p",
			fname, i, new_value_array[i]));
		MX_DEBUG(-2,("%s: #1 first_uint32 = %#x", fname, first_uint32));
#endif
		/*----------------------------------------------------------*/

		/* If value_ptr is not NULL, then we must transform the
		 * ASCII text description of the array contents into
		 * new_value_array[i];
		 */

		if ( value_ptr == NULL ) {
			((char *) new_value_array[i])[0] = '\0';
		} else {
			mx_status = mx_copy_ascii_buffer_to_mx_array(
						value_ptr,
						MXU_DICTIONARY_KEY_LENGTH,
						new_datatype,
						new_value_array[i],
						NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

#if MX_DICTIONARY_DEBUG_VALUE_ARRAY
		MX_DEBUG(-2,("%s: &((uint32_t *) new_value_array[%lu])[0] = %p",
			fname, i, &((uint32_t *) new_value_array[i])[0]));
#endif

		first_uint32 = ((uint32_t *) new_value_array[i])[0];

#if MX_DICTIONARY_DEBUG_VALUE_ARRAY
		MX_DEBUG(-2,("%s: #2 first_uint32 = %#lx",
			fname, (unsigned long) first_uint32));
#endif
		/*----------------------------------------------------------*/

		mx_free(value_dimension_array);
		mx_free(dimension_argv);
		mx_free(buffer_copy);

		/* Increment the value array counter to get the next line. */

		i++;
	}

	/* If we get here, we discard the old key and value arrays and
	 * replace them with new key and value arrays.
	 */

	/* FIXME: Not doing this yet. */

	/* Install the key and value arrays we just created into
	 * the dictionary data structure.
	 */

#if MX_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: DONE: Saving new dictionary contents.",fname));
#endif

	dictionary->num_allocated_keys = new_num_keys;
	dictionary->key_array = new_key_array;
	dictionary->value_array = new_value_array;

	/* We are done now, so return. */

	return MX_SUCCESSFUL_RESULT;
}

