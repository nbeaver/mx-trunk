/*
 * Name:    d_mca_channel.c
 *
 * Purpose: MX scaler driver to readout individual MCA channels as if they
 *          were scalers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "mx_mca.h"
#include "d_mca_channel.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mca_channel_record_function_list = {
	mxd_mca_channel_initialize_type,
	mxd_mca_channel_create_record_structures,
	mxd_mca_channel_finish_record_initialization,
	mxd_mca_channel_delete_record,
	NULL,
	mxd_mca_channel_read_parms_from_hardware,
	mxd_mca_channel_write_parms_to_hardware,
	mxd_mca_channel_open,
	mxd_mca_channel_close
};

MX_SCALER_FUNCTION_LIST mxd_mca_channel_scaler_function_list = {
	mxd_mca_channel_clear,
	mxd_mca_channel_overflow_set,
	mxd_mca_channel_read,
	NULL,
	mxd_mca_channel_is_busy,
	mxd_mca_channel_start,
	mxd_mca_channel_stop,
	mxd_mca_channel_get_parameter,
	mxd_mca_channel_set_parameter
};

/* MX MCA channel scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mca_channel_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_MCA_CHANNEL_STANDARD_FIELDS
};

mx_length_type mxd_mca_channel_num_record_fields
		= sizeof( mxd_mca_channel_record_field_defaults )
		  / sizeof( mxd_mca_channel_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mca_channel_rfield_def_ptr
			= &mxd_mca_channel_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_mca_channel_get_pointers( MX_SCALER *scaler,
			MX_MCA_CHANNEL **mca_channel,
			const char *calling_fname )
{
	const char fname[] = "mxd_mca_channel_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The scaler pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( scaler->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for scaler pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( scaler->record->mx_type != MXT_SCL_MCA_CHANNEL ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The scaler '%s' passed by '%s' is not an MCA channel scaler.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			scaler->record->name, calling_fname,
			scaler->record->mx_superclass,
			scaler->record->mx_class,
			scaler->record->mx_type );
	}

	if ( mca_channel != (MX_MCA_CHANNEL **) NULL ) {

		*mca_channel = (MX_MCA_CHANNEL *)
				(scaler->record->record_type_struct);

		if ( *mca_channel == (MX_MCA_CHANNEL *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_MCA_CHANNEL pointer for scaler record '%s' passed by '%s' is NULL",
				scaler->record->name, calling_fname );
		}
	}

	if ( (*mca_channel)->mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX MCA record pointer for MCA channel '%s' is NULL.",
			scaler->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_mca_channel_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_channel_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_mca_channel_create_record_structures()";

	MX_SCALER *scaler;
	MX_MCA_CHANNEL *mca_channel;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	mca_channel = (MX_MCA_CHANNEL *) malloc( sizeof(MX_MCA_CHANNEL) );

	if ( mca_channel == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA_CHANNEL structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = mca_channel;
	record->class_specific_function_list
			= &mxd_mca_channel_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_channel_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_mca_channel_finish_record_initialization()";

	MX_MCA_CHANNEL *mca_channel;
	MX_RECORD *mca_record;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca_channel = (MX_MCA_CHANNEL *) record->record_type_struct;

	if ( mca_channel == (MX_MCA_CHANNEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MCA_CHANNEL pointer for MCA channel scaler '%s' is NULL.",
			record->name );
	}

	mca_record = mca_channel->mca_record;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"mca_record pointer for MCA channel scaler '%s' is NULL.",
			record->name );
	}

	if ( mca_record->mx_class != MXC_MULTICHANNEL_ANALYZER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The device '%s' named by the 'mca_record' field is not "
		"a multichannel analyzer.", mca_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_channel_delete_record( MX_RECORD *record )
{
	MX_MCA_CHANNEL *mca_channel;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mca_channel = (MX_MCA_CHANNEL *) record->record_type_struct;

	if ( mca_channel != NULL ) {
		free( mca_channel );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_channel_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_channel_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_channel_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_channel_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_channel_clear( MX_SCALER *scaler )
{
	const char fname[] = "mxd_mca_channel_clear()";

	MX_MCA_CHANNEL *mca_channel;
	mx_status_type status;

	status = mxd_mca_channel_get_pointers( scaler, &mca_channel, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_mca_clear( mca_channel->mca_record );

	scaler->raw_value = 0L;

	return status;
}

MX_EXPORT mx_status_type
mxd_mca_channel_overflow_set( MX_SCALER *scaler )
{
	const char fname[] = "mxd_mca_channel_overflow_set()";

	MX_MCA_CHANNEL *mca_channel;
	mx_status_type status;

	status = mxd_mca_channel_get_pointers( scaler, &mca_channel, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* WML: I haven't decided how to do this yet. */

	scaler->overflow_set = FALSE;

	return status;
}

MX_EXPORT mx_status_type
mxd_mca_channel_read( MX_SCALER *scaler )
{
	const char fname[] = "mxd_mca_channel_read()";

	MX_MCA_CHANNEL *mca_channel;
	unsigned long value;
	mx_status_type status;

	status = mxd_mca_channel_get_pointers( scaler, &mca_channel, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_mca_get_channel( mca_channel->mca_record,
					mca_channel->channel_number, &value );

	scaler->raw_value = (long) value;

	return status;
}

MX_EXPORT mx_status_type
mxd_mca_channel_is_busy( MX_SCALER *scaler )
{
	const char fname[] = "mxd_mca_channel_is_busy()";

	MX_MCA_CHANNEL *mca_channel;
	int busy;
	mx_status_type status;

	status = mxd_mca_channel_get_pointers( scaler, &mca_channel, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_mca_is_busy( mca_channel->mca_record, &busy );

	scaler->busy = busy;

	return status;
}

MX_EXPORT mx_status_type
mxd_mca_channel_start( MX_SCALER *scaler )
{
	const char fname[] = "mxd_mca_channel_start()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"This function requires support by the MCA record class that "
	"has not been implemented yet." );
}

MX_EXPORT mx_status_type
mxd_mca_channel_stop( MX_SCALER *scaler )
{
	const char fname[] = "mxd_mca_channel_stop()";

	MX_MCA_CHANNEL *mca_channel;
	mx_status_type status;

	status = mxd_mca_channel_get_pointers( scaler, &mca_channel, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_mca_stop( mca_channel->mca_record );

	scaler->raw_value = 0;

	return status;
}

MX_EXPORT mx_status_type
mxd_mca_channel_get_parameter( MX_SCALER *scaler )
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
mxd_mca_channel_set_parameter( MX_SCALER *scaler )
{
	const char fname[] = "mxd_mca_channel_set_parameter()";

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		if ( scaler->mode != MXCM_COUNTER_MODE ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Cannot set MCA channel scaler '%s' to counter mode %d.  "
		"Only preset time mode is supported for now.",
				scaler->record->name,
				scaler->mode );
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

