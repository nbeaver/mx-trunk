/*
 * Name:    mx_digital_output.c
 *
 * Purpose: MX function library for digital output devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2007-2008 Illinois Institute of Technology
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
#include "mx_hrt.h"
#include "mx_digital_output.h"

/*-----------------------------------------------------------------------*/

static mx_status_type
mx_digital_output_get_pointers( MX_RECORD *record,
			MX_DIGITAL_OUTPUT **doutput,
			MX_DIGITAL_OUTPUT_FUNCTION_LIST **flist_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_digital_output_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( record->mx_class != MXC_DIGITAL_OUTPUT ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not a digital output.",
			record->name, calling_fname );
	}

	if ( doutput != (MX_DIGITAL_OUTPUT **) NULL ) {
		*doutput = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

		if ( *doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_DIGITAL_OUTPUT pointer for record '%s' passed by '%s' is NULL",
				record->name, calling_fname );
		}
	}

	if ( flist_ptr != (MX_DIGITAL_OUTPUT_FUNCTION_LIST **) NULL ) {
		*flist_ptr = (MX_DIGITAL_OUTPUT_FUNCTION_LIST *)
				(record->class_specific_function_list);

		if ( *flist_ptr == (MX_DIGITAL_OUTPUT_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DIGITAL_OUTPUT_FUNCTION_LIST pointer for "
			"record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mx_digital_output_read() is for reading the value currently being output. */

MX_EXPORT mx_status_type
mx_digital_output_read( MX_RECORD *record, unsigned long *value )
{
	static const char fname[] = "mx_digital_output_read()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_DIGITAL_OUTPUT_FUNCTION_LIST *flist;
	mx_status_type ( *read_fn ) ( MX_DIGITAL_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_digital_output_get_pointers( record,
						&doutput, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_fn = flist->read;

	if ( read_fn != NULL ) {
		mx_status = (*read_fn)( doutput );
	}

	if ( value != NULL ) {
		*value = doutput->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_digital_output_write( MX_RECORD *record, unsigned long value )
{
	static const char fname[] = "mx_digital_output_write()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_DIGITAL_OUTPUT_FUNCTION_LIST *flist;
	mx_status_type ( *write_fn ) ( MX_DIGITAL_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_digital_output_get_pointers( record,
						&doutput, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	write_fn = flist->write;

	if ( write_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"write function pointer for record '%s' is NULL.",
			record->name );
	}

	doutput->value = value;

	mx_status = (*write_fn)( doutput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_digital_output_pulse( MX_RECORD *record,
			long pulse_on_value,
			long pulse_off_value,
			double pulse_duration )
{
	static const char fname[] = "mx_digital_output_pulse()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_DIGITAL_OUTPUT_FUNCTION_LIST *flist;
	mx_status_type ( *pulse_fn ) ( MX_DIGITAL_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_digital_output_get_pointers( record,
						&doutput, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_fn = flist->pulse;

	if ( pulse_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"pulse function pointer for record '%s' is NULL.",
			record->name );
	}

	doutput->pulse_on_value = pulse_on_value;
	doutput->pulse_off_value = pulse_off_value;

	doutput->pulse_duration = pulse_duration;

	mx_status = (*pulse_fn)( doutput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_digital_output_pulse_wait( MX_RECORD *record,
			long pulse_on_value,
			long pulse_off_value,
			double pulse_duration,
			mx_bool_type busy_wait )
{
	unsigned long pulse_seconds, pulse_microseconds;
	mx_bool_type use_seconds;
	mx_status_type mx_status;

	if ( pulse_duration > 3600.0 ) {
		pulse_seconds = mx_round( pulse_duration );
		use_seconds = TRUE;
	} else {
		pulse_microseconds = mx_round( 1000000.0 * pulse_duration );
		use_seconds = FALSE;
	}

	mx_status = mx_digital_output_write( record, pulse_on_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( use_seconds ) {
		mx_sleep( pulse_seconds );
	} else {
		if ( busy_wait ) {
			mx_udelay( pulse_microseconds );
		} else {
			mx_usleep( pulse_microseconds );
		}
	}

	mx_status = mx_digital_output_write( record, pulse_off_value );

	return mx_status;
}

