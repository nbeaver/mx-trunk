/*
 * Name:    d_handel_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for XIA Handel devices used
 *          in mapping mode.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_mca.h"
#include "mx_mcs.h"
#include "i_handel.h"
#include "d_handel_mcs.h"

/* Initialize the mcs driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_handel_mcs_record_function_list = {
	mxd_handel_mcs_initialize_driver,
	mxd_handel_mcs_create_record_structures,
	mxd_handel_mcs_finish_record_initialization,
	NULL,
	NULL,
	mxd_handel_mcs_open
};

MX_MCS_FUNCTION_LIST mxd_handel_mcs_mcs_function_list = {
	mxd_handel_mcs_arm,
	NULL,
	mxd_handel_mcs_stop,
	mxd_handel_mcs_clear,
	mxd_handel_mcs_busy,
	NULL,
	mxd_handel_mcs_read_all,
	mxd_handel_mcs_read_scaler,
	mxd_handel_mcs_read_measurement,
	NULL,
	NULL,
	mxd_handel_mcs_get_parameter,
	mxd_handel_mcs_set_parameter
};

/* EPICS mcs data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_handel_mcs_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_HANDEL_MCS_STANDARD_FIELDS
};

long mxd_handel_mcs_num_record_fields
		= sizeof( mxd_handel_mcs_record_field_defaults )
			/ sizeof( mxd_handel_mcs_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_handel_mcs_rfield_def_ptr
			= &mxd_handel_mcs_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_handel_mcs_get_pointers( MX_MCS *mcs,
			MX_HANDEL_MCS **handel_mcs,
			const char *calling_fname )
{
	static const char fname[] = "mxd_handel_mcs_get_pointers()";

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( handel_mcs == (MX_HANDEL_MCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MCS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*handel_mcs = (MX_HANDEL_MCS *) mcs->record->record_type_struct;

	if ( *handel_mcs == (MX_HANDEL_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL_MCS pointer for record '%s' is NULL.",
			mcs->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_handel_mcs_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_scalers_varargs_cookie;
	long maximum_num_measurements_varargs_cookie;
	mx_status_type status;

	status = mx_mcs_initialize_driver( driver,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	return status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_mcs_create_record_structures()";

	MX_MCS *mcs;
	MX_HANDEL_MCS *handel_mcs = NULL;

	/* Allocate memory for the necessary structures. */

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS structure." );
	}

	handel_mcs = (MX_HANDEL_MCS *) malloc( sizeof(MX_HANDEL_MCS) );

	if ( handel_mcs == (MX_HANDEL_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_HANDEL_MCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mcs;
	record->record_type_struct = handel_mcs;
	record->class_specific_function_list =
		&mxd_handel_mcs_mcs_function_list;

	mcs->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_handel_mcs_finish_record_initialization()";

	MX_MCS *mcs;
	mx_status_type mx_status;

	mx_status = mx_mcs_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

	mcs = (MX_MCS *) (record->record_class_struct);

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS pointer for MCS record '%s' is NULL.",
			record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_mcs_open()";

	MX_MCS *mcs;
	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mcs = (MX_MCS *) (record->record_class_struct);

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_handel_mcs_arm( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_arm()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_handel_mcs_stop( mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_stop( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_stop()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_clear()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_busy( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_busy()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_read_all( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_read_all()";

	long i;
	mx_status_type mx_status;

	if ( mcs == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCS pointer passed was NULL." );
	}

	for ( i = 0; i < mcs->current_num_scalers; i++ ) {

		mcs->scaler_index = i;

		mx_status = mxd_handel_mcs_read_scaler( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_read_scaler( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_read_scaler()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}
 
MX_EXPORT mx_status_type
mxd_handel_mcs_read_measurement( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_read_measurement()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_get_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_get_parameter()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', parameter type '%s' (%ld)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_MEASUREMENT_NUMBER:
		break;
	case MXLV_MCS_DARK_CURRENT:
		break;
	case MXLV_MCS_TRIGGER_MODE:
		break;
	default:
		return mx_mcs_default_get_parameter_handler( mcs );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mcs_set_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_handel_mcs_set_parameter()";

	MX_HANDEL_MCS *handel_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mcs_get_pointers( mcs, &handel_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', parameter type '%s' (%ld)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_COUNTING_MODE:
		break;

	case MXLV_MCS_MEASUREMENT_TIME:
		break;

	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
		break;

	case MXLV_MCS_DARK_CURRENT:
		break;

	case MXLV_MCS_TRIGGER_MODE:
		break;
	default:
		return mx_mcs_default_set_parameter_handler( mcs );
		break;
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

