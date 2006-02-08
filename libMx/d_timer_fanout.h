/*
 * Name:    d_timer_fanout.h
 *
 * Purpose: Header file for MX timer fanout driver to control multiple
 *          timers as close to simultaneously as possible.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_TIMER_FANOUT_H__
#define __D_TIMER_FANOUT_H__

#include "mx_timer.h"

/* ==== MX timer fanout data structure ==== */

typedef struct {
	mx_length_type num_timers;
	MX_RECORD **timer_record_array;
} MX_TIMER_FANOUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_timer_fanout_initialize_type( long type );
MX_API mx_status_type mxd_timer_fanout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_timer_fanout_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_timer_fanout_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_timer_fanout_start( MX_TIMER *timer );
MX_API mx_status_type mxd_timer_fanout_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_timer_fanout_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_timer_fanout_read( MX_TIMER *timer );
MX_API mx_status_type mxd_timer_fanout_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_timer_fanout_set_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_timer_fanout_get_last_measurement_time(
							MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_timer_fanout_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_timer_fanout_timer_function_list;

extern mx_length_type mxd_timer_fanout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_timer_fanout_rfield_def_ptr;

#define MXD_TIMER_FANOUT_STANDARD_FIELDS \
  {-1, -1, "num_timers", MXFT_LENGTH, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TIMER_FANOUT, num_timers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "timer_record_array", MXFT_RECORD, \
				NULL, 1, {MXU_VARARGS_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_TIMER_FANOUT, timer_record_array), \
	{sizeof(MX_RECORD *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#endif /* __D_TIMER_FANOUT_H__ */

