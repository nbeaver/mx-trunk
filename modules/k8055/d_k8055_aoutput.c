/*
 * Name:    d_k8055_aoutput.c
 *
 * Purpose: MX driver for Velleman K8055 analog output channels.
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

#define MXD_K8055_AOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "i_k8055.h"
#include "d_k8055_aoutput.h"

/* Initialize the aoutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_k8055_aoutput_record_function_list = {
	NULL,
	mxd_k8055_aoutput_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_k8055_aoutput_analog_output_function_list =
{
	NULL,
	mxd_k8055_aoutput_write
};

/* Analog output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_k8055_aoutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_K8055_AOUTPUT_STANDARD_FIELDS
};

long mxd_k8055_aoutput_num_record_fields
		= sizeof( mxd_k8055_aoutput_rf_defaults )
		  / sizeof( mxd_k8055_aoutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_k8055_aoutput_rfield_def_ptr
			= &mxd_k8055_aoutput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_k8055_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_K8055_AOUTPUT **k8055_aoutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_k8055_aoutput_get_pointers()";

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( k8055_aoutput == (MX_K8055_AOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_K8055_AOUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*k8055_aoutput = (MX_K8055_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *k8055_aoutput == (MX_K8055_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_K8055_AOUTPUT pointer for "
		"analog output '%s' passed by '%s' is NULL",
			aoutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_k8055_aoutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_k8055_aoutput_create_record_structures()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_K8055_AOUTPUT *k8055_aoutput;

	/* Allocate memory for the necessary structures. */

	aoutput = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

	if ( aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_OUTPUT structure." );
	}

	k8055_aoutput = (MX_K8055_AOUTPUT *)
				malloc( sizeof(MX_K8055_AOUTPUT) );

	if ( k8055_aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_K8055_AOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = aoutput;
	record->record_type_struct = k8055_aoutput;
	record->class_specific_function_list
			= &mxd_k8055_aoutput_analog_output_function_list;

	aoutput->record = record;

	aoutput->subclass = MXT_AOU_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_k8055_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_k8055_aoutput_write()";

	MX_K8055_AOUTPUT *k8055_aoutput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_k8055_aoutput_get_pointers( aoutput,
						&k8055_aoutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( aoutput->raw_value.long_value < 0 ) {
		aoutput->raw_value.long_value = 0;
	} else
	if ( aoutput->raw_value.long_value > 255 ) {
		aoutput->raw_value.long_value = 255;
	}

	OutputAnalogChannel( k8055_aoutput->channel,
				aoutput->raw_value.long_value );

	return mx_status;
}

