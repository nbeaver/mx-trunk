/*
 * Name:    d_ainput_as_dinput.c
 *
 * Purpose: MX driver for treating an MX analog input as a digital input.
 *          The driver uses an ADC voltage threshold to determine whether
 *          to return a 0 or 1 as digital input.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_analog_input.h"
#include "mx_digital_input.h"
#include "d_ainput_as_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_ainput_as_dinput_record_function_list = {
	NULL,
	mxd_ainput_as_dinput_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_ainput_as_dinput_digital_input_function_list = {
	mxd_ainput_as_dinput_read,
	mxd_ainput_as_dinput_clear
};

MX_RECORD_FIELD_DEFAULTS mxd_ainput_as_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_AINPUT_AS_DINPUT_STANDARD_FIELDS
};

long mxd_ainput_as_dinput_num_record_fields
		= sizeof( mxd_ainput_as_dinput_record_field_defaults )
			/ sizeof( mxd_ainput_as_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ainput_as_dinput_rfield_def_ptr
			= &mxd_ainput_as_dinput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_ainput_as_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_AINPUT_AS_DINPUT **ainput_as_dinput,
			MX_RECORD **analog_input_record,
			const char *calling_fname )
{
	static const char fname[] = "mxd_ainput_as_dinput_get_pointers()";

	MX_AINPUT_AS_DINPUT *ainput_as_dinput_ptr;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	ainput_as_dinput_ptr = (MX_AINPUT_AS_DINPUT *)
				dinput->record->record_type_struct;

	if ( ainput_as_dinput_ptr == (MX_AINPUT_AS_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AINPUT_AS_DINPUT pointer for record '%s' is NULL.",
			dinput->record->name );
	}

	if ( ainput_as_dinput != (MX_AINPUT_AS_DINPUT **) NULL ) {
		*ainput_as_dinput = ainput_as_dinput_ptr;
	}

	if ( analog_input_record != (MX_RECORD **) NULL ) {
		*analog_input_record = (MX_RECORD *)
			ainput_as_dinput_ptr->analog_input_record;

		if ( (*analog_input_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The analog_input_record pointer for record '%s' is NULL.",
				dinput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_ainput_as_dinput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_ainput_as_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_AINPUT_AS_DINPUT *ainput_as_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        ainput_as_dinput = (MX_AINPUT_AS_DINPUT *)
				malloc( sizeof(MX_AINPUT_AS_DINPUT) );

        if ( ainput_as_dinput == (MX_AINPUT_AS_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_AINPUT_AS_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = ainput_as_dinput;
        record->class_specific_function_list
			= &mxd_ainput_as_dinput_digital_input_function_list;

        digital_input->record = record;
	ainput_as_dinput->record = record;

	digital_input->value = 0;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ainput_as_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_ainput_as_dinput_read()";

	MX_AINPUT_AS_DINPUT *ainput_as_dinput = NULL;
	MX_RECORD *analog_input_record = NULL;
	double adc_voltage;
	mx_status_type mx_status;

	mx_status = mxd_ainput_as_dinput_get_pointers( dinput,
						&ainput_as_dinput,
						&analog_input_record,
						fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_analog_input_read( analog_input_record, &adc_voltage );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ainput_as_dinput->high_voltage >= ainput_as_dinput->low_voltage ) {
		if ( adc_voltage >= ainput_as_dinput->threshold_voltage ) {
			dinput->value = 1;
		} else {
			dinput->value = 0;
		}
	} else {
		if ( adc_voltage <= ainput_as_dinput->threshold_voltage ) {
			dinput->value = 1;
		} else {
			dinput->value = 0;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ainput_as_dinput_clear( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_ainput_as_dinput_clear()";

	MX_AINPUT_AS_DINPUT *ainput_as_dinput = NULL;
	MX_RECORD *analog_input_record = NULL;
	mx_status_type mx_status;

	mx_status = mxd_ainput_as_dinput_get_pointers( dinput,
						&ainput_as_dinput,
						&analog_input_record,
						fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_analog_input_clear( analog_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dinput->value = 0;

	return MX_SUCCESSFUL_RESULT;
}

