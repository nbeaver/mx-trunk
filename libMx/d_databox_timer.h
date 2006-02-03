/*
 * Name:    d_databox_timer.h
 *
 * Purpose: Header file for MX timer driver to use a Databox timer.
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

#ifndef __D_DATABOX_TIMER_H__
#define __D_DATABOX_TIMER_H__

#include "mx_timer.h"

#define MX_DATABOX_TIMER_MAX_READOUT_LINES	20

/* ==== Databox timer data structure ==== */

typedef struct {
	MX_RECORD *databox_record;
} MX_DATABOX_TIMER;

#define MXD_DATABOX_TIMER_STANDARD_FIELDS \
  {-1, -1, "databox_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX_TIMER, databox_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_databox_timer_initialize_type( long type );
MX_API mx_status_type mxd_databox_timer_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_databox_timer_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_databox_timer_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_databox_timer_read_parms_from_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_databox_timer_write_parms_to_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_databox_timer_open( MX_RECORD *record );
MX_API mx_status_type mxd_databox_timer_close( MX_RECORD *record );
MX_API mx_status_type mxd_databox_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_databox_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_databox_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_databox_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_databox_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_databox_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_databox_timer_set_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_databox_timer_set_modes_of_associated_counters(
						MX_TIMER *timer );

/* Databox timer specific functions. */

extern MX_RECORD_FUNCTION_LIST mxd_databox_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_databox_timer_timer_function_list;

extern mx_length_type mxd_databox_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_databox_timer_rfield_def_ptr;

#endif /* __D_DATABOX_TIMER_H__ */

