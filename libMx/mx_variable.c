/* 
 * Name:    mx_variable.c
 *
 * Purpose: Support for generic variable superclass support.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2005, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_driver.h"
#include "mx_variable.h"

MX_EXPORT mx_status_type
mx_variable_initialize_driver( MX_DRIVER *driver )
{
	static const char fname[] = "mx_variable_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array;
	MX_RECORD_FIELD_DEFAULTS *dimension_field, *value_field;
	long num_dimensions_field_index;
	long num_dimensions_varargs_cookie;
	long dimension_field_index;
	long dimension_element_cookie;
	long i;
	mx_status_type status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	MX_DEBUG( 8,("%s invoked.", fname));

	/****
	 **** Construct and place the 'num_dimensions' varargs cookie.
	 ****/

	status = mx_find_record_field_defaults_index( driver,
						"num_dimensions",
						&num_dimensions_field_index );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_construct_varargs_cookie(
		num_dimensions_field_index, 0, &num_dimensions_varargs_cookie);

	if ( status.code != MXE_SUCCESS )
		return status;

	MX_DEBUG( 8,("%s: num_dimensions varargs cookie = %ld",
			fname, num_dimensions_varargs_cookie));

	/*---*/

	status = mx_find_record_field_defaults_index( driver,
						"dimension",
						&dimension_field_index );

	if ( status.code != MXE_SUCCESS )
		return status;

	record_field_defaults_array = *(driver->record_field_defaults_ptr);

	dimension_field = &record_field_defaults_array[dimension_field_index];

	dimension_field->dimension[0] = num_dimensions_varargs_cookie;

	/*---*/

	status = mx_find_record_field_defaults( driver,
						"value", &value_field );

	if ( status.code != MXE_SUCCESS )
		return status;

	value_field->num_dimensions = num_dimensions_varargs_cookie;

	/****
	 **** Construct the varargs cookies for each of the entries in
	 **** the 'value' array.
	 ****/

	for ( i = 0; i < MXU_FIELD_MAX_DIMENSIONS; i++ ) {

		status = mx_construct_varargs_cookie(
			dimension_field_index, i, &dimension_element_cookie );

		value_field->dimension[i] = dimension_element_cookie;

		MX_DEBUG( 8,("%s: dimension[%ld] cookie = %ld",
			fname, i, dimension_element_cookie));
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
	mx_status_type status;

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

	status = ( *fptr ) ( variable );

	return status;
}

MX_EXPORT mx_status_type
mx_receive_variable( MX_RECORD *record )
{
	static const char fname[] = "mx_receive_variable()";

	MX_VARIABLE *variable;
	MX_VARIABLE_FUNCTION_LIST *variable_flist;
	mx_status_type ( *fptr ) ( MX_VARIABLE * );
	mx_status_type status;

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

	status = ( *fptr ) ( variable );

	return status;
}

MX_EXPORT mx_status_type
mx_get_variable_parameters( MX_RECORD *record,
		long *num_dimensions,
		long **dimension_array,
		long *field_type,
		void **pointer_to_value )
{
	MX_RECORD_FIELD *value_field;
	mx_status_type status;

	status = mx_find_record_field( record, "value", &value_field );

	if ( status.code != MXE_SUCCESS )
		return status;

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
	mx_status_type status;

	if ( pointer_to_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pointer_to_value argument passed was NULL." );
	}

	status = mx_find_record_field( record, "value", &value_field );

	if ( status.code != MXE_SUCCESS )
		return status;

	*pointer_to_value = mx_get_field_value_pointer( value_field );

	return MX_SUCCESSFUL_RESULT;
}

/*** Note that the following functions only work for 1-dimensional arrays. ***/

MX_EXPORT mx_status_type
mx_get_1d_array_by_name( MX_RECORD *record_list,
		char *record_name,
		long requested_field_type,
		long *num_elements,
		void **pointer_to_value )
{
	static const char fname[] = "mx_get_1d_array_by_name()";

	MX_RECORD *record;
	long num_dimensions, *dimension_array, actual_field_type;
	mx_status_type status;

	record = mx_get_record( record_list, record_name );

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The record '%s' does not exist.", record_name );
	}

	status = mx_get_variable_parameters( record, &num_dimensions,
			&dimension_array, &actual_field_type,
			pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

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
			fname, record_name, num_dimensions );
	}

	if ( num_elements != NULL ) {
		*num_elements = dimension_array[0];
	}

	/* Fetch the value of the variable from a remote server, if any. */

	status = mx_receive_variable( record );

	return status;
}

MX_EXPORT mx_status_type
mx_get_1d_array( MX_RECORD *record,
		long requested_field_type,
		long *num_elements,
		void **pointer_to_value )
{
	static const char fname[] = "mx_get_1d_array()";

	long num_dimensions, *dimension_array, actual_field_type;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	status = mx_get_variable_parameters( record, &num_dimensions,
			&dimension_array, &actual_field_type,
			pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

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
			fname, record->name, num_dimensions );
	}

	if ( num_elements != NULL ) {
		*num_elements = dimension_array[0];
	}

	/* Fetch the value of the variable from a remote server, if any. */

	status = mx_receive_variable( record );

	return status;
}

MX_EXPORT mx_status_type
mx_set_1d_array( MX_RECORD *record,
		long requested_field_type,
		long num_elements,
		void *pointer_to_supplied_value )
{
	static const char fname[] = "mx_test_1d_array()";

	long num_dimensions, *dimension_array, actual_field_type;
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
			fname, record->name, num_dimensions );
	}

	if ( num_elements > dimension_array[0] ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The array length (%ld) passed is greater than the maximum length of "
"the array (%ld) in the MX database variable '%s'.",
			num_elements, dimension_array[0],
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
		case MXFT_UCHAR:
			value_size = num_elements * sizeof(unsigned char);
			break;
		case MXFT_SHORT:
			value_size = num_elements * sizeof(short);
			break;
		case MXFT_USHORT:
			value_size = num_elements * sizeof(unsigned short);
			break;
		case MXFT_BOOL:
			value_size = num_elements * sizeof(mx_bool_type);
			break;
		case MXFT_LONG:
			value_size = num_elements * sizeof(long);
			break;
		case MXFT_ULONG:
			value_size = num_elements * sizeof(unsigned long);
			break;
		case MXFT_INT64:
			value_size = num_elements * sizeof(int64_t);
			break;
		case MXFT_UINT64:
			value_size = num_elements * sizeof(int64_t);
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
		}

		memmove( pointer_to_value,
				pointer_to_supplied_value, value_size );
	}

	mx_status = mx_send_variable( record );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_get_bool_variable_by_name( MX_RECORD *record_list,
		char *record_name,
		mx_bool_type *bool_value )
{
	static const char fname[] = "mx_get_bool_variable_by_name()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( bool_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"bool_value pointer passed was NULL." );
	}

	status = mx_get_1d_array_by_name( record_list, record_name,
			MXFT_BOOL, &num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*bool_value = *( (mx_bool_type *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_long_variable_by_name( MX_RECORD *record_list,
		char *record_name,
		long *long_value )
{
	static const char fname[] = "mx_get_long_variable_by_name()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( long_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"long_value pointer passed was NULL." );
	}

	status = mx_get_1d_array_by_name( record_list, record_name,
			MXFT_LONG, &num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*long_value = *( (long *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_unsigned_long_variable_by_name( MX_RECORD *record_list,
		char *record_name,
		unsigned long *unsigned_long_value )
{
	static const char fname[] = "mx_get_unsigned_long_variable_by_name()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( unsigned_long_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"unsigned_long_value pointer passed was NULL." );
	}

	status = mx_get_1d_array_by_name( record_list, record_name,
			MXFT_ULONG, &num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*unsigned_long_value = *( (unsigned long *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_double_variable_by_name( MX_RECORD *record_list,
		char *record_name,
		double *double_value )
{
	static const char fname[] = "mx_get_double_variable_by_name()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( double_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"double_value pointer passed was NULL." );
	}

	status = mx_get_1d_array_by_name( record_list, record_name,
			MXFT_DOUBLE, &num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

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

	long num_elements;
	void *pointer_to_value;
	char *pointer_to_string;
	mx_status_type status;

	if ( string_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"string_value pointer passed was NULL." );
	}

	status = mx_get_1d_array_by_name( record_list, record_name,
			MXFT_STRING, &num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

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

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( char_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"char_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_CHAR,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*char_value = *( (char *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_unsigned_char_variable( MX_RECORD *record,
			unsigned char *unsigned_char_value )
{
	static const char fname[] = "mx_get_unsigned_char_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( unsigned_char_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"unsigned_char_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_UCHAR,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*unsigned_char_value = *( (unsigned char *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_short_variable( MX_RECORD *record,
			short *short_value )
{
	static const char fname[] = "mx_get_short_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( short_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"short_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_SHORT,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*short_value = *( (short *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_unsigned_short_variable( MX_RECORD *record,
			unsigned short *unsigned_short_value )
{
	static const char fname[] = "mx_get_unsigned_short_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( unsigned_short_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"unsigned_short_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_USHORT,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*unsigned_short_value = *( (unsigned short *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_bool_variable( MX_RECORD *record,
			mx_bool_type *bool_value )
{
	static const char fname[] = "mx_get_bool_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( bool_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"bool_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_BOOL,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*bool_value = *( (mx_bool_type *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_long_variable( MX_RECORD *record,
			long *long_value )
{
	static const char fname[] = "mx_get_long_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( long_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"long_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_LONG,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*long_value = *( (long *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_unsigned_long_variable( MX_RECORD *record,
			unsigned long *unsigned_long_value )
{
	static const char fname[] = "mx_get_unsigned_long_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( unsigned_long_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"unsigned_long_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_ULONG,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*unsigned_long_value = *( (unsigned long *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_int64_variable( MX_RECORD *record,
			int64_t *int64_value )
{
	static const char fname[] = "mx_get_int64_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( int64_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"int64_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_INT64,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*int64_value = *( (int64_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_uint64_variable( MX_RECORD *record,
			uint64_t *uint64_value )
{
	static const char fname[] = "mx_get_uint64_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( uint64_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"uint64_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_UINT64,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*uint64_value = *( (uint64_t *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_float_variable( MX_RECORD *record,
			float *float_value )
{
	static const char fname[] = "mx_get_float_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( float_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"float_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_FLOAT,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*float_value = *( (float *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_double_variable( MX_RECORD *record,
			double *double_value )
{
	static const char fname[] = "mx_get_double_variable()";

	long num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( double_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"double_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_DOUBLE,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	*double_value = *( (double *) pointer_to_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_string_variable( MX_RECORD *record,
			char *string_value,
			size_t max_string_length )
{
	static const char fname[] = "mx_get_string_variable()";

	long num_elements;
	void *pointer_to_value;
	char *pointer_to_string;
	mx_status_type status;

	if ( string_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"string_value pointer passed was NULL." );
	}

	status = mx_get_1d_array( record, MXFT_STRING,
					&num_elements, &pointer_to_value );

	if ( status.code != MXE_SUCCESS )
		return status;

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
mx_set_unsigned_char_variable( MX_RECORD *record,
			unsigned char unsigned_char_value )
{
	return mx_set_1d_array( record, MXFT_UCHAR, 1L, &unsigned_char_value );
}

MX_EXPORT mx_status_type
mx_set_short_variable( MX_RECORD *record, short short_value )
{
	return mx_set_1d_array( record, MXFT_SHORT, 1L, &short_value );
}

MX_EXPORT mx_status_type
mx_set_unsigned_short_variable( MX_RECORD *record,
			unsigned short unsigned_short_value )
{
	return mx_set_1d_array( record, MXFT_USHORT, 1L, &unsigned_short_value);
}

MX_EXPORT mx_status_type
mx_set_bool_variable( MX_RECORD *record, mx_bool_type bool_value )
{
	return mx_set_1d_array( record, MXFT_BOOL, 1L, &bool_value );
}

MX_EXPORT mx_status_type
mx_set_long_variable( MX_RECORD *record, long long_value )
{
	return mx_set_1d_array( record, MXFT_LONG, 1L, &long_value );
}

MX_EXPORT mx_status_type
mx_set_unsigned_long_variable( MX_RECORD *record,
			unsigned long unsigned_long_value )
{
	return mx_set_1d_array( record, MXFT_ULONG, 1L, &unsigned_long_value );
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

