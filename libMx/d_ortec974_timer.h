/*
 * Name:    d_ortec974_timer.h
 *
 * Purpose: Header file for MX timer driver to control the
 *          timer in EG&G Ortec 974 counter/timer units.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ORTEC974_TIMER_H__
#define __D_ORTEC974_TIMER_H__

#include "mx_timer.h"

/* ==== Ortec 974 timer data structure ==== */

typedef struct {
	MX_RECORD *ortec974_record;
	int timer_mode;
	double seconds_per_tick;
} MX_ORTEC974_TIMER;

#define MX_974_TIMER_MODE_EXTERNAL	1
#define MX_974_TIMER_MODE_MINUTES	2	/* 1 minute per tick */
#define MX_974_TIMER_MODE_SECONDS	3	/* 0.1 seconds per tick */

#define MX_974_MAX_PRESET_COUNT		(9.0e7)

/* Define all of the interface functions. */

MX_API mx_status_type mxd_ortec974_timer_initialize_type( long type );
MX_API mx_status_type mxd_ortec974_timer_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_ortec974_timer_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_ortec974_timer_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_ortec974_timer_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_ortec974_timer_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_ortec974_timer_open( MX_RECORD *record );
MX_API mx_status_type mxd_ortec974_timer_close( MX_RECORD *record );
MX_API mx_status_type mxd_ortec974_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_ortec974_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_ortec974_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_ortec974_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_ortec974_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_ortec974_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_ortec974_timer_set_mode( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_ortec974_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_ortec974_timer_timer_function_list;

extern long mxd_ortec974_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ortec974_timer_rfield_def_ptr;

#define MXD_ORTEC974_TIMER_STANDARD_FIELDS \
  {-1, -1, "ortec974_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ORTEC974_TIMER, ortec974_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_ORTEC974_TIMER_H__ */

