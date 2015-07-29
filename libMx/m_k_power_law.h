/*
 * Name:    m_k_power_law.h
 *
 * Purpose: Header file for XAFS K power law measurements during scans.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __M_K_POWER_LAW_H__
#define __M_K_POWER_LAW_H__

typedef struct {
	MX_RECORD *timer_record;
	double base_time;
	double k_start;
	double delta_k;
	double exponent;

	long measurement_number;
} MX_MEASUREMENT_K_POWER_LAW;

MX_API mx_status_type mxm_k_power_law_configure( MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_k_power_law_deconfigure(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_k_power_law_acquire_data(
					MX_MEASUREMENT *measurement );

extern MX_MEASUREMENT_FUNCTION_LIST mxm_k_power_law_function_list;

#endif /* __M_K_POWER_LAW_H__ */

