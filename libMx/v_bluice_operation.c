/*
 * Name:    v_bluice_operation.c
 *
 * Purpose: Support for Blu-Ice 'operation' messages.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008, 2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_OPERATION_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"

#include "mx_bluice.h"
#include "n_bluice_dcss.h"
#include "n_bluice_dhs.h"
#include "n_bluice_dhs_manager.h"
#include "v_bluice_operation.h"

MX_RECORD_FUNCTION_LIST mxv_bluice_operation_record_function_list = {
	mx_variable_initialize_driver,
	mxv_bluice_operation_create_record_structures,
	mxv_bluice_operation_finish_record_initialization,
	NULL,
	NULL,
	mxv_bluice_operation_open
};

MX_VARIABLE_FUNCTION_LIST mxv_bluice_operation_variable_function_list = {
	mxv_bluice_operation_send_variable,
	mxv_bluice_operation_receive_variable
};

MX_RECORD_FIELD_DEFAULTS mxv_bluice_operation_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_BLUICE_OPERATION_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_bluice_operation_num_record_fields
	= sizeof( mxv_bluice_operation_field_defaults )
	/ sizeof( mxv_bluice_operation_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_bluice_operation_rfield_def_ptr
		= &mxv_bluice_operation_field_defaults[0];

/*---*/

static mx_status_type
mxv_bluice_operation_get_pointers( MX_VARIABLE *variable,
			MX_BLUICE_OPERATION **bluice_operation,
			MX_BLUICE_SERVER **bluice_server,
			MX_BLUICE_FOREIGN_DEVICE **foreign_operation,
			mx_bool_type skip_foreign_device_check,
			const char *calling_fname )
{
	static const char fname[] = "mxv_bluice_operation_get_pointers()";

	MX_BLUICE_OPERATION *bluice_operation_ptr;
	MX_RECORD *bluice_server_record;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( bluice_operation == (MX_BLUICE_OPERATION **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_OPERATION pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_operation_ptr = (MX_BLUICE_OPERATION *)
				variable->record->record_type_struct;

	if ( bluice_operation_ptr == (MX_BLUICE_OPERATION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_OPERATION pointer for record '%s' is NULL.",
			variable->record->name );
	}

	if ( bluice_operation != (MX_BLUICE_OPERATION **) NULL ) {
		*bluice_operation = bluice_operation_ptr;
	}

	bluice_server_record = bluice_operation_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", variable->record->name );
	}

	switch( bluice_server_record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
	case MXN_BLUICE_DHS_SERVER:
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Blu-Ice server record '%s' should be either of type "
		"'bluice_dcss_server' or 'bluice_dhs_server'.  Instead, it is "
		"of type '%s'.",
			bluice_server_record->name,
			mx_get_driver_name( bluice_server_record ) );
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = (MX_BLUICE_SERVER *)
				bluice_server_record->record_class_struct;

		if ( (*bluice_server) == (MX_BLUICE_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BLUICE_SERVER pointer for Blu-Ice server "
			"record '%s' used by record '%s' is NULL.",
				bluice_server_record->name,
				variable->record->name );
		}
	}

	if ( skip_foreign_device_check ) {
		return MX_SUCCESSFUL_RESULT;
	}

	if ( foreign_operation != (MX_BLUICE_FOREIGN_DEVICE **) NULL ) {
		*foreign_operation = bluice_operation_ptr->foreign_device;

		if ((*foreign_operation) == (MX_BLUICE_FOREIGN_DEVICE *) NULL) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"The foreign_operation pointer for operation '%s' "
			"has not been initialized.", variable->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_bluice_operation_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxv_bluice_operation_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_BLUICE_OPERATION *bluice_operation;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	bluice_operation = (MX_BLUICE_OPERATION *)
				malloc( sizeof(MX_BLUICE_OPERATION) );

	if ( bluice_operation == (MX_BLUICE_OPERATION *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_BLUICE_OPERATION structure.");
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = bluice_operation;

	record->superclass_specific_function_list =
				&mxv_bluice_operation_variable_function_list;
	record->class_specific_function_list = NULL;

	bluice_operation->foreign_device = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_operation_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_bluice_operation_finish_record_initialization()";

	MX_VARIABLE *variable;
	MX_BLUICE_OPERATION *bluice_operation;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *fdev;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	/* The foreign device pointer for DCSS operations will be initialized
	 * later, when DCSS sends us the necessary configuration information.
	 */

	if ( record->mx_type == MXV_BLUICE_DCSS_OPERATION ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* The foreign device pointer for DHS operations will be initialized
	 * _now_, since _we_ are the source of the configuration information.
	 */

	variable = record->record_superclass_struct;

	mx_status = mxv_bluice_operation_get_pointers(
					variable, &bluice_operation,
					&bluice_server, NULL, TRUE, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_setup_device_pointer(
					bluice_server,
					bluice_operation->bluice_name,
					&(bluice_server->operation_array),
					&(bluice_server->num_operations),
					&fdev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_operation->foreign_device = fdev;

	fdev->foreign_type = MXT_BLUICE_FOREIGN_OPERATION;

	bluice_dhs_server = bluice_server->record->record_type_struct;

	if ( bluice_dhs_server == (MX_BLUICE_DHS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_DHS_SERVER pointer for "
		"Blu-Ice DHS server '%s' is NULL.",
			bluice_server->record->name );
	}

	strlcpy( fdev->dhs_server_name,
		bluice_dhs_server->dhs_name,
		MXU_BLUICE_DHS_NAME_LENGTH );

	fdev->u.operation.arguments_buffer = NULL;

	fdev->u.operation.arguments_length = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_operation_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_bluice_operation_open()";

	MX_VARIABLE *variable;
	MX_BLUICE_OPERATION *bluice_operation;
	MX_RECORD *bluice_server_record;
	MX_BLUICE_SERVER *bluice_server;
	long num_dimensions, field_type;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_bluice_operation_get_pointers(
				variable, &bluice_operation,
				&bluice_server, NULL, FALSE, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_server_record = bluice_operation->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mx_get_variable_parameters( record, &num_dimensions, NULL,
						&field_type, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( field_type != MXFT_STRING ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX field type for operation variable '%s' is "
		"not MXFT_OPERATION (%d).  Instead, it is %ld.  "
		"This should not be able to happen.",
			record->name, MXFT_STRING, field_type );
	}

	if ( num_dimensions != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Blu-Ice operation record '%s' should be a 1-dimensional "
		"string.  Instead, it is a %ld-dimensional array.",
			record->name, num_dimensions );
	}

#if BLUICE_OPERATION_DEBUG
	MX_DEBUG(-2,
	    ("%s: About to wait for device pointer initialization.", fname));
#endif

	mx_status = mx_bluice_wait_for_device_pointer_initialization(
					bluice_server,
					bluice_operation->bluice_name,
					MXT_BLUICE_FOREIGN_OPERATION,
					&(bluice_server->operation_array),
					&(bluice_server->num_operations),
					&(bluice_operation->foreign_device),
					5.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if BLUICE_OPERATION_DEBUG
	MX_DEBUG(-2,
	("%s: Successfully waited for device pointer initialization.", fname));
#endif

	bluice_operation->foreign_device->u.operation.mx_operation_variable
		= variable;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_operation_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_bluice_operation_send_variable()";

	MX_BLUICE_OPERATION *bluice_operation;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_operation;
	void *value_ptr;
	char command[500];
	uint32_t operation_counter;
	mx_status_type mx_status;

	mx_status = mxv_bluice_operation_get_pointers(
					variable, &bluice_operation,
					&bluice_server, &foreign_operation,
					FALSE, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	operation_counter = mx_bluice_update_operation_counter( bluice_server );

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		bluice_dcss_server = bluice_server->record->record_type_struct;

		if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BLUICE_DCSS_SERVER pointer for "
			"record '%s' is NULL.", bluice_server->record->name );
		}

		snprintf( command, sizeof(command),
			"gtos_start_operation %s %lu.%lu %s",
			foreign_operation->name,
			bluice_dcss_server->client_number,
			operation_counter,
			(char *) value_ptr );
		break;

	case MXN_BLUICE_DHS_SERVER:
		snprintf( command, sizeof(command),
			"stoh_start_operation %s %lu.%lu %s",
			foreign_operation->name,
			1L,			/* DCSS is always client 1. */
			operation_counter,
			(char *) value_ptr );
		break;
	}

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: command = '%s'", fname, command));

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0 );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_update_operation_status( bluice_server,
							command );
	return mx_status;
}

MX_EXPORT mx_status_type
mxv_bluice_operation_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_bluice_operation_receive_variable()";

	MX_BLUICE_OPERATION *bluice_operation;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_operation;
	long *dimension_array;
	void *value_ptr;
	mx_status_type mx_status;
	long mx_status_code;

	mx_status = mxv_bluice_operation_get_pointers(
					variable, &bluice_operation,
					&bluice_server, &foreign_operation,
					FALSE, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_parameters( variable->record, NULL,
						&dimension_array, NULL,
						&value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	if ( (foreign_operation->u.operation.arguments_buffer == NULL)
	  || (foreign_operation->u.operation.arguments_length == 0) )
	{
		strlcpy( value_ptr, "", dimension_array[0] );
	} else {
		strlcpy( value_ptr,
			foreign_operation->u.operation.arguments_buffer,
			dimension_array[0] );
	}

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return mx_status;
}

