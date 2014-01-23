/*
 * Name:    d_nuvant_ezstat_aoutput.c
 *
 * Purpose: MX driver for NuVant EZstat analog output channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NUVANT_EZSTAT_AOUTPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "mx_digital_output.h"
#include "i_nuvant_ezstat.h"
#include "d_nuvant_ezstat_aoutput.h"

/* Initialize the aoutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_nuvant_ezstat_aoutput_record_function_list = {
	NULL,
	mxd_nuvant_ezstat_aoutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_nuvant_ezstat_aoutput_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_nuvant_ezstat_aoutput_analog_output_function_list =
{
	mxd_nuvant_ezstat_aoutput_read,
	mxd_nuvant_ezstat_aoutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_nuvant_ezstat_aoutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_NUVANT_EZSTAT_AOUTPUT_STANDARD_FIELDS
};

long mxd_nuvant_ezstat_aoutput_num_record_fields
		= sizeof( mxd_nuvant_ezstat_aoutput_rf_defaults )
		  / sizeof( mxd_nuvant_ezstat_aoutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezstat_aoutput_rfield_def_ptr
			= &mxd_nuvant_ezstat_aoutput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_nuvant_ezstat_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_NUVANT_EZSTAT_AOUTPUT **ezstat_aoutput,
			MX_NUVANT_EZSTAT **ezstat,
			const char *calling_fname )
{
	static const char fname[] = "mxd_nuvant_ezstat_aoutput_get_pointers()";

	MX_NUVANT_EZSTAT_AOUTPUT *ezstat_aoutput_ptr;
	MX_RECORD *ezstat_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ezstat_aoutput_ptr = (MX_NUVANT_EZSTAT_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( ezstat_aoutput_ptr == (MX_NUVANT_EZSTAT_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NUVANT_EZSTAT_AOUTPUT pointer for "
		"analog output '%s' passed by '%s' is NULL",
			aoutput->record->name, calling_fname );
	}

	if ( ezstat_aoutput != (MX_NUVANT_EZSTAT_AOUTPUT **) NULL ) {
		*ezstat_aoutput = ezstat_aoutput_ptr;
	}

	if ( ezstat != (MX_NUVANT_EZSTAT **) NULL ) {
		ezstat_record = ezstat_aoutput_ptr->nuvant_ezstat_record;

		if ( ezstat_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The nuvant_ezstat_record pointer for "
			"analog output '%s' is NULL.",
				aoutput->record->name );
		}

		*ezstat = (MX_NUVANT_EZSTAT *)ezstat_record->record_type_struct;

		if ( (*ezstat) == (MX_NUVANT_EZSTAT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUVANT_EZSTAT pointer for record '%s' "
			"used by analog output '%s' is NULL.",
				ezstat_record->name,
				aoutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_aoutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_nuvant_ezstat_aoutput_create_record_structures()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_NUVANT_EZSTAT_AOUTPUT *nuvant_ezstat_aoutput;

	/* Allocate memory for the necessary structures. */

	aoutput = (MX_ANALOG_OUTPUT *) malloc( sizeof(MX_ANALOG_OUTPUT) );

	if ( aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_OUTPUT structure." );
	}

	nuvant_ezstat_aoutput = (MX_NUVANT_EZSTAT_AOUTPUT *)
				malloc( sizeof(MX_NUVANT_EZSTAT_AOUTPUT) );

	if ( nuvant_ezstat_aoutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_NUVANT_EZSTAT_AOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = aoutput;
	record->record_type_struct = nuvant_ezstat_aoutput;
	record->class_specific_function_list
		= &mxd_nuvant_ezstat_aoutput_analog_output_function_list;

	aoutput->record = record;

	aoutput->subclass = MXT_AOU_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_aoutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_nuvant_ezstat_aoutput_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_NUVANT_EZSTAT_AOUTPUT *ezstat_aoutput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	char *type_name = NULL;
	size_t len;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_nuvant_ezstat_aoutput_get_pointers( aoutput,
					&ezstat_aoutput, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	type_name = ezstat_aoutput->output_type_name;

	len = strlen( type_name );

	if ( mx_strncasecmp( type_name, "potentiostat_voltage", len ) == 0 ) {
		ezstat_aoutput->output_type =
			MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_VOLTAGE;
	} else
	if ( mx_strncasecmp( type_name, "galvanostat_current", len ) == 0 ) {
		ezstat_aoutput->output_type =
			MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT;
	} else
	if ( mx_strncasecmp( type_name, "potentiostat_current_range", len )
								== 0 )
	{
		ezstat_aoutput->output_type =
			MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_CURRENT_RANGE;
	} else
	if ( mx_strncasecmp( type_name, "galvanostat_current_range", len )
								== 0 )
	{
		ezstat_aoutput->output_type =
			MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT_RANGE;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Input type '%s' is not supported for "
		"analog output '%s'.", type_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_aoutput_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_nuvant_ezstat_aoutput_read()";

	MX_NUVANT_EZSTAT_AOUTPUT *ezstat_aoutput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	unsigned long p11_value, p12_value, p13_value, p14_value;
	double ao0_value;
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezstat_aoutput_get_pointers( aoutput,
					&ezstat_aoutput, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezstat_aoutput->output_type ) {
	case MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_VOLTAGE:
		mx_status = mx_analog_output_read( ezstat->ao0_record,
							&ao0_value );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		aoutput->raw_value.double_value = ao0_value;
		break;
	case MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT:
		mx_status = mx_analog_output_read( ezstat->ao0_record,
							&ao0_value );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		aoutput->raw_value.double_value =
		  mx_divide_safely( ao0_value, ezstat->galvanostat_resistance );

		break;
	case MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_CURRENT_RANGE:

		mx_status = mxi_nuvant_ezstat_get_potentiostat_current_range(
				ezstat, &(aoutput->raw_value.double_value) );
		break;
	case MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT_RANGE:

		mx_status = mxi_nuvant_ezstat_get_galvanostat_current_range(
				ezstat, &(aoutput->raw_value.double_value) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for record '%s'.",
			ezstat_aoutput->output_type,
			aoutput->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_nuvant_ezstat_aoutput_write()";

	MX_NUVANT_EZSTAT_AOUTPUT *ezstat_aoutput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	unsigned long p11_value, p12_value, p13_value, p14_value;
	double ao0_value, raw_value;
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezstat_aoutput_get_pointers( aoutput,
					&ezstat_aoutput, &ezstat, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezstat_aoutput->output_type ) {
	case MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_VOLTAGE:
		mx_status = mx_digital_output_write( ezstat->p10_record, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_analog_output_write( ezstat->ao0_record,
					aoutput->raw_value.double_value );
		break;
	case MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT:
		mx_status = mx_digital_output_write( ezstat->p10_record, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ao0_value = aoutput->raw_value.double_value
				* ezstat->galvanostat_resistance;
			
		mx_status = mx_analog_output_write( ezstat->ao0_record,
								ao0_value );
		break;
	case MXT_NUVANT_EZSTAT_AOUTPUT_POTENTIOSTAT_CURRENT_RANGE:

		mx_status = mxi_nuvant_ezstat_set_potentiostat_current_range(
				ezstat, aoutput->raw_value.double_value );
		break;
	case MXT_NUVANT_EZSTAT_AOUTPUT_GALVANOSTAT_CURRENT_RANGE:

		mx_status = mxi_nuvant_ezstat_set_galvanostat_current_range(
				ezstat, aoutput->raw_value.double_value );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output type %lu requested for record '%s'.",
			ezstat_aoutput->output_type,
			aoutput->record->name );
		break;
	}

	return mx_status;
}

