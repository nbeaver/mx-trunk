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
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DALSA_GEV_CAMERA_H__
#define __D_DALSA_GEV_CAMERA_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *dalsa_gev_record;
	char serial_number[MAX_GEVSTRING_LENGTH+1];
	long num_frame_buffers;
	mx_bool_type show_features;

	long camera_index;
	GEV_CAMERA_INFO *camera_object;
	GEV_CAMERA_HANDLE camera_handle;
	DALSA_GENICAM_GIGE_REGS camera_registers;

	MX_THREAD *next_image_thread;

	int32_t dalsa_total_num_frames;
	int32_t dalsa_last_frame_number;

} MX_DALSA_GEV_CAMERA;

#define MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES	88800

#define MXD_DALSA_GEV_CAMERA_STANDARD_FIELDS \
  {-1, -1, "dalsa_gev_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, dalsa_gev_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "serial_number", MXFT_STRING, NULL, 1, {MAX_GEVSTRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, serial_number), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_frame_buffers", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, num_frame_buffers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_DALSA_GEV_CAMERA_SHOW_FEATURES, -1, "show_features", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV_CAMERA, show_features), \
	{0}, NULL, 0}
	
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

#endif /* __D_DALSA_GEV_CAMERA_H__ */

