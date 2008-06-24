/*
 * Name:    d_bkprecision_912x_wvout.c
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

#define MXD_BKPRECISION_912X_WVOUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_waveform_output.h"
#include "i_bkprecision_912x.h"
#include "d_bkprecision_912x_wvout.h"

#if MXD_BKPRECISION_912X_WVOUT_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_wvout_record_function_list = {
	mxd_bkprecision_912x_wvout_initialize_type,
	mxd_bkprecision_912x_wvout_create_record_structures,
	mxd_bkprecision_912x_wvout_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bkprecision_912x_wvout_open
};

MX_WAVEFORM_OUTPUT_FUNCTION_LIST
		mxd_bkprecision_912x_wvout_wvout_function_list =
{
	mxd_bkprecision_912x_wvout_arm,
	mxd_bkprecision_912x_wvout_trigger,
	mxd_bkprecision_912x_wvout_stop,
	mxd_bkprecision_912x_wvout_busy,
	mxd_bkprecision_912x_wvout_read_all,
	NULL,
	mxd_bkprecision_912x_wvout_read_channel,
	NULL,
};

MX_RECORD_FIELD_DEFAULTS mxd_bkprecision_912x_wvout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_WAVEFORM_OUTPUT_STANDARD_FIELDS,
	MXD_BKPRECISION_912X_WVOUT_STANDARD_FIELDS
};

long mxd_bkprecision_912x_wvout_num_record_fields
		= sizeof( mxd_bkprecision_912x_wvout_record_field_defaults )
		/ sizeof( mxd_bkprecision_912x_wvout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_wvout_rfield_def_ptr
			= &mxd_bkprecision_912x_wvout_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_bkprecision_912x_wvout_get_pointers( MX_WAVEFORM_OUTPUT *wvout,
			MX_BKPRECISION_912X_WVOUT **bkprecision_912x_wvout,
			MX_BKPRECISION_912X **bkprecision_912x,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_get_pointers()";

	MX_RECORD *bkprecision_912x_record;
	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout_ptr;

	if ( wvout == (MX_WAVEFORM_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_WAVEFORM_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bkprecision_912x_wvout_ptr = (MX_BKPRECISION_912X_WVOUT *)
					wvout->record->record_type_struct;

	if (bkprecision_912x_wvout_ptr == (MX_BKPRECISION_912X_WVOUT *) NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BKPRECISION_912X_WVOUT pointer for record '%s' passed "
		"by '%s' is NULL.",
			wvout->record->name, calling_fname );
	}

	if ( bkprecision_912x_wvout != (MX_BKPRECISION_912X_WVOUT **) NULL ) {
		*bkprecision_912x_wvout = bkprecision_912x_wvout_ptr;
	}

	if ( bkprecision_912x != (MX_BKPRECISION_912X **) NULL ) {
		bkprecision_912x_record =
			bkprecision_912x_wvout_ptr->bkprecision_912x_record;

		if ( bkprecision_912x_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_BKPRECISION_912X pointer for analog "
			"input record '%s' passed by '%s' is NULL.",
				wvout->record->name, calling_fname );
		}

		*bkprecision_912x = (MX_BKPRECISION_912X *)
			bkprecision_912x_record->record_type_struct;

		if ( *bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BKPRECISION_912X pointer for BK Precision "
			"power supply '%s' used by analog input record '%s'.",
				bkprecision_912x_record->name,
				wvout->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_channels_varargs_cookie;
	long maximum_num_points_varargs_cookie;
	mx_status_type status;

	status = mx_waveform_output_initialize_type( record_type,
				&num_record_fields,
				&record_field_defaults,
				&maximum_num_channels_varargs_cookie,
				&maximum_num_points_varargs_cookie );

	return status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_bkprecision_912x_wvout_create_record_structures()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;

	wvout = (MX_WAVEFORM_OUTPUT *) malloc( sizeof(MX_WAVEFORM_OUTPUT) );

	if ( wvout == (MX_WAVEFORM_OUTPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_WAVEFORM_OUTPUT structure." );
	}

	bkprecision_912x_wvout = (MX_BKPRECISION_912X_WVOUT *)
				malloc( sizeof(MX_BKPRECISION_912X_WVOUT) );

	if ( bkprecision_912x_wvout == (MX_BKPRECISION_912X_WVOUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_BKPRECISION_912X_WVOUT structure." );
	}

	record->record_class_struct = wvout;
	record->record_type_struct = bkprecision_912x_wvout;

	record->class_specific_function_list =
				&mxd_bkprecision_912x_wvout_wvout_function_list;

	wvout->record = record;
	bkprecision_912x_wvout->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_waveform_output_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_open()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	MX_BKPRECISION_912X *bkprecision_912x;
	int num_groups;
	char command[40];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	wvout = (MX_WAVEFORM_OUTPUT *) record->record_class_struct;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

	MX_DEBUG(-2,("%s: maximum_num_points = %lu",
		fname, wvout->maximum_num_points));
#endif

	if ( wvout->maximum_num_points <= 25 ) {
		num_groups = 8;
	} else
	if ( wvout->maximum_num_points <= 50 ) {
		num_groups = 4;
	} else
	if ( wvout->maximum_num_points <= 100 ) {
		num_groups = 2;
	} else
	if ( wvout->maximum_num_points <= 200 ) {
		num_groups = 1;
	} else {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested maximum number of points (%lu) for waveform "
		"output '%s' is not in the allowed range of 2 to 200.",
			wvout->maximum_num_points, record->name );
	}

	snprintf( command, sizeof(command), "LIST:AREA %d", num_groups );

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x, command,
				NULL, 0, MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"LIST:COUNT %lu", wvout->maximum_num_points );

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x, command,
				NULL, 0, MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_arm( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_arm()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	MX_BKPRECISION_912X *bkprecision_912x;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_trigger( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_trigger()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	MX_BKPRECISION_912X *bkprecision_912x;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_stop( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_stop()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	MX_BKPRECISION_912X *bkprecision_912x;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_busy( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_busy()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	MX_BKPRECISION_912X *bkprecision_912x;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_read_all( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_read_all()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	MX_BKPRECISION_912X *bkprecision_912x;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* First, read channel 0 (voltage). */

	wvout->channel_index = 0;

	mx_status = mxd_bkprecision_912x_wvout_read_channel( wvout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Then, read channel 1 (current). */

	wvout->channel_index = 1;

	mx_status = mxd_bkprecision_912x_wvout_read_channel( wvout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_read_channel( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_read_channel()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	MX_BKPRECISION_912X *bkprecision_912x;
	int ch, n, num_items;
	char command[40];
	char response[80];
	double value;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, wvout->record->name));
#endif

	/* Repoint 'channel_data' to the correct row in 'data_array'. */

	ch = wvout->channel_index;

	wvout->channel_data = (wvout->data_array)[ch];

	/* Loop over all of the points in this channel. */

	for ( n = 0; n < wvout->current_num_points; n++ ) {
		if ( ch == 0 ) {
			snprintf( command, sizeof(command),
			"LIST:VOLTAGE? %d", n+1 );
		} else {
			snprintf( command, sizeof(command),
			"LIST:CURRENT? %d", n+1 );
		}

		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, response, sizeof(response),
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response '%s' to the command '%s' for record '%s' "
			"does not contain a numerical value.",
				response, command, wvout->record->name );
		}

		wvout->channel_data[n] = value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_get_parameter( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] =
			"mxd_bkprecision_912x_wvout_get_parameter()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	MX_BKPRECISION_912X *bkprecision_912x;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,
	("%s invoked for waveform output '%s', parameter type '%s' (%ld)",
		fname, wvout->record->name,
		mx_get_field_label_string(wvout->record, wvout->parameter_type),
		wvout->parameter_type));
#endif

	switch( wvout->parameter_type ) {
	default:
		return mx_waveform_output_default_get_parameter_handler(wvout);
	}

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_set_parameter( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] =
			"mxd_bkprecision_912x_wvout_set_parameter()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	MX_BKPRECISION_912X *bkprecision_912x;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,
	("%s invoked for waveform output '%s', parameter type '%s' (%ld)",
		fname, wvout->record->name,
		mx_get_field_label_string(wvout->record, wvout->parameter_type),
		wvout->parameter_type));
#endif

	switch( wvout->parameter_type ) {
	default:
		return mx_waveform_output_default_set_parameter_handler(wvout);
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

