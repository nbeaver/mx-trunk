/*
 * Name:    d_bkprecision_912x_timer.h
 *
 * Purpose: Header file for MX timer driver for the BK Precision
 *          912x series of power supplies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BKPRECISION_912X_TIMER_H__
#define __D_BKPRECISION_912X_TIMER_H__

#include "mx_timer.h"

typedef struct {
	MX_RECORD *bkprecision_912x_record;
} MX_BKPRECISION_912X_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_bkprecision_912x_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bkprecision_912x_timer_open( MX_RECORD *record );

MX_API mx_status_type mxd_bkprecision_912x_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_bkprecision_912x_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_bkprecision_912x_timer_stop( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_bkprecision_912x_timer_timer_function_list;

extern long mxd_bkprecision_912x_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_timer_rfield_def_ptr;

#define MXD_BKPRECISION_912X_TIMER_STANDARD_FIELDS \
  {-1, -1, "bkprecision_912x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BKPRECISION_912X_TIMER, bkprecision_912x_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_BKPRECISION_912X_TIMER_H__ */

