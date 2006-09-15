/*
 * Name:    d_network_vinput.h
 *
 * Purpose: Header file for MX network video input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_VINPUT_H__
#define __D_NETWORK_VINPUT_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD abort_nf;
	MX_NETWORK_FIELD arm_nf;
	MX_NETWORK_FIELD busy_nf;
	MX_NETWORK_FIELD bytes_per_frame_nf;
	MX_NETWORK_FIELD framesize_nf;
	MX_NETWORK_FIELD image_format_name_nf;
	MX_NETWORK_FIELD image_format_nf;
	MX_NETWORK_FIELD pixel_order_nf;
	MX_NETWORK_FIELD status_nf;
	MX_NETWORK_FIELD stop_nf;
	MX_NETWORK_FIELD trigger_nf;
	MX_NETWORK_FIELD trigger_mode_nf;

	MX_NETWORK_FIELD sequence_type_nf;
	MX_NETWORK_FIELD num_sequence_parameters_nf;
	MX_NETWORK_FIELD sequence_parameter_array_nf;

	MX_NETWORK_FIELD get_frame_nf;
	MX_NETWORK_FIELD frame_buffer_nf;
} MX_NETWORK_VINPUT;


#define MXD_NETWORK_VINPUT_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_VINPUT, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_VINPUT, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxd_network_vinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_vinput_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_vinput_open( MX_RECORD *record );
MX_API mx_status_type mxd_network_vinput_close( MX_RECORD *record );

MX_API mx_status_type mxd_network_vinput_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_network_vinput_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_network_vinput_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_network_vinput_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_network_vinput_busy( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_network_vinput_get_status( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_network_vinput_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_network_vinput_get_parameter(MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_network_vinput_set_parameter(MX_VIDEO_INPUT *vinput);

extern MX_RECORD_FUNCTION_LIST mxd_network_vinput_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_network_vinput_video_function_list;

extern long mxd_network_vinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_vinput_rfield_def_ptr;

#endif /* __D_NETWORK_VINPUT_H__ */

