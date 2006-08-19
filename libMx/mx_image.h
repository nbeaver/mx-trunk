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

typedef struct {
	long image_type;
	void *image_data;

} MX_IMAGE_FRAME;

typedef struct {
	mx_bool_type sequence_data_is_contiguous;

	long num_frames;
	MX_IMAGE_FRAME *frame_array;

} MX_IMAGE_SEQUENCE;

typedef struct {
	long sequence_type;
	long num_parameters;
	double *parameter_array;

} MX_SEQUENCE_INFO;

MX_API mx_status_type mx_copy_image_frame( MX_IMAGE_FRAME **new_frame,
					MX_IMAGE_FRAME *old_frame );

MX_API mx_status_type mx_copy_image_sequence( MX_IMAGE_SEQUENCE **new_sequence,
					MX_IMAGE_SEQUENCE *old_sequence );

#endif /* __MX_IMAGE_H__ */

