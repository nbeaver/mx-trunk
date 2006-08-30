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
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_image.h"

typedef struct {
	int num_source_bytes;
	int num_destination_bytes;
	void (*converter_fn)( unsigned char *src, unsigned char *dest );

} pixel_converter_t;

static void
mxp_rgb565_converter_fn( unsigned char *src, unsigned char *dest )
{
	uint16_t pixel_value;

	pixel_value = *( (uint16_t *) src);

	/* FIXME: We must take care of byteorder issues here. */

	dest[0] = ( pixel_value >> 11 ) & 0x1f;
	dest[1] = ( pixel_value >> 5 ) & 0x3f;
	dest[2] = pixel_value & 0x1f;

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

/* FIXME - This is lame and very inefficient. */

static void
mxp_grey16_converter_fn( unsigned char *src, unsigned char *dest )
{
	uint16_t grey16, *dest16_ptr;

	grey16 = *( (uint16_t *) src );

	dest16_ptr = (uint16_t *) dest;

	*dest16_ptr = grey16;
}

static pixel_converter_t
mxp_grey16_converter = { 2, 2, mxp_grey16_converter_fn };

/*----*/

MX_EXPORT mx_status_type
mx_write_image_file( MX_IMAGE_FRAME *frame,
			unsigned long datafile_type,
			void *datafile_args,
			char *datafile_name )
{
	static const char fname[] = "mx_write_image_file()";

	mx_status_type mx_status;

	switch( datafile_type ) {
	case MXT_IMAGE_FILE_PNM:
		mx_status = mx_write_pnm_image_file( frame, datafile_name );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image file type %lu requested for datafile '%s'.",
			datafile_type, datafile_name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_write_pnm_image_file( MX_IMAGE_FRAME *frame, char *datafile_name )
{
	static const char fname[] = "mx_write_pnm_image_file()";

	FILE *file;
	pixel_converter_t converter;
	void (*converter_fn)( unsigned char *, unsigned char * );
	int src_step, dest_step;
	unsigned char *src;
	unsigned char dest[10];
	unsigned char R, G, B;
	uint16_t grey16_pixel;
	int pnm_type, saved_errno;
	long i;
	unsigned int maxint;
	size_t pixel_length;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

	if ( datafile_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The datafile_name pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for datafile '%s'.",
		fname, datafile_name ));

	MX_DEBUG(-2,("%s: image_type = %ld, width = %ld, height = %ld",
		fname, frame->image_type,
		frame->framesize[0], frame->framesize[1] ));
	MX_DEBUG(-2,("%s: image_format = %ld, pixel_order = %ld",
		fname, frame->image_format, frame->pixel_order));
	MX_DEBUG(-2,("%s: image_length = %lu, image_data = %p",
		fname, (unsigned long) frame->image_length, frame->image_data));

	switch( frame->image_format ) {
	case MXT_IMAGE_FORMAT_RGB565:
		converter = mxp_rgb565_converter;
		pnm_type = 3;
		maxint = 255;
		break;
	case MXT_IMAGE_FORMAT_YUYV:
		converter = mxp_yuyv_converter;
		pnm_type = 3;
		maxint = 255;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		converter = mxp_grey16_converter;
		pnm_type = 2;
		maxint = 65535;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image format %ld requested for datafile '%s'.",
			frame->image_format, datafile_name );
	}

	src_step     = converter.num_source_bytes;
	dest_step    = converter.num_destination_bytes;
	converter_fn = converter.converter_fn;

	if ( dest_step > sizeof(dest) ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"You must increase the size of the dest array "
			"to %d and then recompile MX.",
				dest_step );
	}

	file = fopen( datafile_name, "w" );

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
		if ( i >= 50 )
			break;
#endif

		converter_fn( &src[i], dest );

		switch( frame->image_format ) {

		case MXT_IMAGE_FORMAT_RGB565:
		case MXT_IMAGE_FORMAT_YUYV:
			R = dest[0];
			G = dest[1];
			B = dest[2];

			if ( i < 50 ) {
				MX_DEBUG(-2,
				("%s: i = %lu, R = %lu, G = %lu, B = %lu",

				fname, i, R, G, B));
			}

			fprintf( file, "%lu %lu %lu\n", R, G, B );
			break;

		case MXT_IMAGE_FORMAT_GREY16:
			grey16_pixel = *((uint16_t *) dest);

			if ( i < 50 ) {
				MX_DEBUG(-2,("%s: i = %lu, grey16 = %lu",
					fname, i,
					(unsigned long) grey16_pixel));
			}

			fprintf( file, "%lu ",
					(unsigned long) grey16_pixel );

			if ( ((i+1) % 5) == 0 ) {
				fprintf( file, "\n" );
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Unsupported image format %ld requested "
				"for datafile '%s'.",
				frame->image_format, datafile_name );
		}
	}

	fclose( file );

	MX_DEBUG(-2,
	("%s: PNM file '%s' successfully written.", fname, datafile_name ));

	return MX_SUCCESSFUL_RESULT;
}

