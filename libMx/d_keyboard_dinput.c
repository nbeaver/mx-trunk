/*
 * Name:    d_keyboard_dinput.c
 *
 * Purpose: MX keyboard digital input device driver.  This driver
 *          uses keypresses on a keyboard to simulate a digital input.
 *
 *          Pressing the '+' key or the '1' key turns on the digital input.
 *          Pressing the '-' key or the '0' key turns off the digital input.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
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
#include "mx_key.h"
#include "mx_digital_input.h"
#include "d_keyboard_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_keyboard_dinput_record_function_list = {
	NULL,
	mxd_keyboard_dinput_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_keyboard_dinput_digital_input_function_list = {
	mxd_keyboard_dinput_read,
	mxd_keyboard_dinput_clear
};

MX_RECORD_FIELD_DEFAULTS mxd_keyboard_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_KEYBOARD_DINPUT_STANDARD_FIELDS
};

long mxd_keyboard_dinput_num_record_fields
		= sizeof( mxd_keyboard_dinput_record_field_defaults )
			/ sizeof( mxd_keyboard_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keyboard_dinput_rfield_def_ptr
			= &mxd_keyboard_dinput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_keyboard_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_KEYBOARD_DINPUT **keyboard_dinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_keyboard_dinput_get_pointers()";

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( keyboard_dinput == (MX_KEYBOARD_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEYBOARD_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*keyboard_dinput = (MX_KEYBOARD_DINPUT *)
				dinput->record->record_type_struct;

	if ( *keyboard_dinput == (MX_KEYBOARD_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KEYBOARD_DINPUT pointer for record '%s' is NULL.",
			dinput->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_keyboard_dinput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_keyboard_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_KEYBOARD_DINPUT *keyboard_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        keyboard_dinput = (MX_KEYBOARD_DINPUT *)
				malloc( sizeof(MX_KEYBOARD_DINPUT) );

        if ( keyboard_dinput == (MX_KEYBOARD_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_KEYBOARD_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = keyboard_dinput;
        record->class_specific_function_list
			= &mxd_keyboard_dinput_digital_input_function_list;

        digital_input->record = record;

	digital_input->value = 0;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keyboard_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_keyboard_dinput_read()";

	MX_KEYBOARD_DINPUT *keyboard_dinput;
	int c;
	mx_status_type mx_status;

	mx_status = mxd_keyboard_dinput_get_pointers( dinput,
						&keyboard_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_kbhit() == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	c = mx_getch();

	switch( c ) {
	case '+':
	case '1':
		dinput->value = 1;
		break;
	case '-':
	case '0':
		dinput->value = 0;
		break;
	default:
		return MX_SUCCESSFUL_RESULT;
		break;
	}

	if ( keyboard_dinput->debug ) {
		MX_DEBUG(-2,("%s: '%s' value = %lu",
			fname, dinput->record->name, dinput->value ));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keyboard_dinput_clear( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_keyboard_dinput_clear()";

	MX_KEYBOARD_DINPUT *keyboard_dinput;
	mx_status_type mx_status;

	mx_status = mxd_keyboard_dinput_get_pointers( dinput,
						&keyboard_dinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	while ( mx_kbhit() ) {
		(void) mx_getch();
	}

	return MX_SUCCESSFUL_RESULT;
}

