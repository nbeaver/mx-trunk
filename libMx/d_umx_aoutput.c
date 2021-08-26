/*
 * Name:    d_umx_aoutput.c
 *
 * Purpose: MX analog output driver for UMX analog outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
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
#include "mx_umx.h"
#include "d_umx_aoutput.h"

MX_RECORD_FUNCTION_LIST mxd_umx_aoutput_record_function_list = {
	NULL,
	mxd_umx_aoutput_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_umx_aoutput_analog_output_function_list = {
	NULL,
	mxd_umx_aoutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_umx_aoutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_UMX_AOUTPUT_STANDARD_FIELDS
};

long mxd_umx_aoutput_num_record_fields
		= sizeof( mxd_umx_aoutput_record_field_defaults )
			/ sizeof( mxd_umx_aoutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_umx_aoutput_rfield_def_ptr
			= &mxd_umx_aoutput_record_field_defaults[0];

/*----*/

/* A private function for the use of the driver. */

static mx_status_type
mxd_umx_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
				MX_UMX_AOUTPUT **umx_aoutput,
				MX_RECORD **umx_record,
				const char *calling_fname )
{
	static const char fname[] = "mxd_umx_aoutput_get_pointers()";

	MX_RECORD *record = NULL;
	MX_UMX_AOUTPUT *umx_aoutput_ptr = NULL;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = aoutput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_ANALOG_OUTPUT pointer passed was NULL." );
	}

	umx_aoutput_ptr = (MX_UMX_AOUTPUT *) record->record_type_struct;

	if ( umx_aoutput_ptr == (MX_UMX_AOUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_UMX_AOUTPUT pointer for aoutput '%s' is NULL.",
			record->name );
	}

	if ( umx_aoutput != (MX_UMX_AOUTPUT **) NULL ) {
		*umx_aoutput = umx_aoutput_ptr;
	}

	if ( umx_record != (MX_RECORD **) NULL ) {
		*umx_record = umx_aoutput_ptr->umx_record;

		if ( (*umx_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The 'umx_record' pointer for aoutput '%s' is NULL.",
					record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_umx_aoutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_umx_aoutput_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_UMX_AOUTPUT *umx_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        umx_aoutput = (MX_UMX_AOUTPUT *)
				malloc( sizeof(MX_UMX_AOUTPUT) );

        if ( umx_aoutput == (MX_UMX_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_UMX_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = umx_aoutput;
        record->class_specific_function_list
			= &mxd_umx_aoutput_analog_output_function_list;

        analog_output->record = record;
	umx_aoutput->record = record;

	/* Raw UMX analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_umx_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_umx_aoutput_write()";

	MX_UMX_AOUTPUT *umx_aoutput = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_umx_aoutput_get_pointers( aoutput,
					&umx_aoutput, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"PUT %s.value %ld",
			umx_aoutput->aoutput_name, 
			aoutput->raw_value.long_value );

	mx_status = mx_umx_command( umx_record, command,
				response, sizeof(response),
				FALSE );

	return mx_status;
}

