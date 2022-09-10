/*
 * Name:    d_clock_timer.h
 *
 * Purpose: Header file for MX timer driver based on MX time-of-day clocks.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_CLOCK_TIMER_H__
#define __D_CLOCK_TIMER_H__

#include "mx_timer.h"

/* ==== Software timer data structure ==== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mx_clock_record;

	uint64_t finish_timespec[2];
} MX_CLOCK_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_clock_timer_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_clock_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_clock_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_clock_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_clock_timer_read( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_clock_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_clock_timer_timer_function_list;

extern long mxd_clock_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_clock_timer_rfield_def_ptr;

#define MXD_CLOCK_TIMER_STANDARD_FIELDS \
  {-1, -1, "mx_clock_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_CLOCK_TIMER, mx_clock_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "finish_timespec", MXFT_UINT64, NULL, 1, {2}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_CLOCK_TIMER, finish_timexpec ), \
	{0}, NULL, 0 }

#endif /* __D_CLOCK_TIMER_H__ */
