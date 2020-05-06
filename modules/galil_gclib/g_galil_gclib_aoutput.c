/*
 * Name:    d_galil_gclib_aoutput.c
 *
 * Purpose: MX driver for Galil gclib controller analog output.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
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
#include "d_galil_gclib_aoutput.h"

MX_RECORD_FUNCTION_LIST mxd_galil_gclib_aoutput_record_function_list = {
	NULL,
	mxd_galil_gclib_aoutput_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_galil_gclib_aoutput_analog_output_function_list = {
	mxd_galil_gclib_aoutput_read,
	mxd_galil_gclib_aoutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_galil_gclib_aoutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_GALIL_GCLIB_AOUTPUT_STANDARD_FIELDS
};

long mxd_galil_gclib_aoutput_num_record_fields
		= sizeof( mxd_galil_gclib_aoutput_record_field_defaults )
			/ sizeof( mxd_galil_gclib_aoutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_aoutput_rfield_def_ptr
			= &mxd_galil_gclib_aoutput_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_galil_gclib_aoutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_galil_gclib_aoutput_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_GALIL_GCLIB_AOUTPUT *galil_gclib_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        galil_gclib_aoutput = (MX_GALIL_GCLIB_AOUTPUT *)
				malloc( sizeof(MX_GALIL_GCLIB_AOUTPUT) );

        if ( galil_gclib_aoutput == (MX_GALIL_GCLIB_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_GALIL_GCLIB_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = galil_gclib_aoutput;
        record->class_specific_function_list
			= &mxd_galil_gclib_aoutput_analog_output_function_list;

        analog_output->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_galil_gclib_aoutput_read( MX_ANALOG_OUTPUT *aoutput )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_galil_gclib_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	return MX_SUCCESSFUL_RESULT;
}

