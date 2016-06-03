/*
 * Name:    sq_mcs.h
 *
 * Purpose: Header file for quick scans that use an MX MCS to collect the data.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2002-2003, 2010, 2015-2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SQ_MCS_H__
#define __SQ_MCS_H__

#define MXS_SQ_MCS_NUM_PREMOVE_MEASUREMENTS	2

#define MXS_SQ_MCS_ARRAY_BLOCK_SIZE		10

typedef struct mx_mcs_quick_scan_type {
	double **motor_position_array;

	MX_RECORD **real_motor_record_array;
	MX_RECORD **mce_record_array;

	unsigned long num_mcs;
	MX_RECORD **mcs_record_array;

	unsigned long mcs_array_size;

	long mcs_readout_preference;

	double premove_measurement_time;
	double acceleration_time;
	double scan_body_time;
	double deceleration_time;
	double postmove_measurement_time;

	double *real_start_position;
	double *real_end_position;

	double *backlash_position;

	mx_bool_type *use_window;
	double **window;

	long *mcs_measurement_offset;

	mx_status_type (*move_to_start_fn)( MX_SCAN *,
					MX_QUICK_SCAN *,
					struct mx_mcs_quick_scan_type *,
					double,
					mx_bool_type );

	mx_status_type (*compute_motor_positions_fn)( MX_SCAN *,
						MX_QUICK_SCAN *,
						struct mx_mcs_quick_scan_type *,
						MX_MCS * );

	void *extension_ptr;

} MX_MCS_QUICK_SCAN;

MX_API mx_status_type mxs_mcs_quick_scan_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxs_mcs_quick_scan_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxs_mcs_quick_scan_delete_record( MX_RECORD *record );

MX_API mx_status_type mxs_mcs_quick_scan_prepare_for_scan_start(
							MX_SCAN *scan );
MX_API mx_status_type mxs_mcs_quick_scan_execute_scan_body( MX_SCAN *scan );
MX_API mx_status_type mxs_mcs_quick_scan_cleanup_after_scan_end(
							MX_SCAN *scan );
MX_API mx_status_type mxs_mcs_quick_scan_get_parameter( MX_SCAN *scan );

extern MX_RECORD_FUNCTION_LIST mxs_mcs_quick_scan_record_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_mcs_quick_scan_scan_function_list;

extern long mxs_mcs_quick_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_mcs_quick_scan_def_ptr;

MX_API mx_status_type mxs_mcs_quick_scan_get_pointers( MX_SCAN *scan,
				MX_QUICK_SCAN **quick_scan,
				MX_MCS_QUICK_SCAN **mcs_quick_scan,
				const char *calling_fname );

MX_API mx_status_type
mxs_mcs_quick_scan_move_absolute_and_wait( MX_SCAN *scan,
					double *position_array );

MX_API MX_RECORD *
mxs_mcs_quick_scan_find_encoder_readout( MX_RECORD *motor_record );

MX_API mx_status_type
mxs_mcs_quick_scan_default_move_to_start( MX_SCAN *scan,
				MX_QUICK_SCAN *quick_scan,
				MX_MCS_QUICK_SCAN *mcs_quick_scan,
				double measurement_time,
				mx_bool_type correct_for_quick_scan_backlash );

MX_API mx_status_type
mxs_mcs_quick_scan_default_compute_motor_positions( MX_SCAN *scan,
					MX_QUICK_SCAN *quick_scan,
					MX_MCS_QUICK_SCAN *mcs_quick_scan,
					MX_MCS *mcs );

#endif /* __SQ_MCS_H__ */

