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
mxp_get_rgb565_pixel( char *pixel_ptr,
			unsigned long *R,
			unsigned long *G,
			unsigned long *B )
{
	uint16_t pixel_value;

	pixel_value = *( (uint16_t *) pixel_ptr);

#if 0
	MX_DEBUG(-2,("pixel_value = %hu", (unsigned short) pixel_value));
#endif

	/* FIXME: We must take care of byteorder issues here. */

	*R = ( pixel_value >> 11 ) & 0x1f;
	*G = ( pixel_value >> 5 ) & 0x3f;
	*B = pixel_value & 0x1f;
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
	case MX_IMAGE_FILE_PNM:
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
	void (*rgb_pixel_fn) ( char *,
		unsigned long *, unsigned long *, unsigned long * );
	char *ptr;
	unsigned long R, G, B;
	long i;
	int saved_errno;
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
	case MX_IMAGE_FORMAT_RGB565:
		rgb_pixel_fn = mxp_get_rgb565_pixel;
		pixel_length = 2;			/* 2 bytes - 16 bits */
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

	fprintf( file, "P3\n" );
	fprintf( file, "# %s\n", datafile_name );
	fprintf( file, "%lu %lu\n", frame->framesize[0], frame->framesize[1] );
	fprintf( file, "65535\n" );

	/* Loop through the image file. */

	ptr = frame->image_data;

	for ( i = 0; i < frame->image_length; i += pixel_length ) {

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

