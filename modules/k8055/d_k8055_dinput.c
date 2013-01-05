/*
 * Name:    d_k8055_dinput.c
 *
 * Purpose: MX driver for Velleman K8055 digital input channels.
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

#define MXD_K8055_DINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "i_k8055.h"
#include "d_k8055_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_k8055_dinput_record_function_list = {
	NULL,
	mxd_k8055_dinput_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_k8055_dinput_digital_input_function_list = {
	mxd_k8055_dinput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_k8055_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_K8055_DINPUT_STANDARD_FIELDS
};

long mxd_k8055_dinput_num_record_fields
	= sizeof( mxd_k8055_dinput_record_field_defaults )
		/ sizeof( mxd_k8055_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_k8055_dinput_rfield_def_ptr
			= &mxd_k8055_dinput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_k8055_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_K8055_DINPUT **k8055_dinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_k8055_dinput_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( k8055_dinput == (MX_K8055_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_K8055_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*k8055_dinput = (MX_K8055_DINPUT *) dinput->record->record_type_struct;

	if ( *k8055_dinput == (MX_K8055_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_K8055_DINPUT pointer for "
		"digital input '%s' passed by '%s' is NULL",
			dinput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_k8055_dinput_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_k8055_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_K8055_DINPUT *k8055_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_INPUT structure." );
        }

        k8055_dinput = (MX_K8055_DINPUT *) malloc( sizeof(MX_K8055_DINPUT) );

        if ( k8055_dinput == (MX_K8055_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_K8055_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = k8055_dinput;
        record->class_specific_function_list
			= &mxd_k8055_dinput_digital_input_function_list;

        digital_input->record = record;
	k8055_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_k8055_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_k8055_dinput_read()";

	MX_K8055_DINPUT *k8055_dinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_k8055_dinput_get_pointers( dinput,
						&k8055_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( k8055_dinput->channel == 0 ) {
		dinput->value = ReadAllDigital();
	} else
	if ( k8055_dinput->channel <= 5 ) {
		dinput->value = ReadDigitalChannel( k8055_dinput->channel );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal channel number %lu for record '%s'.  "
		"The legal values are from 1 to 5.",
			k8055_dinput->channel,
			dinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

