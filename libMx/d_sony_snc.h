/*
 * Name:    d_sony_snc.h
 *
 * Purpose: MX header file for Sony Network Camera video inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SONY_SNC_H__
#define __D_SONY_SNC_H__

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	unsigned long port_number;
	unsigned long sony_snc_flags;
	unsigned long num_frame_buffers;

	MX_IMAGE_FRAME *frame_buffer_array;
	MX_SOCKET *camera_socket;

	unsigned long old_total_num_frames;
	unsigned long num_frames_in_sequence;
	double seconds_per_frame;
	MX_CLOCK_TICK start_tick;
	mx_bool_type sequence_in_progress;
} MX_SONY_SNC;


#define MXD_SONY_SNC_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SONY_SNC, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "port_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SONY_SNC, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "sony_snc_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SONY_SNC, sony_snc_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_frame_buffers", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SONY_SNC, num_frame_buffers), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxd_sony_snc_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sony_snc_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_sony_snc_open( MX_RECORD *record );

MX_API mx_status_type mxd_sony_snc_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sony_snc_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sony_snc_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sony_snc_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sony_snc_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sony_snc_get_extended_status(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sony_snc_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_sony_snc_get_parameter(MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_sony_snc_set_parameter(MX_VIDEO_INPUT *vinput);

extern MX_RECORD_FUNCTION_LIST mxd_sony_snc_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_sony_snc_video_function_list;

extern long mxd_sony_snc_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sony_snc_rfield_def_ptr;

MX_API_PRIVATE mx_status_type mxd_sony_snc_send_command(
					MX_SONY_SNC *sony_snc,
					char *command,
					char *data );

MX_API_PRIVATE mx_status_type mxd_sony_snc_update_socket(
					MX_SONY_SNC *sony_snc );

#endif /* __D_SONY_SNC_H__ */

