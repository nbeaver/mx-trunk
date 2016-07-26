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

#define LIBTIFF_MODULE_DEBUG_READ	TRUE

#define LIBTIFF_MODULE_DEBUG_WRITE	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Include file from libtiff. */
#include "tiffio.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_version.h"
#include "mx_time.h"
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
				NULL, NULL );

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
	long row, row_width, column_height, bits_per_pixel;
	long scanline_size_in_bytes;
	int tiff_status;
	char *scanline_buffer = NULL;
	char *image_data_ptr = NULL;

	char mx_version_string[80];

	char timestamp[25];
	time_t time_in_seconds;
	struct tm tm_struct;

	double exposure_seconds;

	uint64_t exif_dir_offset;

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

#if LIBTIFF_MODULE_DEBUG_WRITE
	MX_DEBUG(-2,("%s invoked for datafile '%s'.", fname, datafile_name));
#endif

	tiff = TIFFOpen( datafile_name, "w" );

	/* Copy everything from the MX header into the TIFF header. */

	row_width = MXIF_ROW_FRAMESIZE(frame);

	if (! TIFFSetField( tiff, TIFFTAG_IMAGEWIDTH, row_width ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set ImageWidth TIFF tag." );
	}

	column_height = MXIF_COLUMN_FRAMESIZE(frame);

	if (! TIFFSetField( tiff, TIFFTAG_IMAGELENGTH, column_height ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set ImageLength TIFF tag." );
	}

	/* Currently we do not support multicomponent images like RGB. */

	if (! TIFFSetField( tiff, TIFFTAG_SAMPLESPERPIXEL, 1 ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set SamplesPerPixel TIFF tag." );
	}

	/* Save the pixel bit size.  The real dynamic range is often
	 * smaller than this.
	 */

	bits_per_pixel = MXIF_BITS_PER_PIXEL(frame);

	if (! TIFFSetField( tiff, TIFFTAG_BITSPERSAMPLE, 16 ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set SamplesPerPixel TIFF tag." );
	}

	/* Construct a TIFF-compatible version of the MX timestamp.
	 * Unfortunately, the TIFF tag does not support fractions of a
	 * second, and there is no room in the tag for more characters.
	 */

	time_in_seconds = MXIF_TIMESTAMP_SEC(frame);

#if LIBTIFF_MODULE_DEBUG_WRITE
	MX_DEBUG(-2,("%s: frame timestamp = (%lu,%lu)", fname,
		MXIF_TIMESTAMP_SEC(frame), MXIF_TIMESTAMP_NSEC(frame) ));

	MX_DEBUG(-2,("%s: time_in_seconds = %lu", fname, time_in_seconds));
#endif

	(void) localtime_r( &time_in_seconds, &tm_struct );

	strftime( timestamp, sizeof(timestamp),
		"%Y:%m:%d %H:%M:%S", &tm_struct );

#if LIBTIFF_MODULE_DEBUG_WRITE
	MX_DEBUG(-2,("%s: timestamp = '%s'", fname, timestamp));
#endif

	if (! TIFFSetField( tiff, TIFFTAG_DATETIME, timestamp ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set DateTime TIFF tag." );
	}

	/* The following values are always the same for MX. */

	if (! TIFFSetField( tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set Orientation TIFF tag." );
	}

	if (! TIFFSetField( tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG ) ){
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set PlanarConfiguration TIFF tag." );
	}

	if (! TIFFSetField( tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK ))
	{
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set PhotometricInterpretation TIFF tag." );
	}

	/* MX software version. */

	snprintf( mx_version_string, sizeof(mx_version_string),
		"MX %d.%d.%d (%s) (%s)",
		mx_get_major_version(),
		mx_get_minor_version(),
		mx_get_update_version(),
		mx_get_version_date_string(),
		mx_get_revision_string() );


	if (! TIFFSetField( tiff, TIFFTAG_SOFTWARE, mx_version_string ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set Software TIFF tag." );
	}

	/* Missing MX fields: binsize, bias offset. */

	/* FIXME: Things we might want to add later.
 	 *
 	 * Baseline:
 	 *   Artist, HostComputer, ResolutionUnit, XResolution, YResolution
 	 *
 	 * Extension:
 	 *   ImageID, PageName, PageNumber, SMaxSampleValue, SMinSampleValue,
 	 *   XPosition, YPosition
 	 */

	/* Write the image data to the file. */

	scanline_size_in_bytes = TIFFScanlineSize( tiff );

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

	/* Unfortunately, the only place we have to put the exposure time
	 * information is into a separate EXIF directory, so we must do
	 * that now.
	 */

	/* Close the first IFD. */

	if (! TIFFWriteDirectory( tiff ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"TIFFWriteDirectory() failed for the first IFD." );
	}

	/* Create a new EXIF directory. */

	tiff_status = TIFFCreateEXIFDirectory( tiff );

	if ( tiff_status != 0 ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"TIFFCreateEXIFDirectory() failed for the first IFD." );
	}

	/* Save the exposure time. */

#if 0
	MX_DEBUG(-2,("%s: MXIF_EXPOSURE_TIME_SEC() = %lu",
		fname, MXIF_EXPOSURE_TIME_SEC(frame) ));
	MX_DEBUG(-2,("%s: MXIF_EXPOSURE_TIME_NSEC() = %lu",
		fname, MXIF_EXPOSURE_TIME_NSEC(frame) ));
#endif

	exposure_seconds = MXIF_EXPOSURE_TIME_SEC(frame)
			+ 1.0e-9 * MXIF_EXPOSURE_TIME_NSEC(frame);

#if LIBTIFF_MODULE_DEBUG_WRITE
	MX_DEBUG(-2,("%s: exposure_seconds = %f", fname, exposure_seconds));
#endif

	if (! TIFFSetField( tiff, EXIFTAG_EXPOSURETIME, exposure_seconds ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set EXIFTAG_EXPOSURETIME tag." );
	}

	/* Close the custom EXIF directory. */

	if (! TIFFWriteCustomDirectory( tiff, &exif_dir_offset ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"TIFFWriteCustomDirectory() failed for the EXIF IFD." );
	}

	/* Go back to the first directory and add the EXIF IFD pointer. */

	if (! TIFFSetDirectory( tiff, 0 ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"The attempt to use TIFFSetDirectory() to go back to the "
		"first IFD failed." );
	}

	if (! TIFFSetField( tiff, TIFFTAG_EXIFIFD, exif_dir_offset ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"The attempt to save the EXIF IFD offset failed." );
	}

	/* We are done, so close down everything. */

	TIFFClose( tiff );

	if ( scanline_buffer != NULL ) {
		_TIFFfree( scanline_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/
