/*
 * Name:    v_epics_timeout.c
 *
 * Purpose: Provides a way to set internal EPICS timeouts at MX startup time.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_epics.h"
#include "mx_variable.h"
#include "v_epics_timeout.h"

MX_RECORD_FUNCTION_LIST mxv_epics_timeout_record_function_list = {
	mx_variable_initialize_driver,
	mxv_epics_timeout_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxv_epics_timeout_open
};

MX_VARIABLE_FUNCTION_LIST mxv_epics_timeout_variable_function_list = {
	mxv_epics_timeout_send_variable,
	mxv_epics_timeout_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_epics_timeout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_epics_timeout_num_record_fields
	= sizeof( mxv_epics_timeout_record_field_defaults )
	/ sizeof( mxv_epics_timeout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_epics_timeout_field_def_ptr
		= &mxv_epics_timeout_record_field_defaults[0];

/*---*/

/********************************************************************/

MX_EXPORT mx_status_type
mxv_epics_timeout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxv_epics_timeout_create_record_structures()";

        MX_VARIABLE *variable_struct;
        MX_EPICS_TIMEOUT *epics_timeout;

        /* Allocate memory for the record's data structures. */

        variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

        if ( variable_struct == (MX_VARIABLE *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_VARIABLE structure." );
        }

        epics_timeout = (MX_EPICS_TIMEOUT *) malloc( sizeof(MX_EPICS_TIMEOUT) );

        if ( epics_timeout == (MX_EPICS_TIMEOUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an MX_EPICS_TIMEOUT structure." );
        }

        /* Now set up the necessary pointers. */

        variable_struct->record = record;
	epics_timeout->record = record;

        record->record_superclass_struct = variable_struct;
        record->record_class_struct = NULL;
        record->record_type_struct = epics_timeout;

        record->superclass_specific_function_list =
                                &mxv_epics_timeout_variable_function_list;
        record->class_specific_function_list = NULL;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_epics_timeout_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_epics_timeout_open()";

	MX_VARIABLE *variable;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_epics_timeout_send_variable( variable );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_epics_timeout_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_epics_timeout_send_variable()";

	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	double *double_ptr;
	mx_status_type mx_status;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed was NULL." );
	}

	mx_status = mx_find_record_field( variable->record,
						"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value_ptr = mx_get_field_value_pointer( value_field );

	double_ptr = (double *) value_ptr;

	mx_epics_set_connection_timeout( *double_ptr );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_epics_timeout_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_epics_timeout_receive_variable()";

	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	double *double_ptr;
	mx_status_type mx_status;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed was NULL." );
	}

	mx_status = mx_find_record_field( variable->record,
						"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value_ptr = mx_get_field_value_pointer( value_field );

	double_ptr = (double *) value_ptr;

	*double_ptr = mx_epics_get_connection_timeout();

	return MX_SUCCESSFUL_RESULT;
}

