/*
 * Name:    d_aps_adcmod2_ainput.c
 *
 * Purpose: MX driver for reading the signal from the ADCMOD2 electrometer
 *          system designed by Steve Ross of the Advanced Photon Source.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_analog_input.h"

#include "i_aps_adcmod2.h"
#include "d_aps_adcmod2_ainput.h"

MX_RECORD_FUNCTION_LIST mxd_aps_adcmod2_ainput_record_function_list = {
	NULL,
	mxd_aps_adcmod2_ainput_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_aps_adcmod2_ainput_analog_input_function_list = {
	mxd_aps_adcmod2_ainput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_aps_adcmod2_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_INT32_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_APS_ADCMOD2_AINPUT_STANDARD_FIELDS
};

mx_length_type mxd_aps_adcmod2_ainput_num_record_fields
		= sizeof( mxd_aps_adcmod2_ainput_rf_defaults )
			/ sizeof( mxd_aps_adcmod2_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aps_adcmod2_ainput_rfield_def_ptr
			= &mxd_aps_adcmod2_ainput_rf_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_aps_adcmod2_ainput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_aps_adcmod2_ainput_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_APS_ADCMOD2_AINPUT *aps_adcmod2_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        aps_adcmod2_ainput = (MX_APS_ADCMOD2_AINPUT *)
				malloc( sizeof(MX_APS_ADCMOD2_AINPUT) );

        if ( aps_adcmod2_ainput == (MX_APS_ADCMOD2_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_APS_ADCMOD2_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = aps_adcmod2_ainput;
        record->class_specific_function_list
			= &mxd_aps_adcmod2_ainput_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as 32-bit integers. */

	analog_input->subclass = MXT_AIN_INT32;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_adcmod2_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_aps_adcmod2_ainput_read()";

	MX_APS_ADCMOD2_AINPUT *aps_adcmod2_ainput;
	MX_APS_ADCMOD2 *aps_adcmod2;
	MX_RECORD *aps_adcmod2_record;
	uint16_t raw_value;
	mx_status_type mx_status;

	aps_adcmod2_ainput = (MX_APS_ADCMOD2_AINPUT *)
				ainput->record->record_type_struct;

	if ( aps_adcmod2_ainput == (MX_APS_ADCMOD2_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_APS_ADCMOD2_AINPUT pointer for analog input '%s' is NULL",
			ainput->record->name );
	}

	aps_adcmod2_record = aps_adcmod2_ainput->aps_adcmod2_record;

	if ( aps_adcmod2_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The aps_adcmod2_record pointer for record '%s' is NULL.",
			ainput->record->name );
	}

	aps_adcmod2 = (MX_APS_ADCMOD2 *) aps_adcmod2_record->record_type_struct;

	mx_status = mxi_aps_adcmod2_read_value( aps_adcmod2,
					aps_adcmod2_ainput->ainput_number,
					&raw_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ainput->raw_value.int32_value = (int32_t) raw_value;

	return mx_status;
}

