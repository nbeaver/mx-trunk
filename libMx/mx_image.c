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

static void
mxp_get_rgb565_pixel( unsigned char *pixel_ptr,
			unsigned long *R,
			unsigned long *G,
			unsigned long *B )
{
	uint16_t pixel_value;

	pixel_value = *( (uint16_t *) pixel_ptr);

	/* FIXME: We must take care of byteorder issues here. */

	*R = ( pixel_value >> 11 ) & 0x1f;
	*G = ( pixel_value >> 5 ) & 0x3f;
	*B = pixel_value & 0x1f;

	return;
}

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

/* FIXME: The following treats the pixels as two independent bytes per pixel
 * rather than as a group of four bytes that generate two pixels.  The 
 * symptom of this is that the colormap in the generated image is wrong.
 */

static void
mxp_get_yuyv_pixel( unsigned char *pixel_ptr,
			unsigned long *R,
			unsigned long *G,
			unsigned long *B )
{
	unsigned int Y0, Y1, Cb, Cr;
	double R_temp, G_temp, B_temp;
	double Y1_temp, Cb_temp, Cr_temp;

	Y0 = pixel_ptr[0];
	Cb = pixel_ptr[1];
	Y1 = pixel_ptr[2];
	Cr = pixel_ptr[3];

#if 0
	MX_DEBUG(-2,("Y0 = %u, Cb = %u, Y1 = %u, Cr = %u", Y0, Cb, Y1, Cr));
#endif

	Y1_temp = (255 / 219.0) * ((int)Y1 - 16);
	Cb_temp = (255 / 224.0) * ((int)Cb - 128);
	Cr_temp = (255 / 224.0) * ((int)Cr - 128);

#if 0
	MX_DEBUG(-2,("Y1_temp = %g, Cb_temp = %g, Cr_temp = %g",
			Y1_temp, Cb_temp, Cr_temp));
#endif

	R_temp = 1.0 * Y1_temp + 0     * Cb_temp + 1.402 * Cr_temp;
	G_temp = 1.0 * Y1_temp - 0.344 * Cb_temp + 0.714 * Cr_temp;
	B_temp = 1.0 * Y1_temp + 1.772 * Cb_temp + 0     * Cr_temp;

#if 0
	MX_DEBUG(-2,("R_temp = %g, G_temp = %g, B_temp = %g",
		R_temp, G_temp, B_temp));
#endif

	*R = clamp( R_temp );
	*G = clamp( G_temp );
	*B = clamp( B_temp );

	return;
}

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
	void (*rgb_pixel_fn) ( unsigned char *,
		unsigned long *, unsigned long *, unsigned long * );
	unsigned char *ptr;
	unsigned long R, G, B;
	long i;
	int saved_errno;
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
		rgb_pixel_fn = mxp_get_rgb565_pixel;
		pixel_length = 2;			/* 2 bytes - 16 bits */
		break;
	case MXT_IMAGE_FORMAT_YUYV:
		rgb_pixel_fn = mxp_get_yuyv_pixel;
		pixel_length = 2;	/* 4 bytes per 2 pixels - 16 bits */
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image format %ld requested for datafile '%s'.",
			frame->image_format, datafile_name );
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

	maxint = 255;

	fprintf( file, "P3\n" );
	fprintf( file, "# %s\n", datafile_name );
	fprintf( file, "%lu %lu\n", frame->framesize[0], frame->framesize[1] );
	fprintf( file, "%u\n", maxint );

	/* Loop through the image file. */

	ptr = frame->image_data;

	for ( i = 0; i < frame->image_length; i += pixel_length ) {

#if 0
		if ( i >= 50 )
			break;
#endif

		rgb_pixel_fn( ptr, &R, &G, &B );

		if ( i < 50 ) {
			MX_DEBUG(-2,("%s: i = %lu, R = %lu, G = %lu, B = %lu",
			fname, i, R, G, B));
		}

		fprintf( file, "%lu %lu %lu\n", R, G, B );

		ptr += pixel_length;
	}

	fclose( file );

	return MX_SUCCESSFUL_RESULT;
}

