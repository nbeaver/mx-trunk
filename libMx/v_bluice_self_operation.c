/*
 * Name:    v_bluice_self_operation.c
 *
 * Purpose: Support for Blu-Ice self operation placeholders.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_SELF_OPERATION_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"

#include "mx_bluice.h"
#include "v_bluice_self_operation.h"

MX_RECORD_FUNCTION_LIST mxv_bluice_self_operation_record_function_list = {
	mx_variable_initialize_type,
	mxv_bluice_self_operation_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxv_bluice_self_operation_open
};

MX_VARIABLE_FUNCTION_LIST mxv_bluice_self_operation_variable_function_list = {
	mxv_bluice_self_operation_send_variable,
	mxv_bluice_self_operation_receive_variable
};

MX_RECORD_FIELD_DEFAULTS mxv_bluice_self_operation_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_BLUICE_SELF_OPERATION_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_bluice_self_operation_num_record_fields
	= sizeof( mxv_bluice_self_operation_field_defaults )
	/ sizeof( mxv_bluice_self_operation_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_bluice_self_operation_rfield_def_ptr
		= &mxv_bluice_self_operation_field_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxv_bluice_self_operation_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxv_bluice_self_operation_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_BLUICE_SELF_OPERATION *bluice_self_operation;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	bluice_self_operation = (MX_BLUICE_SELF_OPERATION *)
				malloc( sizeof(MX_BLUICE_SELF_OPERATION) );

	if ( bluice_self_operation == (MX_BLUICE_SELF_OPERATION *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for an MX_BLUICE_SELF_OPERATION structure.");
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = bluice_self_operation;

	record->superclass_specific_function_list =
			&mxv_bluice_self_operation_variable_function_list;

	record->class_specific_function_list = NULL;

	bluice_self_operation->foreign_device = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_self_operation_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_bluice_self_operation_open()";

	MX_BLUICE_SELF_OPERATION *bluice_self_operation;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	bluice_self_operation = record->record_type_struct;

	if ( bluice_self_operation == (MX_BLUICE_SELF_OPERATION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SELF_OPERATION pointer for record '%s' is NULL.",
			record->name );
	}

#if 0
	MX_DEBUG(-2,("%s invoked for record '%s', record_ptr = %p",
		fname, record->name, record));
	MX_DEBUG(-2,("%s: bluice_self_operation = %p",
		fname, bluice_self_operation));
	MX_DEBUG(-2,("%s: bluice_server_name = '%s', bluice_name = '%s'",
		fname, bluice_self_operation->bluice_server_name,
		bluice_self_operation->bluice_name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_self_operation_send_variable( MX_VARIABLE *variable )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_self_operation_receive_variable( MX_VARIABLE *variable )
{
	return MX_SUCCESSFUL_RESULT;
}

