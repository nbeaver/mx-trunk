/*
 * Name:    d_scipe_timer.c
 *
 * Purpose: MX timer driver for SCIPE detectors used as timers.
 *
 *--------------------------------------------------------------------------
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
#include "mx_timer.h"
#include "i_scipe.h"
#include "d_scipe_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_scipe_timer_record_function_list = {
	mxd_scipe_timer_initialize_type,
	mxd_scipe_timer_create_record_structures,
	mxd_scipe_timer_finish_record_initialization,
	mxd_scipe_timer_delete_record,
	NULL,
	mxd_scipe_timer_read_parms_from_hardware,
	mxd_scipe_timer_write_parms_to_hardware,
	mxd_scipe_timer_open,
	mxd_scipe_timer_close
};

MX_TIMER_FUNCTION_LIST mxd_scipe_timer_timer_function_list = {
	mxd_scipe_timer_is_busy,
	mxd_scipe_timer_start,
	mxd_scipe_timer_stop,
	mxd_scipe_timer_clear,
	mxd_scipe_timer_read,
	mxd_scipe_timer_get_mode,
	mxd_scipe_timer_set_mode,
	mxd_scipe_timer_set_modes_of_associated_counters
};

/* SCIPE timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_scipe_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_SCIPE_TIMER_STANDARD_FIELDS
};

long mxd_scipe_timer_num_record_fields
		= sizeof( mxd_scipe_timer_record_field_defaults )
		  / sizeof( mxd_scipe_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_scipe_timer_rfield_def_ptr
			= &mxd_scipe_timer_record_field_defaults[0];

#define SCIPE_TIMER_DEBUG FALSE

/* A private function for the use of the driver. */

static mx_status_type
mxd_scipe_timer_get_pointers( MX_TIMER *timer,
			MX_SCIPE_TIMER **scipe_timer,
			MX_SCIPE_SERVER **scipe_server,
			const char *calling_fname )
{
	const char fname[] = "mxd_scipe_timer_get_pointers()";

	MX_RECORD *scipe_server_record;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scipe_timer == (MX_SCIPE_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCIPE_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scipe_server == (MX_SCIPE_SERVER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCIPE_SERVER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( timer->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_TIMER pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*scipe_timer = (MX_SCIPE_TIMER *) timer->record->record_type_struct;

	if ( *scipe_timer == (MX_SCIPE_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCIPE_TIMER pointer for record '%s' is NULL.",
			timer->record->name );
	}

	scipe_server_record = (*scipe_timer)->scipe_server_record;

	if ( scipe_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'scipe_server_record' pointer for record '%s' is NULL.",
			timer->record->name );
	}

	*scipe_server = (MX_SCIPE_SERVER *)
				scipe_server_record->record_type_struct;

	if ( *scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SCIPE_SERVER pointer for the SCIPE server '%s' used by '%s' is NULL.",
			scipe_server_record->name, timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_scipe_timer_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_scipe_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_SCIPE_TIMER *scipe_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	scipe_timer = (MX_SCIPE_TIMER *)
				malloc( sizeof(MX_SCIPE_TIMER) );

	if ( scipe_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCIPE_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = scipe_timer;
	record->class_specific_function_list
			= &mxd_scipe_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_finish_record_initialization( MX_RECORD *record )
{
	return mx_timer_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_scipe_timer_delete_record( MX_RECORD *record )
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
mxd_scipe_timer_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_open( MX_RECORD *record )
{
	const char fname[] = "mxd_scipe_timer_open()";

	MX_TIMER *timer;
	MX_SCIPE_TIMER *scipe_timer;
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

	timer = (MX_TIMER *) (record->record_class_struct);

	mx_status = mxd_scipe_timer_get_pointers( timer,
				&scipe_timer, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->mode = MXCM_PRESET_MODE;

	/* Find out whether or not the SCIPE server knows about this timer. */

	sprintf( command, "%s desc", scipe_timer->scipe_timer_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for timer '%s'.  Response = '%s'",
				command, record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_is_busy( MX_TIMER *timer )
{
	const char fname[] = "mxd_scipe_timer_is_busy()";

	MX_SCIPE_TIMER *scipe_timer;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_timer_get_pointers( timer,
				&scipe_timer, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s status", scipe_timer->scipe_timer_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scipe_response_code ) {
	case MXF_SCIPE_NOT_COLLECTING:
		timer->busy = FALSE;
		break;

	case MXF_SCIPE_COLLECTING:
		timer->busy = TRUE;
		break;

	default:
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for timer '%s'.  Response = '%s'",
				command, timer->record->name, response );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_start( MX_TIMER *timer )
{
	const char fname[] = "mxd_scipe_timer_start()";

	MX_SCIPE_TIMER *scipe_timer;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	double seconds;
	unsigned long timer_preset;
	mx_status_type mx_status;

	mx_status = mxd_scipe_timer_get_pointers( timer,
				&scipe_timer, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds = timer->value;

	timer_preset = mx_round( seconds * scipe_timer->clock_frequency );

	sprintf( command, "%s preset %lu",
			scipe_timer->scipe_timer_name,
			timer_preset );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for timer '%s'.  Response = '%s'",
				command, timer->record->name, response );
	}

	sprintf( command, "%s start", scipe_timer->scipe_timer_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for timer '%s'.  Response = '%s'",
				command, timer->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_stop( MX_TIMER *timer )
{
	const char fname[] = "mxd_scipe_timer_stop()";

	MX_SCIPE_TIMER *scipe_timer;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_timer_get_pointers( timer,
				&scipe_timer, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s stop", scipe_timer->scipe_timer_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for timer '%s'.  Response = '%s'",
				command, timer->record->name, response );
	}

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_clear( MX_TIMER *timer )
{
	const char fname[] = "mxd_scipe_timer_clear()";

	MX_SCIPE_TIMER *scipe_timer;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_timer_get_pointers( timer,
				&scipe_timer, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s clear", scipe_timer->scipe_timer_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for timer '%s'.  Response = '%s'",
				command, timer->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_read( MX_TIMER *timer )
{
	const char fname[] = "mxd_scipe_timer_read()";

	MX_SCIPE_TIMER *scipe_timer;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int num_items, scipe_response_code;
	double timer_raw_value;
	mx_status_type mx_status;

	mx_status = mxd_scipe_timer_get_pointers( timer,
				&scipe_timer, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a read command to the timer. */

	sprintf( command, "%s read", scipe_timer->scipe_timer_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof(response),
				&scipe_response_code, &result_ptr,
				SCIPE_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for timer '%s'.  Response = '%s'",
				command, timer->record->name, response );
	}

	/* Parse the result string to get the timer value. */

	num_items = sscanf( result_ptr, "%lg", &timer_raw_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot interpret the response to the '%s' command for timer '%s' "
	"as a timer value.  Response = '%s'",
			command, timer->record->name, response );
	}

	timer->value = mx_divide_safely( timer_raw_value,
					scipe_timer->clock_frequency );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_get_mode( MX_TIMER *timer )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_set_mode( MX_TIMER *timer )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_timer_set_modes_of_associated_counters( MX_TIMER *timer )
{
	return MX_SUCCESSFUL_RESULT;
}

