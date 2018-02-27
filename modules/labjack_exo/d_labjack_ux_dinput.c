/*
 * Name:    d_labjack_ux_dinput.c
 *
 * Purpose: MX driver for LabJack U3, U6, and UE9 digital input pins.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_LABJACK_UX_DINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "i_labjack_ux.h"
#include "d_labjack_ux_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_labjack_ux_dinput_record_function_list = {
	NULL,
	mxd_labjack_ux_dinput_create_record_structures,
	mxd_labjack_ux_dinput_finish_record_initialization
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_labjack_ux_dinput_digital_input_function_list
= {
	mxd_labjack_ux_dinput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_labjack_ux_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_LABJACK_UX_DINPUT_STANDARD_FIELDS
};

long mxd_labjack_ux_dinput_num_record_fields
	= sizeof( mxd_labjack_ux_dinput_record_field_defaults )
		/ sizeof( mxd_labjack_ux_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_labjack_ux_dinput_rfield_def_ptr
			= &mxd_labjack_ux_dinput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_labjack_ux_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_LABJACK_UX_DINPUT **labjack_ux_dinput,
			MX_LABJACK_UX **labjack_ux,
			const char *calling_fname )
{
	static const char fname[] = "mxd_labjack_ux_dinput_get_pointers()";

	MX_LABJACK_UX_DINPUT *labjack_ux_dinput_ptr;
	MX_RECORD *labjack_ux_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL",
			calling_fname );
	}

	labjack_ux_dinput_ptr = (MX_LABJACK_UX_DINPUT *)
					dinput->record->record_type_struct;

	if ( labjack_ux_dinput_ptr == (MX_LABJACK_UX_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LABJACK_UX_DINPUT pointer for "
		"digital input '%s' passed by '%s' is NULL",
			dinput->record->name, calling_fname );
	}

	if ( labjack_ux_dinput != (MX_LABJACK_UX_DINPUT **) NULL ) {
		*labjack_ux_dinput = labjack_ux_dinput_ptr;
	}

	if ( labjack_ux != (MX_LABJACK_UX **) NULL ) {
		labjack_ux_record = labjack_ux_dinput_ptr->labjack_ux_record;

		*labjack_ux = (MX_LABJACK_UX *)
				labjack_ux_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_labjack_ux_dinput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_labjack_ux_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *dinput;
        MX_LABJACK_UX_DINPUT *labjack_ux_dinput;

        /* Allocate memory for the necessary structures. */

        dinput = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_DIGITAL_INPUT structure." );
        }

        labjack_ux_dinput = (MX_LABJACK_UX_DINPUT *)
				malloc( sizeof(MX_LABJACK_UX_DINPUT) );

        if ( labjack_ux_dinput == (MX_LABJACK_UX_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_LABJACK_UX_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = dinput;
        record->record_type_struct = labjack_ux_dinput;
        record->class_specific_function_list
			= &mxd_labjack_ux_dinput_digital_input_function_list;

        dinput->record = record;
	labjack_ux_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labjack_ux_dinput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_labjack_ux_dinput_finish_record_initialization()";

	MX_DIGITAL_INPUT *dinput = NULL;
	MX_LABJACK_UX_DINPUT *labjack_ux_dinput = NULL;
	MX_LABJACK_UX *labjack_ux = NULL;
	uint8_t pin_number;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	dinput = (MX_DIGITAL_INPUT *) record->record_class_struct;

	mx_status = mxd_labjack_ux_dinput_get_pointers( dinput,
				&labjack_ux_dinput, &labjack_ux, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: labjack_ux->product_id = %lu",
		fname, labjack_ux->product_id));

	switch( labjack_ux->product_id ) {
	case U3_PRODUCT_ID:
		pin_number = labjack_ux_dinput->pin_number;

		if ( ( pin_number / 8 ) == 0 ) {
			labjack_ux->u.u3_config.fio_analog
				|=  ( 1 << pin_number );
		} else
		if ( ( pin_number / 8 ) == 1 ) {
			labjack_ux->u.u3_config.eio_analog
				|=  ( 1 << ( pin_number - 8 ) );
		} else {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Requested pin number %u for digital input '%s' "
			"is outside the allowed range from 0 to 15.",
				pin_number, record->name );
		}
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"LabJack product '%s' used by record '%s' is not supported.",
			labjack_ux->product_name, record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_labjack_ux_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_labjack_ux_dinput_read()";

	MX_LABJACK_UX_DINPUT *labjack_ux_dinput = NULL;
	MX_LABJACK_UX *labjack_ux = NULL;
	uint8_t command[256];
	uint8_t response[256];
	mx_status_type mx_status;

	mx_status = mxd_labjack_ux_dinput_get_pointers( dinput,
				&labjack_ux_dinput, &labjack_ux, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a feedback command to get the digital input value. */

	command[1] = 0xF8;
	command[2] = 2;
	command[3] = 0x00;

	command[6] = 0xAB;	/* Echo byte is echoed in response[8] */

	command[7] = 10;	/* IOType = 10 (BitStateRead) */

	command[8] = ( labjack_ux_dinput->pin_number ) & 0xF;

	command[9] = 0;		/* Fill to a multiple of 2. */

	mx_status = mxi_labjack_ux_command( labjack_ux, command, 9,
					response, sizeof(response), -1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dinput->value = response[9];

	return MX_SUCCESSFUL_RESULT;
}

