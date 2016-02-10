/*
 * Name:    d_k8055_doutput.c
 *
 * Purpose: MX driver for NI-DAQmx digital output channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_K8055_DOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_k8055.h"
#include "d_k8055_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_k8055_doutput_record_function_list = {
	NULL,
	mxd_k8055_doutput_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_k8055_doutput_digital_output_function_list = {
	NULL,
	mxd_k8055_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_k8055_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_K8055_DOUTPUT_STANDARD_FIELDS
};

long mxd_k8055_doutput_num_record_fields
	= sizeof( mxd_k8055_doutput_record_field_defaults )
		/ sizeof( mxd_k8055_doutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_k8055_doutput_rfield_def_ptr
			= &mxd_k8055_doutput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_k8055_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_K8055_DOUTPUT **k8055_doutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_k8055_doutput_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( k8055_doutput == (MX_K8055_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_K8055_DOUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	*k8055_doutput = (MX_K8055_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *k8055_doutput == (MX_K8055_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_K8055_DOUTPUT pointer for "
		"digital output '%s' passed by '%s' is NULL",
			doutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_k8055_doutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_k8055_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_K8055_DOUTPUT *k8055_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        k8055_doutput = (MX_K8055_DOUTPUT *) malloc( sizeof(MX_K8055_DOUTPUT) );

        if ( k8055_doutput == (MX_K8055_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_K8055_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = k8055_doutput;
        record->class_specific_function_list
			= &mxd_k8055_doutput_digital_output_function_list;

        digital_output->record = record;
	k8055_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_k8055_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_k8055_doutput_write()";

	MX_K8055_DOUTPUT *k8055_doutput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_k8055_doutput_get_pointers( doutput,
						&k8055_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( k8055_doutput->channel == 0 ) {
		WriteAllDigital( doutput->value );
	} else
	if ( k8055_doutput->channel <= 8 ) {
		if ( doutput->value == 0 ) {
			ClearDigitalChannel( k8055_doutput->channel );
		} else {
			SetDigitalChannel( k8055_doutput->channel );
		}
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal channel number %lu for record '%s'.  "
		"The legal values are from 1 to 8.",
			k8055_doutput->channel,
			doutput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

