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

#define MXZ_DICTIONARY_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_process.h"
#include "mx_dictionary.h"
#include "z_dictionary.h"

MX_RECORD_FUNCTION_LIST mxz_dictionary_record_function_list = {
	NULL,
	mxz_dictionary_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxz_dictionary_open,
	NULL,
	NULL,
	NULL,
	mxz_dictionary_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxz_dictionary_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXZ_DICTIONARY_STANDARD_FIELDS
};

long mxz_dictionary_num_record_fields
	= sizeof( mxz_dictionary_field_defaults )
	/ sizeof( mxz_dictionary_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxz_dictionary_rfield_def_ptr =
		&mxz_dictionary_field_defaults[0];

/*----*/

static mx_status_type mxz_dictionary_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

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
	dictionary_record->dictionary = NULL;

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

	MX_DICTIONARY_RECORD *dictionary_record = NULL;
	char *arguments_copy = NULL;
	char *colon_ptr = NULL;
	char *dictionary_type_name = NULL;
	char *dictionary_type_arguments = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	dictionary_record = (MX_DICTIONARY_RECORD *) record->record_type_struct;

	if ( dictionary_record == (MX_DICTIONARY_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DIRECTORY_RECORD pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mx_dictionary_create( &(dictionary_record->dictionary),
								record->name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	arguments_copy = strdup( dictionary_record->arguments );

	if ( arguments_copy == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to copy the arguments for "
		"dictionary record '%s'.", record->name );
	}

	dictionary_type_name = arguments_copy;

	/* Search for the first colon ':' character which separates
	 * the description of the dictionary type from the arguments
	 * used by the dictionary type.
	 */

	colon_ptr = strchr( arguments_copy, ':' );

	if ( colon_ptr == NULL ) {
		dictionary_type_arguments = NULL;
	} else {
		dictionary_type_arguments = colon_ptr + 1;
		*colon_ptr = '\0';
	}

#if MXZ_DICTIONARY_DEBUG
	MX_DEBUG(-2,("%s: dictionary_type_name = '%s'",
		fname, dictionary_type_name));
	MX_DEBUG(-2,("%s: dictionary_type_arguments = '%s'",
		fname, dictionary_type_arguments));
#endif

	if ( strcmp( dictionary_type_name, "file" ) == 0 ) {
		mx_status = mx_dictionary_read_file(
				dictionary_record->dictionary,
				dictionary_type_arguments );
	} else {
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized dictionary type '%s' requested.",
			dictionary_type_name );
	}

	mx_free( arguments_copy );

	return mx_status;
}

MX_EXPORT mx_status_type
mxz_dictionary_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_DICTIONARY_SHOW_DICTIONARY:
			record_field->process_function
					= mxz_dictionary_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

static mx_status_type
mxz_dictionary_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxz_dictionary_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_DICTIONARY_RECORD *dictionary_record;
	MX_DICTIONARY *dictionary;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	dictionary_record = (MX_DICTIONARY_RECORD *) record->record_type_struct;
	dictionary = dictionary_record->dictionary;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_DICTIONARY_SHOW_DICTIONARY:
			mx_status = mx_dictionary_show_dictionary( dictionary );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

