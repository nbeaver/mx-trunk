/*
 * Name:    d_mca_timer.h
 *
 * Purpose: Header file for MX timer driver to control MX MCA-based timers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCA_TIMER_H__
#define __D_MCA_TIMER_H__

#include "mx_timer.h"

/* ==== MX MCA timer data structure ==== */

typedef struct {
	MX_RECORD *mca_record;
	double preset_time;
	int use_real_time;
} MX_MCA_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mca_timer_initialize_type( long type );
MX_API mx_status_type mxd_mca_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mca_timer_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mca_timer_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_mca_timer_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_mca_timer_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_mca_timer_open( MX_RECORD *record );
MX_API mx_status_type mxd_mca_timer_close( MX_RECORD *record );
MX_API mx_status_type mxd_mca_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_mca_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_mca_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_mca_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_mca_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_mca_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_mca_timer_set_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_mca_timer_get_last_measurement_time(MX_TIMER *timer);

extern MX_RECORD_FUNCTION_LIST mxd_mca_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_mca_timer_timer_function_list;

extern long mxd_mca_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mca_timer_rfield_def_ptr;

#define MXD_MCA_TIMER_STANDARD_FIELDS \
  {-1, -1, "mca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_TIMER, mca_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "use_real_time", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCA_TIMER, use_real_time), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_MCA_TIMER_H__ */

