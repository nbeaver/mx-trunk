/*
 * Name:    d_aravis_camera.h
 *
 * Purpose: MX video input driver for a single camera controlled by Aravis.
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

#ifndef __D_ARAVIS_CAMERA_H__
#define __D_ARAVIS_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 'aravis_camera_flags' bitflag macros */

#define MXF_ARAVIS_CAMERA_WRITE_XML_FILE		0x1
#define MXF_ARAVIS_CAMERA_SHOW_INFO			0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *aravis_record;

} MX_ARAVIS_CAMERA;

#define MXD_ARAVIS_CAMERA_STANDARD_FIELDS \
  {-1, -1, "aravis_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ARAVIS_CAMERA, aravis_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }
	
MX_API mx_status_type mxd_aravis_camera_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_aravis_camera_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_aravis_camera_open( MX_RECORD *record );
MX_API mx_status_type mxd_aravis_camera_close( MX_RECORD *record );
MX_API mx_status_type mxd_aravis_camera_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_aravis_camera_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_aravis_camera_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_aravis_camera_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_aravis_camera_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_aravis_camera_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_aravis_camera_get_extended_status(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_aravis_camera_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_aravis_camera_get_parameter(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_aravis_camera_set_parameter(
						MX_VIDEO_INPUT *vinput );

extern MX_RECORD_FUNCTION_LIST mxd_aravis_camera_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST
			mxd_aravis_camera_video_function_list;

extern long mxd_aravis_camera_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aravis_camera_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_ARAVIS_CAMERA_H__ */
