/*
 * Name:    v_indirect_string.c
 *
 * Purpose: Support for a string constructed from record field values.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXV_INDIRECT_STRING_DEBUG		FALSE

#define MXV_INDIRECT_STRING_DEBUG_VALUE		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_variable.h"
#include "v_indirect_string.h"

MX_RECORD_FUNCTION_LIST mxv_indirect_string_record_function_list = {
	mxv_indirect_string_initialize_driver,
	mxv_indirect_string_create_record_structures,
	mxv_indirect_string_finish_record_initialization,
	NULL,
	NULL,
	mx_receive_variable
};

MX_VARIABLE_FUNCTION_LIST mxv_indirect_string_variable_function_list = {
	mxv_indirect_string_send_variable,
	mxv_indirect_string_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_indirect_string_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_INDIRECT_STRING_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_indirect_string_num_record_fields
	= sizeof( mxv_indirect_string_record_field_defaults )
	/ sizeof( mxv_indirect_string_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_indirect_string_rfield_def_ptr
		= &mxv_indirect_string_record_field_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxv_indirect_string_initialize_driver( MX_DRIVER *driver )
{
        MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
        long num_fields_varargs_cookie;
        mx_status_type mx_status;

	mx_status = mx_variable_initialize_driver( driver );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

        mx_status = mx_find_record_field_defaults_index( driver,
                        			"num_fields",
						&referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				&num_fields_varargs_cookie);

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

	mx_status = mx_find_record_field_defaults( driver,
						"record_field_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_fields_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_indirect_string_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_indirect_string_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_INDIRECT_STRING *indirect_string;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	indirect_string = (MX_INDIRECT_STRING *)
				malloc( sizeof(MX_INDIRECT_STRING) );

	if ( indirect_string == (MX_INDIRECT_STRING *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_INDIRECT_STRING structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = indirect_string;

	record->superclass_specific_function_list =
				&mxv_indirect_string_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_indirect_string_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_indirect_string_finish_record_initialization()";

	MX_RECORD_FIELD *string_value_field;

	MX_RECORD_FIELD *referenced_field;
	void *referenced_field_value_pointer;

	MX_INDIRECT_STRING *indirect_string;
	long i, num_fields;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	indirect_string = (MX_INDIRECT_STRING *) record->record_type_struct;

	if ( indirect_string == (MX_INDIRECT_STRING *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INDIRECT_STRING pointer for record '%s' is NULL.",
			record->name );
	}

	if ( indirect_string->record_field_array == (MX_RECORD_FIELD **) NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The record_field_array pointer for record '%s' is NULL.",
			record->name );
	}

	/* Get a pointer to the string field value. */

	mx_status = mx_find_record_field( record, "value",
					&string_value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	indirect_string->string_value_pointer
		= mx_get_field_value_pointer( string_value_field );

	/* Create and populate the referenced_value_array with value
	 * pointers from the record_field_array.
	 */

	num_fields = indirect_string->num_fields;

	indirect_string->referenced_value_array =
				malloc( num_fields * sizeof(void *) );

	if ( indirect_string->referenced_value_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"referenced_value_array for record '%s'.",
			num_fields, record->name );
	}

	for ( i = 0; i < num_fields; i++ ) {
		referenced_field = indirect_string->record_field_array[i];

		if ( referenced_field == (MX_RECORD_FIELD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Element %ld of the record_field_array for "
			"record '%s' is NULL.", i, record->name );
		}

		if ( referenced_field->record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Element %ld of the record_field_array for "
			"record '%s' has a NULL MX_RECORD pointer.",
				i, record->name );
		}

		referenced_field_value_pointer =
			mx_get_field_value_pointer( referenced_field );

		if ( referenced_field_value_pointer == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The field value pointer for record field '%s.%s' "
			"used by record '%s' is NULL.",
				referenced_field->record->name,
				referenced_field->name,
				record->name );
		}

		indirect_string->referenced_value_array[i]
					= referenced_field_value_pointer;

#if MXV_INDIRECT_STRING_DEBUG
		MX_DEBUG(-2,("%s: array[i] = %p, ptr = %p", fname,
			indirect_string->referenced_value_array[i],
			referenced_field_value_pointer));
#endif
	}

#if MXV_INDIRECT_STRING_DEBUG
	MX_DEBUG(-2,("%s: indirect_string = %p, referenced_value_array = %p",
		fname, indirect_string,
		indirect_string->referenced_value_array));

	for ( i = 0; i < num_fields; i++ ) {
		MX_DEBUG(-2,("%s: referenced_value_array[%ld] = %p",
		fname, i, indirect_string->referenced_value_array[i]));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_indirect_string_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_indirect_string_send_variable()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Writing to indirect string record '%s' is not supported.",
		variable->record->name );
}

MX_EXPORT mx_status_type
mxv_indirect_string_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_indirect_string_receive_variable()";

	MX_RECORD *record;
	MX_RECORD_FIELD *referenced_field;
	MX_INDIRECT_STRING *indirect_string;
	long i, bytes_written;
	mx_status_type mx_status;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed was NULL." );
	}

	record = variable->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for variable %p is NULL.",
			variable );
	}

	indirect_string = (MX_INDIRECT_STRING *) record->record_type_struct;

	if ( indirect_string == (MX_INDIRECT_STRING *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INDIRECT_STRING pointer for variable '%s' is NULL.",
			record->name );
	}

	/* Make sure all of the referenced fields have been processed. */

	for ( i = 0; i < indirect_string->num_fields; i++ ) {
		referenced_field = indirect_string->record_field_array[i];

#if MXV_INDIRECT_STRING_DEBUG
		MX_DEBUG(-2,("%s: field[%ld] = '%s.%s', value_ptr = %p",
			fname, i, referenced_field->record->name,
			referenced_field->name,
			mx_get_field_value_pointer( referenced_field ) ));
#endif

		mx_status = mx_process_record_field( referenced_field->record,
							referenced_field,
							MX_PROCESS_GET, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0 && MXV_INDIRECT_STRING_DEBUG_VALUE
		MX_DEBUG(-2,("%s: field[%ld] = '%s.%s', last_value = %f",
			fname, i, referenced_field->record->name,
			referenced_field->name,
			referenced_field->last_value));
#endif
	}

	/* Now write the value to the string value pointer. */

	bytes_written = mx_snprintf_from_pointer_array( 
				indirect_string->string_value_pointer,
				variable->dimension[0],
				indirect_string->format,
				indirect_string->num_fields,
				indirect_string->referenced_value_array );

	MXW_SUPPRESS_SET_BUT_NOT_USED( bytes_written );

#if MXV_INDIRECT_STRING_DEBUG_VALUE
	MX_DEBUG(-2,("%s: string = '%s'",
		fname, indirect_string->string_value_pointer));
#endif

	return MX_SUCCESSFUL_RESULT;
}

