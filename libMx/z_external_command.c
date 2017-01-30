/*
 * Name:    z_external_command.c
 *
 * Purpose: Support for running external commands using mx_spawn().
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXZ_EXTERNAL_COMMAND_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"

#include "z_external_command.h"

MX_RECORD_FUNCTION_LIST mxz_external_command_record_function_list = {
	NULL,
	mxz_external_command_create_record_structures,
	mxz_external_command_finish_record_initialization,
	NULL,
	NULL,
	mxz_external_command_open
};

MX_RECORD_FIELD_DEFAULTS mxz_external_command_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXZ_EXTERNAL_COMMAND_STANDARD_FIELDS
};

long mxz_external_command_num_record_fields
	= sizeof( mxz_external_command_field_defaults )
	/ sizeof( mxz_external_command_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxz_external_command_rfield_def_ptr
		= &mxz_external_command_field_defaults[0];

/*---*/

static mx_status_type
mxz_external_command_get_pointers( MX_RECORD *record,
				MX_EXTERNAL_COMMAND **external_command,
				const char *calling_fname )
{
	static const char fname[] = "mxz_external_command_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( external_command == (MX_EXTERNAL_COMMAND **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EXTERNAL_COMMAND pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*external_command = (MX_EXTERNAL_COMMAND *) record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxz_external_command_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxz_external_command_create_record_structures()";

	MX_EXTERNAL_COMMAND *external_command;

	/* Allocate memory for the necessary structures. */

	external_command = (MX_EXTERNAL_COMMAND *)
	                        malloc( sizeof(MX_EXTERNAL_COMMAND) );

	if ( external_command == (MX_EXTERNAL_COMMAND *) NULL ) {
	        return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_EXTERNAL_COMMAND structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_superclass_struct = NULL;
	record->record_class_struct = NULL;
	record->record_type_struct = external_command;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	external_command->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxz_external_command_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxz_external_command_finish_record_initialization()";

	MX_EXTERNAL_COMMAND *external_command;
	MX_RECORD_FIELD *command_arguments_field;
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxz_external_command_get_pointers( record,
						&external_command, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = external_command->external_command_flags;

	if ( flags & MXF_EXTERNAL_COMMAND_READ_ONLY ) {
		mx_status = mx_find_record_field( record,
						"command_arguments",
						&command_arguments_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		command_arguments_field->flags |= MXFF_READ_ONLY;
	}

	if ( flags
	    & MXF_EXTERNAL_COMMAND_RUN_DURING_FINISH_RECORD_INITIALIZATION )
	{
		mx_status = mxz_external_command_run( external_command );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}


MX_EXPORT mx_status_type
mxz_external_command_open( MX_RECORD *record )
{
	static const char fname[] = "mxz_external_command_open()";

	MX_EXTERNAL_COMMAND *external_command;
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mx_status = mxz_external_command_get_pointers( record,
						&external_command, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = external_command->external_command_flags;

	if ( flags & MXF_EXTERNAL_COMMAND_RUN_DURING_OPEN ) {
		mx_status = mxz_external_command_run( external_command );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxz_external_command_run( MX_EXTERNAL_COMMAND *external_command )
{
	static const char fname[] = "mxz_external_command_run()";

	char command_line[ MXU_FILENAME_LENGTH
			+ MXU_MAX_EXTERNAL_COMMAND_LENGTH + 2 ];
	unsigned long process_id;
	mx_status_type mx_status;

	if ( external_command == (MX_EXTERNAL_COMMAND *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EXTERNAL_COMMAND pointer passed was NULL." );
	}

	strlcpy( command_line, external_command->command_name,
					sizeof(command_line) );

	strlcat( command_line, " ", sizeof(command_line) );

	strlcat( command_line, external_command->command_arguments,
					sizeof(command_line) );

#if MXZ_EXTERNAL_COMMAND_DEBUG
	MX_DEBUG(-2,("%s: Running command '%s'", fname, command_line ));
#endif /* MXZ_EXTERNAL_COMMAND_DEBUG */

	mx_status = mx_spawn( command_line, 0x0, &process_id );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXZ_EXTERNAL_COMMAND_DEBUG
	MX_DEBUG(-2,("%s: Spawned command '%s' with process id %lu",
			fname, command_line, process_id ));
#endif /* MXZ_EXTERNAL_COMMAND_DEBUG */

	mx_status = mx_wait_for_process_id( process_id, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXZ_EXTERNAL_COMMAND_DEBUG
	MX_DEBUG(-2,("%s: Process id %lu finished.", fname, process_id ));
#endif /* MXZ_EXTERNAL_COMMAND_DEBUG */

	return MX_SUCCESSFUL_RESULT;
}

