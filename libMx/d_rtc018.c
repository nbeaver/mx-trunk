/*
 * Name:    d_rtc018.c
 *
 * Purpose: MX timer driver to control DSP RTC-018 timers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "mx_camac.h"
#include "d_rtc018.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_rtc018_record_function_list = {
	NULL,
	mxd_rtc018_create_record_structures,
	mxd_rtc018_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_rtc018_timer_function_list = {
	mxd_rtc018_timer_is_busy,
	mxd_rtc018_timer_start,
	mxd_rtc018_timer_stop
};

MX_RECORD_FIELD_DEFAULTS mxd_rtc018_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_RTC018_STANDARD_FIELDS
};

long mxd_rtc018_num_record_fields
		= sizeof( mxd_rtc018_record_field_defaults )
		  / sizeof( mxd_rtc018_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_rtc018_rfield_def_ptr
			= &mxd_rtc018_record_field_defaults[0];

MX_EXPORT mx_status_type
mxd_rtc018_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_rtc018_create_record_structures()";

	MX_TIMER *timer;
	MX_RTC018 *rtc018;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	rtc018 = (MX_RTC018 *) malloc( sizeof(MX_RTC018) );

	if ( rtc018 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RTC018 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = rtc018;
	record->class_specific_function_list = &mxd_rtc018_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rtc018_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_rtc018_finish_record_initialization()";

	MX_RTC018 *rtc018;
	mx_status_type mx_status;

	rtc018 = (MX_RTC018 *) record->record_type_struct;

	if ( rtc018 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RTC018 pointer for record '%s' is NULL.",
		record->name );
	}

	/* === CAMAC slot number === */

	if ( rtc018->slot < 1 || rtc018->slot > 23 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"CAMAC slot number %d is out of allowed range 1-23.",
			rtc018->slot );
	}

	MX_DEBUG(2, ("slot = %d, timer name = '%s'.",
		rtc018->slot, record->name));

	rtc018->saved_step_down_bit = -1;
	rtc018->seconds_per_tick = 0.0;

	mx_status = mx_timer_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_rtc018_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_rtc018_timer_is_busy()";

	MX_RTC018 *rtc018;
	mx_status_type mx_status;
	int32_t data;
	int camac_Q, camac_X;

	MX_DEBUG(-2, ("mxd_rtc018_timer_is_busy() called."));

	rtc018 = (MX_RTC018 *) (timer->record->record_type_struct);

	if ( rtc018 == (MX_RTC018 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RTC018 structure pointer is NULL.");
	}

	mx_status = mx_camac( rtc018->camac_record,
			rtc018->slot, 0, 27, &data, &camac_Q, &camac_X );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"CAMAC X = 0 error.  Q = %d.", camac_Q);
	}

	if ( camac_Q ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rtc018_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_rtc018_timer_start()";

	MX_RTC018 *rtc018;
	double seconds;
	double number_of_ticks;
	int32_t preset_counter;
	int step_down_bit, camac_Q, camac_X;
	mx_status_type mx_status;

	rtc018 = (MX_RTC018 *) ( timer->record->record_type_struct );

	if ( rtc018 == (MX_RTC018 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RTC018 structure pointer is NULL.");
	}

	seconds = timer->value;

	/* Treat any time less than zero as zero. */

	if ( seconds < 0.0 ) {
		seconds = 0.0;
	}

	/* First see if we need to change the step down ratio. */

	/* Note that the divisor of 65535.5 decreases in size by
	 * a factor of 8 for each range we go up.  This is because
	 * each increment in the step down bit causes the time interval
	 * between clock ticks to increase by a factor of 8.  The
	 * maximum clock tick frequency = 262144 Hz = 2**18 Hz.
	 */

	if ( seconds < ( 65535.5 / 262144. ) ) {

		step_down_bit = 1;

	} else if ( seconds < ( 65535.5 / 32768. ) ) {

		step_down_bit = 2;

	} else if ( seconds < ( 65535.5 / 4096. ) ) {

		step_down_bit = 3;

	} else if ( seconds < ( 65535.5 / 512. ) ) {

		step_down_bit = 4;

	} else if ( seconds < ( 65535.5 / 64. ) ) {

		step_down_bit = 5;

	} else if ( seconds < ( 65535.5 / 8. ) ) {

		step_down_bit = 6;

	} else if ( seconds < 65535.5 ) {

		step_down_bit = 7;

	} else {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested counting time of %g seconds exceeds the\n"
		"maximum allowed counting time of 65535 seconds (18.2 hours)",
			seconds );
	}

	if ( step_down_bit != rtc018->saved_step_down_bit ) {
		mx_status = mxd_rtc018_set_step_down_bit( timer, step_down_bit );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now calculate the number of RTC clock ticks required. */

	number_of_ticks = seconds / rtc018->seconds_per_tick;

	preset_counter = (int32_t) ( 0.5 + number_of_ticks );

	/* Write into the preset counter.  This generates a Preset Out
	 * pulse that is fed by a Lemo cable into the Start input of 
	 * the RTC.  This generates a gate signal out of the Busy output
	 * that is used to gate the scalers on.
	 */

	mx_status = mx_camac( rtc018->camac_record,
		rtc018->slot, 0, 16, &preset_counter, &camac_Q, &camac_X);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to set preset counter of RTC.  Q = %d, X = %d",
			camac_Q, camac_X );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_rtc018_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_rtc018_timer_stop()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"This type of timer cannot be stopped under program control." );
}

MX_EXPORT mx_status_type
mxd_rtc018_set_step_down_bit( MX_TIMER *timer, int step_down_bit )
{
	static const char fname[] = "mxd_rtc018_set_step_down_bit()";

	MX_RTC018 *rtc018;
	int32_t divisor;
	int camac_Q, camac_X;
	mx_status_type mx_status;

	rtc018 = (MX_RTC018 *) ( timer->record->record_type_struct );

	if ( rtc018 == (MX_RTC018 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RTC018 structure pointer is NULL.");
	}

	rtc018->saved_step_down_bit = step_down_bit;

	/* Program the frequency divider. */

	divisor = 1 << (step_down_bit - 1);

	rtc018->seconds_per_tick 
			= 1.0 / (double) ( 1 << (21 - 3*step_down_bit) );

#if 0
	MX_DEBUG( -2, 
	("Debug: step down bit = %d\n"
	 "       frequency divider = %d, seconds per tick = %le\n"
	 "       maximum counting time = %lf seconds.\n",
		step_down_bit, divisor, seconds_per_tick,
					65535.0 * seconds_per_tick) );
#endif

	mx_status = mx_camac( rtc018->camac_record,
			rtc018->slot, 1, 16, &divisor, &camac_Q, &camac_X );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( camac_Q == 0 || camac_X == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Failed to program RTC frequency divider.  Q = %d, X = %d",
			camac_Q, camac_X );
	}

	return MX_SUCCESSFUL_RESULT;
}

