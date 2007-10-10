/*
 * Name:    mx_scan_quick.h
 *
 * Purpose: Header file for MX_QUICK_SCAN scan records.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_SCAN_QUICK_H_
#define _MX_SCAN_QUICK_H_

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_SCAN *scan;	/* Pointer to the parent scan superclass structure. */

	double *old_motor_speed;
	double *start_position;
	double *end_position;
	long requested_num_measurements;
	long actual_num_measurements;

	mx_bool_type use_synchronous_motion_mode;
	mx_bool_type *saved_synchronous_motion_mode;

	double estimated_scan_duration;		/* in seconds */
} MX_QUICK_SCAN;

MX_API_PRIVATE mx_status_type mx_quick_scan_initialize_type( long type );

MX_API_PRIVATE mx_status_type mx_quick_scan_print_scan_structure(
					FILE *file, MX_RECORD *record );

MX_API_PRIVATE mx_status_type mx_quick_scan_set_new_motor_speed(
				MX_QUICK_SCAN *quick_scan,
				long motor_index );

MX_API_PRIVATE mx_status_type mx_quick_scan_restore_old_motor_speed(
				MX_QUICK_SCAN *quick_scan,
				long motor_index );

MX_API_PRIVATE double mx_quick_scan_get_measurement_time(
				MX_QUICK_SCAN *quick_scan );

#define MX_QUICK_SCAN_STANDARD_FIELDS \
  {-1, -1, "old_motor_speed", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_QUICK_SCAN, old_motor_speed),\
	{sizeof(double)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "start_position", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_QUICK_SCAN, start_position),\
	{sizeof(double)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "end_position", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_QUICK_SCAN, end_position),\
	{sizeof(double)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "requested_num_measurements", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_QUICK_SCAN, requested_num_measurements), \
	{0}, NULL, MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY }, \
  \
  {-1, -1, "actual_num_measurements", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_QUICK_SCAN, actual_num_measurements), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "saved_synchronous_motion_mode", MXFT_BOOL, \
		NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_QUICK_SCAN, saved_synchronous_motion_mode),\
	{sizeof(int)}, NULL, MXFF_VARARGS}

#ifdef __cplusplus
}
#endif

#endif /* _MX_SCAN_QUICK_H_ */

