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
			const char *calling_fname )
{
	static const char fname[] = "mxd_bkprecision_912x_wvout_get_pointers()";

	if ( wvout == (MX_WAVEFORM_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_WAVEFORM_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( bkprecision_912x_wvout == (MX_BKPRECISION_912X_WVOUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_BKPRECISION_912X_WVOUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( wvout->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_WAVEFORM_OUTPUT structure "
		"passed by '%s' is NULL.",
			calling_fname );
	}

	*bkprecision_912x_wvout = (MX_BKPRECISION_912X_WVOUT *)
					wvout->record->record_type_struct;

	if ( *bkprecision_912x_wvout == (MX_BKPRECISION_912X_WVOUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_BKPRECISION_912X_WVOUT pointer for record '%s' is NULL.",
			wvout->record->name );
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
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	wvout = (MX_WAVEFORM_OUTPUT *) record->record_class_struct;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_WVOUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

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
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout, fname );

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
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout, fname );

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
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout, fname );

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
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout, fname );

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
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout, fname );

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
	mx_status_type mx_status;

	/* Read the data from the FIFO. */

	mx_status = mxd_bkprecision_912x_wvout_read_all( wvout );

	/* Since wvout->data_array has the channel number as its leading
	 * array index, the data is already laid out in wvout->data_array
	 * in such a way that it can be passed back by the function
	 * mx_waveform_output_read_channel() to the application program
	 * by merely passing back a pointer to
	 * wvout->data_array[channel_number][0].
	 *
	 * Thus, we do not need to copy any data, since it is already laid
	 * out in memory the way we want it to be.
	 */

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_wvout_get_parameter( MX_WAVEFORM_OUTPUT *wvout )
{
	static const char fname[] =
			"mxd_bkprecision_912x_wvout_get_parameter()";

	MX_BKPRECISION_912X_WVOUT *bkprecision_912x_wvout;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout, fname );

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
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_wvout_get_pointers( wvout,
					&bkprecision_912x_wvout, fname );

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

