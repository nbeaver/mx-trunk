/*
 * Name:    d_mcs_timer.h
 *
 * Purpose: Header file for MX timer driver to use an MCS as a timer.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCS_TIMER_H__
#define __D_MCS_TIMER_H__

#include "mx_timer.h"

/* ==== MCS timer data structure ==== */

typedef struct {
	MX_RECORD *mcs_record;
	double clock_frequency;
} MX_MCS_TIMER;

#define MXD_MCS_TIMER_STANDARD_FIELDS \
  {-1, -1, "mcs_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCS_TIMER, mcs_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mcs_timer_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_mcs_timer_finish_record_initialization(
						MX_RECORD *record );

MX_API mx_status_type mxd_mcs_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_mcs_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_mcs_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_mcs_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_mcs_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_mcs_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_mcs_timer_set_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_mcs_timer_set_modes_of_associated_counters(
						MX_TIMER *timer );
MX_API mx_status_type mxd_mcs_timer_get_last_measurement_time(
						MX_TIMER *timer );

/* MCS timer specific functions. */

extern MX_RECORD_FUNCTION_LIST mxd_mcs_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_mcs_timer_timer_function_list;

extern mx_length_type mxd_mcs_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mcs_timer_rfield_def_ptr;

#endif /* __D_MCS_TIMER_H__ */

