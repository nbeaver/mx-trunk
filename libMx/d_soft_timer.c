/*
 * Name:    d_soft_timer.c
 *
 * Purpose: MX timer driver to control simple software timers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SOFT_TIMER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "d_soft_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_timer_record_function_list = {
	NULL,
	mxd_soft_timer_create_record_structures,
	mxd_soft_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_soft_timer_timer_function_list = {
	mxd_soft_timer_is_busy,
	mxd_soft_timer_start,
	mxd_soft_timer_stop,
	NULL,
	mxd_soft_timer_read
};

/* Soft timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_soft_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS
};

mx_length_type mxd_soft_timer_num_record_fields
		= sizeof( mxd_soft_timer_record_field_defaults )
		  / sizeof( mxd_soft_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_timer_rfield_def_ptr
			= &mxd_soft_timer_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_soft_timer_get_pointers( MX_TIMER *timer,
			MX_SOFT_TIMER **soft_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_timer_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The timer pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( timer->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for timer pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( timer->record->mx_type != MXT_TIM_SOFTWARE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The timer '%s' passed by '%s' is not a soft timer.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			timer->record->name, calling_fname,
			timer->record->mx_superclass,
			timer->record->mx_class,
			timer->record->mx_type );
	}

	if ( soft_timer != (MX_SOFT_TIMER **) NULL ) {

		*soft_timer = (MX_SOFT_TIMER *)
				(timer->record->record_type_struct);

		if ( *soft_timer == (MX_SOFT_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SOFT_TIMER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_soft_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_SOFT_TIMER *soft_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	soft_timer = (MX_SOFT_TIMER *)
				malloc( sizeof(MX_SOFT_TIMER) );

	if ( soft_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = soft_timer;
	record->class_specific_function_list
			= &mxd_soft_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_soft_timer_finish_record_initialization()";

	MX_TIMER *timer;
	MX_SOFT_TIMER *soft_timer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	timer->mode = MXCM_PRESET_MODE;

	soft_timer = (MX_SOFT_TIMER *) record->record_type_struct;

	if ( soft_timer == (MX_SOFT_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SOFT_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	soft_timer->finish_time_in_clock_ticks = mx_current_clock_tick();

	mx_status =  mx_timer_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_soft_timer_is_busy()";

	MX_SOFT_TIMER *soft_timer;
	MX_CLOCK_TICK current_time_in_clock_ticks;
	int result;
	mx_status_type mx_status;

	mx_status = mxd_soft_timer_get_pointers( timer, &soft_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	current_time_in_clock_ticks = mx_current_clock_tick();

	result = mx_compare_clock_ticks( current_time_in_clock_ticks,
				soft_timer->finish_time_in_clock_ticks );

	if ( result >= 0 ) {
		timer->busy = FALSE;
	} else {
		timer->busy = TRUE;
	}

#if MX_SOFT_TIMER_DEBUG

	MX_DEBUG(-2,("%s: current time = (%lu,%ld), busy = %d",
		fname, current_time_in_clock_ticks.high_order,
		(unsigned long) current_time_in_clock_ticks.low_order,
		timer->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_soft_timer_start()";

	MX_SOFT_TIMER *soft_timer;
	MX_CLOCK_TICK start_time_in_clock_ticks;
	MX_CLOCK_TICK measurement_time_in_clock_ticks;
	mx_status_type mx_status;

	mx_status = mxd_soft_timer_get_pointers( timer, &soft_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_time_in_clock_ticks = mx_current_clock_tick();

	measurement_time_in_clock_ticks
		= mx_convert_seconds_to_clock_ticks( timer->value );

	soft_timer->finish_time_in_clock_ticks = mx_add_clock_ticks(
				start_time_in_clock_ticks,
				measurement_time_in_clock_ticks );

#if MX_SOFT_TIMER_DEBUG

	MX_DEBUG(-2,("%s: counting for %g seconds, (%lu,%ld) in clock ticks.",
		fname, timer->value,
		measurement_time_in_clock_ticks.high_order,
		(long) measurement_time_in_clock_ticks.low_order));

	MX_DEBUG(-2,("%s: starting time = (%lu,%ld), finish time = (%lu,%ld)",
		fname, start_time_in_clock_ticks.high_order,
		(long) start_time_in_clock_ticks.low_order,
		soft_timer->finish_time_in_clock_ticks.high_order,
		(long) soft_timer->finish_time_in_clock_ticks.low_order));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_soft_timer_stop()";

	MX_SOFT_TIMER *soft_timer;
	mx_status_type mx_status;

	mx_status = mxd_soft_timer_get_pointers( timer, &soft_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	soft_timer->finish_time_in_clock_ticks = mx_current_clock_tick();

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_soft_timer_read()";

	MX_SOFT_TIMER *soft_timer;
	MX_CLOCK_TICK current_time_in_clock_ticks;
	MX_CLOCK_TICK time_left_in_clock_ticks;
	mx_status_type mx_status;

	mx_status = mxd_soft_timer_get_pointers( timer, &soft_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	current_time_in_clock_ticks = mx_current_clock_tick();

	if ( mx_compare_clock_ticks( current_time_in_clock_ticks,
			soft_timer->finish_time_in_clock_ticks ) >= 0 )
	{
		timer->value = 0.0;
	} else {
		time_left_in_clock_ticks = mx_subtract_clock_ticks(
			soft_timer->finish_time_in_clock_ticks,
			current_time_in_clock_ticks );

		timer->value = mx_convert_clock_ticks_to_seconds(
						time_left_in_clock_ticks );
	}

	return MX_SUCCESSFUL_RESULT;
}

