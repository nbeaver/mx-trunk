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
 * Copyright 2006-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_IMAGE_H__
#define __MX_IMAGE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_hrt.h"

/*---- Image format definitions ----*/

#define MXT_IMAGE_FORMAT_DEFAULT		(-1)

#define MXT_IMAGE_FORMAT_RGB			1
#define MXT_IMAGE_FORMAT_GREY8			2
#define MXT_IMAGE_FORMAT_GREY16			3
#define MXT_IMAGE_FORMAT_GREY32			4

#define MXT_IMAGE_FORMAT_RGB565			1001
#define MXT_IMAGE_FORMAT_YUYV			1002

    /* The following are for internal MX calculations. */

#define MXT_IMAGE_FORMAT_INT32			10001	/* signed integer */
#define MXT_IMAGE_FORMAT_DOUBLE			10002

/*---- Datafile format definitions ----*/

#define MXT_IMAGE_FILE_PNM			1

    /* The following are area detector file formats. */

#define MXT_IMAGE_FILE_SMV			101
#define MXT_IMAGE_FILE_MARCCD			102
#define MXT_IMAGE_FILE_EDF			103

#define MXU_IMAGE_SMV_HEADER_LENGTH		512

/*---- Sequence type definitions ----*/


#define MXT_SQ_ONE_SHOT				1
#define MXT_SQ_CONTINUOUS			2
#define MXT_SQ_MULTIFRAME			3
#define MXT_SQ_CIRCULAR_MULTIFRAME		4
#define MXT_SQ_STROBE				5
#define MXT_SQ_BULB				6

/*---- AVIEX specific sequence types ----*/

#define MXT_SQ_GEOMETRICAL			101
#define MXT_SQ_STREAK_CAMERA			102
#define MXT_SQ_SUBIMAGE				103

/*---- 32-bit image header definitions for the 'header_data' array. ----*/

/* 'bytes per million pixel' is a way of representing an intrinsically
 * floating point value as a 32-bit integer with reasonable accuracy.
 *
 * Image formats with a non-integer number of bytes per pixel are not
 * unprecedented.
 */

#define MXT_IMAGE_HEADER_BYTES				0
#define MXT_IMAGE_HEADER_ROW_FRAMESIZE			1
#define MXT_IMAGE_HEADER_COLUMN_FRAMESIZE		2
#define MXT_IMAGE_HEADER_ROW_BINSIZE			3
#define MXT_IMAGE_HEADER_COLUMN_BINSIZE			4
#define MXT_IMAGE_HEADER_IMAGE_FORMAT			5
#define MXT_IMAGE_HEADER_BYTE_ORDER			6
#define MXT_IMAGE_HEADER_BYTES_PER_MILLION_PIXELS	7
#define MXT_IMAGE_HEADER_BITS_PER_PIXEL			8
#define MXT_IMAGE_HEADER_EXPOSURE_TIME_SEC		9
#define MXT_IMAGE_HEADER_EXPOSURE_TIME_NSEC		10
#define MXT_IMAGE_HEADER_TIMESTAMP_SEC			11
#define MXT_IMAGE_HEADER_TIMESTAMP_NSEC			12
#define MXT_IMAGE_HEADER_BIAS_OFFSET_MILLI_ADUS		13

#define MXT_IMAGE_HEADER_LENGTH    (MXT_IMAGE_HEADER_BIAS_OFFSET_MILLI_ADUS+1)

#define MXT_IMAGE_HEADER_LENGTH_IN_BYTES \
			(MXT_IMAGE_HEADER_LENGTH * sizeof(uint32_t))

/*----*/

#define MXIF_HEADER_BYTES(i) \
			((i)->header_data[MXT_IMAGE_HEADER_BYTES])

#define MXIF_ROW_FRAMESIZE(i) \
			((i)->header_data[MXT_IMAGE_HEADER_ROW_FRAMESIZE])

#define MXIF_COLUMN_FRAMESIZE(i) \
			((i)->header_data[MXT_IMAGE_HEADER_COLUMN_FRAMESIZE])

#define MXIF_ROW_BINSIZE(i) \
			((i)->header_data[MXT_IMAGE_HEADER_ROW_BINSIZE])

#define MXIF_COLUMN_BINSIZE(i) \
			((i)->header_data[MXT_IMAGE_HEADER_COLUMN_BINSIZE])

#define MXIF_IMAGE_FORMAT(i) \
			((i)->header_data[MXT_IMAGE_HEADER_IMAGE_FORMAT])

#define MXIF_BYTE_ORDER(i) \
			((i)->header_data[MXT_IMAGE_HEADER_BYTE_ORDER])

#define MXIF_BYTES_PER_MILLION_PIXELS(i) \
		((i)->header_data[MXT_IMAGE_HEADER_BYTES_PER_MILLION_PIXELS])

#define MXIF_BYTES_PER_PIXEL(i) \
(1.0e-6 * (double)((i)->header_data[MXT_IMAGE_HEADER_BYTES_PER_MILLION_PIXELS]))

#define MXIF_SET_BYTES_PER_PIXEL(i,b) \
  ((i)->header_data[MXT_IMAGE_HEADER_BYTES_PER_MILLION_PIXELS] = 1.0e6 * (b))
				

#define MXIF_BITS_PER_PIXEL(i) \
			((i)->header_data[MXT_IMAGE_HEADER_BITS_PER_PIXEL])

#define MXIF_EXPOSURE_TIME_SEC(i) \
			((i)->header_data[MXT_IMAGE_HEADER_EXPOSURE_TIME_SEC])

#define MXIF_EXPOSURE_TIME_NSEC(i) \
			((i)->header_data[MXT_IMAGE_HEADER_EXPOSURE_TIME_NSEC])

#define MXIF_TIMESTAMP_SEC(i) \
			((i)->header_data[MXT_IMAGE_HEADER_TIMESTAMP_SEC])

#define MXIF_TIMESTAMP_NSEC(i) \
			((i)->header_data[MXT_IMAGE_HEADER_TIMESTAMP_NSEC])

#define MXIF_BIAS_OFFSET_MILLI_ADUS(i) \
		((i)->header_data[MXT_IMAGE_HEADER_BIAS_OFFSET_MILLI_ADUS])

/*----*/

typedef struct {
	size_t allocated_header_length;
	size_t allocated_image_length;

	size_t header_length;
	uint32_t *header_data;

	size_t image_length;
	void *image_data;

} MX_IMAGE_FRAME;

typedef struct {
	long num_frames;
	MX_IMAGE_FRAME **frame_array;
} MX_IMAGE_SEQUENCE;

/*----*/

/* DANGER! DANGER! DANGER!
 *
 * Please try to avoid ever having to change the value below of
 * MXU_MAX_SEQUENCE_PARAMETERS from its initial value of 250!
 *
 * One of the features of XDR is that it _REQUIRES_ the
 * receiving end to read out exactly the _same_ number of
 * elements as were transmitted by the transmitting end.
 * If this is not the case, then xdr_array() throws a
 * tantrum over it.  This behavior makes sense when XDR
 * is invisibly buried inside SunRPC, but it is a big
 * nuisance from the point of view of MX.
 *
 * If you ever find yourself needing to increase the value of
 * the MXU_MAX_SEQUENCE_PARAMETERS value, then you will have
 * two choices on how to deal with this:
 *
 * 1.  Write some tricky custom code that interrogates the
 *     MX server for the size of 'parameter_array' and then
 *     figures out how to compensate for the differing 
 *     arrays lengths.  Since, in XDR format, the length of
 *     the array is found as a 32-bit integer at the start
 *     of the array, it may just be sufficient to patch
 *     the value at runtime.  Or maybe not.
 *
 * 2.  Write a custom replacement for xdr_array() that manages
 *     array lengths differently.
 *
 * In the long run, I would guess that solution 2 is probably
 * the easier and most general solution, but I have not tried
 * it yet, so I could be wrong.
 *
 * NOTE (2009-10-06): Subversion revision 1445 changed XDR format
 * support so that the server can send a longer array than the
 * client is expecting without XDR in the client complaining about
 * it.  This may fix the problem mentioned above, but this will
 * need to be checked.
 */

#define MXU_MAX_SEQUENCE_PARAMETERS	250	/* See above! */

typedef struct {
	long sequence_type;
	long num_parameters;
	double parameter_array[MXU_MAX_SEQUENCE_PARAMETERS];

} MX_SEQUENCE_PARAMETERS;

/*----*/

#define MXU_IMAGE_FORMAT_NAME_LENGTH	10

#define MXU_IMAGE_FILE_NAME_LENGTH	10

/* Trigger modes for video inputs and area detectors. */

#define MXT_IMAGE_NO_TRIGGER		0x0
#define MXT_IMAGE_INTERNAL_TRIGGER	0x1
#define MXT_IMAGE_EXTERNAL_TRIGGER	0x2
#define MXT_IMAGE_DATABASE_TRIGGER	0x4
#define MXT_IMAGE_MANUAL_TRIGGER	0x8

/*----*/

MX_API mx_status_type mx_image_get_image_format_type_from_name( char *name,
								long *type );

MX_API mx_status_type mx_image_get_image_format_name_from_type( long type,
								char *name,
							size_t max_name_length);

MX_API mx_status_type mx_image_format_get_bytes_per_pixel( long type,
						double *bytes_per_pixel );

MX_API mx_status_type mx_image_get_file_format_type_from_name( char *name,
								long *type );

MX_API mx_status_type mx_image_get_file_format_name_from_type( long type,
								char *name,
							size_t max_name_length);

MX_API mx_status_type mx_image_get_image_format_type_from_name( char *name,
								long *type );

MX_API mx_status_type mx_image_get_image_format_name_from_type( long type,
								char *name,
							size_t max_name_length);

/*----*/

MX_API mx_status_type mx_image_alloc( MX_IMAGE_FRAME **frame,
					long row_framesize,
					long column_framesize,
					long image_format,
					long byte_order,
					double bytes_per_pixel,
					size_t header_length,
					size_t image_length );

MX_API void mx_image_free( MX_IMAGE_FRAME *frame );

/*----*/

MX_API mx_status_type mx_image_alloc_sector_array( MX_IMAGE_FRAME *frame,
					long num_sector_rows,
					long num_sector_columns,
					void ****sector_array_pointer );

MX_API void mx_image_free_sector_array( void ***sector_array );
					
/*----*/

MX_API mx_status_type mx_image_get_frame_from_sequence(
					MX_IMAGE_SEQUENCE *sequence,
					long frame_number,
					MX_IMAGE_FRAME **image_frame );

MX_API mx_status_type mx_image_get_exposure_time( MX_IMAGE_FRAME *frame,
						double *exposure_time );

MX_API mx_status_type mx_image_get_average_intensity(
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					double *average_intensity );

MX_API mx_status_type mx_image_statistics( MX_IMAGE_FRAME *frame );

MX_API mx_status_type mx_image_get_image_data_pointer( MX_IMAGE_FRAME *frame,
						size_t *image_length,
						void **image_data_pointer );

MX_API mx_status_type mx_image_copy_1d_pixel_array( MX_IMAGE_FRAME *frame,
						void *destination_pixel_array,
						size_t max_array_bytes,
						size_t *num_bytes_copied );

MX_API mx_status_type mx_image_copy_frame( MX_IMAGE_FRAME *old_frame,
					MX_IMAGE_FRAME **new_frame );

MX_API mx_status_type mx_image_copy_header( MX_IMAGE_FRAME *source_frame,
					MX_IMAGE_FRAME *destination_frame );

/*----*/

MX_API mx_status_type mx_image_rebin( MX_IMAGE_FRAME **rebinned_frame,
				MX_IMAGE_FRAME *original_frame,
				unsigned long rebinned_row_framesize,
				unsigned long rebinned_column_framesize );

/*----*/

MX_API mx_status_type mx_image_dezinger( MX_IMAGE_FRAME **dezingered_frame,
					unsigned long num_original_frames,
					MX_IMAGE_FRAME **original_frame_array,
					double threshold );

/*----*/

MX_API mx_status_type mx_image_read_file( MX_IMAGE_FRAME **frame,
					unsigned long datafile_type,
					char *datafile_name );

MX_API mx_status_type mx_image_write_file( MX_IMAGE_FRAME *frame,
					unsigned long datafile_type,
					char *datafile_name );

/*----*/

MX_API mx_status_type mx_image_read_pnm_file( MX_IMAGE_FRAME **frame,
						char *datafile_name );

MX_API mx_status_type mx_image_write_pnm_file( MX_IMAGE_FRAME *frame,
						char *datafile_name );

/*----*/

MX_API mx_status_type mx_image_read_smv_file( MX_IMAGE_FRAME **frame,
						char *datafile_name );

MX_API mx_status_type mx_image_write_smv_file( MX_IMAGE_FRAME *frame,
						char *datafile_name );

/*----*/

MX_API mx_status_type mx_image_read_marccd_file( MX_IMAGE_FRAME **frame,
						char *datafile_name );

/*----*/

MX_API mx_status_type mx_image_read_edf_file( MX_IMAGE_FRAME **frame,
						char *datafile_name );

/*----*/

MX_API mx_status_type mx_sequence_get_exposure_time( MX_SEQUENCE_PARAMETERS *sp,
							long frame_number,
							double *exposure_time );

MX_API mx_status_type mx_sequence_get_frame_time( MX_SEQUENCE_PARAMETERS *sp,
							long frame_number,
							double *frame_time );

MX_API mx_status_type mx_sequence_get_sequence_time( MX_SEQUENCE_PARAMETERS *sp,
							long last_frame_number,
							double *sequence_time );

MX_API mx_status_type mx_sequence_get_num_frames( MX_SEQUENCE_PARAMETERS *sp,
							long *num_frames );

#ifdef __cplusplus
}
#endif

#endif /* __MX_IMAGE_H__ */

