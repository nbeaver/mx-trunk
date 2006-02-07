/*
 * Name:    mx_encoder.c
 *
 * Purpose: MX function library of generic encoder operations.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006 Illinois Institute of Technology
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
#include "mx_encoder.h"

MX_EXPORT mx_status_type
mx_encoder_get_pointers( MX_RECORD *encoder_record,
			MX_ENCODER **encoder,
			MX_ENCODER_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_encoder_get_pointers()";

	if ( encoder_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The encoder_record pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( encoder_record->mx_class != MXC_ENCODER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not a ENCODER interface.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			encoder_record->name, calling_fname,
			encoder_record->mx_superclass,
			encoder_record->mx_class,
			encoder_record->mx_type );
	}

	if ( encoder != (MX_ENCODER **) NULL ) {
		*encoder = (MX_ENCODER *) (encoder_record->record_class_struct);

		if ( *encoder == (MX_ENCODER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ENCODER pointer for record '%s' passed by '%s' is NULL.",
				encoder_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_ENCODER_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_ENCODER_FUNCTION_LIST *)
				(encoder_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_ENCODER_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_ENCODER_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				encoder_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_encoder_get_overflow_status( MX_RECORD *encoder_record,
				mx_bool_type *underflow_set,
				mx_bool_type *overflow_set )
{
	static const char fname[] = "mx_encoder_get_overflow_status()";

	MX_ENCODER *encoder;
	MX_ENCODER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_overflow_status_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_overflow_status_fn = function_list->get_overflow_status;

	if ( get_overflow_status_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"overflow_set function ptr for MX_ENCODER ptr 0x%p is NULL.",
			encoder);
	}

	mx_status = (*get_overflow_status_fn)( encoder );

	*overflow_set = encoder->overflow_set;
	*underflow_set = encoder->underflow_set;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_encoder_reset_overflow_status( MX_RECORD *encoder_record )
{
	static const char fname[] = "mx_encoder_reset_overflow_status()";

	MX_ENCODER *encoder;
	MX_ENCODER_FUNCTION_LIST *function_list;
	mx_status_type ( *reset_overflow_status_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	reset_overflow_status_fn = function_list->reset_overflow_status;

	if ( reset_overflow_status_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"reset_overflow_status function ptr for MX_ENCODER ptr 0x%p is NULL.",
			encoder);
	}

	mx_status = (*reset_overflow_status_fn)( encoder );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_encoder_read( MX_RECORD *encoder_record, int32_t *value )
{
	static const char fname[] = "mx_encoder_read()";

	MX_ENCODER *encoder;
	MX_ENCODER_FUNCTION_LIST *function_list;
	mx_status_type ( *read_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_fn = function_list->read;

	if ( read_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"read function ptr for MX_ENCODER ptr 0x%p is NULL.",
			encoder);
	}

	mx_status = (*read_fn)( encoder );

	if ( value != (int32_t *) NULL ) {
		*value = encoder->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_encoder_write( MX_RECORD *encoder_record, int32_t value )
{
	static const char fname[] = "mx_encoder_write()";

	MX_ENCODER *encoder;
	MX_ENCODER_FUNCTION_LIST *function_list;
	mx_status_type ( *write_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( encoder->encoder_type == MXT_ENC_ABSOLUTE_ENCODER ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an absolute encoder is not allowed.");
	}

	write_fn = function_list->write;

	if ( write_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"read function ptr for MX_ENCODER ptr 0x%p is NULL.",
			encoder);
	}

	encoder->value = value;

	mx_status = (*write_fn)( encoder );

	return mx_status;
}

