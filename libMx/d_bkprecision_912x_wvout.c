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
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_waveform_output.h"
#include "i_bkprecision_912x.h"
#include "d_bkprecision_912x_wvout.h"

/* FIXME: mx_image.h is needed in order to get the trigger macros! */

#include "mx_image.h"

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
	NULL,
	mxd_bkprecision_912x_wvout_wr_all,
	mxd_bkprecision_912x_wvout_read_channel,
	mxd_bkprecision_912x_wvout_write_channel,
	mxd_bkprecision_912x_wvout_get_parameter,
	mxd_bkprecision_912x_wvout_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_bkprecision_912x_wvout_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_WAVEFORM_OUTPUT_STANDARD_FIELDS,
	MXD_BKPRECISION_912X_WVOUT_STANDARD_FIELDS
};

long mxd_bkprecision_912x_wvout_num_record_fields
		= sizeof( mxd_bkprecision_912x_wvout_rf_defaults )
		/ sizeof( mxd_bkprecision_912x_wvout_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_wvout_rfield_def_ptr
			= &mxd_bkprecision_912x_wvout_rf_defaults[0];

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
			"MX_BKPRECISION_912X pointer for waveform output "
			"record '%s' passed by '%s' is NULL.",
				wvout->record->name, calling_fname );
		}

		*bkprecision_912x = (MX_BKPRECISION_912X *)
			bkprecision_912x_record->record_type_struct;

		if ( *bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BKPRECISION_912X pointer for BK Precision "
			"power supply '%s' used by waveform output "
			"record '%s'.",
				bkprecision_912x_record->name,
				wvout->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/* New values for the LISTs are only useable after 'LIST:NAME' and 'LIST:SAVE'
 * are invoked.  If you do not do the save, then LIST mode will continue to
 * use the old list.
 */

static mx_status_type
mxd_bkprecision_912x_wvout_save_list( MX_BKPRECISION_912X *bkprecision_912x )
{
	mx_status_type mx_status;

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
				"LIST:NAME 'MX'", NULL, 0,
				MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
				"LIST:SAVE 1", NULL, 0,
				MXD_BKPRECISION_912X_WVOUT_DEBUG );

	return mx_status;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_initialize_type( long record_type )
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
mxd_bkprecision_912x_wvout_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_bkprecision_912x_wvout_create_record_structures()";

	MX_WAVEFORM_OUTPUT *wvout;
	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;

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
	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
	int num_groups;
	char command[40];
	char response[40];
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
#endif

	bkprecision_912x_wvout->invoked_by_write_all = FALSE;

	if ( wvout->maximum_num_steps <= 25 ) {
		num_groups = 8;
	} else
	if ( wvout->maximum_num_steps <= 50 ) {
		num_groups = 4;
	} else
	if ( wvout->maximum_num_steps <= 100 ) {
		num_groups = 2;
	} else
	if ( wvout->maximum_num_steps <= 200 ) {
		num_groups = 1;
	} else {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested maximum number of steps (%lu) for waveform "
		"output '%s' is not in the allowed range of 2 to 200.",
			wvout->maximum_num_steps, record->name );
	}

	snprintf( command, sizeof(command), "LIST:AREA %d", num_groups );

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x, command,
				NULL, 0, MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"LIST:COUNT %lu", wvout->maximum_num_steps );

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x, command,
				NULL, 0, MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out the initial list mode to use. */

	if ( mx_strncasecmp( "CONTINUOUS",
		bkprecision_912x_wvout->list_mode_name,
		strlen(bkprecision_912x_wvout->list_mode_name) ) == 0 )
	{
		bkprecision_912x_wvout->list_mode =
					MXF_BKPRECISION_912X_WVOUT_CONTINUOUS;
	} else
	if ( mx_strncasecmp( "STEP", bkprecision_912x_wvout->list_mode_name,
		strlen(bkprecision_912x_wvout->list_mode_name) ) == 0 )
	{
		bkprecision_912x_wvout->list_mode =
					MXF_BKPRECISION_912X_WVOUT_STEP;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized list mode '%s' specified for "
		"waveform output '%s'.",
			bkprecision_912x_wvout->list_mode_name,
			record->name );
	}

	/* Figure out what the list units are. */

	if ( mx_strncasecmp( "SECOND", bkprecision_912x_wvout->list_unit_name,
		strlen(bkprecision_912x_wvout->list_unit_name) ) == 0 )
	{
		bkprecision_912x_wvout->list_unit =
					MXF_BKPRECISION_912X_WVOUT_SECOND;

		strlcpy( command, "LIST:UNIT SECOND", sizeof(command) );
	} else
	if ( mx_strncasecmp( "MSECOND", bkprecision_912x_wvout->list_unit_name,
		strlen(bkprecision_912x_wvout->list_unit_name) ) == 0 )
	{
		bkprecision_912x_wvout->list_unit =
					MXF_BKPRECISION_912X_WVOUT_MSECOND;

		strlcpy( command, "LIST:UNIT MSECOND", sizeof(command) );
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized list unit '%s' specified for "
		"waveform output '%s'.",
			bkprecision_912x_wvout->list_unit_name,
			record->name );
	}

	/* Reprogram the list units. */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x, command,
				NULL, 0, MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out whether or not the list is in one-shot mode. */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
				"LIST:STEP?", response, sizeof(response),
				MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "ONCE" ) == 0 ) {
		wvout->trigger_repeat = MXF_WVO_ONE_SHOT;
	} else
	if ( strcmp( response, "REPEAT" ) == 0 ) {
		wvout->trigger_repeat = MXF_WVO_FOREVER;
	} else {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"LIST:STEP for '%s' is not ONCE or REPEAT.  "
		"Instead, it is '%s'.", wvout->record->name, response );
	}

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_arm( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_arm()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
	char command[40];
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
	/* Switch to trigger mode. */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"PORT:FUNCTION TRIGGER", NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the list mode. */

	switch( bkprecision_912x_wvout->list_mode ) {
	case MXF_BKPRECISION_912X_WVOUT_CONTINUOUS:
		/* For some reason, specifying the full name CONTINUOUS
		 * causes the command to fail with error code 40.
		 */

		strlcpy( command, "LIST:MODE CONT", sizeof(command) );
		break;

	case MXF_BKPRECISION_912X_WVOUT_STEP:
		strlcpy( command, "LIST:MODE STEP", sizeof(command) );
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized list mode number %lu for waveform output '%s'",
			bkprecision_912x_wvout->list_mode, wvout->record->name);
		break;
	}

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the list step. */

	if ( wvout->trigger_repeat == MXF_WVO_ONE_SHOT ) {
		strlcpy( command, "LIST:STEP ONCE", sizeof(command) );
	} else {
		strlcpy( command, "LIST:STEP REPEAT", sizeof(command) );
	}

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Finish by switching the power supply to list mode. */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"MODE LIST", NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_trigger( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_trigger()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
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

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"TRIGGER", NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_stop( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_stop()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
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

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"MODE FIX", NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_busy( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_busy()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
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
mxd_bkprecision_912x_wvout_wr_all( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_wr_all()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
	MX_WAVEFORM_OUTPUT_FUNCTION_LIST *function_list;
	unsigned long ch;
	mx_status_type (*write_channel_fn)( MX_WAVEFORM_OUTPUT * );
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

	function_list = wvout->record->class_specific_function_list;

	if ( function_list == (MX_WAVEFORM_OUTPUT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WAVEFORM_OUTPUT_FUNCTION_LIST pointer for "
		"record '%s' is NULL.", wvout->record->name );
	}

	write_channel_fn = function_list->write_channel;

	if ( write_channel_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The write_channel function for record '%s' is NULL.",
			wvout->record->name );
	}

	bkprecision_912x_wvout->invoked_by_write_all = TRUE;

	for ( ch = 0; ch < wvout->current_num_channels; ch++ ) {
		wvout->channel_index = ch;

		mx_status = (*write_channel_fn)( wvout );

		if ( mx_status.code != MXE_SUCCESS ) {
			break;	/* Exit the for() loop, but do not return. */
		}
	}

	/* Using 'break' in the for() loop above guarantees that the
	 * 'invoked_by_write_all' flag will be turned off below.
	 */

	bkprecision_912x_wvout->invoked_by_write_all = FALSE;

	/* Tell the power supply to save the new list. */

	mx_status = mxd_bkprecision_912x_wvout_save_list( bkprecision_912x );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_read_channel( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_read_channel()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
	int ch, n, num_items;
	char command[40];
	char response[80];
	double raw_value;
	unsigned long list_unit;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ch = wvout->channel_index;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', channel %d.",
		fname, wvout->record->name, ch));
#endif

	/* Repoint 'channel_data' to the correct row in 'data_array'. */

	wvout->channel_data = (wvout->data_array)[ch];

	/* Loop over all of the steps in this channel. */

	for ( n = 0; n < wvout->current_num_steps; n++ ) {
		switch( ch ) {
		case MXF_BKPRECISION_912X_WVOUT_VOLTAGE:
			snprintf( command, sizeof(command),
				"LIST:VOLTAGE? %d", n+1 );
			break;

		case MXF_BKPRECISION_912X_WVOUT_CURRENT:
			snprintf( command, sizeof(command),
				"LIST:CURRENT? %d", n+1 );
			break;

		case MXF_BKPRECISION_912X_WVOUT_WIDTH:
			snprintf( command, sizeof(command),
				"LIST:WIDTH? %d", n+1 );
			break;

		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal channel number (%d) specified for "
			"waveform output '%s'.  "
			"The allowed values are 0 (for voltage), "
			"1 (for current), and 2 (for step width).",
				ch, wvout->record->name );
			break;
		}

		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, response, sizeof(response),
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &raw_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The response '%s' to the command '%s' for record '%s' "
			"does not contain a numerical value.",
				response, command, wvout->record->name );
		}

		if ( ch == MXF_BKPRECISION_912X_WVOUT_WIDTH ) {

			list_unit = bkprecision_912x_wvout->list_unit;

			if ( list_unit == MXF_BKPRECISION_912X_WVOUT_MSECOND ) {
				/* Convert BK milliseconds to MX seconds. */

				raw_value = raw_value * 0.001;
			}
		}

		wvout->channel_data[n] =
			wvout->offset + wvout->scale * raw_value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_write_channel( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] =
			"mxd_bkprecision_912x_wvout_write_channel()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
	MX_RECORD *interface_record;
	int ch, n, i, num_attempts;
	char command[40];
	unsigned long list_unit;
	long width_value;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout,
					&bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ch = wvout->channel_index;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', channel %d.",
		fname, wvout->record->name, ch));
#endif

	/* Repoint 'channel_data' to the correct row in 'data_array'. */

	wvout->channel_data = (wvout->data_array)[ch];

	/* Loop over all of the steps in this channel. */

	num_attempts = 2;

	list_unit = bkprecision_912x_wvout->list_unit;

	for ( n = 0; n < wvout->current_num_steps; n++ ) {
		switch( ch ) {
		case MXF_BKPRECISION_912X_WVOUT_VOLTAGE:
			snprintf( command, sizeof(command),
			"LIST:VOLTAGE %d, %f",
				n+1, wvout->channel_data[n] );
			break;

		case MXF_BKPRECISION_912X_WVOUT_CURRENT:
			snprintf( command, sizeof(command),
			"LIST:CURRENT %d, %f",
				n+1, wvout->channel_data[n] );
			break;

		case MXF_BKPRECISION_912X_WVOUT_WIDTH:
			if ( list_unit == MXF_BKPRECISION_912X_WVOUT_MSECOND ) {
				/* Convert MX seconds to BK milliseconds. */

				width_value = mx_round( 1000.0 *
						wvout->channel_data[n] );
			} else {
				width_value = mx_round( wvout->channel_data[n]);
			}
				

			snprintf( command, sizeof(command),
				"LIST:WIDTH %d, %ld", n+1, width_value );
			break;

		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal channel number (%d) specified for "
			"waveform output '%s'.  "
			"The allowed values are 0 (for voltage), "
			"1 (for current), and 2 (for step width).",
				ch, wvout->record->name );
			break;
		}

		/* Sometimes this command fails with a 'Command Error'.
		 * If that happens, we retry the command as many times
		 * as indicated by the 'num_attempts' variable.
		 *
		 * FIXME: The reason for these failures has not yet
		 * been found.
		 */

		for ( i = 0; i < num_attempts; i++ ) {

		    if ( i > 0 ) {
			/* The first attempt failed, so we are retrying
			 * the command.
			 */

			/* Wait for any remaining input characters to arrive. */

			mx_msleep(1000);

			/* If we are connected via RS-232, then throw away
			 * the unread part of the input buffer.
			 */

			interface_record =
				    bkprecision_912x->port_interface.record;

			if ( interface_record->mx_class == MXI_RS232 ) {
			    mx_status = mx_rs232_discard_unread_input(
					interface_record, 
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

			    if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			}
		    }

		    /* Now send the command. */

		    mx_status = mxi_bkprecision_912x_command(
					bkprecision_912x, command, NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

		    /* We only retry if we got an MXE_INTERFACE_IO_ERROR
		     * with an Invalid Command error code (70).
		     */

		    if ( mx_status.code != MXE_INTERFACE_IO_ERROR )
			break;

		    if ( bkprecision_912x->error_code != 70 )
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* If we have not been invoked by the write_all function, then
	 * we must save the new list.  Otherwise, the new list will not
	 * be used.
	 */

	if ( bkprecision_912x_wvout->invoked_by_write_all == FALSE ) {
		mx_status = mxd_bkprecision_912x_wvout_save_list(
							bkprecision_912x );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_get_parameter( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] =
			"mxd_bkprecision_912x_wvout_get_parameter()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
	char command[40];
	char response[80];
	unsigned long list_unit;
	double step_time;
	int num_items;
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
	case MXLV_WVO_FREQUENCY:
		/* We just use the step width from the first step. */

		strlcpy( command, "LIST:WIDTH? 1", sizeof(command) );

		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, response, sizeof(response),
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &step_time );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No step time was seen in the response '%s' to "
			"the '%s' command for waveform output '%s'.",
				response, command, wvout->record->name );
		}

		list_unit = bkprecision_912x_wvout->list_unit;

		if ( list_unit == MXF_BKPRECISION_912X_WVOUT_MSECOND ) {
			/* Convert step time in millisec to frequency in Hz. */

			wvout->frequency = mx_divide_safely( 1000.0, step_time);
		} else {
			/* Convert step time in seconds to frequency in Hz. */

			wvout->frequency = mx_divide_safely( 1.0, step_time );
		}
		break;

	case MXLV_WVO_TRIGGER_MODE:
		strlcpy( command, "TRIGGER:SOURCE?", sizeof(command) );

		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, response, sizeof(response),
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* FIXME:
		 * The trigger mode macros should not come from mx_image.h.
		 */

		if ( strcmp( response, "BUS" ) == 0 ) {
			wvout->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
		} else
		if ( strcmp( response, "EXTERNAL" ) == 0 ) {
			wvout->trigger_mode = MXT_IMAGE_EXTERNAL_TRIGGER;
		} else
		if ( strcmp( response, "IMMEDIATE" ) == 0 ) {
			wvout->trigger_mode = MXT_IMAGE_MANUAL_TRIGGER;
		} else {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unexpected response '%s' seen for command '%s' "
			"sent to waveform output '%s'.",
				response, command, wvout->record->name );
		}
		break;

	case MXLV_WVO_TRIGGER_REPEAT:
		strlcpy( command, "LIST:STEP?", sizeof(command) );

		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, response, sizeof(response),
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( strcmp( response, "ONCE" ) == 0 ) {
			wvout->trigger_repeat = MXF_WVO_ONE_SHOT;
		} else
		if ( strcmp( response, "REPEAT" ) == 0 ) {
			wvout->trigger_repeat = MXF_WVO_FOREVER;
		} else {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unexpected response '%s' seen for command '%s' "
			"sent to waveform output '%s'.",
				response, command, wvout->record->name );
		}
		break;

	default:
		return mx_waveform_output_default_get_parameter_handler(wvout);
	}

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_set_parameter( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] =
			"mxd_bkprecision_912x_wvout_set_parameter()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout = NULL;
	MX_BKPRECISION_912X *bkprecision_912x = NULL;
	char command[40];
	unsigned long list_unit;
	double step_time;
	int n;
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
	case MXLV_WVO_FREQUENCY:
		list_unit = bkprecision_912x_wvout->list_unit;

		if ( list_unit == MXF_BKPRECISION_912X_WVOUT_MSECOND ) {
			/* Convert frequency in Hz to step time in millisec. */

			step_time = mx_divide_safely( 1000.0, wvout->frequency);
		} else {
			/* Convert frequency in Hz to step time in seconds. */

			step_time = mx_divide_safely( 1.0, wvout->frequency );
		}

		/* For this command, we set all of the steps in
		 * channel 2 (step width) for the waveform output.
		 */

		for ( n = 0; n < wvout->maximum_num_steps; n++ ) {
			snprintf(command, sizeof(command),
				"LIST:WIDTH %d, %ld", n+1,
				mx_round(step_time) );

			mx_status = mxi_bkprecision_912x_command(
					bkprecision_912x, command, NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;

	case MXLV_WVO_TRIGGER_MODE:
		/* FIXME:
		 * The trigger mode macros should not come from mx_image.h.
		 */

		switch( wvout->trigger_mode ) {
		case MXT_IMAGE_INTERNAL_TRIGGER:
			strlcpy( command, "TRIGGER:SOURCE BUS",
						sizeof(command) );
			break;
		case MXT_IMAGE_EXTERNAL_TRIGGER:
			strlcpy( command, "TRIGGER:SOURCE EXTERNAL",
						sizeof(command) );
			break;
		case MXT_IMAGE_MANUAL_TRIGGER:
			strlcpy( command, "TRIGGER:SOURCE IMMEDIATE",
						sizeof(command) );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized trigger mode %#lx was requested "
			"for waveform output '%s'.",
				wvout->trigger_mode, wvout->record->name );
			break;
		}

		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

		break;

	case MXLV_WVO_TRIGGER_REPEAT:
		MX_DEBUG(-2,("%s: wvout->trigger_repeat = %ld",
			fname, wvout->trigger_repeat));

		if ( wvout->trigger_repeat == MXF_WVO_ONE_SHOT ) {
			strlcpy( command, "LIST:STEP ONCE", sizeof(command) );
		} else {
			strlcpy( command, "LIST:STEP REPEAT", sizeof(command) );
		}

		mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, NULL, 0,
					MXD_BKPRECISION_912X_WVOUT_DEBUG );

		break;

	default:
		return mx_waveform_output_default_set_parameter_handler(wvout);
	}

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

