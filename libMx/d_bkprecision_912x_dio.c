/*
 * Name:    d_bkprecision_912x_dio.c
 *
 * Purpose: MX digital I/O drivers for the BK Precision 912x series of
 *          power supplies.
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

#define MXD_BKPRECISION_912X_DIO_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_bkprecision_912x.h"
#include "d_bkprecision_912x_dio.h"

/* Initialize the digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_din_record_function_list = {
	NULL,
	mxd_bkprecision_912x_din_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_bkprecision_912x_din_digital_input_function_list =
{
	mxd_bkprecision_912x_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_bkprecision_912x_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_BKPRECISION_912X_DINPUT_STANDARD_FIELDS
};

long mxd_bkprecision_912x_din_num_record_fields
		= sizeof( mxd_bkprecision_912x_din_record_field_defaults )
		    / sizeof( mxd_bkprecision_912x_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_din_rfield_def_ptr
			= &mxd_bkprecision_912x_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_dout_record_function_list = {
	NULL,
	mxd_bkprecision_912x_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_bkprecision_912x_dout_digital_output_function_list =
{
	mxd_bkprecision_912x_dout_read,
	mxd_bkprecision_912x_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_bkprecision_912x_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_BKPRECISION_912X_DOUTPUT_STANDARD_FIELDS
};

long mxd_bkprecision_912x_dout_num_record_fields
		= sizeof( mxd_bkprecision_912x_dout_record_field_defaults )
		    / sizeof( mxd_bkprecision_912x_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_dout_rfield_def_ptr
			= &mxd_bkprecision_912x_dout_record_field_defaults[0];

static mx_status_type
mxd_bkprecision_912x_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_BKPRECISION_912X_DINPUT **bkprecision_912x_dinput,
			MX_BKPRECISION_912X **bkprecision_912x,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bkprecision_912x_din_get_pointers()";

	MX_RECORD *bkprecision_912x_record;
	MX_BKPRECISION_912X_DINPUT *bkprecision_912x_dinput_ptr;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bkprecision_912x_dinput_ptr = (MX_BKPRECISION_912X_DINPUT *)
					dinput->record->record_type_struct;

	if (bkprecision_912x_dinput_ptr == (MX_BKPRECISION_912X_DINPUT *) NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_BKPRECISION_912X_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( bkprecision_912x_dinput != (MX_BKPRECISION_912X_DINPUT **) NULL ) {
		*bkprecision_912x_dinput = bkprecision_912x_dinput_ptr;
	}

	if ( bkprecision_912x != (MX_BKPRECISION_912X **) NULL ) {
		bkprecision_912x_record =
			bkprecision_912x_dinput_ptr->bkprecision_912x_record;

		if ( bkprecision_912x_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_BKPRECISION_912X pointer for digital "
			"input record '%s' passed by '%s' is NULL.",
				dinput->record->name, calling_fname );
		}

		*bkprecision_912x = (MX_BKPRECISION_912X *)
			bkprecision_912x_record->record_type_struct;

		if ( *bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BKPRECISION_912X pointer for BK Precision '%s' "
			"used by digital input record '%s'.",
				bkprecision_912x_record->name,
				dinput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_bkprecision_912x_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_BKPRECISION_912X_DOUTPUT **bkprecision_912x_doutput,
			MX_BKPRECISION_912X **bkprecision_912x,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bkprecision_912x_dout_get_pointers()";

	MX_RECORD *bkprecision_912x_record;
	MX_BKPRECISION_912X_DOUTPUT *bkprecision_912x_doutput_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bkprecision_912x_doutput_ptr = (MX_BKPRECISION_912X_DOUTPUT *)
					doutput->record->record_type_struct;

	if (bkprecision_912x_doutput_ptr == (MX_BKPRECISION_912X_DOUTPUT *)NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_BKPRECISION_912X_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if (bkprecision_912x_doutput != (MX_BKPRECISION_912X_DOUTPUT **) NULL)
	{
		*bkprecision_912x_doutput = bkprecision_912x_doutput_ptr;
	}

	if ( bkprecision_912x != (MX_BKPRECISION_912X **) NULL ) {
		bkprecision_912x_record =
			bkprecision_912x_doutput_ptr->bkprecision_912x_record;

		if ( bkprecision_912x_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_BKPRECISION_912X pointer for digital "
			"output record '%s' passed by '%s' is NULL.",
				doutput->record->name, calling_fname );
		}

		*bkprecision_912x = (MX_BKPRECISION_912X *)
			bkprecision_912x_record->record_type_struct;

		if ( *bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BKPRECISION_912X pointer for BK Precision '%s' "
			"used by digital output record '%s'.",
				bkprecision_912x_record->name,
				doutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_bkprecision_912x_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_bkprecision_912x_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_BKPRECISION_912X_DINPUT *bkprecision_912x_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        bkprecision_912x_dinput = (MX_BKPRECISION_912X_DINPUT *)
				malloc( sizeof(MX_BKPRECISION_912X_DINPUT) );

        if ( bkprecision_912x_dinput == (MX_BKPRECISION_912X_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Cannot allocate memory for MX_BKPRECISION_912X_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = bkprecision_912x_dinput;
        record->class_specific_function_list
			= &mxd_bkprecision_912x_din_digital_input_function_list;

        digital_input->record = record;
	bkprecision_912x_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_bkprecision_912x_din_read()";

	MX_BKPRECISION_912X_DINPUT *bkprecision_912x_dinput;
	MX_BKPRECISION_912X *bkprecision_912x;
	int num_items;
	char response[40];
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_din_get_pointers( dinput,
			&bkprecision_912x_dinput, &bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bkprecision_912x->port_function != MXT_BKPRECISION_912X_DIGITAL )
	{
		mx_warning(
		"BK Precision power supply '%s' is not in 'DIGITAL' mode.",
			bkprecision_912x->record->name );

		dinput->value = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
						"DIGITAL:INPUT?",
						response, sizeof(response),
						MXD_BKPRECISION_912X_DIO_DEBUG);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &(dinput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"A numerical value was not found in the response '%s' "
		"from digital input '%s'.",
			response, dinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_bkprecision_912x_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_bkprecision_912x_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_BKPRECISION_912X_DOUTPUT *bkprecision_912x_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        bkprecision_912x_doutput = (MX_BKPRECISION_912X_DOUTPUT *)
			malloc( sizeof(MX_BKPRECISION_912X_DOUTPUT) );

        if ( bkprecision_912x_doutput == (MX_BKPRECISION_912X_DOUTPUT *) NULL )
	{
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Cannot allocate memory for MX_BKPRECISION_912X_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = bkprecision_912x_doutput;
        record->class_specific_function_list
		= &mxd_bkprecision_912x_dout_digital_output_function_list;

        digital_output->record = record;
	bkprecision_912x_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_bkprecision_912x_dout_write()";

	MX_BKPRECISION_912X_DOUTPUT *bkprecision_912x_doutput;
	MX_BKPRECISION_912X *bkprecision_912x;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_dout_get_pointers( doutput,
			&bkprecision_912x_doutput, &bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bkprecision_912x->port_function != MXT_BKPRECISION_912X_DIGITAL )
	{
		mx_warning(
		"BK Precision power supply '%s' is not in 'DIGITAL' mode.",
			bkprecision_912x->record->name );

		doutput->value = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( doutput->value ) {
		strlcpy( command, "DIGITAL:OUTPUT 1", sizeof(command) );

		doutput->value = 1;
	} else {
		strlcpy( command, "DIGITAL:OUTPUT 0", sizeof(command) );
	}

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, NULL, 0,
					MXD_BKPRECISION_912X_DIO_DEBUG );

	return mx_status;
}

