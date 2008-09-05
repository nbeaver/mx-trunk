/*
 * Name:    d_soft_vinput.h
 *
 * Purpose: MX header file for a software emulated video input device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_VINPUT_H__
#define __D_SOFT_VINPUT_H__

#define MXU_SOFT_VINPUT_IMAGE_PARAMETERS_LENGTH 40

/* Define values for the 'image_type' field below. */

#define MXT_SOFT_VINPUT_DIAGONAL_GRADIENT	1
#define MXT_SOFT_VINPUT_LINES			2
#define MXT_SOFT_VINPUT_LOGARITHMIC_SPIRAL	3

typedef struct {
	MX_RECORD *record;

	long image_type;
	char image_parameters[MXU_SOFT_VINPUT_IMAGE_PARAMETERS_LENGTH+1];

	long old_total_num_frames;
	long num_frames_in_sequence;
	double seconds_per_frame;
	MX_CLOCK_TICK start_tick;
	mx_bool_type sequence_in_progress;
} MX_SOFT_VINPUT;


#define MXD_SOFT_VINPUT_STANDARD_FIELDS \
  {-1, -1, "image_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_VINPUT, image_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "image_parameters", MXFT_STRING, NULL, \
  			1, {MXU_SOFT_VINPUT_IMAGE_PARAMETERS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_VINPUT, image_parameters), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxd_soft_vinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_vinput_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_vinput_open( MX_RECORD *record );
MX_API mx_status_type mxd_soft_vinput_close( MX_RECORD *record );

MX_API mx_status_type mxd_soft_vinput_arm( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_soft_vinput_trigger( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_soft_vinput_stop( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_soft_vinput_abort( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_soft_vinput_asynchronous_capture(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_soft_vinput_get_extended_status(
						MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_soft_vinput_get_frame( MX_VIDEO_INPUT *vinput );
MX_API mx_status_type mxd_soft_vinput_get_parameter(MX_VIDEO_INPUT *vinput);
MX_API mx_status_type mxd_soft_vinput_set_parameter(MX_VIDEO_INPUT *vinput);

extern MX_RECORD_FUNCTION_LIST mxd_soft_vinput_record_function_list;
extern MX_VIDEO_INPUT_FUNCTION_LIST mxd_soft_vinput_video_function_list;

extern long mxd_soft_vinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_vinput_rfield_def_ptr;

#endif /* __D_SOFT_VINPUT_H__ */

