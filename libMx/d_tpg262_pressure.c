/*
 * Name:    d_tpg262_pressure.c
 *
 * Purpose: MX analog input driver that reads vacuum pressure from a
 *          Pfeiffer TPG 261 or TPG 262 vacuum gauge controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_TPG262_PRESSURE_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_tpg262.h"
#include "d_tpg262_pressure.h"

/* Initialize the TPG262 pressure analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_tpg262_pressure_record_function_list = {
	NULL,
	mxd_tpg262_pressure_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_tpg262_pressure_analog_input_function_list = {
	mxd_tpg262_pressure_read
};

MX_RECORD_FIELD_DEFAULTS mxd_tpg262_pressure_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_TPG262_PRESSURE_STANDARD_FIELDS
};

long mxd_tpg262_pressure_num_record_fields
		= sizeof( mxd_tpg262_pressure_record_field_defaults )
			/ sizeof( mxd_tpg262_pressure_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_tpg262_pressure_rfield_def_ptr
			= &mxd_tpg262_pressure_record_field_defaults[0];

static mx_status_type
mxd_tpg262_pressure_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_TPG262_PRESSURE **tpg262_pressure,
			MX_TPG262 **tpg262,
			const char *calling_fname )
{
	static const char fname[] = "mxd_tpg262_pressure_get_pointers()";

	MX_RECORD *tpg262_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( tpg262_pressure == (MX_TPG262_PRESSURE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_TPG262_PRESSURE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( tpg262 == (MX_TPG262 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_TPG262 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*tpg262_pressure = (MX_TPG262_PRESSURE *)
				ainput->record->record_type_struct;

	if ( *tpg262_pressure == (MX_TPG262_PRESSURE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_TPG262_PRESSURE pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	tpg262_record = (*tpg262_pressure)->tpg262_record;

	if ( tpg262_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TPG262 pointer for TPG262 analog input "
		"record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( tpg262_record->mx_type != MXI_GEN_TPG262 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"tpg262_record '%s' for TPG262 analog input '%s' "
		"is not a TPG262 record.  Instead, it is a '%s' record.",
			tpg262_record->name, ainput->record->name,
			mx_get_driver_name( tpg262_record ) );
	}

	*tpg262 = (MX_TPG262 *) tpg262_record->record_type_struct;

	if ( *tpg262 == (MX_TPG262 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_TPG262 pointer for TPG262 record '%s' used by "
	"TPG262 analog input record '%s' and passed by '%s' is NULL.",
			tpg262_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_tpg262_pressure_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_tpg262_pressure_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_TPG262_PRESSURE *tpg262_pressure;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        tpg262_pressure = (MX_TPG262_PRESSURE *)
				malloc( sizeof(MX_TPG262_PRESSURE) );

        if ( tpg262_pressure == (MX_TPG262_PRESSURE *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_TPG262_PRESSURE structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = tpg262_pressure;
        record->class_specific_function_list
			= &mxd_tpg262_pressure_analog_input_function_list;

        analog_input->record = record;
	tpg262_pressure->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tpg262_pressure_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_tpg262_pressure_read()";

	MX_TPG262_PRESSURE *tpg262_pressure;
	MX_TPG262 *tpg262;
	char command[20];
	char response[80];
	int num_items, measurement_status;
	double measurement_value;
	mx_status_type mx_status;

	mx_status = mxd_tpg262_pressure_get_pointers( ainput,
					&tpg262_pressure, &tpg262, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "PR%d", tpg262_pressure->gauge_number );

	mx_status = mxi_tpg262_command( tpg262, command,
					response, sizeof(response),
					MXD_TPG262_PRESSURE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d,%lg", &measurement_status,
						&measurement_value );

	if ( num_items != 2 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unable to parse the response '%s' to command '%s' "
		"for TPG 262 controller '%s'",
			response, command, tpg262->record->name );
	}

	MX_DEBUG(-2,("%s: measurement_status = %d, measurement_value = %g",
		fname, measurement_status, measurement_value));

	ainput->raw_value.double_value = measurement_value;

	return MX_SUCCESSFUL_RESULT;
}

