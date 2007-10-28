/*
 * Name:    mx_digital_output.c
 *
 * Purpose: MX function library for digital output devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2007 Illinois Institute of Technology
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
#include "mx_digital_output.h"

/* mx_digital_output_read() is for reading the value currently being output. */

MX_EXPORT mx_status_type
mx_digital_output_read( MX_RECORD *record, unsigned long *value )
{
	static const char fname[] = "mx_digital_output_read()";

	MX_DIGITAL_OUTPUT *digital_output;
	MX_DIGITAL_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *read_fn ) ( MX_DIGITAL_OUTPUT * );
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.\n");
	}

	digital_output = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	function_list = (MX_DIGITAL_OUTPUT_FUNCTION_LIST *)
				(record->class_specific_function_list);

	if ( function_list == (MX_DIGITAL_OUTPUT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DIGITAL_OUTPUT_FUNCTION_LIST ptr for record '%s' is NULL.",
			record->name );
	}

	read_fn = function_list->read;

	if ( read_fn != NULL ) {
		mx_status = (*read_fn)( digital_output );
	}

	if ( value != NULL ) {
		*value = digital_output->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_digital_output_write( MX_RECORD *record, unsigned long value )
{
	static const char fname[] = "mx_digital_output_write()";

	MX_DIGITAL_OUTPUT *digital_output;
	MX_DIGITAL_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *write_fn ) ( MX_DIGITAL_OUTPUT * );
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	digital_output = (MX_DIGITAL_OUTPUT *) record->record_class_struct;

	if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer for record '%s' is NULL.",
			record->name );
	}

	function_list = (MX_DIGITAL_OUTPUT_FUNCTION_LIST *)
				(record->class_specific_function_list);

	if ( function_list == (MX_DIGITAL_OUTPUT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DIGITAL_OUTPUT_FUNCTION_LIST ptr for record '%s' is NULL.",
			record->name );
	}

	write_fn = function_list->write;

	if ( write_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"read function ptr for record '%s' is NULL.",
			record->name );
	}

	digital_output->value = value;

	mx_status = (*write_fn)( digital_output );

	return mx_status;
}

