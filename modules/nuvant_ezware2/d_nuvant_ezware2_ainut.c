/*
 * Name:    d_nuvant_ezware2_ainput.c
 *
 * Purpose: MX driver for NuVant EZWare2 analog input channels.
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

#define MXD_NUVANT_EZWARE2_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_nuvant_ezware2.h"
#include "d_nuvant_ezware2_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_nuvant_ezware2_ainput_record_function_list = {
	NULL,
	mxd_nuvant_ezware2_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_nuvant_ezware2_ainput_open
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_nuvant_ezware2_ainput_analog_input_function_list =
{
	mxd_nuvant_ezware2_ainput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_nuvant_ezware2_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_NUVANT_EZWARE2_AINPUT_STANDARD_FIELDS
};

long mxd_nuvant_ezware2_ainput_num_record_fields
		= sizeof( mxd_nuvant_ezware2_ainput_rf_defaults )
		  / sizeof( mxd_nuvant_ezware2_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_nuvant_ezware2_ainput_rfield_def_ptr
			= &mxd_nuvant_ezware2_ainput_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_nuvant_ezware2_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_NUVANT_EZWARE2_AINPUT **ezware2_ainput,
				MX_NUVANT_EZWARE2 **ezware2,
				const char *calling_fname )
{
	static const char fname[] = "mxd_nuvant_ezware2_ainput_get_pointers()";

	MX_NUVANT_EZWARE2_AINPUT *ezware2_ainput_ptr;
	MX_RECORD *ezware2_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	ezware2_ainput_ptr = (MX_NUVANT_EZWARE2_AINPUT *)
				ainput->record->record_type_struct;

	if ( ezware2_ainput_ptr == (MX_NUVANT_EZWARE2_AINPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUVANT_EZWARE2_AINPUT pointer for "
			"analog input '%s' passed by '%s' is NULL",
				ainput->record->name, calling_fname );
	}

	if ( ezware2_ainput != (MX_NUVANT_EZWARE2_AINPUT **) NULL ) {
		*ezware2_ainput = ezware2_ainput_ptr;
	}

	if ( ezware2 != (MX_NUVANT_EZWARE2 **) NULL ) {
		ezware2_record = ezware2_ainput_ptr->nuvant_ezware2_record;

		if ( ezware2_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The nuvant_ezware2_record pointer for "
			"analog input '%s' is NULL.",
				ainput->record->name );
		}

		*ezware2 = (MX_NUVANT_EZWARE2 *)ezware2_record->record_type_struct;

		if ( (*ezware2) == (MX_NUVANT_EZWARE2 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NUVANT_EZWARE2 pointer for record '%s' "
			"used by analog input '%s' is NULL.",
				ezware2_record->name,
				ainput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_nuvant_ezware2_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_NUVANT_EZWARE2_AINPUT *nuvant_ezware2_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ANALOG_INPUT structure." );
	}

	nuvant_ezware2_ainput = (MX_NUVANT_EZWARE2_AINPUT *)
				malloc( sizeof(MX_NUVANT_EZWARE2_AINPUT) );

	if ( nuvant_ezware2_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_NUVANT_EZWARE2_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = nuvant_ezware2_ainput;
	record->class_specific_function_list
			= &mxd_nuvant_ezware2_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_nuvant_ezware2_ainput_open()";

	MX_ANALOG_INPUT *ainput = NULL;
	MX_NUVANT_EZWARE2_AINPUT *ezware2_ainput = NULL;
	MX_NUVANT_EZWARE2 *ezware2 = NULL;
	char *type_name = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_nuvant_ezware2_ainput_get_pointers( ainput,
					&ezware2_ainput, &ezware2, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	type_name = ezware2_ainput->input_type_name;

	if ( mx_strcasecmp( type_name, "ai0" ) == 0 )
	{
		ezware2_ainput->input_type = MXT_NE_AINPUT_AI0;
	} else
	if ( mx_strcasecmp( type_name, "ai1" ) == 0 )
	{
		ezware2_ainput->input_type = MXT_NE_AINPUT_AI1;
	} else
	if ( mx_strcasecmp( type_name, "ai2" ) == 0 )
	{
		ezware2_ainput->input_type = MXT_NE_AINPUT_AI2;
	} else
	if ( mx_strcasecmp( type_name, "ai3" ) == 0 )
	{
		ezware2_ainput->input_type = MXT_NE_AINPUT_AI3;
	} else
	if ( mx_strcasecmp( type_name, "ai14" ) == 0 )
	{
		ezware2_ainput->input_type = MXT_NE_AINPUT_AI14;
	} else
	if ( mx_strcasecmp( type_name, "ai15" ) == 0 )
	{
		ezware2_ainput->input_type = MXT_NE_AINPUT_AI15;
	} else
	if ( mx_strcasecmp( type_name, "galvanostat_fuel_cell_current" ) == 0 )
	{
		ezware2_ainput->input_type =
			MXT_NE_AINPUT_GALVANOSTAT_FUEL_CELL_CURRENT;
	} else
	if ( mx_strcasecmp( type_name, "fuel_cell_voltage_drop" ) == 0 )
	{
		ezware2_ainput->input_type =
			MXT_NE_AINPUT_FUEL_CELL_VOLTAGE_DROP;
	} else
	if ( mx_strcasecmp( type_name,
	    "potentiostat_current_feedback_resistor_voltage" ) == 0 )
	{
		ezware2_ainput->input_type =
		  MXT_NE_AINPUT_POTENTIOSTAT_CURRENT_FEEDBACK_RESISTOR_VOLTAGE;
	} else
	if ( mx_strcasecmp( type_name,
	    "potentiostat_fast_scan_fuel_cell_current" ) == 0 )
	{
		ezware2_ainput->input_type =
			MXT_NE_AINPUT_POTENTIOSTAT_FAST_SCAN_FUEL_CELL_CURRENT;
	} else
	if ( mx_strcasecmp( type_name,
	    "fast_scan_fuel_cell_voltage_drop" ) == 0 )
	{
		ezware2_ainput->input_type =
			MXT_NE_AINPUT_FAST_SCAN_FUEL_CELL_VOLTAGE_DROP;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Input type '%s' is not supported for "
		"analog input '%s'.", type_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_nuvant_ezware2_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_nuvant_ezware2_ainput_read()";

	MX_NUVANT_EZWARE2_AINPUT *ezware2_ainput = NULL;
	MX_NUVANT_EZWARE2 *ezware2 = NULL;
	double ai_value_array[6];
	mx_status_type mx_status;

	mx_status = mxd_nuvant_ezware2_ainput_get_pointers( ainput,
						&ezware2_ainput, &ezware2, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_nuvant_ezware2_read_ai_values( ezware2, ai_value_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ezware2_ainput->input_type ) {
	case MXT_NE_AINPUT_AI0:
		ainput->raw_value.double_value = ai_value_array[0];
		break;
	case MXT_NE_AINPUT_AI1:
		ainput->raw_value.double_value = ai_value_array[1];
		break;
	case MXT_NE_AINPUT_AI2:
		ainput->raw_value.double_value = ai_value_array[2];
		break;
	case MXT_NE_AINPUT_AI3:
		ainput->raw_value.double_value = ai_value_array[3];
		break;
	case MXT_NE_AINPUT_AI14:
		ainput->raw_value.double_value = ai_value_array[4];
		break;
	case MXT_NE_AINPUT_AI15:
		ainput->raw_value.double_value = ai_value_array[5];
		break;

	case MXT_NE_AINPUT_GALVANOSTAT_FUEL_CELL_CURRENT:
		ainput->raw_value.double_value 
			= mx_divide_safely( ai_value_array[0],
				ezware2->galvanostat_resistance );
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
			ezware2_ainput->input_type,
			ainput->record->name );
		break;
	}

	return mx_status;
}

