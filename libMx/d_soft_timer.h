/*
 * Name:    d_soft_timer.h
 *
 * Purpose: Header file for MX timer driver to control software timers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_TIMER_H__
#define __D_SOFT_TIMER_H__

#include "mx_timer.h"

/* ==== Software timer data structure ==== */

typedef struct {
	MX_CLOCK_TICK finish_time_in_clock_ticks;
} MX_SOFT_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_soft_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_timer_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_soft_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_soft_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_soft_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_soft_timer_read( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_soft_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_soft_timer_timer_function_list;

extern mx_length_type mxd_soft_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_timer_rfield_def_ptr;

#endif /* __D_SOFT_TIMER_H__ */
