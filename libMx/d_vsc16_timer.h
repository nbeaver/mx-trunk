/*
 * Name:    d_vsc16_timer.h
 *
 * Purpose: Header file for an MX driver to control a Joerger VSC-16
 *          counter channel as a timer.
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_VSC16_TIMER_H__
#define __D_VSC16_TIMER_H__

typedef struct {
	MX_RECORD *vsc16_record;
	long counter_number;

	double clock_frequency;
} MX_VSC16_TIMER;

#define MXD_VSC16_TIMER_STANDARD_FIELDS \
  {-1, -1, "vsc16_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VSC16_TIMER, vsc16_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "counter_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VSC16_TIMER, counter_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "clock_frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VSC16_TIMER, clock_frequency), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_vsc16_timer_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_vsc16_timer_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_vsc16_timer_open( MX_RECORD *record );

MX_API mx_status_type mxd_vsc16_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_vsc16_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_vsc16_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_vsc16_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_vsc16_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_vsc16_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_vsc16_timer_set_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_vsc16_timer_set_modes_of_associated_counters(
						MX_TIMER *timer );

/* VSC-16 timer specific functions. */

MX_API mx_status_type mxd_vsc16_timer_set_step_down_bit(
					MX_TIMER *timer, int step_down_bit );

extern MX_RECORD_FUNCTION_LIST mxd_vsc16_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_vsc16_timer_timer_function_list;

extern long mxd_vsc16_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_vsc16_timer_rfield_def_ptr;

#endif /* __D_VSC16_TIMER_H__ */

