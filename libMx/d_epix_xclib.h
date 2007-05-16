/*
 * Name:    d_epix_xclib.h
 *
 * Purpose: MX driver header for EPIX, Inc. video inputs via XCLIB.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPIX_XCLIB_H__
#define __D_EPIX_XCLIB_H__

/* Values for epix_xclib_vinput_flags */

#define MXF_EPIX_WRITE_TEST	0x1000

typedef struct {
	MX_RECORD *record;

	MX_RECORD *xclib_record;
	long unit_number;
	MX_RECORD *camera_link_record;
	unsigned long epix_xclib_vinput_flags;
	unsigned long write_test_value;

	long unitmap;
	double default_trigger_time;
	long pixel_clock_divisor;

	unsigned long num_write_test_array_bytes;
	uint16_t *write_test_array;

} MX_EPIX_XCLIB_VIDEO_INPUT;

#define MXD_EPIX_XCLIB_STANDARD_FIELDS \
  {-1, -1, "xclib_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, xclib_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "unit_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, unit_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_link_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, camera_link_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "epix_xclib_vinput_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, epix_xclib_vinput_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "write_test_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, write_test_value), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_epix_xclib_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_open( MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_close( MX_RECORD *record );

MX_API mx_status_type mxd_epix_xclib_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_get_last_frame_number(
						MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_epix_xclib_get_total_num_frames(
						MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_epix_xclib_get_status( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_get_extended_status(
						MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_epix_xclib_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_get_parameter( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_set_parameter( MX_VIDEO_INPUT *vinput );

extern MX_RECORD_FUNCTION_LIST mxd_epix_xclib_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_epix_xclib_video_function_list;

extern long mxd_epix_xclib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epix_xclib_rfield_def_ptr;

#endif /* __D_EPIX_XCLIB_H__ */

