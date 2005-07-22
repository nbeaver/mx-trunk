/*
 * Name:    d_spec_timer.h
 *
 * Purpose: Header file for MX timer driver to control Spec timers.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SPEC_TIMER_H__
#define __D_SPEC_TIMER_H__

#include "mx_timer.h"

/* ==== Spec timer data structure ==== */

typedef struct {
	MX_RECORD *spec_server_record;
} MX_SPEC_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_spec_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_spec_timer_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_spec_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_spec_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_spec_timer_stop( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_spec_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_spec_timer_timer_function_list;

extern long mxd_spec_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_spec_timer_rfield_def_ptr;

#define MXD_SPEC_TIMER_STANDARD_FIELDS \
  {-1, -1, "spec_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_TIMER, spec_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_SPEC_TIMER_H__ */

