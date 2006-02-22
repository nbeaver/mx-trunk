/*
 * Name:    d_scipe_scaler.c
 *
 * Purpose: MX scaler driver for SCIPE detectors used as scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "i_scipe.h"
#include "d_scipe_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_scipe_scaler_record_function_list = {
	mxd_scipe_scaler_initialize_type,
	mxd_scipe_scaler_create_record_structures,
	mxd_scipe_scaler_finish_record_initialization,
	mxd_scipe_scaler_delete_record,
	mxd_scipe_scaler_print_structure,
	mxd_scipe_scaler_read_parms_from_hardware,
	mxd_scipe_scaler_write_parms_to_hardware,
	mxd_scipe_scaler_open,
	mxd_scipe_scaler_close
};

MX_SCALER_FUNCTION_LIST mxd_scipe_scaler_scaler_function_list = {
	mxd_scipe_scaler_clear,
	mxd_scipe_scaler_overflow_set,
	mxd_scipe_scaler_read,
	NULL,
	mxd_scipe_scaler_is_busy,
	mxd_scipe_scaler_start,
	mxd_scipe_scaler_stop,
	mxd_scipe_scaler_get_parameter,
	mxd_scipe_scaler_set_parameter,
	mxd_scipe_scaler_set_modes_of_associated_counters
};

/* SCIPE scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_scipe_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_SCIPE_SCALER_STANDARD_FIELDS
};

long mxd_scipe_scaler_num_record_fields
		= sizeof( mxd_scipe_scaler_record_field_defaults )
		  / sizeof( mxd_scipe_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_scipe_scaler_rfield_def_ptr
			= &mxd_scipe_scaler_record_field_defaults[0];

#define SCIPE_SCALER_DEBUG FALSE

/* A private function for the use of the driver. */

static mx_status_type
mxd_scipe_scaler_get_pointers( MX_SCALER *scaler,
			MX_SCIPE_SCALER **scipe_scaler,
			MX_SCIPE_SERVER **scipe_server,
			const char *calling_fname )
{
	const char fname[] = "mxd_scipe_scaler_get_pointers()";

	MX_RECORD *scipe_server_record;

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scipe_scaler == (MX_SCIPE_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCIPE_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scipe_server == (MX_SCIPE_SERVER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCIPE_SERVER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scaler->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_SCALER pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*scipe_scaler = (MX_SCIPE_SCALER *) scaler->record->record_type_struct;

	if ( *scipe_scaler == (MX_SCIPE_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCIPE_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	scipe_server_record = (*scipe_scaler)->scipe_server_record;

	if ( scipe_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'scipe_server_record' pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	*scipe_server = (MX_SCIPE_SERVER *)
				scipe_server_record->record_type_struct;

	if ( *scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SCIPE_SERVER pointer for the SCIPE server '%s' used by '%s' is NULL.",
			scipe_server_record->name, scaler->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_scipe_scaler_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_scipe_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_SCIPE_SCALER *scipe_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	scipe_scaler = (MX_SCIPE_SCALER *)
				malloc( sizeof(MX_SCIPE_SCALER) );

	if ( scipe_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCIPE_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = scipe_scaler;
	record->class_specific_function_list
			= &mxd_scipe_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_finish_record_initialization( MX_RECORD *record )
{
	MX_SCALER *scaler;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler = (MX_SCALER *) record->record_class_struct;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_delete_record( MX_RECORD *record )
{
	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_scipe_scaler_print_structure()";

	MX_SCALER *scaler;
	MX_SCIPE_SCALER *scipe_scaler;
	long current_value;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	scaler = (MX_SCALER *) (record->record_class_struct);

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SCALER pointer for record '%s' is NULL.", record->name);
	}

	scipe_scaler = (MX_SCIPE_SCALER *) (record->record_type_struct);

	if ( scipe_scaler == (MX_SCIPE_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SCIPE_SCALER pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "SCALER parameters for scaler '%s':\n", record->name);

	fprintf(file, "  Scaler type           = SCIPE_SCALER.\n\n");
	fprintf(file, "  SCIPE server          = %s\n",
				scipe_scaler->scipe_server_record->name);
	fprintf(file, "  SCIPE scaler name     = %s\n",
				scipe_scaler->scipe_scaler_name);

	status = mx_scaler_read( record, &current_value );

	if ( status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read value of scaler '%s'",
			record->name );
	}

	fprintf(file, "  present value         = %ld\n", current_value);
	fprintf(file, "  dark current          = %g counts per second.\n",
						scaler->dark_current);
	fprintf(file, "  scaler flags          = %#lx\n", scaler->scaler_flags);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_open( MX_RECORD *record )
{
	const char fname[] = "mxd_scipe_scaler_open()";

	MX_SCALER *scaler;
	MX_SCIPE_SCALER *scipe_scaler;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	scaler = (MX_SCALER *) (record->record_class_struct);

	mx_status = mxd_scipe_scaler_get_pointers( scaler,
				&scipe_scaler, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out whether or not the SCIPE server knows about this scaler. */

	sprintf( command, "%s desc", scipe_scaler->scipe_scaler_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for scaler '%s'.  Response = '%s'",
				command, record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_clear( MX_SCALER *scaler )
{
	const char fname[] = "mxd_scipe_scaler_clear()";

	MX_SCIPE_SCALER *scipe_scaler;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_scaler_get_pointers( scaler,
				&scipe_scaler, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s clear", scipe_scaler->scipe_scaler_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for scaler '%s'.  Response = '%s'",
				command, scaler->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_overflow_set( MX_SCALER *scaler )
{
	const char fname[] = "mxd_scipe_scaler_overflow_set()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"SCIPE scalers cannot check for overflow status." );
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_read( MX_SCALER *scaler )
{
	const char fname[] = "mxd_scipe_scaler_read()";

	MX_SCIPE_SCALER *scipe_scaler;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int num_items, scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_scaler_get_pointers( scaler,
				&scipe_scaler, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a read command to the scaler. */

	sprintf( command, "%s read", scipe_scaler->scipe_scaler_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for scaler '%s'.  Response = '%s'",
				command, scaler->record->name, response );
	}

	/* Parse the result string to get the scaler value. */

	num_items = sscanf( result_ptr, "%ld", &(scaler->raw_value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot interpret the response to the '%s' command for scaler '%s' "
	"as a scaler value.  Response = '%s'",
			command, scaler->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_is_busy( MX_SCALER *scaler )
{
	const char fname[] = "mxd_scipe_scaler_is_busy()";

	MX_SCIPE_SCALER *scipe_scaler;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_scaler_get_pointers( scaler,
				&scipe_scaler, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s status", scipe_scaler->scipe_scaler_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scipe_response_code ) {
	case MXF_SCIPE_NOT_COLLECTING:
		scaler->busy = FALSE;
		break;

	case MXF_SCIPE_COLLECTING:
		scaler->busy = TRUE;
		break;

	default:
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for scaler '%s'.  Response = '%s'",
				command, scaler->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_start( MX_SCALER *scaler )
{
	const char fname[] = "mxd_scipe_scaler_start()";

	MX_SCIPE_SCALER *scipe_scaler;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_scaler_get_pointers( scaler,
				&scipe_scaler, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s preset %ld",
			scipe_scaler->scipe_scaler_name,
			scaler->raw_value );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for scaler '%s'.  Response = '%s'",
				command, scaler->record->name, response );
	}

	sprintf( command, "%s start", scipe_scaler->scipe_scaler_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for scaler '%s'.  Response = '%s'",
				command, scaler->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_stop( MX_SCALER *scaler )
{
	const char fname[] = "mxd_scipe_scaler_stop()";

	MX_SCIPE_SCALER *scipe_scaler;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_scaler_get_pointers( scaler,
				&scipe_scaler, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s stop", scipe_scaler->scipe_scaler_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_SCALER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for scaler '%s'.  Response = '%s'",
				command, scaler->record->name, response );
	}

	scaler->raw_value = 0L;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_get_parameter( MX_SCALER *scaler )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_set_parameter( MX_SCALER *scaler )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_scaler_set_modes_of_associated_counters( MX_SCALER *scaler )
{
	return MX_SUCCESSFUL_RESULT;
}

