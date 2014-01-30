/*
 * Name:    mx_waveform_input.c
 *
 * Purpose: MX function library for waveform input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
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
#include "mx_waveform_input.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_waveform_input_
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_waveform_input_get_pointers( MX_RECORD *wvin_record,
			MX_WAVEFORM_INPUT **waveform_input,
			MX_WAVEFORM_INPUT_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_waveform_input_get_pointers()";

	if ( wvin_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The wvin_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( wvin_record->mx_class != MXC_WAVEFORM_INPUT ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not an WAVEFORM_INPUT.",
			wvin_record->name, calling_fname );
	}

	if ( waveform_input != (MX_WAVEFORM_INPUT **) NULL ) {
		*waveform_input = (MX_WAVEFORM_INPUT *)
					wvin_record->record_class_struct;

		if ( *waveform_input == (MX_WAVEFORM_INPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_WAVEFORM_INPUT pointer for record '%s' passed "
			"by '%s' is NULL",
				wvin_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_WAVEFORM_INPUT_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_WAVEFORM_INPUT_FUNCTION_LIST *)
				wvin_record->class_specific_function_list;

		if ( *function_list_ptr ==
				(MX_WAVEFORM_INPUT_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_WAVEFORM_INPUT_FUNCTION_LIST pointer for record "
			"'%s' passed by '%s' is NULL.",
				wvin_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* Please note that the 'channel_data' field is handled specially and is
 * actually initialized in mx_waveform_input_finish_record_initialization().
 * This is because the 'channel_data' field is just a pointer into the
 * appropriate row of the 'data_array' field.
 */

MX_EXPORT mx_status_type
mx_waveform_input_initialize_driver( MX_DRIVER *driver,
			long *maximum_num_channels_varargs_cookie,
			long *maximum_num_steps_varargs_cookie )
{
	static const char fname[] = "mx_waveform_input_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}
	if ( maximum_num_channels_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "maximum_num_channels_varargs_cookie pointer passed was NULL." );
	}
	if ( maximum_num_steps_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"maximum_num_steps_varargs_cookie pointer passed was NULL." );
	}

	/* Set varargs cookies in 'data_array' that depend on the values
	 * of 'maximum_num_channels' and 'maximum_num_steps'.
	 */

	mx_status = mx_find_record_field_defaults( driver,
						"data_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults_index( driver,
						"maximum_num_channels",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
					maximum_num_channels_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults_index( driver,
						"maximum_num_steps",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				maximum_num_steps_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_channels_varargs_cookie;
	field->dimension[1] = *maximum_num_steps_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_waveform_input_finish_record_initialization( MX_RECORD *wvin_record )
{
	static const char fname[] =
			"mx_waveform_input_finish_record_initialization()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	MX_RECORD_FIELD *channel_data_field;
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( wvin->maximum_num_channels == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The waveform input '%s' is set to have zero channels.  "
	"This is an error since at least one channel is required.",
			wvin_record->name );
	}

	if ( wvin->maximum_num_steps == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The waveform input '%s' is set to have zero points.  "
	"This is an error since at least one point is required.",
			wvin_record->name );
	}

	wvin->current_num_channels = wvin->maximum_num_channels;
	wvin->current_num_steps = wvin->maximum_num_steps;

	/* Initialize the 'channel_data' pointer to point at the first
	 * row of the 'data_array' field.  We also initialize the length
	 * of the 'channel_data' field to be 'maximum_num_steps' long.
	 */

	mx_status = mx_find_record_field( wvin_record, "channel_data",
					&channel_data_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel_data_field->dimension[0] = wvin->maximum_num_steps;

	wvin->channel_index = 0;

	wvin->channel_data = wvin->data_array[0];

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_waveform_input_arm( MX_RECORD *wvin_record )
{
	static const char fname[] = "mx_waveform_input_arm()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *arm_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	arm_fn = function_list->arm;

	if ( arm_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"arm function ptr for record '%s' is NULL.",
			wvin_record->name );
	}

	mx_status = (*arm_fn)( wvin );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_waveform_input_trigger( MX_RECORD *wvin_record )
{
	static const char fname[] = "mx_waveform_input_trigger()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *trigger_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger_fn = function_list->trigger;

	if ( trigger_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trigger function ptr for record '%s' is NULL.",
			wvin_record->name );
	}

	mx_status = (*trigger_fn)( wvin );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_waveform_input_start( MX_RECORD *wvin_record )
{
	mx_status_type mx_status;

	mx_status = mx_waveform_input_arm( wvin_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_waveform_input_trigger( wvin_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_waveform_input_stop( MX_RECORD *wvin_record )
{
	static const char fname[] = "mx_waveform_input_stop()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *stop_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop_fn = function_list->stop;

	if ( stop_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"stop function ptr for record '%s' is NULL.",
			wvin_record->name );
	}

	mx_status = (*stop_fn)( wvin );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_waveform_input_is_busy( MX_RECORD *wvin_record, mx_bool_type *busy )
{
	static const char fname[] = "mx_waveform_input_is_busy()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *busy_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	busy_fn = function_list->busy;

	if ( busy_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"busy function ptr for record '%s' is NULL.",
			wvin_record->name );
	}

	mx_status = (*busy_fn)( wvin );

	if ( busy != NULL ) {
		*busy = wvin->busy;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_waveform_input_read_all( MX_RECORD *wvin_record,
				unsigned long *num_channels,
				unsigned long *num_steps,
				double ***wvin_data )
{
	static const char fname[] = "mx_waveform_input_read_all()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *read_all_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type ( *read_channel_fn ) ( MX_WAVEFORM_INPUT * );
	long ch;
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_all_fn = function_list->read_all;

	if ( read_all_fn != NULL ) {
		mx_status = (*read_all_fn)( wvin );
	} else {
		read_channel_fn = function_list->read_channel;

		if ( read_channel_fn == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The device driver for record '%s' does not have "
			"any working 'read' functions!", wvin_record->name );
		}

		for ( ch = 0; ch < wvin->current_num_channels; ch++ ) {
			wvin->channel_index = ch;

			wvin->channel_data = (wvin->data_array)[ch];

			mx_status = (*read_channel_fn)( wvin );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_channels != NULL ) {
		*num_channels = wvin->current_num_channels;
	}
	if ( num_steps != NULL ) {
		*num_steps = wvin->current_num_steps;
	}
	if ( wvin_data != NULL ) {
		*wvin_data = wvin->data_array;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_waveform_input_read_channel( MX_RECORD *wvin_record,
				unsigned long channel_index,
				unsigned long *num_steps,
				double **channel_data )
{
	static const char fname[] = "mx_waveform_input_read_channel()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *read_channel_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_channel_fn = function_list->read_channel;

	if ( read_channel_fn == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Reading the waveform data one channel at a time "
		"is not supported for waveform input '%s'.",
			wvin_record->name );
	}

	if ( ((long) channel_index) >= wvin->maximum_num_channels ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Channel index %lu for waveform input '%s' is outside "
		"the allowed range of 0-%ld.",
			channel_index, wvin_record->name,
			wvin->maximum_num_channels - 1L );
	}

	wvin->channel_index = (long) channel_index;

	mx_status = (*read_channel_fn)( wvin );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	wvin->channel_data = (wvin->data_array) [ wvin->channel_index ];

	if ( num_steps != NULL ) {
		*num_steps = wvin->current_num_steps;
	}
	if ( channel_data != NULL ) {
		*channel_data = wvin->channel_data;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_waveform_input_get_frequency( MX_RECORD *wvin_record,
					double *frequency )
{
	static const char fname[] = "mx_waveform_input_get_frequency()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_waveform_input_default_get_parameter_handler;
	}

	wvin->parameter_type = MXLV_WVI_FREQUENCY;

	mx_status = (*get_parameter_fn)( wvin );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( frequency != NULL ) {
		*frequency = wvin->frequency;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_waveform_input_set_frequency( MX_RECORD *wvin_record,
					double frequency )
{
	static const char fname[] = "mx_waveform_input_set_frequency()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_waveform_input_default_set_parameter_handler;
	}

	wvin->frequency = frequency;

	wvin->parameter_type = MXLV_WVI_FREQUENCY;

	mx_status = (*set_parameter_fn)( wvin );

	return mx_status;
}

/*--------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_waveform_input_get_trigger_mode( MX_RECORD *wvin_record,
					long *trigger_mode )
{
	static const char fname[] = "mx_waveform_input_get_trigger_mode()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_waveform_input_default_get_parameter_handler;
	}

	wvin->parameter_type = MXLV_WVI_TRIGGER_MODE;

	mx_status = (*get_parameter_fn)( wvin );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( trigger_mode != NULL ) {
		*trigger_mode = wvin->trigger_mode;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_waveform_input_set_trigger_mode( MX_RECORD *wvin_record,
					long trigger_mode )
{
	static const char fname[] = "mx_waveform_input_set_trigger_mode()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_waveform_input_default_set_parameter_handler;
	}

	wvin->trigger_mode = trigger_mode;

	wvin->parameter_type = MXLV_WVI_TRIGGER_MODE;

	mx_status = (*set_parameter_fn)( wvin );

	return mx_status;
}

/*--------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_waveform_input_get_trigger_repeat( MX_RECORD *wvin_record,
					long *trigger_repeat )
{
	static const char fname[] = "mx_waveform_input_get_trigger_repeat()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_waveform_input_default_get_parameter_handler;
	}

	wvin->parameter_type = MXLV_WVI_TRIGGER_REPEAT;

	mx_status = (*get_parameter_fn)( wvin );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( trigger_repeat != NULL ) {
		*trigger_repeat = wvin->trigger_repeat;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_waveform_input_set_trigger_repeat( MX_RECORD *wvin_record,
					long trigger_repeat )
{
	static const char fname[] = "mx_waveform_input_set_trigger_repeat()";

	MX_WAVEFORM_INPUT *wvin;
	MX_WAVEFORM_INPUT_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_WAVEFORM_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_waveform_input_get_pointers( wvin_record,
					&wvin, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_waveform_input_default_set_parameter_handler;
	}

	wvin->trigger_repeat = trigger_repeat;

	wvin->parameter_type = MXLV_WVI_TRIGGER_REPEAT;

	mx_status = (*set_parameter_fn)( wvin );

	return mx_status;
}

/*--------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_waveform_input_default_get_parameter_handler( MX_WAVEFORM_INPUT *wvin )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_waveform_input_default_set_parameter_handler( MX_WAVEFORM_INPUT *wvin )
{
	return MX_SUCCESSFUL_RESULT;
}

