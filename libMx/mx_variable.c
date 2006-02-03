/* 
 * Name:    mx_variable.c
 *
 * Purpose: Support for generic variable superclass support.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_array.h"
#include "mx_driver.h"
#include "mx_variable.h"

MX_EXPORT mx_status_type
mx_variable_initialize_type( long record_type )
{
	static const char fname[] = "mx_variable_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	long num_record_fields;
	mx_status_type mx_status;

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr = driver->record_field_defaults_ptr;

	if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	record_field_defaults = *record_field_defaults_ptr;

	if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (mx_length_type *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	/**** Fix up the record fields common to all variable types. ****/

	num_record_fields = *(driver->num_record_fields);

	mx_status = mx_variable_fixup_varargs_record_field_defaults(
			record_field_defaults, num_record_fields );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_variable_fixup_varargs_record_field_defaults(
		MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array,
		mx_length_type num_record_fields )
{
	static const char fname[] =
		"mx_variable_fixup_varargs_record_field_defaults()";

	MX_RECORD_FIELD_DEFAULTS *dimension_field, *value_field;
	mx_length_type num_dimensions_field_index;
	mx_length_type num_dimensions_varargs_cookie;
	mx_length_type dimension_field_index;
	mx_length_type dimension_element_cookie;
	mx_length_type i;
	mx_status_type mx_status;

	MX_DEBUG( 8,("%s invoked.", fname));

	/****
	 **** Construct and place the 'num_dimensions' varargs cookie.
	 ****/

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults_array, num_record_fields,
			"num_dimensions", &num_dimensions_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
		num_dimensions_field_index, 0, &num_dimensions_varargs_cookie);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 8,("%s: num_dimensions varargs cookie = %ld",
			fname, (long) num_dimensions_varargs_cookie));

	/*---*/

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults_array, num_record_fields,
			"dimension", &dimension_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dimension_field = &record_field_defaults_array[dimension_field_index];

	dimension_field->dimension[0] = num_dimensions_varargs_cookie;

	/*---*/

	mx_status = mx_find_record_field_defaults(
			record_field_defaults_array, num_record_fields,
			"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value_field->num_dimensions = num_dimensions_varargs_cookie;

	/****
	 **** Construct the varargs cookies for each of the entries in
	 **** the 'value' array.
	 ****/

	for ( i = 0; i < MXU_FIELD_MAX_DIMENSIONS; i++ ) {

		mx_status = mx_construct_varargs_cookie(
			dimension_field_index, i, &dimension_element_cookie );

		value_field->dimension[i] = dimension_element_cookie;

		MX_DEBUG( 8,("%s: dimension[%ld] cookie = %ld",
			fname, (long) i, (long) dimension_element_cookie));
	}

	return MX_SUCCESSFUL_RESULT;
}

/* A note concerning the mx_send_variable() and mx_receive_variable()
 * functions below.  For some variable classes, the MX_VARIABLE_FUNCTION_LIST
 * pointer is NULL.  This means that no data transfer need take place.
 * This is also true if the 'send_variable' or 'receive_variable' function
 * pointers are NULL.  An example of this are "inline" variables which have
 * the master copy of the data in the database record itself and do not
 * need to transfer any data anywhere.
 */

MX_EXPORT mx_status_type
mx_send_variable( MX_RECORD *record )
{
	static const char fname[] = "mx_send_variable()";

	MX_VARIABLE *variable;
	MX_VARIABLE_FUNCTION_LIST *variable_flist;
	mx_status_type ( *fptr ) ( MX_VARIABLE * );
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable_flist = (MX_VARIABLE_FUNCTION_LIST *)
				record->superclass_specific_function_list;

	if ( variable_flist == (MX_VARIABLE_FUNCTION_LIST *) NULL ) {
		/* This is _not_ an error. */

		return MX_SUCCESSFUL_RESULT;
	}

	fptr = variable_flist->send_value;

	if ( fptr == NULL ) {
		/* This is also _not_ an error. */

		return MX_SUCCESSFUL_RESULT;
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	if ( variable == (MX_VARIABLE *) NULL ) {
		/* This _is_ an error if we get here. */

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VARIABLE pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = ( *fptr ) ( variable );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_receive_variable( MX_RECORD *record )
{
	static const char fname[] = "mx_receive_variable()";

	MX_VARIABLE *variable;
	MX_VARIABLE_FUNCTION_LIST *variable_flist;
	mx_status_type ( *fptr ) ( MX_VARIABLE * );
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable_flist = (MX_VARIABLE_FUNCTION_LIST *)
				record->superclass_specific_function_list;

	if ( variable_flist == (MX_VARIABLE_FUNCTION_LIST *) NULL ) {
		/* This is _not_ an error. */

		return MX_SUCCESSFUL_RESULT;
	}

	fptr = variable_flist->receive_value;

	if ( fptr == NULL ) {
		/* This is also _not_ an error. */

		return MX_SUCCESSFUL_RESULT;
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	if ( variable == (MX_VARIABLE *) NULL ) {
		/* This _is_ an error if we get here. */

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VARIABLE pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = ( *fptr ) ( variable );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_get_variable_parameters( MX_RECORD *record,
		mx_length_type *num_dimensions,
		mx_length_type **dimension_array,
		long *field_type,
		void **pointer_to_value )
{
	MX_RECORD_FIELD *value_field;
	mx_status_type mx_status;

	mx_status = mx_find_record_field( record, "value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_dimensions != NULL ) {
		*num_dimensions = value_field->num_dimensions;
	}
	if ( dimension_array != NULL ) {
		*dimension_array = value_field->dimension;
	}
	if ( field_type != NULL ) {
		*field_type = value_field->datatype;
	}
	if ( pointer_to_value != NULL ) {
		*pointer_to_value = mx_get_field_value_pointer( value_field );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_variable_pointer( MX_RECORD *record, void **pointer_to_value )
{
	static const char fname[] = "mx_get_variable_pointer()";

	MX_RECORD_FIELD *value_field;
	mx_status_type mx_status;

	if ( pointer_to_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pointer_to_value argument passed was NULL." );
	}

	mx_status = mx_find_record_field( record, "value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*pointer_to_value = mx_get_field_value_pointer( value_field );

	return MX_SUCCESSFUL_RESULT;
}

/*** Note that the following functions only work for 1-dimensional arrays. ***/

MX_EXPORT mx_status_type
mx_get_1d_array_by_name( MX_RECORD *record_list,
		char *record_name,
		long requested_field_type,
		mx_length_type *num_elements,
		void **pointer_to_value )
{
	static const char fname[] = "mx_get_1d_array_by_name()";

	MX_RECORD *record;
	long actual_field_type;
	mx_length_type num_dimensions, *dimension_array;
	mx_status_type mx_status;

	record = mx_get_record( record_list, record_name );

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The record '%s' does not exist.", record_name );
	}

	mx_status = mx_get_variable_parameters( record, &num_dimensions,
			&dimension_array, &actual_field_type,
			pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( actual_field_type != requested_field_type ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"The 'value' field for '%s' is not of type %s.  Instead, it is of type %s.",
			record_name,
			mx_get_field_type_string( requested_field_type ),
			mx_get_field_type_string( actual_field_type ) );
	}

	if ( num_dimensions != 1 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"%s only supports 1-dimensional arrays.  '%s.value' has %ld dimensions.",
			fname, record_name, (long) num_dimensions );
	}

	if ( num_elements != NULL ) {
		*num_elements = dimension_array[0];
	}

	/* Fetch the value of the variable from a remote server, if any. */

	mx_status = mx_receive_variable( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_get_1d_array( MX_RECORD *record,
		long requested_field_type,
		mx_length_type *num_elements,
		void **pointer_to_value )
{
	static const char fname[] = "mx_get_1d_array()";

	long actual_field_type;
	mx_length_type num_dimensions, *dimension_array;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	mx_status = mx_get_variable_parameters( record, &num_dimensions,
			&dimension_array, &actual_field_type,
			pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( actual_field_type != requested_field_type ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"The 'value' field for '%s' is not of type %s.  Instead, it is of type %s.",
			record->name,
			mx_get_field_type_string( requested_field_type ),
			mx_get_field_type_string( actual_field_type ) );
	}

	if ( num_dimensions != 1 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"%s only supports 1-dimensional arrays.  '%s.value' has %ld dimensions.",
			fname, record->name, (long) num_dimensions );
	}

	if ( num_elements != NULL ) {
		*num_elements = dimension_array[0];
	}

	/* Fetch the value of the variable from a remote server, if any. */

	mx_status = mx_receive_variable( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_set_1d_array( MX_RECORD *record,
		long requested_field_type,
		mx_length_type num_elements,
		void *pointer_to_supplied_value )
{
	static const char fname[] = "mx_test_1d_array()";

	long actual_field_type;
	mx_length_type num_dimensions, *dimension_array;
	void *pointer_to_value;
	size_t value_size;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	mx_status = mx_get_variable_parameters( record, &num_dimensions,
			&dimension_array, &actual_field_type,
			&pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( actual_field_type != requested_field_type ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"The 'value' field for '%s' is not of type %s.  Instead, it is of type %s.",
			record->name,
			mx_get_field_type_string( requested_field_type ),
			mx_get_field_type_string( actual_field_type ) );
	}

	if ( num_dimensions != 1 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"%s only supports 1-dimensional arrays.  '%s.value' has %ld dimensions.",
			fname, record->name, (long) num_dimensions );
	}

	if ( num_elements > dimension_array[0] ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The array length (%ld) passed is greater than the maximum length of "
"the array (%ld) in the MX database variable '%s'.",
			(long) num_elements, (long) dimension_array[0],
			record->name );
	}

	/* Send the data. */

	if ( requested_field_type == MXFT_STRING ) {
		strcpy( (char *) pointer_to_value,
			(char *) pointer_to_supplied_value );
	} else {
		switch( requested_field_type ) {
		case MXFT_CHAR:
			value_size = num_elements * sizeof(char);
			break;
		case MXFT_INT8:
			value_size = num_elements * sizeof(int8_t);
			break;
		case MXFT_UINT8:
			value_size = num_elements * sizeof(uint8_t);
			break;
		case MXFT_INT16:
			value_size = num_elements * sizeof(int16_t);
			break;
		case MXFT_UINT16:
			value_size = num_elements * sizeof(uint16_t);
			break;
		case MXFT_INT32:
			value_size = num_elements * sizeof(int32_t);
			break;
		case MXFT_UINT32:
			value_size = num_elements * sizeof(uint32_t);
			break;
		case MXFT_INT64:
			value_size = num_elements * sizeof(int64_t);
			break;
		case MXFT_UINT64:
			value_size = num_elements * sizeof(uint64_t);
			break;
		case MXFT_FLOAT:
			value_size = num_elements * sizeof(float);
			break;
		case MXFT_DOUBLE:
			value_size = num_elements * sizeof(double);
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
	"This function is unsupported for the field type %ld.",
				requested_field_type );
			break;
		}

		memmove( pointer_to_value,
				pointer_to_supplied_value, value_size );
	}

	mx_status = mx_send_variable( record );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_get_int32_variable_by_name( MX_RECORD *record_list,
		char *record_name,
		int32_t *int32_value )
{
	static const char fname[] = "mx_get_int32_variable_by_name()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( int32_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"int32_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array_by_name( record_list, record_name,
			MXFT_INT32, &num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*int32_value = *( (int32_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_uint32_variable_by_name( MX_RECORD *record_list,
		char *record_name,
		uint32_t *uint32_value )
{
	static const char fname[] = "mx_get_uint32_variable_by_name()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( uint32_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"unsigned_long_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array_by_name( record_list, record_name,
			MXFT_UINT32, &num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*uint32_value = *( (uint32_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_double_variable_by_name( MX_RECORD *record_list,
		char *record_name,
		double *double_value )
{
	static const char fname[] = "mx_get_double_variable_by_name()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( double_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"double_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array_by_name( record_list, record_name,
			MXFT_DOUBLE, &num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*double_value = *( (double *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_string_variable_by_name( MX_RECORD *record_list,
		char *record_name,
		char *string_value,
		size_t max_string_length )
{
	static const char fname[] = "mx_get_string_variable_by_name()";

	mx_length_type num_elements;
	void *pointer_to_value;
	char *pointer_to_string;
	mx_status_type mx_status;

	if ( string_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"string_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array_by_name( record_list, record_name,
			MXFT_STRING, &num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pointer_to_string = (char *) pointer_to_value;

	strcpy( string_value, "" );

	strncat( string_value, pointer_to_string, max_string_length );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_get_char_variable( MX_RECORD *record,
			char *char_value )
{
	static const char fname[] = "mx_get_char_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( char_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"char_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_CHAR,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*char_value = *( (char *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_int8_variable( MX_RECORD *record,
			int8_t *int8_value )
{
	static const char fname[] = "mx_get_int8_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( int8_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"int8_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_INT8,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*int8_value = *( (int8_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_uint8_variable( MX_RECORD *record,
			uint8_t *uint8_value )
{
	static const char fname[] = "mx_get_uint8_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( uint8_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"unsigned_char_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_UINT8,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*uint8_value = *( (uint8_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_int16_variable( MX_RECORD *record,
			int16_t *int16_value )
{
	static const char fname[] = "mx_get_int16_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( int16_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"int16_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_INT16,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*int16_value = *( (int16_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_uint16_variable( MX_RECORD *record,
			uint16_t *uint16_value )
{
	static const char fname[] = "mx_get_uint16_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( uint16_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"uint16_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_UINT16,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*uint16_value = *( (uint16_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_int32_variable( MX_RECORD *record,
			int32_t *int32_value )
{
	static const char fname[] = "mx_get_int32_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( int32_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"int_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_INT32,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*int32_value = *( (int32_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_uint32_variable( MX_RECORD *record,
			uint32_t *uint32_value )
{
	static const char fname[] = "mx_get_uint32_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( uint32_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"unsigned_int_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_UINT32,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*uint32_value = *( (uint32_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_int64_variable( MX_RECORD *record,
			int64_t *int64_value )
{
	static const char fname[] = "mx_get_int64_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( int64_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"int64_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_INT64,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*int64_value = *( (int64_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_uint64_variable( MX_RECORD *record,
			uint64_t *uint64_value )
{
	static const char fname[] = "mx_get_uint64_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( uint64_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"uint64_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_UINT64,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*uint64_value = *( (uint64_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_float_variable( MX_RECORD *record,
			float *float_value )
{
	static const char fname[] = "mx_get_float_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( float_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"float_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_FLOAT,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*float_value = *( (float *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_double_variable( MX_RECORD *record,
			double *double_value )
{
	static const char fname[] = "mx_get_double_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type mx_status;

	if ( double_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"double_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_DOUBLE,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*double_value = *( (double *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_string_variable( MX_RECORD *record,
			char *string_value,
			size_t max_string_length )
{
	static const char fname[] = "mx_get_string_variable()";

	mx_length_type num_elements;
	void *pointer_to_value;
	char *pointer_to_string;
	mx_status_type mx_status;

	if ( string_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"string_value pointer passed was NULL." );
	}

	mx_status = mx_get_1d_array( record, MXFT_STRING,
					&num_elements, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pointer_to_string = (char *) pointer_to_value;

	strcpy( string_value, "" );

	strncat( string_value, pointer_to_string, max_string_length );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_set_char_variable( MX_RECORD *record, char char_value )
{
	return mx_set_1d_array( record, MXFT_CHAR, 1L, &char_value );
}

MX_EXPORT mx_status_type
mx_set_int8_variable( MX_RECORD *record, int8_t int8_value )
{
	return mx_set_1d_array( record, MXFT_INT8, 1L, &int8_value );
}

MX_EXPORT mx_status_type
mx_set_uint8_variable( MX_RECORD *record, uint8_t uint8_value )
{
	return mx_set_1d_array( record, MXFT_UINT8, 1L, &uint8_value );
}

MX_EXPORT mx_status_type
mx_set_int16_variable( MX_RECORD *record, int16_t int16_value )
{
	return mx_set_1d_array( record, MXFT_INT16, 1L, &int16_value );
}

MX_EXPORT mx_status_type
mx_set_uint16_variable( MX_RECORD *record, uint16_t uint16_value )
{
	return mx_set_1d_array( record, MXFT_UINT16, 1L, &uint16_value);
}

MX_EXPORT mx_status_type
mx_set_int32_variable( MX_RECORD *record, int32_t int32_value )
{
	return mx_set_1d_array( record, MXFT_INT32, 1L, &int32_value );
}

MX_EXPORT mx_status_type
mx_set_uint32_variable( MX_RECORD *record, uint32_t uint32_value )
{
	return mx_set_1d_array( record, MXFT_UINT32, 1L, &uint32_value );
}

MX_EXPORT mx_status_type
mx_set_int64_variable( MX_RECORD *record, int64_t int64_value )
{
	return mx_set_1d_array( record, MXFT_INT64, 1L, &int64_value );
}

MX_EXPORT mx_status_type
mx_set_uint64_variable( MX_RECORD *record, uint64_t uint64_value )
{
	return mx_set_1d_array( record, MXFT_UINT64, 1L, &uint64_value );
}

MX_EXPORT mx_status_type
mx_set_float_variable( MX_RECORD *record, float float_value )
{
	return mx_set_1d_array( record, MXFT_FLOAT, 1L, &float_value );
}

MX_EXPORT mx_status_type
mx_set_double_variable( MX_RECORD *record, double double_value )
{
	return mx_set_1d_array( record, MXFT_DOUBLE, 1L, &double_value );
}

MX_EXPORT mx_status_type
mx_set_string_variable( MX_RECORD *record, char *string_value )
{
	return mx_set_1d_array( record, MXFT_STRING,
				(long) strlen( string_value ), string_value );
}

