/*
 * Name:    d_scipe_dio.c
 *
 * Purpose: MX input and output drivers to control SCIPE actuators as if they
 *          were digital output registers and SCIPE detectors as if they 
 *          were digital input registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_scipe.h"
#include "d_scipe_dio.h"

#define MXD_SCIPE_DIO_DEBUG	FALSE

/* Initialize the SCIPE digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_scipe_din_record_function_list = {
	NULL,
	mxd_scipe_din_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_scipe_din_digital_input_function_list = {
	mxd_scipe_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_scipe_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_SCIPE_DINPUT_STANDARD_FIELDS
};

long mxd_scipe_din_num_record_fields
		= sizeof( mxd_scipe_din_record_field_defaults )
			/ sizeof( mxd_scipe_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_scipe_din_rfield_def_ptr
			= &mxd_scipe_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_scipe_dout_record_function_list = {
	NULL,
	mxd_scipe_dout_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_scipe_dout_digital_output_function_list = {
	mxd_scipe_dout_read,
	mxd_scipe_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_scipe_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_SCIPE_DOUTPUT_STANDARD_FIELDS
};

long mxd_scipe_dout_num_record_fields
		= sizeof( mxd_scipe_dout_record_field_defaults )
			/ sizeof( mxd_scipe_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_scipe_dout_rfield_def_ptr
			= &mxd_scipe_dout_record_field_defaults[0];

static mx_status_type
mxd_scipe_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_SCIPE_DINPUT **scipe_dinput,
			MX_SCIPE_SERVER **scipe_server,
			const char *calling_fname )
{
	const char fname[] = "mxd_scipe_din_get_pointers()";

	MX_RECORD *scipe_server_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (scipe_dinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCIPE_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (scipe_server == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCIPE_SERVER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*scipe_dinput = (MX_SCIPE_DINPUT *) dinput->record->record_type_struct;

	if ( *scipe_dinput == (MX_SCIPE_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SCIPE_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	scipe_server_record = (*scipe_dinput)->scipe_server_record;

	if ( scipe_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_SCIPE pointer for SCIPE digital input record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( scipe_server_record->mx_type != MXI_GEN_SCIPE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"scipe_server_record '%s' for SCIPE digital input '%s' is not a SCIPE record.  "
"Instead, it is a '%s' record.",
			scipe_server_record->name, dinput->record->name,
			mx_get_driver_name( scipe_server_record ) );
	}

	*scipe_server = (MX_SCIPE_SERVER *)
				scipe_server_record->record_type_struct;

	if ( *scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SCIPE_SERVER pointer for SCIPE record '%s' used by "
	"SCIPE digital input record '%s' and passed by '%s' is NULL.",
			scipe_server_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_scipe_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_SCIPE_DOUTPUT **scipe_doutput,
			MX_SCIPE_SERVER **scipe_server,
			const char *calling_fname )
{
	const char fname[] = "mxd_scipe_dout_get_pointers()";

	MX_RECORD *scipe_server_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (scipe_doutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCIPE_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (scipe_server == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCIPE_SERVER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*scipe_doutput = (MX_SCIPE_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *scipe_doutput == (MX_SCIPE_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SCIPE_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	scipe_server_record = (*scipe_doutput)->scipe_server_record;

	if ( scipe_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_SCIPE pointer for SCIPE digital output record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( scipe_server_record->mx_type != MXI_GEN_SCIPE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"scipe_server_record '%s' for SCIPE digital output '%s' is not a SCIPE record."
"  Instead, it is a '%s' record.",
			scipe_server_record->name, doutput->record->name,
			mx_get_driver_name( scipe_server_record ) );
	}

	*scipe_server = (MX_SCIPE_SERVER *)
				scipe_server_record->record_type_struct;

	if ( *scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SCIPE_SERVER pointer for SCIPE record '%s' used by "
	"SCIPE digital output record '%s' and passed by '%s' is NULL.",
			scipe_server_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_scipe_din_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_scipe_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_SCIPE_DINPUT *scipe_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        scipe_dinput = (MX_SCIPE_DINPUT *) malloc( sizeof(MX_SCIPE_DINPUT) );

        if ( scipe_dinput == (MX_SCIPE_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SCIPE_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = scipe_dinput;
	record->record_function_list = &mxd_scipe_din_record_function_list;
        record->class_specific_function_list
                                = &mxd_scipe_din_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_din_read( MX_DIGITAL_INPUT *dinput )
{
	const char fname[] = "mxd_scipe_din_read()";

	MX_SCIPE_DINPUT *scipe_dinput;
	MX_SCIPE_SERVER *scipe_server;
	char command[40];
	char response[80];
	char *result_ptr;
	int num_items, scipe_response_code;
	double double_value;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	scipe_server = NULL;
	scipe_dinput = NULL;

	mx_status = mxd_scipe_din_get_pointers( dinput,
					&scipe_dinput, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s read", scipe_dinput->scipe_scaler_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof( response ),
				&scipe_response_code, &result_ptr,
				MXD_SCIPE_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for digital input '%s'.  "
	"Response = '%s'",
			command, dinput->record->name, response );
	}

	num_items = sscanf( result_ptr, "%lg", &double_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"SCIPE actuator value not found in response '%s' for "
		"SCIPE digital input record '%s'.",
			response, dinput->record->name );
	}

	dinput->value = mx_round( double_value );

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_scipe_dout_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_scipe_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_SCIPE_DOUTPUT *scipe_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        scipe_doutput = (MX_SCIPE_DOUTPUT *) malloc( sizeof(MX_SCIPE_DOUTPUT) );

        if ( scipe_doutput == (MX_SCIPE_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SCIPE_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = scipe_doutput;
	record->record_function_list = &mxd_scipe_dout_record_function_list;
        record->class_specific_function_list
                                = &mxd_scipe_dout_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_scipe_dout_read()";

	MX_SCIPE_DOUTPUT *scipe_doutput;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int num_items, scipe_response_code;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_scipe_dout_get_pointers( doutput,
					&scipe_doutput, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s position", scipe_doutput->scipe_actuator_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof( response ),
				&scipe_response_code, &result_ptr,
				MXD_SCIPE_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for digital output '%s'.  "
	"Response = '%s'",
			command, doutput->record->name, response );
	}

	num_items = sscanf( result_ptr, "%lg", &double_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"SCIPE actuator value not found in response '%s' for "
		"SCIPE digital output record '%s'.",
			response, doutput->record->name );
	}

	doutput->value = mx_round( double_value );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	const char fname[] = "mxd_scipe_dout_write()";

	MX_SCIPE_DOUTPUT *scipe_doutput;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_dout_get_pointers( doutput,
					&scipe_doutput, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s movenow %lu",
			scipe_doutput->scipe_actuator_name,
			doutput->value );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof( response ),
				&scipe_response_code, &result_ptr,
				MXD_SCIPE_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for digital output '%s'.  "
	"Response = '%s'",
			command, doutput->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

