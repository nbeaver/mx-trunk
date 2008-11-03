/*
 * Name:    d_ilm_sample_rate.c
 *
 * Purpose: Digital output driver for changing the sample rate for an
 *          Oxford Instruments ILM (Intelligent Level Meter) controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ILM_SAMPLE_RATE_DEBUG	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_digital_output.h"
#include "i_isobus.h"
#include "i_ilm.h"
#include "d_ilm_sample_rate.h"

/* Initialize the ILM sample rate driver function tables. */

MX_RECORD_FUNCTION_LIST mxd_ilm_sample_rate_record_function_list = {
	NULL,
	mxd_ilm_sample_rate_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_ilm_sample_rate_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_ilm_sample_rate_digital_output_function_list =
{
	NULL,
	mxd_ilm_sample_rate_write
};

MX_RECORD_FIELD_DEFAULTS mxd_ilm_sample_rate_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_ILM_SAMPLE_RATE_STANDARD_FIELDS
};

long mxd_ilm_sample_rate_num_record_fields
		= sizeof( mxd_ilm_sample_rate_record_field_defaults )
			/ sizeof( mxd_ilm_sample_rate_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ilm_sample_rate_rfield_def_ptr
		= &mxd_ilm_sample_rate_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_ilm_sample_rate_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_ILM_SAMPLE_RATE **ilm_sample_rate,
				MX_ILM **ilm,
				MX_ISOBUS **isobus,
				const char *calling_fname )
{
	static const char fname[] = "mxd_ilm_sample_rate_get_pointers()";

	MX_ILM_SAMPLE_RATE *ilm_sample_rate_ptr;
	MX_RECORD *ilm_record, *isobus_record;
	MX_ILM *ilm_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	ilm_sample_rate_ptr = (MX_ILM_SAMPLE_RATE *)
				doutput->record->record_type_struct;

	if ( ilm_sample_rate_ptr == (MX_ILM_SAMPLE_RATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ILM_SAMPLE_RATE pointer for ILM sample rate '%s' is NULL",
			doutput->record->name );
	}

	if ( ilm_sample_rate != (MX_ILM_SAMPLE_RATE **) NULL ) {
		*ilm_sample_rate = ilm_sample_rate_ptr;
	}

	ilm_record = ilm_sample_rate_ptr->ilm_record;

	if ( ilm_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The ilm_record pointer for ILM sample rate '%s' is NULL.",
			doutput->record->name );
	}

	if ( ilm_record->mx_type != MXI_GEN_ILM ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"ilm_record '%s' for ILM sample rate '%s' "
		"is not an 'ilm' record.  Instead, it is of type '%s'.",
			ilm_record->name, doutput->record->name,
			mx_get_driver_name( ilm_record ) );
	}

	ilm_ptr = (MX_ILM *) ilm_record->record_type_struct;

	if ( ilm_ptr == (MX_ILM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ILM pointer for ILM record '%s' "
		"used by ILM sample rate '%s' is NULL.",
			ilm_record->name,
			doutput->record->name );
	}

	if ( ilm != (MX_ILM **) NULL ) {
		*ilm = ilm_ptr;
	}

	if ( isobus != (MX_ISOBUS **) NULL ) {
		isobus_record = ilm_ptr->isobus_record;

		if ( isobus_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The isobus_record pointer for ILM "
			"record '%s' is NULL.", ilm_record->name );
		}

		*isobus = isobus_record->record_type_struct;

		if ( (*isobus) == (MX_ISOBUS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_ISOBUS pointer for ISOBUS record '%s' "
			"is NULL.", isobus_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_ilm_sample_rate_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_ilm_sample_rate_create_record_structures()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_ILM_SAMPLE_RATE *ilm_sample_rate;

	/* Allocate memory for the necessary structures. */

	doutput = (MX_DIGITAL_OUTPUT *) malloc( sizeof(MX_DIGITAL_OUTPUT) );

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_DIGITAL_OUTPUT structure." );
	}

	ilm_sample_rate = (MX_ILM_SAMPLE_RATE *)
				malloc( sizeof(MX_ILM_SAMPLE_RATE) );

	if ( ilm_sample_rate == (MX_ILM_SAMPLE_RATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_ILM_SAMPLE_RATE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = doutput;
	record->record_type_struct = ilm_sample_rate;
	record->class_specific_function_list =
		&mxd_ilm_sample_rate_digital_output_function_list;

	doutput->record = record;
	ilm_sample_rate->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ilm_sample_rate_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ilm_sample_rate_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_ILM_SAMPLE_RATE *ilm_sample_rate = NULL;
	MX_ILM *ilm = NULL;
	MX_ISOBUS *isobus = NULL;
	char response[80];
	int channel_usage;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXD_ILM_SAMPLE_RATE_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	doutput = record->record_class_struct;

	mx_status = mxd_ilm_sample_rate_get_pointers( doutput,
				&ilm_sample_rate, &ilm, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (ilm_sample_rate->channel < 1)
	  || (ilm_sample_rate->channel > 3) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal variable number %ld specified for ILM sample rate "
		"record '%s'.  The allowed values are from 1 to 3.",
			ilm_sample_rate->channel,
			ilm_sample_rate->record->name );
	}

	/* Get the ILM status. */

	mx_status = mxi_isobus_command( isobus, ilm->isobus_address,
					"X", response, sizeof(response),
					ilm->maximum_retries,
					MXD_ILM_SAMPLE_RATE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel_usage = mx_hex_char_to_unsigned_long(
				response[ilm_sample_rate->channel] );
	switch( channel_usage ) {
	case 1:
	case 2:
	case 3:
		break;
	case 0:
		mx_warning( "Channel %ld for ILM doutput record '%s' "
		"is marked as 'not in use'.",
			ilm_sample_rate->channel,
			ilm_sample_rate->record->name );
		break;
	case 9:
		mx_warning( "Channel %ld for ILM doutput record '%s' "
		"has an error (usually means probe unplugged).",
			ilm_sample_rate->channel,
			ilm_sample_rate->record->name );
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognized channel usage value %x reported "
		"for channel %ld of ILM doutput record '%s'.",
			channel_usage,
			ilm_sample_rate->channel,
			ilm_sample_rate->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ilm_sample_rate_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_ilm_sample_rate_write()";

	MX_ILM_SAMPLE_RATE *ilm_sample_rate = NULL;
	MX_ILM *ilm = NULL;
	MX_ISOBUS *isobus = NULL;
	char command[20];
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_ilm_sample_rate_get_pointers( doutput,
				&ilm_sample_rate, &ilm, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( doutput->value != 0 ) {
		doutput->value = 1;

		snprintf( command, sizeof(command),
			"T%ld", ilm_sample_rate->channel );
	} else {
		snprintf( command, sizeof(command),
			"S%ld", ilm_sample_rate->channel );
	}

	mx_status = mxi_isobus_command( isobus, ilm->isobus_address,
					command, response, sizeof(response),
					ilm->maximum_retries,
					MXD_ILM_SAMPLE_RATE_DEBUG );

	return mx_status;
}

