/*
 * Name:    z_dictionary.c
 *
 * Purpose: Exports MX dictionaries as MX record fields.
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
#include "z_dictionary.h"

MX_RECORD_FUNCTION_LIST mxz_dictionary_record_function_list = {
	NULL,
	mxz_dictionary_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxz_dictionary_open
};

MX_RECORD_FIELD_DEFAULTS mxz_dictionary_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS
};

long mxz_dictionary_num_record_fields
	= sizeof( mxz_dictionary_field_defaults )
	/ sizeof( mxz_dictionary_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxz_dictionary_rfield_def_ptr =
		&mxz_dictionary_field_defaults[0];

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxz_dictionary_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxz_dictionary_create_record_structures()";

	MX_DICTIONARY_RECORD *dictionary_record;

	dictionary_record = (MX_DICTIONARY_RECORD *)
				malloc( sizeof(MX_DICTIONARY_RECORD) );

	if ( dictionary_record == (MX_DICTIONARY_RECORD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for an MX_DICTIONARY_RECORD structure." );
	}

	dictionary_record->record = record;

	record->record_superclass_struct = NULL;
	record->record_class_struct = NULL;
	record->record_type_struct = dictionary_record;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxz_dictionary_open( MX_RECORD *record )
{
	static const char fname[] = "mxz_dictionary_open()";

	MX_DICTIONARY *dictionary = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	dictionary = (MX_DICTIONARY *) record->record_superclass_struct;

	return MX_SUCCESSFUL_RESULT;
}

