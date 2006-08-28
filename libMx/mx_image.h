/*
 * Name:    mx_image.h
 *
 * Purpose: Definitions for 2-dimensional MX images and 3-dimensional
 *          MX sequences.
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

#ifndef __MX_IMAGE_H__
#define __MX_IMAGE_H__

/*---- Image type definitions ----*/

#define MXT_IMAGE_LOCAL_1D_ARRAY	1

/*---- Image format definitions ----*/

#define MXT_IMAGE_FORMAT_RGB565		1
#define MXT_IMAGE_FORMAT_YUYV		2

#define MXT_IMAGE_FORMAT_GREY16		1600

/*---- Pixel order definitions ----*/

#define MXT_IMAGE_PIXEL_ORDER_STANDARD	1

/*---- Datafile format definitions ----*/

#define MXT_IMAGE_FILE_PNM		1
#define MXT_IMAGE_FILE_TIFF		2

/*---- Sequence type definitions ----*/


#define MXT_SQ_ONE_SHOT			1
#define MXT_SQ_CONTINUOUS		2    /* Overwrites a single buffer. */

#define MXT_SQ_MULTI			3
#define MXT_SQ_CONTINUOUS_MULTI		4

/*----*/

typedef struct {
	long image_type;
	long framesize[2];
	long image_format;
	long pixel_order;

	size_t header_length;
	void *header_data;

	size_t image_length;
	void *image_data;

} MX_IMAGE_FRAME;

typedef struct {
	mx_bool_type sequence_data_is_contiguous;

	long num_frames;
	MX_IMAGE_FRAME *frame_array;

} MX_IMAGE_SEQUENCE;

#define MXU_MAX_SEQUENCE_PARAMETERS	250

typedef struct {
	long sequence_type;
	long num_sequence_parameters;
	double sequence_parameters[MXU_MAX_SEQUENCE_PARAMETERS];

} MX_SEQUENCE_INFO;

/*----*/

MX_API mx_status_type mx_copy_image_frame( MX_IMAGE_FRAME **new_frame,
					MX_IMAGE_FRAME *old_frame );

MX_API mx_status_type mx_copy_image_sequence( MX_IMAGE_SEQUENCE **new_sequence,
					MX_IMAGE_SEQUENCE *old_sequence );


MX_API mx_status_type mx_image_read_1d_pixel_array( MX_IMAGE_FRAME *frame,
						long pixel_datatype,
						void *destination_pixel_array,
						size_t max_array_bytes,
						size_t *num_bytes_copied );

MX_API mx_status_type mx_image_read_1d_pixel_sequence(
						MX_IMAGE_SEQUENCE *sequence,
						long pixel_datatype,
						void *destination_pixel_array,
						size_t max_array_bytes,
						size_t *num_bytes_copied );

MX_API mx_status_type mx_read_image_file( MX_IMAGE_FRAME **frame,
					unsigned long datafile_type,
					void *datafile_args,
					char *datafile_name );

MX_API mx_status_type mx_write_image_file( MX_IMAGE_FRAME *frame,
					unsigned long datafile_type,
					void *datafile_args,
					char *datafile_name );

/*----*/

MX_API mx_status_type mx_write_pnm_image_file( MX_IMAGE_FRAME *frame,
						char *datafile_name );

#endif /* __MX_IMAGE_H__ */

