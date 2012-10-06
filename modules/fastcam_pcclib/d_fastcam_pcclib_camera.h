/*
 * Name:    d_fastcam_pcclib_camera.h
 *
 * Purpose: MX video input driver for a Photron FASTCAM camera 
 *          controlled by the PccLib library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_FASTCAM_PCCLIB_CAMERA_H__
#define __D_FASTCAM_PCCLIB_CAMERA_H__

#ifdef __cplusplus

typedef struct {
	MX_RECORD *record;

	MX_RECORD *fastcam_pcclib_record;
	long camera_number;

	CCameraControl *camera_control;

} MX_FASTCAM_PCCLIB_CAMERA;


#define MXD_FASTCAM_PCCLIB_CAMERA_STANDARD_FIELDS \
  {-1, -1, "fastcam_pcclib_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_FASTCAM_PCCLIB_CAMERA, fastcam_pcclib_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "camera_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FASTCAM_PCCLIB_CAMERA, camera_number),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxd_fastcam_pcclib_camera_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_fastcam_pcclib_camera_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_fastcam_pcclib_camera_open( MX_RECORD *record );
MX_API mx_status_type mxd_fastcam_pcclib_camera_close( MX_RECORD *record );

MX_API mx_status_type mxd_fastcam_pcclib_camera_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_fastcam_pcclib_camera_trigger(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_fastcam_pcclib_camera_stop(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_fastcam_pcclib_camera_abort(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_fastcam_pcclib_camera_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_fastcam_pcclib_camera_get_extended_status(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_fastcam_pcclib_camera_get_frame(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_fastcam_pcclib_camera_get_parameter(
						MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_fastcam_pcclib_camera_set_parameter(
						MX_VIDEO_INPUT *vinput);

extern MX_RECORD_FUNCTION_LIST mxd_fastcam_pcclib_camera_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST
			mxd_fastcam_pcclib_camera_video_function_list;

extern long mxd_fastcam_pcclib_camera_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_fastcam_pcclib_camera_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_FASTCAM_PCCLIB_CAMERA_H__ */

