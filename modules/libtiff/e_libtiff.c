/*
 * Name:    e_libtiff.c
 *
 * Purpose: MX libtiff extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define LIBTIFF_MODULE_DEBUG_INITIALIZE	TRUE

#define LIBTIFF_MODULE_DEBUG_FINALIZE	TRUE

#include <stdio.h>
#include <stdlib.h>

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
						&libtiff_library );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if LIBTIFF_MODULE_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s: libtiff library name = '%s', libtiff_library = %p",
		fname, MXP_LIBTIFF_LIBRARY_NAME, libtiff_library));
#endif

	libtiff_ext->libtiff_library = libtiff_library;

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
				scanline_size * image_length );

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

	MX_DEBUG(-2,("%s invoked.", fname));

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented." );
}

/*------*/
