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

typedef struct {
	MX_RECORD *record;

	long parameter_type;

	long framesize[2];
	long image_format;
	long pixel_order;
} MX_VIDEO_INPUT;

#define MXLV_VIN_FRAMESIZE	11001
#define MXLV_VIN_FORMAT		11002
#define MXLV_VIN_PIXEL_ORDER	11003

#define MX_VIDEO_INPUT_STANDARD_FIELDS \
  {MXLV_VIN_FRAMESIZE, -1, "framesize", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, framesize), \
	{sizeof(long)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_VIN_FORMAT, -1, "image_format", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, image_format), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_VIN_PIXEL_ORDER, -1, "pixel_order", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VIDEO_INPUT, pixel_order), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

typedef struct {
	mx_status_type ( *arm ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *trigger ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *get_frame ) ( MX_VIDEO_INPUT *vinput,
					MX_IMAGE_FRAME **frame );
	mx_status_type ( *get_parameter ) ( MX_VIDEO_INPUT *vinput );
	mx_status_type ( *set_parameter ) ( MX_VIDEO_INPUT *vinput );
} MX_VIDEO_INPUT_FUNCTION_LIST;

MX_API mx_status_type mx_video_input_get_pointers( MX_RECORD *record,
					MX_VIDEO_INPUT **vinput,
				MX_VIDEO_INPUT_FUNCTION_LIST **flist_ptr,
					const char *calling_fname );

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

/*---*/

MX_API mx_status_type mx_video_input_set_exposure_time( MX_RECORD *record,
							double exposure_time );

MX_API mx_status_type mx_video_input_set_sequence( MX_RECORD *record,
					MX_SEQUENCE_INFO *sequence_info );

MX_API mx_status_type mx_video_input_set_continuous_mode(MX_RECORD *record,
							double exposure_time );

MX_API mx_status_type mx_video_input_enable_external_trigger(
							MX_RECORD *record );

MX_API mx_status_type mx_video_input_disable_external_trigger(
							MX_RECORD *record );

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
						MX_IMAGE_FRAME **frame );

MX_API mx_status_type mx_video_input_get_sequence( MX_RECORD *record,
						MX_IMAGE_SEQUENCE **sequence );

MX_API mx_status_type mx_video_input_get_frame_from_sequence(
						MX_RECORD *record,
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

