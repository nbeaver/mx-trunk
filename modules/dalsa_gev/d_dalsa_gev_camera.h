/*
 * Name:    d_dalsa_gev_camera.h
 *
 * Purpose: MX video input driver for a DALSA GigE-Vision camera
 *          controlled via the Gev API.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DALSA_GEV_CAMERA_H__
#define __D_DALSA_GEV_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_DALSA_GEV_CAMERA_FEATURE_NAME_LENGTH	80
#define MXU_DALSA_GEV_CAMERA_FEATURE_VALUE_LENGTH	200
#define MXU_DALSA_GEV_CAMERA_FEATURE_RANGE_LENGTH	2000

#define MXU_DALSA_GEV_CAMERA_DEVICE_NAME_LENGTH		80

/* 'dalsa_gev_camera_flags' bitflag macros */

#define MXF_DALSA_GEV_CAMERA_WRITE_XML_FILE		0x1

#define MXF_DALSA_GEV_CAMERA_CHECK_NETWORK_CONNECTION	0x10
#define MXF_DALSA_GEV_CAMERA_CHECK_PACKET_SIZES		0x20

#define MXF_DALSA_GEV_CAMERA_SHOW_FEATURES		0x1000
#define MXF_DALSA_GEV_CAMERA_SHOW_FEATURE_VALUES	0x2000

/* values for 'model_type' */

#define MXT_DALSA_GEV_CAMERA_SHADOBOX		1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *dalsa_gev_record;
	char serial_number[MAX_GEVSTRING_LENGTH+1];
	long user_set_selector;
	unsigned long num_frame_buffers;
	unsigned long dalsa_gev_camera_flags;
	char xml_filename[MXU_FILENAME_LENGTH+1];

	unsigned long model_type;
	char device_vendor_name[MXU_DALSA_GEV_CAMERA_DEVICE_NAME_LENGTH+1];
	char device_model_name[MXU_DALSA_GEV_CAMERA_DEVICE_NAME_LENGTH+1];

	unsigned char **frame_buffer_array;
	uint64_t frame_buffer_array_size_in_bytes;

	long camera_index;
	GEV_CAMERA_INFO *camera_object;
	GEV_CAMERA_HANDLE camera_handle;
	void *feature_node_map;

	MX_THREAD *image_wait_thread;

	MX_MUTEX *frame_counter_mutex;

	/* Note: The following protected variables must be read and written
	 * only when protected by the frame_counter_mutex.
	 */

	unsigned long user_total_num_frames_at_start;
	long user_last_frame_number;
	unsigned long user_total_num_frames;

	unsigned long raw_total_num_frames_at_start;
	long raw_last_frame_number;
	unsigned long raw_total_num_frames;

	unsigned long num_frames_left_to_acquire;

	unsigned long *raw_frame_number_array;
	struct timespec *frame_timespec;
	mx_bool_type *frame_buffer_is_unsaved;

	/* End of protected_variables. */

	long num_frames_to_skip;

	mx_bool_type show_features;
	mx_bool_type show_feature_values;
	double gain;
	double temperature;

	char feature_name[MXU_DALSA_GEV_CAMERA_FEATURE_NAME_LENGTH+1];
	char feature_value[MXU_DALSA_GEV_CAMERA_FEATURE_VALUE_LENGTH+1];

	char feature_range[MXU_DALSA_GEV_CAMERA_FEATURE_RANGE_LENGTH+1];

} MX_DALSA_GEV_CAMERA;

#define MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES		89800
#define MXLV_DALSA_GEV_CAMERA_SHOW_FEATURE_VALUES	89801
#define MXLV_DALSA_GEV_CAMERA_GAIN			89802
#define MXLV_DALSA_GEV_CAMERA_TEMPERATURE		89803
#define MXLV_DALSA_GEV_CAMERA_FEATURE_NAME		89804
#define MXLV_DALSA_GEV_CAMERA_FEATURE_VALUE		89805
#define MXLV_DALSA_GEV_CAMERA_FEATURE_RANGE		89806

#define MXD_DALSA_GEV_CAMERA_STANDARD_FIELDS \
  {-1, -1, "dalsa_gev_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, dalsa_gev_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "serial_number", MXFT_STRING, NULL, 1, {MAX_GEVSTRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, serial_number), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "user_set_selector", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, user_set_selector), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "num_frame_buffers", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, num_frame_buffers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "dalsa_gev_camera_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_DALSA_GEV_CAMERA, dalsa_gev_camera_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "xml_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, xml_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "model_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, model_type), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "device_vendor_name", MXFT_STRING, NULL, \
	  			1, {MXU_DALSA_GEV_CAMERA_DEVICE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, device_vendor_name),\
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "device_model_name", MXFT_STRING, NULL, \
	  			1, {MXU_DALSA_GEV_CAMERA_DEVICE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, device_model_name),\
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "num_frames_to_skip", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, num_frames_to_skip),\
	{0}, NULL, 0}, \
  \
  {MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES, -1, "show_features", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, show_features), \
	{0}, NULL, 0}, \
  \
  {MXLV_DALSA_GEV_CAMERA_SHOW_FEATURE_VALUES, -1, "show_feature_values", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_DALSA_GEV_CAMERA, show_feature_values), \
	{0}, NULL, 0}, \
  \
  {MXLV_DALSA_GEV_CAMERA_GAIN, -1, "gain", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, gain), \
	{0}, NULL, 0}, \
  \
  {MXLV_DALSA_GEV_CAMERA_TEMPERATURE, -1, "temperature", \
						MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, temperature), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_DALSA_GEV_CAMERA_FEATURE_NAME, -1, "feature_name", MXFT_STRING, \
		NULL, 1, {MXU_DALSA_GEV_CAMERA_FEATURE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, feature_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_DALSA_GEV_CAMERA_FEATURE_VALUE, -1, "feature_value", MXFT_STRING, \
		NULL, 1, {MXU_DALSA_GEV_CAMERA_FEATURE_VALUE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, feature_value), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_DALSA_GEV_CAMERA_FEATURE_RANGE, -1, "feature_range", MXFT_STRING, \
		NULL, 1, {MXU_DALSA_GEV_CAMERA_FEATURE_RANGE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, feature_range), \
	{sizeof(char)}, NULL, 0}

	
MX_API mx_status_type mxd_dalsa_gev_camera_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_dalsa_gev_camera_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_dalsa_gev_camera_open( MX_RECORD *record );
MX_API mx_status_type mxd_dalsa_gev_camera_close( MX_RECORD *record );
MX_API mx_status_type mxd_dalsa_gev_camera_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_dalsa_gev_camera_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_dalsa_gev_camera_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_dalsa_gev_camera_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_dalsa_gev_camera_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_dalsa_gev_camera_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_dalsa_gev_camera_get_extended_status(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_dalsa_gev_camera_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_dalsa_gev_camera_get_parameter(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_dalsa_gev_camera_set_parameter(
						MX_VIDEO_INPUT *vinput );

extern MX_RECORD_FUNCTION_LIST mxd_dalsa_gev_camera_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST
			mxd_dalsa_gev_camera_video_function_list;

extern long mxd_dalsa_gev_camera_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dalsa_gev_camera_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_DALSA_GEV_CAMERA_H__ */

