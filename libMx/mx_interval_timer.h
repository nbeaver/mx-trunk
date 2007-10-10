/*
 * Name:    mx_interval_timer.h
 *
 * Purpose: Header file for using MX interval timers.  The interval timers
 *          are modeled after the Posix timer_create(), timer_settime(), etc.
 *          routines.  However, the Posix timers are not available on all
 *          supported MX platforms, so we have to provide a wrapper.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2004, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_INTERVAL_TIMER_H__
#define __MX_INTERVAL_TIMER_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Values for 'timer_type'. */

#define MXIT_ONE_SHOT_TIMER	1
#define MXIT_PERIODIC_TIMER	2

struct mx_interval_timer_struct {
	int timer_type;
	double timer_period;
	unsigned long num_overruns;
	void (*callback_function)( struct mx_interval_timer_struct *itimer,
				void *callback_args );
	void *callback_args;
	void *private_ptr;
};

typedef struct mx_interval_timer_struct MX_INTERVAL_TIMER;

typedef void (MX_INTERVAL_TIMER_CALLBACK)(
			struct mx_interval_timer_struct *, void * );

MX_API mx_status_type
mx_interval_timer_create( MX_INTERVAL_TIMER **itimer,
				int timer_type,
				MX_INTERVAL_TIMER_CALLBACK *callback_function,
				void *callback_args );

MX_API mx_status_type
mx_interval_timer_destroy( MX_INTERVAL_TIMER *itimer );

MX_API mx_status_type
mx_interval_timer_is_busy( MX_INTERVAL_TIMER *itimer, mx_bool_type *busy );

MX_API mx_status_type
mx_interval_timer_start( MX_INTERVAL_TIMER *itimer,
				double timer_period_in_seconds );

MX_API mx_status_type
mx_interval_timer_stop( MX_INTERVAL_TIMER *itimer, double *seconds_left );

MX_API mx_status_type
mx_interval_timer_read( MX_INTERVAL_TIMER *itimer,
				double *seconds_till_expiration );

#ifdef __cplusplus
}
#endif

#endif /* __MX_INTERVAL_TIMER_H__ */

