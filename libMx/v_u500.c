/*
 * Name:    v_u500.c
 *
 * Purpose: Support for U500 V variables.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_U500

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "i_u500.h"
#include "v_u500.h"

/* Aerotech includes */

#include "Aerotype.h"
#include "Build.h"
#include "Quick.h"
#include "Wapi.h"

MX_RECORD_FUNCTION_LIST mxv_u500_variable_record_function_list = {
	mx_variable_initialize_type,
	mxv_u500_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxv_u500_open
};

MX_VARIABLE_FUNCTION_LIST mxv_u500_variable_variable_function_list = {
	mxv_u500_send_variable,
	mxv_u500_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_u500_variable_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_U500_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_u500_variable_num_record_fields
	= sizeof( mxv_u500_variable_field_defaults )
	/ sizeof( mxv_u500_variable_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_u500_variable_rfield_def_ptr
		= &mxv_u500_variable_field_defaults[0];

/*---*/

static mx_status_type
mxv_u500_get_pointers( MX_VARIABLE *variable,
			MX_U500_VARIABLE **u500_variable,
			MX_U500 **u500,
			const char *calling_fname )
{
	static const char fname[] = "mxv_u500_get_pointers()";

	MX_RECORD *u500_record;
	MX_U500_VARIABLE *u500_variable_ptr;

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

	u500_variable_ptr = (MX_U500_VARIABLE *)
				variable->record->record_type_struct;

	if ( u500_variable != (MX_U500_VARIABLE **) NULL ) {
		*u500_variable = u500_variable_ptr;
	}

	u500_record = u500_variable_ptr->u500_record;

	if ( u500_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"u500_record pointer for record '%s' passed by '%s' is NULL.",
			variable->record->name, calling_fname );
	}

	if ( u500_record->mx_type != MXI_GEN_U500 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"u500_record '%s' for U500 variable '%s' is not a U500 controller.  "
"Instead, it is a '%s' record.",
			u500_record->name, variable->record->name,
			mx_get_driver_name( u500_record ) );
	}

	if ( u500 != (MX_U500 **) NULL ) {
		*u500 = (MX_U500 *) u500_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_u500_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxv_u500_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_U500_VARIABLE *u500_variable;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	u500_variable = (MX_U500_VARIABLE *) malloc( sizeof(MX_U500_VARIABLE) );

	if ( u500_variable == (MX_U500_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_U500_VARIABLE structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = u500_variable;

	record->superclass_specific_function_list =
				&mxv_u500_variable_variable_function_list;
	record->class_specific_function_list = NULL;

	u500_variable->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_u500_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_u500_open()";

	MX_VARIABLE *variable;
	MX_U500_VARIABLE *u500_variable;
	MX_U500 *u500;
	MX_RECORD_FIELD *field;
	long num_dimensions, field_type;
	long *dimension_array;
	void *field_value_ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = record->record_superclass_struct;

	mx_status = mxv_u500_get_pointers( variable,
				&u500_variable, &u500, fname );

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
						&field_value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (num_dimensions != 1) && (num_dimensions != 2) ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Only 1-dimensional or 2-dimensional arrays are supported "
		"for 'u500_variable' records, but record '%s' "
		"is %ld-dimensional.",
			variable->record->name,
			num_dimensions );
	}

	u500_variable->mx_vptr = field_value_ptr;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_u500_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_u500_send_variable()";

	MX_U500_VARIABLE *u500_variable;
	MX_U500 *u500;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxv_u500_get_pointers( variable,
				&u500_variable, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( u500_variable->mx_vptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mx_vptr pointer for variable '%s' is NULL.",
			variable->record->name );
	}

	snprintf( command, sizeof(command), "V%lu = %g",
		u500_variable->variable_number, *(u500_variable->mx_vptr) );

	mx_status = mxi_u500_command( u500,
			u500_variable->board_number, command );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_u500_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_u500_receive_variable()";

	MX_U500_VARIABLE *u500_variable;
	MX_U500 *u500;
	double u500_value;
	mx_status_type mx_status;

	mx_status = mxv_u500_get_pointers( variable,
				&u500_variable, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( u500_variable->mx_vptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mx_vptr pointer for variable '%s' is NULL.",
			variable->record->name );
	}

	u500_value = WAPIAerReadVariable( u500_variable->variable_number );

	*(u500_variable->mx_vptr) = u500_value;

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_U500 */

