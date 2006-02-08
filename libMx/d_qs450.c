/*
 * Name:    d_qs450.c
 *
 * Purpose: MX scaler driver to control DSP QS450 scaler/counters and 
 *          Kinetic Systems KS3610 scaler/counters.  The only important
 *          differences is in the handling of overflows and the fact
 *          that the QS450 has 4 channels and the KS3610 has 6 channels.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_scaler.h"
#include "mx_camac.h"
#include "d_qs450.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_qs450_record_function_list = {
	NULL,
	mxd_qs450_create_record_structures,
	mxd_qs450_finish_record_initialization
};

MX_SCALER_FUNCTION_LIST mxd_qs450_scaler_function_list = {
	mxd_qs450_scaler_clear,
	mxd_qs450_scaler_overflow_set,
	mxd_qs450_scaler_read,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_qs450_scaler_get_parameter,
	mxd_qs450_scaler_set_parameter
};

/* DSP QS450 data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_qs450_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_QS450_STANDARD_FIELDS
};

mx_length_type mxd_qs450_num_record_fields
			= sizeof( mxd_qs450_record_field_defaults )
				/ sizeof( mxd_qs450_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_qs450_record_field_def_ptr
			= &mxd_qs450_record_field_defaults[0];

/* KS 3610 data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_ks3610_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_QS450_STANDARD_FIELDS
};

mx_length_type mxd_ks3610_num_record_fields
			= sizeof( mxd_ks3610_record_field_defaults )
				/ sizeof( mxd_ks3610_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ks3610_record_field_def_ptr
			= &mxd_ks3610_record_field_defaults[0];

MX_EXPORT mx_status_type
mxd_qs450_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_qs450_create_record_structures()";

	MX_SCALER *scaler;
	MX_QS450 *qs450;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	qs450 = (MX_QS450 *) malloc( sizeof(MX_QS450) );

	if ( qs450 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_QS450 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = qs450;
	record->class_specific_function_list = &mxd_qs450_scaler_function_list;

	scaler->record = record;

	MX_DEBUG( 2,
		("%s: qs450->camac_record = %p, &(qs450->camac_record) = %p",
		fname, qs450->camac_record, &(qs450->camac_record)));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_qs450_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_qs450_finish_record_initialization()";

	MX_QS450 *qs450;
	MX_RECORD *camac_record;
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the CAMAC crate record found is indeed a CAMAC crate. */

	qs450 = (MX_QS450 *)( record->record_type_struct );

	camac_record = qs450->camac_record;

	MX_DEBUG( 2,("%s: camac_record = %p, name = '%s'",
		fname, camac_record, camac_record->name ));

	if ( camac_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		    "'%s' is not an interface record.", camac_record->name );
	}
	if ( camac_record->mx_class != MXI_CAMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		    "'%s' is not a CAMAC crate.", camac_record->name );
	}

	/* Check that the slot number is valid. */

	if ( qs450->slot < 1 || qs450->slot > 23 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"CAMAC slot number %d is out of allowed range 1-23.",
			qs450->slot );
	}

	/* The allowed range of subaddresses depends on whether this is a
	 * DSP QS450 or a KS3610.
	 */

	switch( record->mx_type ) {
	case MXT_SCL_QS450:
		if ( qs450->subaddress < 0 || qs450->subaddress > 3 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Subaddress number %d is out of allowed range 0-3.",
				qs450->subaddress );
		}
		break;
	case MXT_SCL_KS3610:
		if ( qs450->subaddress < 0 || qs450->subaddress > 5 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Subaddress number %d is out of allowed range 0-5.",
				qs450->subaddress );
		}
		break;
	default:
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Scaler type %ld is not MXT_SCL_QS450 or MXT_SCL_KS3610.",
			record->mx_type );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_qs450_scaler_clear( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_qs450_scaler_clear()";

	MX_QS450 *qs450;
	int32_t data;
	int camac_Q, camac_X;

	qs450 = (MX_QS450 *) (scaler->record->record_type_struct);

	if ( qs450 == (MX_QS450 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"QS450 pointer is NULL.");
	}

	mx_camac( (qs450->camac_record), (qs450->slot), (qs450->subaddress), 9,
		&data, &camac_Q, &camac_X );

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"CAMAC error: Q = %d, X = %d", camac_Q, camac_X );
	}

	scaler->raw_value = 0L;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_qs450_scaler_overflow_set( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_qs450_scaler_overflow_set()";

	scaler->overflow_set = 0;

	return mx_error_quiet( MXE_UNSUPPORTED, fname,
		"This type of scaler cannot check for overflow set." );
}

MX_EXPORT mx_status_type
mxd_qs450_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_qs450_scaler_read()";

	MX_QS450 *qs450;
	int32_t data;
	int camac_Q, camac_X;

	qs450 = (MX_QS450 *) (scaler->record->record_type_struct);

	if ( qs450 == (MX_QS450 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"QS450 pointer is NULL.");
	}

	mx_camac( (qs450->camac_record), (qs450->slot), (qs450->subaddress), 0,
		&data, &camac_Q, &camac_X );

	if ( camac_Q == 0 || camac_X == 0 ) {
		scaler->raw_value = 0L;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"CAMAC error: Q = %d, X = %d", camac_Q, camac_X );
	}

	scaler->raw_value = (long) data;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_qs450_scaler_get_parameter( MX_SCALER *scaler )
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
mxd_qs450_scaler_set_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_qs450_scaler_set_parameter()";

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		if ( scaler->mode != MXCM_COUNTER_MODE ) {
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
				"This operation is not yet implemented." );
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

