/*
 * Name:    d_epics_aio.c
 *
 * Purpose: MX input and output drivers to control EPICS process variables
 *          as if they were analog inputs and outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006, 2008-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_epics.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "d_epics_aio.h"

/* Initialize the EPICS analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_epics_ain_record_function_list = {
	NULL,
	mxd_epics_ain_create_record_structures,
	mx_analog_input_finish_record_initialization,
	NULL,
	NULL,
	mxd_epics_ain_open
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_epics_ain_analog_input_function_list = {
	mxd_epics_ain_read,
	mxd_epics_ain_get_dark_current,
	mxd_epics_ain_set_dark_current
};

MX_RECORD_FIELD_DEFAULTS mxd_epics_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_EPICS_AINPUT_STANDARD_FIELDS
};

long mxd_epics_ain_num_record_fields
		= sizeof( mxd_epics_ain_record_field_defaults )
			/ sizeof( mxd_epics_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_ain_rfield_def_ptr
			= &mxd_epics_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_epics_aout_record_function_list = {
	NULL,
	mxd_epics_aout_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_epics_aout_open
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_epics_aout_analog_output_function_list = {
	mxd_epics_aout_read,
	mxd_epics_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_epics_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_EPICS_AOUTPUT_STANDARD_FIELDS
};

long mxd_epics_aout_num_record_fields
		= sizeof( mxd_epics_aout_record_field_defaults )
			/ sizeof( mxd_epics_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_aout_rfield_def_ptr
			= &mxd_epics_aout_record_field_defaults[0];

static mx_status_type
mxd_epics_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_EPICS_AINPUT **epics_ainput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_ain_get_pointers()";

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (epics_ainput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_EPICS_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*epics_ainput = (MX_EPICS_AINPUT *) ainput->record->record_type_struct;

	if ( *epics_ainput == (MX_EPICS_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_EPICS_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_epics_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_EPICS_AOUTPUT **epics_aoutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_aout_get_pointers()";

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (epics_aoutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_EPICS_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*epics_aoutput = (MX_EPICS_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *epics_aoutput == (MX_EPICS_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_EPICS_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_epics_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_epics_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_EPICS_AINPUT *epics_ainput = NULL;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        epics_ainput = (MX_EPICS_AINPUT *) malloc( sizeof(MX_EPICS_AINPUT) );

        if ( epics_ainput == (MX_EPICS_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_EPICS_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = epics_ainput;
        record->class_specific_function_list
                                = &mxd_epics_ain_analog_input_function_list;

        analog_input->record = record;
	epics_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ain_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_ain_open()";

	MX_ANALOG_INPUT *ainput;
	MX_EPICS_AINPUT *epics_ainput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_epics_ain_get_pointers( ainput, &epics_ainput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_epics_pvname_init( &(epics_ainput->epics_pv),
				epics_ainput->epics_variable_name );

	mx_epics_pvname_init( &(epics_ainput->dark_current_pv),
				epics_ainput->dark_current_variable_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_epics_ain_read()";

	MX_EPICS_AINPUT *epics_ainput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_epics_ain_get_pointers( ainput, &epics_ainput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_ainput->epics_pv),
			MX_CA_DOUBLE, 1, &(ainput->raw_value.double_value));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_ain_get_dark_current( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_epics_ain_get_dark_current()";

	MX_EPICS_AINPUT *epics_ainput = NULL;
	mx_status_type mx_status;

        /* If the database is configured such that the server maintains
         * the dark current rather than the client, then ask the server
         * for the value.  Otherwise, just return our locally cached value.
         */

	if (ainput->analog_input_flags & MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT)
	{
		mx_status = mxd_epics_ain_get_pointers( ainput,
							&epics_ainput, fname );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_ainput->dark_current_pv),
			MX_CA_DOUBLE, 1, &(ainput->dark_current));

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_ain_set_dark_current( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_epics_ain_set_dark_current()";

	MX_EPICS_AINPUT *epics_ainput = NULL;
	mx_status_type mx_status;

        /* If the database is configured such that the server maintains
         * the dark current rather than the client, then ask the server
         * for the value.  Otherwise, just return our locally cached value.
         */

	if (ainput->analog_input_flags & MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT)
	{
		mx_status = mxd_epics_ain_get_pointers( ainput,
							&epics_ainput, fname );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caput( &(epics_ainput->dark_current_pv),
			MX_CA_DOUBLE, 1, &(ainput->dark_current));

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_epics_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_epics_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_EPICS_AOUTPUT *epics_aoutput = NULL;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *)
					malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        epics_aoutput = (MX_EPICS_AOUTPUT *) malloc( sizeof(MX_EPICS_AOUTPUT) );

        if ( epics_aoutput == (MX_EPICS_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_EPICS_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = epics_aoutput;
        record->class_specific_function_list
                                = &mxd_epics_aout_analog_output_function_list;

        analog_output->record = record;
	epics_aoutput->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_aout_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_aout_open()";

	MX_ANALOG_OUTPUT *aoutput;
	MX_EPICS_AOUTPUT *epics_aoutput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	aoutput = (MX_ANALOG_OUTPUT *) record->record_class_struct;

	mx_status = mxd_epics_aout_get_pointers( aoutput,
						&epics_aoutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_epics_pvname_init( &(epics_aoutput->epics_pv),
				epics_aoutput->epics_variable_name );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_epics_aout_read()";

	MX_EPICS_AOUTPUT *epics_aoutput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_epics_aout_get_pointers( aoutput,
						&epics_aoutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_aoutput->epics_pv),
			MX_CA_DOUBLE, 1, &(aoutput->raw_value.double_value));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_epics_aout_write()";

	MX_EPICS_AOUTPUT *epics_aoutput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_epics_aout_get_pointers( aoutput,
						&epics_aoutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caput( &(epics_aoutput->epics_pv),
			MX_CA_DOUBLE, 1, &(aoutput->raw_value.double_value));

	return mx_status;
}

