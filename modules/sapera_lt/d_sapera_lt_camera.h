/*
 * Name:    d_sapera_lt_camera.h
 *
 * Purpose: MX video input driver for a DALSA Sapera LT camera.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SAPERA_LT_CAMERA_H__
#define __D_SAPERA_LT_CAMERA_H__

#ifdef __cplusplus

#define MXU_SAPERA_LT_CAMERA_FEATURE_NAME_LENGTH	80
#define MXU_SAPERA_LT_CAMERA_FEATURE_VALUE_LENGTH	200

typedef struct {
	MX_RECORD *record;

	MX_RECORD *sapera_lt_record;
	long camera_number;
	char config_filename[MXU_FILENAME_LENGTH+1];
	long num_frame_buffers;

	unsigned long user_total_num_frames_at_start;
	unsigned long raw_total_num_frames_at_start;
	unsigned long num_frames_left_to_acquire;

	SapAcqDevice       *acq_device;
	SapBufferWithTrash *buffer;
	SapAcqDeviceToBuf  *transfer;

	SapFeature         *feature;

	struct timespec boot_time;

	struct timespec *frame_time;
	mx_bool_type    *frame_buffer_is_unsaved;

	long old_total_num_frames;
	unsigned long old_status;

	mx_bool_type buffer_overrun_occurred;

	long num_frames_to_skip;

	long frame_number;

	long raw_last_frame_number;
	unsigned long raw_total_num_frames;
	long *raw_frame_number_array;

	mx_bool_type show_features;
	double gain;
	double temperature;

	char feature_name[MXU_SAPERA_LT_CAMERA_FEATURE_NAME_LENGTH+1];
	char feature_value[MXU_SAPERA_LT_CAMERA_FEATURE_VALUE_LENGTH+1];

} MX_SAPERA_LT_CAMERA;

#define MXLV_SAPERA_LT_CAMERA_SHOW_FEATURES	88800
#define MXLV_SAPERA_LT_CAMERA_GAIN		88801
#define MXLV_SAPERA_LT_CAMERA_TEMPERATURE	88802
#define MXLV_SAPERA_LT_CAMERA_FEATURE_NAME	88803
#define MXLV_SAPERA_LT_CAMERA_FEATURE_VALUE	88804

#define MXD_SAPERA_LT_CAMERA_STANDARD_FIELDS \
  {-1, -1, "sapera_lt_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, sapera_lt_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "camera_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, camera_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "config_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, config_filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_frame_buffers", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, num_frame_buffers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "buffer_overrun_occurred", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SAPERA_LT_CAMERA, buffer_overrun_occurred), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "num_frames_to_skip", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, num_frames_to_skip),\
	{0}, NULL, 0}, \
  \
  {MXLV_SAPERA_LT_CAMERA_SHOW_FEATURES, -1, "show_features", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, show_features), \
	{0}, NULL, 0}, \
  \
  {MXLV_SAPERA_LT_CAMERA_GAIN, -1, "gain", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, gain), \
	{0}, NULL, 0}, \
  \
  {MXLV_SAPERA_LT_CAMERA_TEMPERATURE, -1, "temperature", \
						MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, temperature), \
	{0}, NULL, 0}, \
  \
  {MXLV_SAPERA_LT_CAMERA_FEATURE_NAME, -1, "feature_name", MXFT_STRING, \
		NULL, 1, {MXU_SAPERA_LT_CAMERA_FEATURE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, feature_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_SAPERA_LT_CAMERA_FEATURE_VALUE, -1, "feature_value", MXFT_STRING, \
		NULL, 1, {MXU_SAPERA_LT_CAMERA_FEATURE_VALUE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_CAMERA, feature_value), \
	{sizeof(char)}, NULL, 0}
	

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxd_sapera_lt_camera_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_camera_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_camera_open( MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_camera_close( MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_camera_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_camera_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_sapera_lt_camera_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_camera_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_camera_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_camera_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_camera_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_camera_get_extended_status(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_camera_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_camera_get_parameter(
						MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_sapera_lt_camera_set_parameter(
						MX_VIDEO_INPUT *vinput);

extern MX_RECORD_FUNCTION_LIST mxd_sapera_lt_camera_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST
			mxd_sapera_lt_camera_video_function_list;

extern long mxd_sapera_lt_camera_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sapera_lt_camera_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_SAPERA_LT_CAMERA_H__ */

