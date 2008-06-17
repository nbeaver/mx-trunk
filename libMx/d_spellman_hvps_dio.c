/*
 * Name:    d_spellman_hvps_dio.c
 *
 * Purpose: MX digital I/O drivers for the Spellman high voltage
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

#define MXD_SPELLMAN_HVPS_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_spellman_hvps.h"
#include "d_spellman_hvps_dio.h"

/* Initialize the digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_spellman_hvps_din_record_function_list = {
	NULL,
	mxd_spellman_hvps_din_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_spellman_hvps_din_digital_input_function_list =
{
	mxd_spellman_hvps_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_spellman_hvps_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_SPELLMAN_HVPS_DINPUT_STANDARD_FIELDS
};

long mxd_spellman_hvps_din_num_record_fields
		= sizeof( mxd_spellman_hvps_din_record_field_defaults )
		    / sizeof( mxd_spellman_hvps_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_spellman_hvps_din_rfield_def_ptr
			= &mxd_spellman_hvps_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_spellman_hvps_dout_record_function_list = {
	NULL,
	mxd_spellman_hvps_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_spellman_hvps_dout_digital_output_function_list =
{
	mxd_spellman_hvps_dout_read,
	mxd_spellman_hvps_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_spellman_hvps_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_SPELLMAN_HVPS_DOUTPUT_STANDARD_FIELDS
};

long mxd_spellman_hvps_dout_num_record_fields
		= sizeof( mxd_spellman_hvps_dout_record_field_defaults )
		    / sizeof( mxd_spellman_hvps_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_spellman_hvps_dout_rfield_def_ptr
			= &mxd_spellman_hvps_dout_record_field_defaults[0];

static mx_status_type
mxd_spellman_hvps_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_SPELLMAN_HVPS_DINPUT **spellman_hvps_dinput,
			MX_SPELLMAN_HVPS **spellman_hvps,
			const char *calling_fname )
{
	static const char fname[] = "mxd_spellman_hvps_din_get_pointers()";

	MX_RECORD *spellman_hvps_record;
	MX_SPELLMAN_HVPS_DINPUT *spellman_hvps_dinput_ptr;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	spellman_hvps_dinput_ptr = (MX_SPELLMAN_HVPS_DINPUT *)
					dinput->record->record_type_struct;

	if ( spellman_hvps_dinput_ptr == (MX_SPELLMAN_HVPS_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_SPELLMAN_HVPS_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( spellman_hvps_dinput != (MX_SPELLMAN_HVPS_DINPUT **) NULL ) {
		*spellman_hvps_dinput = spellman_hvps_dinput_ptr;
	}

	if ( spellman_hvps != (MX_SPELLMAN_HVPS **) NULL ) {
		spellman_hvps_record =
			spellman_hvps_dinput_ptr->spellman_hvps_record;

		if ( spellman_hvps_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SPELLMAN_HVPS pointer for SPELLMAN_HVPS digital "
			"input record '%s' passed by '%s' is NULL.",
				dinput->record->name, calling_fname );
		}

		*spellman_hvps = (MX_SPELLMAN_HVPS *)
			spellman_hvps_record->record_type_struct;

		if ( *spellman_hvps == (MX_SPELLMAN_HVPS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SPELLMAN_HVPS pointer for Spellman HVPS  '%s' "
			"used by digital input record '%s'.",
				spellman_hvps_record->name,
				dinput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_spellman_hvps_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_SPELLMAN_HVPS_DOUTPUT **spellman_hvps_doutput,
			MX_SPELLMAN_HVPS **spellman_hvps,
			const char *calling_fname )
{
	static const char fname[] = "mxd_spellman_hvps_dout_get_pointers()";

	MX_RECORD *spellman_hvps_record;
	MX_SPELLMAN_HVPS_DOUTPUT *spellman_hvps_doutput_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	spellman_hvps_doutput_ptr = (MX_SPELLMAN_HVPS_DOUTPUT *)
					doutput->record->record_type_struct;

	if ( spellman_hvps_doutput_ptr == (MX_SPELLMAN_HVPS_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_SPELLMAN_HVPS_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( spellman_hvps_doutput != (MX_SPELLMAN_HVPS_DOUTPUT **) NULL ) {
		*spellman_hvps_doutput = spellman_hvps_doutput_ptr;
	}

	if ( spellman_hvps != (MX_SPELLMAN_HVPS **) NULL ) {
		spellman_hvps_record =
			spellman_hvps_doutput_ptr->spellman_hvps_record;

		if ( spellman_hvps_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SPELLMAN_HVPS pointer for SPELLMAN_HVPS digital "
			"output record '%s' passed by '%s' is NULL.",
				doutput->record->name, calling_fname );
		}

		*spellman_hvps = (MX_SPELLMAN_HVPS *)
			spellman_hvps_record->record_type_struct;

		if ( *spellman_hvps == (MX_SPELLMAN_HVPS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SPELLMAN_HVPS pointer for Spellman HVPS  '%s' "
			"used by digital output record '%s'.",
				spellman_hvps_record->name,
				doutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_spellman_hvps_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_spellman_hvps_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_SPELLMAN_HVPS_DINPUT *spellman_hvps_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        spellman_hvps_dinput = (MX_SPELLMAN_HVPS_DINPUT *)
				malloc( sizeof(MX_SPELLMAN_HVPS_DINPUT) );

        if ( spellman_hvps_dinput == (MX_SPELLMAN_HVPS_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_SPELLMAN_HVPS_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = spellman_hvps_dinput;
        record->class_specific_function_list
			= &mxd_spellman_hvps_din_digital_input_function_list;

        digital_input->record = record;
	spellman_hvps_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spellman_hvps_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_spellman_hvps_din_read()";

	MX_SPELLMAN_HVPS_DINPUT *spellman_hvps_dinput;
	MX_SPELLMAN_HVPS *spellman_hvps;
	unsigned long offset, num_digital_monitors;
	mx_status_type mx_status;

	mx_status = mxd_spellman_hvps_din_get_pointers( dinput,
				&spellman_hvps_dinput, &spellman_hvps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a query command to the power supply. */

	mx_status = mxi_spellman_hvps_query_command( spellman_hvps );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Fetch the value from the digital monitor array. */

	offset = spellman_hvps_dinput->input_type;

	num_digital_monitors = sizeof(spellman_hvps->digital_monitor)
				/ sizeof(spellman_hvps->digital_monitor[0]);

	if ( offset >= num_digital_monitors ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested input type %lu for Spellman HVPS "
		"digital input '%s' is larger than the maximum value of %lu.",
			offset, dinput->record->name,
			num_digital_monitors - 1 );
	}

	dinput->value = spellman_hvps->digital_monitor[offset];

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_spellman_hvps_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_spellman_hvps_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_SPELLMAN_HVPS_DOUTPUT *spellman_hvps_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        spellman_hvps_doutput = (MX_SPELLMAN_HVPS_DOUTPUT *)
			malloc( sizeof(MX_SPELLMAN_HVPS_DOUTPUT) );

        if ( spellman_hvps_doutput == (MX_SPELLMAN_HVPS_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_SPELLMAN_HVPS_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = spellman_hvps_doutput;
        record->class_specific_function_list
			= &mxd_spellman_hvps_dout_digital_output_function_list;

        digital_output->record = record;
	spellman_hvps_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spellman_hvps_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spellman_hvps_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_spellman_hvps_dout_write()";

	MX_SPELLMAN_HVPS_DOUTPUT *spellman_hvps_doutput;
	MX_SPELLMAN_HVPS *spellman_hvps;
	unsigned long offset, num_digital_controls;
	mx_status_type mx_status;

	mx_status = mxd_spellman_hvps_dout_get_pointers( doutput,
				&spellman_hvps_doutput, &spellman_hvps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	offset = spellman_hvps_doutput->output_type;

	num_digital_controls = sizeof(spellman_hvps->digital_control)
				/ sizeof(spellman_hvps->digital_control[0]);

	if ( offset >= num_digital_controls ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested output type %lu for Spellman HVPS "
		"digital output '%s' is larger than the maximum value of %lu.",
			offset, doutput->record->name,
			num_digital_controls - 1 );
	}

	/* Only one of the three digital controls can be set in any
	 * given command.  In addition, the X-ray on and X-ray off
	 * commands must interact with each other.
	 */

	memset( spellman_hvps->digital_control, 0, 
		sizeof(spellman_hvps->digital_control) );

	switch( spellman_hvps_doutput->output_type ) {
	case MXF_SPELLMAN_HVPS_XRAY_ON:
	    if ( doutput->value ) {
		spellman_hvps->digital_control[MXF_SPELLMAN_HVPS_XRAY_ON] = 1;
	    } else {
		spellman_hvps->digital_control[MXF_SPELLMAN_HVPS_XRAY_OFF] = 1;
	    }
	    break;

	case MXF_SPELLMAN_HVPS_XRAY_OFF:
	    if ( doutput->value ) {
		spellman_hvps->digital_control[MXF_SPELLMAN_HVPS_XRAY_OFF] = 1;
	    } else {
		spellman_hvps->digital_control[MXF_SPELLMAN_HVPS_XRAY_ON] = 1;
	    }
	    break;

	case MXF_SPELLMAN_HVPS_POWER_SUPPLY_RESET:
	    spellman_hvps->digital_control[
			    	MXF_SPELLMAN_HVPS_POWER_SUPPLY_RESET] = 1;
		break;
	}

	/* Send the set command. */

	mx_status = mxi_spellman_hvps_set_command( spellman_hvps );

	return mx_status;
}

