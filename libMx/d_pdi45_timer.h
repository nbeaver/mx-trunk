/*
 * Name:    d_pdi45_timer.h
 *
 * Purpose: Header file for MX timer driver to control a Prairie Digital
 *          Model 45 digital I/O line as an MX timer.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PDI45_TIMER_H__
#define __D_PDI45_TIMER_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pdi45_record;
	int line_number;

	int gated_counters_io_field;
} MX_PDI45_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_pdi45_timer_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_pdi45_timer_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_pdi45_timer_open( MX_RECORD *record );

MX_API mx_status_type mxd_pdi45_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_pdi45_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_pdi45_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_pdi45_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_pdi45_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_pdi45_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_pdi45_timer_set_mode( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_pdi45_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_pdi45_timer_timer_function_list;

extern long mxd_pdi45_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_timer_rfield_def_ptr;

#define MXD_PDI45_TIMER_STANDARD_FIELDS \
  {-1, -1, "pdi45_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_TIMER, pdi45_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "line_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45_TIMER, line_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_PDI45_TIMER_H__ */

