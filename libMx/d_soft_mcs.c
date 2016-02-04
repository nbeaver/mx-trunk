/*
 * Name:    d_soft_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for software emulated
 *          multichannel scalers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2004, 2006, 2008, 2010, 2012, 2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_MCS_DEBUG_START	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_mce.h"
#include "mx_scaler.h"
#include "d_soft_scaler.h"

#include "mx_mcs.h"
#include "d_soft_mcs.h"

/* Initialize the mcs driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_mcs_record_function_list = {
	mxd_soft_mcs_initialize_driver,
	mxd_soft_mcs_create_record_structures,
	mxd_soft_mcs_finish_record_initialization
};

MX_MCS_FUNCTION_LIST mxd_soft_mcs_mcs_function_list = {
	mxd_soft_mcs_start,
	mxd_soft_mcs_stop,
	mxd_soft_mcs_clear,
	mxd_soft_mcs_busy,
	mxd_soft_mcs_read_all,
	mxd_soft_mcs_read_scaler,
	mxd_soft_mcs_read_measurement,
	NULL,
	NULL,
	mxd_soft_mcs_get_parameter,
	mxd_soft_mcs_set_parameter
};

/* Soft MCS data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_soft_mcs_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_SOFT_MCS_STANDARD_FIELDS
};

long mxd_soft_mcs_num_record_fields
		= sizeof( mxd_soft_mcs_record_field_defaults )
			/ sizeof( mxd_soft_mcs_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_mcs_rfield_def_ptr
			= &mxd_soft_mcs_record_field_defaults[0];

static mx_status_type
mxd_soft_mcs_fill_data_array( MX_MCS *mcs, MX_SOFT_MCS *soft_mcs );

/* A private function for the use of the driver. */

static mx_status_type
mxd_soft_mcs_get_pointers( MX_MCS *mcs,
			MX_SOFT_MCS **soft_mcs,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_mcs_get_pointers()";

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( soft_mcs == (MX_SOFT_MCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOFT_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MCS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*soft_mcs = (MX_SOFT_MCS *) mcs->record->record_type_struct;

	if ( *soft_mcs == (MX_SOFT_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOFT_MCS pointer for record '%s' is NULL.",
			mcs->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_soft_mcs_initialize_driver( MX_DRIVER *driver )
{
	MX_RECORD_FIELD_DEFAULTS *field;
	long maximum_num_scalers_varargs_cookie;
	long maximum_num_measurements_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mcs_initialize_driver( driver,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults( driver,
					"soft_scaler_record_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = maximum_num_scalers_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_mcs_create_record_structures()";

	MX_MCS *mcs;
	MX_SOFT_MCS *soft_mcs = NULL;

	/* Allocate memory for the necessary structures. */

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS structure." );
	}

	soft_mcs = (MX_SOFT_MCS *) malloc( sizeof(MX_SOFT_MCS) );

	if ( soft_mcs == (MX_SOFT_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_MCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mcs;
	record->record_type_struct = soft_mcs;
	record->class_specific_function_list = &mxd_soft_mcs_mcs_function_list;

	mcs->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_soft_mcs_finish_record_initialization()";

	MX_MCS *mcs;
	MX_SOFT_MCS *soft_mcs = NULL;
	MX_RECORD *scaler_record;
	long i;

	mx_status_type mx_status;

	mx_status = mx_mcs_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs = (MX_MCS *) record->record_class_struct;

	soft_mcs = (MX_SOFT_MCS *) record->record_type_struct;

	/* Verify that all the listed scaler records are soft scaler records. */

	for ( i = 0; i < mcs->maximum_num_scalers; i++ ) {

		scaler_record = (soft_mcs->soft_scaler_record_array)[i];

		if ( scaler_record->mx_type != MXT_SCL_SOFTWARE ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Scaler '%s' used by soft MCS record '%s' is not a soft scaler.",
			scaler_record->name,
			record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_soft_mcs_start( MX_MCS *mcs )
{
	static const char fname[] = "mxd_soft_mcs_start()";

	MX_SOFT_MCS *soft_mcs = NULL;
	MX_CLOCK_TICK start_time_in_clock_ticks;
	MX_CLOCK_TICK total_counting_time_in_clock_ticks;
	double total_counting_time;
	mx_status_type mx_status;

	mx_status = mxd_soft_mcs_get_pointers( mcs, &soft_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCS_DEBUG
	MX_DEBUG(-2,("%s invoked for MCS '%s'", fname, mcs->record->name));
#endif

	start_time_in_clock_ticks = mx_current_clock_tick();

	total_counting_time = mcs->measurement_time
				* (double) mcs->current_num_measurements;

	total_counting_time_in_clock_ticks
		= mx_convert_seconds_to_clock_ticks( total_counting_time );

	soft_mcs->finish_time_in_clock_ticks = mx_add_clock_ticks(
				start_time_in_clock_ticks,
				total_counting_time_in_clock_ticks );

#if MXD_SOFT_MCS_DEBUG
	MX_DEBUG(-2,
		("%s: measurement_time = %g seconds, num_measurements = %lu",
		fname, mcs->measurement_time, mcs->current_num_measurements));

	MX_DEBUG(-2,
	("%s: total counting time = %g seconds, (%lu, %lu) in clock_ticks.",
		fname, total_counting_time,
		total_counting_time_in_clock_ticks.high_order,
		(unsigned long) total_counting_time_in_clock_ticks.low_order));

	MX_DEBUG(-2,("%s: starting time = (%lu,%lu), finish time = (%lu,%lu)",
		fname, start_time_in_clock_ticks.high_order,
		(unsigned long) start_time_in_clock_ticks.low_order,
		soft_mcs->finish_time_in_clock_ticks.high_order,
	(unsigned long) soft_mcs->finish_time_in_clock_ticks.low_order));
#endif

	mx_status = mxd_soft_mcs_fill_data_array( mcs, soft_mcs );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcs->external_channel_advance ) {

		/* Use external channel advance. */

		switch( mcs->external_channel_advance_record->mx_class ) {
		case MXC_MULTICHANNEL_ENCODER:
			mx_status = mx_mce_start(
				mcs->external_channel_advance_record );
			break;
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_stop( MX_MCS *mcs )
{
	static const char fname[] = "mxd_soft_mcs_stop()";

	MX_SOFT_MCS *soft_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mcs_get_pointers( mcs, &soft_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCS_DEBUG
	MX_DEBUG(-2,("%s invoked for MCS '%s'", fname, mcs->record->name));
#endif

	soft_mcs->finish_time_in_clock_ticks = mx_current_clock_tick();

	if ( mcs->external_channel_advance ) {

		/* Use external channel advance. */

		switch( mcs->external_channel_advance_record->mx_class ) {
		case MXC_MULTICHANNEL_ENCODER:
			mx_status = mx_mce_stop(
				mcs->external_channel_advance_record );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_soft_mcs_clear()";

	MX_SOFT_MCS *soft_mcs = NULL;
	long i, j;
	mx_status_type mx_status;

	mx_status = mxd_soft_mcs_get_pointers( mcs, &soft_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCS_DEBUG
	MX_DEBUG(-2,("%s invoked for MCS '%s'", fname, mcs->record->name));
#endif

	for ( i = 0; i < mcs->maximum_num_scalers; i++ ) {
		for ( j = 0; j < mcs->maximum_num_measurements; j++ ) {
			mcs->data_array[i][j] = 0L;
		}
	}

	if ( mcs->external_channel_advance ) {

		/* Use external channel advance. */

		switch( mcs->external_channel_advance_record->mx_class ) {
		case MXC_MULTICHANNEL_ENCODER:
			mx_status = mx_mce_clear(
				mcs->external_channel_advance_record );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_busy( MX_MCS *mcs )
{
	static const char fname[] = "mxd_soft_mcs_busy()";

	MX_SOFT_MCS *soft_mcs = NULL;
	MX_CLOCK_TICK current_time_in_clock_ticks;
	int result;
	unsigned long mce_status;
	mx_status_type mx_status;

	mx_status = mxd_soft_mcs_get_pointers( mcs, &soft_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	current_time_in_clock_ticks = mx_current_clock_tick();

	result = mx_compare_clock_ticks( current_time_in_clock_ticks,
				soft_mcs->finish_time_in_clock_ticks );

	if ( result >= 0 ) {
		mcs->busy = FALSE;
	} else {
		mcs->busy = TRUE;
	}

#if MXD_SOFT_MCS_DEBUG
	MX_DEBUG(-2,("%s: MCS '%s', busy = %d, time = (%lu,%lu)",
		fname, mcs->record->name, (int) mcs->busy,
		current_time_in_clock_ticks.high_order,
		(unsigned long) current_time_in_clock_ticks.low_order ));
#endif

	if ( mcs->busy == FALSE ) {
	    if ( mcs->external_channel_advance ) {

		/* Use external channel advance. */

		switch( mcs->external_channel_advance_record->mx_class ) {
		case MXC_MULTICHANNEL_ENCODER:
			mx_status = mx_mce_get_status( 
				mcs->external_channel_advance_record,
				&mce_status );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( mce_status != 0 ) {
				mx_status = mx_mce_stop(
					mcs->external_channel_advance_record );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
			break;
		}
	    }
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_read_all( MX_MCS *mcs )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_read_scaler( MX_MCS *mcs )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_read_measurement( MX_MCS *mcs )
{
	static const char fname[] = "mxd_soft_mcs_read_measurement()";

	MX_SOFT_MCS *soft_mcs = NULL;
	unsigned long i;
	mx_status_type mx_status;

	mx_status = mxd_soft_mcs_get_pointers( mcs, &soft_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCS_DEBUG
	MX_DEBUG(-2,("%s invoked for MCS '%s', measurement_index = %ld",
		fname, mcs->record->name, mcs->measurement_index));
#endif

	for ( i = 0; i < mcs->current_num_scalers; i++ ) {
		mcs->measurement_data[i]
			= mcs->data_array[i][ mcs->measurement_index ];

		MX_DEBUG( 2,("%s: mcs->measurement_data[%lu] = %ld",
			fname, i, mcs->measurement_data[i]));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_get_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_soft_mcs_get_parameter()";

	MX_SOFT_MCS *soft_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mcs_get_pointers( mcs, &soft_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCS_DEBUG
	MX_DEBUG(-2,("%s invoked for MCS '%s', type = %ld",
		fname, mcs->record->name, mcs->parameter_type));
#endif

	if ( mcs->parameter_type == MXLV_MCS_MODE ) {

		MX_DEBUG( 2,("%s: MCS mode = %ld", fname, mcs->mode));

	} else if ( mcs->parameter_type == MXLV_MCS_MEASUREMENT_TIME ) {

		MX_DEBUG( 2,("%s: MCS measurement time = %g",
				fname, mcs->measurement_time));

	} else if ( mcs->parameter_type == MXLV_MCS_CURRENT_NUM_MEASUREMENTS ){

		MX_DEBUG( 2,("%s: MCS number of measurements = %lu",
			fname, mcs->current_num_measurements));

	} else {
		return mx_mcs_default_get_parameter_handler( mcs );
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_mcs_set_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_soft_mcs_set_parameter()";

	MX_SOFT_MCS *soft_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_mcs_get_pointers( mcs, &soft_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_MCS_DEBUG
	MX_DEBUG(-2,("%s invoked for MCS '%s', type = %ld",
		fname, mcs->record->name, mcs->parameter_type));
#endif

	if ( mcs->parameter_type == MXLV_MCS_MODE ) {

#if MXD_SOFT_MCS_DEBUG
		MX_DEBUG(-2,("%s: MCS mode = %ld", fname, mcs->mode));
#endif

		if ( mcs->mode != MXM_PRESET_TIME ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal MCS mode %ld selected.  Only preset time mode is "
		"allowed for a soft MCS.", mcs->mode );
		}

	} else if ( mcs->parameter_type == MXLV_MCS_MEASUREMENT_TIME ) {

#if MXD_SOFT_MCS_DEBUG
		MX_DEBUG(-2,("%s: MCS measurement time = %g",
				fname, mcs->measurement_time));
#endif

	} else if ( mcs->parameter_type == MXLV_MCS_CURRENT_NUM_MEASUREMENTS ){

#if MXD_SOFT_MCS_DEBUG
		MX_DEBUG(-2,("%s: MCS number of measurements = %lu",
			fname, mcs->current_num_measurements));
#endif

	} else if ( mcs->parameter_type == MXLV_MCS_EXTERNAL_CHANNEL_ADVANCE ) {

		if ( mcs->external_channel_advance != FALSE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"The driver for soft MCS record '%s' does not support "
		"external channel advance.  You must use internal channel "
		"advance.", mcs->record->name );
		}

	} else {
		return mx_mcs_default_set_parameter_handler( mcs );
	}

#if MXD_SOFT_MCS_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxd_soft_mcs_fill_data_array( MX_MCS *mcs, MX_SOFT_MCS *soft_mcs )
{
	MX_RECORD *soft_scaler_record;
	MX_SOFT_SCALER *soft_scaler;
	long i, j;
	double position, scaler_value;

	for ( i = 0; i < mcs->current_num_scalers; i++ ) {

		soft_scaler_record = (soft_mcs->soft_scaler_record_array)[i];

#if 0
		fprintf(stderr, "Scaler '%s': ", soft_scaler_record->name);
#endif

		soft_scaler = (MX_SOFT_SCALER *)
				soft_scaler_record->record_type_struct;

		for ( j = 0; j < mcs->current_num_measurements; j++ ) {

			position = soft_mcs->starting_position
				+ soft_mcs->delta_position * (double) j;

			scaler_value = mxd_soft_scaler_get_value_from_position(
					soft_scaler, position );

			(mcs->data_array)[i][j] = mx_round( scaler_value );

#if 0
			fprintf(stderr,"%ld ", (mcs->data_array)[i][j]);
#endif
		}
#if 0
		fprintf(stderr,"\n");
#endif
	}
	return MX_SUCCESSFUL_RESULT;
}

