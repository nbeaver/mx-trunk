/*
 * Name:    mx_encoder.c
 *
 * Purpose: MX function library of generic encoder operations.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2012, 2016, 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_encoder.h"

MX_EXPORT mx_status_type
mx_encoder_get_pointers( MX_RECORD *encoder_record,
			MX_ENCODER **encoder,
			MX_ENCODER_FUNCTION_LIST **flist_ptr,
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

	if ( flist_ptr != (MX_ENCODER_FUNCTION_LIST **) NULL ) {
		*flist_ptr = (MX_ENCODER_FUNCTION_LIST *)
				(encoder_record->class_specific_function_list);

		if ( *flist_ptr == (MX_ENCODER_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_ENCODER_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				encoder_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_encoder_read( MX_RECORD *encoder_record, double *value )
{
	static const char fname[] = "mx_encoder_read()";

	MX_ENCODER *encoder;
	MX_ENCODER_FUNCTION_LIST *flist;
	mx_status_type ( *read_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_fn = flist->read;

	if ( read_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"read function ptr for MX_ENCODER '%s' is NULL.",
			encoder_record->name );
	}

	mx_status = (*read_fn)( encoder );

	*value = encoder->value;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_encoder_write( MX_RECORD *encoder_record, double value )
{
	static const char fname[] = "mx_encoder_write()";

	MX_ENCODER *encoder;
	MX_ENCODER_FUNCTION_LIST *flist;
	mx_status_type ( *write_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( encoder->encoder_type == MXT_ENC_ABSOLUTE_ENCODER ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an absolute encoder is not allowed.");
	}

	write_fn = flist->write;

	if ( write_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"write function ptr for MX_ENCODER '%s' is NULL.",
			encoder_record->name );
	}

	encoder->value = value;

	mx_status = (*write_fn)( encoder );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_encoder_reset( MX_RECORD *encoder_record )
{
	static const char fname[] = "mx_encoder_reset()";

	MX_ENCODER *encoder;
	MX_ENCODER_FUNCTION_LIST *flist;
	mx_status_type ( *reset_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	reset_fn = flist->reset;

	if ( reset_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"reset function ptr for MX_ENCODER '%s' is NULL.",
			encoder_record->name );
	}

	mx_status = (*reset_fn)( encoder );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_encoder_get_status( MX_RECORD *encoder_record,
			unsigned long *encoder_status )
{
	static const char fname[] = "mx_encoder_get_status()";

	MX_ENCODER *encoder;
	MX_ENCODER_FUNCTION_LIST *flist;
	mx_status_type ( *get_status_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_status_fn = flist->get_status;

	if ( get_status_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_status function ptr for MX_RECORD '%s' is NULL.",
			encoder_record->name );
	}

	mx_status = (*get_status_fn)( encoder );

	*encoder_status = encoder->status;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_encoder_get_measurement_time( MX_RECORD *encoder_record,
					double *measurement_time )
{
	static const char fname[] = "mx_encoder_get_measurement_time()";

	MX_ENCODER *encoder = NULL;
	MX_ENCODER_FUNCTION_LIST *flist = NULL;
	mx_status_type ( *get_measurement_time_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_measurement_time_fn = flist->get_measurement_time;

	if ( get_measurement_time_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_measurement_time function ptr for MX_RECORD '%s' is NULL.",
			encoder_record->name );
	}

	mx_status = (*get_measurement_time_fn)( encoder );

	*measurement_time = encoder->measurement_time;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_encoder_set_measurement_time( MX_RECORD *encoder_record,
					double measurement_time )
{
	static const char fname[] = "mx_encoder_set_measurement_time()";

	MX_ENCODER *encoder = NULL;
	MX_ENCODER_FUNCTION_LIST *flist = NULL;
	mx_status_type ( *set_measurement_time_fn ) ( MX_ENCODER * );
	mx_status_type mx_status;

	mx_status = mx_encoder_get_pointers( encoder_record, &encoder,
					&flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_measurement_time_fn = flist->set_measurement_time;

	if ( set_measurement_time_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_measurement_time function ptr for MX_RECORD '%s' is NULL.",
			encoder_record->name );
	}

	encoder->measurement_time = measurement_time;

	mx_status = (*set_measurement_time_fn)( encoder );

	return mx_status;
}

