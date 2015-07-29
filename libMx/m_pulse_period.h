/*
 * Name:    m_pulse_period.h
 *
 * Purpose: Header file for preset pulse period measurements during scans.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __M_PULSE_PERIOD_H__
#define __M_PULSE_PERIOD_H__

typedef struct {
	MX_RECORD *pulse_generator_record;
	double pulse_period;
} MX_MEASUREMENT_PRESET_PULSE_PERIOD;

MX_API mx_status_type mxm_preset_pulse_period_configure(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_pulse_period_deconfigure(
					MX_MEASUREMENT *measurement );
MX_API mx_status_type mxm_preset_pulse_period_acquire_data(
					MX_MEASUREMENT *measurement );

extern MX_MEASUREMENT_FUNCTION_LIST mxm_preset_pulse_period_function_list;

#endif /* __M_PULSE_PERIOD_H__ */

