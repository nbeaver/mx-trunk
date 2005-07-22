/*
 * Name:    mx_analog_output.c
 *
 * Purpose: MX function library for analog output devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_analog_output.h"

static mx_status_type
mx_analog_output_get_pointers( MX_RECORD *record,
				MX_ANALOG_OUTPUT **analog_output,
				MX_ANALOG_OUTPUT_FUNCTION_LIST **flist_ptr,
				const char *calling_fname )
{
	static const char fname[] = "mx_analog_output_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( record->mx_class != MXC_ANALOG_OUTPUT ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not an analog output record.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			record->name, calling_fname,
			record->mx_superclass,
			record->mx_class,
			record->mx_type );
	}

	if ( analog_output != (MX_ANALOG_OUTPUT **) NULL ) {
		*analog_output = (MX_ANALOG_OUTPUT *)
						(record->record_class_struct);

		if ( *analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ANALOG_OUTPUT pointer for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	if ( flist_ptr != (MX_ANALOG_OUTPUT_FUNCTION_LIST **) NULL ) {
		*flist_ptr = (MX_ANALOG_OUTPUT_FUNCTION_LIST *)
				(record->class_specific_function_list);

		if ( *flist_ptr == (MX_ANALOG_OUTPUT_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_ANALOG_OUTPUT_FUNCTION_LIST ptr for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* mx_analog_output_read() is for reading the value currently being output. */

MX_EXPORT mx_status_type
mx_analog_output_read( MX_RECORD *record, double *value )
{
	static const char fname[] = "mx_analog_output_read()";

	MX_ANALOG_OUTPUT *analog_output;
	MX_ANALOG_OUTPUT_FUNCTION_LIST *function_list;
	double raw_value;
	mx_status_type ( *read_fn ) ( MX_ANALOG_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_analog_output_get_pointers( record, &analog_output,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_fn = function_list->read;

	/* If the read function is defined, invoke it.  Otherwise, we just
	 * return the value already stored in analog_output->value.
	 */

	if ( read_fn != NULL ) {
		mx_status = (*read_fn)( analog_output );
	}

	if ( analog_output->subclass == MXT_AOU_LONG ) {
		raw_value = (double) analog_output->raw_value.long_value;
	} else {
		raw_value = analog_output->raw_value.double_value;
	}

	analog_output->value = analog_output->offset
				+ analog_output->scale * raw_value;

	if ( value != NULL ) {
		*value = analog_output->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_analog_output_write( MX_RECORD *record, double value )
{
	static const char fname[] = "mx_analog_output_write()";

	MX_ANALOG_OUTPUT *analog_output;
	MX_ANALOG_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *write_fn ) ( MX_ANALOG_OUTPUT * );
	double scale, offset, raw_value;
	mx_status_type mx_status;

	mx_status = mx_analog_output_get_pointers( record, &analog_output,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	write_fn = function_list->write;

	if ( write_fn == NULL ){
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"read function ptr for record '%s' is NULL.",
			record->name );
	}

	analog_output->value = value;

	/* Compute the raw value */

	scale = analog_output->scale;
	offset = analog_output->offset;

	raw_value = mx_divide_safely( value - offset, scale );

	if ( analog_output->subclass == MXT_AOU_LONG ) {
		analog_output->raw_value.long_value = mx_round( raw_value );
	} else {
		analog_output->raw_value.double_value = raw_value;
	}

	mx_status = (*write_fn)( analog_output );

	return mx_status;
}

/* mx_analog_output_read_raw_long() is for reading the value 
 * currently being output.
 */

MX_EXPORT mx_status_type
mx_analog_output_read_raw_long( MX_RECORD *record, long *raw_value )
{
	static const char fname[] = "mx_analog_output_read_raw_long()";

	MX_ANALOG_OUTPUT *analog_output;
	MX_ANALOG_OUTPUT_FUNCTION_LIST *function_list;
	long local_raw_value;
	mx_status_type ( *read_fn ) ( MX_ANALOG_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_analog_output_get_pointers( record, &analog_output,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( analog_output->subclass != MXT_AOU_LONG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"This function may only be used with analog outputs that send values "
	"as long integers.  The record '%s' sends values as doubles so "
	"you should use mx_analog_output_read_raw_double() with this record.",
			record->name );
	}

	read_fn = function_list->read;

	if ( read_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"read function ptr for record '%s' is NULL.",
			record->name );
	}

	mx_status = (*read_fn)( analog_output );

	local_raw_value = analog_output->raw_value.long_value;

	analog_output->value = analog_output->offset + analog_output->scale
					* (double) local_raw_value;

	if ( raw_value != NULL ) {
		*raw_value = local_raw_value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_analog_output_write_raw_long( MX_RECORD *record, long raw_value )
{
	static const char fname[] = "mx_analog_output_write_raw_long()";

	MX_ANALOG_OUTPUT *analog_output;
	MX_ANALOG_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *write_fn ) ( MX_ANALOG_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_analog_output_get_pointers( record, &analog_output,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( analog_output->subclass != MXT_AOU_LONG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"This function may only be used with analog outputs that send values "
	"as long integers.  The record '%s' sends values as doubles so "
	"you should use mx_analog_output_read_raw_double() with this record.",
			record->name );
	}

	write_fn = function_list->write;

	if ( write_fn == NULL ){
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"read function ptr for record '%s' is NULL.",
			record->name );
	}

	analog_output->raw_value.long_value = raw_value;

	analog_output->value = analog_output->offset + analog_output->scale
						* (double) raw_value;

	mx_status = (*write_fn)( analog_output );

	return mx_status;
}

/* mx_analog_output_read_raw_double() is for reading the value 
 * currently being output.
 */

MX_EXPORT mx_status_type
mx_analog_output_read_raw_double( MX_RECORD *record, double *raw_value )
{
	static const char fname[] = "mx_analog_output_read_raw_double()";

	MX_ANALOG_OUTPUT *analog_output;
	MX_ANALOG_OUTPUT_FUNCTION_LIST *function_list;
	double local_raw_value;
	mx_status_type ( *read_fn ) ( MX_ANALOG_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_analog_output_get_pointers( record, &analog_output,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( analog_output->subclass != MXT_AOU_LONG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"This function may only be used with analog outputs that send values "
	"as doubles.  The record '%s' sends values as long integers so "
	"you should use mx_analog_output_read_raw_long() with this record.",
			record->name );
	}

	read_fn = function_list->read;

	if ( read_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"read function ptr for record '%s' is NULL.",
			record->name );
	}

	mx_status = (*read_fn)( analog_output );

	local_raw_value = analog_output->raw_value.double_value;

	analog_output->value = analog_output->offset + analog_output->scale
					* local_raw_value;

	if ( raw_value != NULL ) {
		*raw_value = local_raw_value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_analog_output_write_raw_double( MX_RECORD *record, double raw_value )
{
	static const char fname[] = "mx_analog_output_write_raw_double()";

	MX_ANALOG_OUTPUT *analog_output;
	MX_ANALOG_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *write_fn ) ( MX_ANALOG_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_analog_output_get_pointers( record, &analog_output,
						&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( analog_output->subclass != MXT_AOU_LONG ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"This function may only be used with analog outputs that send values "
	"as doubles.  The record '%s' sends values as long integers so "
	"you should use mx_analog_output_read_raw_long() with this record.",
			record->name );
	}

	write_fn = function_list->write;

	if ( write_fn == NULL ){
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"read function ptr for record '%s' is NULL.",
			record->name );
	}

	analog_output->raw_value.double_value = raw_value;

	analog_output->value = analog_output->offset + analog_output->scale
						* raw_value;

	mx_status = (*write_fn)( analog_output );

	return mx_status;
}

