/*
 * Name:    v_indirect_string.c
 *
 * Purpose: Support for a string constructed from record field values.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "v_indirect_string.h"

MX_RECORD_FUNCTION_LIST mxv_indirect_string_record_function_list = {
	mxv_indirect_string_initialize_driver,
	mxv_indirect_string_create_record_structures,
	NULL,
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
	return MX_SUCCESSFUL_RESULT;
}

