/*
 * Name:    d_interval_timer.h
 *
 * Purpose: Header file for using an MX interval timer as a timer device.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_INTERVAL_TIMER_H__
#define __D_INTERVAL_TIMER_H__

#include "mx_timer.h"

/* ==== Interval timer data structure ==== */

typedef struct {
	MX_INTERVAL_TIMER *itimer;
} MX_INTERVAL_TIMER_DEVICE;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_interval_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_interval_timer_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_interval_timer_open( MX_RECORD *record );
MX_API mx_status_type mxd_interval_timer_close( MX_RECORD *record );

MX_API mx_status_type mxd_interval_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_interval_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_interval_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_interval_timer_read( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_interval_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_interval_timer_timer_function_list;

extern long mxd_interval_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_interval_timer_rfield_def_ptr;

#define MXD_INTERVAL_TIMER_STANDARD_FIELDS 

#endif /* __D_INTERVAL_TIMER_H__ */

