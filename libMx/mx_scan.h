/*
 * Name:    mx_scan.h
 *
 * Purpose: Header file for generic scan superclass support.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_SCAN_H_
#define _MX_SCAN_H_

#include "mx_util.h"
#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_motor.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_measurement.h"
#include "mx_datafile.h"
#include "mx_plot.h"
#include "mx_mpermit.h"
#include "mx_mfault.h"
#include "mx_log.h"

/* Values for scan->scan_flags */

#define MXF_SCAN_EARLY_MOVE			0x1
#define MXF_SCAN_SUPPRESS_PROGRESS_DISPLAY	0x2

/* Values for scan->shutter_policy */

#define MXF_SCAN_SHUTTER_IGNORE			0
#define MXF_SCAN_SHUTTER_OPEN_FOR_SCAN		1
#define MXF_SCAN_SHUTTER_OPEN_FOR_DATAPOINT	2

#define MX_SCAN_SHUTTER_POLICY_RECORD_NAME	"mx_scan_shutter"

/* Values for scan early move policy */

#define MXF_SCAN_PROHIBIT_EARLY_MOVE		0
#define MXF_SCAN_REQUIRE_EARLY_MOVE		1
#define MXF_SCAN_ALLOW_EARLY_MOVE		2

#if 0   /* FIXME - When we get long variable names, change the name here. */
#define MX_SCAN_EARLY_MOVE_RECORD_NAME		"mx_scan_early_move"
#else
#define MX_SCAN_EARLY_MOVE_RECORD_NAME		"scan_early_move"
#endif

/*---*/

#define MX_SCAN_PERMIT_HANDLER_LIST		"mx_scan_permit"
#define MX_SCAN_FAULT_HANDLER_LIST		"mx_scan_fault"

#define MX_SCAN_SYNCHRONOUS_MOTION_MODE		"mx_sync_mode"

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
                            * to this MX_SCAN structure.
                            */

	long num_scans;
	long current_scan_number;

	long num_independent_variables;
	long num_motors;
	double *motor_position;
	MX_RECORD **motor_record_array;
	long *motor_is_independent_variable;
	long num_input_devices;
	MX_RECORD **input_device_array;
	unsigned long scan_flags;
	double settling_time;
	mx_status_type execute_scan_body_status;

	long num_missing_records;
	MX_RECORD **missing_record_array;

	long shutter_policy;
	MX_RECORD *shutter_record;

	long num_measurement_permit_handlers;
	MX_MEASUREMENT_PERMIT **measurement_permit_handler_array;

	long num_measurement_fault_handlers;
	MX_MEASUREMENT_FAULT **measurement_fault_handler_array;

	MX_MEASUREMENT measurement;
	long measurement_number;
	char measurement_type[MXU_MEASUREMENT_TYPE_NAME_LENGTH + 1];
	char measurement_arguments[MXU_MEASUREMENT_ARGUMENTS_LENGTH + 1];

	MX_DATAFILE datafile;
	char datafile_description[MXU_DATAFILE_DESCRIPTION_LENGTH+1];
	char datafile_name[MXU_FILENAME_LENGTH + 1];

	MX_PLOT plot;
	char plot_description[MXU_PLOT_DESCRIPTION_LENGTH + 1];
	char plot_arguments[MXU_PLOT_ARGUMENTS_LENGTH + 1];

	void *application_ptr;
} MX_SCAN;

typedef struct {
	mx_status_type ( *prepare_for_scan_start ) ( MX_SCAN *scan );
	mx_status_type ( *execute_scan_body ) ( MX_SCAN *scan );
	mx_status_type ( *cleanup_after_scan_end ) ( MX_SCAN *scan );
} MX_SCAN_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_scan_finish_record_initialization(
						MX_RECORD *record);

MX_API mx_status_type mx_scan_print_scan_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mx_perform_scan( MX_RECORD *record );

MX_API mx_status_type mx_scan_set_custom_datafile_handler(
					MX_RECORD *record,
					void *custom_args,
					MX_DATAFILE_FUNCTION_LIST *flist );

MX_API mx_status_type mx_scan_set_custom_plot_handler(
					MX_RECORD *record,
					void *custom_args,
					MX_PLOT_FUNCTION_LIST *flist );

/*----*/

MX_API mx_status_type mx_standard_prepare_for_scan_start( MX_SCAN *scan );

MX_API mx_status_type mx_standard_cleanup_after_scan_end( MX_SCAN *scan );

MX_API mx_status_type mx_scan_save_speeds( MX_SCAN *scan );

MX_API mx_status_type mx_scan_restore_speeds( MX_SCAN *scan );

MX_API mx_status_type mx_scan_wait_for_all_permits( MX_SCAN *scan );

MX_API mx_status_type mx_scan_check_for_all_faults( MX_SCAN *scan,
						mx_bool_type *fault_occurred );

MX_API mx_status_type mx_scan_reset_all_faults( MX_SCAN *scan );

MX_API mx_status_type mx_scan_acquire_and_readout_data( MX_SCAN *scan );

MX_API mx_status_type mx_scan_acquire_data( MX_SCAN *scan );

MX_API mx_status_type mx_scan_update_old_destinations( MX_SCAN *scan );

MX_API mx_status_type mx_scan_handle_pause_request( MX_SCAN *scan );

MX_API mx_status_type mx_default_scan_pause_request_handler( MX_SCAN *scan );

MX_API void mx_set_scan_pause_request_handler(
		mx_status_type (*pause_request_handler)( MX_SCAN * ) );

MX_API mx_status_type mx_scan_display_scan_progress( MX_SCAN *scan );

MX_API mx_status_type mx_clear_scan_input_devices( MX_SCAN *scan );

MX_API void mx_log_scan_start( MX_LIST_HEAD *list_head, MX_SCAN *scan );

MX_API void mx_log_scan_end( MX_LIST_HEAD *list_head, MX_SCAN *scan,
				mx_status_type status );

MX_API void mx_handle_abnormal_scan_termination( MX_LIST_HEAD *list_head,
				MX_SCAN *scan,
				mx_status_type status );

MX_API double mx_scan_get_measurement_time( MX_SCAN *scan );

MX_API mx_status_type mx_scan_fixup_varargs_record_field_defaults(
			MX_RECORD_FIELD_DEFAULTS *record_field_defaults,
			long num_record_field,
			long *num_independent_variables_varargs_cookie,
			long *num_motors_varargs_cookie,
			long *num_input_devices_varargs_cookie );

MX_API mx_status_type mx_compute_normalized_device_value(
				MX_RECORD *input_device,
				double measurement_time,
				double *normalized_input_device_value );

MX_API mx_status_type mx_convert_normalized_device_value_to_string(
				MX_RECORD *input_device,
				double measurement_time,
				char *buffer, size_t buffer_length );

MX_API mx_status_type mx_scan_find_parent_scan( MX_SCAN *child_scan,
						MX_SCAN **parent_scan );

MX_API mx_status_type mx_scan_get_pointer_to_measurement_number( MX_SCAN *scan,
								long **ptr );

MX_API mx_status_type mx_scan_increment_measurement_number( MX_SCAN *scan );

MX_API mx_status_type mx_scan_get_pointer_to_datafile_filename( MX_SCAN *scan,
								char **ptr );

MX_API mx_status_type mx_scan_get_subdirectory_and_filename( MX_SCAN *scan,
					MX_RECORD *input_device,
					long input_device_class,
					char *subdirectory_name,
					size_t max_dirname_length,
					char *filename,
					size_t max_filename_length );

MX_API mx_status_type mx_scan_save_mca_measurements( MX_SCAN *scan,
							long num_mcas );

MX_API mx_status_type mx_scan_save_area_detector_image( MX_SCAN *scan,
							MX_RECORD *ad_record );

MX_API mx_status_type mx_scan_handle_alternate_x_motors( MX_SCAN *scan );

MX_API mx_status_type mx_scan_get_early_move_flag( MX_SCAN *scan,
						mx_bool_type *early_move_flag );

#define MX_SCAN_STANDARD_FIELDS  \
  {-1, -1, "num_scans", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, num_scans), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_independent_variables", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
	offsetof(MX_SCAN, num_independent_variables), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_motors", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, num_motors), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "motor_position", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, motor_position), \
	{sizeof(double)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "motor_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, motor_record_array), \
	{sizeof(MX_RECORD *)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "motor_is_independent_variable", MXFT_LONG, \
	NULL, 1, {MXU_VARARGS_LENGTH}, MXF_REC_SUPERCLASS_STRUCT, \
			offsetof(MX_SCAN, motor_is_independent_variable), \
	{sizeof(long)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "num_input_devices", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, num_input_devices), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_device_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, input_device_array), \
	{sizeof(MX_RECORD *)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS)}, \
  \
  {-1, -1, "scan_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, scan_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "settling_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, settling_time), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "measurement_type", MXFT_STRING, NULL, \
				1, {MXU_MEASUREMENT_TYPE_NAME_LENGTH},\
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, measurement_type), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "measurement_arguments", MXFT_STRING, NULL, \
				1, {MXU_MEASUREMENT_ARGUMENTS_LENGTH},\
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, measurement_arguments), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "datafile_description", MXFT_STRING, \
			NULL, 1, {MXU_DATAFILE_DESCRIPTION_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, datafile_description), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "datafile_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, datafile_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "plot_description", MXFT_STRING, \
	  		NULL, 1, {MXU_PLOT_DESCRIPTION_LENGTH},\
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, plot_description), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "plot_arguments", MXFT_STRING, NULL, 1, {MXU_PLOT_ARGUMENTS_LENGTH},\
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_SCAN, plot_arguments), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}

#ifdef __cplusplus
}
#endif

#endif /* _MX_SCAN_H_ */

