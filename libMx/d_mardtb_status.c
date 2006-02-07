/*
 * Name:    d_mardtb_status.c
 *
 * Purpose: MX driver for the MarUSA Desktop Beamline's 'status_dump' command.
 *
 *          The driver reads out the value of a particular parameter.  The
 *          most interesting parameters are parameter 136 and 137, which
 *          are the readouts for the goniostat ion chambers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MARDTB_STATUS_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_mardtb.h"
#include "d_mardtb_status.h"

MX_RECORD_FUNCTION_LIST mxd_mardtb_status_record_function_list = {
	NULL,
	mxd_mardtb_status_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_mardtb_status_analog_input_function_list = {
	mxd_mardtb_status_read
};

MX_RECORD_FIELD_DEFAULTS mxd_mardtb_status_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_INT32_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_MARDTB_STATUS_STANDARD_FIELDS
};

mx_length_type mxd_mardtb_status_num_record_fields
		= sizeof( mxd_mardtb_status_record_field_defaults )
			/ sizeof( mxd_mardtb_status_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mardtb_status_rfield_def_ptr
			= &mxd_mardtb_status_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_mardtb_status_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_mardtb_status_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_MARDTB_STATUS *mardtb_status;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        mardtb_status = (MX_MARDTB_STATUS *)
				malloc( sizeof(MX_MARDTB_STATUS) );

        if ( mardtb_status == (MX_MARDTB_STATUS *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MARDTB_STATUS structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = mardtb_status;
        record->class_specific_function_list
			= &mxd_mardtb_status_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as 32-bit integers. */

	analog_input->subclass = MXT_AIN_INT32;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mardtb_status_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_mardtb_status_read()";

	MX_MARDTB_STATUS *mardtb_status;
	MX_MARDTB *mardtb;
	MX_RECORD *mardtb_record;
	unsigned long raw_value;
	mx_status_type mx_status;

	mardtb_status = (MX_MARDTB_STATUS *)
				ainput->record->record_type_struct;

	if ( mardtb_status == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MARDTB_STATUS pointer for analog input '%s' is NULL",
			ainput->record->name );
	}

	mardtb_record = mardtb_status->mardtb_record;

	if ( mardtb_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mardtb_record pointer for analog input '%s' is NULL.",
			ainput->record->name );
	}

	mardtb = (MX_MARDTB *) mardtb_record->record_type_struct;

	if ( mardtb == (MX_MARDTB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MARDTB pointer for Mar DTB record '%s' is NULL.",
			mardtb_record->name );
	}

	mx_status = mxi_mardtb_read_status_parameter( mardtb,
						mardtb_status->parameter_number,
						&raw_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ainput->raw_value.int32_value = (int32_t) raw_value;

	return mx_status;
}

