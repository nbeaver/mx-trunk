/*
 * Name:    d_scipe_timer.h
 *
 * Purpose: Header file for MX timer driver to control SCIPE detectors
 *          used as timers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SCIPE_TIMER_H__
#define __D_SCIPE_TIMER_H__

#include "mx_timer.h"

/* ==== SCIPE timer data structure ==== */

typedef struct {
	MX_RECORD *scipe_server_record;

	char scipe_timer_name[MX_SCIPE_OBJECT_NAME_LENGTH+1];
	double clock_frequency;
} MX_SCIPE_TIMER;

#define MXD_SCIPE_TIMER_STANDARD_FIELDS \
  {-1, -1, "scipe_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_TIMER, scipe_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scipe_timer_name", MXFT_STRING, NULL, \
					1, {MX_SCIPE_OBJECT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_TIMER, scipe_timer_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "clock_frequency", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_TIMER, clock_frequency), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_scipe_timer_initialize_type( long type );
MX_API mx_status_type mxd_scipe_timer_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_scipe_timer_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_scipe_timer_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_scipe_timer_read_parms_from_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_scipe_timer_write_parms_to_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_scipe_timer_open( MX_RECORD *record );
MX_API mx_status_type mxd_scipe_timer_close( MX_RECORD *record );
MX_API mx_status_type mxd_scipe_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_scipe_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_scipe_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_scipe_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_scipe_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_scipe_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_scipe_timer_set_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_scipe_timer_set_modes_of_associated_counters(
						MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_scipe_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_scipe_timer_timer_function_list;

extern mx_length_type mxd_scipe_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scipe_timer_rfield_def_ptr;

#endif /* __D_SCIPE_TIMER_H__ */

