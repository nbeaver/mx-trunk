/*
 * Name:    d_k8055_scaler.c
 *
 * Purpose: MX scaler driver for Velleman K8055 counters used as scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_GATE_CONTROL_PV_ARRAY	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "i_k8055.h"
#include "d_k8055_timer.h"
#include "d_k8055_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_k8055_scaler_record_function_list = {
	NULL,
	mxd_k8055_scaler_create_record_structures,
	mx_scaler_finish_record_initialization
};

MX_SCALER_FUNCTION_LIST mxd_k8055_scaler_scaler_function_list = {
	NULL,
	NULL,
	mxd_k8055_scaler_read,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_scaler_default_get_parameter_handler,
	mx_scaler_default_set_parameter_handler
};

/* K8055 scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_k8055_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_K8055_SCALER_STANDARD_FIELDS
};

long mxd_k8055_scaler_num_record_fields
		= sizeof( mxd_k8055_scaler_record_field_defaults )
		  / sizeof( mxd_k8055_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_k8055_scaler_rfield_def_ptr
			= &mxd_k8055_scaler_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_k8055_scaler_get_pointers( MX_SCALER *scaler,
			MX_K8055_SCALER **k8055_scaler,
			const char *calling_fname )
{
	static const char fname[] = "mxd_k8055_scaler_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( k8055_scaler == (MX_K8055_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_K8055_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( scaler->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_SCALER pointer "
		"passed by '%s' was NULL.", calling_fname );
	}

	*k8055_scaler = (MX_K8055_SCALER *) scaler->record->record_type_struct;

	if ( *k8055_scaler == (MX_K8055_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_K8055_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*======*/

MX_EXPORT mx_status_type
mxd_k8055_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_k8055_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_K8055_SCALER *k8055_scaler = NULL;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	k8055_scaler = (MX_K8055_SCALER *) malloc( sizeof(MX_K8055_SCALER) );

	if ( k8055_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_K8055_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = k8055_scaler;
	record->class_specific_function_list
			= &mxd_k8055_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_k8055_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_k8055_scaler_read()";

	MX_K8055_SCALER *k8055_scaler = NULL;
	mx_status_type mx_status;

	mx_status = mxd_k8055_scaler_get_pointers( scaler,
						&k8055_scaler, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->raw_value = ReadCounter( k8055_scaler->channel );

	return MX_SUCCESSFUL_RESULT;
}

