/*
 * Name:    mx_virtual_timer.h
 *
 * Purpose: Header file for using MX virtual interval timers, which are
 *          built on top of master MX interval timers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_VIRTUAL_TIMER_H__
#define __MX_VIRTUAL_TIMER_H__

#include "mx_stdint.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_interval_timer.h"

struct mx_virtual_timer_struct {
	MX_INTERVAL_TIMER *master_timer;
	int timer_type;
	struct timespec timer_period;
	unsigned long num_overruns;
	void (*callback_function)( struct mx_virtual_timer_struct *vtimer,
				void *callback_args );
	void *callback_args;
	void *private_ptr;
};

typedef struct mx_virtual_timer_struct MX_VIRTUAL_TIMER;

typedef void (MX_VIRTUAL_TIMER_CALLBACK)(
			struct mx_virtual_timer_struct *, void * );

/*----*/

MX_API mx_status_type
mx_virtual_timer_create_master( MX_INTERVAL_TIMER **master_timer,
				double master_timer_period_in_seconds );

MX_API mx_status_type
mx_virtual_timer_destroy_master( MX_INTERVAL_TIMER *master_timer );

/*----*/

MX_API mx_status_type
mx_virtual_timer_create( MX_VIRTUAL_TIMER **vtimer,
				MX_INTERVAL_TIMER *master_timer,
				int timer_type,
				MX_VIRTUAL_TIMER_CALLBACK *callback_function,
				void *callback_args );

MX_API mx_status_type
mx_virtual_timer_destroy( MX_VIRTUAL_TIMER *vtimer );

MX_API mx_status_type
mx_virtual_timer_is_busy( MX_VIRTUAL_TIMER *vtimer, mx_bool_type *busy );

MX_API mx_status_type
mx_virtual_timer_start( MX_VIRTUAL_TIMER *vtimer,
				double timer_period_in_seconds );

#define mx_virtual_timer_restart(v) \
		mx_virtual_timer_start( (v), -1.0 )

MX_API mx_status_type
mx_virtual_timer_stop( MX_VIRTUAL_TIMER *vtimer, double *seconds_left );

MX_API mx_status_type
mx_virtual_timer_read( MX_VIRTUAL_TIMER *vtimer,
				double *seconds_till_expiration );

#ifdef __cplusplus
}
#endif

#endif /* __MX_VIRTUAL_TIMER_H__ */

