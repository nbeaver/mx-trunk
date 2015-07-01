/*
 * Name:    d_ortec974_timer.c
 *
 * Purpose: MX timer driver to control the timer in EG&G Ortec 974
 *          counter/timer units.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2006-2007, 2010, 2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define ORTEC974_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "i_ortec974.h"
#include "d_ortec974_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_ortec974_timer_record_function_list = {
	NULL,
	mxd_ortec974_timer_create_record_structures,
	mxd_ortec974_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_ortec974_timer_timer_function_list = {
	mxd_ortec974_timer_is_busy,
	mxd_ortec974_timer_start,
	mxd_ortec974_timer_stop,
	NULL,
	NULL,
	mxd_ortec974_timer_get_mode,
	mxd_ortec974_timer_set_mode
};

/* Ortec 974 timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_ortec974_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_ORTEC974_TIMER_STANDARD_FIELDS
};

long mxd_ortec974_timer_num_record_fields
		= sizeof( mxd_ortec974_timer_record_field_defaults )
		  / sizeof( mxd_ortec974_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ortec974_timer_rfield_def_ptr
			= &mxd_ortec974_timer_record_field_defaults[0];

MX_EXPORT mx_status_type
mxd_ortec974_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_ortec974_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_ORTEC974_TIMER *ortec974_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	ortec974_timer = (MX_ORTEC974_TIMER *)
				malloc( sizeof(MX_ORTEC974_TIMER) );

	if ( ortec974_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ORTEC974_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = ortec974_timer;
	record->class_specific_function_list
			= &mxd_ortec974_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_ortec974_timer_finish_record_initialization()";

	MX_TIMER *timer;
	MX_ORTEC974_TIMER *ortec974_timer;
	MX_RECORD *ortec974_record;
	mx_status_type mx_status;

	timer = (MX_TIMER *)( record->record_class_struct );

	timer->mode = MXCM_PRESET_MODE;

	/* Verify that the ortec974_record field is actually an Ortec 974
	 * interface record.
	 */

	ortec974_timer = (MX_ORTEC974_TIMER *)( record->record_type_struct );

	ortec974_record = ortec974_timer->ortec974_record;

	MX_DEBUG( 2,("%s: ortec974_record = %p, name = '%s'",
		fname, ortec974_record, ortec974_record->name ));

	if ( ortec974_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' is not an interface record.", ortec974_record->name );
	}
	if ( ortec974_record->mx_class != MXI_CONTROLLER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' is not a generic record.", ortec974_record->name );
	}

	/* Fill in the structure with the Ortec 974 timer parameters. */

	ortec974_timer->timer_mode = MX_974_TIMER_MODE_SECONDS;
	ortec974_timer->seconds_per_tick = 0.0;

	mx_status = mx_timer_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ortec974_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_ortec974_timer_is_busy()";

	MX_ORTEC974_TIMER *ortec974_timer;
	MX_ORTEC974 *ortec974;
	char response[80];
	int num_items;
	unsigned long nap_milliseconds;
	long time1, time2;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	ortec974_timer = (MX_ORTEC974_TIMER *)
				(timer->record->record_type_struct);

	if ( ortec974_timer == (MX_ORTEC974_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ORTEC974_TIMER structure pointer is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)
		(ortec974_timer->ortec974_record->record_type_struct);

	if ( ortec974 == (MX_ORTEC974 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ORTEC974 structure pointer is NULL.");
	}

	/* The Ortec 974 doesn't seem to have a simple command for determining
	 * whether or not the timer is still counting.  However, if we read
	 * out the timer channel twice with more than one clock tick interval
	 * between them, we should see the timer channel value change if the
	 * clock is still running.  Otherwise, it has stopped.  This procedure
	 * will add somewhat to the dead time, but I haven't figured out
	 * another way to do this.
	 */

	timer->busy = TRUE;

	/* Read the current timer value in the timer channel. */

	mx_status = mxi_ortec974_command( ortec974, "SHOW_COUNTS 1",
		response, sizeof(response)-1, NULL, 0, ORTEC974_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &time1 );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see timer channel value in Ortec 974 response '%s'",
			response );
	}

	switch( ortec974_timer->timer_mode ) {
	case MX_974_TIMER_MODE_SECONDS:
		nap_milliseconds = 110;
		break;
	case MX_974_TIMER_MODE_EXTERNAL:
		nap_milliseconds
			= 10 + mx_round(ortec974_timer->seconds_per_tick);
		break;
	case MX_974_TIMER_MODE_MINUTES:
		nap_milliseconds = 60010;	/* A really long time. */
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal Ortec 974 timer mode %d",
			ortec974_timer->timer_mode );
	}

	mx_msleep( nap_milliseconds );

	/* Read the current timer value in the timer channel again. */

	mx_status = mxi_ortec974_command( ortec974, "SHOW_COUNTS 1",
		response, sizeof(response)-1, NULL, 0, ORTEC974_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &time2 );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see timer channel value in Ortec 974 response '%s'",
			response );
	}

	if ( time1 == time2 ) {
		timer->busy = FALSE;

		/* Turn the timer gate off. */

		mx_status = mxi_ortec974_command( ortec974, "STOP",
					NULL, 0, NULL, 0, ORTEC974_DEBUG );
	} else {
		timer->busy = TRUE;
	}

	MX_DEBUG( 2,("%s: timer->busy = %d", fname, (int) timer->busy));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ortec974_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_ortec974_timer_start()";

	MX_ORTEC974_TIMER *ortec974_timer;
	MX_ORTEC974 *ortec974;
	char command[80];
	double seconds;
	int timer_mode, multiplier, exponent;
	double preset_count, mantissa;
	int multiplier_above, exponent_above;
	double preset_count_above, preset_count_below;
	double seconds_above, seconds_below;
	mx_status_type mx_status;

	seconds = timer->value;

	MX_DEBUG( 2,("%s invoked for %g seconds.", fname, seconds));

	ortec974_timer = (MX_ORTEC974_TIMER *)
				( timer->record->record_type_struct );

	if ( ortec974_timer == (MX_ORTEC974_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ORTEC974_TIMER structure pointer is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)
		(ortec974_timer->ortec974_record->record_type_struct);

	if ( ortec974 == (MX_ORTEC974 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ORTEC974 structure pointer is NULL.");
	}

#if 1
	/* FIXME: I don't know if the following section actually needs to
	 * be here or not.  It was added in response to a problem reported
	 * at CAMD, but I have never heard from them as to whether or not
	 * it fixed the problem.  (W. Lavender)
	 */

	/* Stop and clear the Ortec 974 in case it is counting. */

	mx_status = mxi_ortec974_command( ortec974, "STOP",
					NULL, 0, NULL, 0, ORTEC974_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_ortec974_command( ortec974, "CLEAR_COUNTERS",
					NULL, 0, NULL, 0, ORTEC974_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Treat any time less than zero as zero. */

	if ( seconds < 0.0 ) {
		seconds = 0.0;
	}

	if ( ortec974_timer->timer_mode == MX_974_TIMER_MODE_EXTERNAL ) {

		timer_mode = MX_974_TIMER_MODE_EXTERNAL;

		if ( fabs( ortec974_timer->seconds_per_tick ) < 1.0e-37 ) {
			preset_count = 0.0;
		} else {
			preset_count = seconds 
				/ ortec974_timer->seconds_per_tick;
		}

		if ( preset_count > MX_974_MAX_PRESET_COUNT ) {

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Counting time of %g seconds exceeds the maximum allowed by the "
	"external time base of %g seconds per tick ( maximum = %g seconds ).",
				seconds, ortec974_timer->seconds_per_tick,
		ortec974_timer->seconds_per_tick * MX_974_MAX_PRESET_COUNT );

		}
	} else {
		if ( seconds <= 0.1 * MX_974_MAX_PRESET_COUNT ) {

			timer_mode = MX_974_TIMER_MODE_SECONDS;

			ortec974_timer->seconds_per_tick = 0.1;

		} else if ( seconds <= 60.0 * MX_974_MAX_PRESET_COUNT ) {

			timer_mode = MX_974_TIMER_MODE_MINUTES;

			ortec974_timer->seconds_per_tick = 60.0;

		} else {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"A counting time of %g seconds exceeds the maximum allowed by the "
	"internal time base ( maximum = %g seconds ).",
				seconds, 60.0 * MX_974_MAX_PRESET_COUNT );
		}

		preset_count = seconds / ortec974_timer->seconds_per_tick;
	}

	/* If the preset count is less than one tick, return immediately. */

	if ( preset_count < 1.0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Change the timer mode if necessary. */

	if ( timer_mode != ortec974_timer->timer_mode ) {

		switch( timer_mode ) {
		case MX_974_TIMER_MODE_EXTERNAL:
			strlcpy( command, "SET_MODE_EXTERNAL",
					sizeof(command) );
			break;
		case MX_974_TIMER_MODE_MINUTES:
			strlcpy( command, "SET_MODE_MINUTES",
					sizeof(command) );
			break;
		case MX_974_TIMER_MODE_SECONDS:
			strlcpy( command, "SET_MODE_SECONDS",
					sizeof(command) );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal timer mode %d requested.", timer_mode );
		}

		mx_status = mxi_ortec974_command( ortec974, command,
					NULL, 0, NULL, 0, ORTEC974_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the preset count. */

	exponent = (int) log10( preset_count );

	mantissa = preset_count / pow( 10.0, (double) exponent );

	multiplier = (int) ( mantissa + 0.001 );

	if ( fabs(mantissa - (double) multiplier) > 1.0e-9 ) {

		if ( multiplier == 9 ) {
			multiplier_above = 1;
			exponent_above = exponent + 1;
		} else {
			multiplier_above = multiplier + 1;
			exponent_above = exponent;
		}

		preset_count_above = (double) multiplier_above
					* pow( 10.0, (double) exponent_above );

		preset_count_below = (double) multiplier
					* pow( 10.0, (double) exponent );

		seconds_above = preset_count_above
					* ortec974_timer->seconds_per_tick;

		seconds_below = preset_count_below
					* ortec974_timer->seconds_per_tick;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Ortec 974 integration times may have only one significant digit.  "
"Integration times of %g sec or %g sec would be allowed, but not %g sec.",
			seconds_below, seconds_above, seconds );
	}

	snprintf( command, sizeof(command),
			"SET_COUNT_PRESET %d,%d", multiplier, exponent );

	mx_status = mxi_ortec974_command( ortec974, command,
					NULL, 0, NULL, 0, ORTEC974_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the timer. */

	mx_status = mxi_ortec974_command( ortec974, "START",
					NULL, 0, NULL, 0, ORTEC974_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ortec974_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_ortec974_timer_stop()";

	MX_ORTEC974_TIMER *ortec974_timer;
	MX_ORTEC974 *ortec974;
	mx_status_type mx_status;

	ortec974_timer = (MX_ORTEC974_TIMER *)
				( timer->record->record_type_struct );

	if ( ortec974_timer == (MX_ORTEC974_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ORTEC974_TIMER structure pointer is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)
		(ortec974_timer->ortec974_record->record_type_struct);

	if ( ortec974 == (MX_ORTEC974 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ORTEC974 structure pointer is NULL.");
	}

	/* Stop the timer. */

	mx_status = mxi_ortec974_command( ortec974, "STOP",
					NULL, 0, NULL, 0, ORTEC974_DEBUG );

	timer->value = 0.0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ortec974_timer_get_mode( MX_TIMER *timer )
{
	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ortec974_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_ortec974_timer_set_mode()";

	if ( timer->mode != MXCM_PRESET_MODE ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"This operation is not yet implemented." );
	}

	return MX_SUCCESSFUL_RESULT;
}

