/*
 * Name:    mx_waveform_output.c
 *
 * Purpose: MX function library for waveform output devices such as
 *          waveform generators.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_driver.h"
#include "mx_waveform_output.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_waveform_output_
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_waveform_output_get_pointers( MX_RECORD *wvout_record,
			MX_WAVEFORM_OUTPUT **waveform_output,
			MX_WAVEFORM_OUTPUT_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_waveform_output_get_pointers()";

	if ( wvout_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The wvout_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( wvout_record->mx_class != MXC_WAVEFORM_OUTPUT ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not an WAVEFORM_OUTPUT.",
			wvout_record->name, calling_fname );
	}

	if ( waveform_output != (MX_WAVEFORM_OUTPUT **) NULL ) {
		*waveform_output = (MX_WAVEFORM_OUTPUT *)
					wvout_record->record_class_struct;

		if ( *waveform_output == (MX_WAVEFORM_OUTPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_WAVEFORM_OUTPUT pointer for record '%s' passed "
			"by '%s' is NULL",
				wvout_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_WAVEFORM_OUTPUT_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_WAVEFORM_OUTPUT_FUNCTION_LIST *)
				wvout_record->class_specific_function_list;

		if ( *function_list_ptr ==
				(MX_WAVEFORM_OUTPUT_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_WAVEFORM_OUTPUT_FUNCTION_LIST pointer for record "
			"'%s' passed by '%s' is NULL.",
				wvout_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* Please note that the 'channel_data' field is handled specially and is
 * actually initialized in mx_waveform_output_finish_record_initialization().
 * This is because the 'channel_data' field is just a pointer into the
 * appropriate row of the 'data_array' field.
 */

MX_EXPORT mx_status_type
mx_waveform_output_initialize_type( long record_type,
			long *num_record_fields,
			MX_RECORD_FIELD_DEFAULTS **record_field_defaults,
			long *maximum_num_channels_varargs_cookie,
			long *maximum_num_points_varargs_cookie )
{
	static const char fname[] = "mx_waveform_output_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	if ( num_record_fields == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"num_record_fields pointer passed was NULL." );
	}
	if ( record_field_defaults == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"record_field_defaults pointer passed was NULL." );
	}
	if ( maximum_num_channels_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "maximum_num_channels_varargs_cookie pointer passed was NULL." );
	}
	if ( maximum_num_points_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"maximum_num_points_varargs_cookie pointer passed was NULL." );
	}

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr
			= driver->record_field_defaults_ptr;

	if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	*record_field_defaults = *record_field_defaults_ptr;

	if ( *record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (long *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	*num_record_fields = *(driver->num_record_fields);

	/* Set varargs cookies in 'data_array' that depend on the values
	 * of 'maximum_num_channels' and 'maximum_num_points'.
	 */

	mx_status = mx_find_record_field_defaults( *record_field_defaults,
			*num_record_fields, "data_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults_index(
			*record_field_defaults, *num_record_fields,
			"maximum_num_channels", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
					maximum_num_channels_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults_index(
			*record_field_defaults, *num_record_fields,
			"maximum_num_points", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				maximum_num_points_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_channels_varargs_cookie;
	field->dimension[1] = *maximum_num_points_varargs_cookie;

	/* 'timer_data' depends on 'maximum_num_points'. */

	mx_status = mx_find_record_field_defaults( *record_field_defaults,
			*num_record_fields, "timer_data", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_points_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_waveform_output_finish_record_initialization( MX_RECORD *wvout_record )
{
	static const char fname[] =
			"mx_waveform_output_finish_record_initialization()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_WAVEFORM_OUTPUT_FUNCTION_LIST *function_list;
	MX_RECORD_FIELD *channel_data_field;
	MX_RECORD_FIELD *timer_data_field;
	mx_status_type mx_status;

	mx_status = mx_waveform_output_get_pointers( wvout_record,
					&wvout, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( wvout->maximum_num_channels == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The waveform output '%s' is set to have zero channels.  "
	"This is an error since at least one channel is required.",
			wvout_record->name );
	}

	if ( wvout->maximum_num_points == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The waveform output '%s' is set to have zero points.  "
	"This is an error since at least one point is required.",
			wvout_record->name );
	}

	wvout->current_num_channels = wvout->maximum_num_channels;
	wvout->current_num_points = wvout->maximum_num_points;

	/* Initialize the 'channel_data' pointer to point at the first
	 * row of the 'data_array' field.  We also initialize the length
	 * of the 'channel_data' field to be 'maximum_num_points' long.
	 */

	mx_status = mx_find_record_field( wvout_record, "channel_data",
					&channel_data_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel_data_field->dimension[0] = wvout->maximum_num_points;

	wvout->channel_index = 0;

	wvout->channel_data = wvout->data_array[0];

	mx_status = mx_find_record_field( wvout_record, "timer_data",
					&timer_data_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_waveform_output_arm( MX_RECORD *wvout_record )
{
	static const char fname[] = "mx_waveform_output_arm()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_WAVEFORM_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *arm_fn ) ( MX_WAVEFORM_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_output_get_pointers( wvout_record,
					&wvout, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	arm_fn = function_list->arm;

	if ( arm_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"arm function ptr for record '%s' is NULL.",
			wvout_record->name );
	}

	mx_status = (*arm_fn)( wvout );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_waveform_output_trigger( MX_RECORD *wvout_record )
{
	static const char fname[] = "mx_waveform_output_trigger()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_WAVEFORM_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *trigger_fn ) ( MX_WAVEFORM_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_output_get_pointers( wvout_record,
					&wvout, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger_fn = function_list->trigger;

	if ( trigger_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trigger function ptr for record '%s' is NULL.",
			wvout_record->name );
	}

	mx_status = (*trigger_fn)( wvout );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_waveform_output_stop( MX_RECORD *wvout_record )
{
	static const char fname[] = "mx_waveform_output_stop()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_WAVEFORM_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *stop_fn ) ( MX_WAVEFORM_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_output_get_pointers( wvout_record,
					&wvout, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop_fn = function_list->stop;

	if ( stop_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"stop function ptr for record '%s' is NULL.",
			wvout_record->name );
	}

	mx_status = (*stop_fn)( wvout );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_waveform_output_is_busy( MX_RECORD *wvout_record, mx_bool_type *busy )
{
	static const char fname[] = "mx_waveform_output_is_busy()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_WAVEFORM_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *busy_fn ) ( MX_WAVEFORM_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_output_get_pointers( wvout_record,
					&wvout, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	busy_fn = function_list->busy;

	if ( busy_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"busy function ptr for record '%s' is NULL.",
			wvout_record->name );
	}

	mx_status = (*busy_fn)( wvout );

	if ( busy != NULL ) {
		*busy = wvout->busy;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_waveform_output_read_all( MX_RECORD *wvout_record,
			unsigned long *num_channels,
			unsigned long *num_points,
			double ***wvout_data )
{
	static const char fname[] = "mx_waveform_output_read_all()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_WAVEFORM_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *read_all_fn ) ( MX_WAVEFORM_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_output_get_pointers( wvout_record,
					&wvout, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_all_fn = function_list->read_all;

	if ( read_all_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Reading the WAVEFORM_OUTPUT data as one big block "
		"is not supported for WAVEFORM_OUTPUT '%s'.",
			wvout_record->name );
	}

	mx_status = (*read_all_fn)( wvout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_channels != NULL ) {
		*num_channels = wvout->current_num_channels;
	}
	if ( num_points != NULL ) {
		*num_points = wvout->current_num_points;
	}
	if ( wvout_data != NULL ) {
		*wvout_data = wvout->data_array;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_waveform_output_read_channel( MX_RECORD *wvout_record,
			unsigned long channel_index,
			unsigned long *num_points,
			double **channel_data )
{
	static const char fname[] = "mx_waveform_output_read_channel()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_WAVEFORM_OUTPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *read_channel_fn ) ( MX_WAVEFORM_OUTPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_output_get_pointers( wvout_record,
					&wvout, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_channel_fn = function_list->read_channel;

	if ( read_channel_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Reading the WAVEFORM_OUTPUT data one channel at a time "
			"is not supported for WAVEFORM_OUTPUT '%s'.",
			wvout_record->name );
	}

	if ( ((long) channel_index) >= wvout->maximum_num_channels ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Channel index %lu for WAVEFORM_OUTPUT record '%s' is outside "
		"the allowed range of 0-%ld.",
			channel_index, wvout_record->name,
			wvout->maximum_num_channels - 1L );
	}

	wvout->channel_index = (long) channel_index;

	mx_status = (*read_channel_fn)( wvout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	wvout->channel_data = (wvout->data_array) [ wvout->channel_index ];

	if ( num_points != NULL ) {
		*num_points = wvout->current_num_points;
	}
	if ( channel_data != NULL ) {
		*channel_data = wvout->channel_data;
	}

	return MX_SUCCESSFUL_RESULT;
}

