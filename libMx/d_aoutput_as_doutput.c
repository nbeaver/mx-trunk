/*
 * Name:    d_aoutput_as_doutput.c
 *
 * Purpose: MX digital output driver for treating an MX analog output as
 *          an MX digital output.  The driver commands the analog output
 *          to put out a high or low voltage depending on the value of
 *          the digital output (0/1).
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "mx_digital_output.h"
#include "d_aoutput_as_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_aoutput_as_doutput_record_function_list = {
	NULL,
	mxd_aoutput_as_doutput_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
	mxd_aoutput_as_doutput_digital_output_function_list =
{
	mxd_aoutput_as_doutput_read,
	mxd_aoutput_as_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_aoutput_as_doutput_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_AOUTPUT_AS_DOUTPUT_STANDARD_FIELDS
};

long mxd_aoutput_as_doutput_num_record_fields
		= sizeof( mxd_aoutput_as_doutput_field_default )
		/ sizeof( mxd_aoutput_as_doutput_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aoutput_as_doutput_rfield_def_ptr
			= &mxd_aoutput_as_doutput_field_default[0];

/* ===== */

static mx_status_type
mxd_aoutput_as_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_AOUTPUT_AS_DOUTPUT **aoutput_as_doutput,
				MX_RECORD **analog_output_record,
				const char *calling_fname )
{
	static const char fname[] = "mxd_aoutput_as_doutput_get_pointers()";

	MX_AOUTPUT_AS_DOUTPUT *aoutput_as_doutput_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	aoutput_as_doutput_ptr = (MX_AOUTPUT_AS_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( aoutput_as_doutput_ptr == (MX_AOUTPUT_AS_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AOUTPUT_AS_DOUTPUT pointer for digital output '%s' is NULL",
			doutput->record->name );
	}

	if ( aoutput_as_doutput != (MX_AOUTPUT_AS_DOUTPUT **) NULL ) {
		*aoutput_as_doutput = aoutput_as_doutput_ptr;
	}

	if ( analog_output_record != (MX_RECORD **) NULL ) {
		*analog_output_record =
			aoutput_as_doutput_ptr->analog_output_record;

		if ( (*analog_output_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The analog_output_record pointer for record '%s' is NULL.",
				doutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_aoutput_as_doutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_aoutput_as_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_AOUTPUT_AS_DOUTPUT *aoutput_as_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
				malloc( sizeof(MX_DIGITAL_OUTPUT) );

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an MX_DIGITAL_OUTPUT structure." );
        }

        aoutput_as_doutput = (MX_AOUTPUT_AS_DOUTPUT *)
				malloc( sizeof(MX_AOUTPUT_AS_DOUTPUT) );

        if ( aoutput_as_doutput == (MX_AOUTPUT_AS_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Can't allocate memory for MX_AOUTPUT_AS_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = aoutput_as_doutput;
        record->class_specific_function_list
			= &mxd_aoutput_as_doutput_digital_output_function_list;

        digital_output->record = record;
	aoutput_as_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aoutput_as_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_aoutput_as_doutput_read()";

	MX_AOUTPUT_AS_DOUTPUT *aoutput_as_doutput = NULL;
	MX_RECORD *analog_output_record = NULL;
	double dac_voltage;
	mx_status_type mx_status;

	mx_status = mxd_aoutput_as_doutput_get_pointers( doutput,
						&aoutput_as_doutput,
						&analog_output_record,
						fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_analog_output_read( analog_output_record, &dac_voltage );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( aoutput_as_doutput->high_voltage
		>= aoutput_as_doutput->low_voltage )
	{
		if ( dac_voltage >= aoutput_as_doutput->threshold_voltage ) {
			doutput->value = 1;
		} else {
			doutput->value = 0;
		}
	} else {
		if ( dac_voltage <= aoutput_as_doutput->threshold_voltage ) {
			doutput->value = 1;
		} else {
			doutput->value = 0;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aoutput_as_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_aoutput_as_doutput_write()";

	MX_AOUTPUT_AS_DOUTPUT *aoutput_as_doutput = NULL;
	MX_RECORD *analog_output_record = NULL;
	double dac_voltage;
	mx_status_type mx_status;

	mx_status = mxd_aoutput_as_doutput_get_pointers( doutput,
						&aoutput_as_doutput,
						&analog_output_record,
						fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( doutput->value == 0 ) {
		dac_voltage = aoutput_as_doutput->low_voltage;
	} else {
		dac_voltage = aoutput_as_doutput->high_voltage;
	}

	mx_status = mx_analog_output_write( analog_output_record, dac_voltage );

	return mx_status;
}

