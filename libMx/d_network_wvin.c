/*
 * Name:    d_network_wvin.c
 *
 * Purpose: MX network waveform input driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_WVIN_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_waveform_input.h"
#include "d_network_wvin.h"

/* FIXME: mx_image.h is needed in order to get the trigger macros! */

#include "mx_image.h"

MX_RECORD_FUNCTION_LIST mxd_network_wvin_record_function_list = {
	mxd_network_wvin_initialize_driver,
	mxd_network_wvin_create_record_structures,
	mxd_network_wvin_finish_record_initialization
};

MX_WAVEFORM_INPUT_FUNCTION_LIST
		mxd_network_wvin_wvin_function_list =
{
	mxd_network_wvin_arm,
	mxd_network_wvin_trigger,
	mxd_network_wvin_stop,
	mxd_network_wvin_busy,
	NULL,
	mxd_network_wvin_read_channel,
	mxd_network_wvin_get_parameter,
	mxd_network_wvin_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_network_wvin_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_WAVEFORM_INPUT_STANDARD_FIELDS,
	MXD_NETWORK_WVIN_STANDARD_FIELDS
};

long mxd_network_wvin_num_record_fields
		= sizeof( mxd_network_wvin_record_field_defaults )
		/ sizeof( mxd_network_wvin_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_wvin_rfield_def_ptr
			= &mxd_network_wvin_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_network_wvin_get_pointers( MX_WAVEFORM_INPUT *wvin,
			MX_NETWORK_WVIN **network_wvin,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_wvin_get_pointers()";

	MX_NETWORK_WVIN *network_wvin_ptr;

	if ( wvin == (MX_WAVEFORM_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_WAVEFORM_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	network_wvin_ptr = (MX_NETWORK_WVIN *)
					wvin->record->record_type_struct;

	if (network_wvin_ptr == (MX_NETWORK_WVIN *) NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_WVIN pointer for record '%s' passed "
		"by '%s' is NULL.",
			wvin->record->name, calling_fname );
	}

	if ( network_wvin != (MX_NETWORK_WVIN **) NULL ) {
		*network_wvin = network_wvin_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_network_wvin_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_channels_varargs_cookie;
	long maximum_num_steps_varargs_cookie;
	mx_status_type status;

	status = mx_waveform_input_initialize_driver( driver,
					&maximum_num_channels_varargs_cookie,
					&maximum_num_steps_varargs_cookie );

	return status;
}

MX_EXPORT mx_status_type
mxd_network_wvin_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_wvin_create_record_structures()";

	MX_WAVEFORM_INPUT *wvin;
	MX_NETWORK_WVIN *network_wvin = NULL;

	wvin = (MX_WAVEFORM_INPUT *) malloc( sizeof(MX_WAVEFORM_INPUT) );

	if ( wvin == (MX_WAVEFORM_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_WAVEFORM_INPUT structure." );
	}

	network_wvin = (MX_NETWORK_WVIN *)
				malloc( sizeof(MX_NETWORK_WVIN) );

	if ( network_wvin == (MX_NETWORK_WVIN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_NETWORK_WVIN structure." );
	}

	record->record_class_struct = wvin;
	record->record_type_struct = network_wvin;

	record->class_specific_function_list =
				&mxd_network_wvin_wvin_function_list;

	wvin->record = record;
	network_wvin->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_wvin_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_network_wvin_finish_record_initialization()";

	MX_WAVEFORM_INPUT *wvin;
	MX_NETWORK_WVIN *network_wvin = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	wvin = record->record_class_struct;

	mx_status = mxd_network_wvin_get_pointers( wvin,
					&network_wvin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvin->record->name));
#endif

	mx_status = mx_waveform_input_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_wvin->arm_nf),
		network_wvin->server_record,
		"%s.arm", network_wvin->remote_record_name );

	mx_network_field_init( &(network_wvin->busy_nf),
		network_wvin->server_record,
		"%s.busy", network_wvin->remote_record_name );

	mx_network_field_init( &(network_wvin->current_num_steps_nf),
		network_wvin->server_record,
		"%s.current_num_steps", network_wvin->remote_record_name );

	mx_network_field_init( &(network_wvin->channel_data_nf),
		network_wvin->server_record,
		"%s.channel_data", network_wvin->remote_record_name );

	mx_network_field_init( &(network_wvin->channel_index_nf),
		network_wvin->server_record,
		"%s.channel_index", network_wvin->remote_record_name );

	mx_network_field_init( &(network_wvin->frequency_nf),
		network_wvin->server_record,
		"%s.frequency", network_wvin->remote_record_name );

	mx_network_field_init( &(network_wvin->trigger_repeat_nf),
		network_wvin->server_record,
		"%s.trigger_repeat", network_wvin->remote_record_name );

	mx_network_field_init( &(network_wvin->stop_nf),
		network_wvin->server_record,
		"%s.stop", network_wvin->remote_record_name );

	mx_network_field_init( &(network_wvin->trigger_nf),
		network_wvin->server_record,
		"%s.trigger", network_wvin->remote_record_name );

	mx_network_field_init( &(network_wvin->trigger_mode_nf),
		network_wvin->server_record,
		"%s.trigger_mode", network_wvin->remote_record_name );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_network_wvin_arm( MX_WAVEFORM_INPUT *wvin )
{
	static const char fname[] = "mxd_network_wvin_arm()";

	MX_NETWORK_WVIN *network_wvin = NULL;
	mx_status_type mx_status;

	mx_status = mxd_network_wvin_get_pointers( wvin,
					&network_wvin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvin->record->name));
#endif
	wvin->arm = TRUE;

	mx_status = mx_put( &(network_wvin->arm_nf),
				MXFT_BOOL, &(wvin->arm) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvin_trigger( MX_WAVEFORM_INPUT *wvin )
{
	static const char fname[] = "mxd_network_wvin_trigger()";

	MX_NETWORK_WVIN *network_wvin = NULL;
	mx_status_type mx_status;

	mx_status = mxd_network_wvin_get_pointers( wvin,
					&network_wvin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvin->record->name));
#endif
	wvin->trigger = TRUE;

	mx_status = mx_put( &(network_wvin->trigger_nf),
				MXFT_BOOL, &(wvin->trigger) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvin_stop( MX_WAVEFORM_INPUT *wvin )
{
	static const char fname[] = "mxd_network_wvin_stop()";

	MX_NETWORK_WVIN *network_wvin = NULL;
	mx_status_type mx_status;

	mx_status = mxd_network_wvin_get_pointers( wvin,
					&network_wvin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvin->record->name));
#endif
	wvin->stop = TRUE;

	mx_status = mx_put( &(network_wvin->stop_nf),
				MXFT_BOOL, &(wvin->stop) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvin_busy( MX_WAVEFORM_INPUT *wvin )
{
	static const char fname[] = "mxd_network_wvin_busy()";

	MX_NETWORK_WVIN *network_wvin = NULL;
	mx_status_type mx_status;

	mx_status = mxd_network_wvin_get_pointers( wvin,
					&network_wvin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvin->record->name));
#endif
	mx_status = mx_get( &(network_wvin->busy_nf),
				MXFT_BOOL, &(wvin->busy) );

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s: busy = %d", fname, (int) wvin->busy));
#endif
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvin_read_channel( MX_WAVEFORM_INPUT *wvin )
{
	static const char fname[] = "mxd_network_wvin_read_channel()";

	MX_NETWORK_WVIN *network_wvin = NULL;
	long dimension_array[1];
	long ch;
	mx_status_type mx_status;

	mx_status = mxd_network_wvin_get_pointers( wvin,
					&network_wvin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvin->record->name));
#endif

	/* Repoint 'channel_data' to the correct row in 'data_array'. */

	ch = (long) wvin->channel_index;

	wvin->channel_data = (wvin->data_array)[ch];

	mx_status = mx_put( &(network_wvin->channel_index_nf),
				MXFT_LONG, &(wvin->channel_index) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the value of current_num_steps. */

	mx_status = mx_get( &(network_wvin->current_num_steps_nf),
				MXFT_LONG, &(wvin->current_num_steps) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the channel data itself. */

	dimension_array[0] = wvin->maximum_num_steps;

	mx_status = mx_get_array( &(network_wvin->channel_data_nf),
				MXFT_DOUBLE, 1, dimension_array,
				wvin->channel_data );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvin_write_channel( MX_WAVEFORM_INPUT *wvin )
{
	static const char fname[] = "mxd_network_wvin_write_channel()";

	MX_NETWORK_WVIN *network_wvin = NULL;
	long dimension_array[1];
	long ch;
	mx_status_type mx_status;

	mx_status = mxd_network_wvin_get_pointers( wvin,
					&network_wvin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvin->record->name));
#endif

	/* Repoint 'channel_data' to the correct row in 'data_array'. */

	ch = (long) wvin->channel_index;

	wvin->channel_data = (wvin->data_array)[ch];

	mx_status = mx_put( &(network_wvin->channel_index_nf),
				MXFT_LONG, &(wvin->channel_index) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Write the channel data itself. */

	dimension_array[0] = wvin->maximum_num_steps;

	mx_status = mx_put_array( &(network_wvin->channel_data_nf),
				MXFT_DOUBLE, 1, dimension_array,
				wvin->channel_data );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvin_get_parameter( MX_WAVEFORM_INPUT *wvin )
{
	static const char fname[] =
			"mxd_network_wvin_get_parameter()";

	MX_NETWORK_WVIN *network_wvin = NULL;
	mx_status_type mx_status;

	mx_status = mxd_network_wvin_get_pointers( wvin,
					&network_wvin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,
	("%s invoked for waveform output '%s', parameter type '%s' (%ld)",
		fname, wvin->record->name,
		mx_get_field_label_string(wvin->record, wvin->parameter_type),
		wvin->parameter_type));
#endif

	switch( wvin->parameter_type ) {
	case MXLV_WVI_FREQUENCY:
		mx_status = mx_get( &(network_wvin->frequency_nf),
				MXFT_DOUBLE, &(wvin->frequency) );
		break;

	case MXLV_WVI_TRIGGER_REPEAT:
		mx_status = mx_get( &(network_wvin->trigger_repeat_nf),
				MXFT_LONG, &(wvin->trigger_repeat) );
		break;

	case MXLV_WVI_TRIGGER_MODE:
		mx_status = mx_get( &(network_wvin->trigger_mode_nf),
				MXFT_LONG, &(wvin->trigger_mode) );
		break;

	default:
		return mx_waveform_input_default_get_parameter_handler(wvin);
	}

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvin_set_parameter( MX_WAVEFORM_INPUT *wvin )
{
	static const char fname[] =
			"mxd_network_wvin_set_parameter()";

	MX_NETWORK_WVIN *network_wvin = NULL;
	mx_status_type mx_status;

	mx_status = mxd_network_wvin_get_pointers( wvin,
					&network_wvin, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,
	("%s invoked for waveform output '%s', parameter type '%s' (%ld)",
		fname, wvin->record->name,
		mx_get_field_label_string(wvin->record, wvin->parameter_type),
		wvin->parameter_type));
#endif

	switch( wvin->parameter_type ) {
	case MXLV_WVI_FREQUENCY:
		mx_status = mx_put( &(network_wvin->frequency_nf),
				MXFT_DOUBLE, &(wvin->frequency) );
		break;

	case MXLV_WVI_TRIGGER_REPEAT:
		mx_status = mx_put( &(network_wvin->trigger_repeat_nf),
				MXFT_LONG, &(wvin->trigger_repeat) );
		break;

	case MXLV_WVI_TRIGGER_MODE:
		mx_status = mx_put( &(network_wvin->trigger_mode_nf),
				MXFT_LONG, &(wvin->trigger_mode) );
		break;

	default:
		return mx_waveform_input_default_set_parameter_handler(wvin);
	}

#if MXD_NETWORK_WVIN_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

