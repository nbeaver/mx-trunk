/*
 * Name:    d_ortec974_scaler.c
 *
 * Purpose: MX scaler driver to control EG&G Ortec 974 scaler/counters.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "i_ortec974.h"
#include "d_ortec974_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_ortec974_scaler_record_function_list = {
	mxd_ortec974_scaler_initialize_type,
	mxd_ortec974_scaler_create_record_structures,
	mxd_ortec974_scaler_finish_record_initialization,
	mxd_ortec974_scaler_delete_record,
	NULL,
	mxd_ortec974_scaler_read_parms_from_hardware,
	mxd_ortec974_scaler_write_parms_to_hardware,
	mxd_ortec974_scaler_open,
	mxd_ortec974_scaler_close
};

MX_SCALER_FUNCTION_LIST mxd_ortec974_scaler_scaler_function_list = {
	mxd_ortec974_scaler_clear,
	mxd_ortec974_scaler_overflow_set,
	mxd_ortec974_scaler_read,
	NULL,
	mxd_ortec974_scaler_is_busy,
	mxd_ortec974_scaler_start,
	mxd_ortec974_scaler_stop,
	mxd_ortec974_scaler_get_parameter,
	mxd_ortec974_scaler_set_parameter
};

/* Ortec 974 scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_ortec974_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_ORTEC974_SCALER_STANDARD_FIELDS
};

long mxd_ortec974_scaler_num_record_fields
		= sizeof( mxd_ortec974_scaler_record_field_defaults )
		  / sizeof( mxd_ortec974_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ortec974_scaler_rfield_def_ptr
			= &mxd_ortec974_scaler_record_field_defaults[0];

#define ORTEC974_DEBUG	FALSE

#define CHANNEL_MASK(x)  (1 << ((x)-1))

/* A private function for the use of the driver. */

static mx_status_type
mxd_ortec974_get_pointers( MX_SCALER *scaler,
				MX_ORTEC974_SCALER **ortec974_scaler,
				MX_ORTEC974 **ortec974,
				const char *calling_fname )
{
	const char fname[] = "mxd_ortec974_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( ortec974_scaler == (MX_ORTEC974_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ORTEC974_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( ortec974 == (MX_ORTEC974 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ORTEC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*ortec974_scaler = (MX_ORTEC974_SCALER *)
				(scaler->record->record_type_struct);

	if ( *ortec974_scaler == (MX_ORTEC974_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ORTEC974_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	*ortec974 = (MX_ORTEC974 *)
		((*ortec974_scaler)->ortec974_record->record_type_struct);

	if ( *ortec974 == (MX_ORTEC974 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ORTEC974 pointer for record '%s' is NULL.",
			(*ortec974_scaler)->ortec974_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_ortec974_scaler_initialize_type( long type )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_ortec974_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_ORTEC974_SCALER *ortec974_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	ortec974_scaler = (MX_ORTEC974_SCALER *)
				malloc( sizeof(MX_ORTEC974_SCALER) );

	if ( ortec974_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ORTEC974_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = ortec974_scaler;
	record->class_specific_function_list
			= &mxd_ortec974_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_finish_record_initialization( MX_RECORD *record )
{
	const char fname[]
		= "mxd_ortec974_scaler_finish_record_initialization()";

	MX_ORTEC974_SCALER *ortec974_scaler;
	MX_RECORD *ortec974_record;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the ortec974_record field is actually an Ortec 974
	 * interface record.
	 */

	ortec974_scaler = (MX_ORTEC974_SCALER *)( record->record_type_struct );

	ortec974_record = ortec974_scaler->ortec974_record;

	MX_DEBUG( 2,("%s: ortec974_record = %p, name = '%s'",
		fname, ortec974_record, ortec974_record->name ));

	if ( ortec974_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' is not an interface record.", ortec974_record->name );
	}
	if ( ortec974_record->mx_class != MXI_GENERIC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' is not a generic record.", ortec974_record->name );
	}

	/* Check that the channel number is valid. */

	if ( (ortec974_scaler->channel_number < 2)
	  || (ortec974_scaler->channel_number > 4 ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Ortec 974 channel number %d is out of allowed range 2-4.",
			ortec974_scaler->channel_number );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_delete_record( MX_RECORD *record )
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
mxd_ortec974_scaler_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_clear( MX_SCALER *scaler )
{
	const char fname[] = "mxd_ortec974_scaler_clear()";

	MX_ORTEC974_SCALER *ortec974_scaler;
	MX_ORTEC974 *ortec974;
	char command[40];
	mx_status_type status;

	status = mxd_ortec974_get_pointers( scaler,
			&ortec974_scaler, &ortec974, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	sprintf( command, "CLEAR_COUNTERS %d",
			CHANNEL_MASK(ortec974_scaler->channel_number) );

	status = mxi_ortec974_command( ortec974, command,
					NULL, 0, NULL, 0, ORTEC974_DEBUG );

	scaler->raw_value = 0L;

	return status;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_overflow_set( MX_SCALER *scaler )
{
	const char fname[] = "mxd_ortec974_scaler_overflow_set()";

	scaler->overflow_set = 0;

	return mx_error_quiet( MXE_UNSUPPORTED, fname,
		"This type of scaler cannot check for overflow set." );
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_read( MX_SCALER *scaler )
{
	const char fname[] = "mxd_ortec974_scaler_read()";

	MX_ORTEC974_SCALER *ortec974_scaler;
	MX_ORTEC974 *ortec974;
	char command[40];
	char response[40];
	long data;
	int num_items;
	mx_status_type status;

	scaler->raw_value = 0L;

	status = mxd_ortec974_get_pointers( scaler,
			&ortec974_scaler, &ortec974, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	sprintf( command, "SHOW_COUNTS %d",
			CHANNEL_MASK(ortec974_scaler->channel_number) );

	status = mxi_ortec974_command( ortec974, command,
			response, sizeof(response), NULL, 0, ORTEC974_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%ld", &data );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"No counter value found in Ortec 974 response '%s' for channel %d.",
			response, ortec974_scaler->channel_number );
	}

	scaler->raw_value = data;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_is_busy( MX_SCALER *scaler )
{
	const char fname[] = "mxd_ortec974_scaler_is_busy()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"This function is not yet implemented." );
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_start( MX_SCALER *scaler )
{
	const char fname[] = "mxd_ortec974_scaler_start()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"This function is not yet implemented." );
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_stop( MX_SCALER *scaler )
{
	const char fname[] = "mxd_ortec974_scaler_stop()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"This function is not yet implemented." );
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_get_parameter( MX_SCALER *scaler )
{
	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		scaler->mode = MXCM_COUNTER_MODE;
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		return mx_scaler_default_get_parameter_handler( scaler );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_scaler_set_parameter( MX_SCALER *scaler )
{
	const char fname[] = "mxd_ortec974_scaler_set_parameter()";

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		if ( scaler->mode != MXCM_COUNTER_MODE ) {
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
				"This operation is not yet implemented." );
		}
		break;
	case MXLV_SCL_DARK_CURRENT:
		break;
	default:
		return mx_scaler_default_set_parameter_handler( scaler );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

