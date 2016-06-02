/*
 * Name:    d_timer_fanout.c
 *
 * Purpose: MX timer fanout driver to control multiple MX timers as if they
 *          were one timer.
 *
 * Author:  William Lavender
 *
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING 
 *
 * This driver does NOT attempt to ensure that all the timers start at
 * exactly the same time.  This means that devices gated by different
 * timers may not be gated on for exactly the same time interval, although
 * the lengths of time they are gated on for should be the same.  The
 * result is that you may get SYSTEMATIC ERRORS if you do not use this
 * driver intelligently.  It is up to you to decide whether or not this
 * makes a difference to the experiment you are performing.  The best
 * solution is to make sure that all of your measuring devices are gated
 * by the same timer, but if that is not possible, then this driver may
 * be useful as a stopgap.
 *
 * Caveat emptor.
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2004, 2006, 2008, 2010, 2013
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "d_timer_fanout.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_timer_fanout_record_function_list = {
	mxd_timer_fanout_initialize_driver,
	mxd_timer_fanout_create_record_structures,
	mxd_timer_fanout_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_timer_fanout_timer_function_list = {
	mxd_timer_fanout_is_busy,
	mxd_timer_fanout_start,
	mxd_timer_fanout_stop,
	mxd_timer_fanout_clear,
	mxd_timer_fanout_read,
	mxd_timer_fanout_get_mode,
	mxd_timer_fanout_set_mode,
	NULL,
	mxd_timer_fanout_get_last_measurement_time
};

/* MX timer fanout data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_timer_fanout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_TIMER_FANOUT_STANDARD_FIELDS
};

long mxd_timer_fanout_num_record_fields
		= sizeof( mxd_timer_fanout_record_field_defaults )
		  / sizeof( mxd_timer_fanout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_timer_fanout_rfield_def_ptr
			= &mxd_timer_fanout_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_timer_fanout_get_pointers( MX_TIMER *timer,
			MX_TIMER_FANOUT **timer_fanout,
			const char *calling_fname )
{
	static const char fname[] = "mxd_timer_fanout_get_pointers()";

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

	if ( timer->record->mx_type != MXT_TIM_FANOUT ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The timer '%s' passed by '%s' is not a timer fanout.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			timer->record->name, calling_fname,
			timer->record->mx_superclass,
			timer->record->mx_class,
			timer->record->mx_type );
	}

	if ( timer_fanout != (MX_TIMER_FANOUT **) NULL ) {

		*timer_fanout = (MX_TIMER_FANOUT *)
				(timer->record->record_type_struct);

		if ( *timer_fanout == (MX_TIMER_FANOUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_TIMER_FANOUT pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_timer_fanout_initialize_driver( MX_DRIVER *driver )
{
        static const char fname[] = "mxs_timer_fanout_initialize_driver()";

        MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
        long num_timers_varargs_cookie;
        mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

        mx_status = mx_find_record_field_defaults_index( driver,
                        			"num_timers",
						&referenced_field_index );

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        mx_status = mx_construct_varargs_cookie(
                        referenced_field_index, 0, &num_timers_varargs_cookie);

        if ( mx_status.code != MXE_SUCCESS )
                return mx_status;

        MX_DEBUG( 2,("%s: num_timers varargs cookie = %ld",
                        fname, num_timers_varargs_cookie));

	mx_status = mx_find_record_field_defaults( driver,
						"timer_record_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_timers_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_timer_fanout_create_record_structures()";

	MX_TIMER *timer;
	MX_TIMER_FANOUT *timer_fanout = NULL;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	timer_fanout = (MX_TIMER_FANOUT *) malloc( sizeof(MX_TIMER_FANOUT) );

	if ( timer_fanout == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER_FANOUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = timer_fanout;
	record->class_specific_function_list
			= &mxd_timer_fanout_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_timer_fanout_finish_record_initialization()";

	MX_TIMER *timer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	timer->mode = MXCM_UNKNOWN_MODE;

	mx_status = mx_timer_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_timer_fanout_is_busy()";

	MX_TIMER_FANOUT *timer_fanout = NULL;
	MX_RECORD *child_timer_record;
	long i;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_timer_fanout_get_pointers(timer, &timer_fanout, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->busy = FALSE;

	for ( i = 0; i < timer_fanout->num_timers; i++ ) {

		child_timer_record = (timer_fanout->timer_record_array)[i];

		mx_status = mx_timer_is_busy( child_timer_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( busy ) {
			timer->busy = TRUE;
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_timer_fanout_start()";

	MX_TIMER_FANOUT *timer_fanout = NULL;
	MX_RECORD *child_timer_record;
	long i;
	double seconds_to_count;
	mx_status_type mx_status;

	mx_status = mxd_timer_fanout_get_pointers(timer, &timer_fanout, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

	for ( i = 0; i < timer_fanout->num_timers; i++ ) {

		child_timer_record = (timer_fanout->timer_record_array)[i];

		mx_status = mx_timer_start( child_timer_record, seconds_to_count );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_timer_fanout_stop()";

	MX_TIMER_FANOUT *timer_fanout = NULL;
	MX_RECORD *child_timer_record;
	long i;
	double seconds_left;
	mx_status_type mx_status;

	mx_status = mxd_timer_fanout_get_pointers(timer, &timer_fanout, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->value = 0.0;

	for ( i = 0; i < timer_fanout->num_timers; i++ ) {

		child_timer_record = (timer_fanout->timer_record_array)[i];

		mx_status = mx_timer_stop( child_timer_record, &seconds_left );

		/* Do not stop on errors. */

		if ( mx_status.code == MXE_SUCCESS ) {
			if ( seconds_left > timer->value ) {
				timer->value = seconds_left;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_clear( MX_TIMER *timer )
{
	static const char fname[] = "mxd_timer_fanout_clear()";

	MX_TIMER_FANOUT *timer_fanout = NULL;
	MX_RECORD *child_timer_record;
	long i;
	mx_status_type mx_status;

	mx_status = mxd_timer_fanout_get_pointers(timer, &timer_fanout, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < timer_fanout->num_timers; i++ ) {

		child_timer_record = (timer_fanout->timer_record_array)[i];

		mx_status = mx_timer_clear( child_timer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_timer_fanout_read()";

	MX_TIMER_FANOUT *timer_fanout = NULL;
	MX_RECORD *child_timer_record;
	long i;
	double seconds;
	mx_status_type mx_status;

	mx_status = mxd_timer_fanout_get_pointers(timer, &timer_fanout, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->value = 0.0;

	for ( i = 0; i < timer_fanout->num_timers; i++ ) {

		child_timer_record = (timer_fanout->timer_record_array)[i];

		mx_status = mx_timer_read( child_timer_record, &seconds );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( seconds > timer->value ) {
			timer->value = seconds;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_get_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_timer_fanout_get_mode()";

	MX_TIMER_FANOUT *timer_fanout = NULL;
	MX_RECORD *child_timer_record;
	long i, mode;
	mx_bool_type first_time;
	mx_status_type mx_status;

	mx_status = mxd_timer_fanout_get_pointers(timer, &timer_fanout, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	first_time = TRUE;

	for ( i = 0; i < timer_fanout->num_timers; i++ ) {

		child_timer_record = (timer_fanout->timer_record_array)[i];

		mx_status = mx_timer_get_mode( child_timer_record, &mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( first_time ) {
			timer->mode = mode;
			first_time = FALSE;
		} else {
			if ( mode != timer->mode ) {
				/* Send an error message, but do not stop. */

				(void) mx_error( MXE_TYPE_MISMATCH, fname,
		"The timers controlled by timer fanout '%s' do not all have "
		"the same timer mode.  Operation may be erratic as a result.",
					timer->record->name );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_timer_fanout_set_mode()";

	MX_TIMER_FANOUT *timer_fanout = NULL;
	MX_RECORD *child_timer_record;
	long i, mode;
	mx_status_type mx_status;

	mx_status = mxd_timer_fanout_get_pointers(timer, &timer_fanout, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mode = timer->mode;

	for ( i = 0; i < timer_fanout->num_timers; i++ ) {

		child_timer_record = (timer_fanout->timer_record_array)[i];

		mx_status = mx_timer_set_mode( child_timer_record, mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_timer_fanout_get_last_measurement_time( MX_TIMER *timer )
{
	static const char fname[] =
			"mxd_timer_fanout_get_last_measurement_time()";

#if 0
	return mx_error( MXE_UNSUPPORTED, fname,
	"Do not attempt to get the last measurement time from "
	"timer fanout record '%s' .  There is no way to guarantee "
	"that this value is correct.  If you are getting this message "
	"due to having configured timer fanout record '%s' as the timer "
	"for a scaler or analog input record, use the raw timer that "
	"_really_ gates the module instead.", timer->record->name,
					timer->record->name );
#else
	MX_TIMER_FANOUT *timer_fanout = NULL;
	MX_RECORD *child_timer_record;
	long i;
	double last_measurement_time, last_measurement_time_sum;
	mx_status_type mx_status;

	mx_status = mxd_timer_fanout_get_pointers(timer, &timer_fanout, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If there are more than one timer, then it is not completely
	 * obvious what to do if the different timers have different
	 * last measurement times.  For lack of a better idea, we average
	 * the reported times.  Nevertheless, in general it is better for
	 * you to configure your scaler and analog input records to list
	 * the timer that directly gates them, rather than listing this
	 * timer fanout record.
	 */

	last_measurement_time_sum = 0.0;

	for ( i = 0; i < timer_fanout->num_timers; i++ ) {

		child_timer_record = (timer_fanout->timer_record_array)[i];

		mx_status = mx_timer_get_last_measurement_time(
				child_timer_record, &last_measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		last_measurement_time_sum += last_measurement_time;
	}

	timer->last_measurement_time = mx_divide_safely(
					last_measurement_time_sum,
					(double) timer_fanout->num_timers );

	return MX_SUCCESSFUL_RESULT;
#endif
}

