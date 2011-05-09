/*
 * Name:    d_i404_amp.c
 *
 * Purpose: Driver for the Pyramid Technical Consultants I404
 *          digital electrometer.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_I404_AMP_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_amplifier.h"
#include "i_i404.h"
#include "d_i404_amp.h"

MX_RECORD_FUNCTION_LIST mxd_i404_amp_record_function_list = {
	NULL,
	mxd_i404_amp_create_record_structures
};

MX_AMPLIFIER_FUNCTION_LIST mxd_i404_amp_amplifier_function_list = {
	mxd_i404_amp_get_gain,
	mxd_i404_amp_set_gain
};

MX_RECORD_FIELD_DEFAULTS mxd_i404_amp_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_I404_AMP_STANDARD_FIELDS
};

long mxd_i404_amp_num_record_fields
		= sizeof( mxd_i404_amp_record_field_defaults )
		    / sizeof( mxd_i404_amp_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_i404_amp_rfield_def_ptr
			= &mxd_i404_amp_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_i404_amp_get_pointers( MX_AMPLIFIER *amplifier,
			MX_I404_AMP **i404_amp,
			MX_I404 **i404,
			const char *calling_fname )
{
	static const char fname[] = "mxd_i404_amp_get_pointers()";

	MX_RECORD *i404_record;

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( i404_amp == (MX_I404_AMP **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_I404_AMP pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( i404 == (MX_I404 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_I404 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*i404_amp = (MX_I404_AMP *) amplifier->record->record_type_struct;

	if ( *i404_amp == (MX_I404_AMP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_I404_AMP pointer for record '%s' is NULL.",
			amplifier->record->name );
	}

	i404_record = (*i404_amp)->i404_record;

	if ( i404_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The i404_record pointer for '%s' is NULL.",
			amplifier->record->name );
	}

	*i404 = (MX_I404 *) i404_record->record_type_struct;

	if ( *i404 == (MX_I404 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_I404 pointer for record '%s' is NULL.",
			i404_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_i404_amp_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_i404_amp_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_I404_AMP *i404_amp;

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AMPLIFIER structure.");
	}

	i404_amp = (MX_I404_AMP *)
				malloc( sizeof(MX_I404_AMP) );

	if ( i404_amp == (MX_I404_AMP *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_I404_AMP structure." );
	}

	record->record_class_struct = amplifier;
	record->record_type_struct = i404_amp;

	record->class_specific_function_list
			= &mxd_i404_amp_amplifier_function_list;

	amplifier->record = record;
	i404_amp->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_i404_amp_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_i404_amp_get_gain()";

	MX_I404_AMP *i404_amp;
	MX_I404 *i404;
	char response[100];
	double gain_value;
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_i404_amp_get_pointers( amplifier,
						&i404_amp, &i404, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_i404_command( i404, "CONF:RANGE?",
				response, sizeof(response),
				MXD_I404_AMP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg", &gain_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see a number in the response '%s' to the "
		"command 'CONF:RANGE?' for '%s'.",
			response, amplifier->record->name );
	}

	amplifier->gain = gain_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_i404_amp_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_i404_amp_set_gain()";

	MX_I404_AMP *i404_amp;
	MX_I404 *i404;
	char command[100];
	mx_status_type mx_status;

	mx_status = mxd_i404_amp_get_pointers( amplifier,
						&i404_amp, &i404, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "CONF:RANGE %g", amplifier->gain );

	mx_status = mxi_i404_command( i404, command,
				NULL, 0, MXD_I404_AMP_DEBUG );

	return mx_status;
}

