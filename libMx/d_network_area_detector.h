/*
 * Name:    d_network_area_detector.h
 *
 * Purpose: Header file for MX network area detector devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_AREA_DETECTOR_H__
#define __D_NETWORK_AREA_DETECTOR_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD abort_nf;
	MX_NETWORK_FIELD arm_nf;
	MX_NETWORK_FIELD binsize_nf;
	MX_NETWORK_FIELD bits_per_pixel_nf;
	MX_NETWORK_FIELD byte_order_nf;
	MX_NETWORK_FIELD bytes_per_frame_nf;
	MX_NETWORK_FIELD bytes_per_pixel_nf;
	MX_NETWORK_FIELD copy_frame_nf;
	MX_NETWORK_FIELD correct_frame_nf;
	MX_NETWORK_FIELD correction_flags_nf;
	MX_NETWORK_FIELD correction_frame_geom_corr_last_nf;
	MX_NETWORK_FIELD correction_frame_no_geom_corr_nf;
	MX_NETWORK_FIELD correction_measurement_time_nf;
	MX_NETWORK_FIELD correction_measurement_type_nf;
	MX_NETWORK_FIELD current_num_rois_nf;
	MX_NETWORK_FIELD datafile_directory_nf;
	MX_NETWORK_FIELD datafile_format_nf;
	MX_NETWORK_FIELD datafile_name_nf;
	MX_NETWORK_FIELD datafile_pattern_nf;
	MX_NETWORK_FIELD detector_readout_time_nf;
	MX_NETWORK_FIELD extended_status_nf;
	MX_NETWORK_FIELD framesize_nf;
	MX_NETWORK_FIELD frame_filename_nf;
	MX_NETWORK_FIELD geom_corr_after_flood_nf;
	MX_NETWORK_FIELD image_format_name_nf;
	MX_NETWORK_FIELD image_format_nf;
	MX_NETWORK_FIELD image_frame_data_nf;
	MX_NETWORK_FIELD image_frame_header_nf;
	MX_NETWORK_FIELD image_frame_header_length_nf;
	MX_NETWORK_FIELD last_datafile_name_nf;
	MX_NETWORK_FIELD last_frame_number_nf;
	MX_NETWORK_FIELD load_frame_nf;
	MX_NETWORK_FIELD register_name_nf;
	MX_NETWORK_FIELD register_value_nf;
	MX_NETWORK_FIELD maximum_frame_number_nf;
	MX_NETWORK_FIELD maximum_framesize_nf;
	MX_NETWORK_FIELD maximum_num_rois_nf;
	MX_NETWORK_FIELD num_correction_measurements_nf;
	MX_NETWORK_FIELD readout_frame_nf;
	MX_NETWORK_FIELD resynchronize_nf;
	MX_NETWORK_FIELD roi_nf;
	MX_NETWORK_FIELD roi_array_nf;
	MX_NETWORK_FIELD roi_bytes_per_frame_nf;
	MX_NETWORK_FIELD roi_number_nf;
	MX_NETWORK_FIELD save_frame_nf;
	MX_NETWORK_FIELD sequence_start_delay_nf;
	MX_NETWORK_FIELD shutter_enable_nf;
	MX_NETWORK_FIELD status_nf;
	MX_NETWORK_FIELD stop_nf;
	MX_NETWORK_FIELD subframe_size_nf;
	MX_NETWORK_FIELD total_acquisition_time_nf;
	MX_NETWORK_FIELD total_num_frames_nf;
	MX_NETWORK_FIELD total_sequence_time_nf;
	MX_NETWORK_FIELD transfer_frame_nf;
	MX_NETWORK_FIELD trigger_nf;
	MX_NETWORK_FIELD trigger_mode_nf;
	MX_NETWORK_FIELD use_scaled_dark_current_nf;

	MX_NETWORK_FIELD sequence_type_nf;
	MX_NETWORK_FIELD num_sequence_parameters_nf;
	MX_NETWORK_FIELD sequence_parameter_array_nf;

	MX_NETWORK_FIELD get_roi_frame_nf;
	MX_NETWORK_FIELD roi_frame_buffer_nf;
} MX_NETWORK_AREA_DETECTOR;


#define MXD_NETWORK_AREA_DETECTOR_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NETWORK_AREA_DETECTOR, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NETWORK_AREA_DETECTOR, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxd_network_area_detector_initialize_type(
							long record_type );
MX_API mx_status_type mxd_network_area_detector_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_area_detector_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_area_detector_open( MX_RECORD *record );
MX_API mx_status_type mxd_network_area_detector_close( MX_RECORD *record );
MX_API mx_status_type mxd_network_area_detector_resynchronize(
							MX_RECORD *record );

MX_API mx_status_type mxd_network_area_detector_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_get_last_frame_number(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_get_total_num_frames(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_get_status(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_get_extended_status(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_readout_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_correct_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_transfer_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_load_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_save_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_copy_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_get_roi_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_get_parameter(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_set_parameter(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_network_area_detector_measure_correction(
							MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_network_area_detector_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_network_area_detector_ad_function_list;

extern long mxd_network_area_detector_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_area_detector_rfield_def_ptr;

#endif /* __D_NETWORK_AREA_DETECTOR_H__ */

