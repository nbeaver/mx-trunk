/*
 * Name:    d_clock_timer.c
 *
 * Purpose: MX timer driver based on MX time-of-day clocks.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_CLOCK_TIMER_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_clock.h"
#include "mx_timer.h"
#include "d_clock_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_clock_timer_record_function_list = {
	NULL,
	mxd_clock_timer_create_record_structures,
	mx_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_clock_timer_timer_function_list = {
	mxd_clock_timer_is_busy,
	mxd_clock_timer_start,
	mxd_clock_timer_stop,
	NULL,
	mxd_clock_timer_read
};

/* Soft timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_clock_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS
};

long mxd_clock_timer_num_record_fields
		= sizeof( mxd_clock_timer_record_field_defaults )
		  / sizeof( mxd_clock_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_clock_timer_rfield_def_ptr
			= &mxd_clock_timer_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_clock_timer_get_pointers( MX_TIMER *timer,
			MX_CLOCK_TIMER **clock_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_clock_timer_get_pointers()";

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

	if ( clock_timer != (MX_CLOCK_TIMER **) NULL ) {

		*clock_timer = (MX_CLOCK_TIMER *)
				(timer->record->record_type_struct);

		if ( *clock_timer == (MX_CLOCK_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_CLOCK_TIMER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_clock_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_clock_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_CLOCK_TIMER *clock_timer = NULL;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	clock_timer = (MX_CLOCK_TIMER *) malloc( sizeof(MX_CLOCK_TIMER) );

	if ( clock_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_CLOCK_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = clock_timer;
	record->class_specific_function_list
			= &mxd_clock_timer_timer_function_list;

	timer->record = record;
	clock_timer->record = record;

	clock_timer->finish_timespec[0] = 0;
	clock_timer->finish_timespec[1] = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_clock_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_clock_timer_is_busy()";

	MX_CLOCK_TIMER *clock_timer = NULL;
	uint64_t current_timespec[2];
	mx_status_type mx_status;

	mx_status = mxd_clock_timer_get_pointers( timer, &clock_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_clock_get_timespec( clock_timer->mx_clock_record,
							current_timespec );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( current_timespec[0] < clock_timer->finish_timespec[0] ) {
		timer->busy = TRUE;
	} else
	if ( current_timespec[0] == clock_timer->finish_timespec[0] ) {
		if ( current_timespec[1] <= clock_timer->finish_timespec[1] ) {
			timer->busy = TRUE;
		} else {
			timer->busy = FALSE;
		}
	} else {
		timer->busy = FALSE;
	}

#if MX_CLOCK_TIMER_DEBUG
	MX_DEBUG(-2,("%s: current time = (%lu,%ld), busy = %d", fname,
			(unsigned long) current_timespec[0],
			(long) current_timespec[1], timer->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_clock_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_clock_timer_start()";

	MX_CLOCK_TIMER *clock_timer = NULL;
	double start_time, finish_time;
	mx_status_type mx_status;

	mx_status = mxd_clock_timer_get_pointers( timer, &clock_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_clock_get_time( clock_timer->mx_clock_record,
							&start_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	finish_time = start_time + timer->value;

	clock_timer->finish_timespec[0] = (uint64_t) finish_time;

	clock_timer->finish_timespec[1] = (uint64_t)
		( 1.0e9 * ( finish_time - clock_timer->finish_timespec[0] ) );

#if MX_CLOCK_TIMER_DEBUG
	MX_DEBUG(-2,("%s: counting for %g seconds", fname, timer->value));

	MX_DEBUG(-2,("%s: starting time = %f, finish time = %f", 
		fname, start_time, finish_time ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_clock_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_clock_timer_stop()";

	MX_CLOCK_TIMER *clock_timer = NULL;
	mx_status_type mx_status;

	mx_status = mxd_clock_timer_get_pointers( timer, &clock_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clock_timer->finish_timespec[0] = 0;
	clock_timer->finish_timespec[1] = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_clock_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_clock_timer_read()";

	MX_CLOCK_TIMER *clock_timer = NULL;
	mx_status_type mx_status;

	mx_status = mxd_clock_timer_get_pointers( timer, &clock_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_clock_get_time( clock_timer->mx_clock_record,
							&(timer->value) );

	return mx_status;
}

