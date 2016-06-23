/*
 * Name:    h_file.c
 *
 * Purpose: Supports dictionaries that are initialized from a disk file.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_dictionary.h"
#include "h_file.h"

MX_RECORD_FUNCTION_LIST mxh_file_record_function_list = {
	NULL,
	mxh_file_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxh_file_open
};

MX_DICTIONARY_FUNCTION_LIST mxh_file_dictionary_function_list = {
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxh_file_initial_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS
};

long mxh_file_initial_num_record_fields
	= sizeof( mxh_file_initial_field_defaults )
	/ sizeof( mxh_file_initial_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxh_file_initial_rfield_def_ptr =
		&mxh_file_initial_field_defaults[0];

/*------------------------------------------------------------------------*/

static mx_status_type
mxh_file_get_pointers( MX_DICTIONARY *dictionary,
			MX_FILE_DICTIONARY **file_dictionary,
			const char *calling_fname )
{
	static const char fname[] = "mxh_file_get_pointers()";

	if ( dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DICTIONARY pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dictionary->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_DICTIONARY pointer %p "
		"passed by '%s' was NULL.",
			dictionary, calling_fname );
	}
	if ( file_dictionary != (MX_FILE_DICTIONARY **) NULL ) {
		*file_dictionary = (MX_FILE_DICTIONARY *)
			dictionary->record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxh_file_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxh_file_create_record_structures()";

	MX_DICTIONARY *dictionary;
	MX_FILE_DICTIONARY *file_dictionary;

	dictionary = (MX_DICTIONARY *) malloc( sizeof(MX_DICTIONARY) );

	if ( dictionary == (MX_DICTIONARY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_DICTIONARY structure." );
	}

	file_dictionary = (MX_FILE_DICTIONARY *)
			malloc( sizeof(MX_FILE_DICTIONARY) );

	if ( file_dictionary == (MX_FILE_DICTIONARY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_FILE_DICTIONARY structure." );
	}

	dictionary->record = record;
	file_dictionary->record = record;

	record->record_superclass_struct = dictionary;
	record->record_class_struct = NULL;
	record->record_type_struct = file_dictionary;

	record->superclass_specific_function_list =
				&mxh_file_dictionary_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxh_file_open( MX_RECORD *record )
{
	static const char fname[] = "mxh_file_open()";

	MX_DICTIONARY *dictionary = NULL;
	MX_FILE_DICTIONARY *file_dictionary = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	dictionary = (MX_DICTIONARY *) record->record_superclass_struct;

	mx_status = mxh_file_get_pointers(dictionary, &file_dictionary, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

