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
 * Copyright 2000-2001, 2004, 2010, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_TIMER_FANOUT_H__
#define __D_TIMER_FANOUT_H__

#include "mx_timer.h"

/* Flag bits for timer_fanout_flags. */

#define MXF_TFN_SEQUENTIAL_COUNTING_INTERVALS	0x1

/* ==== MX timer fanout data structure ==== */

typedef struct {
	unsigned long timer_fanout_flags;
	long num_timers;
	MX_RECORD **timer_record_array;

	long current_timer_number;
} MX_TIMER_FANOUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_timer_fanout_initialize_driver( MX_DRIVER *driver );
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

extern long mxd_timer_fanout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_timer_fanout_rfield_def_ptr;

#define MXD_TIMER_FANOUT_STANDARD_FIELDS \
  {-1, -1, "timer_fanout_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TIMER_FANOUT, timer_fanout_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "num_timers", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TIMER_FANOUT, num_timers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "timer_record_array", MXFT_RECORD, \
				NULL, 1, {MXU_VARARGS_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_TIMER_FANOUT, timer_record_array), \
	{sizeof(MX_RECORD *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }, \
  \
  {-1, -1, "current_timer_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TIMER_FANOUT, current_timer_number), \
	{0}, NULL, MXFF_READ_ONLY }

#endif /* __D_TIMER_FANOUT_H__ */

