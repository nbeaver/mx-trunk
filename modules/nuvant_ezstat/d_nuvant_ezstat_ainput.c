/*
 * Name:    d_nuvant_ezstat_ainput.c
 *
 * Purpose: MX driver for NuVant EZstat analog input channels.
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

#define MXD_NUVANT_EZSTAT_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_nuvant_ezstat.h"
#include "d_nuvant_ezstat_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_nuvant_ezstat_ainput_record_function_list = {
	NULL,
	mxd_nuvant_ezstat_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_nuvant_ezstat_ainput_open
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_nuvant_ezstat_ainput_analog_input_function_list =
{
	mxd_nuvant_ezstat_ainput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_nuvant_ezstat_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_NUVANT_EZSTAT_AINPUT_STANDARD_FIELDS
};

long mxd_nuvant_ezstat_ainput_num_record_fields
		= sizeof( mxd_nuvant_ezstat_ainput_rf_defaults )
		  / sizeof( mxd_nuvant_ezstat_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezstat_ainput_rfield_def_ptr
			= &mxd_nuvant_ezstat_ainput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_nuvant_ezstat_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_NUVANT_EZSTAT_AINPUT **ezstat_ainput,
				MX_NUVANT_EZSTAT **ezstat,
				const char *calling_fname )
{
	static const char fname[] = "mxd_nuvant_ezstat_ainput_get_pointers()";

	MX_NUVANT_EZSTAT_AINPUT *ezstat_ainput_ptr;
	MX_RECORD *ezstat_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ezstat_ainput_ptr = (MX_NUVANT_EZSTAT_AINPUT *)
				ainput->record->record_type_struct;

	if ( ezstat_ainput_ptr == (MX_NUVANT_EZSTAT_AINPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUVANT_EZSTAT_AINPUT pointer for "
			"analog input '%s' passed by '%s' is NULL",
				ainput->record->name, calling_fname );
	}

	if ( ezstat_ainput != (MX_NUVANT_EZSTAT_AINPUT **) NULL ) {
		*ezstat_ainput = ezstat_ainput_ptr;
	}

	if ( ezstat != (MX_NUVANT_EZSTAT **) NULL ) {
		ezstat_record = ezstat_ainput_ptr->nuvant_ezstat_record;

		if ( ezstat_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The nuvant_ezstat_record pointer for "
			"analog input '%s' is NULL.",
				ainput->record->name );
		}

		*ezstat = (MX_NUVANT_EZSTAT *)ezstat_record->record_type_struct;

		if ( (*ezstat) == (MX_NUVANT_EZSTAT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUVANT_EZSTAT pointer for record '%s' "
			"used by analog input '%s' is NULL.",
				ezstat_record->name,
				ainput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_nuvant_ezstat_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_NUVANT_EZSTAT_AINPUT *nuvant_ezstat_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_INPUT structure." );
	}

	nuvant_ezstat_ainput = (MX_NUVANT_EZSTAT_AINPUT *)
				malloc( sizeof(MX_NUVANT_EZSTAT_AINPUT) );

	if ( nuvant_ezstat_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_NUVANT_EZSTAT_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = nuvant_ezstat_ainput;
	record->class_specific_function_list
			= &mxd_nuvant_ezstat_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_nuvant_ezstat_ainput_open()";

	MX_ANALOG_INPUT *ainput = NULL;
	MX_NUVANT_EZSTAT_AINPUT *ezstat_ainput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	char *type_name = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_nuvant_ezstat_ainput_get_pointers( ainput,
					&ezstat_ainput, &ezstat, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	type_name = ezstat_ainput->input_type_name;

	if ( mx_strcasecmp( type_name, "galvanostat_fuel_cell_current" ) == 0 )
	{
		ezstat_ainput->input_type =
			MXT_NE_AINPUT_GALVANOSTAT_FUEL_CELL_CURRENT;
	} else
	if ( mx_strcasecmp( type_name, "fuel_cell_voltage_drop" ) == 0 )
	{
		ezstat_ainput->input_type =
			MXT_NE_AINPUT_FUEL_CELL_VOLTAGE_DROP;
	} else
	if ( mx_strcasecmp( type_name,
	    "potentiostat_current_feedback_resistor_voltage" ) == 0 )
	{
		ezstat_ainput->input_type =
		  MXT_NE_AINPUT_POTENTIOSTAT_CURRENT_FEEDBACK_RESISTOR_VOLTAGE;
	} else
	if ( mx_strcasecmp( type_name,
	    "potentiostat_fast_scan_fuel_cell_current" ) == 0 )
	{
		ezstat_ainput->input_type =
			MXT_NE_AINPUT_POTENTIOSTAT_FAST_SCAN_FUEL_CELL_CURRENT;
	} else
	if ( mx_strcasecmp( type_name,
	    "fast_scan_fuel_cell_voltage_drop" ) == 0 )
	{
		ezstat_ainput->input_type =
			MXT_NE_AINPUT_FAST_SCAN_FUEL_CELL_VOLTAGE_DROP;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Input type '%s' is not supported for "
		"analog input '%s'.", type_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezstat_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_nuvant_ezstat_ainput_read()";

	MX_NUVANT_EZSTAT_AINPUT *ezstat_ainput = NULL;
	MX_NUVANT_EZSTAT *ezstat = NULL;
	double ai_value_array[6];
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezstat_ainput_get_pointers( ainput,
						&ezstat_ainput, &ezstat, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_nuvant_ezstat_read_ai_values( ezstat, ai_value_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezstat_ainput->input_type ) {
	case MXT_NE_AINPUT_GALVANOSTAT_FUEL_CELL_CURRENT:
		ainput->raw_value.double_value 
			= mx_divide_safely( ai_value_array[0],
				ezstat->galvanostat_resistance );
		break;
	case MXT_NE_AINPUT_FUEL_CELL_VOLTAGE_DROP:
		if ( fabs(ai_value_array[1]) >= 0.05 ) {
			ainput->raw_value.double_value = - ai_value_array[1];
		} else {
			ainput->raw_value.double_value
				= 101.0 * ai_value_array[2];
		}
		break;
	case MXT_NE_AINPUT_POTENTIOSTAT_CURRENT_FEEDBACK_RESISTOR_VOLTAGE:
		ainput->raw_value.double_value = ai_value_array[3];
		break;
	case MXT_NE_AINPUT_POTENTIOSTAT_FAST_SCAN_FUEL_CELL_CURRENT:
		ainput->raw_value.double_value = 0.2 * ai_value_array[4];
		break;
	case MXT_NE_AINPUT_FAST_SCAN_FUEL_CELL_VOLTAGE_DROP:
		ainput->raw_value.double_value = ai_value_array[5];
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal input type %lu requested for record '%s'.",
			ezstat_ainput->input_type,
			ainput->record->name );
		break;
	}

	return mx_status;
}

