/*
 * Name:    v_bluice_master.c
 *
 * Purpose: Support for Blu-Ice variables.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXV_BLUICE_MASTER_DEBUG	TRUE

#include <stdio.h>

#include "mx_record.h"
#include "mx_driver.h"
#include "mx_types.h"
#include "mx_socket.h"
#include "mx_thread.h"
#include "mx_variable.h"
#include "mx_bluice.h"
#include "n_bluice_dcss.h"
#include "v_bluice_master.h"

MX_RECORD_FUNCTION_LIST mxv_bluice_master_record_function_list = {
	mx_variable_initialize_type,
	mxv_bluice_master_create_record_structures
};

MX_VARIABLE_FUNCTION_LIST mxv_bluice_master_variable_function_list = {
	mxv_bluice_master_send_variable,
	mxv_bluice_master_receive_variable
};

MX_RECORD_FIELD_DEFAULTS mxv_bluice_master_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_BLUICE_MASTER_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_INT_VARIABLE_STANDARD_FIELDS
};

long mxv_bluice_master_num_record_fields
	= sizeof( mxv_bluice_master_field_defaults )
	/ sizeof( mxv_bluice_master_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_bluice_master_rfield_def_ptr
		= &mxv_bluice_master_field_defaults[0];

/*---*/

static mx_status_type
mxv_bluice_master_get_pointers( MX_VARIABLE *variable,
			MX_BLUICE_MASTER **bluice_master,
			const char *calling_fname )
{
	static const char fname[] = "mxv_bluice_master_get_pointers()";

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( bluice_master == (MX_BLUICE_MASTER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_BLUICE_MASTER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*bluice_master = (MX_BLUICE_MASTER *)
				variable->record->record_type_struct;

	if ( *bluice_master == (MX_BLUICE_MASTER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_BLUICE_MASTER pointer for Blu-Ice master "
			"record '%s' passed by '%s' is NULL.",
				variable->record->name, calling_fname );
	}

	if ( (*bluice_master)->bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The 'bluice_server_record' pointer for Blu-Ice "
			"master record '%s' passed by '%s' is NULL.",
				variable->record->name, calling_fname );
	}

	if ( (*bluice_master)->bluice_server_record->mx_type
					!= MXN_BLUICE_DCSS_SERVER )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Blu-Ice server record '%s' used by master variable '%s' "
		"is not a DCSS server.  Only 'bluice_dcss_server' records "
		"are allowed.", 
			(*bluice_master)->bluice_server_record->name,
			variable->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_bluice_master_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxv_bluice_master_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_BLUICE_MASTER *bluice_master;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	bluice_master = (MX_BLUICE_MASTER *) malloc( sizeof(MX_BLUICE_MASTER) );

	if ( bluice_master == (MX_BLUICE_MASTER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_BLUICE_MASTER structure.");
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = bluice_master;

	record->superclass_specific_function_list =
				&mxv_bluice_master_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_master_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_bluice_master_send_variable()";

	MX_BLUICE_MASTER *bluice_master;
	char command[80];
	void *value_ptr;
	int take_master;
	mx_status_type mx_status;

	mx_status = mxv_bluice_master_get_pointers( variable,
					&bluice_master, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	take_master = *( (int *) value_ptr );

	MX_DEBUG(-2,("%s: take_master = %d", fname, take_master ));

	if ( take_master ) {
		strcpy( command, "gtos_become_master force" );
	} else {
		strcpy( command, "gtos_become_slave" );
	}

	mx_status = mx_bluice_send_message(
				bluice_master->bluice_server_record,
				command, NULL, 0, -1, TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_bluice_master_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_bluice_master_receive_variable()";

	MX_BLUICE_MASTER *bluice_master;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	void *value_ptr;
	int *master_ptr;
	mx_status_type mx_status;

	mx_status = mxv_bluice_master_get_pointers( variable,
						&bluice_master, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	master_ptr = (int *) value_ptr;

	bluice_dcss_server = (MX_BLUICE_DCSS_SERVER *)
		bluice_master->bluice_server_record->record_type_struct;

	if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BLUICE_DCSS_SERVER pointer for Blu-Ice "
			"server '%s' used by record '%s' is NULL.",
				bluice_master->bluice_server_record->name,
				variable->record->name );
	}

	*master_ptr = bluice_dcss_server->is_master;

	return MX_SUCCESSFUL_RESULT;
}

