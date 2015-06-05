/*
 * Name:    d_nuvant_ezware2_aoutput.c
 *
 * Purpose: MX driver for NuVant EZWare2 analog output channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NUVANT_EZWARE2_AOUTPUT_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "mx_digital_output.h"
#include "i_nuvant_ezware2.h"
#include "d_nuvant_ezware2_aoutput.h"

/* Initialize the aoutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_nuvant_ezware2_aoutput_record_function_list = {
	NULL,
	mxd_nuvant_ezware2_aoutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_nuvant_ezware2_aoutput_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_nuvant_ezware2_aoutput_analog_output_function_list =
{
	NULL,
	mxd_nuvant_ezware2_aoutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_nuvant_ezware2_aoutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_NUVANT_EZWARE2_AOUTPUT_STANDARD_FIELDS
};

long mxd_nuvant_ezware2_aoutput_num_record_fields
		= sizeof( mxd_nuvant_ezware2_aoutput_rf_defaults )
		  / sizeof( mxd_nuvant_ezware2_aoutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezware2_aoutput_rfield_def_ptr
			= &mxd_nuvant_ezware2_aoutput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_nuvant_ezware2_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_NUVANT_EZWARE2_AOUTPUT **ezware2_aoutput,
			MX_NUVANT_EZWARE2 **ezware2,
			const char *calling_fname )
{
	static const char fname[] = "mxd_nuvant_ezware2_aoutput_get_pointers()";

	MX_NUVANT_EZWARE2_AOUTPUT *ezware2_aoutput_ptr;
	MX_RECORD *ezware2_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ezware2_aoutput_ptr = (MX_NUVANT_EZWARE2_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( ezware2_aoutput_ptr == (MX_NUVANT_EZWARE2_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NUVANT_EZWARE2_AOUTPUT pointer for "
		"analog output '%s' passed by '%s' is NULL",
			aoutput->record->name, calling_fname );
	}

	if ( ezware2_aoutput != (MX_NUVANT_EZWARE2_AOUTPUT **) NULL ) {
		*ezware2_aoutput = ezware2_aoutput_ptr;
	}

	if ( ezware2 != (MX_NUVANT_EZWARE2 **) NULL ) {
		ezware2_record = ezware2_aoutput_ptr->nuvant_ezware2_record;

		if ( ezware2_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The nuvant_ezware2_record pointer for "
			"analog output '%s' is NULL.",
				aoutput->record->name );
		}

		*ezware2 = (MX_NUVANT_EZWARE2 *)
				ezware2_record->record_type_struct;

		if ( (*ezware2) == (MX_NUVANT_EZWARE2 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUVANT_EZWARE2 pointer for record '%s' "
			"used by analog output '%s' is NULL.",
				ezware2_record->name,
				aoutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_aoutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_nuvant_ezware2_aoutput_create_record_structures()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_NUVANT_EZWARE2_AOUTPUT *nuvant_ezware2_aoutput;

	/* Allocate memory for the necessary structures. */

	aoutput = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

	if ( aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_OUTPUT structure." );
	}

	nuvant_ezware2_aoutput = (MX_NUVANT_EZWARE2_AOUTPUT *)
				malloc( sizeof(MX_NUVANT_EZWARE2_AOUTPUT) );

	if ( nuvant_ezware2_aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_NUVANT_EZWARE2_AOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = aoutput;
	record->record_type_struct = nuvant_ezware2_aoutput;
	record->class_specific_function_list
		= &mxd_nuvant_ezware2_aoutput_analog_output_function_list;

	aoutput->record = record;

	aoutput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_aoutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_nuvant_ezware2_aoutput_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_NUVANT_EZWARE2_AOUTPUT *ezware2_aoutput = NULL;
	MX_NUVANT_EZWARE2 *ezware2 = NULL;
	char *type_name = NULL;
	size_t len;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_nuvant_ezware2_aoutput_get_pointers( aoutput,
					&ezware2_aoutput, &ezware2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	type_name = ezware2_aoutput->output_type_name;

	len = strlen( type_name );

	if ( mx_strncasecmp( type_name, "potentiostat_voltage", len ) == 0 ) {
		ezware2_aoutput->output_type =
			MXT_NUVANT_EZWARE2_AOUTPUT_POTENTIOSTAT_VOLTAGE;
	} else
	if ( mx_strncasecmp( type_name, "galvanostat_current", len ) == 0 ) {
		ezware2_aoutput->output_type =
			MXT_NUVANT_EZWARE2_AOUTPUT_GALVANOSTAT_CURRENT;
	} else
	if ( mx_strncasecmp( type_name, "potentiostat_current_range", len )
								== 0 )
	{
		ezware2_aoutput->output_type =
			MXT_NUVANT_EZWARE2_AOUTPUT_POTENTIOSTAT_CURRENT_RANGE;
	} else
	if ( mx_strncasecmp( type_name, "galvanostat_current_range", len )
								== 0 )
	{
		ezware2_aoutput->output_type =
			MXT_NUVANT_EZWARE2_AOUTPUT_GALVANOSTAT_CURRENT_RANGE;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Input type '%s' is not supported for "
		"analog output '%s'.", type_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_nuvant_ezware2_aoutput_write()";

	MX_NUVANT_EZWARE2_AOUTPUT *ezware2_aoutput = NULL;
	MX_NUVANT_EZWARE2 *ezware2 = NULL;
	long ez_status;
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezware2_aoutput_get_pointers( aoutput,
					&ezware2_aoutput, &ezware2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezware2_aoutput->output_type ) {
	case MXT_NUVANT_EZWARE2_AOUTPUT_POTENTIOSTAT_VOLTAGE:

		ezware2->potentiostat_current_range =
    mxp_nuvant_ezware2_get_potentiostat_range_from_voltage( aoutput->value );

		ez_status = SetVoltage2( ezware2->potentiostat_current_range,
					aoutput->value,
					ezware2->device_number );
		break;
	case MXT_NUVANT_EZWARE2_AOUTPUT_GALVANOSTAT_CURRENT:

		ezware2->galvanostat_current_range =
    mxp_nuvant_ezware2_get_galvanostat_range_from_current( aoutput->value );

		ez_status = SetCurrent2( ezware2->galvanostat_current_range,
					aoutput->value,
					ezware2->device_number );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for record '%s'.",
			ezware2_aoutput->output_type,
			aoutput->record->name );
		break;
	}

	switch( ez_status ) {
	case 0:
		break;
	case 1:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		    "Communication with the hardware failed for record '%s'",
			aoutput->record->name );
		break;
	case 2:
		return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
		"The value %f written to '%s' was outside the allowed range.",
			aoutput->value, aoutput->record->name );
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The hardware returned unknown error code %ld for record '%s'.",
			ez_status, aoutput->record->name );
		break;
	}

	return mx_status;
}

