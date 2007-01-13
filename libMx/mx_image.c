/*
 * Name:    mx_image.c
 *
 * Purpose: Functions for 2-dimensional MX images and 3-dimensional
 *          MX sequences.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_IMAGE_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_bit.h"
#include "mx_image.h"

typedef struct {
	int num_source_bytes;
	int num_destination_bytes;
	void (*converter_fn)( unsigned char *src, unsigned char *dest );

} pixel_converter_t;

static void
mxp_rgb565_converter_fn( unsigned char *src, unsigned char *dest )
{
	unsigned char byte0, byte1;

	byte0 = src[0];
	byte1 = src[1];

	/* Red */
	dest[0]  = byte0 & 0x1f;

	/* Green */
	dest[1]  = (( byte0 >> 5 ) & 0x7) | (( byte1 & 0x7 ) << 3);

	/* Blue */
	dest[2]  = ( byte1 >> 3 ) & 0x1f;

	return;
}

static pixel_converter_t
mxp_rgb565_converter = { 2, 3, mxp_rgb565_converter_fn };

static long
clamp( double x )
{
	long r;

	r = mx_round(x);

	if (r < 0) {
		return 0;
	} else if (r > 255) {
		return 255;
	} else {
		return r;
	}
}

static void
mxp_yuyv_converter_fn( unsigned char *src, unsigned char *dest )
{
	unsigned int Y0, Y1, Cb, Cr;
	double Y0_temp, Y1_temp, Cb_temp, Cr_temp;
	double R0_temp, G0_temp, B0_temp;
	double R1_temp, G1_temp, B1_temp;

	Y0 = src[0];
	Cb = src[1];
	Y1 = src[2];
	Cr = src[3];

#if 0
	MX_DEBUG(-2,("Y0 = %u, Cb = %u, Y1 = %u, Cr = %u", Y0, Cb, Y1, Cr));
#endif

	Y0_temp = (255 / 219.0) * ((int)Y0 - 16);
	Y1_temp = (255 / 219.0) * ((int)Y1 - 16);
	Cb_temp = (255 / 224.0) * ((int)Cb - 128);
	Cr_temp = (255 / 224.0) * ((int)Cr - 128);

#if 0
	MX_DEBUG(-2,("Y0_temp = %g, Y1_temp = %g, Cb_temp = %g, Cr_temp = %g",
			Y0_temp, Y1_temp, Cb_temp, Cr_temp));
#endif

	R0_temp = 1.0 * Y0_temp + 0     * Cb_temp + 1.402 * Cr_temp;
	G0_temp = 1.0 * Y0_temp - 0.344 * Cb_temp + 0.714 * Cr_temp;
	B0_temp = 1.0 * Y0_temp + 1.772 * Cb_temp + 0     * Cr_temp;

#if 0
	MX_DEBUG(-2,("R0_temp = %g, G0_temp = %g, B0_temp = %g",
		R0_temp, G0_temp, B0_temp));
#endif


	R1_temp = 1.0 * Y1_temp + 0     * Cb_temp + 1.402 * Cr_temp;
	G1_temp = 1.0 * Y1_temp - 0.344 * Cb_temp + 0.714 * Cr_temp;
	B1_temp = 1.0 * Y1_temp + 1.772 * Cb_temp + 0     * Cr_temp;

#if 0
	MX_DEBUG(-2,("R1_temp = %g, G1_temp = %g, B1_temp = %g",
		R1_temp, G1_temp, B1_temp));
#endif

	dest[0] = clamp( R0_temp );
	dest[1] = clamp( G0_temp );
	dest[2] = clamp( B0_temp );
	dest[3] = clamp( R1_temp );
	dest[4] = clamp( G1_temp );
	dest[5] = clamp( B1_temp );

	return;
}

static pixel_converter_t
mxp_yuyv_converter = { 4, 6, mxp_yuyv_converter_fn };

/* FIXME - The following converters are lame and inefficient. */

static void
mxp_rgb_converter_fn( unsigned char *src, unsigned char *dest )
{
	/* Move three bytes from the source to the destination. */

	memcpy( dest, src, 3 );
}

static pixel_converter_t
mxp_rgb_converter = { 3, 3, mxp_rgb_converter_fn };

static void
mxp_grey8_converter_fn( unsigned char *src, unsigned char *dest )
{
	/* Move one byte from the source to the destination. */

	*dest = *src;
}

static pixel_converter_t
mxp_grey8_converter = { 1, 1, mxp_grey8_converter_fn };

static void
mxp_grey16_converter_fn( unsigned char *src, unsigned char *dest )
{
	/* Move two bytes from the source to the destination. */

	memcpy( dest, src, 2 );
}

static pixel_converter_t
mxp_grey16_converter = { 2, 2, mxp_grey16_converter_fn };

/*----*/

typedef struct {
	char name[MXU_IMAGE_FORMAT_NAME_LENGTH+1];
	long type;
} MX_IMAGE_FORMAT_ENTRY;

static MX_IMAGE_FORMAT_ENTRY mxp_image_format_table[] =
{
	{"DEFAULT", MXT_IMAGE_FORMAT_DEFAULT},

	{"RGB565",  MXT_IMAGE_FORMAT_RGB565},
	{"YUYV",    MXT_IMAGE_FORMAT_YUYV},

	{"RGB",     MXT_IMAGE_FORMAT_RGB},
	{"GREY8",   MXT_IMAGE_FORMAT_GREY8},
	{"GREY16",  MXT_IMAGE_FORMAT_GREY16},

	{"GRAY8",   MXT_IMAGE_FORMAT_GREY8},
	{"GRAY16",  MXT_IMAGE_FORMAT_GREY16},
};

static size_t mxp_image_format_table_length
	= sizeof(mxp_image_format_table) / sizeof(mxp_image_format_table[0]);

MX_EXPORT mx_status_type
mx_image_get_format_type_from_name( char *name, long *type )
{
	static const char fname[] = "mx_image_get_format_type_from_name()";

	MX_IMAGE_FORMAT_ENTRY *entry;
	long i;

	if ( name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image format name pointer passed was NULL." );
	}
	if ( type == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image format type pointer passed was NULL." );
	}

	if ( strlen(name) == 0 ) {
		*type = MXT_IMAGE_FORMAT_DEFAULT;

		return MX_SUCCESSFUL_RESULT;
	}

	for ( i = 0; i < mxp_image_format_table_length; i++ ) {
		entry = &mxp_image_format_table[i];

		if ( mx_strcasecmp( entry->name, name ) == 0 ) {
			*type = entry->type;

			return MX_SUCCESSFUL_RESULT;
		}
	}

	return mx_error( MXE_UNSUPPORTED, fname,
	"Image format type '%s' is not currently supported by MX.", name );
}

MX_EXPORT mx_status_type
mx_image_get_format_name_from_type( long type,
				char *name, size_t max_name_length )
{
	static const char fname[] = "mx_image_get_format_name_from_type()";

	MX_IMAGE_FORMAT_ENTRY *entry;
	long i;

	if ( name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image format name pointer passed was NULL." );
	}

	for ( i = 0; i < mxp_image_format_table_length; i++ ) {
		entry = &mxp_image_format_table[i];

		if ( entry->type == type ) {
			strlcpy( name, entry->name, max_name_length );

			return MX_SUCCESSFUL_RESULT;
		}
	}

	return mx_error( MXE_UNSUPPORTED, fname,
	"Image format type %ld is not currently supported by MX.", type );
}

/*----*/

MX_EXPORT mx_status_type
mx_image_alloc( MX_IMAGE_FRAME **frame,
			long image_type,
			long *framesize,
			long image_format,
			long pixel_order,
			double bytes_per_pixel,
			size_t header_length,
			size_t image_length )
{
	static const char fname[] = "mx_image_alloc()";

	unsigned long bytes_per_frame;
	double double_bytes_per_frame;

	if ( frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		    "The MX_IMAGE_FRAME pointer to pointer passed was NULL.");
	}
	if ( framesize == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The framesize pointer passed was NULL." );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s invoked for *frame = %p", fname, *frame));
#endif

	/* We either reuse an existing MX_IMAGE_FRAME or create a new one. */

	if ( (*frame) == (MX_IMAGE_FRAME *) NULL ) {

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: Allocating a new MX_IMAGE_FRAME.", fname));
#endif
		/* Allocate a new MX_IMAGE_FRAME. */

		*frame = malloc( sizeof(MX_IMAGE_FRAME) );

		if ( (*frame) == (MX_IMAGE_FRAME *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"a new MX_IMAGE_FRAME structure." );
		}

		memset( *frame, 0, sizeof(MX_IMAGE_FRAME) );
	}

	/* Fill in some parameters. */

	(*frame)->image_type = image_type;
	(*frame)->framesize[0] = framesize[0];
	(*frame)->framesize[1] = framesize[1];
	(*frame)->image_format = image_format;
	(*frame)->pixel_order = pixel_order;
	(*frame)->bytes_per_pixel = bytes_per_pixel;

	/*** See if the header buffer is already big enough for the header. ***/

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: (*frame)->header_data = %p",
		fname, (*frame)->header_data));
	MX_DEBUG(-2,
	("%s: (*frame)->header_length = %lu, header_length = %lu",
		fname, (unsigned long) (*frame)->header_length,
		(unsigned long) header_length));
#endif

	if ( header_length == 0 ) {
#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: No header needed.", fname));
#endif
		(*frame)->header_length = 0;

		if ( (*frame)->header_data != NULL ) {

#if MX_IMAGE_DEBUG
			MX_DEBUG(-2,
			("%s: Freeing unneeded header buffer.", fname));
#endif
			free( (*frame)->header_data );
		}

		(*frame)->header_data = NULL;
	} else
	if ( (*frame)->header_data == NULL ) {

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: Allocating new %lu byte header.",
			fname, (unsigned long) header_length ));
#endif
		(*frame)->header_data = malloc( header_length );

		if ( (*frame)->header_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a "
			"%lu byte header buffer.",
				(unsigned long) header_length );
		}

		(*frame)->header_length = header_length;
	} else {
		if ( (*frame)->header_length >= header_length ) {
#if MX_IMAGE_DEBUG
			MX_DEBUG(-2,
			("%s: The header buffer is already big enough.",fname));
#endif
		} else {
#if MX_IMAGE_DEBUG
			MX_DEBUG(-2,
			("%s: Allocating a new header buffer of %lu bytes.",
				fname, (unsigned long) header_length));
#endif
			if ( (*frame)->header_data != NULL ) {
				free( (*frame)->header_data );
			}

			(*frame)->header_data = malloc( header_length );

			if ( (*frame)->header_data == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to allocate a "
				"%lu byte header buffer for frame %p.",
					(unsigned long) header_length, *frame );
			}

			(*frame)->header_length = header_length;
		}
	}

	/*** See if the image buffer is already big enough for the image. ***/

	double_bytes_per_frame =
	    bytes_per_pixel * ((double) framesize[0]) * ((double) framesize[1]);

	bytes_per_frame = mx_round( double_bytes_per_frame );

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: (*frame)->image_data = %p",
		fname, (*frame)->image_data));
	MX_DEBUG(-2,
	("%s: (*frame)->image_length = %lu, bytes_per_frame = %lu",
		fname, (unsigned long) (*frame)->image_length,
		bytes_per_frame));
#endif

	/* Setup the image data buffer. */

	if ( ((*frame)->image_length == 0) && (bytes_per_frame == 0)) {

		/* Zero length image buffers are not allowed. */

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Attempted to create a zero length image buffer for frame %p.",
			*frame );

	} else
	if ( ( (*frame)->image_data != NULL )
	  && ( (*frame)->image_length >= bytes_per_frame ) )
	{
#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,
		("%s: The image buffer is already big enough.", fname));
#endif
	} else {

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: Allocating a new image buffer of %lu bytes.",
			fname, bytes_per_frame));
#endif
		/* If not, then allocate a new one. */

		if ( (*frame)->image_data != NULL ) {
			free( (*frame)->image_data );
		}

		(*frame)->image_data = malloc( bytes_per_frame );

		if ( (*frame)->image_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld byte "
			"image buffer for frame %p",
				bytes_per_frame, *frame );
		}

		(*frame)->image_length = bytes_per_frame;

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: allocated new frame buffer.", fname));
#endif
	}

#if 0  /* FIXME!!! - This should not be present in the final version. */
	memset( (*frame)->image_data, 0, 50 );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_image_free( MX_IMAGE_FRAME *frame )
{
	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return;
	}

	if ( frame->header_data != NULL ) {
		free( frame->header_data );
	}

	if ( frame->image_data != NULL ) {
		free( frame->image_data );
	}

	free( frame );

	return;
}

/*----*/

MX_EXPORT mx_status_type
mx_image_get_frame_from_sequence( MX_IMAGE_SEQUENCE *image_sequence,
				long frame_number,
				MX_IMAGE_FRAME **image_frame )
{
	static const char fname[] = "mx_image_get_frame_from_sequence()";

	if ( frame_number < ( - image_sequence->num_frames ) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Since the image sequence only has %ld frames, "
		"frame %ld would be before the first frame in the sequence.",
			image_sequence->num_frames, frame_number );
	} else
	if ( frame_number < 0 ) {

		/* -1 returns the last frame, -2 returns the second to last,
		 * and so forth.
		 */

		frame_number = image_sequence->num_frames - ( - frame_number );
	} else
	if ( frame_number >= image_sequence->num_frames ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Frame %ld would be beyond the last frame (%ld), "
		"since the sequence only has %ld frames.",
			frame_number, image_sequence->num_frames - 1,
			image_sequence->num_frames ) ;
	}

	*image_frame = image_sequence->frame_array[ frame_number ];

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_image_get_exposure_time( MX_IMAGE_FRAME *frame,
				double *exposure_time )
{
	static const char fname[] = "mx_image_get_exposure_time()";

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}
	if ( exposure_time == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The exposure_time pointer passed was NULL." );
	}

	/* FIXME - FIXME - FIXME */

	/* This is a stub that will need to be filled in when we have
	 * implemented real image file headers.
	 */

	*exposure_time = 1.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_image_get_image_data_pointer( MX_IMAGE_FRAME *frame,
				size_t *image_length,
				void **image_data_pointer )
{
	static const char fname[] = "mx_image_get_image_data_pointer()";

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}
	if ( image_length == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_length pointer passed was NULL." );
	}
	if ( image_data_pointer == (void **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_data_pointer argument passed was NULL." );
	}

	if ( frame->image_type != MXT_IMAGE_LOCAL_1D_ARRAY ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Image frame %p is not an image of type "
		"MXT_IMAGE_LOCAL_1D_ARRAY (%d).  Instead, it is of type %ld, "
		"which means that the actual image data is not stored in "
		"this data structure.", frame, MXT_IMAGE_LOCAL_1D_ARRAY,
			frame->image_type );
	}

	if ( frame->image_data == NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No image data has been read into image frame %p.", frame );
	}

	*image_length       = frame->image_length;
	*image_data_pointer = frame->image_data;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_image_copy_1d_pixel_array( MX_IMAGE_FRAME *frame,
				void *destination_pixel_array,
				size_t max_array_bytes,
				size_t *num_bytes_copied )
{
	static const char fname[] = "mx_image_copy_1d_pixel_array()";

	void *image_data_pointer;
	size_t image_length, bytes_to_copy;
	mx_status_type mx_status;

	mx_status = mx_image_get_image_data_pointer( frame,
					&image_length, &image_data_pointer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( destination_pixel_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The destination_pixel_array pointer passed was NULL." );
	}

	if ( max_array_bytes >= image_length ) {
		bytes_to_copy = image_length;
	} else {
		bytes_to_copy = max_array_bytes;
	}

	memcpy( destination_pixel_array, image_data_pointer, bytes_to_copy );

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = bytes_to_copy;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_image_copy_frame( MX_IMAGE_FRAME **new_frame_ptr,
			MX_IMAGE_FRAME *old_frame )
{
	static const char fname[] = "mx_image_copy_frame()";

	mx_status_type mx_status;

	if ( new_frame_ptr == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The new frame pointer passed was NULL." );
	}
	if ( old_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The old frame pointer passed was NULL." );
	}

	mx_status = mx_image_alloc( new_frame_ptr,
				old_frame->image_type,
				old_frame->framesize,
				old_frame->image_format,
				old_frame->pixel_order,
				old_frame->bytes_per_pixel,
				old_frame->header_length,
				old_frame->image_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( old_frame->header_length != 0 ) {
		memcpy( (*new_frame_ptr)->header_data, old_frame->header_data,
				old_frame->header_length );
	}

	if ( old_frame->image_length != 0 ) {
		memcpy( (*new_frame_ptr)->image_data, old_frame->image_data,
				old_frame->image_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_image_read_file( MX_IMAGE_FRAME **frame_ptr,
			unsigned long datafile_type,
			char *datafile_name )
{
	static const char fname[] = "mx_image_read_file()";

	mx_status_type mx_status;

	switch( datafile_type ) {
	case MXT_IMAGE_FILE_PNM:
		mx_status = mx_image_read_pnm_file( frame_ptr, datafile_name );
		break;
	default:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image file type %lu requested for datafile '%s'.",
			datafile_type, datafile_name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_image_write_file( MX_IMAGE_FRAME *frame,
			unsigned long datafile_type,
			char *datafile_name )
{
	static const char fname[] = "mx_image_write_file()";

	mx_status_type mx_status;

	switch( datafile_type ) {
	case MXT_IMAGE_FILE_PNM:
		mx_status = mx_image_write_pnm_file( frame, datafile_name );
		break;
	default:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image file type %lu requested for datafile '%s'.",
			datafile_type, datafile_name );
	}

	return mx_status;
}

/*----*/

/*
 * WARNING: mx_image_read_pnm_file() currently does not support arbitrary
 * PNM files.  It only supports PNM files written by mx_image_write_pnm_file().
 * This means that the file _must_ have the following format:
 *
 * Line 1: Contains either the string 'P5' or the string 'P6'.
 * Line 2: Starts with a comment character '#' and is followed
 *         by the original filename of the PNM file.
 * Line 3: Contains the width followed by the height in ASCII.
 * Line 4: Contains the maximum integer pixel value.  For greyscale
 *         'P5' files, this can be either 255 or 65535.  For color
 *         'P6' files, this must be 255.
 * After line 4, the rest of the file is binary image data.  For 16-bit 
 * greyscale images, the data are stored in big-endian order.
 *
 * Example 1: 24-bit RGB color
 *
 *    P6
 *    # test1.pnm
 *    1024 768
 *    255
 *    <binary image data>
 *
 * Example 2: 8-bit greyscale
 *
 *    P5
 *    # test1.pnm
 *    1024 768
 *    255
 *    <binary image data>
 *
 * Example 3: 16-bit greyscale
 *
 *    P5
 *    # test1.pnm
 *    1024 768
 *    65535
 *    <binary image data>
 *
 */

MX_EXPORT mx_status_type
mx_image_read_pnm_file( MX_IMAGE_FRAME **frame, char *datafile_name )
{
	static const char fname[] = "mx_image_read_pnm_file()";

	FILE *file;
	char buffer[100];
	char *ptr;
	int saved_errno, num_items, pnm_type;
	long framesize[2];
	long maxint, bytes_per_pixel, bytes_per_frame, bytes_read, image_format;
	mx_status_type mx_status;

	if ( frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

	if ( datafile_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The datafile_name pointer passed was NULL." );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s invoked for datafile '%s'.",
		fname, datafile_name ));
#endif

	/* Figure out the size and format of the file from the PNM header. */

	file = fopen( datafile_name, "rb" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_NOT_FOUND, fname,
		"Cannot open PNM image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	/* The first line tells us what type of PNM file this is.
	 * At present, we only support greyscale (P5, 8-bit or 16-bit)
	 * and color (P6, RGB 24-bit).
	 */

	ptr = fgets( buffer, sizeof(buffer), file );

	if ( ptr == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read the first line of PNM image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	num_items = sscanf( buffer, "P%d", &pnm_type );

	if ( num_items != 1 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"File '%s' does not seem to be a PNM file, since the first "
		"two bytes of the file are not the letter 'P' followed by "
		"an integer.  Instead, the first line looks like this -> '%s'",
			datafile_name, buffer );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: PNM type = %d", fname, pnm_type));
#endif

	if ( (pnm_type != 5) && (pnm_type != 6) ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"PNM file format P%d used by image file '%s' is not supported."
		"  The only PNM filetypes supported are the raw formats, "
		"'P5' and 'P6'.", pnm_type, datafile_name );
	}

	/* The second line should be a comment and should be skipped. */

	ptr = fgets( buffer, sizeof(buffer), file );

	if ( ptr == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read the second line of PNM image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: comment line = '%s'", fname, buffer));
#endif

	/* The third line should contain the width and height. */

	ptr = fgets( buffer, sizeof(buffer), file );

	if ( ptr == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read the third line of PNM image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	num_items = sscanf( buffer, "%ld %ld", &framesize[0], &framesize[1] );

	if ( num_items != 2 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Did not find the width and height of PNM file '%s'.  "
		"Instead, we saw this -> '%s'",
			datafile_name, buffer );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: framesize = (%ld,%ld)",
		fname, framesize[0], framesize[1]));
#endif

	/* The fourth line should contain the maximum integer value. */

	ptr = fgets( buffer, sizeof(buffer), file );

	if ( ptr == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read the third line of PNM image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	num_items = sscanf( buffer, "%ld", &maxint );

	if ( num_items != 1 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Did not find the maximum integer value of PNM file '%s'.  "
		"Instead, we saw this -> '%s'",
			datafile_name, buffer );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: maxint = %ld", fname, maxint));
#endif

	bytes_per_pixel = 0;
	image_format = 0;

	switch( pnm_type ) {
	case 5:
		switch( maxint ) {
		case 255:
			image_format = MXT_IMAGE_FORMAT_GREY8;
			bytes_per_pixel = 1;
			break;

		case 65535:
			image_format = MXT_IMAGE_FORMAT_GREY16;
			bytes_per_pixel = 2;
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Greyscale PNM file '%s' reports that its maximum "
			"integer value is %lu.  The only supported values "
			"are 255 and 65535.", datafile_name, maxint );
		}
		break;
	case 6:
		switch( maxint ) {
		case 255:
			image_format = MXT_IMAGE_FORMAT_RGB;
			bytes_per_pixel = 3;
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Color PNM file '%s' reports that its maximum "
			"integer value is %lu.  The only supported value "
			"is 255.", datafile_name, maxint );
		}
	}

	bytes_per_frame = bytes_per_pixel * framesize[0] * framesize[1];

	/* Change the size of the MX_IMAGE_FRAME to match the PNM file. */

	mx_status = mx_image_alloc( frame,
					MXT_IMAGE_LOCAL_1D_ARRAY,
					framesize,
					image_format,
					MXT_IMAGE_PIXEL_ORDER_STANDARD,
					(double) bytes_per_pixel,
					0,
					bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read in the binary part of the image file. */

	bytes_read = fread( (*frame)->image_data, sizeof(unsigned char),
				bytes_per_frame, file );

	if ( bytes_read < bytes_per_frame ) {
		if ( feof(file) ) {
			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"End of file at pixel %ld for PNM image file '%s'.",
				bytes_read, datafile_name );
		}
		if ( ferror(file) ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while reading pixel %ld "
			"for PNM image file '%s'.",
				bytes_read, datafile_name );
		}
		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Only %ld image bytes were read from "
			"PNM image file '%s' when %ld bytes were expected.",
				bytes_read, datafile_name, bytes_per_frame );
	}

	/* Close the PNM image file. */

	fclose( file );

	/* 16-bit PNM files are stored in big-endian order.  Thus, if we are
	 * are reading a 16-bit greyscale image on a little-endian computer,
	 * we must byteswap the image data.
	 */

	if ( ( image_format == MXT_IMAGE_FORMAT_GREY16 )
	  && ( mx_native_byteorder() == MX_DATAFMT_LITTLE_ENDIAN ) )
	{
		uint16_t *uint16_array;
		long i, words_per_frame;

		/* Byteswap the 16-bit integers. */

		uint16_array = (*frame)->image_data;

		words_per_frame = framesize[0] * framesize[1];

		for ( i = 0; i < words_per_frame; i++ ) {
			uint16_array[i] = mx_16bit_byteswap( uint16_array[i] );
		}
	}

	/* We are done, so return. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_image_write_pnm_file( MX_IMAGE_FRAME *frame, char *datafile_name )
{
	static const char fname[] = "mx_image_write_pnm_file()";

	FILE *file;
	pixel_converter_t converter;
	void (*converter_fn)( unsigned char *, unsigned char * );
	int src_step, dest_step;
	unsigned char *src;
	unsigned char dest[10];
	unsigned char byte0;
	int pnm_type, saved_errno;
	unsigned long byteorder;
	long i;
	unsigned int maxint;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

	if ( datafile_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The datafile_name pointer passed was NULL." );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s invoked for datafile '%s'.",
		fname, datafile_name ));

	MX_DEBUG(-2,("%s: image_type = %ld, width = %ld, height = %ld",
		fname, frame->image_type,
		frame->framesize[0], frame->framesize[1] ));
	MX_DEBUG(-2,("%s: image_format = %ld, pixel_order = %ld",
		fname, frame->image_format, frame->pixel_order));
	MX_DEBUG(-2,("%s: image_length = %lu, image_data = %p",
		fname, (unsigned long) frame->image_length, frame->image_data));
#endif

	byteorder = mx_native_byteorder();

	switch( frame->image_format ) {
	case MXT_IMAGE_FORMAT_RGB565:
		converter = mxp_rgb565_converter;
		pnm_type = 6;
		maxint = 255;
		break;
	case MXT_IMAGE_FORMAT_YUYV:
		converter = mxp_yuyv_converter;
		pnm_type = 6;
		maxint = 255;
		break;

	case MXT_IMAGE_FORMAT_RGB:
		converter = mxp_rgb_converter;
		pnm_type = 6;
		maxint = 255;
		break;
	case MXT_IMAGE_FORMAT_GREY8:
		converter = mxp_grey8_converter;
		pnm_type = 5;
		maxint = 255;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		converter = mxp_grey16_converter;
		pnm_type = 5;
		maxint = 65535;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image format %ld requested for datafile '%s'.",
			frame->image_format, datafile_name );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: pnm_type = %d, maxint = %d",
		fname, pnm_type, maxint));
#endif

	src_step     = converter.num_source_bytes;
	dest_step    = converter.num_destination_bytes;
	converter_fn = converter.converter_fn;

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: src_step = %d, dest_step = %d, converter_fn = %p",
		fname, src_step, dest_step, converter_fn));
#endif

	if ( dest_step > sizeof(dest) ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"You must increase the size of the dest array "
			"to %d and then recompile MX.",
				dest_step );
	}

	file = fopen( datafile_name, "wb" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open PNM image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	/* Write the PPM header. */

	fprintf( file, "P%d\n", pnm_type );
	fprintf( file, "# %s\n", datafile_name );
	fprintf( file, "%lu %lu\n", frame->framesize[0], frame->framesize[1] );
	fprintf( file, "%u\n", maxint );

	/* Loop through the image file. */

	src = frame->image_data;

	for ( i = 0; i < frame->image_length; i += src_step ) {

#if 0
		if ( i >= 50 ) {
			MX_DEBUG(-2,("%s: aborting early", fname));
			break;
		}
#endif

		converter_fn( &src[i], dest );

		switch( frame->image_format ) {

		case MXT_IMAGE_FORMAT_RGB565:
		case MXT_IMAGE_FORMAT_RGB:
		case MXT_IMAGE_FORMAT_YUYV:
		case MXT_IMAGE_FORMAT_GREY8:
			/* 8-bit formats can be written immediately. */

			fwrite( dest, sizeof(unsigned char), dest_step, file );
			break;

		case MXT_IMAGE_FORMAT_GREY16:
			/* If we are on a big-endian machine, we can directly
			 * write the bytes.  On a little-endian machine, we
			 * byteswap to big-endian byte order before writing
			 * out the bytes.
			 */

			if ( byteorder == MX_DATAFMT_LITTLE_ENDIAN ) {
				/* Swap the bytes. */

				byte0   = dest[0];
				dest[0] = dest[1];
				dest[1] = byte0;
			}

			fwrite( dest, sizeof(unsigned char), dest_step, file );
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Unsupported image format %ld requested "
				"for datafile '%s'.",
				frame->image_format, datafile_name );
		}
	}

	fclose( file );

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,
	("%s: PNM file '%s' successfully written.", fname, datafile_name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

