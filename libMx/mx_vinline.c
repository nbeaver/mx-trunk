/*
 * Name:    mx_vinline.c
 *
 * Purpose: Support for inline variables in MX database description files.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "mx_record.h"
#include "mx_variable.h"
#include "mx_vinline.h"

MX_RECORD_FUNCTION_LIST mxv_inline_variable_record_function_list = {
	mxv_inline_variable_initialize_type,
	mxv_inline_variable_create_record_structures,
	mxv_inline_variable_finish_record_initialization,
	mxv_inline_variable_delete_record,
	NULL,
	mxv_inline_variable_dummy_function,
	mxv_inline_variable_dummy_function,
	mxv_inline_variable_dummy_function,
	mxv_inline_variable_dummy_function
};

/********************************************************************/

MX_RECORD_FIELD_DEFAULTS mxv_inline_string_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_string_variable_num_record_fields
			= sizeof( mxv_inline_string_variable_defaults )
			/ sizeof( mxv_inline_string_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_string_variable_def_ptr
			= &mxv_inline_string_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_char_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_CHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_char_variable_num_record_fields
			= sizeof( mxv_inline_char_variable_defaults )
			/ sizeof( mxv_inline_char_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_char_variable_def_ptr
			= &mxv_inline_char_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_uchar_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UCHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_uchar_variable_num_record_fields
			= sizeof( mxv_inline_uchar_variable_defaults )
			/ sizeof( mxv_inline_uchar_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_uchar_variable_def_ptr
			= &mxv_inline_uchar_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_short_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_SHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_short_variable_num_record_fields
			= sizeof( mxv_inline_short_variable_defaults )
			/ sizeof( mxv_inline_short_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_short_variable_def_ptr
			= &mxv_inline_short_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_ushort_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_USHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_ushort_variable_num_record_fields
			= sizeof( mxv_inline_ushort_variable_defaults )
			/ sizeof( mxv_inline_ushort_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_ushort_variable_def_ptr
			= &mxv_inline_ushort_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_int_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_INT_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_int_variable_num_record_fields
			= sizeof( mxv_inline_int_variable_defaults )
			/ sizeof( mxv_inline_int_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_int_variable_def_ptr
			= &mxv_inline_int_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_uint_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UINT_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_uint_variable_num_record_fields
			= sizeof( mxv_inline_uint_variable_defaults )
			/ sizeof( mxv_inline_uint_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_uint_variable_def_ptr
			= &mxv_inline_uint_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_long_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_LONG_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_long_variable_num_record_fields
			= sizeof( mxv_inline_long_variable_defaults )
			/ sizeof( mxv_inline_long_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_long_variable_def_ptr
			= &mxv_inline_long_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_ulong_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_ULONG_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_ulong_variable_num_record_fields
			= sizeof( mxv_inline_ulong_variable_defaults )
			/ sizeof( mxv_inline_ulong_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_ulong_variable_def_ptr
			= &mxv_inline_ulong_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_float_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_FLOAT_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_float_variable_num_record_fields
			= sizeof( mxv_inline_float_variable_defaults )
			/ sizeof( mxv_inline_float_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_float_variable_def_ptr
			= &mxv_inline_float_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_double_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_double_variable_num_record_fields
			= sizeof( mxv_inline_double_variable_defaults )
			/ sizeof( mxv_inline_double_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_double_variable_def_ptr
			= &mxv_inline_double_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_inline_record_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_RECORD_VARIABLE_STANDARD_FIELDS
};

long mxv_inline_record_variable_num_record_fields
			= sizeof( mxv_inline_record_variable_defaults )
			/ sizeof( mxv_inline_record_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_inline_record_variable_def_ptr
			= &mxv_inline_record_variable_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxv_inline_variable_initialize_type( long record_type )
{
	return mx_variable_initialize_type( record_type );
}

MX_EXPORT mx_status_type
mxv_inline_variable_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxv_inline_variable_create_record_structures()";

	MX_VARIABLE *variable_struct;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCAN structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = NULL;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

#if 1
#include "mx_driver.h"
#include "mx_array.h"
#endif

MX_EXPORT mx_status_type
mxv_inline_variable_finish_record_initialization( MX_RECORD *record )
{
	/* Not needed by this variable class. */

#if 0
	/* The following is debugging code for testing string variables.
	 * It is not normally enabled.
	 */

	const char fname[] =
		"mxv_inline_variable_finish_record_initialization()";

	MX_RECORD_FIELD *value_field;
	long num_dimensions;
	long *dimension;
	long i;
	char *data_ptr;
	void *array_ptr;
	char **string_array;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for record '%s'",
		fname, record->name));

	status = mx_find_record_field( record, "value", &value_field );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_dimensions = value_field->num_dimensions;
	dimension = value_field->dimension;

	MX_DEBUG(-2,("Record type = %ld", record->mx_type));

	if ( record->mx_type != MXV_INL_STRING )
		return MX_SUCCESSFUL_RESULT;

	MX_DEBUG(-2,("Num dimensions = %ld", num_dimensions));

	if ( num_dimensions != 2 )
		return MX_SUCCESSFUL_RESULT;

	data_ptr = value_field->data_pointer;

	MX_DEBUG(-2,("data_ptr = %p", data_ptr));

	array_ptr = mx_read_void_pointer_from_memory_location( data_ptr );

	MX_DEBUG(-2,("array_ptr = %p", array_ptr));

	string_array = (char **) array_ptr;

	MX_DEBUG(-2,("string_array = %p", string_array));
	MX_DEBUG(-2,("string_array[0] ptr = %p", string_array[0]));
	MX_DEBUG(-2,("string_array[1] ptr = %p", string_array[1]));
	MX_DEBUG(-2,("string_array[0] value = '%s'", string_array[0]));
	MX_DEBUG(-2,("string_array[1] value = '%s'", string_array[1]));

	for ( i = 0; i < dimension[0]; i++ ) {
		MX_DEBUG(-2,( "string[%ld] ptr = %p", i, string_array[i]));
		MX_DEBUG(-2,( "string[%ld] = '%s'", i, string_array[i]));
	}
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_inline_variable_delete_record( MX_RECORD *record )
{
	const char fname[] = "mxv_inline_variable_delete_record()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"This function is not yet implemented." );
}

MX_EXPORT mx_status_type
mxv_inline_variable_dummy_function( MX_RECORD *record )
{
	/* This function serves as a placeholder for functions that are
	 * not needed by this variable class.
	 */

	return MX_SUCCESSFUL_RESULT;
}

