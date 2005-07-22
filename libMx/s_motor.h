/*
 * Name:    s_motor.h
 *
 * Purpose: Header file for motor linear scan.
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

#ifndef __S_MOTOR_H__
#define __S_MOTOR_H__

MX_API mx_status_type mxs_motor_scan_create_record_structures(
		MX_RECORD *record, MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan);

MX_API mx_status_type mxs_motor_scan_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxs_motor_scan_compute_motor_positions(
				MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan );

extern MX_LINEAR_SCAN_FUNCTION_LIST mxs_motor_linear_scan_function_list;

extern long mxs_motor_linear_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_motor_linear_scan_def_ptr;

#endif /* __S_MOTOR_H__ */

