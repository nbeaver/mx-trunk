/*
 * Name:    d_xia_dxp_timer.h
 *
 * Purpose: Header file for MX timer driver to control XIA DXP MCA timers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_XIA_DXP_TIMER_H__
#define __D_XIA_DXP_TIMER_H__

#include "mx_timer.h"

/* ==== MX MCA timer data structure ==== */

typedef struct {
	MX_RECORD *xia_dxp_record;
	MX_RECORD *mca_record;
	mx_bool_type use_real_time;

	double preset_time;
} MX_XIA_DXP_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_xia_dxp_timer_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_xia_dxp_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_xia_dxp_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_xia_dxp_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_xia_dxp_timer_clear( MX_TIMER *timer );
MX_API mx_status_type mxd_xia_dxp_timer_read( MX_TIMER *timer );
MX_API mx_status_type mxd_xia_dxp_timer_get_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_xia_dxp_timer_set_mode( MX_TIMER *timer );
MX_API mx_status_type mxd_xia_dxp_timer_get_last_measurement_time(
							MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_xia_dxp_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_xia_dxp_timer_timer_function_list;

extern long mxd_xia_dxp_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_xia_dxp_timer_rfield_def_ptr;

#define MXD_XIA_DXP_TIMER_STANDARD_FIELDS \
  {-1, -1, "xia_dxp_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_TIMER, xia_dxp_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "mca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_TIMER, mca_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "use_real_time", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_DXP_TIMER, use_real_time), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_XIA_DXP_TIMER_H__ */

