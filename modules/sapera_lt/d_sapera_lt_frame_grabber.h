/*
 * Name:    d_sapera_lt_frame_grabber.h
 *
 * Purpose: MX video input driver for a DALSA Sapera LT frame grabber.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SAPERA_LT_FRAME_GRABBER_H__
#define __D_SAPERA_LT_FRAME_GRABBER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Values for the 'sapera_frame_grabber_flags' field. */

#define MXF_SAPERA_FRAME_GRABBER_RETRY_CREATE	0x1
#define MXF_SAPERA_FRAME_GRABBER_DEBUG_SNAP	0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *sapera_lt_record;
	long frame_grabber_number;
	char config_filename[MXU_FILENAME_LENGTH+1];
	long num_frame_buffers;
	unsigned long sapera_frame_grabber_flags;

	unsigned long total_num_frames_at_start;
	unsigned long num_frames_left_to_acquire;

	struct timespec boot_time;

	struct timespec *frame_time;
	mx_bool_type    *frame_buffer_is_unsaved;

	mx_bool_type buffer_overrun_occurred;

	unsigned long signal_status;

#ifdef __cplusplus

	SapAcquisition *acquisition;
	SapBuffer      *buffer;
	SapAcqToBuf    *transfer;

#endif /* __cplusplus */

} MX_SAPERA_LT_FRAME_GRABBER;

#ifdef __cplusplus
}
#endif

#define MXLV_SAPERA_LT_FRAME_GRABBER_SIGNAL_STATUS	56701

#define MXD_SAPERA_LT_FRAME_GRABBER_STANDARD_FIELDS \
  {-1, -1, "sapera_lt_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SAPERA_LT_FRAME_GRABBER, sapera_lt_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "frame_grabber_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SAPERA_LT_FRAME_GRABBER, frame_grabber_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "config_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SAPERA_LT_FRAME_GRABBER, config_filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_frame_buffers", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SAPERA_LT_FRAME_GRABBER, num_frame_buffers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "sapera_frame_grabber_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
  	    offsetof(MX_SAPERA_LT_FRAME_GRABBER, sapera_frame_grabber_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "buffer_overrun_occurred", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SAPERA_LT_FRAME_GRABBER, buffer_overrun_occurred), \
	{0}, NULL, 0}, \
  \
  {MXLV_SAPERA_LT_FRAME_GRABBER_SIGNAL_STATUS, -1, "signal_status", \
		  				MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SAPERA_LT_FRAME_GRABBER, signal_status), \
	{0}, NULL, MXFF_READ_ONLY}
	
#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxd_sapera_lt_frame_grabber_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_open( MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_close( MX_RECORD *record );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_sapera_lt_frame_grabber_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_trigger(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_stop(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_abort(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_get_extended_status(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_get_frame(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sapera_lt_frame_grabber_get_parameter(
						MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_sapera_lt_frame_grabber_set_parameter(
						MX_VIDEO_INPUT *vinput);

extern MX_RECORD_FUNCTION_LIST mxd_sapera_lt_frame_grabber_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST
			mxd_sapera_lt_frame_grabber_video_function_list;

extern long mxd_sapera_lt_frame_grabber_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sapera_lt_frame_grabber_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_SAPERA_LT_FRAME_GRABBER_H__ */

