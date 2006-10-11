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

#define MXT_IMAGE_FORMAT_DEFAULT	(-1)

#define MXT_IMAGE_FORMAT_RGB565		1
#define MXT_IMAGE_FORMAT_YUYV		2

#define MXT_IMAGE_FORMAT_RGB		1001
#define MXT_IMAGE_FORMAT_GREY8		1002
#define MXT_IMAGE_FORMAT_GREY16		1003

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
	double bytes_per_pixel;

	size_t header_length;
	void *header_data;

	size_t image_length;
	void *image_data;

} MX_IMAGE_FRAME;

typedef struct {
	long num_frames;
	MX_IMAGE_FRAME **frame_array;
} MX_IMAGE_SEQUENCE;

#define MXU_MAX_SEQUENCE_PARAMETERS	250

typedef struct {
	long sequence_type;
	long num_parameters;
	double parameter_array[MXU_MAX_SEQUENCE_PARAMETERS];

} MX_SEQUENCE_PARAMETERS;

/*----*/

#define MXU_IMAGE_FORMAT_NAME_LENGTH	10

/* Trigger modes for video inputs and area detectors. */

#define MXT_IMAGE_NO_TRIGGER		0x0
#define MXT_IMAGE_INTERNAL_TRIGGER	0x1
#define MXT_IMAGE_EXTERNAL_TRIGGER	0x2

/*----*/

MX_API mx_status_type mx_get_image_format_type_from_name( char *name,
							long *type );

MX_API mx_status_type mx_get_image_format_name_from_type( long type,
							char *name,
							size_t max_name_length);

MX_API mx_status_type mx_image_alloc( MX_IMAGE_FRAME **frame,
					long image_type,
					long *framesize,
					long image_format,
					long pixel_order,
					double bytes_per_pixel,
					size_t header_length,
					size_t image_length );

MX_API void mx_image_free( MX_IMAGE_FRAME *frame );
					

MX_API mx_status_type mx_image_get_exposure_time( MX_IMAGE_FRAME *frame,
						double *exposure_time );

MX_API mx_status_type mx_copy_image_frame( MX_IMAGE_FRAME **new_frame,
					MX_IMAGE_FRAME *old_frame );

MX_API mx_status_type mx_image_get_image_data_pointer( MX_IMAGE_FRAME *frame,
						size_t *image_length,
						void **image_data_pointer );

MX_API mx_status_type mx_image_copy_1d_pixel_array( MX_IMAGE_FRAME *frame,
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

MX_API mx_status_type mx_read_pnm_image_file( MX_IMAGE_FRAME **frame,
						char *datafile_name );

MX_API mx_status_type mx_write_pnm_image_file( MX_IMAGE_FRAME *frame,
						char *datafile_name );

#endif /* __MX_IMAGE_H__ */

