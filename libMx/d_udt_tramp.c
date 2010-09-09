/*
 * Name:    d_udt_tramp.c
 *
 * Purpose: MX driver for the UDT Instruments TRAMP current amplifier.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2008, 2010 Illinois Institute of Technology
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
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "mx_amplifier.h"
#include "d_udt_tramp.h"

/* Initialize the amplifier driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_udt_tramp_record_function_list = {
	NULL,
	mxd_udt_tramp_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_udt_tramp_open,
	NULL,
	NULL,
	mxd_udt_tramp_open
};

MX_AMPLIFIER_FUNCTION_LIST mxd_udt_tramp_amplifier_function_list = {
	mxd_udt_tramp_get_gain,
	mxd_udt_tramp_set_gain
};

/* UDT_TRAMP amplifier data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_udt_tramp_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_UDT_TRAMP_STANDARD_FIELDS
};

long mxd_udt_tramp_num_record_fields
		= sizeof( mxd_udt_tramp_record_field_defaults )
		  / sizeof( mxd_udt_tramp_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_udt_tramp_rfield_def_ptr
			= &mxd_udt_tramp_record_field_defaults[0];

/* Private functions for the use of the driver. */

static mx_status_type
mxd_udt_tramp_get_pointers( MX_AMPLIFIER *amplifier,
				MX_UDT_TRAMP **udt_tramp,
				const char *calling_fname )
{
	static const char fname[] = "mxd_udt_tramp_get_pointers()";

	MX_RECORD *record;

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( udt_tramp == (MX_UDT_TRAMP **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_UDT_TRAMP pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = amplifier->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_AMPLIFIER structure "
		"passed by '%s' is NULL.", calling_fname );
	}

	*udt_tramp = (MX_UDT_TRAMP *) record->record_type_struct;

	if ( *udt_tramp == (MX_UDT_TRAMP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_UDT_TRAMP pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_udt_tramp_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_udt_tramp_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_UDT_TRAMP *udt_tramp = NULL;

	/* Allocate memory for the necessary structures. */

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPLIFIER structure." );
	}

	udt_tramp = (MX_UDT_TRAMP *) malloc( sizeof(MX_UDT_TRAMP) );

	if ( udt_tramp == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_UDT_TRAMP structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = amplifier;
	record->record_type_struct = udt_tramp;
	record->class_specific_function_list
			= &mxd_udt_tramp_amplifier_function_list;

	amplifier->record = record;

	/* The gain range for an UDT_TRAMP is from 1e3 to 1e12. */

	amplifier->gain_range[0] = 1.0e3;
	amplifier->gain_range[1] = 1.0e10;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_udt_tramp_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_udt_tramp_open()";

	MX_AMPLIFIER *amplifier;
	MX_UDT_TRAMP *udt_tramp = NULL;
	mx_status_type mx_status;

	amplifier = (MX_AMPLIFIER *) (record->record_class_struct);

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_udt_tramp_get_pointers( amplifier, &udt_tramp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_udt_tramp_set_gain( amplifier );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_udt_tramp_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_udt_tramp_get_gain()";

	MX_UDT_TRAMP *udt_tramp = NULL;
	unsigned long doutput_value;
	mx_status_type mx_status;

	mx_status = mxd_udt_tramp_get_pointers( amplifier, &udt_tramp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the digital output register. */

	mx_status = mx_digital_output_read( udt_tramp->doutput_record,
						&doutput_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the gain using the 3 lowest order bits of the value read. */

	doutput_value &= 0x7;

	amplifier->gain = pow( 10.0, 3.0 + (double) doutput_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_udt_tramp_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_udt_tramp_set_gain()";

	MX_UDT_TRAMP *udt_tramp = NULL;
	unsigned long power, doutput_value;
	mx_status_type mx_status;

	mx_status = mxd_udt_tramp_get_pointers( amplifier, &udt_tramp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Round the gain to the nearest power of 10. */

	power = mx_round( log10( amplifier->gain ) );

	if ( ( power < 3 ) || ( power > 10 ) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested gain %g for amplifier '%s' is outside "
		"the allowed range of 1.0e3 to 1.0e10.",
			amplifier->gain, amplifier->record->name );
	}

	amplifier->gain = pow( 10.0, (double) power );

	doutput_value = power - 3L;

	mx_status = mx_digital_output_write( udt_tramp->doutput_record,
							doutput_value );

	return mx_status;
}

