/*
 * Name:    s_k_power_law.h
 *
 * Purpose: Header file for xafs wavenumber scans for which the counting
 *          time follows a power law.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __S_K_POWER_LAW_H__
#define __S_K_POWER_LAW_H__

typedef struct {
	double base_time;
	double k_start;
	double delta_k;
} MX_K_POWER_LAW_SCAN;

MX_API mx_status_type mxs_k_power_law_scan_create_record_structures(
				MX_RECORD *record,
				MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan );

MX_API mx_status_type mxs_k_power_law_scan_finish_record_initialization(
				MX_RECORD *record );

extern MX_LINEAR_SCAN_FUNCTION_LIST mxs_k_power_law_linear_scan_function_list;

extern long mxs_k_power_law_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_k_power_law_scan_def_ptr;

#endif /* __S_K_POWER_LAW_H__ */
