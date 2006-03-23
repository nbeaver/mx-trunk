/*
 * Name:    mx_interval_timer.c
 *
 * Purpose: Functions for using MX virtual interval timers.
 *
 * Author:  William Lavender
 *
 * WARNING: NOT YET FINISHED! (as of March 24, 2006)
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_VIRTUAL_TIMER_DEBUG		FALSE

#include <stdio.h>

#include "mx_util.h"
#include "mx_virtual_timer.h"

static void *mx_master_callback_timer_list = NULL;

static void
mx_master_timer_callback_function( MX_INTERVAL_TIMER *itimer, void *args )
{
	static const char fname[] = "mx_master_timer_callback_function()";

	MX_DEBUG(-2,("%s: itimer = %p", fname, itimer));
	MX_DEBUG(-2,("%s: args = %p", fname, args));
}

MX_EXPORT mx_status_type
mx_virtual_timer_create_master( MX_INTERVAL_TIMER **master_timer )
{
	static const char fname[] = "mx_virtual_timer_create_master()";

	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( master_timer == (MX_INTERVAL_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	mx_status = mx_interval_timer_create( master_timer,
					MXIT_PERIODIC_TIMER,
					mx_master_timer_callback_function,
					mx_master_callback_timer_list );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_virtual_timer_destroy_master( MX_INTERVAL_TIMER *master_timer )
{
	static const char fname[] = "mx_virtual_timer_destroy_master()";

	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( master_timer == (MX_INTERVAL_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERVAL_TIMER pointer passed was NULL." );
	}

	mx_status = mx_interval_timer_destroy( master_timer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_virtual_timer_create( MX_VIRTUAL_TIMER **vtimer,
			MX_INTERVAL_TIMER *master_timer,
			int timer_type,
			MX_VIRTUAL_TIMER_CALLBACK *callback_function,
			void *callback_args )
{
	static const char fname[] = "mx_virtual_timer_create()";

	MX_DEBUG(-2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_destroy( MX_VIRTUAL_TIMER *vtimer )
{
	static const char fname[] = "mx_virtual_timer_destroy()";

	MX_DEBUG(-2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_is_busy( MX_VIRTUAL_TIMER *vtimer,
			mx_bool_type *busy )
{
	static const char fname[] = "mx_virtual_timer_is_busy()";

	MX_DEBUG(-2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_start( MX_VIRTUAL_TIMER *vtimer,
			double timer_period_in_seconds )
{
	static const char fname[] = "mx_virtual_timer_start()";

	MX_DEBUG(-2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_stop( MX_VIRTUAL_TIMER *vtimer,
			double *seconds_left )
{
	static const char fname[] = "mx_virtual_timer_stop()";

	MX_DEBUG(-2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_virtual_timer_read( MX_VIRTUAL_TIMER *vtimer,
			double *seconds_till_expiration )
{
	static const char fname[] = "mx_virtual_timer_read()";

	MX_DEBUG(-2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

