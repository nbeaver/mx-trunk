/*
 * Name:    d_k8055_ainput.c
 *
 * Purpose: MX driver for Velleman K8055 analog input channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_K8055_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_k8055.h"
#include "d_k8055_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_k8055_ainput_record_function_list = {
	NULL,
	mxd_k8055_ainput_create_record_structures
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_k8055_ainput_analog_input_function_list =
{
	mxd_k8055_ainput_read
};

/* Analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_k8055_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_K8055_AINPUT_STANDARD_FIELDS
};

long mxd_k8055_ainput_num_record_fields
		= sizeof( mxd_k8055_ainput_rf_defaults )
		  / sizeof( mxd_k8055_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_k8055_ainput_rfield_def_ptr
			= &mxd_k8055_ainput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_k8055_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_K8055_AINPUT **k8055_ainput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_k8055_ainput_get_pointers()";

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( k8055_ainput == (MX_K8055_AINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"the MX_K8055_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*k8055_ainput = (MX_K8055_AINPUT *) ainput->record->record_type_struct;

	if ( *k8055_ainput == (MX_K8055_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_K8055_AINPUT pointer for "
		"analog input '%s' passed by '%s' is NULL",
				ainput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_k8055_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_k8055_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_K8055_AINPUT *k8055_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_INPUT structure." );
	}

	k8055_ainput = (MX_K8055_AINPUT *) malloc( sizeof(MX_K8055_AINPUT) );

	if ( k8055_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_K8055_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = k8055_ainput;
	record->class_specific_function_list
			= &mxd_k8055_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_k8055_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_k8055_ainput_read()";

	MX_K8055_AINPUT *k8055_ainput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_k8055_ainput_get_pointers( ainput,
						&k8055_ainput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( k8055_ainput->channel ) {
	case 1:
	case 2:
		ainput->raw_value.long_value =
			ReadAnalogChannel( k8055_ainput->channel );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal channel number %lu requested for record '%s'.  "
		"The allowed channel values are 1 and 2.",
			k8055_ainput->channel, ainput->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

