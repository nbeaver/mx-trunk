/*
 * Name:    d_epix_xclib_dio.c
 *
 * Purpose: MX input and output drivers to control digital I/O ports
 *          on EPIX imaging boards.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPIX_XCLIB_DIO_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#if defined(OS_WIN32)
#  include <windows.h>
#endif

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"

#include "xcliball.h"	/* Vendor include file */

#include "i_epix_xclib.h"
#include "d_epix_xclib.h"
#include "d_epix_xclib_dio.h"

/* Initialize the EPIX_XCLIB digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_epix_xclib_dinput_record_function_list = {
	NULL,
	mxd_epix_xclib_dinput_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST
mxd_epix_xclib_dinput_digital_input_function_list = {
	mxd_epix_xclib_dinput_read,
	mxd_epix_xclib_dinput_clear
};

MX_RECORD_FIELD_DEFAULTS mxd_epix_xclib_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_EPIX_XCLIB_DIGITAL_INPUT_STANDARD_FIELDS
};

long mxd_epix_xclib_dinput_num_record_fields
	= sizeof( mxd_epix_xclib_dinput_record_field_defaults )
		/ sizeof( mxd_epix_xclib_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epix_xclib_dinput_rfield_def_ptr
			= &mxd_epix_xclib_dinput_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_epix_xclib_doutput_record_function_list = {
	NULL,
	mxd_epix_xclib_doutput_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
mxd_epix_xclib_doutput_digital_output_function_list = {
	mxd_epix_xclib_doutput_read,
	mxd_epix_xclib_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_epix_xclib_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_EPIX_XCLIB_DIGITAL_OUTPUT_STANDARD_FIELDS
};

long mxd_epix_xclib_doutput_num_record_fields
	= sizeof( mxd_epix_xclib_doutput_record_field_defaults )
		/ sizeof( mxd_epix_xclib_doutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epix_xclib_doutput_rfield_def_ptr
			= &mxd_epix_xclib_doutput_record_field_defaults[0];

static mx_status_type
mxd_epix_xclib_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_EPIX_XCLIB_DIGITAL_INPUT **epix_xclib_dinput,
			MX_EPIX_XCLIB_VIDEO_INPUT **epix_xclib_vinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epix_xclib_dinput_get_pointers()";

	MX_EPIX_XCLIB_DIGITAL_INPUT *epix_xclib_dinput_ptr;
	MX_RECORD *epix_xclib_vinput_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	epix_xclib_dinput_ptr = (MX_EPIX_XCLIB_DIGITAL_INPUT *)
					dinput->record->record_type_struct;

	if ( epix_xclib_dinput_ptr == (MX_EPIX_XCLIB_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_EPIX_XCLIB_DIGITAL_INPUT pointer for record '%s' "
		"passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( epix_xclib_dinput != (MX_EPIX_XCLIB_DIGITAL_INPUT **) NULL ) {
		*epix_xclib_dinput = epix_xclib_dinput_ptr;
	}

	if ( epix_xclib_vinput != (MX_EPIX_XCLIB_VIDEO_INPUT **) NULL ) {

		epix_xclib_vinput_record =
			epix_xclib_dinput_ptr->epix_xclib_vinput_record;

		if ( epix_xclib_vinput_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"epix_xclib_vinput_record pointer for EPIX_XCLIB "
			"digital input record '%s' passed by '%s' is NULL.",
				dinput->record->name, calling_fname );
		}

		*epix_xclib_vinput =
			epix_xclib_vinput_record->record_type_struct;

		if (*epix_xclib_vinput == (MX_EPIX_XCLIB_VIDEO_INPUT *) NULL) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_EPIX_XCLIB_VIDEO_INPUT pointer for record '%s' "
			"used by digital input record '%s' and passed "
			"by '%s' is NULL.",
				epix_xclib_vinput_record->name,
				dinput->record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_epix_xclib_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_EPIX_XCLIB_DIGITAL_OUTPUT **epix_xclib_doutput,
			MX_EPIX_XCLIB_VIDEO_INPUT **epix_xclib_vinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epix_xclib_doutput_get_pointers()";

	MX_EPIX_XCLIB_DIGITAL_OUTPUT *epix_xclib_doutput_ptr;
	MX_RECORD *epix_xclib_vinput_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	epix_xclib_doutput_ptr = (MX_EPIX_XCLIB_DIGITAL_OUTPUT *)
					doutput->record->record_type_struct;

	if ( epix_xclib_doutput_ptr == (MX_EPIX_XCLIB_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_EPIX_XCLIB_DIGITAL_OUTPUT pointer for record '%s' "
		"passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( epix_xclib_doutput != (MX_EPIX_XCLIB_DIGITAL_OUTPUT **) NULL ) {
		*epix_xclib_doutput = epix_xclib_doutput_ptr;
	}

	if ( epix_xclib_vinput != (MX_EPIX_XCLIB_VIDEO_INPUT **) NULL ) {

		epix_xclib_vinput_record =
			epix_xclib_doutput_ptr->epix_xclib_vinput_record;

		if ( epix_xclib_vinput_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"epix_xclib_vinput_record pointer for EPIX_XCLIB "
			"digital output record '%s' passed by '%s' is NULL.",
				doutput->record->name, calling_fname );
		}

		*epix_xclib_vinput =
			epix_xclib_vinput_record->record_type_struct;

		if (*epix_xclib_vinput == (MX_EPIX_XCLIB_VIDEO_INPUT *) NULL) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_EPIX_XCLIB_VIDEO_INPUT pointer for record '%s' "
			"used by digital output record '%s' and passed "
			"by '%s' is NULL.",
				epix_xclib_vinput_record->name,
				doutput->record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_epix_xclib_dinput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_epix_xclib_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_EPIX_XCLIB_DIGITAL_INPUT *epix_xclib_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_INPUT structure." );
        }

        epix_xclib_dinput = (MX_EPIX_XCLIB_DIGITAL_INPUT *)
				malloc( sizeof(MX_EPIX_XCLIB_DIGITAL_INPUT) );

        if ( epix_xclib_dinput == (MX_EPIX_XCLIB_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for MX_EPIX_XCLIB_DIGITAL_INPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = epix_xclib_dinput;
        record->class_specific_function_list
			= &mxd_epix_xclib_dinput_digital_input_function_list;

        digital_input->record = record;
	epix_xclib_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_epix_xclib_dinput_read()";

	MX_EPIX_XCLIB_DIGITAL_INPUT *epix_xclib_dinput;
	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	char error_message[80];
	int value;
	mx_status_type mx_status;

	/* Suppress GCC 4 uninitialized variable warning. */

	epix_xclib_vinput = NULL;

	mx_status = mxd_epix_xclib_dinput_get_pointers( dinput,
				&epix_xclib_dinput, &epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epix_xclib_dinput->trigger_number >= 0 ) {
		value = pxd_getGPTrigger( epix_xclib_vinput->unitmap,
					epix_xclib_dinput->trigger_number );
	} else {
		value = pxd_getGPIn( epix_xclib_vinput->unitmap, 0 );
	}

	if ( value < 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, value,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to read from digital input '%s' failed.  %s",
			dinput->record->name, error_message );
	}

	dinput->value = value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_dinput_clear( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_epix_xclib_dinput_clear()";

	MX_EPIX_XCLIB_DIGITAL_INPUT *epix_xclib_dinput;
	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	char error_message[80];
	int epix_status;
	mx_status_type mx_status;

	/* Suppress GCC 4 uninitialized variable warning. */

	epix_xclib_vinput = NULL;

	mx_status = mxd_epix_xclib_dinput_get_pointers( dinput,
				&epix_xclib_dinput, &epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epix_xclib_dinput->trigger_number >= 0 ) {
		/* You cannot clear a trigger count. */

		return MX_SUCCESSFUL_RESULT;
	}

	epix_status = pxd_setGPIn( epix_xclib_vinput->unitmap, 0 );

	if ( epix_status < 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, epix_status,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to clear digital input '%s' failed.  %s",
			dinput->record->name, error_message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_epix_xclib_doutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_epix_xclib_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_EPIX_XCLIB_DIGITAL_OUTPUT *epix_xclib_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        epix_xclib_doutput = (MX_EPIX_XCLIB_DIGITAL_OUTPUT *)
			malloc( sizeof(MX_EPIX_XCLIB_DIGITAL_OUTPUT) );

        if ( epix_xclib_doutput == (MX_EPIX_XCLIB_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for MX_EPIX_XCLIB_DIGITAL_OUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = epix_xclib_doutput;
        record->class_specific_function_list
			= &mxd_epix_xclib_doutput_digital_output_function_list;

        digital_output->record = record;
	epix_xclib_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_epix_xclib_doutput_read()";

	MX_EPIX_XCLIB_DIGITAL_OUTPUT *epix_xclib_doutput;
	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	char error_message[80];
	int value;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_doutput_get_pointers( doutput,
			&epix_xclib_doutput, &epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value = pxd_getGPOut( epix_xclib_vinput->unitmap, 0 );

	if ( value < 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, value,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to read from digital output '%s' failed.  %s",
			doutput->record->name, error_message );
	}

	doutput->value = value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epix_xclib_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_epix_xclib_doutput_write()";

	MX_EPIX_XCLIB_DIGITAL_OUTPUT *epix_xclib_doutput;
	MX_EPIX_XCLIB_VIDEO_INPUT *epix_xclib_vinput;
	char error_message[80];
	int epix_status;
	mx_status_type mx_status;

	mx_status = mxd_epix_xclib_doutput_get_pointers( doutput,
			&epix_xclib_doutput, &epix_xclib_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epix_status = pxd_setGPOut(epix_xclib_vinput->unitmap, doutput->value);

	if ( epix_status < 0 ) {
		mxi_epix_xclib_error_message(
			epix_xclib_vinput->unitmap, epix_status,
			error_message, sizeof(error_message) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to write to digital output '%s' failed.  %s",
			doutput->record->name, error_message );
	}

	return mx_status;
}

