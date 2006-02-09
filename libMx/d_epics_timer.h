/*
 * Name:    d_epics_timer.h
 *
 * Purpose: Header file for MX timer driver to control EPICS scalers
 *          used as timers.
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_TIMER_H__
#define __D_EPICS_TIMER_H__

#include "mx_timer.h"

/* ==== EPICS timer data structure ==== */

typedef struct {
	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	double clock_frequency;
	double epics_record_version;

	MX_EPICS_PV cnt_pv;
	MX_EPICS_PV freq_pv;
	MX_EPICS_PV mode_pv;
	MX_EPICS_PV t_pv;
	MX_EPICS_PV tp_pv;
	MX_EPICS_PV vers_pv;

	int num_epics_counters;
	MX_EPICS_PV *gate_control_pv_array;
} MX_EPICS_TIMER;

#define MXD_EPICS_TIMER_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_TIMER, epics_record_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "epics_record_version", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_TIMER, epics_record_version), \
	{0}, NULL, 0}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_epics_timer_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_epics_timer_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_epics_timer_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_epics_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_epics_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_epics_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_epics_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_epics_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_epics_timer_set_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_epics_timer_set_modes_of_associated_counters(
						MX_TIMER *timer );

/* EPICS timer specific functions. */

MX_API mx_status_type mxd_epics_timer_set_step_down_bit(
					MX_TIMER *timer, int step_down_bit );

extern MX_RECORD_FUNCTION_LIST mxd_epics_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_epics_timer_timer_function_list;

extern mx_length_type mxd_epics_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_timer_rfield_def_ptr;

#endif /* __D_EPICS_TIMER_H__ */

