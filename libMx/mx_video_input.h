/*
 * Name:    mx_video_input.h
 *
 * Purpose: Functions for reading frames from a video input source.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_VIDEO_INPUT_H__
#define __MX_VIDEO_INPUT_H__

/* Status bit definitions for the 'status' field. */

#define MXSF_VIN_IS_BUSY	0x1

typedef struct {
	MX_RECORD *record;

	long parameter_type;
	long frame_number;

	long framesize[2];
	char image_format_name[MXU_IMAGE_FORMAT_NAME_LENGTH+1];
	long image_format;
	long pixel_order;
	long trigger_mode;
	long bytes_per_frame;

	mx_bool_type arm;
	mx_bool_type trigger;
	mx_bool_type stop;
	mx_bool_type abort;
	mx_bool_type busy;
	unsigned long status;

	MX_SEQUENCE_PARAMETERS sequence_parameters;

	/* Note: The get_frame() method expects to read the new frame
	 * into the 'frame' MX_IMAGE_FRAME structure.
	 */

	long get_frame;
	MX_IMAGE_FRAME *frame;

	/* 'frame_buffer' is used to provide a place for MX event handlers
	 * to find the contents of the most recently taken frame.  It must
	 * only be modified by mx_video_input_process_function() in
	 * libMx/pr_video_input.c.  No other functions should modify it.
	 */

	char *frame_buffer;
} MX_VIDEO_INPUT;

#define MXLV_VIN_FRAMESIZE			11001
#define MXLV_VIN_FORMAT_NAME			11002
#define MXLV_VIN_FORMAT				11003
#define MXLV_VIN_PIXEL_ORDER			11004
#define MXLV_VIN_TRIGGER_MODE			11005
#define MXLV_VIN_BYTES_PER_FRAME		11006
#define MXLV_VIN_ARM				11007
#define MXLV_VIN_TRIGGER			11008
#define MXLV_VIN_STOP				11009
#define MXLV_VIN_ABORT				11010
#define MXLV_VIN_BUSY				11011
#define MXLV_VIN_STATUS				11012
#define MXLV_VIN_SEQUENCE_TYPE			11013
#define MXLV_VIN_NUM_SEQUENCE_PARAMETERS	11014
#define MXLV_VIN_SEQUENCE_PARAMETER_ARRAY	11015
#define MXLV_VIN_GET_FRAME			11016
#define MXLV_VIN_FRAME_BUFFER			11017

#define MX_VIDEO_INPUT_STANDARD_FIELDS \
  {MXLV_VIN_FRAMESIZE, -1, "framesize", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, framesize), \
	{sizeof(long)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_VIN_FORMAT_NAME, -1, "image_format_name", MXFT_STRING, \
		NULL, 1, {MXU_IMAGE_FORMAT_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, image_format_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_VIN_FORMAT, -1, "image_format", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, image_format), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_PIXEL_ORDER, -1, "pixel_order", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, pixel_order), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_VIN_TRIGGER_MODE, -1, "trigger_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, trigger_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_BYTES_PER_FRAME, -1, "bytes_per_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, bytes_per_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_ARM, -1, "arm", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, arm), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_TRIGGER, -1, "trigger", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, trigger), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, stop), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_ABORT, -1, "abort", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, abort), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, busy), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, status), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_SEQUENCE_TYPE, -1, "sequence_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_VIDEO_INPUT, sequence_parameters.sequence_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_NUM_SEQUENCE_PARAMETERS, -1, "num_sequence_parameters", \
						MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
	    offsetof(MX_VIDEO_INPUT, sequence_parameters.num_parameters), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_SEQUENCE_PARAMETER_ARRAY, -1, "sequence_parameter_array", \
			MXFT_DOUBLE, NULL, 1, {MXU_MAX_SEQUENCE_PARAMETERS}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_VIDEO_INPUT, sequence_parameters.parameter_array), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_VIN_GET_FRAME, -1, "get_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, get_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_VIN_FRAME_BUFFER, -1, "frame_buffer", MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, frame_buffer), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}

typedef struct {
	mx_status_type ( *arm ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *trigger ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *stop ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *abort ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *busy ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *get_status ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *get_frame ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *get_sequence ) ( MX_VIDEO_INPUT *vinput,
					MX_IMAGE_SEQUENCE **sequence );
	mx_status_type ( *get_parameter ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *set_parameter ) ( MX_VIDEO_INPUT *vinput );
} MX_VIDEO_INPUT_FUNCTION_LIST;

MX_API mx_status_type mx_video_input_get_pointers( MX_RECORD *record,
					MX_VIDEO_INPUT **vinput,
				MX_VIDEO_INPUT_FUNCTION_LIST **flist_ptr,
					const char *calling_fname );

MX_API mx_status_type mx_video_input_finish_record_initialization(
						MX_RECORD *record );

/*---*/

MX_API mx_status_type mx_video_input_get_image_format( MX_RECORD *record,
							long *format );

MX_API mx_status_type mx_video_input_set_image_format( MX_RECORD *record,
							long format );

MX_API mx_status_type mx_video_input_get_framesize( MX_RECORD *record,
							long *x_framesize,
							long *y_framesize );

MX_API mx_status_type mx_video_input_set_framesize( MX_RECORD *record,
							long x_framesize,
							long y_framesize );

MX_API mx_status_type mx_video_input_get_bytes_per_frame( MX_RECORD *record,
							long *bytes_per_frame );
/*---*/

MX_API mx_status_type mx_video_input_set_exposure_time( MX_RECORD *record,
							double exposure_time );

MX_API mx_status_type mx_video_input_set_continuous_mode(MX_RECORD *record,
							double exposure_time );

MX_API mx_status_type mx_video_input_set_sequence_parameters(
				MX_RECORD *record,
				MX_SEQUENCE_PARAMETERS *sequence_parameters );

MX_API mx_status_type mx_video_input_set_trigger_mode( MX_RECORD *record,
							long trigger_mode );

MX_API mx_status_type mx_video_input_arm( MX_RECORD *record );

MX_API mx_status_type mx_video_input_trigger( MX_RECORD *record );

MX_API mx_status_type mx_video_input_start( MX_RECORD *record );

MX_API mx_status_type mx_video_input_stop( MX_RECORD *record );

MX_API mx_status_type mx_video_input_abort( MX_RECORD *record );

MX_API mx_status_type mx_video_input_is_busy( MX_RECORD *record,
						mx_bool_type *busy );

MX_API mx_status_type mx_video_input_get_status( MX_RECORD *record,
						long *last_frame_number,
						unsigned long *status_flags );

/*---*/

MX_API mx_status_type mx_video_input_get_frame( MX_RECORD *record,
						long frame_number,
						MX_IMAGE_FRAME **frame );

MX_API mx_status_type mx_video_input_get_sequence( MX_RECORD *record,
						MX_IMAGE_SEQUENCE **sequence );

MX_API mx_status_type mx_video_input_get_frame_from_sequence(
						MX_IMAGE_SEQUENCE *sequence,
						long frame_number,
						MX_IMAGE_FRAME **image_frame );

MX_API mx_status_type mx_video_input_read_1d_pixel_array(
						MX_IMAGE_FRAME *frame,
						long pixel_datatype,
						void *destination_pixel_array,
						size_t max_array_bytes,
						size_t *num_bytes_copied );

MX_API mx_status_type mx_video_input_read_1d_pixel_sequence(
						MX_IMAGE_SEQUENCE *sequence,
						long pixel_datatype,
						void *destination_pixel_array,
						size_t max_array_bytes,
						size_t *num_bytes_copied );

MX_API mx_status_type mx_video_input_default_get_parameter_handler(
						MX_VIDEO_INPUT *vinput );

MX_API mx_status_type mx_video_input_default_set_parameter_handler(
						MX_VIDEO_INPUT *vinput );


#endif /* __MX_VIDEO_INPUT_H__ */

