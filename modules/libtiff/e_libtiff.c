/*
 * Name:    e_libtiff.c
 *
 * Purpose: MX libtiff extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define LIBTIFF_MODULE_DEBUG_INITIALIZE	TRUE

#define LIBTIFF_MODULE_DEBUG_FINALIZE	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Include file from libtiff. */
#include "tiffio.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_module.h"
#include "mx_image.h"
#include "e_libtiff.h"

MX_EXTENSION_FUNCTION_LIST mxext_libtiff_extension_function_list = {
	mxext_libtiff_initialize,
};

#if defined(OS_WIN32)
#  define MXP_LIBTIFF_LIBRARY_NAME	"libtiff.dll"
#else
#  define MXP_LIBTIFF_LIBRARY_NAME	"libtiff.so"
#endif

/*------*/

static void
mxext_libtiff_error_handler( const char *module, const char *fmt, ... )
{
	va_list va_alist;
	char buffer[2500];

	va_start( va_alist, fmt );
	vsnprintf( buffer, sizeof(buffer), fmt, va_alist );
	va_end( va_alist );

	if ( module != NULL ) {
		fprintf( stderr, "%s: %s\n", module, buffer );
	} else {
		fprintf( stderr, "%s\n", buffer );
	}
}

/*------*/

MX_EXPORT mx_status_type
mxext_libtiff_initialize( MX_EXTENSION *extension )
{
	static const char fname[] = "mxext_libtiff_initialize()";

	MX_LIBTIFF_EXTENSION_PRIVATE *libtiff_ext;
	MX_DYNAMIC_LIBRARY *libtiff_library;
	mx_status_type mx_status;

	libtiff_ext = (MX_LIBTIFF_EXTENSION_PRIVATE *)
			malloc( sizeof(MX_LIBTIFF_EXTENSION_PRIVATE) );

	if ( libtiff_ext == (MX_LIBTIFF_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_LIBTIFF_EXTENSION_PRIVATE structure." );
	}

	extension->ext_private = libtiff_ext;

	/* Find and save a copy of the MX_DYNAMIC_LIBRARY pointer for
	 * the libtiff library where other MX routines can find it.
	 * Then, we can use mx_dynamic_library_get_symbol_pointer()
	 * to look for individual libtiff library functions later.
	 */

	mx_status = mx_dynamic_library_open( MXP_LIBTIFF_LIBRARY_NAME,
						&libtiff_library, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if LIBTIFF_MODULE_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s: libtiff library name = '%s', libtiff_library = %p",
		fname, MXP_LIBTIFF_LIBRARY_NAME, libtiff_library));
#endif

	libtiff_ext->libtiff_library = libtiff_library;

	/* Setup error and warning handlers */

	TIFFSetErrorHandler( (TIFFErrorHandler) mxext_libtiff_error_handler );

	TIFFSetWarningHandler( (TIFFErrorHandler) mxext_libtiff_error_handler );

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_libtiff_read_tiff_file( MX_IMAGE_FRAME **image_frame,
				char *datafile_name )
{
	static const char fname[] = "mxext_libtiff_read_tiff_file()";

	TIFF *tif = NULL;
	uint32 image_length;
	tsize_t scanline_size; 
	tdata_t buffer;
	uint32 row;
	uint32 col;
	uint16_t *uint16_buffer;

	uint16_t *uint16_image_array;
	long image_format;
	double bytes_per_pixel;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_breakpoint();

	tif = TIFFOpen( datafile_name, "r" );

	if ( tif == (TIFF *) NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to read TIFF file '%s' failed.",
			datafile_name );
	}

	/* Figure out how big the TIFF file is. */

	TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &image_length );

	scanline_size = TIFFScanlineSize( tif );
	buffer = _TIFFmalloc( scanline_size );

	uint16_buffer = buffer;

	/* Create an MX_IMAGE_FRAME structure to copy the TIFF into. */

	image_format = MXT_IMAGE_FORMAT_GREY16;
	bytes_per_pixel = 2.0;

	mx_status = mx_image_alloc( image_frame,
				scanline_size,
				image_length,
				image_format,
				mx_native_byteorder(),
				bytes_per_pixel, 0,
				scanline_size * image_length,
				NULL );

	if ( mx_status.code != MXE_SUCCESS ) {
		_TIFFfree( buffer );
		TIFFClose( tif );
		return mx_status;
	}

	uint16_image_array = (*image_frame)->image_data;

	/* Now copy the contents to the MX_IMAGE_FRAME structure. */

	for ( row = 0; row < image_length; row++ ) {
		TIFFReadScanline( tif, buffer, row, 0 );

		for ( col = 0; col < scanline_size; col++ ) {
			uint16_image_array[ (row * scanline_size) + col ]
				= uint16_buffer[ col ];
		}
	}

	_TIFFfree( buffer );
	TIFFClose( tif );

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_libtiff_write_tiff_file( MX_IMAGE_FRAME *frame,
				char *datafile_name )
{
	static const char fname[] = "mxext_libtiff_write_tiff_file()";

	TIFF *tiff = NULL;
	long row, row_width, column_height;
	long scanline_size_in_bytes;
	int tiff_status;
	char *scanline_buffer = NULL;
	char *image_data_ptr = NULL;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}
	if ( datafile_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The datafile_name pointer passed was NULL." );
	}
	if ( frame->image_data == NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"MX_IMAGE_FRAME %p for datafile '%s' does not yet have "
		"any image data.", frame, datafile_name );
	}

	MX_DEBUG(-2,("%s invoked for datafile '%s'.", fname, datafile_name));

	/* mx_breakpoint(); */

	tiff = TIFFOpen( datafile_name, "w" );

	MX_DEBUG(-2,("%s: tiff = %p", fname, tiff));

	/* Set up a small header. */

	row_width = MXIF_ROW_FRAMESIZE(frame);
	column_height = MXIF_COLUMN_FRAMESIZE(frame);

	TIFFSetField( tiff, TIFFTAG_IMAGEWIDTH, row_width );
	TIFFSetField( tiff, TIFFTAG_IMAGELENGTH, column_height );
	TIFFSetField( tiff, TIFFTAG_SAMPLESPERPIXEL, 1 );
	TIFFSetField( tiff, TIFFTAG_BITSPERSAMPLE, 16 );
	TIFFSetField( tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT );
	TIFFSetField( tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
	TIFFSetField( tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK );

	/* Write image data to the file. */

	scanline_size_in_bytes = TIFFScanlineSize( tiff );

	MX_DEBUG(-2,("%s: scanline_size_in_bytes = %ld",
		fname, scanline_size_in_bytes));

	scanline_buffer = _TIFFmalloc( scanline_size_in_bytes );

	image_data_ptr = frame->image_data;

	for ( row = 0; row < column_height; row++ ) {
		memcpy( scanline_buffer,
			image_data_ptr,
			scanline_size_in_bytes );

		tiff_status = TIFFWriteScanline(tiff, scanline_buffer, row, 0);

		if ( tiff_status <= 0 ) {
			break;
		}

		image_data_ptr += scanline_size_in_bytes;
	}

	/* We are done, so close down everything. */

	TIFFClose( tiff );

	if ( scanline_buffer != NULL ) {
		_TIFFfree( scanline_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/
