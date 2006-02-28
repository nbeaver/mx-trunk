/*
 * Name:    s_slit.h
 *
 * Purpose: Header file for slit linear scan.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __S_SLIT_H__
#define __S_SLIT_H__

typedef struct {
	int *is_slit_motor;
	long num_real_motors;
	MX_RECORD **real_motor_record_array;
	double *real_motor_start_position;
	double *real_motor_position;
	mx_bool_type first_step;
} MX_SLIT_SCAN;

MX_API mx_status_type mxs_slit_scan_create_record_structures(
		MX_RECORD *record, MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan);

MX_API mx_status_type mxs_slit_scan_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxs_slit_scan_delete_record( MX_RECORD *record );

MX_API mx_status_type mxs_slit_scan_compute_motor_positions(
				MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan );

MX_API mx_status_type mxs_slit_scan_motor_record_array_move_special(
				MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan,
				long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position,
				MX_MOTOR_MOVE_REPORT_FUNCTION fptr,
				int flags );

MX_API mx_status_type mxs_slit_scan_prepare_for_scan_start( MX_SCAN *scan );

extern MX_LINEAR_SCAN_FUNCTION_LIST mxs_slit_linear_scan_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_slit_scan_function_list;

extern long mxs_slit_linear_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_slit_linear_scan_def_ptr;

#endif /* __S_SLIT_H__ */

