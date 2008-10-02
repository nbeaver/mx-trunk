/*
 * Name:    d_network_wvout.c
 *
 * Purpose: MX waveform output driver for the BK Precision 912x series
 *          of power supplies.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_WVOUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_waveform_output.h"
#include "d_network_wvout.h"

/* FIXME: mx_image.h is needed in order to get the trigger macros! */

#include "mx_image.h"

MX_RECORD_FUNCTION_LIST mxd_network_wvout_record_function_list = {
	mxd_network_wvout_initialize_type,
	mxd_network_wvout_create_record_structures,
	mxd_network_wvout_finish_record_initialization
};

MX_WAVEFORM_OUTPUT_FUNCTION_LIST
		mxd_network_wvout_wvout_function_list =
{
	mxd_network_wvout_arm,
	mxd_network_wvout_trigger,
	mxd_network_wvout_stop,
	mxd_network_wvout_busy,
	NULL,
	NULL,
	mxd_network_wvout_read_channel,
	mxd_network_wvout_write_channel,
	mxd_network_wvout_get_parameter,
	mxd_network_wvout_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_network_wvout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_WAVEFORM_OUTPUT_STANDARD_FIELDS,
	MXD_NETWORK_WVOUT_STANDARD_FIELDS
};

long mxd_network_wvout_num_record_fields
		= sizeof( mxd_network_wvout_record_field_defaults )
		/ sizeof( mxd_network_wvout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_wvout_rfield_def_ptr
			= &mxd_network_wvout_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_network_wvout_get_pointers( MX_WAVEFORM_OUTPUT *wvout,
			MX_NETWORK_WVOUT **network_wvout,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_wvout_get_pointers()";

	MX_NETWORK_WVOUT *network_wvout_ptr;

	if ( wvout == (MX_WAVEFORM_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_WAVEFORM_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	network_wvout_ptr = (MX_NETWORK_WVOUT *)
					wvout->record->record_type_struct;

	if (network_wvout_ptr == (MX_NETWORK_WVOUT *) NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_WVOUT pointer for record '%s' passed "
		"by '%s' is NULL.",
			wvout->record->name, calling_fname );
	}

	if ( network_wvout != (MX_NETWORK_WVOUT **) NULL ) {
		*network_wvout = network_wvout_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_network_wvout_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_channels_varargs_cookie;
	long maximum_num_steps_varargs_cookie;
	mx_status_type status;

	status = mx_waveform_output_initialize_type( record_type,
				&num_record_fields,
				&record_field_defaults,
				&maximum_num_channels_varargs_cookie,
				&maximum_num_steps_varargs_cookie );

	return status;
}

MX_EXPORT mx_status_type
mxd_network_wvout_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_wvout_create_record_structures()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_NETWORK_WVOUT *network_wvout;

	wvout = (MX_WAVEFORM_OUTPUT *) malloc( sizeof(MX_WAVEFORM_OUTPUT) );

	if ( wvout == (MX_WAVEFORM_OUTPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_WAVEFORM_OUTPUT structure." );
	}

	network_wvout = (MX_NETWORK_WVOUT *)
				malloc( sizeof(MX_NETWORK_WVOUT) );

	if ( network_wvout == (MX_NETWORK_WVOUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_NETWORK_WVOUT structure." );
	}

	record->record_class_struct = wvout;
	record->record_type_struct = network_wvout;

	record->class_specific_function_list =
				&mxd_network_wvout_wvout_function_list;

	wvout->record = record;
	network_wvout->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_wvout_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_network_wvout_finish_record_initialization()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_NETWORK_WVOUT *network_wvout;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	wvout = record->record_class_struct;

	mx_status = mxd_network_wvout_get_pointers( wvout,
					&network_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif

	mx_status = mx_waveform_output_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_wvout->arm_nf),
		network_wvout->server_record,
		"%s.arm", network_wvout->remote_record_name );

	mx_network_field_init( &(network_wvout->busy_nf),
		network_wvout->server_record,
		"%s.busy", network_wvout->remote_record_name );

	mx_network_field_init( &(network_wvout->current_num_steps_nf),
		network_wvout->server_record,
		"%s.current_num_steps", network_wvout->remote_record_name );

	mx_network_field_init( &(network_wvout->channel_data_nf),
		network_wvout->server_record,
		"%s.channel_data", network_wvout->remote_record_name );

	mx_network_field_init( &(network_wvout->channel_index_nf),
		network_wvout->server_record,
		"%s.channel_index", network_wvout->remote_record_name );

	mx_network_field_init( &(network_wvout->frequency_nf),
		network_wvout->server_record,
		"%s.frequency", network_wvout->remote_record_name );

	mx_network_field_init( &(network_wvout->trigger_repeat_nf),
		network_wvout->server_record,
		"%s.trigger_repeat", network_wvout->remote_record_name );

	mx_network_field_init( &(network_wvout->stop_nf),
		network_wvout->server_record,
		"%s.stop", network_wvout->remote_record_name );

	mx_network_field_init( &(network_wvout->trigger_nf),
		network_wvout->server_record,
		"%s.trigger", network_wvout->remote_record_name );

	mx_network_field_init( &(network_wvout->trigger_mode_nf),
		network_wvout->server_record,
		"%s.trigger_mode", network_wvout->remote_record_name );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_network_wvout_arm( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_network_wvout_arm()";

	MX_NETWORK_WVOUT *network_wvout;
	mx_status_type mx_status;

	mx_status = mxd_network_wvout_get_pointers( wvout,
					&network_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif
	wvout->arm = TRUE;

	mx_status = mx_put( &(network_wvout->arm_nf),
				MXFT_BOOL, &(wvout->arm) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvout_trigger( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_network_wvout_trigger()";

	MX_NETWORK_WVOUT *network_wvout;
	mx_status_type mx_status;

	mx_status = mxd_network_wvout_get_pointers( wvout,
					&network_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif
	wvout->trigger = TRUE;

	mx_status = mx_put( &(network_wvout->trigger_nf),
				MXFT_BOOL, &(wvout->trigger) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvout_stop( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_network_wvout_stop()";

	MX_NETWORK_WVOUT *network_wvout;
	mx_status_type mx_status;

	mx_status = mxd_network_wvout_get_pointers( wvout,
					&network_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif
	wvout->stop = TRUE;

	mx_status = mx_put( &(network_wvout->stop_nf),
				MXFT_BOOL, &(wvout->stop) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvout_busy( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_network_wvout_busy()";

	MX_NETWORK_WVOUT *network_wvout;
	mx_status_type mx_status;

	mx_status = mxd_network_wvout_get_pointers( wvout,
					&network_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif
	mx_status = mx_get( &(network_wvout->busy_nf),
				MXFT_BOOL, &(wvout->busy) );

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s: busy = %d", fname, (int) wvout->busy));
#endif
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvout_read_channel( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_network_wvout_read_channel()";

	MX_NETWORK_WVOUT *network_wvout;
	long dimension_array[1];
	long ch;
	mx_status_type mx_status;

	mx_status = mxd_network_wvout_get_pointers( wvout,
					&network_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif

	/* Repoint 'channel_data' to the correct row in 'data_array'. */

	ch = wvout->channel_index;

	wvout->channel_data = (wvout->data_array)[ch];

	mx_status = mx_put( &(network_wvout->channel_index_nf),
				MXFT_LONG, &(wvout->channel_index) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the value of current_num_steps. */

	mx_status = mx_get( &(network_wvout->current_num_steps_nf),
				MXFT_LONG, &(wvout->current_num_steps) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the channel data itself. */

	dimension_array[0] = wvout->maximum_num_steps;

	mx_status = mx_get_array( &(network_wvout->channel_data_nf),
				MXFT_DOUBLE, 1, dimension_array,
				wvout->channel_data );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvout_write_channel( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_network_wvout_write_channel()";

	MX_NETWORK_WVOUT *network_wvout;
	long dimension_array[1];
	long ch;
	mx_status_type mx_status;

	mx_status = mxd_network_wvout_get_pointers( wvout,
					&network_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif

	/* Repoint 'channel_data' to the correct row in 'data_array'. */

	ch = wvout->channel_index;

	wvout->channel_data = (wvout->data_array)[ch];

	mx_status = mx_put( &(network_wvout->channel_index_nf),
				MXFT_LONG, &(wvout->channel_index) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Write the channel data itself. */

	dimension_array[0] = wvout->maximum_num_steps;

	mx_status = mx_put_array( &(network_wvout->channel_data_nf),
				MXFT_DOUBLE, 1, dimension_array,
				wvout->channel_data );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvout_get_parameter( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] =
			"mxd_network_wvout_get_parameter()";

	MX_NETWORK_WVOUT *network_wvout;
	mx_status_type mx_status;

	mx_status = mxd_network_wvout_get_pointers( wvout,
					&network_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,
	("%s invoked for waveform output '%s', parameter type '%s' (%ld)",
		fname, wvout->record->name,
		mx_get_field_label_string(wvout->record, wvout->parameter_type),
		wvout->parameter_type));
#endif

	switch( wvout->parameter_type ) {
	case MXLV_WVO_FREQUENCY:
		mx_status = mx_get( &(network_wvout->frequency_nf),
				MXFT_DOUBLE, &(wvout->frequency) );
		break;

	case MXLV_WVO_TRIGGER_REPEAT:
		mx_status = mx_get( &(network_wvout->trigger_repeat_nf),
				MXFT_LONG, &(wvout->trigger_repeat) );
		break;

	case MXLV_WVO_TRIGGER_MODE:
		mx_status = mx_get( &(network_wvout->trigger_mode_nf),
				MXFT_LONG, &(wvout->trigger_mode) );
		break;

	default:
		return mx_waveform_output_default_get_parameter_handler(wvout);
	}

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_wvout_set_parameter( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] =
			"mxd_network_wvout_set_parameter()";

	MX_NETWORK_WVOUT *network_wvout;
	mx_status_type mx_status;

	mx_status = mxd_network_wvout_get_pointers( wvout,
					&network_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,
	("%s invoked for waveform output '%s', parameter type '%s' (%ld)",
		fname, wvout->record->name,
		mx_get_field_label_string(wvout->record, wvout->parameter_type),
		wvout->parameter_type));
#endif

	switch( wvout->parameter_type ) {
	case MXLV_WVO_FREQUENCY:
		mx_status = mx_put( &(network_wvout->frequency_nf),
				MXFT_DOUBLE, &(wvout->frequency) );
		break;

	case MXLV_WVO_TRIGGER_REPEAT:
		mx_status = mx_put( &(network_wvout->trigger_repeat_nf),
				MXFT_LONG, &(wvout->trigger_repeat) );
		break;

	case MXLV_WVO_TRIGGER_MODE:
		mx_status = mx_put( &(network_wvout->trigger_mode_nf),
				MXFT_LONG, &(wvout->trigger_mode) );
		break;

	default:
		return mx_waveform_output_default_set_parameter_handler(wvout);
	}

#if MXD_NETWORK_WVOUT_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

