/*
 * Name:    s_input.h
 *
 * Purpose: Header file for input device linear scan.
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

#ifndef __S_INPUT_H__
#define __S_INPUT_H__

MX_API mx_status_type mxs_input_scan_create_record_structures(
		MX_RECORD *record, MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan);

MX_API mx_status_type mxs_input_scan_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxs_input_scan_compute_motor_positions(
				MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan );

extern MX_LINEAR_SCAN_FUNCTION_LIST mxs_input_linear_scan_function_list;

extern long mxs_input_linear_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_input_linear_scan_def_ptr;

#endif /* __S_INPUT_H__ */

