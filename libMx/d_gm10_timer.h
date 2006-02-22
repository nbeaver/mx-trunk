/*
 * Name:    d_gm10_timer.h
 *
 * Purpose: Header file for Black Cat Systems GM-10, GM-45, GM-50, and
 *          GM-90 radiation detectors.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_GM10_TIMER_H__
#define __D_GM10_TIMER_H__

#include "mx_timer.h"

/* ==== GM-10 timer data structure ==== */

typedef struct {
	long num_scalers;
	MX_RECORD **scaler_record_array;

	MX_CLOCK_TICK finish_time_in_clock_ticks;
} MX_GM10_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_gm10_timer_initialize_type( long type );
MX_API mx_status_type mxd_gm10_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_gm10_timer_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_gm10_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_gm10_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_gm10_timer_stop( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_gm10_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_gm10_timer_timer_function_list;

extern long mxd_gm10_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_gm10_timer_rfield_def_ptr;

#define MXD_GM10_TIMER_STANDARD_FIELDS \
  {-1, -1, "num_scalers", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GM10_TIMER, num_scalers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "scaler_record_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GM10_TIMER, scaler_record_array), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#endif /* __D_GM10_TIMER_H__ */

