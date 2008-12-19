/*
 * Name:    d_spellman_df3_aio.c
 *
 * Purpose: MX analog I/O drivers for the Spellman DF3/FF3 series of
 *          high voltage power supplies.
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

#define MXD_SPELLMAN_DF3_AIO_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "i_spellman_df3.h"
#include "d_spellman_df3_aio.h"

/* Initialize the analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_spellman_df3_ain_record_function_list = {
	NULL,
	mxd_spellman_df3_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_spellman_df3_ain_analog_input_function_list =
{
	mxd_spellman_df3_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_spellman_df3_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_SPELLMAN_DF3_AINPUT_STANDARD_FIELDS
};

long mxd_spellman_df3_ain_num_record_fields
		= sizeof( mxd_spellman_df3_ain_record_field_defaults )
		    / sizeof( mxd_spellman_df3_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_spellman_df3_ain_rfield_def_ptr
			= &mxd_spellman_df3_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_spellman_df3_aout_record_function_list = {
	NULL,
	mxd_spellman_df3_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_spellman_df3_aout_analog_output_function_list =
{
	mxd_spellman_df3_aout_read,
	mxd_spellman_df3_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_spellman_df3_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_SPELLMAN_DF3_AOUTPUT_STANDARD_FIELDS
};

long mxd_spellman_df3_aout_num_record_fields
		= sizeof( mxd_spellman_df3_aout_record_field_defaults )
			/ sizeof( mxd_spellman_df3_aout_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_spellman_df3_aout_rfield_def_ptr
			= &mxd_spellman_df3_aout_record_field_defaults[0];

static mx_status_type
mxd_spellman_df3_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_SPELLMAN_DF3_AINPUT **spellman_df3_ainput,
			MX_SPELLMAN_DF3 **spellman_df3,
			const char *calling_fname )
{
	static const char fname[] = "mxd_spellman_df3_ain_get_pointers()";

	MX_RECORD *spellman_df3_record;
	MX_SPELLMAN_DF3_AINPUT *spellman_df3_ainput_ptr;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	spellman_df3_ainput_ptr = (MX_SPELLMAN_DF3_AINPUT *)
					ainput->record->record_type_struct;

	if ( spellman_df3_ainput_ptr == (MX_SPELLMAN_DF3_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_SPELLMAN_DF3_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( spellman_df3_ainput != (MX_SPELLMAN_DF3_AINPUT **) NULL ) {
		*spellman_df3_ainput = spellman_df3_ainput_ptr;
	}

	if ( spellman_df3 != (MX_SPELLMAN_DF3 **) NULL ) {
		spellman_df3_record =
			spellman_df3_ainput_ptr->spellman_df3_record;

		if ( spellman_df3_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SPELLMAN_DF3 pointer for SPELLMAN_DF3 analog "
			"input record '%s' passed by '%s' is NULL.",
				ainput->record->name, calling_fname );
		}

		*spellman_df3 = (MX_SPELLMAN_DF3 *)
			spellman_df3_record->record_type_struct;

		if ( *spellman_df3 == (MX_SPELLMAN_DF3 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SPELLMAN_DF3 pointer for Spellman DF3/FF3  '%s' "
			"used by analog input record '%s'.",
				spellman_df3_record->name,
				ainput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_spellman_df3_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_SPELLMAN_DF3_AOUTPUT **spellman_df3_aoutput,
			MX_SPELLMAN_DF3 **spellman_df3,
			const char *calling_fname )
{
	static const char fname[] = "mxd_spellman_df3_aout_get_pointers()";

	MX_RECORD *spellman_df3_record;
	MX_SPELLMAN_DF3_AOUTPUT *spellman_df3_aoutput_ptr;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	spellman_df3_aoutput_ptr = (MX_SPELLMAN_DF3_AOUTPUT *)
					aoutput->record->record_type_struct;

	if ( spellman_df3_aoutput_ptr == (MX_SPELLMAN_DF3_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_SPELLMAN_DF3_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( spellman_df3_aoutput != (MX_SPELLMAN_DF3_AOUTPUT **) NULL ) {
		*spellman_df3_aoutput = spellman_df3_aoutput_ptr;
	}

	if ( spellman_df3 != (MX_SPELLMAN_DF3 **) NULL ) {
		spellman_df3_record =
			spellman_df3_aoutput_ptr->spellman_df3_record;

		if ( spellman_df3_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SPELLMAN_DF3 pointer for SPELLMAN_DF3 analog "
			"output record '%s' passed by '%s' is NULL.",
				aoutput->record->name, calling_fname );
		}

		*spellman_df3 = (MX_SPELLMAN_DF3 *)
			spellman_df3_record->record_type_struct;

		if ( *spellman_df3 == (MX_SPELLMAN_DF3 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SPELLMAN_DF3 pointer for Spellman DF3/FF3  '%s' "
			"used by analog output record '%s'.",
				spellman_df3_record->name,
				aoutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_spellman_df3_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_spellman_df3_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_SPELLMAN_DF3_AINPUT *spellman_df3_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        spellman_df3_ainput = (MX_SPELLMAN_DF3_AINPUT *)
				malloc( sizeof(MX_SPELLMAN_DF3_AINPUT) );

        if ( spellman_df3_ainput == (MX_SPELLMAN_DF3_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_SPELLMAN_DF3_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = spellman_df3_ainput;
        record->class_specific_function_list
			= &mxd_spellman_df3_ain_analog_input_function_list;

        analog_input->record = record;
	spellman_df3_ainput->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spellman_df3_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_spellman_df3_ain_read()";

	MX_SPELLMAN_DF3_AINPUT *spellman_df3_ainput = NULL;
	MX_SPELLMAN_DF3 *spellman_df3 = NULL;
	unsigned long offset, num_analog_monitors;
	mx_status_type mx_status;

	mx_status = mxd_spellman_df3_ain_get_pointers( ainput,
				&spellman_df3_ainput, &spellman_df3, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a query command to the power supply. */

	mx_status = mxi_spellman_df3_query_command( spellman_df3 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Fetch the value from the analog monitor array. */

	offset = spellman_df3_ainput->input_type;

	num_analog_monitors = sizeof(spellman_df3->analog_monitor)
				/ sizeof(spellman_df3->analog_monitor[0]);

	if ( offset >= num_analog_monitors ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested input type %lu for Spellman DF3/FF3 "
		"analog input '%s' is larger than the maximum value of %lu.",
			offset, ainput->record->name,
			num_analog_monitors - 1 );
	}

	ainput->raw_value.long_value = spellman_df3->analog_monitor[offset];

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_spellman_df3_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_spellman_df3_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_SPELLMAN_DF3_AOUTPUT *spellman_df3_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        spellman_df3_aoutput = (MX_SPELLMAN_DF3_AOUTPUT *)
				malloc( sizeof(MX_SPELLMAN_DF3_AOUTPUT) );

        if ( spellman_df3_aoutput == (MX_SPELLMAN_DF3_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_SPELLMAN_DF3_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = spellman_df3_aoutput;
        record->class_specific_function_list
			= &mxd_spellman_df3_aout_analog_output_function_list;

        analog_output->record = record;
	spellman_df3_aoutput->record = record;

	/* Raw analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spellman_df3_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spellman_df3_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_spellman_df3_aout_write()";

	MX_SPELLMAN_DF3_AOUTPUT *spellman_df3_aoutput = NULL;
	MX_SPELLMAN_DF3 *spellman_df3 = NULL;
	unsigned long offset, num_analog_controls;
	mx_status_type mx_status;

	mx_status = mxd_spellman_df3_aout_get_pointers( aoutput,
				&spellman_df3_aoutput, &spellman_df3, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	offset = spellman_df3_aoutput->output_type;

	num_analog_controls = sizeof(spellman_df3->analog_control)
				/ sizeof(spellman_df3->analog_control[0]);

	if ( offset >= num_analog_controls ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested output type %lu for Spellman DF3/FF3 "
		"analog output '%s' is larger than the maximum value of %lu.",
			offset, aoutput->record->name,
			num_analog_controls - 1 );
	}

	spellman_df3->analog_control[offset] = aoutput->raw_value.long_value;

	mx_status = mxi_spellman_df3_set_command( spellman_df3 );

	return mx_status;
}

