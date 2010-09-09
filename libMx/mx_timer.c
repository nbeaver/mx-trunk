/*
 * Name:    mx_timer.c
 *
 * Purpose: MX function library of generic timer operations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_timer.h"
#include "mx_measurement.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_timer_...
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_timer_get_pointers( MX_RECORD *timer_record,
			MX_TIMER **timer,
			MX_TIMER_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_timer_get_pointers()";

	if ( timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The timer_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( timer_record->mx_class != MXC_TIMER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' passed by '%s' is not a timer.",
			timer_record->name, calling_fname );
	}

	if ( timer != (MX_TIMER **) NULL ) {
		*timer = (MX_TIMER *) (timer_record->record_class_struct);

		if ( *timer == (MX_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TIMER pointer for record '%s' passed by '%s' is NULL",
				timer_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_TIMER_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_TIMER_FUNCTION_LIST *)
				(timer_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_TIMER_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_TIMER_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				timer_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_timer_finish_record_initialization( MX_RECORD *timer_record )
{
	static const char fname[] = "mx_timer_finish_record_initialization()";

	MX_TIMER *timer;

	if ( timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	timer = (MX_TIMER *)(timer_record->record_class_struct);

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TIMER pointer for record '%s' is NULL.",
			timer_record->name );
	}

	timer->value = 0.0;
	timer->busy = 0;
	timer->stop = 0;
	timer->clear = 0;
	timer->last_measurement_time = -1.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_timer_is_busy( MX_RECORD *timer_record, mx_bool_type *busy )
{
	static const char fname[] = "mx_timer_is_busy()";

	MX_TIMER *timer;
	MX_TIMER_FUNCTION_LIST *function_list;
	mx_status_type ( *timer_is_busy_fn ) ( MX_TIMER * );
	mx_status_type mx_status;

	mx_status = mx_timer_get_pointers( timer_record,
					&timer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer_is_busy_fn = function_list->timer_is_busy;

	if ( timer_is_busy_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"timer_is_busy function ptr for timer record '%s' is NULL.",
			timer_record->name );
	}

	mx_status = (*timer_is_busy_fn)( timer );

	if ( busy != (mx_bool_type *) NULL) {
		*busy = timer->busy;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_timer_start( MX_RECORD *timer_record, double seconds )
{
	static const char fname[] = "mx_timer_start()";

	MX_TIMER *timer;
	MX_TIMER_FUNCTION_LIST *function_list;
	mx_status_type ( *timer_start ) ( MX_TIMER * );
	mx_status_type mx_status;

	mx_status = mx_timer_get_pointers( timer_record,
					&timer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer_start = function_list->start;

	if ( timer_start == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"start function ptr for timer record '%s' is NULL.",
			timer_record->name );
	}

	/* If the current timer mode is MXCM_UNKNOWN_MODE, then the timer
	 * mode needs to be initialized.  By default, we initialize it
	 * to MXCM_PRESET_MODE.
	 */

	if ( timer->mode == MXCM_UNKNOWN_MODE ) {

		mx_status = mx_timer_set_mode( timer_record, MXCM_PRESET_MODE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status =
		    mx_timer_set_modes_of_associated_counters( timer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Start the timer. */

	timer->value = seconds;

	mx_status = (*timer_start)( timer );

	timer->last_measurement_time = seconds;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_timer_stop( MX_RECORD *timer_record, double *seconds_left )
{
	static const char fname[] = "mx_timer_stop()";

	MX_TIMER *timer;
	MX_TIMER_FUNCTION_LIST *function_list;
	mx_status_type ( *timer_stop ) ( MX_TIMER * );
	mx_status_type mx_status;

	mx_status = mx_timer_get_pointers( timer_record,
					&timer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->last_measurement_time = -1.0;

	timer_stop = function_list->stop;

	if ( timer_stop == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Stopping timer '%s' is not supported.",
			timer_record->name );
	}

	mx_status = (*timer_stop)( timer );

	if ( seconds_left != (double *) NULL ) {
		*seconds_left = timer->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_timer_clear( MX_RECORD *timer_record )
{
	static const char fname[] = "mx_timer_clear()";

	MX_TIMER *timer;
	MX_TIMER_FUNCTION_LIST *function_list;
	mx_status_type ( *timer_clear ) ( MX_TIMER * );
	mx_status_type mx_status;

	mx_status = mx_timer_get_pointers( timer_record,
					&timer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->last_measurement_time = -1.0;

	timer_clear = function_list->clear;

	if ( timer_clear != NULL ) {
		mx_status = (*timer_clear)( timer );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_timer_read( MX_RECORD *timer_record, double *seconds )
{
	static const char fname[] = "mx_timer_read()";

	MX_TIMER *timer;
	MX_TIMER_FUNCTION_LIST *function_list;
	mx_status_type ( *timer_read ) ( MX_TIMER * );
	mx_status_type mx_status;

	mx_status = mx_timer_get_pointers( timer_record,
					&timer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer_read = function_list->read;

	if ( timer_read != NULL ) {
		mx_status = (*timer_read)( timer );
	} else {
		timer->value = 0.0;
	}

	if ( seconds != (double *) NULL ) {
		*seconds = timer->value;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_timer_get_mode( MX_RECORD *timer_record, long *mode )
{
	static const char fname[] = "mx_timer_get_mode()";

	MX_TIMER *timer;
	MX_TIMER_FUNCTION_LIST *function_list;
	mx_status_type ( *timer_get_mode ) ( MX_TIMER * );
	mx_status_type mx_status;

	mx_status = mx_timer_get_pointers( timer_record,
					&timer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer_get_mode = function_list->get_mode;

	if ( timer_get_mode != NULL ) {
		mx_status = (*timer_get_mode)( timer );
	}

	if ( mode != (long *) NULL ) {
		*mode = timer->mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_timer_set_mode( MX_RECORD *timer_record, long mode )
{
	static const char fname[] = "mx_timer_set_mode()";

	MX_TIMER *timer;
	MX_TIMER_FUNCTION_LIST *function_list;
	mx_status_type ( *timer_set_mode ) ( MX_TIMER * );
	mx_status_type mx_status;

	mx_status = mx_timer_get_pointers( timer_record,
					&timer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->last_measurement_time = -1.0;

	timer->mode = mode;

	timer_set_mode = function_list->set_mode;

	if ( timer_set_mode != NULL ) {
		mx_status = (*timer_set_mode)( timer );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_timer_set_modes_of_associated_counters( MX_RECORD *timer_record )
{
	static const char fname[] =
		"mx_timer_set_modes_of_associated_counters()";

	MX_TIMER *timer;
	MX_TIMER_FUNCTION_LIST *function_list;
	mx_status_type ( *set_modes_of_associated_counters ) ( MX_TIMER * );
	mx_status_type mx_status;

	mx_status = mx_timer_get_pointers( timer_record,
					&timer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_modes_of_associated_counters =
			function_list->set_modes_of_associated_counters;

	if ( set_modes_of_associated_counters != NULL ) {
		mx_status = (*set_modes_of_associated_counters)( timer );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_timer_get_last_measurement_time( MX_RECORD *timer_record,
					double *last_measurement_time )
{
	static const char fname[] = "mx_timer_get_last_measurement_time()";

	MX_TIMER *timer;
	MX_TIMER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_last_measurement_time ) ( MX_TIMER * );
	mx_status_type mx_status;

	mx_status = mx_timer_get_pointers( timer_record,
					&timer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_last_measurement_time = function_list->get_last_measurement_time;

	if ( get_last_measurement_time != NULL ) {
		mx_status = (*get_last_measurement_time)( timer );
	}

	if ( last_measurement_time != (double *) NULL ) {
		*last_measurement_time = timer->last_measurement_time;
	}

	return mx_status;
}

