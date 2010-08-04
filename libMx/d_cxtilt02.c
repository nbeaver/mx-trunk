/*
 * Name:    d_cxtilt02.c
 *
 * Purpose: MX analog input driver that reads one of the angles from the
 *          Crossbow Technology CXTILT02 series of digital inclinometers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_CXTILT02_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_cxtilt02.h"
#include "d_cxtilt02.h"

/* Initialize the CXTILT02 analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_cxtilt02_record_function_list = {
	NULL,
	mxd_cxtilt02_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_cxtilt02_analog_input_function_list = {
	mxd_cxtilt02_read
};

MX_RECORD_FIELD_DEFAULTS mxd_cxtilt02_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_CXTILT02_ANGLE_STANDARD_FIELDS
};

long mxd_cxtilt02_num_record_fields
		= sizeof( mxd_cxtilt02_record_field_defaults )
			/ sizeof( mxd_cxtilt02_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_cxtilt02_rfield_def_ptr
			= &mxd_cxtilt02_record_field_defaults[0];

static mx_status_type
mxd_cxtilt02_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_CXTILT02_ANGLE **cxtilt02_angle,
			MX_CXTILT02 **cxtilt02,
			const char *calling_fname )
{
	static const char fname[] = "mxd_cxtilt02_get_pointers()";

	MX_RECORD *cxtilt02_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( cxtilt02_angle == (MX_CXTILT02_ANGLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_CXTILT02_ANGLE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( cxtilt02 == (MX_CXTILT02 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_CXTILT02 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*cxtilt02_angle = (MX_CXTILT02_ANGLE *)
				ainput->record->record_type_struct;

	if ( *cxtilt02_angle == (MX_CXTILT02_ANGLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_CXTILT02_ANGLE pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	cxtilt02_record = (*cxtilt02_angle)->cxtilt02_record;

	if ( cxtilt02_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CXTILT02 pointer for CXTILT02 analog input "
		"record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( cxtilt02_record->mx_type != MXI_CTRL_CXTILT02 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"cxtilt02_record '%s' for CXTILT02 analog input '%s' "
		"is not a CXTILT02 record.  Instead, it is a '%s' record.",
			cxtilt02_record->name, ainput->record->name,
			mx_get_driver_name( cxtilt02_record ) );
	}

	*cxtilt02 = (MX_CXTILT02 *) cxtilt02_record->record_type_struct;

	if ( *cxtilt02 == (MX_CXTILT02 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_CXTILT02 pointer for CXTILT02 record '%s' used by "
	"CXTILT02 analog input record '%s' and passed by '%s' is NULL.",
			cxtilt02_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_cxtilt02_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_cxtilt02_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_CXTILT02_ANGLE *cxtilt02_angle;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        cxtilt02_angle = (MX_CXTILT02_ANGLE *)
				malloc( sizeof(MX_CXTILT02_ANGLE) );

        if ( cxtilt02_angle == (MX_CXTILT02_ANGLE *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_CXTILT02_ANGLE structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = cxtilt02_angle;
        record->class_specific_function_list
			= &mxd_cxtilt02_analog_input_function_list;

        analog_input->record = record;
	cxtilt02_angle->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cxtilt02_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_cxtilt02_read()";

	MX_CXTILT02_ANGLE *cxtilt02_angle;
	MX_CXTILT02 *cxtilt02;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	cxtilt02 = NULL;
	cxtilt02_angle = NULL;

	mx_status = mxd_cxtilt02_get_pointers( ainput,
					&cxtilt02_angle, &cxtilt02, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_cxtilt02_read_angles( cxtilt02 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( cxtilt02_angle->angle_id == MXF_CXTILT02_PITCH ) {
		ainput->raw_value.long_value = cxtilt02->raw_pitch;
	} else
	if ( cxtilt02_angle->angle_id == MXF_CXTILT02_ROLL ) {
		ainput->raw_value.long_value = cxtilt02->raw_roll;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal angle ID for CXTILT02 angle record '%s'.  "
		"The allowed values are '1' for pitch and '2' for roll.",
			ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

