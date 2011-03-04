/*
 * Name:    d_pleora_iport_dio.cpp
 *
 * Purpose: MX input and output drivers to control digital I/O ports
 *          on Pleora iPORT IP engines.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PLEORA_IPORT_DIO_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"

#include "i_pleora_iport.h"
#include "d_pleora_iport_vinput.h"
#include "d_pleora_iport_dio.h"

/* Initialize the PLEORA_IPORT digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_pleora_iport_dinput_record_function_list = {
	NULL,
	mxd_pleora_iport_dinput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_pleora_iport_dinput_open
};

MX_DIGITAL_INPUT_FUNCTION_LIST
mxd_pleora_iport_dinput_digital_input_function_list = {
	mxd_pleora_iport_dinput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_pleora_iport_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_PLEORA_IPORT_DINPUT_STANDARD_FIELDS
};

long mxd_pleora_iport_dinput_num_record_fields
	= sizeof( mxd_pleora_iport_dinput_record_field_defaults )
		/ sizeof( mxd_pleora_iport_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pleora_iport_dinput_rfield_def_ptr
			= &mxd_pleora_iport_dinput_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_pleora_iport_doutput_record_function_list = {
	NULL,
	mxd_pleora_iport_doutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_pleora_iport_doutput_open
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
mxd_pleora_iport_doutput_digital_output_function_list = {
	mxd_pleora_iport_doutput_read,
	mxd_pleora_iport_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_pleora_iport_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_PLEORA_IPORT_DOUTPUT_STANDARD_FIELDS
};

long mxd_pleora_iport_doutput_num_record_fields
	= sizeof( mxd_pleora_iport_doutput_record_field_defaults )
		/ sizeof( mxd_pleora_iport_doutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pleora_iport_doutput_rfield_def_ptr
			= &mxd_pleora_iport_doutput_record_field_defaults[0];

static mx_status_type
mxd_pleora_iport_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_PLEORA_IPORT_DIGITAL_INPUT **pleora_iport_dinput,
			MX_PLEORA_IPORT_VINPUT **pleora_iport_vinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pleora_iport_dinput_get_pointers()";

	MX_PLEORA_IPORT_DIGITAL_INPUT *pleora_iport_dinput_ptr;
	MX_RECORD *pleora_iport_vinput_record;

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

	pleora_iport_dinput_ptr = (MX_PLEORA_IPORT_DIGITAL_INPUT *)
					dinput->record->record_type_struct;

	if (pleora_iport_dinput_ptr == (MX_PLEORA_IPORT_DIGITAL_INPUT *) NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLEORA_IPORT_DIGITAL_INPUT pointer for record '%s' "
		"passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( pleora_iport_dinput != (MX_PLEORA_IPORT_DIGITAL_INPUT **) NULL ) {
		*pleora_iport_dinput = pleora_iport_dinput_ptr;
	}

	if ( pleora_iport_vinput != (MX_PLEORA_IPORT_VINPUT **) NULL ) {

		pleora_iport_vinput_record =
			pleora_iport_dinput_ptr->pleora_iport_vinput_record;

		if ( pleora_iport_vinput_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"pleora_iport_vinput_record pointer for PLEORA_IPORT "
			"digital input record '%s' passed by '%s' is NULL.",
				dinput->record->name, calling_fname );
		}

		*pleora_iport_vinput = (MX_PLEORA_IPORT_VINPUT *)
			pleora_iport_vinput_record->record_type_struct;

		if (*pleora_iport_vinput == (MX_PLEORA_IPORT_VINPUT *) NULL) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT_VINPUT pointer for record '%s' "
			"used by digital input record '%s' and passed "
			"by '%s' is NULL.",
				pleora_iport_vinput_record->name,
				dinput->record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pleora_iport_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_PLEORA_IPORT_DIGITAL_OUTPUT **pleora_iport_doutput,
			MX_PLEORA_IPORT_VINPUT **pleora_iport_vinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pleora_iport_doutput_get_pointers()";

	MX_PLEORA_IPORT_DIGITAL_OUTPUT *pleora_iport_doutput_ptr;
	MX_RECORD *pleora_iport_vinput_record;

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

	pleora_iport_doutput_ptr = (MX_PLEORA_IPORT_DIGITAL_OUTPUT *)
					doutput->record->record_type_struct;

	if (pleora_iport_doutput_ptr == (MX_PLEORA_IPORT_DIGITAL_OUTPUT *) NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLEORA_IPORT_DIGITAL_OUTPUT pointer for record '%s' "
		"passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if (pleora_iport_doutput != (MX_PLEORA_IPORT_DIGITAL_OUTPUT **) NULL) {
		*pleora_iport_doutput = pleora_iport_doutput_ptr;
	}

	if ( pleora_iport_vinput != (MX_PLEORA_IPORT_VINPUT **) NULL ) {

		pleora_iport_vinput_record =
			pleora_iport_doutput_ptr->pleora_iport_vinput_record;

		if ( pleora_iport_vinput_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"pleora_iport_vinput_record pointer for PLEORA_IPORT "
			"digital output record '%s' passed by '%s' is NULL.",
				doutput->record->name, calling_fname );
		}

		*pleora_iport_vinput = (MX_PLEORA_IPORT_VINPUT *)
			pleora_iport_vinput_record->record_type_struct;

		if (*pleora_iport_vinput == (MX_PLEORA_IPORT_VINPUT *) NULL) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT_VINPUT pointer for record '%s' "
			"used by digital output record '%s' and passed "
			"by '%s' is NULL.",
				pleora_iport_vinput_record->name,
				doutput->record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_pleora_iport_dinput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_pleora_iport_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_PLEORA_IPORT_DIGITAL_INPUT *pleora_iport_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_INPUT structure." );
        }

        pleora_iport_dinput = (MX_PLEORA_IPORT_DIGITAL_INPUT *)
				malloc( sizeof(MX_PLEORA_IPORT_DIGITAL_INPUT) );

        if ( pleora_iport_dinput == (MX_PLEORA_IPORT_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for MX_PLEORA_IPORT_DIGITAL_INPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = pleora_iport_dinput;
        record->class_specific_function_list
			= &mxd_pleora_iport_dinput_digital_input_function_list;

        digital_input->record = record;
	pleora_iport_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_dinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pleora_iport_dinput_open()";

	MX_DIGITAL_INPUT *dinput;
	MX_PLEORA_IPORT_DIGITAL_INPUT *pleora_iport_dinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_dinput_get_pointers( dinput,
			&pleora_iport_dinput, &pleora_iport_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_strcasecmp( "A0", pleora_iport_dinput->line_name ) == 0 ) {
		pleora_iport_dinput->line_id = 0;
	} else
	if ( mx_strcasecmp( "A1", pleora_iport_dinput->line_name ) == 0 ) {
		pleora_iport_dinput->line_id = 1;
	} else
	if ( mx_strcasecmp( "A2", pleora_iport_dinput->line_name ) == 0 ) {
		pleora_iport_dinput->line_id = 2;
	} else
	if ( mx_strcasecmp( "A3", pleora_iport_dinput->line_name ) == 0 ) {
		pleora_iport_dinput->line_id = 3;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized line name '%s' requested for "
		"digital input '%s'.  The allowed names are "
		"A0, A1, A2, and A3.",
			pleora_iport_dinput->line_name,
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_pleora_iport_dinput_read()";

	MX_PLEORA_IPORT_DIGITAL_INPUT *pleora_iport_dinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	__int64 raw_value;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_dinput_get_pointers( dinput,
			&pleora_iport_dinput, &pleora_iport_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	CyDevice &device = pleora_iport_vinput->grabber->GetDevice();

	CyDeviceExtension *extension =
			&device.GetExtension( CY_DEVICE_EXT_GPIO_CONTROL_BITS );

	extension->LoadFromDevice();
	extension->GetParameter(CY_GPIO_CONTROL_BITS_INPUTS_VALUE, raw_value);

	if ( raw_value & ( 1 << pleora_iport_dinput->line_id ) ) {
		dinput->value = 1;
	} else {
		dinput->value = 0;
	}

#if MXD_PLEORA_IPORT_DIO_DEBUG
	MX_DEBUG(-2,("%s: raw_value = %#lI64x, dinput->value = %lu",
		fname, raw_value, dinput->value));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_pleora_iport_doutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_pleora_iport_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_PLEORA_IPORT_DIGITAL_OUTPUT *pleora_iport_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        pleora_iport_doutput = (MX_PLEORA_IPORT_DIGITAL_OUTPUT *)
			malloc( sizeof(MX_PLEORA_IPORT_DIGITAL_OUTPUT) );

        if ( pleora_iport_doutput == (MX_PLEORA_IPORT_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for MX_PLEORA_IPORT_DIGITAL_OUTPUT structure.");
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = pleora_iport_doutput;
        record->class_specific_function_list
		= &mxd_pleora_iport_doutput_digital_output_function_list;

        digital_output->record = record;
	pleora_iport_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_doutput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pleora_iport_doutput_open()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_PLEORA_IPORT_DIGITAL_OUTPUT *pleora_iport_doutput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	doutput = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_doutput_get_pointers( doutput,
			&pleora_iport_doutput, &pleora_iport_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_strcasecmp( "Q0", pleora_iport_doutput->line_name ) == 0 ) {
		pleora_iport_doutput->line_id = 0;
	} else
	if ( mx_strcasecmp( "Q1", pleora_iport_doutput->line_name ) == 0 ) {
		pleora_iport_doutput->line_id = 1;
	} else
	if ( mx_strcasecmp( "Q2", pleora_iport_doutput->line_name ) == 0 ) {
		pleora_iport_doutput->line_id = 2;
	} else
	if ( mx_strcasecmp( "Q3", pleora_iport_doutput->line_name ) == 0 ) {
		pleora_iport_doutput->line_id = 3;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized line name '%s' requested for "
		"digital output '%s'.  The allowed names are "
		"A0, A1, A2, and A3.",
			pleora_iport_doutput->line_name,
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_pleora_iport_doutput_write()";

	MX_PLEORA_IPORT_DIGITAL_OUTPUT *pleora_iport_doutput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput;
	char program_buffer[20];
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_doutput_get_pointers( doutput,
			&pleora_iport_doutput, &pleora_iport_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( doutput->value == 0 ) {
		snprintf( program_buffer, sizeof(program_buffer),
			"Q%d=0\r\n", pleora_iport_doutput->line_id );
	} else {
		doutput->value = 1;

		snprintf( program_buffer, sizeof(program_buffer),
			"Q%d=1\r\n", pleora_iport_doutput->line_id );
	}

	CyGrabber *grabber = pleora_iport_vinput->grabber;

	CyString lut_program = program_buffer;

	mxi_pleora_iport_send_lookup_table_program( grabber, lut_program );

	return mx_status;
}

