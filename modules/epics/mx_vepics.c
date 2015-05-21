/*
 * Name:    mx_vepics.c
 *
 * Purpose: Support for EPICS variables in MX database description files.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2006, 2009-2011, 2014-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_program_model.h"
#include "mx_epics.h"
#include "mx_variable.h"
#include "mx_vepics.h"

MX_RECORD_FUNCTION_LIST mxv_epics_variable_record_function_list = {
	mx_variable_initialize_driver,
	mxv_epics_variable_create_record_structures,
	mxv_epics_variable_finish_record_initialization,
	NULL,
	NULL,
	mxv_epics_variable_open
};

MX_VARIABLE_FUNCTION_LIST mxv_epics_variable_variable_function_list = {
	mxv_epics_variable_send_variable,
	mxv_epics_variable_receive_variable
};

/********************************************************************/

MX_RECORD_FIELD_DEFAULTS mxv_epics_string_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_EPICS_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_epics_string_variable_num_record_fields
			= sizeof( mxv_epics_string_variable_defaults )
			/ sizeof( mxv_epics_string_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_epics_string_variable_def_ptr
			= &mxv_epics_string_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_epics_char_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_EPICS_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_CHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_epics_char_variable_num_record_fields
			= sizeof( mxv_epics_char_variable_defaults )
			/ sizeof( mxv_epics_char_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_epics_char_variable_def_ptr
			= &mxv_epics_char_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_epics_short_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_EPICS_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_SHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_epics_short_variable_num_record_fields
			= sizeof( mxv_epics_short_variable_defaults )
			/ sizeof( mxv_epics_short_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_epics_short_variable_def_ptr
			= &mxv_epics_short_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_epics_long_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_EPICS_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_LONG_VARIABLE_STANDARD_FIELDS
};

long mxv_epics_long_variable_num_record_fields
			= sizeof( mxv_epics_long_variable_defaults )
			/ sizeof( mxv_epics_long_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_epics_long_variable_def_ptr
			= &mxv_epics_long_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_epics_float_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_EPICS_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_FLOAT_VARIABLE_STANDARD_FIELDS
};

long mxv_epics_float_variable_num_record_fields
			= sizeof( mxv_epics_float_variable_defaults )
			/ sizeof( mxv_epics_float_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_epics_float_variable_def_ptr
			= &mxv_epics_float_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_epics_double_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_EPICS_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_epics_double_variable_num_record_fields
			= sizeof( mxv_epics_double_variable_defaults )
			/ sizeof( mxv_epics_double_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_epics_double_variable_def_ptr
			= &mxv_epics_double_variable_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxv_epics_variable_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxv_epics_variable_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_EPICS_VARIABLE *epics_variable;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	epics_variable = (MX_EPICS_VARIABLE *)
				malloc( sizeof(MX_EPICS_VARIABLE) );

	if ( epics_variable == (MX_EPICS_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_VARIABLE structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = epics_variable;
	record->record_type_struct = NULL;

	record->superclass_specific_function_list =
				&mxv_epics_variable_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_epics_variable_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxv_epics_variable_finish_record_initialization()";

	MX_EPICS_VARIABLE *epics_variable;
	MX_RECORD_FIELD *value_field;
	long epics_type;
	mx_status_type mx_status;

	/* EPICS variables may only be associated with 1-dimensional
	 * MX variables.
	 *
	 * Test this by looking at the number of dimensions in the 
	 * 'value' field.
	 */

	mx_status = mx_find_record_field( record, "value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

	if ( value_field->num_dimensions != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"EPICS variables may only be associated with 1-dimensional MX variables.  "
"The field '%s.value' has %ld variables.",
			record->name,
			value_field->num_dimensions );
	}

	/* Store the EPICS type associated with this MX field's datatype. */

	mx_status = mx_epics_convert_mx_type_to_epics_type(
			value_field->datatype, &epics_type );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_variable = (MX_EPICS_VARIABLE *) record->record_class_struct;

	epics_variable->epics_type = epics_type;

	epics_variable->num_elements = -1;

	mx_epics_pvname_init( &(epics_variable->pv), epics_variable->pvname );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_epics_variable_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_epics_variable_open()";

	
	MX_EPICS_VARIABLE *epics_variable;
	MX_EPICS_PV *pv;
	size_t array_size_in_bytes, num_elements;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	epics_variable = (MX_EPICS_VARIABLE *) record->record_class_struct;

	if ( epics_variable == (MX_EPICS_VARIABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_VARIABLE pointer for record '%s' is NULL.",
			record->name );
	}

	/* If not already known, find out the number of elements in the
	 * EPICS process variable.
	 */

	pv = &(epics_variable->pv);

	if ( epics_variable->num_elements < 0 ) {
		num_elements = mx_epics_pv_get_element_count( pv );

		if ( num_elements == 0 ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to get the element count for EPICS PV '%s'.",
				pv->pvname );
		}

		epics_variable->num_elements = (long) num_elements;
	}

	/* Figure out whether or not we have to do a 64-bit conversion
	 * for this PV.
	 */

	#if ( MX_WORDSIZE == 64 )
	epics_variable->do_64bit_conversion = TRUE;
	#else
	epics_variable->do_64bit_conversion = FALSE;
	#endif

	if ( epics_variable->do_64bit_conversion == FALSE ) {
		epics_variable->int32_array = NULL;
	} else {
		array_size_in_bytes =
			epics_variable->num_elements * sizeof(int32_t);

		epics_variable->int32_array = (int32_t *)
			malloc( array_size_in_bytes );

		if ( epics_variable->int32_array == (int32_t *) NULL ) {
			epics_variable->do_64bit_conversion = FALSE;

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld element "
			"array of 32-bit integers for record '%s'.",
				epics_variable->num_elements, record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_epics_variable_send_variable( MX_VARIABLE *variable )
{
	MX_EPICS_VARIABLE *epics_variable;
	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	unsigned long num_elements;
	mx_status_type mx_status;

	epics_variable = (MX_EPICS_VARIABLE *)
				variable->record->record_class_struct;

	/* Find the location of the data to be sent and the number of values
	 * to send.
	 */

	mx_status = mx_find_record_field( variable->record,
						"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( value_field->dimension[0] >= epics_variable->num_elements ) {
		num_elements = epics_variable->num_elements;
	} else {
		num_elements = value_field->dimension[0];
	}

	value_ptr = mx_get_field_value_pointer( value_field );

	/* The value field for MX 'long' and 'unsigned long' fields always
	 * use the native wordlength of the system that we are running on,
	 * either 32-bit or 64-bit.  However, EPICS 'longs' are always
	 * 32-bits.  Therefore, on a computer with 64-bit longs, we must
	 * copy and clip the 64-bit long array over to a local int32_t
	 * array that belongs to this record.  Then we tell mx_caput()
	 * to send that array instead.
	 */

	if ( epics_variable->do_64bit_conversion ) {
		long *long_array;
		int32_t *int32_array;
		unsigned long i;

		long_array  = (long *) value_ptr;
		int32_array = epics_variable->int32_array;

		for ( i = 0; i < epics_variable->num_elements; i++ ) {
			int32_array[i] = long_array[i];
		}

		value_ptr = int32_array;
	}

	/* Send the data. */

	mx_status = mx_caput( &(epics_variable->pv),
			epics_variable->epics_type,
			num_elements,
			value_ptr );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_epics_variable_receive_variable( MX_VARIABLE *variable )
{
	MX_EPICS_VARIABLE *epics_variable;
	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	unsigned long num_elements;
	mx_status_type mx_status;

	epics_variable = (MX_EPICS_VARIABLE *)
				variable->record->record_class_struct;

	/* Find the location that the data is to be written to and the
	 * number of values to receive.
	 */

	mx_status = mx_find_record_field( variable->record,
						"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( value_field->dimension[0] >= epics_variable->num_elements ) {
		num_elements = epics_variable->num_elements;
	} else {
		num_elements = value_field->dimension[0];
	}

	/* The value field for MX 'long' and 'unsigned long' fields always
	 * use the native wordlength of the system that we are running on,
	 * either 32-bit or 64-bit.  However, EPICS 'longs' are always
	 * 32-bits.  Therefore, on a computer with 64-bit longs, we begin
	 * by copying to a local array that is known to be for 32-bit
	 * integers.  Then, on a 64-bit long machine, we copy the data
	 * from the 32-bit temporary array to the MX value field's
	 * value array.
	 */

	if ( epics_variable->do_64bit_conversion ) {
		value_ptr = epics_variable->int32_array;
	} else {
		value_ptr = mx_get_field_value_pointer( value_field );
	}

	/* Receive the data. */

	mx_status = mx_caget( &(epics_variable->pv),
			epics_variable->epics_type,
			num_elements,
			value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epics_variable->do_64bit_conversion ) {
		long *long_array;
		int32_t *int32_array;
		unsigned long i;

		long_array = (long *) value_ptr;
		int32_array = epics_variable->int32_array;

		for ( i = 0; i < epics_variable->num_elements; i++ ) {
			long_array[i] = int32_array[i];
		}
	}

	return mx_status;
}

