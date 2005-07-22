/*
 * Name:    m_count.h
 *
 * Purpose: Header file for preset count measurements during scans.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __M_COUNT_H__
#define __M_COUNT_H__

typedef struct {
	MX_RECORD *scaler_record;
	long preset_count;
} MX_MEASUREMENT_PRESET_COUNT;

MX_API mx_status_type mxm_preset_count_configure(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_count_deconfigure(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_count_prescan_processing(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_count_postscan_processing(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_count_preslice_processing(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_count_postslice_processing(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_count_measure_data(
					MX_MEASUREMENT *measurement );

extern MX_MEASUREMENT_FUNCTION_LIST mxm_preset_count_function_list;

#endif /* __M_COUNT_H__ */

