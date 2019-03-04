/*
 * Name:    mx_dictionary.c
 *
 * Purpose: MX dictionaries are a key-value database where the keys are
 *          found in doubly-linked list.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
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
			const char *dictionary_name,
			MX_RECORD *record )
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

	(*new_dictionary)->head_element = NULL;

	(*new_dictionary)->record = record;
	(*new_dictionary)->application_ptr = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

