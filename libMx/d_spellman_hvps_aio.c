/*
 * Name:    d_spellman_hvps_aio.c
 *
 * Purpose: MX analog I/O drivers for the Spellman high voltage
 *          power supply.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SPELLMAN_HVPS_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "i_spellman_hvps.h"
#include "d_spellman_hvps_aio.h"

/* Initialize the analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_spellman_hvps_ain_record_function_list = {
	NULL,
	mxd_spellman_hvps_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_spellman_hvps_ain_analog_input_function_list =
{
	mxd_spellman_hvps_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_spellman_hvps_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_SPELLMAN_HVPS_AINPUT_STANDARD_FIELDS
};

long mxd_spellman_hvps_ain_num_record_fields
		= sizeof( mxd_spellman_hvps_ain_record_field_defaults )
		    / sizeof( mxd_spellman_hvps_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_spellman_hvps_ain_rfield_def_ptr
			= &mxd_spellman_hvps_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_spellman_hvps_aout_record_function_list = {
	NULL,
	mxd_spellman_hvps_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_spellman_hvps_aout_analog_output_function_list =
{
	mxd_spellman_hvps_aout_read,
	mxd_spellman_hvps_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_spellman_hvps_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_SPELLMAN_HVPS_AOUTPUT_STANDARD_FIELDS
};

long mxd_spellman_hvps_aout_num_record_fields
		= sizeof( mxd_spellman_hvps_aout_record_field_defaults )
			/ sizeof( mxd_spellman_hvps_aout_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_spellman_hvps_aout_rfield_def_ptr
			= &mxd_spellman_hvps_aout_record_field_defaults[0];

static mx_status_type
mxd_spellman_hvps_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_SPELLMAN_HVPS_AINPUT **spellman_hvps_ainput,
			MX_SPELLMAN_HVPS **spellman_hvps,
			const char *calling_fname )
{
	static const char fname[] = "mxd_spellman_hvps_ain_get_pointers()";

	MX_RECORD *spellman_hvps_record;
	MX_SPELLMAN_HVPS_AINPUT *spellman_hvps_ainput_ptr;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	spellman_hvps_ainput_ptr = (MX_SPELLMAN_HVPS_AINPUT *)
					ainput->record->record_type_struct;

	if ( spellman_hvps_ainput_ptr == (MX_SPELLMAN_HVPS_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_SPELLMAN_HVPS_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( spellman_hvps_ainput != (MX_SPELLMAN_HVPS_AINPUT **) NULL ) {
		*spellman_hvps_ainput = spellman_hvps_ainput_ptr;
	}

	if ( spellman_hvps != (MX_SPELLMAN_HVPS **) NULL ) {
		spellman_hvps_record =
			spellman_hvps_ainput_ptr->spellman_hvps_record;

		if ( spellman_hvps_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SPELLMAN_HVPS pointer for SPELLMAN_HVPS analog "
			"input record '%s' passed by '%s' is NULL.",
				ainput->record->name, calling_fname );
		}

		*spellman_hvps = (MX_SPELLMAN_HVPS *)
			spellman_hvps_record->record_type_struct;

		if ( *spellman_hvps == (MX_SPELLMAN_HVPS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SPELLMAN_HVPS pointer for Spellman HVPS  '%s' "
			"used by analog input record '%s'.",
				spellman_hvps_record->name,
				ainput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_spellman_hvps_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_SPELLMAN_HVPS_AOUTPUT **spellman_hvps_aoutput,
			MX_SPELLMAN_HVPS **spellman_hvps,
			const char *calling_fname )
{
	static const char fname[] = "mxd_spellman_hvps_aout_get_pointers()";

	MX_RECORD *spellman_hvps_record;
	MX_SPELLMAN_HVPS_AOUTPUT *spellman_hvps_aoutput_ptr;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	spellman_hvps_aoutput_ptr = (MX_SPELLMAN_HVPS_AOUTPUT *)
					aoutput->record->record_type_struct;

	if ( spellman_hvps_aoutput_ptr == (MX_SPELLMAN_HVPS_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_SPELLMAN_HVPS_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( spellman_hvps_aoutput != (MX_SPELLMAN_HVPS_AOUTPUT **) NULL ) {
		*spellman_hvps_aoutput = spellman_hvps_aoutput_ptr;
	}

	if ( spellman_hvps != (MX_SPELLMAN_HVPS **) NULL ) {
		spellman_hvps_record =
			spellman_hvps_aoutput_ptr->spellman_hvps_record;

		if ( spellman_hvps_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SPELLMAN_HVPS pointer for SPELLMAN_HVPS analog "
			"output record '%s' passed by '%s' is NULL.",
				aoutput->record->name, calling_fname );
		}

		*spellman_hvps = (MX_SPELLMAN_HVPS *)
			spellman_hvps_record->record_type_struct;

		if ( *spellman_hvps == (MX_SPELLMAN_HVPS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SPELLMAN_HVPS pointer for Spellman HVPS  '%s' "
			"used by analog output record '%s'.",
				spellman_hvps_record->name,
				aoutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_spellman_hvps_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_spellman_hvps_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_SPELLMAN_HVPS_AINPUT *spellman_hvps_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        spellman_hvps_ainput = (MX_SPELLMAN_HVPS_AINPUT *)
				malloc( sizeof(MX_SPELLMAN_HVPS_AINPUT) );

        if ( spellman_hvps_ainput == (MX_SPELLMAN_HVPS_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_SPELLMAN_HVPS_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = spellman_hvps_ainput;
        record->class_specific_function_list
			= &mxd_spellman_hvps_ain_analog_input_function_list;

        analog_input->record = record;
	spellman_hvps_ainput->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spellman_hvps_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_spellman_hvps_ain_read()";

	MX_SPELLMAN_HVPS_AINPUT *spellman_hvps_ainput;
	MX_SPELLMAN_HVPS *spellman_hvps;
	unsigned long offset, num_analog_monitors;
	mx_status_type mx_status;

	mx_status = mxd_spellman_hvps_ain_get_pointers( ainput,
				&spellman_hvps_ainput, &spellman_hvps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a query command to the power supply. */

	mx_status = mxi_spellman_hvps_query_command( spellman_hvps );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Fetch the value from the analog monitor array. */

	offset = spellman_hvps_ainput->input_type;

	num_analog_monitors = sizeof(spellman_hvps->analog_monitor)
				/ sizeof(spellman_hvps->analog_monitor[0]);

	if ( offset >= num_analog_monitors ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested input type %lu for Spellman HVPS "
		"analog input '%s' is larger than the maximum value of %lu.",
			offset, ainput->record->name,
			num_analog_monitors - 1 );
	}

	ainput->raw_value.long_value = spellman_hvps->analog_monitor[offset];

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_spellman_hvps_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_spellman_hvps_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_SPELLMAN_HVPS_AOUTPUT *spellman_hvps_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        spellman_hvps_aoutput = (MX_SPELLMAN_HVPS_AOUTPUT *)
				malloc( sizeof(MX_SPELLMAN_HVPS_AOUTPUT) );

        if ( spellman_hvps_aoutput == (MX_SPELLMAN_HVPS_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_SPELLMAN_HVPS_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = spellman_hvps_aoutput;
        record->class_specific_function_list
			= &mxd_spellman_hvps_aout_analog_output_function_list;

        analog_output->record = record;
	spellman_hvps_aoutput->record = record;

	/* Raw analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spellman_hvps_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spellman_hvps_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_spellman_hvps_aout_write()";

	MX_SPELLMAN_HVPS_AOUTPUT *spellman_hvps_aoutput;
	MX_SPELLMAN_HVPS *spellman_hvps;
	unsigned long offset, num_analog_controls;
	mx_status_type mx_status;

	mx_status = mxd_spellman_hvps_aout_get_pointers( aoutput,
				&spellman_hvps_aoutput, &spellman_hvps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	offset = spellman_hvps_aoutput->output_type;

	num_analog_controls = sizeof(spellman_hvps->analog_control)
				/ sizeof(spellman_hvps->analog_control[0]);

	if ( offset >= num_analog_controls ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested output type %lu for Spellman HVPS "
		"analog output '%s' is larger than the maximum value of %lu.",
			offset, aoutput->record->name,
			num_analog_controls - 1 );
	}

	spellman_hvps->analog_control[offset] = aoutput->raw_value.long_value;

	mx_status = mxi_spellman_hvps_set_command( spellman_hvps );

	return mx_status;
}

