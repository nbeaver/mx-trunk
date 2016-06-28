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
#include "mx_dictionary.h"

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

	(*new_dictionary)->num_keys = 0;
	(*new_dictionary)->key_array = NULL;
	(*new_dictionary)->value_array = NULL;

	(*new_dictionary)->application_ptr = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dictionary_read_file( MX_DICTIONARY *dictionary,
			const char *dictionary_filename )
{
#if MX_DICTIONARY_DEBUG
	static const char fname[] = "mx_dictionary_read_file()";

	MX_DEBUG(-2,("%s: dictionary_filename = '%s'",
		fname, dictionary_filename));
#endif
	return MX_SUCCESSFUL_RESULT;
}

