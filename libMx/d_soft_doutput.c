/*
 * Name:    d_soft_doutput.c
 *
 * Purpose: MX soft digital output device driver.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2008, 2019 Illinois Institute of Technology
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
#include "mx_net.h"
#include "mx_digital_output.h"
#include "d_soft_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_soft_doutput_record_function_list = {
	NULL,
	mxd_soft_doutput_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_soft_doutput_digital_output_function_list = {
	mxd_soft_doutput_read,
	mxd_soft_doutput_write,
	mxd_soft_doutput_pulse
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_SOFT_DOUTPUT_STANDARD_FIELDS
};

long mxd_soft_doutput_num_record_fields
		= sizeof( mxd_soft_doutput_record_field_defaults )
			/ sizeof( mxd_soft_doutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_doutput_rfield_def_ptr
			= &mxd_soft_doutput_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_soft_doutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_soft_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_SOFT_DOUTPUT *soft_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
			malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        soft_doutput = (MX_SOFT_DOUTPUT *)
				malloc( sizeof(MX_SOFT_DOUTPUT) );

        if ( soft_doutput == (MX_SOFT_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SOFT_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = soft_doutput;
        record->class_specific_function_list
			= &mxd_soft_doutput_digital_output_function_list;

        digital_output->record = record;
	soft_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_soft_doutput_write()";

	MX_DEBUG( 2,("%s: writing value %#lx (%lu)", fname,
				doutput->value, doutput->value ));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_doutput_pulse( MX_DIGITAL_OUTPUT *doutput )
{
	mx_status_type mx_status;

	mx_status = mx_digital_output_pulse_wait( doutput->record,
						doutput->pulse_on_value,
						doutput->pulse_off_value,
						doutput->pulse_duration,
						FALSE );

	return mx_status;
}

