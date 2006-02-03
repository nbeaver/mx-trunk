/*
 * Name:     mx_fvarargs.c
 *
 * Purpose:  Generic MX_RECORD_FIELD varargs field support.
 *
 *           The functions in this file are prototyped in libmx/mx_record.h
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_array.h"

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_convert_varargs_cookie_to_value(
		MX_RECORD *record,
		mx_length_type varargs_cookie,
		mx_length_type *returned_value )
{
	static const char fname[] = "mx_convert_varargs_cookie_to_value()";

	MX_RECORD_FIELD *record_field_array, *field;
	long num_record_fields;
	mx_length_type field_index, array_in_field_index;
	mx_length_type *data_pointer, *length_array;
	void *array_pointer;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}
	if ( returned_value == (mx_length_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'returned_value' pointer passed is NULL." );
	}

	MX_DEBUG( 8,("%s: record = '%s', varargs_cookie = %ld",
		fname, record->name, (long) varargs_cookie));

	if ( varargs_cookie >= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal value %ld.  Varargs field values must be < 0.",
			(long) varargs_cookie );
	}

	record_field_array = record->record_field_array;
	num_record_fields = record->num_record_fields;

	if ( record_field_array == (MX_RECORD_FIELD *) NULL
	  || num_record_fields <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Record type for record '%s' is not handled by record field support.",
			record->name );
	}

	/* Split the varargs cookie into its component parts. */

	field_index = (-varargs_cookie) / MXU_VARARGS_COOKIE_MULTIPLIER;
	array_in_field_index
			= (-varargs_cookie) % MXU_VARARGS_COOKIE_MULTIPLIER;

	if ( field_index < 0 || field_index >= num_record_fields ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Field index %ld computed from varargs cookie %ld is out of range (0 - %ld).",
			(long) field_index, (long) varargs_cookie,
			(long) num_record_fields );
	}

	field = &record_field_array[ field_index ];

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RECORD_FIELD pointer for field index %ld is NULL. "
			"This should not be able to happen.",
				(long) field_index );
	}

	MX_DEBUG( 8,(
		"%s: cookie points to field '%s', array_in_field_index = %ld",
		fname, field->name, (long) array_in_field_index));

	if ( field->datatype != MXFT_LENGTH ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Field type %s for length field '%s' is invalid.  "
		"Only MXFT_LENGTH is allowed for a varargs length field.",
				mx_get_field_type_string( field->datatype ),
				field->name );
	}

	switch( field->num_dimensions ) {
	case 0:
		if ( array_in_field_index != 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Array in field index %ld computed from varargs "
		"cookie %ld is nonzero for a scalar MXFT_LENGTH field.",
			(long) array_in_field_index, (long) varargs_cookie );
		}

		data_pointer = (mx_length_type *)(field->data_pointer);

		*returned_value = *data_pointer;

		MX_DEBUG( 8,("%s: case 0 -> *returned_value = %ld",
				fname, (long) *returned_value));

		break;

	case 1:
		if ( field->dimension[0] < 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"field->dimension[0] = %ld is less than zero.  "
			"Recursive varargs length are not allowed.",
				(long) field->dimension[0] );
		}
		if ( array_in_field_index < 0
		  || array_in_field_index >= field->dimension[0] ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Array in field index %ld computed from "
		"varargs cookie %ld is out of allowed range (0 - %ld)",
				(long) array_in_field_index,
				(long) varargs_cookie,
				(long) field->dimension[0] );
		}

		array_pointer = mx_read_void_pointer_from_memory_location(
					field->data_pointer );

		length_array = (mx_length_type *) array_pointer;

		*returned_value = length_array[ array_in_field_index ];

		MX_DEBUG( 8,("%s: case 1 -> *returned_value = %ld",
				fname, (long) *returned_value));

		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The field '%s' pointed to by varargs cookie %ld is not "
		"a 0 or 1 dimensional array.",
			field->name, (long) varargs_cookie );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_construct_varargs_cookie(
		mx_length_type field_index,
		mx_length_type array_in_field_index,
		mx_length_type *returned_varargs_cookie )
{
	static const char fname[] = "mx_construct_varargs_cookie()";

	mx_length_type temp_value;

	if ( returned_varargs_cookie == (mx_length_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'returned_varargs_cookie' pointer passed is NULL." );
	}

	if ( field_index < 0 || field_index >= MXU_VARARGS_COOKIE_MULTIPLIER ){
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"field_index = %ld is out of allowed range (0 - %d).",
			(long) field_index, MXU_VARARGS_COOKIE_MULTIPLIER );
	}

	if ( array_in_field_index < 0
	  || array_in_field_index >= MXU_VARARGS_COOKIE_MULTIPLIER ){

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"array_in_field_index = %ld is out of allowed range (0 - %d).",
		  (long) array_in_field_index, MXU_VARARGS_COOKIE_MULTIPLIER );
	}

	temp_value = array_in_field_index +
			field_index * MXU_VARARGS_COOKIE_MULTIPLIER;

	*returned_varargs_cookie = -temp_value;

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_replace_varargs_cookies_with_values( MX_RECORD *record,
		mx_length_type field_index, int allow_forward_references )
{
	static const char fname[] = "mx_replace_varargs_cookies_with_values()";

	MX_RECORD_FIELD *field_array;
	MX_RECORD_FIELD *field;
	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *field_defaults_array;
	MX_RECORD_FIELD_DEFAULTS *field_defaults;
	long varargs_cookie, referenced_field_index;
	mx_length_type old_num_dimensions, new_num_dimensions;
	mx_length_type i, *temp_ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}
	if ( field_index < 0 || field_index >= record->num_record_fields ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"'field_index' = %ld is out of allowed range (0 - %ld).",
			(long) field_index, (long) record->num_record_fields );
	}

	field_array = record->record_field_array;

	if ( field_array == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_array' pointer for record '%s' is NULL.",
			record->name );
	}

	field = &field_array[ field_index ];

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD_FIELD pointer for field %ld in record '%s' is NULL.",
			(long) field_index, record->name );
	}

	/**** Does 'num_dimensions' have a varargs cookie in it? ****/

	MX_DEBUG( 8,("%s: field = '%s', num_dimensions = %ld",
			fname, field->name, (long) field->num_dimensions));

	if ( field->num_dimensions >= 0 ) {

		old_num_dimensions = field->num_dimensions;
		new_num_dimensions = field->num_dimensions;

		MX_DEBUG( 8,(
			"%s: field->num_dimensions is not varargs.", fname));

	} else {
		old_num_dimensions = MXU_FIELD_MAX_DIMENSIONS;

		varargs_cookie = field->num_dimensions;

		referenced_field_index
			= varargs_cookie / MXU_VARARGS_COOKIE_MULTIPLIER;

		if ( referenced_field_index == field_index ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The varargs cookie %ld points to the same "
				"field number %ld that we are currently in.",
				(long) varargs_cookie, (long) field_index );
		}
		if ( allow_forward_references == FALSE ) {
			if ( referenced_field_index > field_index ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"A forward reference by field %ld to field %ld is not allowed here.",
					(long) field_index,
					(long) referenced_field_index );
			}
		}

		MX_DEBUG( 8,("%s: referenced_field_index = %ld",
			fname, (long) referenced_field_index ));

		mx_status = mx_convert_varargs_cookie_to_value(
			record, varargs_cookie, &new_num_dimensions );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*
	 * We need to allocate a new array to contain the dimension values.
	 *
	 * If 'field->dimension' points to some memory that we have allocated
	 * ourselves, then we will need to free it, but we don't want to try
	 * to free something in an MX_RECORD_FIELD_DEFAULTS structure, since
	 * that was allocated in a different manner.
	 *
	 * So, our first task will be to discover whether or not
	 * 'field->dimension' is currently pointing to 
	 * 'field_defaults->dimension'.
	 */

	driver = mx_get_driver_by_type( record->mx_type );

	if ( driver == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Driver for record type %ld does not exist.",
				record->mx_type );
	}

	MX_DEBUG( 8,("%s: driver '%s' selected.", fname, driver->name));

	field_defaults_array = *(driver->record_field_defaults_ptr);

	if (field_defaults_array == (MX_RECORD_FIELD_DEFAULTS *)NULL){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Record field defaults array pointer for type %ld is NULL.",
				record->mx_type );
	}

	field_defaults = &field_defaults_array[ field_index ];

	if ( field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD_FIELD_DEFAULTS pointer for field index %ld is NULL.",
				(long) field_index );
	}

	temp_ptr = field->dimension;

	if ( field->num_dimensions == 1 ) {
		MX_DEBUG( 8,("%s: field->dimension[0] = %ld",
					fname, (long) field->dimension[0]));
	}

	field->dimension = (mx_length_type *)
			malloc(new_num_dimensions * sizeof(mx_length_type));

	if ( field->dimension == (mx_length_type *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory allocating new dimension array.  Array size = %ld items.",
				(long) new_num_dimensions );
	}

	if ( new_num_dimensions > old_num_dimensions ) {
		for ( i = 0; i < old_num_dimensions; i++ ) {
			field->dimension[i] = temp_ptr[i];
		}
		for ( i = old_num_dimensions; i < new_num_dimensions; i++ )
		{
			field->dimension[i] = 0;
		}
	} else {
		for ( i = 0; i < new_num_dimensions; i++ ) {
			field->dimension[i] = temp_ptr[i];
		}
	}

	MX_DEBUG( 8,
("%s: temp_ptr = %p, field->dimension = %p, field_defaults->dimension = %p",
		fname, temp_ptr, field->dimension, field_defaults->dimension));

	if ( temp_ptr == field_defaults->dimension ) {
		MX_DEBUG( 8,
			("%s: temp_ptr == field_defaults->dimension", fname));
	} else {
		MX_DEBUG( 8,
			("%s: temp_ptr != field_defaults->dimension", fname));

		free( temp_ptr );
	}

	field->num_dimensions = new_num_dimensions;

	/**** Loop through the dimensions looking for varargs cookies. ****/

	for ( i = 0; i < field->num_dimensions; i++ ) {
		if ( field->dimension[i] < 0 ) {
			varargs_cookie = field->dimension[i];
	
			referenced_field_index = varargs_cookie
					/ MXU_VARARGS_COOKIE_MULTIPLIER;
	
			if ( referenced_field_index == field_index ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The varargs cookie %ld points to the same "
				"field number %ld that we are currently in.",
					(long) varargs_cookie,
					(long) field_index );
			}
			if ( allow_forward_references == FALSE ) {
				if ( referenced_field_index > field_index ) {
					return mx_error(
						MXE_ILLEGAL_ARGUMENT, fname,
	"A forward reference by field %ld to field %ld is not allowed here.",
						(long) field_index,
						(long) referenced_field_index );
				}
			}
			mx_status = mx_convert_varargs_cookie_to_value(
				record, varargs_cookie,
				&(field->dimension[i]) );
	
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

