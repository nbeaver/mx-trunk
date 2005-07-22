/*
 * Name:    v_spec.c
 *
 * Purpose: Support for spec property variables.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "mx_record.h"
#include "mx_driver.h"
#include "mx_types.h"
#include "mx_variable.h"
#include "mx_spec.h"
#include "n_spec.h"
#include "v_spec.h"

MX_RECORD_FUNCTION_LIST mxv_spec_property_record_function_list = {
	mx_variable_initialize_type,
	mxv_spec_create_record_structures
};

MX_VARIABLE_FUNCTION_LIST mxv_spec_property_variable_function_list = {
	mxv_spec_send_variable,
	mxv_spec_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_spec_long_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_SPEC_PROPERTY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_LONG_VARIABLE_STANDARD_FIELDS
};

long mxv_spec_long_num_record_fields
	= sizeof( mxv_spec_long_field_defaults )
	/ sizeof( mxv_spec_long_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_spec_long_rfield_def_ptr
		= &mxv_spec_long_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_spec_ulong_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_SPEC_PROPERTY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_ULONG_VARIABLE_STANDARD_FIELDS
};

long mxv_spec_ulong_num_record_fields
	= sizeof( mxv_spec_ulong_field_defaults )
	/ sizeof( mxv_spec_ulong_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_spec_ulong_rfield_def_ptr
		= &mxv_spec_ulong_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_spec_short_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_SPEC_PROPERTY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_SHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_spec_short_num_record_fields
	= sizeof( mxv_spec_short_field_defaults )
	/ sizeof( mxv_spec_short_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_spec_short_rfield_def_ptr
		= &mxv_spec_short_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_spec_ushort_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_SPEC_PROPERTY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_USHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_spec_ushort_num_record_fields
	= sizeof( mxv_spec_ushort_field_defaults )
	/ sizeof( mxv_spec_ushort_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_spec_ushort_rfield_def_ptr
		= &mxv_spec_ushort_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_spec_char_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_SPEC_PROPERTY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_CHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_spec_char_num_record_fields
	= sizeof( mxv_spec_char_field_defaults )
	/ sizeof( mxv_spec_char_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_spec_char_rfield_def_ptr
		= &mxv_spec_char_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_spec_uchar_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_SPEC_PROPERTY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UCHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_spec_uchar_num_record_fields
	= sizeof( mxv_spec_uchar_field_defaults )
	/ sizeof( mxv_spec_uchar_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_spec_uchar_rfield_def_ptr
		= &mxv_spec_uchar_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_spec_float_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_SPEC_PROPERTY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_FLOAT_VARIABLE_STANDARD_FIELDS
};

long mxv_spec_float_num_record_fields
	= sizeof( mxv_spec_float_field_defaults )
	/ sizeof( mxv_spec_float_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_spec_float_rfield_def_ptr
		= &mxv_spec_float_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_spec_double_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_SPEC_PROPERTY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_spec_double_num_record_fields
	= sizeof( mxv_spec_double_field_defaults )
	/ sizeof( mxv_spec_double_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_spec_double_rfield_def_ptr
		= &mxv_spec_double_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_spec_string_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_SPEC_PROPERTY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_spec_string_num_record_fields
	= sizeof( mxv_spec_string_field_defaults )
	/ sizeof( mxv_spec_string_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_spec_string_rfield_def_ptr
		= &mxv_spec_string_field_defaults[0];

/*---*/

static mx_status_type
mxv_spec_get_pointers( MX_VARIABLE *variable,
			MX_SPEC_PROPERTY **spec_property,
			MX_SPEC_SERVER **spec_server,
			const char *calling_fname )
{
	const char fname[] = "mxv_spec_get_pointers()";

	MX_RECORD *spec_server_record;
	MX_SPEC_PROPERTY *spec_property_ptr;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	spec_property_ptr = (MX_SPEC_PROPERTY *)
				variable->record->record_type_struct;

	if ( spec_property != (MX_SPEC_PROPERTY **) NULL ) {
		*spec_property = spec_property_ptr;
	}

	spec_server_record = spec_property_ptr->spec_server_record;

	if ( spec_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"spec_server_record pointer for record '%s' passed by '%s' is NULL.",
			variable->record->name, calling_fname );
	}

	if ( spec_server_record->mx_type != MXN_SPEC_SERVER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"spec_server_record '%s' for SPEC variable '%s' is not a SPEC server.  "
"Instead, it is a '%s' record.",
			spec_server_record->name, variable->record->name,
			mx_get_driver_name( spec_server_record ) );
	}

	if ( spec_server != (MX_SPEC_SERVER **) NULL ) {
		*spec_server = (MX_SPEC_SERVER *)
				spec_server_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_spec_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxv_spec_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_SPEC_PROPERTY *spec_property;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	spec_property = (MX_SPEC_PROPERTY *)
				malloc( sizeof(MX_SPEC_PROPERTY) );

	if ( spec_property == (MX_SPEC_PROPERTY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SPEC_PROPERTY structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = spec_property;

	record->superclass_specific_function_list =
				&mxv_spec_property_variable_function_list;
	record->class_specific_function_list = NULL;

	spec_property->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_spec_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_spec_send_variable()";

	MX_SPEC_PROPERTY *spec_property;
	MX_SPEC_SERVER *spec_server;
	MX_RECORD_FIELD *field;
	mx_status_type (*token_constructor)( void *, char *, size_t,
					MX_RECORD *, MX_RECORD_FIELD * );
	long num_dimensions, field_type;
	long *dimension_array;
	void *value_ptr;
	char buffer[2000];
	char *buffer_ptr;
	mx_status_type mx_status;

	mx_status = mxv_spec_get_pointers( variable,
				&spec_property, &spec_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field( variable->record,
					"value", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_parameters( variable->record,
						&num_dimensions,
						&dimension_array,
						&field_type,
						&value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (num_dimensions != 1) && (num_dimensions != 2) ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Only 1-dimensional or 2-dimensional arrays are supported "
		"for 'spec_property' records, but record '%s' "
		"is %ld-dimensional.",
			variable->record->name,
			num_dimensions );
	}

	mx_status = mx_get_token_constructor( field_type, &token_constructor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strcpy( buffer, "" );

	mx_status = mx_create_array_description( value_ptr, num_dimensions - 1,
					buffer, sizeof(buffer) - 1,
					variable->record, field,
					token_constructor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* mx_create_array_description() puts a space character at the 
	 * beginning of the buffer.  In order to avoid the possibility
	 * of spec being confused by this whitespace character, we skip
	 * over the leading space character.
	 */

	buffer_ptr = buffer + 1;

	mx_status = mx_spec_put_string( spec_server->record,
					spec_property->property_name,
					buffer_ptr );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_spec_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_spec_receive_variable()";

	MX_SPEC_PROPERTY *spec_property;
	MX_SPEC_SERVER *spec_server;
	MX_RECORD_FIELD *field;
	MX_RECORD_FIELD_PARSE_STATUS parse_status;
	mx_status_type (*token_parser)( void *, char *,
				MX_RECORD *, MX_RECORD_FIELD *,
				MX_RECORD_FIELD_PARSE_STATUS * );
	long num_dimensions, field_type;
	long *dimension_array;
	void *value_ptr;
	static char separators[] = MX_RECORD_FIELD_SEPARATORS;
	char buffer[2000];
	mx_status_type mx_status;

	mx_status = mxv_spec_get_pointers( variable,
				&spec_property, &spec_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field( variable->record,
					"value", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_parameters( variable->record,
						&num_dimensions,
						&dimension_array,
						&field_type,
						&value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (num_dimensions != 1) && (num_dimensions != 2) ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Only 1-dimensional or 2-dimensional arrays are supported "
		"for 'spec_property' records, but record '%s' "
		"is %ld-dimensional.",
			variable->record->name,
			num_dimensions );
	}

	mx_status = mx_get_token_parser( field_type, &token_parser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_spec_get_string( spec_server->record,
					spec_property->property_name,
					buffer, sizeof(buffer) - 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_initialize_parse_status( &parse_status, buffer, separators );

	if ( field_type == MXFT_STRING ) {
		parse_status.max_string_token_length =
				mx_get_max_string_token_length( field );
	}

	mx_status = mx_parse_array_description( value_ptr, num_dimensions - 1,
						variable->record, field,
						&parse_status, token_parser );

	return mx_status;
}

