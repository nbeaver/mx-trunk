/*
 * Name:    d_tracker_dio.c
 *
 * Purpose: MX drivers to control Data Track Tracker locations as
 *          digital I/O ports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_TRACKER_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_tracker_aio.h"
#include "d_tracker_dio.h"

/* Initialize the Tracker digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_tracker_din_record_function_list = {
	NULL,
	mxd_tracker_din_create_record_structures,
	mxd_tracker_din_finish_record_initialization
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_tracker_din_digital_input_function_list = {
	mxd_tracker_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_tracker_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_TRACKER_DINPUT_STANDARD_FIELDS
};

long mxd_tracker_din_num_record_fields
		= sizeof( mxd_tracker_din_record_field_defaults )
			/ sizeof( mxd_tracker_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_tracker_din_rfield_def_ptr
			= &mxd_tracker_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_tracker_dout_record_function_list = {
	NULL,
	mxd_tracker_dout_create_record_structures,
	mxd_tracker_dout_finish_record_initialization
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_tracker_dout_digital_output_function_list = {
	mxd_tracker_dout_read,
	mxd_tracker_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_tracker_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_TRACKER_DOUTPUT_STANDARD_FIELDS
};

long mxd_tracker_dout_num_record_fields
		= sizeof( mxd_tracker_dout_record_field_defaults )
			/ sizeof( mxd_tracker_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_tracker_dout_rfield_def_ptr
			= &mxd_tracker_dout_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_tracker_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_TRACKER_DINPUT **tracker_dinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_tracker_din_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( tracker_dinput == (MX_TRACKER_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_TRACKER_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*tracker_dinput = (MX_TRACKER_DINPUT *)
				dinput->record->record_type_struct;

	if ( *tracker_dinput == (MX_TRACKER_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_TRACKER_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_tracker_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_TRACKER_DOUTPUT **tracker_doutput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_tracker_dout_get_pointers()";

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( tracker_doutput == (MX_TRACKER_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_TRACKER_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*tracker_doutput = (MX_TRACKER_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *tracker_doutput == (MX_TRACKER_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_TRACKER_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_tracker_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_tracker_din_create_record_structures()";

        MX_DIGITAL_INPUT *dinput;
        MX_TRACKER_DINPUT *tracker_dinput;

        /* Allocate memory for the necessary structures. */

        dinput = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        tracker_dinput = (MX_TRACKER_DINPUT *)
				malloc( sizeof(MX_TRACKER_DINPUT) );

        if ( tracker_dinput == (MX_TRACKER_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_TRACKER_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = dinput;
        record->record_type_struct = tracker_dinput;
        record->class_specific_function_list
                                = &mxd_tracker_din_digital_input_function_list;

        dinput->record = record;
	tracker_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tracker_din_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_tracker_din_finish_record_initialization()";

	MX_DIGITAL_INPUT *dinput;
	MX_TRACKER_DINPUT *tracker_dinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_tracker_din_get_pointers( dinput,
						&tracker_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_verify_configuration(tracker_dinput->rs232_record,
				MXF_232_DONT_CARE, MXF_232_DONT_CARE,
				(char) MXF_232_DONT_CARE, MXF_232_DONT_CARE,
				(char) MXF_232_DONT_CARE, 0x0d0a, 0x0d0a );
        return mx_status;
}

MX_EXPORT mx_status_type
mxd_tracker_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_tracker_din_read()";

	MX_TRACKER_DINPUT *tracker_dinput;
	char command[80], response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_tracker_din_get_pointers( dinput,
						&tracker_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		";%03ld RA %ld 1", tracker_dinput->address,
				tracker_dinput->location );

	mx_status = mxd_tracker_command( dinput->record,
					tracker_dinput->rs232_record,
					command,
					response, sizeof(response),
					MXD_TRACKER_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &(dinput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"A numerical value was not found in the response "
		"to command '%s' for digital input '%s'.  Response = '%s'",
			command, dinput->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_tracker_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_tracker_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *doutput;
        MX_TRACKER_DOUTPUT *tracker_doutput;

        /* Allocate memory for the necessary structures. */

        doutput = (MX_DIGITAL_OUTPUT *) malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        tracker_doutput = (MX_TRACKER_DOUTPUT *)
				malloc( sizeof(MX_TRACKER_DOUTPUT) );

        if ( tracker_doutput == (MX_TRACKER_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_TRACKER_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = doutput;
        record->record_type_struct = tracker_doutput;
        record->class_specific_function_list
        		= &mxd_tracker_dout_digital_output_function_list;

        doutput->record = record;
	tracker_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tracker_dout_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_tracker_dout_finish_record_initialization()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_TRACKER_DOUTPUT *tracker_doutput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_tracker_dout_get_pointers( doutput,
						&tracker_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_verify_configuration(tracker_doutput->rs232_record,
				MXF_232_DONT_CARE, MXF_232_DONT_CARE,
				(char) MXF_232_DONT_CARE, MXF_232_DONT_CARE,
				(char) MXF_232_DONT_CARE, 0x0d0a, 0x0d0a );
        return mx_status;
}

MX_EXPORT mx_status_type
mxd_tracker_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_tracker_dout_read()";

	MX_TRACKER_DOUTPUT *tracker_doutput;
	char command[80], response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_tracker_dout_get_pointers( doutput,
						&tracker_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		";%03ld RA %ld 1", tracker_doutput->address,
				tracker_doutput->location );

	mx_status = mxd_tracker_command( doutput->record,
					tracker_doutput->rs232_record,
					command,
					response, sizeof(response),
					MXD_TRACKER_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &(doutput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"A numerical value was not found out the response "
		"to command '%s' for digital output '%s'.  Response = '%s'",
			command, doutput->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tracker_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_tracker_dout_write()";

	MX_TRACKER_DOUTPUT *tracker_doutput;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_tracker_dout_get_pointers( doutput,
						&tracker_doutput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		";%03ld SA %ld %lu", tracker_doutput->address,
				tracker_doutput->location,
				doutput->value );

	mx_status = mxd_tracker_command( doutput->record,
					tracker_doutput->rs232_record,
					command,
					NULL, 0,
					MXD_TRACKER_DIO_DEBUG );
	return mx_status;
}

