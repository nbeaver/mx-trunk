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
 * Copyright 2006-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_IMAGE_DEBUG			FALSE

#define MX_IMAGE_DEBUG_REBIN		FALSE

#define MX_IMAGE_DEBUG_MARCCD		FALSE

#define MX_IMAGE_DEBUG_TIMESTAMP	FALSE

#define MX_IMAGE_TEST_DEZINGER		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__GNUC__)
#  define __USE_XOPEN		/* For strptime() */
#endif

#include "mx_util.h"
#include "mx_time.h"
#include "mx_hrt.h"
#include "mx_stdint.h"
#include "mx_bit.h"
#include "mx_array.h"
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

static unsigned char
clamp( double x )
{
	long r_long;
	unsigned r_uchar;

	r_long = mx_round(x);

	if (r_long < 0) {
		r_long = 0;
	} else if (r_long > 255) {
		r_long = 255;
	}

	r_uchar = (unsigned char) r_long;

	return r_uchar;
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

	{"INT32",   MXT_IMAGE_FORMAT_INT32},
	{"DOUBLE",  MXT_IMAGE_FORMAT_DOUBLE},
};

static size_t mxp_image_format_table_length
	= sizeof(mxp_image_format_table) / sizeof(mxp_image_format_table[0]);

MX_EXPORT mx_status_type
mx_image_get_image_format_type_from_name( char *name, long *type )
{
	static const char fname[] =
		"mx_image_get_image_format_type_from_name()";

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
mx_image_get_image_format_name_from_type( long type,
				char *name, size_t max_name_length )
{
	static const char fname[] =
		"mx_image_get_image_format_name_from_type()";

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

/*--------*/

static MX_IMAGE_FORMAT_ENTRY mxp_file_format_table[] =
{
	{"PNM",    MXT_IMAGE_FILE_PNM},

	{"SMV",    MXT_IMAGE_FILE_SMV},
	{"MARCCD", MXT_IMAGE_FILE_MARCCD},
	{"EDF",    MXT_IMAGE_FILE_EDF}
};

static size_t mxp_file_format_table_length
	= sizeof(mxp_file_format_table) / sizeof(mxp_file_format_table[0]);

MX_EXPORT mx_status_type
mx_image_get_file_format_type_from_name( char *name, long *type )
{
	static const char fname[] =
		"mx_image_get_file_format_type_from_name()";

	MX_IMAGE_FORMAT_ENTRY *entry;
	long i;

	if ( name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The file format name pointer passed was NULL." );
	}
	if ( type == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The file format type pointer passed was NULL." );
	}

	if ( strlen(name) == 0 ) {
		*type = MXT_IMAGE_FILE_PNM;

		return MX_SUCCESSFUL_RESULT;
	}

	for ( i = 0; i < mxp_file_format_table_length; i++ ) {
		entry = &mxp_file_format_table[i];

		if ( mx_strcasecmp( entry->name, name ) == 0 ) {
			*type = entry->type;

			return MX_SUCCESSFUL_RESULT;
		}
	}

	return mx_error( MXE_UNSUPPORTED, fname,
	"File format type '%s' is not currently supported by MX.", name );
}

MX_EXPORT mx_status_type
mx_image_get_file_format_name_from_type( long type,
				char *name, size_t max_name_length )
{
	static const char fname[] =
		"mx_image_get_file_format_name_from_type()";

	MX_IMAGE_FORMAT_ENTRY *entry;
	long i;

	if ( name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The file format name pointer passed was NULL." );
	}

	for ( i = 0; i < mxp_file_format_table_length; i++ ) {
		entry = &mxp_file_format_table[i];

		if ( entry->type == type ) {
			strlcpy( name, entry->name, max_name_length );

			return MX_SUCCESSFUL_RESULT;
		}
	}

	return mx_error( MXE_UNSUPPORTED, fname,
	"File format type %ld is not currently supported by MX.", type );
}

MX_EXPORT mx_status_type
mx_image_format_get_bytes_per_pixel( long image_format,
				double *bytes_per_pixel )
{
	static const char fname[] = "mx_image_format_get_bytes_per_pixel()";

	if ( bytes_per_pixel == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The bytes_per_pixel pointer passed was NULL." );
	}

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_RGB:
		*bytes_per_pixel = 3.0;
		break;
	case MXT_IMAGE_FORMAT_GREY8:
		*bytes_per_pixel = 1.0;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		*bytes_per_pixel = 2.0;
		break;
	case MXT_IMAGE_FORMAT_GREY32:
		*bytes_per_pixel = 4.0;
		break;

	case MXT_IMAGE_FORMAT_RGB565:
		*bytes_per_pixel = 1.5;
		break;
	case MXT_IMAGE_FORMAT_YUYV:
		*bytes_per_pixel = 2.0;
		break;

	case MXT_IMAGE_FORMAT_INT32:
		*bytes_per_pixel = 4.0;
		break;

	case MXT_IMAGE_FORMAT_DOUBLE:
		*bytes_per_pixel = 8.0;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image format %ld was requested.",
			image_format );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_alloc( MX_IMAGE_FRAME **frame,
			long row_framesize,
			long column_framesize,
			long image_format,
			long byte_order,
			double bytes_per_pixel,
			size_t header_length,
			size_t image_length )
{
	static const char fname[] = "mx_image_alloc()";

	char *ptr;
	unsigned long bytes_per_frame, additional_length;
	double bytes_per_frame_as_double;
	time_t timestamp;

	if ( frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		    "The MX_IMAGE_FRAME pointer to pointer passed was NULL.");
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s invoked for *frame = %p", fname, *frame));

	MX_DEBUG(-2,
    ("%s: Invoked as mx_image_alloc( %p, %ld, %ld, %ld, %ld, %g, %ld, %ld )",
		fname, frame, row_framesize, column_framesize,
		image_format, byte_order, bytes_per_pixel,
		(long) header_length, (long) image_length ));
#endif

	/* We either reuse an existing MX_IMAGE_FRAME or create a new one. */

	if ( (*frame) == (MX_IMAGE_FRAME *) NULL ) {

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: Allocating a new MX_IMAGE_FRAME.", fname));
#endif
		/* Allocate a new MX_IMAGE_FRAME. */

		*frame = malloc( sizeof(MX_IMAGE_FRAME) );

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: *frame = malloc(sizeof(MX_IMAGE_FRAME)) = %p",
			fname, *frame));
#endif

		if ( (*frame) == (MX_IMAGE_FRAME *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"a new MX_IMAGE_FRAME structure." );
		}

		memset( *frame, 0, sizeof(MX_IMAGE_FRAME) );
	}

	/* Make sure the requested header length is long enough to hold
	 * the standard header fields.
	 */

	if ( header_length < MXT_IMAGE_HEADER_LENGTH_IN_BYTES ) {
		header_length = MXT_IMAGE_HEADER_LENGTH_IN_BYTES;
	}

	/*** See if the header buffer is already big enough for the header. ***/

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: (*frame)->header_data = %p",
		fname, (*frame)->header_data));
	MX_DEBUG(-2,
	("%s: (*frame)->header_length = %lu, header_length = %lu",
		fname, (unsigned long) (*frame)->header_length,
		(unsigned long) header_length));
#endif

	if ( (*frame)->header_data == NULL ) {
#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: Allocating new %lu byte header.",
			fname, (unsigned long) header_length ));
#endif
		(*frame)->header_data = malloc( header_length );

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,
		("%s: (*frame) = %p, (*frame)->header_data = malloc(%lu) = %p",
			fname, *frame, (unsigned long) header_length,
			(*frame)->header_data));
#endif

		if ( (*frame)->header_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a "
			"%lu byte header buffer.",
				(unsigned long) header_length );
		}

		/* Initialize the header data to 0. */

		memset( (*frame)->header_data, 0, header_length );

		/* Save the length. */

		(*frame)->header_length = header_length;
		(*frame)->allocated_header_length = header_length;
	} else
	if ( (*frame)->allocated_header_length >= header_length ) {
#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,
		("%s: The header buffer is already big enough.",fname));
#endif
		(*frame)->header_length = header_length;
	} else {
#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,
		("%s: Changing header buffer to %lu bytes.",
				fname, (unsigned long) header_length));

		MX_DEBUG(-2,("%s: *frame = %p", fname, *frame));
		MX_DEBUG(-2,("%s: BEFORE realloc(), (*frame)->header_data = %p",
			fname, (*frame)->header_data));
#endif

		(*frame)->header_data = realloc( (*frame)->header_data,
							header_length );
#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,
	("%s: AFTER realloc(), (*frame)->header_data = realloc(x,%lu) = %p",
			fname, (unsigned long) header_length,
			(*frame)->header_data));
#endif

		if ( (*frame)->header_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a "
			"%lu byte header buffer for frame %p.",
				(unsigned long) header_length, *frame );
		}

		/* Initialize the additional part of the header to 0. */

		ptr = (char *)((*frame)->header_data) + (*frame)->header_length;

		additional_length = header_length - (*frame)->header_length;

		memset( ptr, 0, additional_length );

		/* Save the length. */

		(*frame)->header_length = header_length;
		(*frame)->allocated_header_length = header_length;
	}

	/*** See if the image buffer is already big enough for the image. ***/

	bytes_per_frame_as_double = bytes_per_pixel
		* ((double) row_framesize) * ((double) column_framesize);

	bytes_per_frame = mx_round( bytes_per_frame_as_double );

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: ********** image_data buffer *********", fname));
	MX_DEBUG(-2,
	("%s: (*frame)->image_data = %p, (*frame)->image_length = %lu",
		fname, (*frame)->image_data,
		(unsigned long)(*frame)->image_length));
	MX_DEBUG(-2,
	("%s: (*frame)->allocated_image_length = %lu, bytes_per_frame = %lu",
		fname, (unsigned long) (*frame)->allocated_image_length,
		bytes_per_frame));
#endif

	/* Setup the image data buffer. */

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: *frame = %p", fname, *frame));
#endif

	if ( ((*frame)->image_length == 0) && (bytes_per_frame == 0)) {

		/* Zero length image buffers are not allowed. */

		(*frame)->allocated_image_length = 0;
		(*frame)->image_data = NULL;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Attempted to create a zero length image buffer for frame %p.",
			*frame );
	} else
	if ( (*frame)->image_data == NULL ) {
		(*frame)->image_data = malloc( bytes_per_frame );

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: (*frame)->image_data = malloc( %lu ) = %p",
			fname, bytes_per_frame, (*frame)->image_data));
#endif

		if ( (*frame)->image_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld byte "
			"image buffer for frame %p",
				bytes_per_frame, *frame );
		}

		(*frame)->image_length = bytes_per_frame;
		(*frame)->allocated_image_length = bytes_per_frame;
	} else
	if ( (*frame)->allocated_image_length >= bytes_per_frame ) {

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,
		("%s: The image buffer is already big enough.", fname));
#endif
		(*frame)->image_length = bytes_per_frame;
	} else {

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: Resizing the image buffer %p to %lu bytes.",
			fname, (*frame)->image_data, bytes_per_frame));
#endif

		(*frame)->image_data = realloc( (*frame)->image_data,
							bytes_per_frame );

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,
		("%s: (*frame)->image_data = realloc(x, %lu) = %p",
			fname, bytes_per_frame, (*frame)->image_data));
#endif

		if ( (*frame)->image_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld byte "
			"image buffer for frame %p",
				bytes_per_frame, *frame );
		}

		(*frame)->image_length = bytes_per_frame;
		(*frame)->allocated_image_length = bytes_per_frame;
	}

	/* Fill in some parameters. */

	MXIF_HEADER_BYTES(*frame)     = (*frame)->header_length;
	MXIF_ROW_FRAMESIZE(*frame)    = row_framesize;
	MXIF_COLUMN_FRAMESIZE(*frame) = column_framesize;
	MXIF_IMAGE_FORMAT(*frame)     = image_format;
	MXIF_BYTE_ORDER(*frame)       = byte_order;

	MXIF_SET_BYTES_PER_PIXEL(*frame, bytes_per_pixel);

	/* Initialize some parameters if they are currently set to 0. */

	if ( MXIF_ROW_BINSIZE(*frame) == 0 ) {
		MXIF_ROW_BINSIZE(*frame) = 1;
	}
	if ( MXIF_COLUMN_BINSIZE(*frame) == 0 ) {
		MXIF_COLUMN_BINSIZE(*frame) = 1;
	}
	if ( MXIF_BITS_PER_PIXEL(*frame) == 0 ) {
		MXIF_BITS_PER_PIXEL(*frame) = mx_round( 8.0 * bytes_per_pixel );
	}
	if ( ( MXIF_EXPOSURE_TIME_SEC(*frame) == 0 )
	  && ( MXIF_EXPOSURE_TIME_NSEC(*frame) == 0 ) )
	{
		MXIF_EXPOSURE_TIME_SEC(*frame) = 1;
		MXIF_EXPOSURE_TIME_NSEC(*frame) = 0;
	}
	if ( ( MXIF_TIMESTAMP_SEC(*frame) == 0 )
	  && ( MXIF_TIMESTAMP_NSEC(*frame) == 0 ) )
	{
		time( &timestamp ) ;
		MXIF_TIMESTAMP_SEC(*frame) = timestamp;
		MXIF_TIMESTAMP_NSEC(*frame) = 0;
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,
("%s: (*frame) = %p, (*frame)->header_data = %p, (*frame)->image_data = %p",
		fname, (*frame), (*frame)->header_data, (*frame)->image_data ));

	MX_DEBUG(-2,("%s: header_length = %ld, allocated_header_length = %ld",
		fname, (long) (*frame)->header_length,
		(long) (*frame)->allocated_header_length));

	MX_DEBUG(-2,("%s: image_length = %ld, allocated_image_length = %ld",
		fname, (long) (*frame)->image_length,
		(long) (*frame)->allocated_image_length));
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

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_alloc_sector_array( MX_IMAGE_FRAME *image_frame,
				long num_sector_rows,
				long num_sector_columns,
				void ****sector_array_ptr )
{
	static const char fname[] = "mx_image_alloc_sector_array()";

	char *image_data;
	long image_width, image_height, sector_width, sector_height, pixel_size;
	double dbl_pixel_size;
	long n, sector_row, row, sector_column, num_sectors;
	long sizeof_row_of_sectors, sizeof_full_row, sizeof_sector_row;
	long row_byte_offset, row_ptr_offset;
	long byte_offset;
	void ***sector_array;
	void **sector_array_row_ptr;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}
	if ( sector_array_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The sector array pointer passed was NULL." );
	}

	image_data = image_frame->image_data;

	image_width  = MXIF_ROW_FRAMESIZE(image_frame);
	image_height = MXIF_COLUMN_FRAMESIZE(image_frame);

	sector_width  = image_width  / num_sector_columns;
	sector_height = image_height / num_sector_rows;

	dbl_pixel_size = MXIF_BYTES_PER_PIXEL(image_frame);

	pixel_size = mx_round( dbl_pixel_size );

	if ( mx_difference( pixel_size, dbl_pixel_size ) > 0.01 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The pixel size %f in bytes of the specified image frame %p "
		"is not an integer.", dbl_pixel_size, image_frame );
	}

	/*---*/

	num_sectors = num_sector_rows * num_sector_columns;

	*sector_array_ptr = NULL;

	sector_array = malloc( num_sectors * sizeof(void **) );

	if ( sector_array == (void ***) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"sector array pointer.", num_sectors );
	}

	memset( sector_array, 0, num_sectors * sizeof(void **) );

	/*---*/

	sector_array_row_ptr =
		malloc( num_sectors * sector_height * sizeof(void *) );

	if ( sector_array_row_ptr == (void **) NULL ) {
		free( sector_array );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of row pointers.", num_sectors * sector_height );
	}

	/*---*/

	row_byte_offset =
		(long) ( sector_height * sizeof(sector_array_row_ptr) );

	row_ptr_offset = (long) ( row_byte_offset / sizeof(void *) );

	for ( n = 0; n < num_sectors; n++ ) {
		sector_array[n] = sector_array_row_ptr + n * row_ptr_offset;
	}

	/* The 'sizeof_full_row' is the number of bytes in a single horizontal
	 * line of the image.  The 'sizeof_row_of_sectors' is the number of
	 * bytes in a horizontal row of sectors.  In unbinned mode,
	 * sizeof_row_of_sectors = sector_height * sizeof_full_row.
	 */

	/* sizeof_sector_row     = number of bytes in a single horizontal
	 *                         row of a sector.
	 *
	 * sizeof_full_row       = number of bytes in a single horizontal
	 *                         row of the full image.
	 *
	 * sizeof_row_of_sectors = number of bytes in all the rows of a
	 *                         single row of sectors.
	 *
	 * For an unbinned image:
	 *
	 *   sizeof_row_of_sectors = sector_height * sizeof_full_row
	 *        = sector_height * num_sector_columns * sizeof_sector_row
	 */

	sizeof_sector_row = (long) ( sector_width * pixel_size );

#if 0
	sizeof_full_row = num_sector_columns * sizeof_sector_row;
#else
	/* Note: This should work even if the original image width
	 * is not a power of two.
	 */

	sizeof_full_row = image_width * pixel_size;
#endif

	sizeof_row_of_sectors = sizeof_full_row * sector_height;

	/*---*/

	for ( sector_row = 0; sector_row < num_sector_rows; sector_row++ ) {
	    for ( row = 0; row < sector_height; row++ ) {
		for ( sector_column = 0; sector_column < num_sector_columns;
							sector_column++ )
		{
		    n = sector_column + num_sector_columns * sector_row;

		    byte_offset = sector_row * sizeof_row_of_sectors
				+ row * sizeof_full_row
				+ sector_column * sizeof_sector_row;

		    /* image_data is a 'char' array here, so we use
		     * the byte offset directly.
		     */

		    sector_array[n][row] = ( image_data + byte_offset );
		}
	    }
	}

	*sector_array_ptr = sector_array;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_image_free_sector_array( void ***sector_array )
{
	void **sector_array_row_ptr;

	if ( sector_array == NULL )
		return;

	sector_array_row_ptr =
		mx_read_void_pointer_from_memory_location( sector_array );

	if ( sector_array_row_ptr != NULL ) {
		free( sector_array_row_ptr );
	}

	free( sector_array );

	return;
}

/*--------------------------------------------------------------------------*/

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

	if ( (MXIF_EXPOSURE_TIME_SEC(frame) == 0)
	  && (MXIF_EXPOSURE_TIME_NSEC(frame) == 0) )
	{
		mx_warning(
"%s: The header for image frame %p does not contain the exposure time.  "
"The exposure time will be assumed to be 1 second.", fname, frame );

		*exposure_time = 1.0;
	} else {
		*exposure_time = ((double) MXIF_EXPOSURE_TIME_SEC(frame))
			+ 1.0e-9 * ((double) MXIF_EXPOSURE_TIME_NSEC(frame));
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: exposure_time = %g", fname, *exposure_time));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_image_get_average_intensity( MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				double *average_intensity )
{
	static const char fname[] = "mx_image_get_average_intensity()";

	uint16_t *image_data, *mask_data;
	unsigned long i, num_pixels, num_unmasked_pixels;
	double intensity_sum, diff;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}
	if ( average_intensity == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The average_intensity pointer passed was NULL." );
	}

	if ( mask_frame != (MX_IMAGE_FRAME *) NULL ) {
		/* If a mask frame was passed to us, verify that the
		 * mask frame has the same image parameters as the
		 * image frame.
		 */

		if ( ( MXIF_ROW_FRAMESIZE(mask_frame)
				!= MXIF_ROW_FRAMESIZE(image_frame) )
		  || ( MXIF_COLUMN_FRAMESIZE(mask_frame)
				!= MXIF_COLUMN_FRAMESIZE(image_frame) ) )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The mask frame has different dimensions (%ld,%ld) "
			"than the image frame (%ld,%ld).",
				(long) MXIF_ROW_FRAMESIZE(mask_frame),
				(long) MXIF_COLUMN_FRAMESIZE(mask_frame),
				(long) MXIF_ROW_FRAMESIZE(image_frame),
				(long) MXIF_COLUMN_FRAMESIZE(image_frame) );
		}
		if ( MXIF_IMAGE_FORMAT(mask_frame)
			!= MXIF_IMAGE_FORMAT(image_frame) )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The mask frame has a different image format (%ld) "
			"than the image frame (%ld).",
				(long) MXIF_IMAGE_FORMAT(mask_frame),
				(long) MXIF_IMAGE_FORMAT(image_frame) );	
		}
		if ( MXIF_BYTE_ORDER(mask_frame)
			!= MXIF_BYTE_ORDER(image_frame) )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The mask frame has a different byte order (%ld) "
			"than the image frame (%ld).",
				(long) MXIF_BYTE_ORDER(mask_frame),
				(long) MXIF_BYTE_ORDER(image_frame) );
		}

		diff = mx_difference( MXIF_BYTES_PER_MILLION_PIXELS(mask_frame),
				MXIF_BYTES_PER_MILLION_PIXELS(image_frame) );

		if ( diff > 10.0 ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The mask frame has a different number "
			"of bytes per pixel (%g) "
			"than the image frame (%g).",
				MXIF_BYTES_PER_PIXEL(mask_frame),
				MXIF_BYTES_PER_PIXEL(image_frame) );	
		}
	}

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	intensity_sum = 0.0;

	num_unmasked_pixels = 0L;

	image_data = image_frame->image_data;

	if ( image_data == (uint16_t *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The image_data pointer for the specified image frame is NULL.");
	}

	if ( mask_frame == (MX_IMAGE_FRAME *) NULL ) {

		for ( i = 0; i < num_pixels; i++ ) {
			intensity_sum += (double) image_data[i];
		}

		num_unmasked_pixels = num_pixels;
	} else {
		mask_data = mask_frame->image_data;

		if ( mask_data == (uint16_t *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The image_data pointer for the specified "
			"mask frame is NULL." );
		}

		for ( i = 0; i < num_pixels; i++ ) {
			if ( mask_data[i] != 0 ) {
				intensity_sum += (double) image_data[i];

				num_unmasked_pixels++;
			}
		}
	}

	if ( num_unmasked_pixels == 0 ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The specified mask frame has NO unmasked pixels!" );
	}

	*average_intensity = intensity_sum / (double) num_unmasked_pixels;

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: average_intensity = %g, num_unmasked_pixels = %lu",
		fname, *average_intensity, num_unmasked_pixels));
#endif

	return MX_SUCCESSFUL_RESULT;
}

#define MX_IMAGE_STATISTICS_MAX_SD	10

#define MX_IMAGE_STATISTICS_BINS	(2*MX_IMAGE_STATISTICS_MAX_SD + 1)

MX_EXPORT mx_status_type
mx_image_statistics( MX_IMAGE_FRAME *frame )
{
	static const char fname[] = "mx_image_statistics()";

	uint8_t *uint8_array;
	uint16_t *uint16_array;
	unsigned long i, num_pixels, image_format;
	unsigned long row_framesize, column_framesize;
	double sum, sum_of_squares, mean, standard_deviation;
	double pixel, diff, pixel_sd;
	double min_pixel, max_pixel;
	double first_pixel;
	long sd_bin;
	double sd_value;
	mx_bool_type pixels_are_all_equal;
	long pixel_bin;
	unsigned long sd_histogram[MX_IMAGE_STATISTICS_BINS];

	first_pixel = 0;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

	uint8_array = NULL;
	uint16_array = NULL;
	pixel = 0.0;

	row_framesize = MXIF_ROW_FRAMESIZE(frame);
	column_framesize = MXIF_COLUMN_FRAMESIZE(frame);
	image_format = MXIF_IMAGE_FORMAT(frame);

	num_pixels = row_framesize * column_framesize;

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		uint8_array = frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		uint16_array = frame->image_data;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"The image format %lu for the MX_IMAGE_FRAME passed "
		"is not supported by this routine.", image_format );
	}

	/* First compute the mean. */

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		first_pixel = uint8_array[0];
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		first_pixel = uint16_array[0];
		break;
	}

	sum = 0.0;
	pixels_are_all_equal = TRUE;

	min_pixel = 1.0e38;
	max_pixel = -1.0e38;

	for ( i = 0; i < num_pixels; i++ ) {
		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			pixel = uint8_array[i];
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			pixel = uint16_array[i];
			break;
		}

		if ( pixels_are_all_equal ) {
			if ( fabs(pixel - first_pixel) > 0.1 ) {
				pixels_are_all_equal = FALSE;
			}
		}

		if ( pixel < min_pixel )
			min_pixel = pixel;

		if ( pixel > max_pixel )
			max_pixel = pixel;

		sum += pixel;
	}

	if ( pixels_are_all_equal ) {
		mx_warning(
		"All of the pixels in the image have the same value %g",
			first_pixel );
		return MX_SUCCESSFUL_RESULT;
	}

	mean = sum / (double) num_pixels;

	/* Next compute the standard deviation. */

	sum_of_squares = 0.0;

	for ( i = 0; i < num_pixels; i++ ) {
		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			pixel = uint8_array[i];
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			pixel = uint16_array[i];
			break;
		}

		diff = pixel - mean;

		sum_of_squares += (diff * diff);
	}

	standard_deviation = sqrt( sum_of_squares
		/ ( ((double) num_pixels) - 1.0 ) );

	/* Finish by generating a simple histogram of pixel values. */

	for ( i = 0; i < MX_IMAGE_STATISTICS_BINS; i++ ) {
		sd_histogram[i] = 0;
	}

	for ( i = 0; i < num_pixels; i++ ) {
		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			pixel = uint8_array[i];
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			pixel = uint16_array[i];
			break;
		}

		pixel_sd = ( ( pixel - mean ) / standard_deviation );

		pixel_bin = mx_round( pixel_sd + MX_IMAGE_STATISTICS_MAX_SD );

		if ( pixel_bin >= MX_IMAGE_STATISTICS_BINS ) {
			(sd_histogram[MX_IMAGE_STATISTICS_BINS - 1])++;
		} else
		if ( pixel_bin < 0 ) {
			(sd_histogram[0])++;
		} else {
			(sd_histogram[pixel_bin])++;
		}
	}

	/* Show the results. */

	mx_info( "(%lux%lu) %lu-bit image frame",
		row_framesize, column_framesize,
		(unsigned long) MXIF_BITS_PER_PIXEL(frame) );

	mx_info( "  mean = %g, standard deviation = %g",
		mean, standard_deviation );

	mx_info( " " );

	mx_info( "        min = %g, max = %g", min_pixel, max_pixel );

	mx_info( " " );

	for ( i = 0; i < MX_IMAGE_STATISTICS_BINS; i++ ) {

		sd_bin = i - MX_IMAGE_STATISTICS_MAX_SD,

		sd_value = mean + standard_deviation * sd_bin;

		mx_info( " %15.3f (%3ld) : %lu",
			sd_value, sd_bin, sd_histogram[i] );
	}

	mx_info( " " );

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

	if ( frame->image_data == NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No image data has been read into image frame %p.", frame );
	}

	if ( image_length != (size_t *) NULL ) {
		*image_length = frame->image_length;
	}

	if ( image_data_pointer != (void **) NULL ) {
		*image_data_pointer = frame->image_data;
	}

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
mx_image_copy_frame( MX_IMAGE_FRAME *old_frame,
			MX_IMAGE_FRAME **new_frame_ptr )
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
				(long) MXIF_ROW_FRAMESIZE(old_frame),
				(long) MXIF_COLUMN_FRAMESIZE(old_frame),
				(long) MXIF_IMAGE_FORMAT(old_frame),
				(long) MXIF_BYTE_ORDER(old_frame),
				MXIF_BYTES_PER_PIXEL(old_frame),
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
mx_image_copy_header( MX_IMAGE_FRAME *source_frame,
			MX_IMAGE_FRAME *destination_frame )
{
	static const char fname[] = "mx_image_copy_header()";

	void *dest_header_ptr, *src_header_ptr;
	void *dest_trailing_ptr;
	size_t dest_header_length, src_header_length;
	size_t dest_trailing_length;

	if ( destination_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The destination frame pointer passed was NULL." );
	}
	if ( source_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The source frame pointer passed was NULL." );
	}

	dest_header_length = destination_frame->header_length;
	dest_header_ptr    = destination_frame->header_data;

	if ( dest_header_ptr == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The header_data array for the destination frame "
		"has not been initialized." );
	}

	src_header_length = source_frame->header_length;
	src_header_ptr    = source_frame->header_data;

	if ( src_header_ptr == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The header_data array for the source frame "
		"has not been initialized." );
	}

	if ( dest_header_length <= src_header_length ) {
		memcpy( dest_header_ptr, src_header_ptr, dest_header_length );
	} else {
		memcpy( dest_header_ptr, src_header_ptr, src_header_length );

		/* Set the unused part of the header to 0. */

		dest_trailing_length = dest_header_length - src_header_length;

		dest_trailing_ptr = (char *)dest_header_ptr + src_header_length;

		memset( dest_trailing_ptr, 0, dest_trailing_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_rebin( MX_IMAGE_FRAME **rebinned_frame,
		MX_IMAGE_FRAME *original_frame,
		unsigned long rebinned_width,
		unsigned long rebinned_height )
{
	static const char fname[] = "mx_image_rebin()";

	unsigned long original_width, original_height;
	unsigned long rebinned_size;
	unsigned long bytes_per_pixel;
	double diff;
	long original_dimensions[2], rebinned_dimensions[2];
	size_t element_size[2];
	void *original_array, *rebinned_array;
	uint16_t **original_array_16, **rebinned_array_16;
	double sum, pixels_per_bin, pixel_average;
	unsigned long width_shrink_factor, width_growth_factor;
	unsigned long height_shrink_factor, height_growth_factor;
	unsigned long row, col, orow, ocol;
	unsigned long row_start, row_end, col_start, col_end;
	unsigned long orow_start, orow_end, ocol_start, ocol_end;
	uint16_t opixel_16;
	mx_bool_type shrink_width, shrink_height;
	mx_status_type mx_status, mx_status2;

	width_shrink_factor = 0;
	width_growth_factor = 0;
	height_shrink_factor = 0;
	height_growth_factor = 0;

	if ( original_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The original_frame pointer passed was NULL." );
	}
	if ( rebinned_frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The rebinned_frame pointer passed was NULL." );
	}
	if ( rebinned_width == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The rebinned width of an image cannot be 0." );
	}
	if ( rebinned_height == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The rebinned height of an image cannot be 0." );
	}

	/* How many bytes are there in a pixel of the original image? */

	bytes_per_pixel = mx_round( MXIF_BYTES_PER_PIXEL(original_frame) );

	/* Rebinning is only supported for images with an integer number
	 * of bytes per pixel.
	 */

	diff = MXIF_BYTES_PER_PIXEL(original_frame) - (double) bytes_per_pixel;

	if ( fabs(diff) > 0.01 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image rebinning is only supported for images that have "
		"an integer number of bytes per pixel.  The MX_IMAGE_FRAME "
		"passed to us has %f bytes per pixel.",
			MXIF_BYTES_PER_PIXEL(original_frame) );
	}

	/* Actually, we currently only support 8, 16, or 32 bit pixels. */

	switch( bytes_per_pixel ) {
	case 1:
	case 2:
	case 4:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"The original image frame contains an unsupported "
			"number of bytes per pixel (%ld).  Currently, we only "
			"support 1, 2 or 4 bytes per pixel.",
				bytes_per_pixel );
		break;
	}

	/* Are the new rebinned dimensions an integer multiple or factor
	 * of the original dimensions?  If not, then we cannot rebin the
	 * original array.
	 */

	original_width = MXIF_ROW_FRAMESIZE(original_frame);
	original_height = MXIF_COLUMN_FRAMESIZE(original_frame);

	if ( original_width == 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The width of the image frame passed to this function was 0.");
	}
	if ( original_height == 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The height of the image frame passed to this function was 0.");
	}

	if ( original_width >= rebinned_width ) {
#if 0
		if ( (original_width % rebinned_width) != 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The rebinned width (%lu) of the image is not "
			"an integer factor of the original width (%lu).",
				rebinned_width, original_width );
		}
#endif

		shrink_width = TRUE;

		width_shrink_factor = original_width / rebinned_width;
	} else {
#if 0
		if ( (rebinned_width % original_width) != 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The rebinned width (%lu) of the image is not "
			"an integer multiple of the original width (%lu).",
				rebinned_width, original_width );
		}
#endif

		shrink_width = FALSE;

		width_growth_factor = rebinned_width / original_width;
	}

	if ( original_height >= rebinned_height ) {
#if 0
		if ( (original_height % rebinned_height) != 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The rebinned height (%lu) of the image is not "
			"an integer factor of the original height (%lu).",
				rebinned_height, original_height );
		}
#endif

		shrink_height = TRUE;

		height_shrink_factor = original_height / rebinned_height;
	} else {
#if 0
		if ( (rebinned_height % original_height) != 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The rebinned height (%lu) of the image is not "
			"an integer multiple of the original height (%lu).",
				rebinned_height, original_height );
		}
#endif

		shrink_height = FALSE;

		height_growth_factor = rebinned_height / original_height;
	}

#if MX_IMAGE_DEBUG_REBIN
	    MX_DEBUG(-2,
		("\nREBIN: original_width = %lu original_height = %lu",
			original_width, original_height));
	    MX_DEBUG(-2,
		("REBIN: rebinned_width = %lu rebinned_height = %lu",
			rebinned_width, rebinned_height));
	    MX_DEBUG(-2,("REBIN: shrink_width = %d, shrink_height = %d",
	    		(int) shrink_width, (int) shrink_height));
	    MX_DEBUG(-2,
	("REBIN: width_shrink_factor = %lu height_shrink_factor = %lu",
			width_shrink_factor, height_shrink_factor));
	    MX_DEBUG(-2,
	("REBIN: width_growth_factor = %lu height_growth_factor = %lu",
			width_growth_factor, height_growth_factor));
#endif

	/* Create or resize the rebinned correction frame structure.  If
	 * there was an existing rebinned frame and the new rebinned frame
	 * will be smaller than the allocated length of the existing frame,
	 * then this will allow us to skip invoking malloc().
	 */

	rebinned_size = rebinned_width * rebinned_height
			* mx_round( MXIF_BYTES_PER_PIXEL(original_frame) );

	mx_status = mx_image_alloc( rebinned_frame,
				(long) rebinned_width,
				(long) rebinned_height,
				(long) MXIF_IMAGE_FORMAT(original_frame),
				(long) MXIF_BYTE_ORDER(original_frame),
				bytes_per_pixel,
				original_frame->header_length,
				rebinned_size );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Fix up the image header of the rebinned image. */

	if ( shrink_width ) {
		MXIF_ROW_BINSIZE(*rebinned_frame) =
		  width_shrink_factor * MXIF_ROW_BINSIZE(original_frame);
	} else {
		MXIF_ROW_BINSIZE(*rebinned_frame) =
		  MXIF_ROW_BINSIZE(original_frame) / width_growth_factor;
	}

	if ( shrink_height ) {
		MXIF_COLUMN_BINSIZE(*rebinned_frame) =
		  height_shrink_factor * MXIF_COLUMN_BINSIZE(original_frame);
	} else {
		MXIF_COLUMN_BINSIZE(*rebinned_frame) =
		  MXIF_COLUMN_BINSIZE(original_frame) / height_growth_factor;
	}

	MXIF_BITS_PER_PIXEL(*rebinned_frame) =
				MXIF_BITS_PER_PIXEL(original_frame);

	MXIF_EXPOSURE_TIME_SEC(*rebinned_frame) =
				MXIF_EXPOSURE_TIME_SEC(original_frame);

	MXIF_EXPOSURE_TIME_NSEC(*rebinned_frame) =
				MXIF_EXPOSURE_TIME_NSEC(original_frame);

	MXIF_TIMESTAMP_SEC(*rebinned_frame) =
				MXIF_TIMESTAMP_SEC(original_frame);

	MXIF_TIMESTAMP_NSEC(*rebinned_frame) =
				MXIF_TIMESTAMP_NSEC(original_frame);

	/* Zero out the existing contents of the rebinned frame. */

	memset( (*rebinned_frame)->image_data, 0,
			(*rebinned_frame)->allocated_image_length );

	/* Construct 2-dimensional arrays on top of the 1-dimensional
	 * image frame buffers to make the math simpler.
	 */

	element_size[0] = bytes_per_pixel;
	element_size[1] = sizeof(void *);

	original_dimensions[0] = (long) original_height;
	original_dimensions[1] = (long) original_width;

	rebinned_dimensions[0] = (long) rebinned_height;
	rebinned_dimensions[1] = (long) rebinned_width;

	mx_status = mx_array_add_overlay( original_frame->image_data, 2,
					original_dimensions, element_size,
					&original_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_array_add_overlay( (*rebinned_frame)->image_data, 2,
					rebinned_dimensions, element_size,
					&rebinned_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now construct the rebinned pixel values from the original
	 * pixel values.  Regrettably, the code has to be duplicated for
	 * each bytes per pixel size since C is not polymorphic.
	 */

	switch( bytes_per_pixel ) {
	case 1:
	case 4:
	    return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Support for 8-bit and 32-bit arrays has not yet "
			"been implemented." );
	    break;

	case 2:
	    original_array_16 = original_array;
	    rebinned_array_16 = rebinned_array;

#if MX_IMAGE_DEBUG_REBIN
	    MX_DEBUG(-2,("%s: shrink_width = %d, shrink_height = %d",
	    	fname, (int) shrink_width, (int) shrink_height));
#endif

	    if ( shrink_width & shrink_height ) {

		pixels_per_bin = width_shrink_factor * height_shrink_factor;

		for ( row = 0; row < rebinned_height; row++ ) {
		    for ( col = 0; col < rebinned_width; col++ ) {
			sum = 0.0;

			orow_start = row * height_shrink_factor;
			orow_end = orow_start + height_shrink_factor;

			ocol_start = col * width_shrink_factor;
			ocol_end = ocol_start + width_shrink_factor;

#if 0 && MX_IMAGE_DEBUG_REBIN
			MX_DEBUG(-2,("REBIN_loop: %lu %lu %lu %lu",
			orow_start, orow_end, ocol_start, ocol_end));
#endif
			for ( orow = orow_start; orow < orow_end; orow++ ) {
			    for ( ocol = ocol_start; ocol < ocol_end; ocol++ ) {
			    	sum += (double) original_array_16[orow][ocol];
			    }
			}

			pixel_average = sum / pixels_per_bin;

			/* Round to the nearest integer. */

			rebinned_array_16[row][col] = pixel_average + 0.5;
		    }
		}

	    } else
	    if ( (!shrink_width) & (!shrink_height) ) {

	        for ( orow = 0; orow < original_height; orow++ ) {
		    for ( ocol = 0; ocol < original_height; ocol++ ) {

		    	row_start = orow * height_growth_factor;
			row_end = row_start + height_growth_factor;

			col_start = ocol * width_growth_factor;
			col_end = col_start + width_growth_factor;

#if 0 && MX_IMAGE_DEBUG_REBIN
			MX_DEBUG(-2,("REBIN_loop: %lu %lu %lu %lu",
			row_start, row_end, col_start, col_end));
#endif
			opixel_16 = original_array_16[orow][ocol];

			for ( row = row_start; row < row_end; row++ ) {
			    for ( col = col_start; col < col_end; col++ ) {
			    	rebinned_array_16[row][col] = opixel_16;
			    }
			}
		    }
		}

	    } else {
	    	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The combination of shrink_width = %d and shrink_height = %d "
		"has not yet been implemented.",
			(int) shrink_width, (int) shrink_height );
	    }
	    break;
	}

	/* Finish by freeing the overlay arrays. */

	mx_status = mx_array_free_overlay( original_array, 2 );

	mx_status2 = mx_array_free_overlay( rebinned_array, 2 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status2;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_image_dezinger( MX_IMAGE_FRAME **dezingered_frame,
			unsigned long num_original_frames,
			MX_IMAGE_FRAME **original_frame_array,
			double threshold )
{
	static const char fname[] = "mx_image_dezinger()";

	MX_IMAGE_FRAME *dz_frame, *original_frame;
	unsigned long i, num_pixels;
	double diff;
	mx_bool_type skip_dezinger;

	if ( original_frame_array == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The original_frame_array pointer passed was NULL." );
	}

	if ( dezingered_frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dezingered_frame pointer passed was NULL." );
	}

	if ( (*dezingered_frame) == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The value of the dezingered_frame pointer passed was NULL." );
	}

	if ( num_original_frames < 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of original frames to be dezingered (%lu) "
		"is less than the minimum value of 2.", num_original_frames );
	}

	threshold = fabs(threshold);

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s invoked for %lu image frames.",
			fname, num_original_frames));
#endif

	dz_frame = *dezingered_frame;

	/* Verify that all of the frames have the same image type. */

	for ( i = 0; i < num_original_frames; i++ ) {

		original_frame = original_frame_array[i];
#if 0
		mx_image_statistics(original_frame);
#endif
		if ( original_frame == (MX_IMAGE_FRAME *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The frame pointer for frame %lu in the "
			"original frame array is NULL.", i );
		}

		if ( MXIF_ROW_FRAMESIZE(dz_frame)
			!= MXIF_ROW_FRAMESIZE(original_frame) )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The X framesize %ld of the dezingered frame "
			"is different than the X framesize %ld "
			"of element %lu in the original frame array.",
				(long) MXIF_ROW_FRAMESIZE(dz_frame),
				(long) MXIF_ROW_FRAMESIZE(original_frame), i );
		}

		if ( MXIF_COLUMN_FRAMESIZE(dz_frame)
			!= MXIF_COLUMN_FRAMESIZE(original_frame) )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The Y framesize %ld of the dezingered frame "
			"is different than the Y framesize %ld "
			"of element %lu in the original frame array.",
				(long) MXIF_COLUMN_FRAMESIZE(dz_frame),
			    (long) MXIF_COLUMN_FRAMESIZE(original_frame), i );
		}

		if ( MXIF_IMAGE_FORMAT(dz_frame)
			!= MXIF_IMAGE_FORMAT(original_frame) )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The image format %ld of the dezingered frame "
			"is different than the image format %ld "
			"of element %lu in the original frame array.",
				(long) MXIF_IMAGE_FORMAT(dz_frame),
				(long) MXIF_IMAGE_FORMAT(original_frame), i );
		}

		if ( MXIF_BYTE_ORDER(dz_frame)
			!= MXIF_BYTE_ORDER(original_frame) )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The byte order %ld of the dezingered frame "
			"is different than the byte order %ld "
			"of element %lu in the original frame array.",
				(long) MXIF_BYTE_ORDER(dz_frame),
				(long) MXIF_BYTE_ORDER(original_frame), i );
		}

		if ( MXIF_BYTE_ORDER(dz_frame)
			!= MXIF_BYTE_ORDER(original_frame) )
		{
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The byte order %ld of the dezingered frame "
			"is different than the byte order %ld "
			"of element %lu in the original frame array.",
				(long) MXIF_BYTE_ORDER(dz_frame),
				(long) MXIF_BYTE_ORDER(original_frame), i );
		}

		diff = mx_difference( MXIF_BYTES_PER_MILLION_PIXELS(dz_frame),
				MXIF_BYTES_PER_MILLION_PIXELS(original_frame) );

		if ( diff > 10.0 ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The number of bytes per pixel (%g) of the "
			"dezingered frame is different than the number "
			"of bytes per pixel (%g) of element %lu in the "
			"original frame array.",
				MXIF_BYTES_PER_PIXEL(dz_frame),
				MXIF_BYTES_PER_PIXEL(original_frame), i );
		}

		if ( dz_frame->image_length != original_frame->image_length ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The image length %lu of the dezingered frame "
			"is different than the image length %lu "
			"of element %lu in the original frame array.",
				(unsigned long) dz_frame->image_length,
				(unsigned long) original_frame->image_length,
				i );
		}
	}

	if ( MXIF_IMAGE_FORMAT(dz_frame) != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Image dezingering is currently only supported for "
		"16-bit greyscale images." );
	}

	num_pixels = dz_frame->image_length / sizeof(uint16_t);

	/* If the dezinger threshold is very close to DBL_MAX, then we
	 * do not dezinger the image.
	 */

	if ( threshold > (DBL_MAX / 1.01) ) {
		skip_dezinger = TRUE;
	} else {
		skip_dezinger = FALSE;
	}

	if (1) {
		/* num_original_frames > 2 */

		/* For this case, we compute the standard deviation of
		 * the pixel values.  Pixel values that are larger than
		 * the threshold (in units of standard deviation) are
		 * left out of the sum.
		 */

		uint16_t *dz_image_data, *original_image_data;
		double sum, sum_of_squares;
		double mean, standard_deviation, pixel;
		double dz_sum, dz_mean, scaled_threshold;
		unsigned long j, dz_num_frames;

		dz_image_data = dz_frame->image_data;

		for ( i = 0; i < num_pixels; i++ ) {

			/* First compute the mean. */

			sum = 0.0;

			for ( j = 0; j < num_original_frames; j++ ) {

				original_image_data =
					original_frame_array[j]->image_data;

				pixel = original_image_data[i];

				sum += pixel;
			}

			mean = sum / (double) num_original_frames;

			if ( skip_dezinger ) {
				dz_image_data[i] = (uint16_t) mx_round(mean);

				continue;  /* Go back to the top of the loop. */
			}

			/* Next compute the standard deviation. */

			sum_of_squares = 0.0;

			for ( j = 0; j < num_original_frames; j++ ) {

				original_image_data =
					original_frame_array[j]->image_data;

				pixel = original_image_data[i];

				diff = pixel - mean;

				sum_of_squares += (diff * diff);
			}

			standard_deviation = sqrt( sum_of_squares
			    / ( ((double) num_original_frames) - 1.0) );

			/* Now compute the dezingered mean.  Pixels that
			 * are larger than the scaled threshold are
			 * left out of the sum.
			 */

			scaled_threshold = mx_multiply_safely( threshold,
							standard_deviation );

			if ( fabs(scaled_threshold) < 1.0e-30 ) {
				dz_image_data[i] = (uint16_t) mx_round(mean);
			} else {
				dz_sum = 0.0;

				dz_num_frames = 0;

				for ( j = 0; j < num_original_frames; j++ ) {

					original_image_data =
					    original_frame_array[j]->image_data;

					pixel = original_image_data[i];

					if ((pixel - mean) < scaled_threshold) {
						dz_sum += pixel;
						dz_num_frames += 1L;
#if MX_IMAGE_TEST_DEZINGER
						MX_DEBUG(-2,(
			"%s: Discarding pixel value %g at location %lu, "
			"frame = %lu, mean = %g, scaled_threshold = %g",
							fname, pixel, i, j,
							mean,scaled_threshold));
#endif
					}
				}

				dz_mean = dz_sum / (double) dz_num_frames;

				dz_image_data[i] = (uint16_t) mx_round(dz_mean);
			}
		}
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s complete for %lu image frames.",
		fname, num_original_frames));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

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
	case MXT_IMAGE_FILE_RAW:
		mx_status = mx_image_read_raw_file( frame_ptr, datafile_name );
		break;
	case MXT_IMAGE_FILE_SMV:
		mx_status = mx_image_read_smv_file( frame_ptr, datafile_name );
		break;
	case MXT_IMAGE_FILE_MARCCD:
		mx_status = mx_image_read_marccd_file( frame_ptr,
							datafile_name );
		break;
	case MXT_IMAGE_FILE_EDF:
		mx_status = mx_image_read_edf_file( frame_ptr, datafile_name );
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

#if 0
	{
		uint16_t *image_data;
		uint8_t *byte_data;

		image_data = (uint16_t *) frame->image_data;

		byte_data = (uint8_t *) frame->image_data;

		MX_DEBUG(-2,("%s: byte_data = %u %u %u %u %u %u ...",
		fname, byte_data[0], byte_data[1], byte_data[2],
		byte_data[3], byte_data[4], byte_data[5]));

		MX_DEBUG(-2,("%s: image_data = %u %u %u ...",
		fname, image_data[0], image_data[1], image_data[2]));
	}
#endif

#if 0
	mx_image_statistics( frame );
#endif

	switch( datafile_type ) {
	case MXT_IMAGE_FILE_PNM:
		mx_status = mx_image_write_pnm_file( frame, datafile_name );
		break;
	case MXT_IMAGE_FILE_RAW:
		mx_status = mx_image_write_raw_file( frame, datafile_name );
		break;
	case MXT_IMAGE_FILE_SMV:
		mx_status = mx_image_write_smv_file( frame, datafile_name );
		break;
	case MXT_IMAGE_FILE_EDF:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
			"Writing EDF format image files for datafile '%s' "
			"is not currently supported.", datafile_name );
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

	/* The third line should contain the width and height.
	 *
	 * If the second line read did not start with a # character,
	 * we assume that this file was produced by NetPBM and that
	 * the second line is the one that contains the width and height.
	 * In that case, we do not read another line here.
	 */

	if ( buffer[0] == '#' ) {
		ptr = fgets( buffer, sizeof(buffer), file );

		if ( ptr == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot read the third line of PNM image file '%s'.  "
			"Errno = %d, error message = '%s'",
				datafile_name,
				saved_errno, strerror(saved_errno) );
		}
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
					framesize[0],
					framesize[1],
					image_format,
					MX_DATAFMT_BIG_ENDIAN,
					(double) bytes_per_pixel,
					0,
					bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read in the binary part of the image file. */

	bytes_read = (long) fread( (*frame)->image_data, sizeof(unsigned char),
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

		MXIF_BYTE_ORDER(*frame) = MX_DATAFMT_LITTLE_ENDIAN;
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

	MX_DEBUG(-2,("%s: width = %ld, height = %ld", fname,
		(long) MXIF_ROW_FRAMESIZE(frame),
		(long) MXIF_COLUMN_FRAMESIZE(frame) ));
	MX_DEBUG(-2,("%s: image_format = %ld, byte_order = %ld", fname,
		(long) MXIF_IMAGE_FORMAT(frame),
		(long) MXIF_BYTE_ORDER(frame)));
	MX_DEBUG(-2,("%s: image_length = %lu, image_data = %p",
		fname, (unsigned long) frame->image_length, frame->image_data));
#endif

	byteorder = mx_native_byteorder();

	switch( MXIF_IMAGE_FORMAT(frame) ) {
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
			(long) MXIF_IMAGE_FORMAT(frame), datafile_name );
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
	fprintf( file, "%lu %lu\n",
		(unsigned long) MXIF_ROW_FRAMESIZE(frame),
		(unsigned long) MXIF_COLUMN_FRAMESIZE(frame) );
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

		switch( MXIF_IMAGE_FORMAT(frame) ) {

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
				(long) MXIF_IMAGE_FORMAT(frame),
				datafile_name );
		}
	}

	fclose( file );

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,
	("%s: PNM file '%s' successfully written.", fname, datafile_name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_image_read_raw_file( MX_IMAGE_FRAME **frame, char *datafile_name )
{
	static const char fname[] = "mx_image_read_raw_file()";

	FILE *file;
	struct stat file_stat;
	int os_status, saved_errno;
	long framesize[2];
	long image_format, datafile_byteorder;
	long bytes_per_frame, bytes_read;
	double bytes_per_pixel;
	unsigned long image_size_in_bytes;
	double image_size_in_pixels, sqrt_image_size;
	struct timespec timestamp_timespec;
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
	/* We can get the timestamp from the filesystem. */

	os_status = stat( datafile_name, &file_stat );

	if ( os_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot get file status for image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name, saved_errno,
			strerror( saved_errno ) );
	}

	timestamp_timespec.tv_sec = file_stat.st_mtime;
	timestamp_timespec.tv_nsec = 0;

	/* Since the file will not have a header, we need to figure out
	 * the rest of the information about the image format in some
	 * other fashion.
	 */

	if ( (*frame) != (MX_IMAGE_FRAME *) NULL ) {

		/* If (*frame) points to an already existing image frame,
		 * then we can get the necessary information from there.
		 */

		framesize[0] = MXIF_ROW_FRAMESIZE(*frame);
		framesize[1] = MXIF_COLUMN_FRAMESIZE(*frame);
		image_format = MXIF_IMAGE_FORMAT(*frame);
		datafile_byteorder = MXIF_BYTE_ORDER(*frame);
		bytes_per_pixel    = MXIF_BYTES_PER_PIXEL(*frame);
	} else {
		/* If (*frame) is NULL, then we must guess. */

		image_format = MXT_IMAGE_FORMAT_GREY16;
		datafile_byteorder = mx_native_byteorder();
		bytes_per_pixel = 2;

		/* We attempt to infer the framesize by assuming that
		 * the image frame is square.
		 */

		image_size_in_bytes = file_stat.st_size;

		image_size_in_pixels = image_size_in_bytes / bytes_per_pixel;

		sqrt_image_size = sqrt( image_size_in_pixels );

		framesize[0] = mx_round( sqrt_image_size );
		framesize[1] = framesize[0];
	}

	/* Open the data file. */

	file = fopen( datafile_name, "rb" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_NOT_FOUND, fname,
		"Cannot open RAW image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	/* --- */

	bytes_per_frame = bytes_per_pixel * framesize[0] * framesize[1];

	/* Change the size of the MX_IMAGE_FRAME to match the SMV file. */

	mx_status = mx_image_alloc( frame,
					framesize[0],
					framesize[1],
					image_format,
					datafile_byteorder,
					(double) bytes_per_pixel,
					0,
					bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MXIF_TIMESTAMP_SEC(*frame)  = timestamp_timespec.tv_sec;
	MXIF_TIMESTAMP_NSEC(*frame) = timestamp_timespec.tv_nsec;

	/* Add some plausible 'fake' information to the MX_IMAGE_FRAME header.*/

	MXIF_ROW_BINSIZE(*frame)    = 1;
	MXIF_COLUMN_BINSIZE(*frame) = 1;

	MXIF_EXPOSURE_TIME_SEC(*frame)  = 1;
	MXIF_EXPOSURE_TIME_NSEC(*frame) = 0;

	MXIF_BIAS_OFFSET_MILLI_ADUS(*frame) = 0;

	/* Read in the binary image. */

	bytes_read = (long) fread( (*frame)->image_data, sizeof(unsigned char),
				bytes_per_frame, file );

	if ( bytes_read < bytes_per_frame ) {
		if ( feof(file) ) {
			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"End of file at image byte offset %ld for SMV image file '%s'.",
				bytes_read, datafile_name );
		}
		if ( ferror(file) ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred at image byte offset %ld "
			"for SMV image file '%s'.",
				bytes_read, datafile_name );
		}
		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Only %ld image bytes were read from "
			"SMV image file '%s' when %ld bytes were expected.",
				bytes_read, datafile_name, bytes_per_frame );
	}

	fclose( file );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_image_write_raw_file( MX_IMAGE_FRAME *frame, char *datafile_name )
{
	static const char fname[] = "mx_image_write_raw_file()";

	FILE *file;
	int saved_errno;
	size_t bytes_written;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

	if ( datafile_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The datafile_name pointer passed was NULL." );
	}

	if ( frame->image_data == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_IMAGE_FRAME passed (%p) has a NULL image_data pointer.",
			frame );
	}

	file = fopen( datafile_name, "wb" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open SMV image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	bytes_written = fwrite(frame->image_data, 1, frame->image_length, file);

	fclose( file );

	if ( bytes_written < frame->image_length ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Fewer bytes were written (%lu) to RAW image file '%s' "
		"than the number (%lu) in the original image.",
			(unsigned long) bytes_written,
			datafile_name,
			(unsigned long) frame->image_length );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,
	("%s: RAW file '%s' successfully written.", fname, datafile_name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

/*
 *
 * These routines read and write the SMV format used by ADSC detectors
 * and Aviex detectors.
 *
 */

static mx_status_type
mxp_image_parse_smv_date( char *buffer, struct timespec *timestamp )
{
	static const char fname[] = "mxp_image_parse_smv_date()";

	struct tm tm;
	char *ptr, *value_start;
	int num_items;
	unsigned long nanoseconds;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}
	if ( timestamp == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The struct timespec pointer passed was NULL." );
	}

	/* Skip over the variable name, if present. */

	ptr = strchr( buffer, '=' );

	if ( ptr == NULL ) {
		value_start = buffer;
	} else {
		value_start = ptr + 1;
	}

	/*--------------------------------------------------------------*/

	/* This is the format used by mx_os_time_string(). */

	ptr = strptime( value_start, "%a %b %d %Y %H:%M:%S", &tm );

	if ( ptr != NULL ) {
		/* The format matched. */

		timestamp->tv_sec = mktime( &tm );

		if ( *ptr == '.' ) {
			ptr++;

			num_items = sscanf( ptr, "%lu", &nanoseconds );

			if ( num_items == 0 ) {
				timestamp->tv_nsec = 0;
			} else {
				timestamp->tv_nsec = nanoseconds;
			}
		} else {
			timestamp->tv_nsec = 0;
		}

#if MX_IMAGE_DEBUG_TIMESTAMP
		MX_DEBUG(-2,("%s: timestamp = (%lu,%lu)",
			fname, timestamp->tv_sec, timestamp->tv_nsec));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/*--------------------------------------------------------------*/

	/* This is the format used by the Unix 'date' command:
	 *
	 *   DATE=Mon Nov 23 17:08:47 CST 2009;
	 */

	ptr = strptime( value_start, "%a %b %d %H:%M:%S %Z %Y", &tm );

	if ( ptr != NULL ) {
		timestamp->tv_sec = mktime( &tm );
		timestamp->tv_nsec = 0;

#if MX_IMAGE_DEBUG_TIMESTAMP
		MX_DEBUG(-2,("%s: timestamp = (%lu,%lu)  (Timezone OK)",
			fname, timestamp->tv_sec, timestamp->tv_nsec));
#endif

		return MX_SUCCESSFUL_RESULT;

	}	

	/* Unfortunately, some versions of strptime(), such as the Linux
	 * one, do not correctly support timestamps with timezones embedded
	 * in the middle of the string, even if the man page says they do.
	 * The only obvious way around this is to rewrite the timestamp
	 * without the timezone string in it.
	 */

	/* Does this match the part of the pattern before the timezone? */

	ptr = strptime( value_start, "%a %b %d %H:%M:%S", &tm );

	if ( ptr != NULL ) {
		int argc; char **argv;
		char *value_dup;

		/* It _does_ match, so split the string into tokens. */

		value_dup = strdup( value_start );

		mx_string_split( value_dup, " \t", &argc, &argv );

		if ( argc >= 6 ) {
			char time_string[80];

			/* Paste the string back together,
			 * skipping the timezone.
			 */

			snprintf( time_string, sizeof(time_string),
				"%s %s %s %s %s",
				argv[0], argv[1], argv[2], argv[3], argv[5] );

			ptr = strptime( time_string,
					"%a %b %d %H:%M:%S %Y", &tm );

			if ( ptr != NULL ) {
				/* OK, we have succeeded at parsing the time. */

				timestamp->tv_sec = mktime( &tm );
				timestamp->tv_nsec = 0;

#if MX_IMAGE_DEBUG_TIMESTAMP
				MX_DEBUG(-2,
				("%s: timestamp = (%lu,%lu) (Timezone NOT OK)",
					fname,
					timestamp->tv_sec, timestamp->tv_nsec));
#endif
				mx_free( argv );
				mx_free( value_dup );

				return MX_SUCCESSFUL_RESULT;
			}
		}

		mx_free( argv );
		mx_free( value_dup );
	}

	/*--------------------------------------------------------------*/

	mx_warning( "The timestamp '%s' is not in a format recognized by "
			"%s.  The timestamp will be set to 0.",
			buffer, fname );

	timestamp->tv_sec = 0;
	timestamp->tv_nsec = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_image_read_smv_file( MX_IMAGE_FRAME **frame, char *datafile_name )
{
	static const char fname[] = "mx_image_read_smv_file()";

	FILE *file;
	char buffer[100];
	char byte_order_buffer[40];
	char *ptr;
	int saved_errno, os_status, num_items;
	long framesize[2], binsize[2];
	long bytes_per_pixel, bytes_per_frame, bytes_read, image_format;
	long header_length, datafile_byteorder, num_whitespace_chars;
	double exposure_time;
	struct timespec exposure_timespec;
	struct timespec timestamp_timespec;
	double bias_offset_in_adus;
	unsigned long bias_offset_in_milli_adus;
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

	/* Open the data file. */

	file = fopen( datafile_name, "rb" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_NOT_FOUND, fname,
		"Cannot open SMV image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	/* The first line of an SMV file should be a '{' character
	 * followed by a line feed character.
	 */

	ptr = fgets( buffer, sizeof(buffer), file );

	if ( ptr == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read the first line of SMV image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	if ( ( buffer[0] != '{' ) || ( buffer[1] != '\n' ) ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Data file '%s' does not appear to be an SMV file.",
			datafile_name );
	}

	/* The second line should tell us the length of the header. */

	ptr = fgets( buffer, sizeof(buffer), file );

	if ( ptr == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot find the second line of SMV image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	num_items = sscanf( buffer, "HEADER_BYTES=%ld", &header_length );

	if ( num_items != 1 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The file '%s' does not appear to be an SMV file, since the "
		"second line of the file does not contain the header length.",
			datafile_name );
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: HEADER_BYTES = %ld", fname, header_length));
#endif

	/* Read in the rest of the header. */

	datafile_byteorder = -1;
	framesize[0] = -1;
	framesize[1] = -1;
	binsize[0] = 1;
	binsize[1] = 1;
	exposure_time = 1.0;
	memset( &exposure_timespec, 0, sizeof(exposure_timespec) );
	memset( &timestamp_timespec, 0, sizeof(timestamp_timespec) );
	bias_offset_in_adus = 0.0;
	bias_offset_in_milli_adus = 0;

	for (;;)  {
		ptr = fgets( buffer, sizeof(buffer), file );

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif

		if ( ptr == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot read the next header line of "
			"SMV image file '%s'.  "
			"Errno = %d, error message = '%s'",
				datafile_name,
				saved_errno, strerror(saved_errno) );
		}

		if ( buffer[0] == '}' ) {
			/* We have reached the end of the text header,
			 * so break out of the for(;;) loop.
			 */

			break;
		} else
		if ( strncmp( buffer, "BYTE_ORDER=", 11 ) == 0 ) {

			/* Look for the first non-whitespace character 
			 * after the equals sign.
			 */

			ptr = strchr( buffer, '=' );

			if ( ptr == NULL ) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				fname, "Apparently the buffer '%s' does not "
				"contain a '=' character.  However a previous "
				"statement said that it _did_ contain a '=' "
				"character.  This is inconsistent.", buffer );
			}

			ptr++;	/* Step over the '=' character. */

			num_whitespace_chars = (long) strspn( ptr, " \t" );

			ptr += num_whitespace_chars;

			if ( strncmp(ptr, "little_endian", 13 ) == 0 ) {
				datafile_byteorder = MX_DATAFMT_LITTLE_ENDIAN;
			} else
			if ( strncmp(ptr, "big_endian", 10 ) == 0 ) {
				datafile_byteorder = MX_DATAFMT_BIG_ENDIAN;
			} else {
				return mx_error( MXE_UNSUPPORTED, fname,
				"The BYTE_ORDER statement in the header of "
				"data file '%s' says that the data file "
				"byte order has the unrecognized value '%s'.",
					datafile_name, byte_order_buffer );
			}
		} else
		if ( strncmp( buffer, "SIZE1=", 6 ) == 0 ) {
			num_items = sscanf(buffer, "SIZE1=%lu", &framesize[0]);

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"SIZE1 statement.",
					buffer, datafile_name );
			}
		} else
		if ( strncmp( buffer, "SIZE2=", 6 ) == 0 ) {
			num_items = sscanf(buffer, "SIZE2=%lu", &framesize[1]);

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"SIZE2 statement.",
					buffer, datafile_name );
			}
		} else
		if ( strncmp( buffer, "BIN=", 4 ) == 0 ) {
			if ( strncmp( buffer, "BIN=none;", 9  ) == 0 ) {
				binsize[0] = 1;
				binsize[1] = 1;
			} else {
				num_items = sscanf(buffer, "BIN=%lux%lu",
						&binsize[0], &binsize[1]);

				if ( num_items != 2 ) {
					return mx_error(
					MXE_UNPARSEABLE_STRING, fname,
					"Header line '%s' from data file '%s' "
					"appears to contain an incorrectly "
					"formatted BIN statement.",
						buffer, datafile_name );
				}
			}
		} else
		if ( strncmp( buffer, "TIME=", 5 ) == 0 ) {
			num_items = sscanf(buffer, "TIME=%lg", &exposure_time );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"TIME statement.",
					buffer, datafile_name );
			}
		} else
		if ( strncmp( buffer, "DATE=", 5 ) == 0 ) {
			mx_status = mxp_image_parse_smv_date( buffer,
							&timestamp_timespec );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else
		if ( strncmp( buffer, "ZEROOFFSET=", 11 ) == 0 ) {
			num_items = sscanf(buffer, "ZEROOFFSET=%lg",
						&bias_offset_in_adus );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"ZEROOFFSET statement.",
					buffer, datafile_name );
			}

			bias_offset_in_milli_adus = 
				mx_round( 1000.0 * bias_offset_in_adus );
		}
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: datafile_byteorder = %ld, framesize = (%ld,%ld)",
		fname, datafile_byteorder, framesize[0], framesize[1]));
	MX_DEBUG(-2,("%s: binsize = (%ld,%ld), exposure time = %f",
		fname, binsize[0], binsize[1], exposure_time));
#endif

	if ( datafile_byteorder < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"SMV data file '%s' did not contain a BYTE_ORDER "
		"statement in its header.", datafile_name );
	}

	if ( (framesize[0] < 0) || (framesize[1] < 0) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The header for SMV file '%s' did not contain one or both "
		"of the SIZE1 and SIZE2 statements.  These statements "
		"are used to specify the dimensions of the image and "
		"must be present.", datafile_name );
	}

	/* --- */

	image_format = MXT_IMAGE_FORMAT_GREY16;

	bytes_per_pixel = 2;

	bytes_per_frame = bytes_per_pixel * framesize[0] * framesize[1];

	/* Change the size of the MX_IMAGE_FRAME to match the SMV file. */

	mx_status = mx_image_alloc( frame,
					framesize[0],
					framesize[1],
					image_format,
					datafile_byteorder,
					(double) bytes_per_pixel,
					0,
					bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Add more information to the MX_IMAGE_FRAME header. */

	MXIF_ROW_BINSIZE(*frame)    = binsize[0];
	MXIF_COLUMN_BINSIZE(*frame) = binsize[1];

	exposure_timespec =
		mx_convert_seconds_to_high_resolution_time( exposure_time );

	MXIF_EXPOSURE_TIME_SEC(*frame)  = exposure_timespec.tv_sec;
	MXIF_EXPOSURE_TIME_NSEC(*frame) = exposure_timespec.tv_nsec;

	MXIF_TIMESTAMP_SEC(*frame)  = timestamp_timespec.tv_sec;
	MXIF_TIMESTAMP_NSEC(*frame) = timestamp_timespec.tv_nsec;

	MXIF_BIAS_OFFSET_MILLI_ADUS(*frame) = bias_offset_in_milli_adus;

	/* Move to the first byte after the header. */

#if 1
	os_status = fseek( file, header_length, SEEK_SET );
#else
	os_status = fseek( file, MXU_IMAGE_SMV_HEADER_LENGTH, SEEK_SET );
#endif

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An attempt to seek to the end of the header block "
		"in image file '%s' failed with errno = %d, "
		"error message = '%s'", datafile_name,
			saved_errno, strerror( saved_errno ) );
	}

	/* Read in the binary part of the image file. */

	bytes_read = (long) fread( (*frame)->image_data, sizeof(unsigned char),
				bytes_per_frame, file );

	if ( bytes_read < bytes_per_frame ) {
		if ( feof(file) ) {
			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"End of file at image byte offset %ld for SMV image file '%s'.",
				bytes_read, datafile_name );
		}
		if ( ferror(file) ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred at image byte offset %ld "
			"for SMV image file '%s'.",
				bytes_read, datafile_name );
		}
		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Only %ld image bytes were read from "
			"SMV image file '%s' when %ld bytes were expected.",
				bytes_read, datafile_name, bytes_per_frame );
	}

	/* Close the SMV image file. */

	fclose( file );

	/* If the byte order in the file does not match the native
	 * byte order of the machine we are running on, then we must
	 * byteswap the bytes in the image.
	 */

	if ( mx_native_byteorder() != datafile_byteorder ) {
		uint16_t *uint16_array;
		long i, words_per_frame;

		/* Byteswap the 16-bit integers. */

		uint16_array = (*frame)->image_data;

		words_per_frame = framesize[0] * framesize[1];

		for ( i = 0; i < words_per_frame; i++ ) {
			uint16_array[i] = mx_16bit_byteswap( uint16_array[i] );
		}

		MXIF_BYTE_ORDER(*frame) = mx_native_byteorder();
	}

	/* We are done, so return. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_image_write_smv_file( MX_IMAGE_FRAME *frame, char *datafile_name )
{
	static const char fname[] = "mx_image_write_smv_file()";

	FILE *file;
	pixel_converter_t converter;
	void (*converter_fn)( unsigned char *, unsigned char * );
	int src_step, dest_step;
	unsigned char *src;
	unsigned char dest[10];
	int saved_errno, os_status, num_items_written;
	unsigned long byteorder;
	unsigned char null_header_bytes[MXU_IMAGE_SMV_HEADER_LENGTH] = { 0 };
	long i;
	double exposure_time;
	struct timespec exposure_timespec;
	char timestamp[80];
	struct timespec timestamp_timespec;
	unsigned long bias_offset_milli_adus;

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

	MX_DEBUG(-2,("%s: width = %ld, height = %ld", fname, 
		(long) MXIF_ROW_FRAMESIZE(frame),
		(long) MXIF_COLUMN_FRAMESIZE(frame) ));

	MX_DEBUG(-2,("%s: image_format = %ld, byte_order = %ld", fname,
		(long) MXIF_IMAGE_FORMAT(frame),
		(long) MXIF_BYTE_ORDER(frame)));

	MX_DEBUG(-2,("%s: image_length = %lu, image_data = %p",
		fname, (unsigned long) frame->image_length, frame->image_data));
#endif

	byteorder = mx_native_byteorder();

	switch( MXIF_IMAGE_FORMAT(frame) ) {
	case MXT_IMAGE_FORMAT_RGB565:
		converter = mxp_rgb565_converter;
		break;
	case MXT_IMAGE_FORMAT_YUYV:
		converter = mxp_yuyv_converter;
		break;

	case MXT_IMAGE_FORMAT_RGB:
		converter = mxp_rgb_converter;
		break;
	case MXT_IMAGE_FORMAT_GREY8:
		converter = mxp_grey8_converter;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		converter = mxp_grey16_converter;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image format %ld requested for datafile '%s'.",
			(long) MXIF_IMAGE_FORMAT(frame), datafile_name );
	}

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
		"Cannot open SMV image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	/* Write the SMV header. */

	/* First null out the first 512 bytes of the file. */

	num_items_written = fwrite( null_header_bytes,
				sizeof(null_header_bytes), 1, file );

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: num_items_written = %d", fname, num_items_written));
#endif

	/* Now go back to the start of the file. */

	os_status = fseek( file, 0, SEEK_SET );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An attempt to seek to the end of the header block "
		"in image file '%s' failed with errno = %d, "
		"error message = '%s'", datafile_name,
			saved_errno, strerror( saved_errno ) );
	}

	/* Write the text header. */

	/* The first six headers are written in the order
	 *
	 *   HEADER_BYTES
	 *   DIM
	 *   SIZE1
	 *   SIZE2
	 *   BYTE_ORDER
	 *   TYPE
	 *
	 * This was done for compatibility with the Fit2d program, which
	 * seems to expect this order.
	 *
	 * Older versions of MX used the order
	 *
	 *   HEADER_BYTES
	 *   BYTE_ORDER
	 *   DIM
	 *   TYPE
	 *   SIZE1
	 *   SIZE2
	 *
	 * which somehow caused Fit2d to not correctly figure out what the
	 * image dimensions were.
	 */

	fprintf( file, "{\n" );
	fprintf( file, "HEADER_BYTES=  512;\n" );

	fprintf( file, "DIM=2;\n" );

	fprintf( file, "SIZE1=%lu;\n",
				(unsigned long) MXIF_ROW_FRAMESIZE(frame) );

	fprintf( file, "SIZE2=%lu;\n",
				(unsigned long) MXIF_COLUMN_FRAMESIZE(frame) );

	switch( byteorder ) {
	case MX_DATAFMT_BIG_ENDIAN:
		fprintf( file, "BYTE_ORDER=big_endian;\n" );
		break;
	case MX_DATAFMT_LITTLE_ENDIAN:
		fprintf( file, "BYTE_ORDER=little_endian;\n" );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Byteorder %ld is not supported.", byteorder );
	}

	if ( MXIF_IMAGE_FORMAT(frame) == MXT_IMAGE_FORMAT_GREY16 ) {
		fprintf( file, "TYPE=unsigned_short;\n" );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"8-bit file formats are not supported by SMV format files." );
	}

	fprintf( file, "BIN=%lux%lu;\n",
				(unsigned long) MXIF_ROW_BINSIZE(frame),
				(unsigned long) MXIF_COLUMN_BINSIZE(frame) );

	exposure_timespec.tv_sec  = (long) MXIF_EXPOSURE_TIME_SEC(frame);
	exposure_timespec.tv_nsec = (long) MXIF_EXPOSURE_TIME_NSEC(frame);

	exposure_time =
		mx_convert_high_resolution_time_to_seconds( exposure_timespec );

	fprintf( file, "TIME=%.6f;\n", exposure_time );

	/* Write the time of frame acquisition to the header. */

	timestamp_timespec.tv_sec  = (long) MXIF_TIMESTAMP_SEC(frame);
	timestamp_timespec.tv_nsec = (long) MXIF_TIMESTAMP_NSEC(frame);

	mx_os_time_string( timestamp_timespec, timestamp, sizeof(timestamp) );

	fprintf( file, "DATE=%s;\n", timestamp );

	/* If present, write the bias offset to the file. */

	bias_offset_milli_adus = MXIF_BIAS_OFFSET_MILLI_ADUS(frame);

	if ( (bias_offset_milli_adus % 1000) == 0 ) {
		/* If the bias is an integer number of ADUs, then write
		 * it to the header as an integer.
		 */

		fprintf( file, "ZEROOFFSET=%lu\n",
			bias_offset_milli_adus / 1000 );
	} else {
		fprintf( file, "ZEROOFFSET=%f\n",
			((double) bias_offset_milli_adus) / 1000.0 );
	}

	/* Terminate the part of the header block that we are using. */

	fprintf( file, "}\f" );

	/* Move to the first byte after the header. */

	os_status = fseek( file, MXU_IMAGE_SMV_HEADER_LENGTH, SEEK_SET );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An attempt to seek to the end of the header block "
		"in image file '%s' failed with errno = %d, "
		"error message = '%s'", datafile_name,
			saved_errno, strerror( saved_errno ) );
	}

	/* Loop through the image file. */

	src = frame->image_data;

	for ( i = 0; i < frame->image_length; i += src_step ) {

		/* Convert and write out the pixels. */

		converter_fn( &src[i], dest );

		fwrite( dest, sizeof(unsigned char), dest_step, file );
	}

	fclose( file );

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,
	("%s: SMV file '%s' successfully written.", fname, datafile_name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

/* Read MarCCD image files. */

/* FIXME: MarCCD images are TIFF images, but for now I do a brute force
 * read of the image assuming that there is a 4096 byte header followed
 * by 16-bit image pixels in little endian order.
 */

#define MX_MARCCD_HEADER_SIZE	4096L

mx_status_type
mx_image_read_marccd_file( MX_IMAGE_FRAME **frame, char *marccd_filename )
{
	static const char fname[] = "mx_image_read_marccd_file()";

	FILE *marccd_file;
	struct stat marccd_stat;
	int marccd_fd, os_status, saved_errno;
	unsigned long file_size_in_bytes, image_size_in_bytes;
	unsigned long image_size_in_pixels;
	long image_width, image_height;
	unsigned long items_read;
	double sqrt_image_size;
	mx_status_type mx_status;

	if ( frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}
	if ( marccd_filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The marccd_filename pointer passed was NULL." );
	}

#if MX_IMAGE_DEBUG_MARCCD
	MX_DEBUG(-2,("%s invoked for MarCCD file '%s'.",
		fname, marccd_filename));
#endif

	/* Try to open the file. */

	marccd_file = fopen( marccd_filename, "r" );

	if ( marccd_file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
	    "Cannot open MarCCD file '%s'.  Errno = %d, error message = '%s'",
			marccd_filename, saved_errno, strerror(saved_errno) );
	}

	/* Find out how big the file is. */

	marccd_fd = fileno(marccd_file);

	os_status = fstat( marccd_fd, &marccd_stat );

	if ( os_status < 0 ) {
		saved_errno = errno;

		fclose(marccd_file);

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot get the file status for MarCCD file '%s'.  "
		"Errno = %d, error message = '%s'",
			marccd_filename, saved_errno, strerror(saved_errno) );
	}

	file_size_in_bytes = (unsigned long) marccd_stat.st_size;

	/* Subtract 4096 bytes for the MarCCD header. */

	image_size_in_bytes = file_size_in_bytes - MX_MARCCD_HEADER_SIZE;

	image_size_in_pixels = image_size_in_bytes / 2L;

	/* FIXME: _If_ (!) the image is square, we can find the
	 * image dimensions by taking the square root of the
	 * image size.  What we should _really_ be doing is to
	 * fetch the image dimensions from the TIFF header.
	 */

#if MX_IMAGE_DEBUG_MARCCD
	MX_DEBUG(-2,("%s: image_size_in_pixels = %lu",
		fname, image_size_in_pixels));
#endif

	sqrt_image_size = sqrt( (double) image_size_in_pixels );

	image_width = mx_round( sqrt_image_size );

	image_height = image_width;

#if MX_IMAGE_DEBUG_MARCCD
	MX_DEBUG(-2,("%s: image dimensions = (%lu,%lu)",
		fname, image_width, image_height));
#endif

	if ( image_size_in_pixels != ( image_width * image_height ) ) {
		fclose(marccd_file);

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The computed image dimensions (%lu, %lu) do not match "
		"the total number of pixels (%lu) in the image '%s'.",
			image_width, image_height,
			image_size_in_pixels,
			marccd_filename );
	}
	
	/* Allocate an MX_IMAGE_FRAME with the right size for the image. */

	mx_status = mx_image_alloc( frame,
				image_width,
				image_height,
				MXT_IMAGE_FORMAT_GREY16,
				MX_DATAFMT_LITTLE_ENDIAN,
				2,
				MXT_IMAGE_HEADER_LENGTH_IN_BYTES,
				image_size_in_bytes );

	if ( mx_status.code != MXE_SUCCESS ) {
		fclose( marccd_file );

		return mx_status;
	}

	/* If setting up the image frame worked, then try to read the
	 * image data into the image_data field of ad->image_frame.
	 */

	/* First, move the file pointer to the start of image data. */

	os_status = fseek( marccd_file, MX_MARCCD_HEADER_SIZE, SEEK_SET );

	if ( os_status < 0 ) {
		saved_errno = errno;

		fclose(marccd_file);

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot seek to %ld bytes from the start of MarCCD file '%s'.  "
		"Errno = %d, error message = '%s'", MX_MARCCD_HEADER_SIZE,
			marccd_filename, saved_errno, strerror(saved_errno) );
	}

	/* Now read in the image data. */

	items_read = fread( (*frame)->image_data,
				image_size_in_bytes, 1,
				marccd_file );

	fclose(marccd_file);

	if ( items_read != 1 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An attempt to read %lu bytes from MarCCD file '%s' failed.",
			image_size_in_bytes, marccd_filename );
	}

	/* MarCCD image data is in little-endian order, so if the machine
	 * we are running on is _not_ little-endian, then we must byteswap
	 * the image data.
	 */

	if ( mx_native_byteorder() != MX_DATAFMT_LITTLE_ENDIAN ) {
		uint16_t *uint16_array;
		unsigned long i;

		uint16_array = (*frame)->image_data;

		for ( i = 0; i < image_size_in_pixels; i++ ) {
			uint16_array[i] = mx_16bit_byteswap( uint16_array[i] );
		}
	}

	/* Patch the timestamp in the header using the last modification
	 * time from the 'marccd_stat' structure that we read earlier.
	 */

	MXIF_TIMESTAMP_SEC(*frame)  = marccd_stat.st_mtime;
	MXIF_TIMESTAMP_NSEC(*frame) = 0;

#if MX_IMAGE_DEBUG_MARCCD
	MX_DEBUG(-2,("%s complete.",fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

/* Read EDF files from the SOLEIL SWING beamline. */

mx_status_type
mx_image_read_edf_file( MX_IMAGE_FRAME **frame, char *datafile_name )
{
	static const char fname[] = "mx_image_read_edf_file()";

	FILE *file;
	char buffer[100];
	char *ptr;
	int saved_errno, os_status, num_items;
	long framesize[2], binsize[2];
	long header_length, image_length, datafile_byteorder;
	long bytes_per_pixel, bytes_per_frame, bytes_read, image_format;
	char header_token[80];
	char header_token_format[20];
	double exposure_time;
	struct timespec exposure_timespec;
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
		fname, datafile_name));
#endif
	snprintf( header_token_format, sizeof(header_token_format),
			"%%*s %%*s %%%ds", (int) sizeof(header_token) - 1 );

	/* Open the data file. */

	file = fopen( datafile_name, "rb" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_NOT_FOUND, fname,
		"Cannot open EDF image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	/* The first line of an EDF file should be a '{' character
	 * followed by a line feed character.
	 */

	ptr = fgets( buffer, sizeof(buffer), file );

	if ( ptr == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot read the first line of EDF image file '%s'.  "
		"Errno = %d, error message = '%s'",
			datafile_name,
			saved_errno, strerror(saved_errno) );
	}

	if ( ( buffer[0] != '{' ) || ( buffer[1] != '\n' ) ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Data file '%s' does not appear to be an EDF file.",
			datafile_name );
	}

	/* Read in the rest of the header. */

	header_length = -1;
	image_length = -1;
	datafile_byteorder = -1;
	image_format = -1;
	bytes_per_pixel = -1;
	framesize[0] = -1;
	framesize[1] = -1;
	binsize[0] = -1;
	binsize[1] = -1;
	exposure_time = 1.0;

	for (;;) {
		ptr = fgets( buffer, sizeof(buffer), file );

#if MX_IMAGE_DEBUG
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif
		if ( ptr == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot read the next header line of "
			"EDF image file '%s'.  "
			"Errno = %d, error message = '%s'",
				datafile_name,
				saved_errno, strerror(saved_errno) );
		}

		if ( buffer[0] == '}' ) {
			/* We have reached the end of the text header,
			 * so break out of the for(;;) loop.
			 */

			break;
		} else
		if ( strncmp( buffer, "EDF_BinarySize =", 16 ) == 0 ) {
			num_items = sscanf(buffer,
					"%*s %*s %lu", &image_length);

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"EDF_BinarySize statement.",
					datafile_name, buffer );
			}
		} else
		if ( strncmp( buffer, "EDF_HeaderSize =", 16 ) == 0 ) {
			num_items = sscanf(buffer,
					"%*s %*s %lu", &header_length);

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"EDF_HeaderSize statement.",
					datafile_name, buffer );
			}
		} else
		if ( strncmp( buffer, "ByteOrder =", 11 ) == 0 ) {
			num_items = sscanf(buffer,
					header_token_format, header_token );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"ByteOrder statement.",
					datafile_name, buffer );
			}

			if ( strcmp( header_token, "LowByteFirst" ) == 0 ) {
				datafile_byteorder = MX_DATAFMT_LITTLE_ENDIAN;
			} else
			if ( strcmp( header_token, "HighByteFirst" ) == 0 ) {
				datafile_byteorder = MX_DATAFMT_BIG_ENDIAN;
			} else {
				return mx_error( MXE_UNSUPPORTED, fname,
				"The ByteOrder statement in the header of "
				"data file '%s' says that the data file "
				"byte order has the unrecognized value '%s'.",
					datafile_name, header_token );
			}
		} else
		if ( strncmp( buffer, "DataType =", 10 ) == 0 ) {
			num_items = sscanf(buffer,
					header_token_format, header_token );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"DataType statement.",
					datafile_name, buffer );
			}

			if ( strcmp( header_token, "UnsignedShort" ) == 0 ) {
				image_format = MXT_IMAGE_FORMAT_GREY16;
				bytes_per_pixel = 2;
			} else {
				return mx_error( MXE_UNSUPPORTED, fname,
				"The DataType statement in the header of "
				"data file '%s' says that the data type "
				"has the unrecognized value '%s'.",
					datafile_name, header_token );
			}
		} else
		if ( strncmp( buffer, "Dim_1 =", 7 ) == 0 ) {
			num_items = sscanf(buffer,
					"%*s %*s %lu", &framesize[0]);

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"Dim_1 statement.",
					datafile_name, buffer );
			}
		} else
		if ( strncmp( buffer, "Dim_2 =", 7 ) == 0 ) {
			num_items = sscanf(buffer,
					"%*s %*s %lu", &framesize[1]);

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"Dim_2 statement.",
					datafile_name, buffer );
			}
		} else
		if ( strncmp( buffer, "Xbin =", 6 ) == 0 ) {
			num_items = sscanf(buffer,
					"%*s %*s %lu", &binsize[0]);

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"Xbin statement.",
					datafile_name, buffer );
			}
		} else
		if ( strncmp( buffer, "Ybin =", 6 ) == 0 ) {
			num_items = sscanf(buffer,
					"%*s %*s %lu", &binsize[1]);

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Header line '%s' from data file '%s' "
				"appears to contain an incorrectly formatted "
				"Ybin statement.",
					datafile_name, buffer );
			}
		}
	}

#if MX_IMAGE_DEBUG
	MX_DEBUG(-2,("%s: header_length = %ld, image_length = %ld",
		fname, header_length, image_length));
	MX_DEBUG(-2,("%s: datafile_byteorder = %ld",
		fname, datafile_byteorder));
	MX_DEBUG(-2,("%s: image_format = %ld, bytes_per_pixel = %ld",
		fname, image_format, bytes_per_pixel));
	MX_DEBUG(-2,("%s: framesize = (%ld,%ld), binsize = (%ld,%ld)",
		fname, framesize[0], framesize[1], binsize[0], binsize[1]));
#endif
	if ( header_length < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The image header length was not found for EDF file '%s'.",
			datafile_name );
	}
	if ( image_length < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The image length was not found for EDF file '%s'.",
			datafile_name );
	}
	if ( datafile_byteorder < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The datafile byteorder was not found for EDF file '%s'.",
			datafile_name );
	}
	if ( image_format < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The image format was not found for EDF file '%s'.",
			datafile_name );
	}
	if ( bytes_per_pixel < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
	    "The number of bytes per pixel was not found for EDF file '%s'.",
			datafile_name );
	}
	if ( (framesize[0] < 0) || (framesize[1] < 0) ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The framesize was not found for EDF file '%s'.",
			datafile_name );
	}
	if ( (binsize[0] < 0) || (binsize[1] < 0) ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The binsize was not found for EDF file '%s'.",
			datafile_name );
	}

	bytes_per_frame = bytes_per_pixel * framesize[0] * framesize[1];

	/* Change the size of the MX_IMAGE_FRAME to match the EDF file. */

	mx_status = mx_image_alloc( frame,
					framesize[0],
					framesize[1],
					image_format,
					datafile_byteorder,
					(double) bytes_per_pixel,
					0,
					bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Add more information to the MX_IMAGE_FRAME header. */

	MXIF_ROW_BINSIZE(*frame)    = binsize[0];
	MXIF_COLUMN_BINSIZE(*frame) = binsize[1];

	exposure_timespec =
		mx_convert_seconds_to_high_resolution_time( exposure_time );

	MXIF_EXPOSURE_TIME_SEC(*frame)  = exposure_timespec.tv_sec;
	MXIF_EXPOSURE_TIME_NSEC(*frame) = exposure_timespec.tv_nsec;

	/* Move to the first byte after the header. */

	os_status = fseek( file, header_length, SEEK_SET );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An attempt to seek to the end of the header block "
		"in image file '%s' failed with errno = %d, "
		"error message = '%s'", datafile_name,
			saved_errno, strerror( saved_errno ) );
	}

	/* Read in the binary part of the image file. */

	bytes_read = (long) fread( (*frame)->image_data, sizeof(unsigned char),
				bytes_per_frame, file );

	if ( bytes_read < bytes_per_frame ) {
		if ( feof(file) ) {
			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
		"End of file at image byte offset %ld for EDF image file '%s'.",
				bytes_read, datafile_name );
		}
		if ( ferror(file) ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred at image byte offset %ld "
			"for SMV image file '%s'.",
				bytes_read, datafile_name );
		}
		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Only %ld image bytes were read from "
			"SMV image file '%s' when %ld bytes were expected.",
				bytes_read, datafile_name, bytes_per_frame );
	}

	/* Close the EDF image file. */

	fclose( file );

	/* If the byte order in the file does not match the native
	 * byte order of the machine we are running on, then we must
	 * byteswap the bytes in the image.
	 */

	if ( mx_native_byteorder() != datafile_byteorder ) {
		uint16_t *uint16_array;
		long i, words_per_frame;

		/* Byteswap the 16-bit integers. */

		uint16_array = (*frame)->image_data;

		words_per_frame = framesize[0] * framesize[1];

		for ( i = 0; i < words_per_frame; i++ ) {
			uint16_array[i] = mx_16bit_byteswap( uint16_array[i] );
		}

		MXIF_BYTE_ORDER(*frame) = mx_native_byteorder();
	}

	/* We are done, so return. */

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_sequence_get_exposure_time( MX_SEQUENCE_PARAMETERS *sp,
				long frame_number,
				double *exposure_time )
{
	static const char fname[] = "mx_sequence_get_exposure_time()";

	long i;
	double multiplier;

	if ( sp == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEQUENCE_PARAMETERS pointer passed was NULL." );
	}
	if ( exposure_time == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The exposure_time pointer passed was NULL." );
	}

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
		*exposure_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
	case MXT_SQ_STROBE:
		*exposure_time = sp->parameter_array[1];
		break;
	case MXT_SQ_STREAK_CAMERA:
		*exposure_time = sp->parameter_array[0]
					* sp->parameter_array[1];
		break;
	case MXT_SQ_GEOMETRICAL:
		*exposure_time = sp->parameter_array[1];

		multiplier = sp->parameter_array[3];

		for ( i = 1; i <= frame_number; i++ ) {
			(*exposure_time) *= multiplier;
		}
		break;
	case MXT_SQ_SUBIMAGE:
		*exposure_time = sp->parameter_array[2];

		multiplier = sp->parameter_array[4];

		for ( i = 1; i <= frame_number; i++ ) {
			(*exposure_time) *= multiplier;
		}
		break;
	default:
		*exposure_time = -1.0;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_sequence_get_frame_time( MX_SEQUENCE_PARAMETERS *sp,
				long frame_number,
				double *frame_time )
{
	static const char fname[] = "mx_sequence_get_frame_time()";

	long i;
	double exposure_time, gap_time;
	double exposure_multiplier, gap_multiplier;

	if ( sp == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEQUENCE_PARAMETERS pointer passed was NULL." );
	}
	if ( frame_time == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The frame_time pointer passed was NULL." );
	}

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
		*frame_time = sp->parameter_array[0];
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
		*frame_time = sp->parameter_array[2];
		break;
	case MXT_SQ_STROBE:
		*frame_time = sp->parameter_array[1];
		break;
	case MXT_SQ_STREAK_CAMERA:
		*frame_time = sp->parameter_array[0]
					* sp->parameter_array[1];
		break;
	case MXT_SQ_GEOMETRICAL:
		exposure_time       = sp->parameter_array[1];
		exposure_multiplier = sp->parameter_array[3];
		gap_multiplier      = sp->parameter_array[4];

		gap_time = sp->parameter_array[2] - exposure_time;

		for ( i = 1; i <= frame_number; i++ ) {
			exposure_time *= exposure_multiplier;
			gap_time      *= gap_multiplier;
		}

		*frame_time = exposure_time + gap_time;
		break;
	case MXT_SQ_SUBIMAGE:
		exposure_time       = sp->parameter_array[2];
		exposure_multiplier = sp->parameter_array[4];
		gap_multiplier      = sp->parameter_array[5];

		gap_time = sp->parameter_array[3] - exposure_time;

		for ( i = 1; i <= frame_number; i++ ) {
			exposure_time *= exposure_multiplier;
			gap_time      *= gap_multiplier;
		}

		*frame_time = exposure_time + gap_time;
		break;
	default:
		*frame_time = -1.0;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_sequence_get_sequence_time( MX_SEQUENCE_PARAMETERS *sp,
				long frame_number,
				double *sequence_time )
{
	static const char fname[] = "mx_sequence_get_sequence_time()";

	long i;
	double exposure_time, gap_time;
	double exposure_multiplier, gap_multiplier;
	double num_frames;

	if ( sp == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEQUENCE_PARAMETERS pointer passed was NULL." );
	}
	if ( sequence_time == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The sequence_time pointer passed was NULL." );
	}

	num_frames = 1.0 + (double) frame_number;

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
		*sequence_time = sp->parameter_array[0] * num_frames;
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
		*sequence_time = sp->parameter_array[2] * num_frames;
		break;
	case MXT_SQ_STROBE:
		*sequence_time = sp->parameter_array[1] * num_frames;
		break;
	case MXT_SQ_STREAK_CAMERA:
		*sequence_time = sp->parameter_array[0]
					* sp->parameter_array[1];
		break;
	case MXT_SQ_GEOMETRICAL:
		exposure_time       = sp->parameter_array[1];
		exposure_multiplier = sp->parameter_array[3];
		gap_multiplier      = sp->parameter_array[4];

		gap_time = sp->parameter_array[2] - exposure_time;

		*sequence_time = 0.0;

		for ( i = 1; i <= frame_number; i++ ) {
			exposure_time *= exposure_multiplier;
			gap_time      *= gap_multiplier;

			*sequence_time += (exposure_time + gap_time);
		}
		break;
	case MXT_SQ_SUBIMAGE:
		exposure_time       = sp->parameter_array[2];
		exposure_multiplier = sp->parameter_array[4];
		gap_multiplier      = sp->parameter_array[5];

		gap_time = sp->parameter_array[3] - exposure_time;

		*sequence_time = 0.0;

		for ( i = 1; i <= frame_number; i++ ) {
			exposure_time *= exposure_multiplier;
			gap_time      *= gap_multiplier;

			*sequence_time += (exposure_time + gap_time);
		}
		break;
	default:
		*sequence_time = -1.0;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_sequence_get_num_frames( MX_SEQUENCE_PARAMETERS *sp,
				long *num_frames )
{
	static const char fname[] = "mx_sequence_get_num_frames()";

	if ( sp == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEQUENCE_PARAMETERS pointer passed was NULL." );
	}
	if ( num_frames == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_frames pointer passed was NULL." );
	}

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
	case MXT_SQ_STREAK_CAMERA:
	case MXT_SQ_SUBIMAGE:
		*num_frames = 1;
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
	case MXT_SQ_STROBE:
	case MXT_SQ_DURATION:
	case MXT_SQ_GEOMETRICAL:
		*num_frames = mx_round( sp->parameter_array[0] );
		break;
	default:
		*num_frames = -1;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

