/*
 * Name:    v_bluice_command.c
 *
 * Purpose: Support for Blu-Ice variables.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_COMMAND_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_thread.h"
#include "mx_mutex.h"
#include "mx_variable.h"

#include "mx_bluice.h"
#include "n_bluice_dcss.h"
#include "v_bluice_command.h"

MX_RECORD_FUNCTION_LIST mxv_bluice_command_record_function_list = {
	mx_variable_initialize_type,
	mxv_bluice_command_create_record_structures
};

MX_VARIABLE_FUNCTION_LIST mxv_bluice_command_variable_function_list = {
	mxv_bluice_command_send_variable,
	mxv_bluice_command_receive_variable
};

MX_RECORD_FIELD_DEFAULTS mxv_bluice_command_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_BLUICE_COMMAND_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_bluice_command_num_record_fields
	= sizeof( mxv_bluice_command_field_defaults )
	/ sizeof( mxv_bluice_command_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_bluice_command_rfield_def_ptr
		= &mxv_bluice_command_field_defaults[0];

/*---*/

static mx_status_type
mxv_bluice_command_get_pointers( MX_VARIABLE *variable,
			MX_BLUICE_COMMAND **bluice_command,
			const char *calling_fname )
{
	static const char fname[] = "mxv_bluice_command_get_pointers()";

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( bluice_command == (MX_BLUICE_COMMAND **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_COMMAND pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*bluice_command = (MX_BLUICE_COMMAND *)
				variable->record->record_type_struct;

	if ( *bluice_command == (MX_BLUICE_COMMAND *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_BLUICE_COMMAND pointer for Blu-Ice master "
			"record '%s' passed by '%s' is NULL.",
				variable->record->name, calling_fname );
	}

	if ( (*bluice_command)->bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The 'bluice_server_record' pointer for Blu-Ice "
			"master record '%s' passed by '%s' is NULL.",
				variable->record->name, calling_fname );
	}

	if ( (*bluice_command)->bluice_server_record->mx_type
					!= MXN_BLUICE_DCSS_SERVER )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Blu-Ice server record '%s' used by master variable '%s' "
		"is not a DCSS server.  Only 'bluice_dcss_server' records "
		"are allowed.", 
			(*bluice_command)->bluice_server_record->name,
			variable->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_bluice_command_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxv_bluice_command_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_BLUICE_COMMAND *bluice_command;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	bluice_command = (MX_BLUICE_COMMAND *)
				malloc( sizeof(MX_BLUICE_COMMAND) );

	if ( bluice_command == (MX_BLUICE_COMMAND *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_BLUICE_COMMAND structure.");
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = bluice_command;

	record->superclass_specific_function_list =
				&mxv_bluice_command_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_command_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_bluice_command_send_variable()";

	MX_BLUICE_COMMAND *bluice_command;
	void *value_ptr;
	char *command_ptr;
	mx_status_type mx_status;

	bluice_command = NULL;		/* Suppress annoying GCC 4 warning. */

	mx_status = mxv_bluice_command_get_pointers( variable,
					&bluice_command, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	command_ptr = (char *) value_ptr;

#if BLUICE_COMMAND_DEBUG
	MX_DEBUG(-2,("%s: command_ptr = '%s'", fname, command_ptr ));
#endif

	mx_status = mx_bluice_send_message(
				bluice_command->bluice_server_record,
				command_ptr, NULL, 0, -1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_bluice_command_receive_variable( MX_VARIABLE *variable )
{
	return MX_SUCCESSFUL_RESULT;
}

