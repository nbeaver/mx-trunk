/*
 * Name:    d_fg_scaler.c
 *
 * Purpose: MX scaler driver for a software emulated scaler that acts
 *          like a function generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_constants.h"
#include "mx_hrt.h"
#include "mx_scaler.h"
#include "d_fg_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_fg_scaler_record_function_list = {
	NULL,
	mxd_fg_scaler_create_record_structures,
	mx_scaler_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_fg_scaler_open
};

MX_SCALER_FUNCTION_LIST mxd_fg_scaler_scaler_function_list = {
	mxd_fg_scaler_clear,
	NULL,
	mxd_fg_scaler_read
};

/* Soft scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_fg_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_FG_SCALER_STANDARD_FIELDS
};

long mxd_fg_scaler_num_record_fields
		= sizeof( mxd_fg_scaler_record_field_defaults )
		  / sizeof( mxd_fg_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_fg_scaler_rfield_def_ptr
			= &mxd_fg_scaler_record_field_defaults[0];

/* === */

MX_EXPORT mx_status_type
mxd_fg_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_fg_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_FG_SCALER *fg_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	fg_scaler = (MX_FG_SCALER *) malloc( sizeof(MX_FG_SCALER) );

	if ( fg_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_FG_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = fg_scaler;
	record->class_specific_function_list
			= &mxd_fg_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fg_scaler_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_fg_scaler_open()";

	MX_FG_SCALER *fg_scaler;

	fg_scaler = (MX_FG_SCALER *) record->record_type_struct;

	if ( fg_scaler == (MX_FG_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_FG_SCALER pointer for record '%s' is NULL.",
			record->name );
	}

	fg_scaler->start_time = mx_high_resolution_time();

	fg_scaler->period = mx_divide_safely( 1.0, fg_scaler->frequency );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fg_scaler_clear( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_fg_scaler_clear()";

	MX_FG_SCALER *fg_scaler;

	fg_scaler = (MX_FG_SCALER *) scaler->record->record_type_struct;

	if ( fg_scaler == (MX_FG_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_FG_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	fg_scaler->start_time = mx_high_resolution_time();

	fg_scaler->period = mx_divide_safely( 1.0, fg_scaler->frequency );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_fg_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_fg_scaler_read()";

	MX_FG_SCALER *fg_scaler;
	struct timespec current_time, time_difference;
	double seconds_since_start, period_offset, period_fraction, raw_value;
	unsigned long num_periods;

	fg_scaler = (MX_FG_SCALER *) scaler->record->record_type_struct;

	if ( fg_scaler == (MX_FG_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_FG_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	current_time = mx_high_resolution_time();

	time_difference = mx_subtract_high_resolution_times( current_time,
							fg_scaler->start_time );

	seconds_since_start =
		mx_convert_high_resolution_time_to_seconds( time_difference );

	num_periods = (unsigned long) mx_divide_safely( seconds_since_start,
							 fg_scaler->period );

	period_offset = seconds_since_start - num_periods * (fg_scaler->period);

	period_fraction = mx_divide_safely( period_offset, fg_scaler->period );

	switch( fg_scaler->function ) {
	case MXT_FG_SCALER_SQUARE_WAVE:
		if ( period_fraction <= 0.5 ) {
			raw_value = fg_scaler->amplitude;
		} else {
			raw_value = 0.0;
		}
		break;
	case MXT_FG_SCALER_SINE_WAVE:
		raw_value = fg_scaler->amplitude
				* sin( 2.0 * MX_PI * period_fraction );
		break;
	case MXT_FG_SCALER_SAWTOOTH_WAVE:
		raw_value = period_fraction * fg_scaler->amplitude;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Function type %ld is not supported for "
		"function generator scaler '%s'.", 
			fg_scaler->function, scaler->record->name );
		break;
	}

	scaler->raw_value = mx_round( raw_value );

	return MX_SUCCESSFUL_RESULT;
}

