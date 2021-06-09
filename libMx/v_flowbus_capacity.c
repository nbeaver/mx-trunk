/*
 * Name:    v_flowbus_capacity.c
 *
 * Purpose: Support for Bronkhorst FLOW-BUS capacity values.
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
#include "v_flowbus_capacity.h"

MX_RECORD_FUNCTION_LIST mxv_flowbus_capacity_record_function_list = {
	mx_variable_initialize_driver,
	mxv_flowbus_capacity_create_record_structures,
	mxv_flowbus_capacity_finish_record_initialization
};

MX_VARIABLE_FUNCTION_LIST mxv_flowbus_capacity_variable_function_list = {
	mxv_flowbus_capacity_send_variable,
	mxv_flowbus_capacity_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_flowbus_capacity_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_FLOWBUS_CAPACITY_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_FLOAT_VARIABLE_STANDARD_FIELDS
};

long mxv_flowbus_capacity_num_record_fields
	= sizeof( mxv_flowbus_capacity_field_defaults )
	/ sizeof( mxv_flowbus_capacity_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_flowbus_capacity_rfield_def_ptr
		= &mxv_flowbus_capacity_field_defaults[0];

/*---*/

static mx_status_type
mxv_flowbus_capacity_get_pointers( MX_VARIABLE *variable,
			MX_FLOWBUS_CAPACITY **flowbus_capacity,
			MX_FLOWBUS **flowbus,
			const char *calling_fname )
{
	static const char fname[] = "mxv_flowbus_capacity_get_pointers()";

	MX_RECORD *flowbus_record;
	MX_FLOWBUS_CAPACITY *flowbus_capacity_ptr;

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

	flowbus_capacity_ptr = (MX_FLOWBUS_CAPACITY *)
				variable->record->record_type_struct;

	if ( flowbus_capacity != (MX_FLOWBUS_CAPACITY **) NULL ) {
		*flowbus_capacity = flowbus_capacity_ptr;
	}

	flowbus_record = flowbus_capacity_ptr->flowbus_record;

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
mxv_flowbus_capacity_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_flowbus_capacity_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_FLOWBUS_CAPACITY *flowbus_capacity;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_VARIABLE structure." );
	}

	flowbus_capacity = (MX_FLOWBUS_CAPACITY *)
				malloc( sizeof(MX_FLOWBUS_CAPACITY) );

	if ( flowbus_capacity == (MX_FLOWBUS_CAPACITY *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_FLOWBUS_CAPACITY structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = flowbus_capacity;

	record->superclass_specific_function_list =
				&mxv_flowbus_capacity_variable_function_list;
	record->class_specific_function_list = NULL;

	flowbus_capacity->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_flowbus_capacity_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_flowbus_capacity_finish_record_initialization()";

	MX_VARIABLE *variable = NULL;
	MX_FLOWBUS_CAPACITY *flowbus_capacity = NULL;
	MX_FLOWBUS *flowbus = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_flowbus_capacity_get_pointers( variable,
				&flowbus_capacity, &flowbus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_flowbus_capacity_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_flowbus_capacity_send_variable()";

	MX_FLOWBUS_CAPACITY *flowbus_capacity = NULL;
	MX_FLOWBUS *flowbus = NULL;
	MX_RECORD_FIELD *field = NULL;
	long num_dimensions, field_type;
	long *dimension_array;
	void *value_ptr;
	char flowbus_status_response[80];
	mx_status_type mx_status;

	mx_status = mxv_flowbus_capacity_get_pointers( variable,
				&flowbus_capacity, &flowbus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If this is not a writeable parameter, then we return
	 * an error message that says so.
	 */

	if ( ( flowbus_capacity->access_mode
			& MXF_FLOWBUS_CAPACITY_WRITE ) == 0 )
	{
		return mx_error( MXE_READ_ONLY, fname,
		"Flowbus variable '%s' is read-only.", variable->record->name );
	}

	/* Get the variable parameters. */

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
		"for 'flowbus_capacity' records, but record '%s' "
		"is %ld-dimensional.",
			variable->record->name,
			num_dimensions );
	}

	/* Select the capacity unit. */

	mx_status = mxi_flowbus_send_parameter( flowbus,
					flowbus_capacity->node_address,
					1, 31, MXDT_FLOWBUS_STRING,
					flowbus_capacity->capacity_unit,
					flowbus_status_response,
					sizeof(flowbus_status_response), 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the new capacity value. */

	mx_status = mxi_flowbus_send_parameter( flowbus,
					flowbus_capacity->node_address,
					1, 13, MXDT_FLOWBUS_ULONG_FLOAT,
					value_ptr,
					flowbus_status_response,
					sizeof(flowbus_status_response), 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_flowbus_capacity_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_flowbus_capacity_receive_variable()";

	MX_FLOWBUS_CAPACITY *flowbus_capacity = NULL;
	MX_FLOWBUS *flowbus = NULL;
	MX_RECORD_FIELD *field = NULL;
	long num_dimensions, mx_field_type;
	long *dimension_array;
	void *value_ptr;
	long max_value_length_in_bytes;
	size_t *sizeof_array;
	char flowbus_status_response[80];
	mx_status_type mx_status;

	mx_status = mxv_flowbus_capacity_get_pointers( variable,
				&flowbus_capacity, &flowbus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If this is not a readable parameter, then we just return
	 * without doing anything.
	 */

	if ( ( flowbus_capacity->access_mode
			& MXF_FLOWBUS_CAPACITY_READ ) == 0 )
	{
		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the variable parameters */

	mx_status = mx_find_record_field( variable->record,
					"value", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_parameters( variable->record,
						&num_dimensions,
						&dimension_array,
						&mx_field_type,
						&value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_dimensions != 1 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Only 1-dimensional parameters are supported "
		"for 'flowbus_capacity' records, but record '%s' "
		"is %ld-dimensional.",
			variable->record->name,
			num_dimensions );
	}

	/* Compute the maximum length in bytes of the value. */

	mx_status = mx_get_datatype_sizeof_array( mx_field_type, &sizeof_array);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	max_value_length_in_bytes = sizeof_array[0] * dimension_array[0];

#if 0
	MX_DEBUG(-2,("%s: Variable '%s' max_value_length_in_bytes = %ld",
		fname, variable->record->name, max_value_length_in_bytes));
#endif

	/* Select the capacity unit. */

	mx_status = mxi_flowbus_send_parameter( flowbus,
					flowbus_capacity->node_address,
					1, 31, MXDT_FLOWBUS_STRING,
					flowbus_capacity->capacity_unit,
					flowbus_status_response,
					sizeof(flowbus_status_response), 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the capacity value. */

	mx_status = mxi_flowbus_request_parameter( flowbus,
					flowbus_capacity->node_address,
					1, 13, MXDT_FLOWBUS_STRING,
					value_ptr,
					max_value_length_in_bytes, 0 );

	return mx_status;
}

