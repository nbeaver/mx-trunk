/*
 * Name:    d_epics_dio.c
 *
 * Purpose: MX input and output drivers to control EPICS variables as if they
 *          were digital input/output registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2006, 2008-2011, 2014-2015, 2018
 *    Illinois Institute of Technology
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
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_epics_dio.h"

/* Initialize the EPICS digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_epics_din_record_function_list = {
	NULL,
	mxd_epics_din_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_epics_din_open
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_epics_din_digital_input_function_list = {
	mxd_epics_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_epics_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_EPICS_DINPUT_STANDARD_FIELDS
};

long mxd_epics_din_num_record_fields
		= sizeof( mxd_epics_din_record_field_defaults )
			/ sizeof( mxd_epics_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_din_rfield_def_ptr
			= &mxd_epics_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_epics_dout_record_function_list = {
	NULL,
	mxd_epics_dout_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_epics_dout_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_epics_dout_digital_output_function_list = {
	mxd_epics_dout_read,
	mxd_epics_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_epics_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_EPICS_DOUTPUT_STANDARD_FIELDS
};

long mxd_epics_dout_num_record_fields
		= sizeof( mxd_epics_dout_record_field_defaults )
			/ sizeof( mxd_epics_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_dout_rfield_def_ptr
			= &mxd_epics_dout_record_field_defaults[0];

static mx_status_type
mxd_epics_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_EPICS_DINPUT **epics_dinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_din_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (epics_dinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_EPICS_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*epics_dinput = (MX_EPICS_DINPUT *) dinput->record->record_type_struct;

	if ( *epics_dinput == (MX_EPICS_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_EPICS_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_epics_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_EPICS_DOUTPUT **epics_doutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_dout_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (epics_doutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_EPICS_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*epics_doutput = (MX_EPICS_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *epics_doutput == (MX_EPICS_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_EPICS_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_epics_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_epics_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_EPICS_DINPUT *epics_dinput = NULL;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        epics_dinput = (MX_EPICS_DINPUT *) malloc( sizeof(MX_EPICS_DINPUT) );

        if ( epics_dinput == (MX_EPICS_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_EPICS_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = epics_dinput;
        record->class_specific_function_list
                                = &mxd_epics_din_digital_input_function_list;

        digital_input->record = record;
	epics_dinput->record = record;

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_din_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_din_open()";

	MX_DIGITAL_INPUT *dinput;
	MX_EPICS_DINPUT *epics_dinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_epics_din_get_pointers( dinput, &epics_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_epics_pvname_init( &(epics_dinput->epics_pv), "%s",
				epics_dinput->epics_variable_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_epics_din_read()";

	MX_EPICS_DINPUT *epics_dinput = NULL;
	int32_t int32_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_din_get_pointers( dinput, &epics_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_dinput->epics_pv),
				MX_CA_LONG, 1, &int32_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dinput->value = int32_value;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_epics_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_epics_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_EPICS_DOUTPUT *epics_doutput = NULL;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        epics_doutput = (MX_EPICS_DOUTPUT *) malloc( sizeof(MX_EPICS_DOUTPUT) );

        if ( epics_doutput == (MX_EPICS_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_EPICS_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = epics_doutput;
        record->class_specific_function_list
                                = &mxd_epics_dout_digital_output_function_list;

        digital_output->record = record;
	epics_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_dout_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_dout_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_EPICS_DOUTPUT *epics_doutput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_epics_dout_get_pointers(doutput, &epics_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_epics_pvname_init( &(epics_doutput->epics_pv), "%s",
				epics_doutput->epics_variable_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_epics_dout_read()";

	MX_EPICS_DOUTPUT *epics_doutput = NULL;
	int32_t int32_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_dout_get_pointers(doutput, &epics_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_doutput->epics_pv),
				MX_CA_LONG, 1, &int32_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	doutput->value = int32_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_epics_dout_write()";

	MX_EPICS_DOUTPUT *epics_doutput = NULL;
	int32_t int32_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_dout_get_pointers(doutput, &epics_doutput, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	int32_value = doutput->value;

	mx_status = mx_caput( &(epics_doutput->epics_pv),
				MX_CA_LONG, 1, &int32_value );

	return mx_status;
}

