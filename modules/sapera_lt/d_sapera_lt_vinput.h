/*
 * Name:    d_sapera_lt_vinput.h
 *
 * Purpose: MX video input driver for a DALSA Sapera LT video capture device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SAPERA_LT_VINPUT_H__
#define __D_SAPERA_LT_VINPUT_H__

#ifdef __cplusplus

typedef struct {
	MX_RECORD *record;

	MX_RECORD *sapera_lt_record;
	char server_name[CORSERVER_MAX_STRLEN+1];
	long device_number;
	char config_filename[MXU_FILENAME_LENGTH+1];

	mx_bool_type grab_in_progress;

	SapAcquisition *acquisition_object;
	SapBuffer      *buffer_object;
	SapView        *view_object;
	SapAcqToBuf    *transfer_object;
} MX_SAPERA_LT_VINPUT;


#define MXD_SAPERA_LT_VINPUT_STANDARD_FIELDS \
  {-1, -1, "sapera_lt_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_VINPUT, sapera_lt_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "server_name", MXFT_STRING, NULL, 1, {CORSERVER_MAX_STRLEN}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_VINPUT, server_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "device_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_VINPUT, device_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "config_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT_VINPUT, config_filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxd_sapera_lt_vinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_vinput_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_vinput_open( MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_vinput_close( MX_RECORD *record );

MX_API mx_status_type mxd_sapera_lt_vinput_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_vinput_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_vinput_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_vinput_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_vinput_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_vinput_get_extended_status(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_vinput_get_frame(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_vinput_get_parameter(
						MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_sapera_lt_vinput_set_parameter(
						MX_VIDEO_INPUT *vinput);

extern MX_RECORD_FUNCTION_LIST mxd_sapera_lt_vinput_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_sapera_lt_vinput_video_function_list;

extern long mxd_sapera_lt_vinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sapera_lt_vinput_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_SAPERA_LT_VINPUT_H__ */

