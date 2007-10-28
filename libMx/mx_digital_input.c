/*
 * Name:    mx_digital_input.c
 *
 * Purpose: MX function library for digital input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_digital_input.h"

MX_EXPORT mx_status_type
mx_digital_input_read( MX_RECORD *record, unsigned long *value )
{
	static const char fname[] = "mx_digital_input_read()";

	MX_DIGITAL_INPUT *digital_input;
	MX_DIGITAL_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *read_fn ) ( MX_DIGITAL_INPUT * );
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	digital_input = (MX_DIGITAL_INPUT *) record->record_class_struct;

	if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	function_list = (MX_DIGITAL_INPUT_FUNCTION_LIST *)
				(record->class_specific_function_list);

	if ( function_list == (MX_DIGITAL_INPUT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DIGITAL_INPUT_FUNCTION_LIST ptr for record '%s' is NULL.",
			record->name );
	}

	read_fn = function_list->read;

	if ( read_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"read function ptr for record '%s' is NULL.",
			record->name );
	}

	mx_status = (*read_fn)( digital_input );

	if ( value != NULL ) {
		*value = digital_input->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_digital_input_clear( MX_RECORD *record )
{
	static const char fname[] = "mx_digital_input_read()";

	MX_DIGITAL_INPUT *digital_input;
	MX_DIGITAL_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *clear_fn ) ( MX_DIGITAL_INPUT * );
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	digital_input = (MX_DIGITAL_INPUT *) record->record_class_struct;

	if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	function_list = (MX_DIGITAL_INPUT_FUNCTION_LIST *)
				(record->class_specific_function_list);

	if ( function_list == (MX_DIGITAL_INPUT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DIGITAL_INPUT_FUNCTION_LIST ptr for record '%s' is NULL.",
			record->name );
	}

	clear_fn = function_list->clear;

	if ( clear_fn == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = (*clear_fn)( digital_input );

	return mx_status;
}

