/*
 * Name:    d_pfcu_shutter_timer.c
 *
 * Purpose: MX timer driver to control the exposure time of a PF2S2 shutter
 *          controlled by an XIA PFCU controller.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PFCU_SHUTTER_TIMER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "mx_rs232.h"
#include "i_pfcu.h"
#include "d_pfcu_shutter_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_pfcu_shutter_timer_record_function_list = {
	NULL,
	mxd_pfcu_shutter_timer_create_record_structures,
	mxd_pfcu_shutter_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_pfcu_shutter_timer_timer_function_list = {
	mxd_pfcu_shutter_timer_is_busy,
	mxd_pfcu_shutter_timer_start,
	mxd_pfcu_shutter_timer_stop,
	mxd_pfcu_shutter_timer_clear,
	mxd_pfcu_shutter_timer_read,
	mxd_pfcu_shutter_timer_get_mode,
	mxd_pfcu_shutter_timer_set_mode
};

/* MX mca timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pfcu_shutter_timer_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_PFCU_SHUTTER_TIMER_STANDARD_FIELDS
};

long mxd_pfcu_shutter_timer_num_record_fields
		= sizeof( mxd_pfcu_shutter_timer_rf_defaults )
		  / sizeof( mxd_pfcu_shutter_timer_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pfcu_shutter_timer_rfield_def_ptr
			= &mxd_pfcu_shutter_timer_rf_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_pfcu_shutter_timer_get_pointers( MX_TIMER *timer,
			      MX_PFCU_SHUTTER_TIMER **pfcu_shutter_timer,
			      MX_PFCU **pfcu,
			      const char *fname )
{
	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_TIMER pointer passed is NULL." );
	}
	if ( pfcu_shutter_timer == (MX_PFCU_SHUTTER_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PFCU_SHUTTER_TIMER pointer passed is NULL." );
	}
	if ( pfcu == (MX_PFCU **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PFCU pointer passed is NULL." );
	}

	*pfcu_shutter_timer = (MX_PFCU_SHUTTER_TIMER *)
					timer->record->record_type_struct;

	if ( *pfcu_shutter_timer == (MX_PFCU_SHUTTER_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PFCU_SHUTTER_TIMER pointer for record '%s' is NULL.",
			timer->record->name );
	}

	*pfcu = (MX_PFCU *)
		(*pfcu_shutter_timer)->pfcu_record->record_type_struct;

	if ( *pfcu == (MX_PFCU *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PFCU pointer for PFCU record '%s' used by "
		"PFCU timer record '%s' is NULL.",
			(*pfcu_shutter_timer)->pfcu_record->name,
			timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_pfcu_shutter_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pfcu_shutter_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_PFCU_SHUTTER_TIMER *pfcu_shutter_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	pfcu_shutter_timer = (MX_PFCU_SHUTTER_TIMER *)
				malloc( sizeof(MX_PFCU_SHUTTER_TIMER) );

	if ( pfcu_shutter_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PFCU_SHUTTER_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = pfcu_shutter_timer;
	record->class_specific_function_list
			= &mxd_pfcu_shutter_timer_timer_function_list;

	timer->record = record;
	pfcu_shutter_timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pfcu_shutter_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pfcu_shutter_timer_finish_record_initialization()";

	MX_TIMER *timer;
	MX_PFCU_SHUTTER_TIMER *pfcu_shutter_timer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	timer->mode = MXCM_PRESET_MODE;

	pfcu_shutter_timer = (MX_PFCU_SHUTTER_TIMER *)
				record->record_type_struct;

	if ( pfcu_shutter_timer == (MX_PFCU_SHUTTER_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PFCU_SHUTTER_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mx_timer_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pfcu_shutter_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_pfcu_shutter_timer_is_busy()";

	MX_PFCU_SHUTTER_TIMER *pfcu_shutter_timer;
	MX_PFCU *pfcu;
	unsigned long num_input_bytes_available;
	mx_status_type mx_status;

	mx_status = mxd_pfcu_shutter_timer_get_pointers( timer,
					&pfcu_shutter_timer, &pfcu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pfcu->exposure_in_progress == FALSE ) {
		timer->busy = FALSE;
	} else {
		mx_status = mx_rs232_num_input_bytes_available(
				pfcu->rs232_record, &num_input_bytes_available);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_input_bytes_available > 0 ) {
			mx_status = mx_rs232_discard_unread_input(
						pfcu->rs232_record,
						MXD_PFCU_SHUTTER_TIMER_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			timer->busy = FALSE;
		} else {
			timer->busy = TRUE;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pfcu_shutter_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_pfcu_shutter_timer_start()";

	MX_PFCU_SHUTTER_TIMER *pfcu_shutter_timer;
	MX_PFCU *pfcu;
	double seconds_to_count, centiseconds_to_count;
	unsigned long decimation, exposure_time;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_pfcu_shutter_timer_get_pointers( timer,
					&pfcu_shutter_timer, &pfcu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

	if ( seconds_to_count > MX_PFCU_SHUTTER_TIMER_MAXIMUM_COUNTING_TIME ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested counting time of %g seconds exceeds the "
			"maximum allowed counting time of %g seconds for "
			"PFCU shutter timer '%s'.", seconds_to_count, 
			MX_PFCU_SHUTTER_TIMER_MAXIMUM_COUNTING_TIME,
			timer->record->name );
	}

	/* Always count for at least the minimum allowed time. */

	if ( seconds_to_count < MX_PFCU_SHUTTER_TIMER_MINIMUM_COUNTING_TIME ) {
		seconds_to_count = MX_PFCU_SHUTTER_TIMER_MINIMUM_COUNTING_TIME;
	}

	/* How many centiseconds will we be counting for?
	 *
	 * 1 centisecond = 10 milliseconds (of course).
	 *
	 */

	centiseconds_to_count = 100.0 * seconds_to_count;

	/* Compute the decimation and exposure time.  We want the decimation
	 * to be as small as possible for this counting time to get the
	 * most accuracy possible in computing the exposure time.
	 */

	decimation = mx_round_away_from_zero(
				centiseconds_to_count / 65536.0, 1.0e-9 );

	exposure_time = mx_round( centiseconds_to_count / (double) decimation );

	MX_DEBUG(-2,("%s: seconds_to_count = %g, centiseconds_to_count = %g",
		fname, seconds_to_count, centiseconds_to_count));
	MX_DEBUG(-2,("%s: decimation = %lu, exposure_time = %lu",
		fname, decimation, exposure_time));

	/* Set the decimation. */

	sprintf( command, "D %lu", decimation );

	mx_status = mxi_pfcu_command( pfcu, pfcu_shutter_timer->module_number,
					command, NULL, 0,
					MXD_PFCU_SHUTTER_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the exposure. */

	sprintf( command, "E %lu", exposure_time );

	mx_status = mxi_pfcu_command( pfcu, pfcu_shutter_timer->module_number,
					command, NULL, 0,
					MXD_PFCU_SHUTTER_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pfcu->exposure_in_progress = TRUE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pfcu_shutter_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_pfcu_shutter_timer_stop()";

	MX_PFCU_SHUTTER_TIMER *pfcu_shutter_timer;
	MX_PFCU *pfcu;
	mx_status_type mx_status;

	mx_status = mxd_pfcu_shutter_timer_get_pointers( timer,
					&pfcu_shutter_timer, &pfcu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
	
	/* Command the shutter to close. */

	mx_status = mxi_pfcu_command( pfcu, pfcu_shutter_timer->module_number,
					"C", NULL, 0,
					MXD_PFCU_SHUTTER_TIMER_DEBUG );

	pfcu->exposure_in_progress = FALSE;

	timer->value = 0.0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pfcu_shutter_timer_clear( MX_TIMER *timer )
{
	static const char fname[] = "mxd_pfcu_shutter_timer_clear()";

	MX_PFCU_SHUTTER_TIMER *pfcu_shutter_timer;
	MX_PFCU *pfcu;
	mx_status_type mx_status;

	mx_status = mxd_pfcu_shutter_timer_get_pointers( timer,
					&pfcu_shutter_timer, &pfcu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
	
	pfcu->exposure_in_progress = FALSE;

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pfcu_shutter_timer_read( MX_TIMER *timer )
{
	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pfcu_shutter_timer_get_mode( MX_TIMER *timer )
{
	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pfcu_shutter_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_pfcu_shutter_timer_set_mode()";

	if ( timer->mode != MXCM_PRESET_MODE ) {
		long mode;

		mode = timer->mode;
		timer->mode = MXCM_PRESET_MODE;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Timer mode %d is illegal for MCA timer '%s'",
			mode, timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

