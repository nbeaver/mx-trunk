/*
 * Name:    v_fix_regions.c
 *
 * Purpose: Support for setting up the fixing of image regions in
 *          an area detector image.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXV_FIX_REGIONS_DEBUG		FALSE

#define MXV_FIX_REGIONS_DEBUG_VALUE	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_variable.h"
#include "v_fix_regions.h"

MX_RECORD_FUNCTION_LIST mxv_fix_regions_record_function_list = {
	mxv_fix_regions_initialize_driver,
	mxv_fix_regions_create_record_structures,
	mxv_fix_regions_finish_record_initialization,
};

MX_VARIABLE_FUNCTION_LIST mxv_fix_regions_variable_function_list = {
	mxv_fix_regions_send_variable,
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_fix_regions_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_FIX_REGIONS_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_fix_regions_num_record_fields
	= sizeof( mxv_fix_regions_record_field_defaults )
	/ sizeof( mxv_fix_regions_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_fix_regions_rfield_def_ptr
		= &mxv_fix_regions_record_field_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxv_fix_regions_initialize_driver( MX_DRIVER *driver )
{
        MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
        long num_fix_regions_varargs_cookie;
        mx_status_type mx_status;

	mx_status = mx_variable_initialize_driver( driver );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

        mx_status = mx_find_record_field_defaults_index( driver,
                        			"num_fix_regions",
						&referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				&num_fix_regions_varargs_cookie);

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

	mx_status = mx_find_record_field_defaults( driver,
						"fix_region_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_fix_regions_varargs_cookie;
	field->dimension[1] = 5;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_fix_regions_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_fix_regions_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_FIX_REGIONS *fix_regions_struct;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_VARIABLE structure." );
	}

	fix_regions_struct = (MX_FIX_REGIONS *) malloc( sizeof(MX_FIX_REGIONS));

	if ( fix_regions_struct == (MX_FIX_REGIONS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_FIX_REGIONS structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = fix_regions_struct;

	record->superclass_specific_function_list =
				&mxv_fix_regions_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_fix_regions_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_fix_regions_finish_record_initialization()";

	MX_VARIABLE *variable;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_fix_regions_send_variable( variable );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_fix_regions_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_fix_regions_send_variable()";

	MX_FIX_REGIONS *fix_regions_struct = NULL;
	MX_RECORD_FIELD *string_value_field = NULL;
	const char **string_value_pointer = NULL;
	long i;
	mx_status_type mx_status;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed was NULL." );
	}

	fix_regions_struct = (MX_FIX_REGIONS *)
			variable->record->record_type_struct;

	if ( fix_regions_struct == (MX_FIX_REGIONS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_FIX_REGIONS pointer for record '%s' is NULL.",
			variable->record->name );
	}

	/* Get a pointer to the string field value. */

	mx_status = mx_find_record_field( variable->record, "value",
					&string_value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	string_value_pointer = mx_get_field_value_pointer( string_value_field );

	for ( i = 0; i < fix_regions_struct->num_fix_regions; i++ ) {
		MX_DEBUG(-2,("%s: region [%ld] = '%s'",
		fname, i, string_value_pointer[i] ));
	}

	/* FIXME: Fill in the rest of this driver method. */

	return MX_SUCCESSFUL_RESULT;
}

