/*
 * Name:    m_time.h
 *
 * Purpose: Header file for preset time measurements during scans.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __M_TIME_H__
#define __M_TIME_H__

typedef struct {
	MX_RECORD *timer_record;
	double integration_time;
} MX_MEASUREMENT_PRESET_TIME;

MX_API mx_status_type mxm_preset_time_configure(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_time_deconfigure(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_time_prescan_processing(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_time_postscan_processing(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_time_preslice_processing(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_time_postslice_processing(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_time_acquire_data(
					MX_MEASUREMENT *measurement );

extern MX_MEASUREMENT_FUNCTION_LIST mxm_preset_time_function_list;

#endif /* __M_TIME_H__ */

