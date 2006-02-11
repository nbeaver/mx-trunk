/*
 * Name:    d_als_robot_java.c
 *
 * Purpose: MX driver for the ALS sample changing robot initially designed
 *          for Tom Earnest's beamlines at the Advanced Light Source.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ALS_ROBOT_JAVA_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_sample_changer.h"

#include "d_als_robot_java.h"

MX_RECORD_FUNCTION_LIST mxd_als_robot_java_record_function_list = {
	NULL,
	mxd_als_robot_java_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_als_robot_java_open,
	NULL,
	NULL,
	NULL
};

MX_SAMPLE_CHANGER_FUNCTION_LIST mxd_als_robot_java_sample_changer_function_list
  = {
	mxd_als_robot_java_initialize,
	mxd_als_robot_java_shutdown,
	mxd_als_robot_java_mount_sample,
	mxd_als_robot_java_unmount_sample,
	mxd_als_robot_java_grab_sample,
	mxd_als_robot_java_ungrab_sample,
	mxd_als_robot_java_select_sample_holder,
	mxd_als_robot_java_unselect_sample_holder,
	mxd_als_robot_java_soft_abort,
	mxd_als_robot_java_immediate_abort,
	mxd_als_robot_java_idle,
	mxd_als_robot_java_reset,
	mxd_als_robot_java_get_status,
	mx_sample_changer_default_get_parameter_handler,
	mxd_als_robot_java_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_als_robot_java_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SAMPLE_CHANGER_STANDARD_FIELDS,
	MXD_ALS_ROBOT_JAVA_STANDARD_FIELDS
};

mx_length_type mxd_als_robot_java_num_record_fields
		= sizeof( mxd_als_robot_java_record_field_defaults )
			/ sizeof( mxd_als_robot_java_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_als_robot_java_rfield_def_ptr
			= &mxd_als_robot_java_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_als_robot_java_get_pointers( MX_SAMPLE_CHANGER *changer,
			MX_ALS_ROBOT_JAVA **als_robot_java,
			const char *calling_fname )
{
	static const char fname[] = "mxd_als_robot_java_get_pointers()";

	if ( changer == (MX_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAMPLE_CHANGER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( als_robot_java == (MX_ALS_ROBOT_JAVA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ALS_ROBOT_JAVA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*als_robot_java = (MX_ALS_ROBOT_JAVA *)
			changer->record->record_type_struct;

	if ( *als_robot_java == (MX_ALS_ROBOT_JAVA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ALS_ROBOT_JAVA pointer for record '%s' is NULL.",
			changer->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_als_robot_java_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_als_robot_java_create_record_structures()";

	MX_SAMPLE_CHANGER *changer;
	MX_ALS_ROBOT_JAVA *als_robot_java;

	changer = (MX_SAMPLE_CHANGER *)
				malloc( sizeof(MX_SAMPLE_CHANGER) );

	if ( changer == (MX_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SAMPLE_CHANGER structure." );
	}

	als_robot_java = (MX_ALS_ROBOT_JAVA *)
				malloc( sizeof(MX_ALS_ROBOT_JAVA) );

	if ( als_robot_java == (MX_ALS_ROBOT_JAVA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ALS_ROBOT_JAVA structure." );
	}

	record->record_class_struct = changer;
	record->record_type_struct = als_robot_java;
	record->class_specific_function_list = 
		&mxd_als_robot_java_sample_changer_function_list;

	changer->record = record;
	als_robot_java->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_als_robot_java_open()";

	MX_SAMPLE_CHANGER *changer;
	MX_ALS_ROBOT_JAVA *als_robot_java;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	changer = (MX_SAMPLE_CHANGER *) record->record_class_struct;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
			"control_request", NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_initialize( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_initialize()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				"init", NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_shutdown( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_shutdown()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				"shutdown", NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_mount_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_mount_sample()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	char command[100];
	unsigned long interaction_id;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	interaction_id = als_robot_java->interaction_id;

	(als_robot_java->interaction_id)++;

	sprintf( command, "run_op %lu mount %s %ld",
		interaction_id, changer->requested_sample_holder,
				(long) changer->requested_sample_id );

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				command, NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_unmount_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_unmount_sample()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	char command[100];
	unsigned long interaction_id;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	interaction_id = als_robot_java->interaction_id;

	(als_robot_java->interaction_id)++;

	sprintf( command, "run_op %lu unmount", interaction_id );

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				command, NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_grab_sample( MX_SAMPLE_CHANGER *changer )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_ungrab_sample( MX_SAMPLE_CHANGER *changer )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_select_sample_holder( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_select_sample_holder()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	char command[100];
	unsigned long interaction_id;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	interaction_id = als_robot_java->interaction_id;

	(als_robot_java->interaction_id)++;

	sprintf( command, "run_op %lu select %s",
		interaction_id, changer->requested_sample_holder );

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				command, NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_unselect_sample_holder( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] =
		"mxd_als_robot_java_unselect_sample_holder()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	char command[100];
	unsigned long interaction_id;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	interaction_id = als_robot_java->interaction_id;

	(als_robot_java->interaction_id)++;

	sprintf( command, "run_op %lu unselect", interaction_id );

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				command, NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_soft_abort( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_soft_abort()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				"abort", NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_immediate_abort( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_immediate_abort()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				"estop", NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_idle( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_idle()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_reset( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_reset()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				"reset", NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_get_status( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_status()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	char command[100];
	char response[100];
	int num_items, value;
	unsigned long current_sample_id;
	unsigned long interaction_id;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	interaction_id = als_robot_java->interaction_id;

	(als_robot_java->interaction_id)++;

	sprintf( command, "run_op %lu dev_status", interaction_id );

	mx_status = mxd_als_robot_java_command( changer, als_robot_java,
					command, response, sizeof(response),
					MXD_ALS_ROBOT_JAVA_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%s %lu %d",
			changer->current_sample_holder,
			&current_sample_id, &value );

	if ( num_items != 3 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not the expected response to a 'dev_status' command.  "
		"num_items = %d, response = '%s'", num_items, response );
	}

	changer->current_sample_id = current_sample_id;

	if ( strcmp( changer->current_sample_holder, "0" ) == 0 ) {
		changer->current_sample_holder[0] = '\0';
	}

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_set_parameter( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_als_robot_java_set_parameter()";

	MX_ALS_ROBOT_JAVA *als_robot_java;
	char command[100];
	unsigned long interaction_id;
	mx_status_type mx_status;

	mx_status = mxd_als_robot_java_get_pointers( changer,
						&als_robot_java, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	("%s invoked for sample changer '%s', parameter type '%s' (%d)",
		fname, changer->record->name,
		mx_get_field_label_string( changer->record,
					changer->parameter_type ),
		changer->parameter_type));

	switch( changer->parameter_type ) {
	case MXLV_CHG_COOLDOWN:
		interaction_id = als_robot_java->interaction_id;

		(als_robot_java->interaction_id)++;

		sprintf( command, "run_op %lu cooldown", interaction_id );

		mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				command, NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXLV_CHG_DEICE:
		interaction_id = als_robot_java->interaction_id;

		(als_robot_java->interaction_id)++;

		sprintf( command, "run_op %lu deice", interaction_id );

		mx_status = mxd_als_robot_java_command( changer, als_robot_java,
				command, NULL, 0, MXD_ALS_ROBOT_JAVA_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_sample_changer_default_set_parameter_handler(
								changer );
		break;
	}
	MX_DEBUG(-2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/* === Extra functions for the use of this driver. === */

static char *
mxd_als_robot_java_next_token( char *old_token_ptr )
{
	char *new_token_ptr, *ptr;
	size_t length;

	if ( old_token_ptr == NULL )
		return NULL;

	length = strcspn( old_token_ptr, " " );

	if ( length == 0 )
		return NULL;

	ptr = old_token_ptr + length;

	length = strspn( ptr, " " );

	if ( length == 0 )
		return NULL;

	new_token_ptr = ptr + length;

	*ptr = '\0';

	return new_token_ptr;
}

static mx_status_type
mxd_als_robot_java_parse_result_code( MX_SAMPLE_CHANGER *changer,
					char *command,
					char *result_code_ptr )
{
	static const char fname[] = "mxd_als_robot_java_parse_result_code()";

	if ( strcmp( result_code_ptr, "SUCCESS" ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	} else
	if ( strcmp( result_code_ptr, "CTRL_REQ_DENIED" ) == 0 ) {
		return mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
    "Control request '%s' failed since sample changer '%s' is in LOCAL mode.",
			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "HARDWARE_ERROR" ) == 0 ) {
		return mx_error( MXE_HARDWARE_FAULT, fname,
	"Command '%s' for sample changer '%s' failed due to a hardware fault.",
			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "COMM_ERROR" ) == 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
    "Command '%s' for sample changer '%s' failed during a socket input read.",
    			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "TIMEOUT" ) == 0 ) {
		return mx_error( MXE_TIMED_OUT, fname,
			"Command '%s' for sample changer '%s' timed out.",
			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "STATE_INCORRECT" ) == 0 ) {
		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		"Command '%s' for sample changer '%s' is not valid for "
		"the controller's current state.",
			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "UNKNOWN_COMMAND" ) == 0 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Sample changer '%s' did not recognize command '%s'.",
			changer->record->name, command );
	} else
	if ( strcmp( result_code_ptr, "WRONG_ARG_LIST" ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
    "Command '%s' for sample changer '%s' had the wrong number of arguments.",
    			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "DATA_ID_UNKNOWN" ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unknown data id for command '%s' sent to sample changer '%s'.",
			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "INVALID_DATA" ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Wrong data type or range in command '%s' for sample changer '%s'.",
			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "ACCESS_DENIED" ) == 0 ) {
		return mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
	"Command '%s' failed since sample changer '%s' is in LOCAL mode.",
			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "STOPPED" ) == 0 ) {
		return mx_error( MXE_INTERRUPTED, fname,
		"Command '%s' for sample changer '%s' did not complete.",
			command, changer->record->name );
	} else
	if ( strcmp( result_code_ptr, "UNSPECIFIED_ERR" ) == 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
	"Command '%s' failed for sample changer '%s' due to an unknown error.",
			command, changer->record->name );
	} else {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
    "Unrecognized result code '%s' from sample changer '%s' for command '%s'.",
    			result_code_ptr, changer->record->name, command );
	}
}

static mx_status_type
mxd_als_robot_java_parse_current_state( MX_SAMPLE_CHANGER *changer,
					char *current_state_ptr )
{
	static const char fname[] = "mxd_als_robot_java_parse_current_state()";

	changer->status = 0;

	if ( strcmp( current_state_ptr, "POWERED_UP" ) == 0 ) {
		/* changer->status |= MXSF_CHG_POWERED_UP; */
	} else
	if ( strcmp( current_state_ptr, "INITIALIZING" ) == 0 ) {
		changer->status |= MXSF_CHG_INITIALIZATION_IN_PROGRESS;
	} else
	if ( strcmp( current_state_ptr, "READY" ) == 0 ) {
		/* changer->status |= MXSF_CHG_READY_FOR_COMMANDS; */
	} else
	if ( strcmp( current_state_ptr, "BUSY" ) == 0 ) {
		changer->status |= MXSF_CHG_OPERATION_IN_PROGRESS;
	} else
	if ( strcmp( current_state_ptr, "MOUNTED" ) == 0 ) {
		changer->status |= MXSF_CHG_SAMPLE_MOUNTED;
	} else
	if ( strcmp( current_state_ptr, "SELECTED" ) == 0 ) {
		changer->status |= MXSF_CHG_SAMPLE_HOLDER_SELECTED;
	} else {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Received unrecognized current state '%s' for sample changer '%s'.",
		current_state_ptr, changer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_als_robot_java_parse_control_mode( MX_SAMPLE_CHANGER *changer,
					char *control_mode_ptr )
{
	static const char fname[] = "mxd_als_robot_java_parse_control_mode()";

	if ( strcmp( control_mode_ptr, "LOCAL" ) == 0 ) {
		changer->control_mode = MXF_CHG_CONTROL_MODE_LOCAL;
	} else
	if ( strcmp( control_mode_ptr, "RELEASED" ) == 0 ) {
		changer->control_mode = 0;
	} else
	if ( strcmp( control_mode_ptr, "REMOTE" ) == 0 ) {
		changer->control_mode = MXF_CHG_CONTROL_MODE_REMOTE;
	} else {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Received unrecognized control mode '%s' for sample changer '%s'.",
		control_mode_ptr, changer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_robot_java_command( MX_SAMPLE_CHANGER *changer,
			MX_ALS_ROBOT_JAVA *als_robot_java,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_als_robot_java_command()";

	char local_response_buffer[200];
	char *result_code_ptr, *current_state_ptr;
	char *control_mode_ptr, *return_values_ptr;
	mx_status_type mx_status;

	if ( als_robot_java == (MX_ALS_ROBOT_JAVA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL MX_ALS_ROBOT_JAVA pointer passed." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL command buffer pointer passed." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: sending '%s' to '%s'.",
			fname, command, als_robot_java->record->name));
	}

	/* Discard any existing characters in the input buffer. */

	mx_status = mx_rs232_discard_unread_input( als_robot_java->rs232_record,
							debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the command string. */

	mx_status = mx_rs232_putline( als_robot_java->rs232_record,
						command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response. */

	mx_status = mx_rs232_getline( als_robot_java->rs232_record,
					local_response_buffer,
					sizeof(local_response_buffer),
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: raw response = '%s' from '%s'",
				fname, local_response_buffer,
				als_robot_java->record->name));
	}
	/* Parse the response. */

	result_code_ptr = local_response_buffer;

	current_state_ptr = mxd_als_robot_java_next_token( result_code_ptr );

	if ( current_state_ptr == NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Could not find the current state after "
			"the result code '%s' from ALS robot '%s'.",
			result_code_ptr, als_robot_java->record->name );
	}

	mx_status = mxd_als_robot_java_parse_result_code( changer, command,
							result_code_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	control_mode_ptr = mxd_als_robot_java_next_token( current_state_ptr );

	if ( control_mode_ptr == NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Could not find the control mode after "
			"the current state '%s' from ALS robot '%s'.",
			current_state_ptr, als_robot_java->record->name );
	}

	mx_status = mxd_als_robot_java_parse_current_state( changer,
							current_state_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return_values_ptr = mxd_als_robot_java_next_token( control_mode_ptr );

	mx_status = mxd_als_robot_java_parse_control_mode( changer,
							control_mode_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Copy the response if requested. */

	if ( response != NULL ) {
		/* return_values_ptr being NULL is only an error if the
		 * response pointer is not NULL.
		 */

		if ( return_values_ptr == NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
				"Could not find the return values after "
				"the control mode '%s' from ALS robot '%s'.",
				control_mode_ptr, als_robot_java->record->name);
		}

		strlcpy( response, return_values_ptr,
				response_buffer_length );

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'.",
				fname, response, als_robot_java->record->name));
		}
	}

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

