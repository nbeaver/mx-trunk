/*
 * Name:    d_rtc018.h
 *
 * Purpose: Header file for MX timer driver to control DSP RTC-018 timers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */


#ifndef __D_RTC018_H__
#define __D_RTC018_H__

#include "mx_timer.h"

/* ==== RTC-018 timer data structure ==== */

typedef struct {
	MX_RECORD *camac_record;
	int slot;
	int saved_step_down_bit;
	double seconds_per_tick;
} MX_RTC018;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_rtc018_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_rtc018_finish_record_initialization(
					MX_RECORD *record );

MX_API mx_status_type mxd_rtc018_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_rtc018_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_rtc018_timer_stop( MX_TIMER *timer );

/* RTC-018 specific functions. */

MX_API mx_status_type mxd_rtc018_set_step_down_bit(
					MX_TIMER *timer, int step_down_bit );

extern MX_RECORD_FUNCTION_LIST mxd_rtc018_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_rtc018_timer_function_list;

extern long mxd_rtc018_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_rtc018_rfield_def_ptr;

#define MXD_RTC018_STANDARD_FIELDS \
  {-1, -1, "camac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RTC018, camac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "slot", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RTC018, slot), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_RTC018_H__ */

