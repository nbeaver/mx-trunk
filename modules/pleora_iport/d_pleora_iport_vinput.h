/*
 * Name:    d_pleora_iport_vinput.h
 *
 * Purpose: MX video input driver for a Pleora iPORT video capture device.
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

#ifndef __D_PLEORA_IPORT_VINPUT_H__
#define __D_PLEORA_IPORT_VINPUT_H__

#ifdef __cplusplus

#ifndef MXU_MAC_ADDRESS_STRING_LENGTH
#   define MXU_MAC_ADDRESS_STRING_LENGTH	18
#endif

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pleora_iport_record;
	char mac_address_string[ MXU_MAC_ADDRESS_STRING_LENGTH+1 ];
	char ip_address_string[ MXU_HOSTNAME_LENGTH+1 ];

	CyGrabber *grabber;
	CyResultEvent *grab_finished_event;
	mx_bool_type grab_in_progress;

} MX_PLEORA_IPORT_VINPUT;


#define MXD_PLEORA_IPORT_VINPUT_STANDARD_FIELDS \
  {-1, -1, "pleora_iport_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PLEORA_IPORT_VINPUT, pleora_iport_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "mac_address_string", MXFT_STRING, NULL, \
		1, {MXU_MAC_ADDRESS_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PLEORA_IPORT_VINPUT, mac_address_string), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "ip_address_string", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PLEORA_IPORT_VINPUT, ip_address_string), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxd_pleora_iport_vinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pleora_iport_vinput_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_pleora_iport_vinput_open( MX_RECORD *record );
MX_API mx_status_type mxd_pleora_iport_vinput_close( MX_RECORD *record );

MX_API mx_status_type mxd_pleora_iport_vinput_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_pleora_iport_vinput_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_pleora_iport_vinput_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_pleora_iport_vinput_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_pleora_iport_vinput_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_pleora_iport_vinput_get_extended_status(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_pleora_iport_vinput_get_frame(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_pleora_iport_vinput_get_parameter(
						MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_pleora_iport_vinput_set_parameter(
						MX_VIDEO_INPUT *vinput);

extern MX_RECORD_FUNCTION_LIST mxd_pleora_iport_vinput_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_pleora_iport_vinput_video_function_list;

extern long mxd_pleora_iport_vinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pleora_iport_vinput_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_PLEORA_IPORT_VINPUT_H__ */

