/*
 * Name:    mx_vnet.c
 *
 * Purpose: Support for network variables in MX database description files.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2005, 2010, 2014, 2021
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
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_net.h"
#include "mx_variable.h"
#include "mx_vnet.h"

MX_RECORD_FUNCTION_LIST mxv_network_variable_record_function_list = {
	mx_variable_initialize_driver,
	mxv_network_variable_create_record_structures,
	mxv_network_variable_finish_record_initialization
};

MX_VARIABLE_FUNCTION_LIST mxv_network_variable_variable_function_list = {
	mxv_network_variable_send_variable,
	mxv_network_variable_receive_variable
};

/********************************************************************/

MX_RECORD_FIELD_DEFAULTS mxv_network_string_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_network_string_variable_num_record_fields
			= sizeof( mxv_network_string_variable_defaults )
			/ sizeof( mxv_network_string_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_string_variable_dptr
			= &mxv_network_string_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_char_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_CHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_network_char_variable_num_record_fields
			= sizeof( mxv_network_char_variable_defaults )
			/ sizeof( mxv_network_char_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_char_variable_dptr
			= &mxv_network_char_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_uchar_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UCHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_network_uchar_variable_num_record_fields
			= sizeof( mxv_network_uchar_variable_defaults )
			/ sizeof( mxv_network_uchar_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_uchar_variable_dptr
			= &mxv_network_uchar_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_int8_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_INT8_VARIABLE_STANDARD_FIELDS
};

long mxv_network_int8_variable_num_record_fields
			= sizeof( mxv_network_int8_variable_defaults )
			/ sizeof( mxv_network_int8_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_int8_variable_dptr
			= &mxv_network_int8_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_uint8_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UINT8_VARIABLE_STANDARD_FIELDS
};

long mxv_network_uint8_variable_num_record_fields
			= sizeof( mxv_network_uint8_variable_defaults )
			/ sizeof( mxv_network_uint8_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_uint8_variable_dptr
			= &mxv_network_uint8_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_short_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_SHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_network_short_variable_num_record_fields
			= sizeof( mxv_network_short_variable_defaults )
			/ sizeof( mxv_network_short_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_short_variable_dptr
			= &mxv_network_short_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_ushort_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_USHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_network_ushort_variable_num_record_fields
			= sizeof( mxv_network_ushort_variable_defaults )
			/ sizeof( mxv_network_ushort_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_ushort_variable_dptr
			= &mxv_network_ushort_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_bool_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_BOOL_VARIABLE_STANDARD_FIELDS
};

long mxv_network_bool_variable_num_record_fields
			= sizeof( mxv_network_bool_variable_defaults )
			/ sizeof( mxv_network_bool_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_bool_variable_dptr
			= &mxv_network_bool_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_long_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_LONG_VARIABLE_STANDARD_FIELDS
};

long mxv_network_long_variable_num_record_fields
			= sizeof( mxv_network_long_variable_defaults )
			/ sizeof( mxv_network_long_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_long_variable_dptr
			= &mxv_network_long_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_ulong_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_ULONG_VARIABLE_STANDARD_FIELDS
};

long mxv_network_ulong_variable_num_record_fields
			= sizeof( mxv_network_ulong_variable_defaults )
			/ sizeof( mxv_network_ulong_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_ulong_variable_dptr
			= &mxv_network_ulong_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_int64_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_INT64_VARIABLE_STANDARD_FIELDS
};

long mxv_network_int64_variable_num_record_fields
			= sizeof( mxv_network_int64_variable_defaults )
			/ sizeof( mxv_network_int64_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_int64_variable_dptr
			= &mxv_network_int64_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_uint64_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UINT64_VARIABLE_STANDARD_FIELDS
};

long mxv_network_uint64_variable_num_record_fields
			= sizeof( mxv_network_uint64_variable_defaults )
			/ sizeof( mxv_network_uint64_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_uint64_variable_dptr
			= &mxv_network_uint64_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_float_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_FLOAT_VARIABLE_STANDARD_FIELDS
};

long mxv_network_float_variable_num_record_fields
			= sizeof( mxv_network_float_variable_defaults )
			/ sizeof( mxv_network_float_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_float_variable_dptr
			= &mxv_network_float_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_double_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_network_double_variable_num_record_fields
			= sizeof( mxv_network_double_variable_defaults )
			/ sizeof( mxv_network_double_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_double_variable_dptr
			= &mxv_network_double_variable_defaults[0];

/* ==== */

MX_RECORD_FIELD_DEFAULTS mxv_network_record_variable_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_NETWORK_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_network_record_variable_num_record_fields
			= sizeof( mxv_network_record_variable_defaults )
			/ sizeof( mxv_network_record_variable_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_network_record_variable_dptr
			= &mxv_network_record_variable_defaults[0];

/********************************************************************/

static mx_status_type
mxv_network_variable_get_pointers( MX_VARIABLE *variable,
				MX_NETWORK_VARIABLE **network_variable,
				MX_RECORD_FIELD **local_value_field,
				void **local_value_pointer,
				const char *calling_fname )
{
	static const char fname[] = "mxv_network_variable_get_pointers()";

	MX_RECORD_FIELD *local_value_field_ptr;
	mx_status_type mx_status;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for MX_VARIABLE structure %p passed by '%s' is NULL.",
			variable, calling_fname );
	}

	if ( (variable->record->mx_superclass != MXR_VARIABLE)
	  || (variable->record->mx_class != MXV_NETWORK) ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not a network variable.",
			variable->record->name, calling_fname );
	}

	if ( network_variable != (MX_NETWORK_VARIABLE **) NULL ) {

		*network_variable = (MX_NETWORK_VARIABLE *)
				variable->record->record_class_struct;

		if ( *network_variable == (MX_NETWORK_VARIABLE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_VARIABLE pointer for variable '%s' passed by '%s' is NULL.",
				variable->record->name, calling_fname );
		}
	}

	mx_status = mx_find_record_field( variable->record, "value",
						&local_value_field_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( local_value_field_ptr == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD_FIELD pointer not found for variable '%s' "
		"passed by '%s'",
			variable->record->name, calling_fname );
	}

	if ( local_value_field != (MX_RECORD_FIELD **) NULL ) {
		*local_value_field = local_value_field_ptr;
	}

	if ( local_value_pointer != NULL ) {
		*local_value_pointer = mx_get_field_value_pointer(
							local_value_field_ptr );

		if ( *local_value_pointer == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Value pointer not found for record field '%s.%s' "
			"passed by '%s'", variable->record->name,
				local_value_field_ptr->name,
				calling_fname );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_network_variable_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_network_variable_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_NETWORK_VARIABLE *network_variable;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCAN structure." );
	}

	network_variable = (MX_NETWORK_VARIABLE *)
				malloc( sizeof(MX_NETWORK_VARIABLE) );

	if ( network_variable == (MX_NETWORK_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_VARIABLE structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = network_variable;
	record->record_type_struct = NULL;

	record->superclass_specific_function_list =
				&mxv_network_variable_variable_function_list;
	record->class_specific_function_list = NULL;
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_network_variable_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_network_variable_finish_record_initialization()";

	MX_VARIABLE *variable;
	MX_NETWORK_VARIABLE *network_variable;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_network_variable_get_pointers( variable,
						&network_variable,
						NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy(record->network_type_name, "mx", MXU_NETWORK_TYPE_NAME_LENGTH);

	mx_network_field_init( &(network_variable->value_nf),
		network_variable->server_record,
		"%s", network_variable->remote_field_name );

#if 0
	/* The following is debugging code for testing string variables.
	 * It is not normally enabled.
	 */

	{
		MX_RECORD_FIELD *value_field;
		long num_dimensions;
		long *dimension;
		long i;
		char *data_ptr;
		void *array_ptr;
		char **string_array;

		MX_DEBUG( 2,("%s invoked for record '%s'",
			fname, record->name));

		mx_status = mx_find_record_field( record, "value",
							&value_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_dimensions = value_field->num_dimensions;
		dimension = value_field->dimension;

		MX_DEBUG(-2,("Record type = %ld", record->mx_type));

		if ( record->mx_type != MXV_NET_STRING )
			return MX_SUCCESSFUL_RESULT;

		MX_DEBUG(-2,("Num dimensions = %ld", num_dimensions));

		if ( num_dimensions != 2 )
			return MX_SUCCESSFUL_RESULT;

		data_ptr = value_field->data_pointer;

		MX_DEBUG(-2,("data_ptr = %p", data_ptr));

		array_ptr =
			mx_read_void_pointer_from_memory_location( data_ptr );

		MX_DEBUG(-2,("array_ptr = %p", array_ptr));

		string_array = (char **) array_ptr;

		MX_DEBUG(-2,("string_array = %p", string_array));
		MX_DEBUG(-2,("string_array[0] ptr = %p", string_array[0]));
		MX_DEBUG(-2,("string_array[1] ptr = %p", string_array[1]));
		MX_DEBUG(-2,("string_array[0] value = '%s'", string_array[0]));
		MX_DEBUG(-2,("string_array[1] value = '%s'", string_array[1]));

		for ( i = 0; i < dimension[0]; i++ ) {
			MX_DEBUG(-2,
			("string[%ld] ptr = %p", i, string_array[i]));

			MX_DEBUG(-2,
			("string[%ld] = '%s'", i, string_array[i]));
		}
	}
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_network_variable_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_network_variable_send_variable()";

	MX_NETWORK_VARIABLE *network_variable;
	MX_RECORD_FIELD *local_value_field;
	void *local_value_ptr;
	mx_status_type mx_status;

	mx_status = mxv_network_variable_get_pointers( variable,
						&network_variable,
						&local_value_field,
						&local_value_ptr,
						fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put_array( &(network_variable->value_nf),
				local_value_field->datatype,
				local_value_field->num_dimensions,
				local_value_field->dimension,
				local_value_ptr );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_network_variable_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_network_variable_receive_variable()";

	MX_NETWORK_VARIABLE *network_variable;
	MX_RECORD_FIELD *local_value_field;
	void *local_value_ptr;
	mx_status_type mx_status;

	mx_status = mxv_network_variable_get_pointers( variable,
						&network_variable,
						&local_value_field,
						&local_value_ptr,
						fname );


	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_array( &(network_variable->value_nf),
				local_value_field->datatype,
				local_value_field->num_dimensions,
				local_value_field->dimension,
				local_value_ptr );

	return mx_status;
}

