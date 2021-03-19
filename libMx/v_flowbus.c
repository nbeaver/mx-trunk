/*
 * Name:    v_flowbus.c
 *
 * Purpose: Support for Bronkhorst FLOW-BUS parameters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
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
#include "i_flowbus.h"
#include "v_flowbus.h"

MX_RECORD_FUNCTION_LIST mxv_flowbus_parameter_record_function_list = {
	mx_variable_initialize_driver,
	mxv_flowbus_create_record_structures
};

MX_VARIABLE_FUNCTION_LIST mxv_flowbus_parameter_variable_function_list = {
	mxv_flowbus_send_variable,
	mxv_flowbus_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_flowbus_string_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_FLOWBUS_PARAMETER_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_flowbus_string_num_record_fields
	= sizeof( mxv_flowbus_string_field_defaults )
	/ sizeof( mxv_flowbus_string_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_string_rfield_def_ptr
		= &mxv_flowbus_string_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_flowbus_uchar_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_FLOWBUS_PARAMETER_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UCHAR_VARIABLE_STANDARD_FIELDS
};

long mxv_flowbus_uchar_num_record_fields
	= sizeof( mxv_flowbus_uchar_field_defaults )
	/ sizeof( mxv_flowbus_uchar_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_uchar_rfield_def_ptr
		= &mxv_flowbus_uchar_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_flowbus_ushort_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_FLOWBUS_PARAMETER_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_USHORT_VARIABLE_STANDARD_FIELDS
};

long mxv_flowbus_ushort_num_record_fields
	= sizeof( mxv_flowbus_ushort_field_defaults )
	/ sizeof( mxv_flowbus_ushort_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_ushort_rfield_def_ptr
		= &mxv_flowbus_ushort_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_flowbus_ulong_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_FLOWBUS_PARAMETER_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_ULONG_VARIABLE_STANDARD_FIELDS
};

long mxv_flowbus_ulong_num_record_fields
	= sizeof( mxv_flowbus_ulong_field_defaults )
	/ sizeof( mxv_flowbus_ulong_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_ulong_rfield_def_ptr
		= &mxv_flowbus_ulong_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_flowbus_float_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_FLOWBUS_PARAMETER_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_FLOAT_VARIABLE_STANDARD_FIELDS
};

long mxv_flowbus_float_num_record_fields
	= sizeof( mxv_flowbus_float_field_defaults )
	/ sizeof( mxv_flowbus_float_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_float_rfield_def_ptr
		= &mxv_flowbus_float_field_defaults[0];

/*---*/

static mx_status_type
mxv_flowbus_get_pointers( MX_VARIABLE *variable,
			MX_FLOWBUS_PARAMETER **flowbus_parameter,
			MX_FLOWBUS **flowbus,
			const char *calling_fname )
{
	static const char fname[] = "mxv_flowbus_get_pointers()";

	MX_RECORD *flowbus_record;
	MX_FLOWBUS_PARAMETER *flowbus_parameter_ptr;

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

	flowbus_parameter_ptr = (MX_FLOWBUS_PARAMETER *)
				variable->record->record_type_struct;

	if ( flowbus_parameter != (MX_FLOWBUS_PARAMETER **) NULL ) {
		*flowbus_parameter = flowbus_parameter_ptr;
	}

	flowbus_record = flowbus_parameter_ptr->flowbus_record;

	if ( flowbus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"flowbus_record pointer for record '%s' passed by '%s' is NULL.",
			variable->record->name, calling_fname );
	}

	if ( flowbus_record->mx_type != MXI_CTRL_FLOWBUS ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"flowbus_record '%s' for FLOWBUS variable '%s' is not a FLOWBUS interface.  "
"Instead, it is a '%s' record.",
			flowbus_record->name, variable->record->name,
			mx_get_driver_name( flowbus_record ) );
	}

	if ( flowbus != (MX_FLOWBUS **) NULL ) {
		*flowbus = (MX_FLOWBUS *)
				flowbus_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_flowbus_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxv_flowbus_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_FLOWBUS_PARAMETER *flowbus_parameter;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_VARIABLE structure." );
	}

	flowbus_parameter = (MX_FLOWBUS_PARAMETER *)
				malloc( sizeof(MX_FLOWBUS_PARAMETER) );

	if ( flowbus_parameter == (MX_FLOWBUS_PARAMETER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_FLOWBUS_PARAMETER structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = flowbus_parameter;

	record->superclass_specific_function_list =
				&mxv_flowbus_parameter_variable_function_list;
	record->class_specific_function_list = NULL;

	flowbus_parameter->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_flowbus_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_flowbus_send_variable()";

	MX_FLOWBUS_PARAMETER *flowbus_parameter = NULL;
	MX_FLOWBUS *flowbus = NULL;
	MX_RECORD_FIELD *field = NULL;
	long num_dimensions, field_type;
	long *dimension_array;
	void *value_ptr;
	char flowbus_status_response[80];
	mx_status_type mx_status;

	mx_status = mxv_flowbus_get_pointers( variable,
				&flowbus_parameter, &flowbus, fname );

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

	if ( num_dimensions != 1) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Only 1-dimensional parameters are supported "
		"for 'flowbus_parameter' records, but record '%s' "
		"is %ld-dimensional.",
			variable->record->name,
			num_dimensions );
	}

	mx_status = mxi_flowbus_send_parameter( flowbus,
					flowbus_parameter->node_address,
					flowbus_parameter->process_number,
					flowbus_parameter->parameter_number,
					value_ptr,
					flowbus_status_response,
					sizeof(flowbus_status_response) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_flowbus_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_flowbus_receive_variable()";

	MX_FLOWBUS_PARAMETER *flowbus_parameter = NULL;
	MX_FLOWBUS *flowbus = NULL;
	MX_RECORD_FIELD *field = NULL;
	long num_dimensions, field_type;
	long *dimension_array;
	void *value_ptr;
	mx_status_type mx_status;

	mx_status = mxv_flowbus_get_pointers( variable,
				&flowbus_parameter, &flowbus, fname );

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

	if ( num_dimensions != 1 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Only 1-dimensional parameters are supported "
		"for 'flowbus_parameter' records, but record '%s' "
		"is %ld-dimensional.",
			variable->record->name,
			num_dimensions );
	}

	mx_status = mxi_flowbus_request_parameter( flowbus,
					flowbus_parameter->node_address,
					flowbus_parameter->process_number,
					flowbus_parameter->parameter_number,
					value_ptr,
					80 /* FIXME */ );

	return mx_status;
}

