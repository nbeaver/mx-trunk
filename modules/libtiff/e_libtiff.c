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
#include "mx_driver.h"
#include "mx_bit.h"
#include "mx_version.h"
#include "mx_time.h"
#include "mx_socket.h"
#include "mx_variable.h"
#include "mx_module.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_video_input.h"
#include "e_libtiff.h"

/* FIXME: The following definition of strptime() should not be necessary. */

#if defined(OS_LINUX)
extern char *strptime( const char *, const char *, struct tm * );
#endif

MX_EXTENSION_FUNCTION_LIST mxext_libtiff_extension_function_list = {
	mxext_libtiff_initialize,
};

#if defined(OS_WIN32)
#  define MXP_LIBTIFF_LIBRARY_NAME	"libtiff.dll"
#else
#  define MXP_LIBTIFF_LIBRARY_NAME	"libtiff.so"
#endif

/*------*/

static char mx_tiff_site_name[256] = { '\0' };
static char mx_tiff_hostname[MXU_HOSTNAME_LENGTH+1] = { '\0' };

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

	TIFF *tiff = NULL;
	uint16_t row_width, column_height, samples_per_pixel;
	uint16_t bits_per_sample;
	tsize_t scanline_size_in_bytes; 
	tdata_t scanline_buffer;
	unsigned long row;
	int tiff_status;

	char *ptr;
	struct tm tm_struct;
	char tiff_datetime_buffer[25];
	time_t time_in_seconds;

	uint32_t exif_offset;
	double exposure_time;
	uint32_t exposure_seconds, exposure_nanoseconds;

	uint16_t *image_data_ptr;
	long image_format;
	double bytes_per_pixel;
	size_t image_size_in_bytes;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	tiff = TIFFOpen( datafile_name, "r" );

	if ( tiff == (TIFF *) NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to read TIFF file '%s' failed.",
			datafile_name );
	}

	/* Figure out how big the TIFF file is. */

	if (! TIFFGetField( tiff, TIFFTAG_IMAGEWIDTH, &row_width ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot read the ImageLength TIFF tag." );
	}

	if (! TIFFGetField( tiff, TIFFTAG_IMAGELENGTH, &column_height ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot read the ImageLength TIFF tag." );
	}

	if (! TIFFGetField( tiff, TIFFTAG_SAMPLESPERPIXEL,
			&samples_per_pixel ) )
	{
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot read the ImageLength TIFF tag." );
	}

	if ( samples_per_pixel != 1 ) {
		TIFFClose( tiff );
		return mx_error( MXE_UNSUPPORTED, fname,
		"TIFF image '%s' is a multicomponent image with %lu "
		"samples per pixel.  Currently MX only supports reading "
		"TIFF files with one sample per pixel.",
			datafile_name,
			(unsigned long) samples_per_pixel );
	}

	if (! TIFFGetField( tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot read the BitsPerSample TIFF tag." );
	}

	if ( bits_per_sample != 16 ) {
		TIFFClose( tiff );
		return mx_error( MXE_UNSUPPORTED, fname,
		"TIFF image '%s' has %lu bits per sample.  However, MX "
		"currently only supports reading images with 16 bits "
		"per sample.", datafile_name, (unsigned long) bits_per_sample );
	}

	/* Read and parse the timestamp in the TIFF image. */

	if (! TIFFGetField( tiff, TIFFTAG_DATETIME, tiff_datetime_buffer ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot read the DateTime TIFF tag." );
	}

	ptr = strptime( tiff_datetime_buffer, "%Y:%m:%d %H:%M:%S", &tm_struct );

	if ( ptr == NULL ) {
		TIFFClose( tiff );
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"TIFF image '%s' does not have a DateTime timestamp.",
			datafile_name );
	}

	time_in_seconds = mktime( &tm_struct );

	/*----*/

	scanline_size_in_bytes = TIFFScanlineSize( tiff );
	scanline_buffer = _TIFFmalloc( scanline_size_in_bytes );

	/* Create an MX_IMAGE_FRAME structure to copy the TIFF into. */

	image_format = MXT_IMAGE_FORMAT_GREY16;
	bytes_per_pixel = 2.0;
	image_size_in_bytes = 2 * row_width * column_height;

	mx_status = mx_image_alloc( image_frame,
				row_width,
				column_height,
				image_format,
				mx_native_byteorder(),
				bytes_per_pixel,
				MXT_IMAGE_HEADER_LENGTH_IN_BYTES,
				image_size_in_bytes,
				NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS ) {
		_TIFFfree( scanline_buffer );
		TIFFClose( tiff );
		return mx_status;
	}

	image_data_ptr = (*image_frame)->image_data;

	/* Now copy the contents to the MX_IMAGE_FRAME structure. */

	for ( row = 0; row < column_height; row++ ) {
		tiff_status = TIFFReadScanline( tiff, scanline_buffer, row, 0 );

		if ( tiff_status <= 0 ) {
			break;
		}

		memcpy( image_data_ptr,
			scanline_buffer,
			scanline_size_in_bytes );

		image_data_ptr += scanline_size_in_bytes;
	}

	_TIFFfree( scanline_buffer );

	MXIF_TIMESTAMP_SEC( (*image_frame) ) = time_in_seconds;
	MXIF_TIMESTAMP_NSEC( (*image_frame) ) = 0;

	/* If present, the exposure time is found in the EXIF directory. */

	if (! TIFFGetField( tiff, TIFFTAG_EXIFIFD, &exif_offset ) ) {
		TIFFClose( tiff );
		MXIF_EXPOSURE_TIME_SEC( (*image_frame) ) = 1;
		MXIF_EXPOSURE_TIME_NSEC( (*image_frame) ) = 0;
		return MX_SUCCESSFUL_RESULT;
	}

	if (! TIFFReadEXIFDirectory( tiff, exif_offset ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_NOT_FOUND, fname,
		"TIFF image file '%s' is supposed to have an EXIF IFD "
		"at offset %lu, but no EXIF IFD was found there.",
			datafile_name, (unsigned long) exif_offset );
	}

	if (! TIFFGetField( tiff, EXIFTAG_EXPOSURETIME, &exposure_time ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot read the ExposureTime EXIF TIFF tag." );
	}

	exposure_seconds = (unsigned long) exposure_time;

	exposure_nanoseconds =
		mx_round( 1.0e9 * ( exposure_time - exposure_seconds ) );

	MXIF_EXPOSURE_TIME_SEC( (*image_frame) ) = exposure_seconds;
	MXIF_EXPOSURE_TIME_NSEC( (*image_frame) ) = exposure_nanoseconds;

	TIFFClose( tiff );

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

	long sample_format;

	double exposure_seconds;

	uint64_t exif_dir_offset;

	MX_AREA_DETECTOR *ad = NULL;
	MX_VIDEO_INPUT *vinput = NULL;

	mx_status_type mx_status;

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
	/* See if there is an MX_RECORD pointer in this MX_IMAGE_FRAME. */

	if ( frame->record != (MX_RECORD *) NULL ) {
		switch( frame->record->mx_class ) {
		case MXC_AREA_DETECTOR:
			ad = (MX_AREA_DETECTOR *)
					frame->record->record_class_struct;
			break;
		case MXC_VIDEO_INPUT:
			vinput = (MX_VIDEO_INPUT *)
					frame->record->record_class_struct;
			break;
		default:
			/* If this is not an area detector or video input
			 * record, then do nothing.
			 */
			break;
		}
	}

	MXW_UNUSED( vinput );

	/* Open the TIFF image for writing. */

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

	MXW_UNUSED( bits_per_pixel );

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
		(unsigned long) MXIF_TIMESTAMP_SEC(frame),
		(unsigned long) MXIF_TIMESTAMP_NSEC(frame) ));

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

	/* Make the entire image be a single TIFF strip by setting the
	 * rows per strip to the total number of rows in the image.
	 */

	if (! TIFFSetField( tiff, TIFFTAG_ROWSPERSTRIP, column_height )) {
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

	/* If present, store the pixel resolution and pixel resolution units. */

	if ( ad != NULL ) {
#if 1
	    if (1)
#else
	    if ( ( mx_difference( ad->resolution[0], 0.0 ) >= 1.0e-10 )
	      || ( mx_difference( ad->resolution[1], 0.0 ) >= 1.0e-10 ) )
#endif
	    {

		if (! TIFFSetField( tiff, TIFFTAG_XRESOLUTION,
				ad->resolution[0] ) )
		{
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Cannot set XResolution TIFF tag." );
		}

		if (! TIFFSetField( tiff, TIFFTAG_YRESOLUTION,
				ad->resolution[1] ) )
		{
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Cannot set YResolution TIFF tag." );
		}

		/* ResolutionUnit is specified as 'centimeters' (3). */

		if (! TIFFSetField( tiff, TIFFTAG_RESOLUTIONUNIT, 3 ) ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Cannot set ResolutionUnit TIFF tag." );
		}
	    }

	    switch( ad->image_format ) {
	    case MXT_IMAGE_FORMAT_GREY8:
	    case MXT_IMAGE_FORMAT_GREY16:
	    case MXT_IMAGE_FORMAT_GREY32:
		sample_format = SAMPLEFORMAT_UINT;
		break;
	    case MXT_IMAGE_FORMAT_INT32:
		sample_format = SAMPLEFORMAT_INT;
		break;
	    case MXT_IMAGE_FORMAT_FLOAT:
	    case MXT_IMAGE_FORMAT_DOUBLE:
		sample_format = SAMPLEFORMAT_IEEEFP;
		break;
	    default:
		sample_format = SAMPLEFORMAT_VOID;
		break;
	    }

	    if (! TIFFSetField( tiff, TIFFTAG_SAMPLEFORMAT, sample_format ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set SampleFormat TIFF tag." );
	    }
	}

	/* Save a copy of the name of this computer. */

	if ( mx_tiff_hostname[0] == '\0' ) {
	    mx_gethostname( mx_tiff_hostname, sizeof(mx_tiff_hostname) );
	}

	if ( mx_tiff_hostname[0] != '\0' ) {
	    if (! TIFFSetField( tiff, TIFFTAG_HOSTCOMPUTER, mx_tiff_hostname ) )
	    {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set HostComputer TIFF tag." );
	    }
	}

	/* If present, specify the MX site name. */

	if ( frame->record != (MX_RECORD *) NULL ) {
	    MX_RECORD *site_name_record;
	    const char *driver_name;

	    site_name_record = mx_get_record( frame->record, "mx_site" );

	    if ( site_name_record == (MX_RECORD *) NULL ) {
		/* 'beamline_name' is an obsolete name that is
		 * still used occasionally at APS.
		 */

		site_name_record =
			mx_get_record( frame->record, "beamline_name");
	    }

	    if ( ( site_name_record != (MX_RECORD *) NULL )
	      && ( site_name_record->mx_superclass == MXR_VARIABLE ) )
	    {
		if ( mx_tiff_site_name[0] == '\0' ) {
		    mx_status = mx_get_string_variable( site_name_record,
						mx_tiff_site_name,
						sizeof(mx_tiff_site_name) );

		    MXW_UNUSED( mx_status );
		}

		if ( mx_tiff_site_name[0] != '\0' ) {

		    if (! TIFFSetField( tiff, TIFFTAG_ARTIST,
						mx_tiff_site_name ) )
		    {
			TIFFClose( tiff );
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Cannot set Artist TIFF tag." );
		    }
		}
	    }

	    /* Specify the MX driver name in the Model field. */

	    driver_name = mx_get_driver_name( frame->record );

	    if ( driver_name != NULL ) {
		char model_name[MXU_DRIVER_NAME_LENGTH+1];

		snprintf( model_name, sizeof(model_name),
			"MX '%s' driver.", driver_name );

		if (! TIFFSetField( tiff, TIFFTAG_MODEL, model_name ) ) {
		    TIFFClose( tiff );
		    return mx_error( MXE_FUNCTION_FAILED, fname,
		    "Cannot set Model TIFF tag." );
		}
	    }
	}

	/* Missing MX fields: binsize, bias offset. */

	/* FIXME: Things we might want to add later.
 	 *
 	 * Baseline:
 	 *   HostComputer
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
