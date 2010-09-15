/*
 * Name:    v_position_select.c
 *
 * Purpose: Support for motor position select records.  This variable,
 *          when set selects from a list of motor positions specified
 *          by the 'value' field of the record.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2010 Illinois Institute of Technology
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
#include "mx_motor.h"
#include "mx_variable.h"
#include "v_position_select.h"

MX_RECORD_FUNCTION_LIST mxv_position_select_record_function_list = {
	mxv_position_select_initialize_driver,
	mxv_position_select_create_record_structures,
	NULL,
	NULL,
	NULL,
	mx_receive_variable
};

MX_VARIABLE_FUNCTION_LIST mxv_position_select_variable_function_list = {
	mxv_position_select_send_variable,
	mxv_position_select_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_position_select_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_POSITION_SELECT_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_LONG_VARIABLE_STANDARD_FIELDS
};

long mxv_position_select_num_record_fields
	= sizeof( mxv_position_select_record_field_defaults )
	/ sizeof( mxv_position_select_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_position_select_rfield_def_ptr
		= &mxv_position_select_record_field_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxv_position_select_initialize_driver( MX_DRIVER *driver )
{
        MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
        long num_positions_varargs_cookie;
        mx_status_type mx_status;

	mx_status = mx_variable_initialize_driver( driver );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

        mx_status = mx_find_record_field_defaults_index( driver,
                        			"num_positions",
						&referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				&num_positions_varargs_cookie);

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

	mx_status = mx_find_record_field_defaults( driver,
						"position_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_positions_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_position_select_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_position_select_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_POSITION_SELECT *position_select;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	position_select = (MX_POSITION_SELECT *)
				malloc( sizeof(MX_POSITION_SELECT) );

	if ( position_select == (MX_POSITION_SELECT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_POSITION_SELECT structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = position_select;

	record->superclass_specific_function_list =
				&mxv_position_select_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_position_select_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_position_select_send_variable()";

	MX_POSITION_SELECT *position_select;
	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	long long_value;
	double requested_position;
	mx_status_type mx_status;

	position_select = (MX_POSITION_SELECT *)
				variable->record->record_type_struct;

	mx_status = mx_find_record_field(variable->record,
						"value", &value_field);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value_ptr = mx_get_field_value_pointer( value_field );

	long_value = *((long *) value_ptr);

	if ( ( long_value < 0 )
	  || ( long_value >= position_select->num_positions ) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested value %ld for position select record '%s' "
		"is outside the allowed range of 0-%ld",
			long_value, variable->record->name,
			position_select->num_positions );
	}

	/* Get the motor position that corresponds to the
	 * requested selection and move there.
	 */

	requested_position = position_select->position_array[ long_value ];

	mx_status = mx_motor_move_absolute( position_select->motor_record,
					requested_position, 0 );

	return mx_status;
}


MX_EXPORT mx_status_type
mxv_position_select_receive_variable( MX_VARIABLE *variable )
{
	return MX_SUCCESSFUL_RESULT;
}

