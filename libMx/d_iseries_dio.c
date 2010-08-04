/*
 * Name:    d_iseries_dio.c
 *
 * Purpose: MX drivers to control iSeries functions as if they were
 *          digital I/O ports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ISERIES_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_iseries.h"
#include "d_iseries_dio.h"

/* Initialize the iSeries digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_iseries_din_record_function_list = {
	NULL,
	mxd_iseries_din_create_record_structures,
	mxd_iseries_din_finish_record_initialization
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_iseries_din_digital_input_function_list = {
	mxd_iseries_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_iseries_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_ISERIES_DINPUT_STANDARD_FIELDS
};

long mxd_iseries_din_num_record_fields
		= sizeof( mxd_iseries_din_record_field_defaults )
			/ sizeof( mxd_iseries_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_iseries_din_rfield_def_ptr
			= &mxd_iseries_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_iseries_dout_record_function_list = {
	NULL,
	mxd_iseries_dout_create_record_structures,
	mxd_iseries_dout_finish_record_initialization
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_iseries_dout_digital_output_function_list = {
	mxd_iseries_dout_read,
	mxd_iseries_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_iseries_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_ISERIES_DOUTPUT_STANDARD_FIELDS
};

long mxd_iseries_dout_num_record_fields
		= sizeof( mxd_iseries_dout_record_field_defaults )
			/ sizeof( mxd_iseries_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_iseries_dout_rfield_def_ptr
			= &mxd_iseries_dout_record_field_defaults[0];

static mx_status_type
mxd_iseries_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_ISERIES_DINPUT **iseries_dinput,
			MX_ISERIES **iseries,
			const char *calling_fname )
{
	static const char fname[] = "mxd_iseries_din_get_pointers()";

	MX_RECORD *iseries_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (iseries_dinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ISERIES_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (iseries == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ISERIES pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*iseries_dinput = (MX_ISERIES_DINPUT *)
			dinput->record->record_type_struct;

	if ( *iseries_dinput == (MX_ISERIES_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ISERIES_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	iseries_record = (*iseries_dinput)->iseries_record;

	if ( iseries_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ISERIES pointer for iSeries digital input "
		"record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( iseries_record->mx_type != MXI_CTRL_ISERIES ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"iseries_record '%s' for iSeries digital input '%s' is "
		"not an 'iseries' record.  Instead, it is a '%s' record.",
			iseries_record->name, dinput->record->name,
			mx_get_driver_name( iseries_record ) );
	}

	*iseries = (MX_ISERIES *) iseries_record->record_type_struct;

	if ( *iseries == (MX_ISERIES *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ISERIES pointer for 'iseries' record '%s' used by "
	"iSeries digital input record '%s' and passed by '%s' is NULL.",
			iseries_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_iseries_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_ISERIES_DOUTPUT **iseries_doutput,
			MX_ISERIES **iseries,
			const char *calling_fname )
{
	static const char fname[] = "mxd_iseries_dout_get_pointers()";

	MX_RECORD *iseries_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (iseries_doutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ISERIES_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (iseries == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ISERIES pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*iseries_doutput = (MX_ISERIES_DOUTPUT *)
			doutput->record->record_type_struct;

	if ( *iseries_doutput == (MX_ISERIES_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ISERIES_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	iseries_record = (*iseries_doutput)->iseries_record;

	if ( iseries_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ISERIES pointer for iSeries digital output "
		"record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( iseries_record->mx_type != MXI_CTRL_ISERIES ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"iseries_record '%s' for iSeries digital output '%s' is "
		"not an 'iseries' record.  Instead, it is a '%s' record.",
			iseries_record->name, doutput->record->name,
			mx_get_driver_name( iseries_record ) );
	}

	*iseries = (MX_ISERIES *) iseries_record->record_type_struct;

	if ( *iseries == (MX_ISERIES *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ISERIES pointer for 'iseries' record '%s' used by "
	"iSeries digital output record '%s' and passed by '%s' is NULL.",
			iseries_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_iseries_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_iseries_din_create_record_structures()";

        MX_DIGITAL_INPUT *dinput;
        MX_ISERIES_DINPUT *iseries_dinput;

        /* Allocate memory for the necessary structures. */

        dinput = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        iseries_dinput = (MX_ISERIES_DINPUT *)
				malloc( sizeof(MX_ISERIES_DINPUT) );

        if ( iseries_dinput == (MX_ISERIES_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ISERIES_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = dinput;
        record->record_type_struct = iseries_dinput;
        record->class_specific_function_list
                                = &mxd_iseries_din_digital_input_function_list;

        dinput->record = record;
	iseries_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_iseries_din_finish_record_initialization( MX_RECORD *record )
{
	MX_ISERIES_DINPUT *iseries_dinput;
	char *ptr;

	iseries_dinput = (MX_ISERIES_DINPUT *) record->record_type_struct;

	iseries_dinput->command_prefix = iseries_dinput->command[0];

	ptr = &(iseries_dinput->command[1]);

	iseries_dinput->command_index = mx_hex_string_to_unsigned_long( ptr );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_iseries_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_iseries_din_read()";

	MX_ISERIES_DINPUT *iseries_dinput;
	MX_ISERIES *iseries;
	double double_value;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	iseries = NULL;
	iseries_dinput = NULL;

	mx_status = mxd_iseries_din_get_pointers( dinput,
					&iseries_dinput, &iseries, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_iseries_command( iseries,
					iseries_dinput->command_prefix,
					iseries_dinput->command_index,
					iseries_dinput->num_command_bytes,
					0,
					&double_value,
					0,
					MXD_ISERIES_DIO_DEBUG );

	dinput->value = mx_round( double_value );

	return mx_status;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_iseries_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_iseries_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *doutput;
        MX_ISERIES_DOUTPUT *iseries_doutput;

        /* Allocate memory for the necessary structures. */

        doutput = (MX_DIGITAL_OUTPUT *) malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        iseries_doutput = (MX_ISERIES_DOUTPUT *)
				malloc( sizeof(MX_ISERIES_DOUTPUT) );

        if ( iseries_doutput == (MX_ISERIES_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ISERIES_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = doutput;
        record->record_type_struct = iseries_doutput;
        record->class_specific_function_list
			= &mxd_iseries_dout_digital_output_function_list;

        doutput->record = record;
	iseries_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_iseries_dout_finish_record_initialization( MX_RECORD *record )
{
	MX_ISERIES_DOUTPUT *iseries_doutput;
	char *ptr;

	iseries_doutput = (MX_ISERIES_DOUTPUT *) record->record_type_struct;

	iseries_doutput->command_prefix = iseries_doutput->command[0];

	ptr = &(iseries_doutput->command[1]);

	iseries_doutput->command_index = mx_hex_string_to_unsigned_long( ptr );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_iseries_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_iseries_dout_read()";

	MX_ISERIES_DOUTPUT *iseries_doutput;
	MX_ISERIES *iseries;
	char command_prefix;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_iseries_dout_get_pointers( doutput,
					&iseries_doutput, &iseries, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( iseries_doutput->command_prefix ) {
	case 'W':
		command_prefix = 'R';
		break;
	case 'P':
		command_prefix = 'G';
		break;
	default:
		command_prefix = iseries_doutput->command_prefix;
		break;
	}

	mx_status = mxi_iseries_command( iseries,
					command_prefix,
					iseries_doutput->command_index,
					iseries_doutput->num_command_bytes,
					0,
					&double_value,
					0,
					MXD_ISERIES_DIO_DEBUG );

	doutput->value = mx_round( double_value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_iseries_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_iseries_dout_write()";

	MX_ISERIES_DOUTPUT *iseries_doutput;
	MX_ISERIES *iseries;
	mx_status_type mx_status;

	mx_status = mxd_iseries_dout_get_pointers( doutput,
					&iseries_doutput, &iseries, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_iseries_command( iseries,
					iseries_doutput->command_prefix,
					iseries_doutput->command_index,
					iseries_doutput->num_command_bytes,
					(double) doutput->value,
					NULL,
					0,
					MXD_ISERIES_DIO_DEBUG );
	return mx_status;
}

