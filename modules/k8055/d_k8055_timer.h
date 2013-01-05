/*
 * Name:    d_k8055_timer.h
 *
 * Purpose: Header file for Velleman K8055 counter channels used as timers.
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_K8055_TIMER_H__
#define __D_K8055_TIMER_H__

#include "mx_timer.h"

/* ==== K8055 timer data structure ==== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *k8055_record;

	struct timespec finish_timespec;
} MX_K8055_TIMER;

#define MXD_K8055_TIMER_STANDARD_FIELDS \
  {-1, -1, "k8055_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_K8055_TIMER, k8055_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_k8055_timer_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_k8055_timer_open( MX_RECORD *record );

MX_API mx_status_type mxd_k8055_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_k8055_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_k8055_timer_stop( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_k8055_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_k8055_timer_timer_function_list;

extern long mxd_k8055_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_k8055_timer_rfield_def_ptr;

#endif /* __D_K8055_TIMER_H__ */

