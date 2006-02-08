/*
 * Name:    d_scipe_aio.c
 *
 * Purpose: MX input and output drivers to control SCIPE actuators as if they
 *          were analog output registers and SCIPE detectors as if they 
 *          were analog input registers.
 *
 * Author:  William Lavender and Steven Weigand
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SCIPE_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "i_scipe.h"
#include "d_scipe_aio.h"

/* Initialize the SCIPE analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_scipe_ain_record_function_list = {
	NULL,
	mxd_scipe_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_scipe_ain_analog_input_function_list = {
	mxd_scipe_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_scipe_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_SCIPE_AINPUT_STANDARD_FIELDS
};

mx_length_type mxd_scipe_ain_num_record_fields
		= sizeof( mxd_scipe_ain_record_field_defaults )
			/ sizeof( mxd_scipe_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_scipe_ain_rfield_def_ptr
			= &mxd_scipe_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_scipe_aout_record_function_list = {
	NULL,
	mxd_scipe_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_scipe_aout_analog_output_function_list = {
	mxd_scipe_aout_read,
	mxd_scipe_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_scipe_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_SCIPE_AOUTPUT_STANDARD_FIELDS
};

mx_length_type mxd_scipe_aout_num_record_fields
		= sizeof( mxd_scipe_aout_record_field_defaults )
			/ sizeof( mxd_scipe_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_scipe_aout_rfield_def_ptr
			= &mxd_scipe_aout_record_field_defaults[0];

static mx_status_type
mxd_scipe_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_SCIPE_AINPUT **scipe_ainput,
			MX_SCIPE_SERVER **scipe_server,
			const char *calling_fname )
{
	const char fname[] = "mxd_scipe_ain_get_pointers()";

	MX_RECORD *scipe_server_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (scipe_ainput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCIPE_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (scipe_server == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCIPE_SERVER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*scipe_ainput = (MX_SCIPE_AINPUT *) ainput->record->record_type_struct;

	if ( *scipe_ainput == (MX_SCIPE_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SCIPE_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	scipe_server_record = (*scipe_ainput)->scipe_server_record;

	if ( scipe_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_SCIPE pointer for SCIPE analog input record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( scipe_server_record->mx_type != MXI_GEN_SCIPE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"scipe_server_record '%s' for SCIPE analog input '%s' is not a SCIPE record.  "
"Instead, it is a '%s' record.",
			scipe_server_record->name, ainput->record->name,
			mx_get_driver_name( scipe_server_record ) );
	}

	*scipe_server = (MX_SCIPE_SERVER *)
				scipe_server_record->record_type_struct;

	if ( *scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SCIPE_SERVER pointer for SCIPE record '%s' used by "
	"SCIPE analog input record '%s' and passed by '%s' is NULL.",
			scipe_server_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_scipe_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_SCIPE_AOUTPUT **scipe_aoutput,
			MX_SCIPE_SERVER **scipe_server,
			const char *calling_fname )
{
	const char fname[] = "mxd_scipe_aout_get_pointers()";

	MX_RECORD *scipe_server_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (scipe_aoutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCIPE_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (scipe_server == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCIPE_SERVER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*scipe_aoutput = (MX_SCIPE_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *scipe_aoutput == (MX_SCIPE_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SCIPE_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	scipe_server_record = (*scipe_aoutput)->scipe_server_record;

	if ( scipe_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_SCIPE pointer for SCIPE analog output record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( scipe_server_record->mx_type != MXI_GEN_SCIPE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"scipe_server_record '%s' for SCIPE analog output '%s' is not a SCIPE record."
"  Instead, it is a '%s' record.",
			scipe_server_record->name, aoutput->record->name,
			mx_get_driver_name( scipe_server_record ) );
	}

	*scipe_server = (MX_SCIPE_SERVER *)
				scipe_server_record->record_type_struct;

	if ( *scipe_server == (MX_SCIPE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SCIPE_SERVER pointer for SCIPE record '%s' used by "
	"SCIPE analog output record '%s' and passed by '%s' is NULL.",
			scipe_server_record->name,
			aoutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_scipe_ain_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_scipe_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_SCIPE_AINPUT *scipe_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        scipe_ainput = (MX_SCIPE_AINPUT *) malloc( sizeof(MX_SCIPE_AINPUT) );

        if ( scipe_ainput == (MX_SCIPE_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SCIPE_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = scipe_ainput;
	record->record_function_list = &mxd_scipe_ain_record_function_list;
        record->class_specific_function_list
                                = &mxd_scipe_ain_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_ain_read( MX_ANALOG_INPUT *ainput )
{
	const char fname[] = "mxd_scipe_ain_read()";

	MX_SCIPE_AINPUT *scipe_ainput;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int num_items, scipe_response_code;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	scipe_server = NULL;
	scipe_ainput = NULL;

	mx_status = mxd_scipe_ain_get_pointers( ainput,
					&scipe_ainput, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s read", scipe_ainput->scipe_scaler_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof( response ),
				&scipe_response_code, &result_ptr,
				MXD_SCIPE_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for analog input '%s'.  "
	"Response = '%s'",
			command, ainput->record->name, response );
	}

	num_items = sscanf( result_ptr, "%lg",
				&(ainput->raw_value.double_value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"SCIPE actuator value not found in response '%s' for "
		"SCIPE analog input record '%s'.",
			response, ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_scipe_aout_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_scipe_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_SCIPE_AOUTPUT *scipe_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *)
					malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        scipe_aoutput = (MX_SCIPE_AOUTPUT *) malloc( sizeof(MX_SCIPE_AOUTPUT) );

        if ( scipe_aoutput == (MX_SCIPE_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SCIPE_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = scipe_aoutput;
	record->record_function_list = &mxd_scipe_aout_record_function_list;
        record->class_specific_function_list
                                = &mxd_scipe_aout_analog_output_function_list;

        analog_output->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	const char fname[] = "mxd_scipe_aout_read()";

	MX_SCIPE_AOUTPUT *scipe_aoutput;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int num_items, scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_aout_get_pointers( aoutput,
					&scipe_aoutput, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s position", scipe_aoutput->scipe_actuator_name );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof( response ),
				&scipe_response_code, &result_ptr,
				MXD_SCIPE_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK_WITH_RESULT ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for analog output '%s'.  "
	"Response = '%s'",
			command, aoutput->record->name, response );
	}

	num_items = sscanf( result_ptr, "%lg",
				&(aoutput->raw_value.double_value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"SCIPE actuator value not found in response '%s' for "
		"SCIPE analog output record '%s'.",
			response, aoutput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_scipe_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	const char fname[] = "mxd_scipe_aout_write()";

	MX_SCIPE_AOUTPUT *scipe_aoutput;
	MX_SCIPE_SERVER *scipe_server;
	char command[80];
	char response[80];
	char *result_ptr;
	int scipe_response_code;
	mx_status_type mx_status;

	mx_status = mxd_scipe_aout_get_pointers( aoutput,
					&scipe_aoutput, &scipe_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s movenow %g",
			scipe_aoutput->scipe_actuator_name,
			aoutput->value );

	mx_status = mxi_scipe_command( scipe_server, command,
				response, sizeof( response ),
				&scipe_response_code, &result_ptr,
				MXD_SCIPE_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scipe_response_code != MXF_SCIPE_OK ) {
		return mx_error(
			mxi_scipe_get_mx_status( scipe_response_code ), fname,
	"Unexpected response to '%s' command for analog output '%s'.  "
	"Response = '%s'",
			command, aoutput->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

