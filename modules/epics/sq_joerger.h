/*
 * Name:    sq_joerger.h
 *
 * Purpose: Header file for quick scans that use Joerger scalers via EPICS.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2009-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SQ_JOERGER_H__
#define __SQ_JOERGER_H__

#define MXS_JQ_MAX_MOTORS		8
#define MXS_JQ_MAX_JOERGER_SCALERS	16

#define MX_JOERGER_QUICK_SCAN_ENABLE_RECORD	"mx_joerger_qscan"

#define MXS_JQ_MAX_JOERGER_CLOCK_TICKS	4294967295UL    /* 2**32 - 1 */

typedef struct {
	char epics_record_name[80];

	long num_completed_measurements;

	int scaler_number[ MXS_JQ_MAX_JOERGER_SCALERS ];

	char motor_name_array[ MXS_JQ_MAX_MOTORS][ MXU_EPICS_PVNAME_LENGTH+1 ];
	char data_name_array[ MXS_JQ_MAX_JOERGER_SCALERS ] \
				[ MXU_EPICS_PVNAME_LENGTH+1 ];

	double *motor_position_array[ MXS_JQ_MAX_MOTORS ];
	uint32_t *data_array[ MXS_JQ_MAX_JOERGER_SCALERS ];

	double acceleration_time;
	double real_start_position[ MXS_JQ_MAX_MOTORS ];
	double real_end_position[ MXS_JQ_MAX_MOTORS ];
} MX_JOERGER_QUICK_SCAN;

MX_API mx_status_type mxs_joerger_quick_scan_initialize_driver(
						MX_DRIVER *driver );
MX_API mx_status_type mxs_joerger_quick_scan_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxs_joerger_quick_scan_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxs_joerger_quick_scan_delete_record( MX_RECORD *record );

MX_API mx_status_type mxs_joerger_quick_scan_prepare_for_scan_start(
						MX_SCAN *scan );
MX_API mx_status_type mxs_joerger_quick_scan_execute_scan_body(
						MX_SCAN *scan );
MX_API mx_status_type mxs_joerger_quick_scan_cleanup_after_scan_end(
						MX_SCAN *scan );

extern MX_RECORD_FUNCTION_LIST mxs_joerger_quick_scan_record_function_list;
extern MX_SCAN_FUNCTION_LIST mxs_joerger_quick_scan_scan_function_list;

extern long mxs_joerger_quick_scan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxs_joerger_quick_scan_def_ptr;

#endif /* __SQ_JOERGER_H__ */

