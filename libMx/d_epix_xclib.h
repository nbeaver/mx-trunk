/*
 * Name:    d_epix_xclib.h
 *
 * Purpose: MX driver header for EPIX, Inc. video inputs via XCLIB.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPIX_XCLIB_H__
#define __D_EPIX_XCLIB_H__

/* Values for epix_xclib_vinput_flags */

#define MXF_EPIX_SHOW_CONFIG		0x1

#define MXF_EPIX_WRITE_TEST		0x1000

typedef struct {
	MX_RECORD *record;

	MX_RECORD *xclib_record;
	long unit_number;

	char ready_for_trigger_name[MXU_RECORD_NAME_LENGTH+1];
	MX_RECORD *ready_for_trigger_record;

	unsigned long epix_xclib_vinput_flags;
	unsigned long write_test_value;

	long unitmap;
	double default_trigger_time;
	long pixel_clock_divisor;

	mx_bool_type sequence_in_progress;
	mx_bool_type new_sequence;
	long old_total_num_frames;

	long circular_frame_period;

	unsigned long num_write_test_array_bytes;
	uint16_t *write_test_array;

	long fake_frame_numbers[2];		/* Used for testing only. */

	long test_num_pixels_to_save;

#if defined(OS_WIN32)
	HANDLE captured_field_event;
	HANDLE captured_field_thread;
	uint32_t volatile uint32_total_num_frames;
#else
	int captured_field_signal;
#endif

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
  {-1, -1, "ready_for_trigger_name", MXFT_STRING, \
				NULL, 1, {MXU_RECORD_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, ready_for_trigger_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "epix_xclib_vinput_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, epix_xclib_vinput_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "write_test_value", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, write_test_value), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "fake_frame_numbers", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, fake_frame_numbers), \
	{sizeof(long)}, NULL, 0}, \
  \
  {-1, -1, "test_num_pixels_to_save", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPIX_XCLIB_VIDEO_INPUT, test_num_pixels_to_save), \
	{0}, NULL, 0}

MX_API mx_status_type mxd_epix_xclib_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_open( MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_close( MX_RECORD *record );
MX_API mx_status_type mxd_epix_xclib_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_epix_xclib_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_get_last_frame_number(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_epix_xclib_get_total_num_frames(
						MX_VIDEO_INPUT *vinput );
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

