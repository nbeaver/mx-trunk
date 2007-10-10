/*
 * Name:    mx_scan_linear.h
 *
 * Purpose: Header file for MX_LINEAR_SCAN step scan records.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_SCAN_LINEAR_H_
#define _MX_SCAN_LINEAR_H_

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_SCAN *scan;	/* Pointer to the parent scan superclass structure. */

	double *start_position;
	double *step_size;
	long *num_measurements;
	long *step_number;

	void *record_type_struct;
} MX_LINEAR_SCAN;

typedef struct {
	mx_status_type ( *create_record_structures ) (
				MX_RECORD *record,
				MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan );
	mx_status_type ( *finish_record_initialization )(
				MX_RECORD *record );
	mx_status_type ( *delete_record ) (
				MX_RECORD *record );
	mx_status_type ( *prepare_for_scan_start ) (
				MX_SCAN *scan );
	mx_status_type ( *compute_motor_positions ) (
				MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan );
	mx_status_type ( *motor_record_array_move_special ) (
				MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan,
				long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position,
				MX_MOTOR_MOVE_REPORT_FUNCTION fptr,
				unsigned long flags );
} MX_LINEAR_SCAN_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mxs_linear_scan_initialize_type( long );
MX_API_PRIVATE mx_status_type mxs_linear_scan_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxs_linear_scan_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxs_linear_scan_delete_record(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxs_linear_scan_print_scan_structure(
					FILE *file, MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxs_linear_scan_prepare_for_scan_start(
							MX_SCAN *scan );
MX_API_PRIVATE mx_status_type mxs_linear_scan_execute_scan_body(
							MX_SCAN *scan );
MX_API_PRIVATE mx_status_type mxs_linear_scan_cleanup_after_scan_end(
							MX_SCAN *scan );

extern MX_RECORD_FUNCTION_LIST mxs_linear_scan_record_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_linear_scan_scan_function_list;

#define MX_LINEAR_SCAN_STANDARD_FIELDS \
  {-1, -1, "start_position", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_LINEAR_SCAN, start_position), \
	{sizeof(double)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  {-1, -1, "step_size", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_LINEAR_SCAN, step_size), \
	{sizeof(double)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  {-1, -1, "num_measurements", MXFT_LONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_LINEAR_SCAN, num_measurements), \
	{sizeof(long)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  {-1, -1, "step_number", MXFT_LONG, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_LINEAR_SCAN, step_number), \
	{sizeof(long)}, NULL, MXFF_VARARGS}

#ifdef __cplusplus
}
#endif

#endif /* _MX_SCAN_LINEAR_H_ */

