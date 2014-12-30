/*
 * Name:    d_avt_pvapi.h
 *
 * Purpose: MX video input driver header for the PvAPI C API
 *          used by cameras from Allied Vision Technologies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AVT_PVAPI_H__
#define __D_AVT_PVAPI_H__

/* Flag values for the 'avt_pvapi_camera_flags' field. */

/* Flag values for the 'id_type' field. */

#define MXT_USE_AVT_PVAPI_UNIQUE_ID	1
#define MXT_USE_AVT_PVAPI_IP_ADDRESS	2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *avt_pvapi_record;
	char camera_name[MXU_HOSTNAME_LENGTH+1];
	unsigned long avt_pvapi_camera_flags;

#if IS_MX_DRIVER
	tPvHandle *camera_handle;
#endif

} MX_AVT_PVAPI_CAMERA;


#define MXD_AVT_PVAPI_STANDARD_FIELDS \
  {-1, -1, "avt_pvapi_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVT_PVAPI_CAMERA, avt_pvapi_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_name", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVT_PVAPI_CAMERA, camera_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "avt_pvapi_camera_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVT_PVAPI_CAMERA, avt_pvapi_camera_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

MX_API mx_status_type mxd_avt_pvapi_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_avt_pvapi_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_avt_pvapi_open( MX_RECORD *record );
MX_API mx_status_type mxd_avt_pvapi_close( MX_RECORD *record );

MX_API mx_status_type mxd_avt_pvapi_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_avt_pvapi_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_avt_pvapi_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_avt_pvapi_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_avt_pvapi_get_status( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_avt_pvapi_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_avt_pvapi_get_parameter( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_avt_pvapi_set_parameter( MX_VIDEO_INPUT *vinput );

extern MX_RECORD_FUNCTION_LIST mxd_avt_pvapi_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_avt_pvapi_video_function_list;

extern long mxd_avt_pvapi_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_avt_pvapi_rfield_def_ptr;

#endif /* __D_AVT_PVAPI_H__ */

