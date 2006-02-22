/*
 * Name:    d_soft_ainput.c
 *
 * Purpose: MX soft analog input device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "mx_analog_input.h"
#include "d_soft_ainput.h"

MX_RECORD_FUNCTION_LIST mxd_soft_ainput_record_function_list = {
	NULL,
	mxd_soft_ainput_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_soft_ainput_analog_input_function_list = {
	mxd_soft_ainput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_ainput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_SOFT_AINPUT_STANDARD_FIELDS
};

long mxd_soft_ainput_num_record_fields
		= sizeof( mxd_soft_ainput_record_field_defaults )
			/ sizeof( mxd_soft_ainput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_ainput_rfield_def_ptr
			= &mxd_soft_ainput_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_soft_ainput_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_soft_ainput_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_SOFT_AINPUT *soft_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        soft_ainput = (MX_SOFT_AINPUT *)
				malloc( sizeof(MX_SOFT_AINPUT) );

        if ( soft_ainput == (MX_SOFT_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SOFT_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = soft_ainput;
        record->class_specific_function_list
			= &mxd_soft_ainput_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_ainput_read( MX_ANALOG_INPUT *ainput )
{
	const char fname[] = "mxd_soft_ainput_read()";

	MX_SOFT_AINPUT *soft_ainput;
	double raw_value;
	mx_status_type mx_status;

	soft_ainput = (MX_SOFT_AINPUT *) ainput->record->record_type_struct;

	if ( soft_ainput == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SOFT_AINPUT pointer for analog input '%s' is NULL",
			ainput->record->name );
	}

	mx_status = mx_analog_output_read( soft_ainput->aoutput_record,
						&raw_value );

	ainput->raw_value.double_value = raw_value;

	return mx_status;
}

