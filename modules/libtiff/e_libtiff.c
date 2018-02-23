/*
 * Name:    e_libtiff.c
 *
 * Purpose: MX libtiff extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define LIBTIFF_MODULE_DEBUG_INITIALIZE	FALSE

#define LIBTIFF_MODULE_DEBUG_FINALIZE	FALSE

#define LIBTIFF_MODULE_DEBUG_READ	FALSE

#define LIBTIFF_MODULE_DEBUG_WRITE	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Include file from libtiff. */
#include "tiffio.h"

/* Note (TIFFLIB_VERSION == 20120615) corresponds to LibTIFF 4.0.2, which was
 * the first version to support TIFFCreateEXIFDirectory().
 */

#if (defined(TIFF_VERSION_BIG) && (TIFFLIB_VERSION >= 20120615))
#  define MX_USE_EXIF_TIFF_TAGS		TRUE
#else
#  define MX_USE_EXIF_TIFF_TAGS		FALSE
#endif

#if ( !defined(TIFF_VERSION_BIG) )
#error MX only supports LibTIFF 4.0 and above.
#endif

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
#elif defined(OS_MACOSX)
#  define MXP_LIBTIFF_LIBRARY_NAME	"libtiff.dylib"
#else
#  define MXP_LIBTIFF_LIBRARY_NAME	"libtiff.so"
#endif

/*------*/

static char mxp_tiff_site_description[256] = { '\0' };

static char mxp_tiff_hostcomputer[(2*MXU_HOSTNAME_LENGTH)+1] = { '\0' };

/*------*/

static void
mxext_libtiff_error_handler( const char *module, const char *fmt, ... )
{
	va_list va_alist;
	char buffer[2500];

	va_start( va_alist, fmt );
	vsnprintf( buffer, sizeof(buffer), fmt, va_alist );
	va_end( va_alist );

	MX_DEBUG(-2,
		("mxext_libtiff_error_handler(): module = '%s', fmt = '%s'",
		module, fmt ));

	fprintf( stderr, "%s\n", buffer );
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
	uint32_t row_width, column_height;
	uint16_t samples_per_pixel;
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
		"Cannot read the ImageWidth TIFF tag." );
	}

	if (! TIFFGetField( tiff, TIFFTAG_IMAGELENGTH, &column_height ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot read the ImageLength TIFF tag." );
	}

	if (! TIFFGetField( tiff, TIFFTAG_SAMPLESPERPIXEL,
			&samples_per_pixel ) )
	{
		/* If the image does not have a Samples/Pixel header value
		 * such as a Pilatus image, we assume that it is 1.
		 */

		samples_per_pixel = 1;
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

	switch( bits_per_sample ) {
	case 16:
	case 32:	/* We have 16-bit and 32-bit image support here.*/
		break;
	default:
		TIFFClose( tiff );
		return mx_error( MXE_UNSUPPORTED, fname,
		"TIFF image '%s' has %lu bits per sample.  However, MX "
		"currently only supports reading images with 16 or 32 bits "
		"per sample.", datafile_name, (unsigned long) bits_per_sample );
		break;
	}

	/* Read and parse the timestamp in the TIFF image. */

	if (! TIFFGetField( tiff, TIFFTAG_DATETIME, &tiff_datetime_buffer ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot read the DateTime TIFF tag." );
	}

	ptr = strptime( tiff_datetime_buffer, "%Y:%m:%d %H:%M:%S", &tm_struct );

	if ( ptr == NULL ) {
		/* If strptime() cannot parse the time returned by LibTIFF,
		 * then just set the time to the current time.
		 */

		time_in_seconds = time(NULL);
	} else {
		time_in_seconds = mktime( &tm_struct );
	}

	/*----*/

	scanline_size_in_bytes = TIFFScanlineSize( tiff );
	scanline_buffer = _TIFFmalloc( scanline_size_in_bytes );

	/* Create an MX_IMAGE_FRAME structure to copy the TIFF into. */

	switch( bits_per_sample ) {
	case 16:
		image_format = MXT_IMAGE_FORMAT_GREY16;
		bytes_per_pixel = 2.0;
		image_size_in_bytes = 2 * row_width * column_height;
		break;
	case 32:
		image_format = MXT_IMAGE_FORMAT_INT32;
		bytes_per_pixel = 4.0;
		image_size_in_bytes = 4 * row_width * column_height;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Only 16-bits per sample and 32-bits per sample are "
		"supported by MX.  But this was tested for earlier in "
		"this function, so how did you get here?" );
		break;
	}

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
#if 0
	long bytes_per_image;
	double bytes_per_pixel;
#endif
	double horz_pixels_per_cm, vert_pixels_per_cm;

	int tiff_status;

	char *scanline_buffer = NULL;
	char *image_data_ptr = NULL;

	char mx_version_string[80];

	char tiff_timestamp[25];
	char full_timestamp[80];
	char full_leading_timestamp[80];
	time_t time_in_seconds;
	struct tm tm_struct;

	long sample_format;

	double exposure_seconds;

	char image_description[500];
	char temp_buffer[2*MXU_FILENAME_LENGTH+3];

#if MX_USE_EXIF_TIFF_TAGS
	uint64 exif_dir_offset;
#endif

	MX_AREA_DETECTOR *ad = NULL;
	MX_VIDEO_INPUT *vinput = NULL;

	MX_SEQUENCE_PARAMETERS seq;
	long i, num_frames_in_sequence;

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
		"Cannot set BitsPerSample TIFF tag." );
	}

	/* MX does not compress TIFF images created by it, so we set
	 * the Compression field to No compression (1).
	 */

	if (! TIFFSetField( tiff, TIFFTAG_COMPRESSION, 1 ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set Compression TIFF tag." );
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

	strftime( tiff_timestamp, sizeof(tiff_timestamp),
		"%Y:%m:%d %H:%M:%S", &tm_struct );

#if LIBTIFF_MODULE_DEBUG_WRITE
	MX_DEBUG(-2,("%s: timestamp = '%s'", fname, timestamp));
#endif

	if (! TIFFSetField( tiff, TIFFTAG_DATETIME, tiff_timestamp ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set DateTime TIFF tag." );
	}

	/* Construct a full timestamp to be written into the ImageDescription
	 * field of the TIFF header later on in this function.
	 */

	strftime( full_leading_timestamp, sizeof(full_leading_timestamp),
		"%Y-%m-%d %H:%M:%S", &tm_struct );

	snprintf( full_timestamp, sizeof(full_timestamp),
			"# Timestamp: %s.%09lu\n",
			full_leading_timestamp,
			(unsigned long) MXIF_TIMESTAMP_NSEC(frame) );

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
		"Cannot set RowsPerStrip TIFF tag." );
	}

	/* Specify the byte offset of the image data relative to the start
	 * of the TIFF file.  Currently, the offset is 4096 bytes.  Note that
	 * we only use 1 strip in MX.
	 */

#if 0
	/* FIXME: The following does not work for some reason. */

	if (! TIFFSetField( tiff, TIFFTAG_STRIPOFFSETS, 4096 ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set StripOffsets TIFF tag." );
	}

	/* Specify the number of bytes in the image. */

	bytes_per_pixel = MXIF_BYTES_PER_PIXEL(frame);

	bytes_per_image = mx_round(bytes_per_pixel * row_width * column_height);

	if (! TIFFSetField( tiff, TIFFTAG_STRIPBYTECOUNTS, bytes_per_image ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set StripByteCounts TIFF tag." );
	}
#endif

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
		/* Convert from MX mm/pixel to TIFF pixels/cm. */

		horz_pixels_per_cm = mx_divide_safely(10.0, ad->resolution[0]);

		vert_pixels_per_cm = mx_divide_safely(10.0, ad->resolution[1]);

		if (! TIFFSetField( tiff, TIFFTAG_XRESOLUTION,
				horz_pixels_per_cm ) )
		{
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Cannot set XResolution TIFF tag." );
		}

		if (! TIFFSetField( tiff, TIFFTAG_YRESOLUTION,
				vert_pixels_per_cm ) )
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

	/* If present, specify the MX site name. */

	if ( frame->record != (MX_RECORD *) NULL ) {
	    MX_RECORD *site_description_record;
	    const char *driver_name;

	    site_description_record = mx_get_record( frame->record,
						"mx_site_description" );

	    if ( ( site_description_record != (MX_RECORD *) NULL )
	      && ( site_description_record->mx_superclass == MXR_VARIABLE ) )
	    {
		if ( mxp_tiff_site_description[0] == '\0' ) {
		    mx_status = mx_get_string_variable( site_description_record,
					mxp_tiff_site_description,
					sizeof(mxp_tiff_site_description) );

		    MXW_UNUSED( mx_status );
		}

		if ( mxp_tiff_site_description[0] != '\0' ) {

		    if (! TIFFSetField( tiff, TIFFTAG_ARTIST,
						mxp_tiff_site_description ) )
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

	/* Get the exposure time from the image frame header. */

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

	/* Tradition appears to be to put some information that does not
	 * fit well into any of the existing tags into the string tag called
	 * ImageDescription.  That way, TIFF readers that do not understand
	 * the EXIF IFD can still find this information.
	 */

	snprintf( temp_buffer, sizeof(temp_buffer),
		"# Exposure_time: %f seconds\n", exposure_seconds );

	strlcpy( image_description,
		temp_buffer,
		sizeof(image_description) );

	/* Describe the currently installed sequence in the image header. */

	if ( ad != (MX_AREA_DETECTOR *) NULL ) {
		mx_status = mx_area_detector_get_sequence_parameters(
				frame->record, &seq );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( seq.sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			strlcat( image_description,
				"# Sequence: one_shot ",
				sizeof(image_description) );
			break;
		case MXT_SQ_STREAM:
			strlcat( image_description,
				"# Sequence: stream ",
				sizeof(image_description) );
			break;
		case MXT_SQ_MULTIFRAME:
			strlcat( image_description,
				"# Sequence: multiframe ",
				sizeof(image_description) );
			break;
		case MXT_SQ_STROBE:
			strlcat( image_description,
				"# Sequence: strobe ",
				sizeof(image_description) );
			break;
		case MXT_SQ_DURATION:
			strlcat( image_description,
				"# Sequence: duration ",
				sizeof(image_description) );
			break;
		case MXT_SQ_GATED:
			strlcat( image_description,
				"# Sequence: gated ",
				sizeof(image_description) );
			break;
		case MXT_SQ_GEOMETRICAL:
			strlcat( image_description,
				"# Sequence: geometrical ",
				sizeof(image_description) );
			break;
		case MXT_SQ_STREAK_CAMERA:
			strlcat( image_description,
				"# Sequence: streak_camera ",
				sizeof(image_description) );
			break;
		case MXT_SQ_SUBIMAGE:
			strlcat( image_description,
				"# Sequence: subimage ",
				sizeof(image_description) );
			break;
		default:
			snprintf( temp_buffer, sizeof(temp_buffer),
				"# Sequence: unknown(%ld) ",
				seq.sequence_type );
			strlcat( image_description,
				temp_buffer, sizeof(image_description) );
			break;
		}

		snprintf( temp_buffer, sizeof(temp_buffer),
			"%f", seq.parameter_array[0] );
		strlcat( image_description,
			temp_buffer, sizeof(image_description) );

		for ( i = 1; i < seq.num_parameters; i++ ) {
			snprintf( temp_buffer, sizeof(temp_buffer),
				", %f", seq.parameter_array[i] );
			strlcat( image_description,
				temp_buffer, sizeof(image_description) );
		}
		strlcat( image_description, "\n", sizeof(image_description) );

		/*------------------*/

		strlcat( image_description, full_timestamp,
				sizeof(image_description) );

		/*------------------*/

		mx_status = mx_sequence_get_num_frames( &seq,
						&num_frames_in_sequence );

		if ( mx_status.code != MXE_SUCCESS ) {
			TIFFClose( tiff );
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Could not get the number of frames in this sequence.");
		}

		snprintf( temp_buffer, sizeof(temp_buffer),
			"# Frame_number: %ld\n",
			ad->datafile_last_frame_number - 1 );

		strlcat( image_description, temp_buffer,
				sizeof(image_description) );

		snprintf( temp_buffer, sizeof(temp_buffer),
			"# Num_frames_in_sequence: %ld\n",
			num_frames_in_sequence );

		strlcat( image_description, temp_buffer,
				sizeof(image_description) );

		/*------------------*/

		snprintf( temp_buffer, sizeof(temp_buffer),
			"# trigger_mode: %#lx\n",
			ad->trigger_mode );

		strlcat( image_description, temp_buffer,
				sizeof(image_description) );

		snprintf( temp_buffer, sizeof(temp_buffer),
			"# correction_flags: %#lx\n",
			ad->correction_flags );

		strlcat( image_description, temp_buffer,
				sizeof(image_description) );
	}

	/*----------------------------------------------------------------*/

	if (! TIFFSetField( tiff, TIFFTAG_IMAGEDESCRIPTION, image_description) )
	{
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set ImageDescription TIFF tag." );
	}

	if ( ad != (MX_AREA_DETECTOR *) NULL ) {
		snprintf( temp_buffer, sizeof(temp_buffer), "%s/%s",
			ad->datafile_directory, ad->datafile_pattern );

		if (! TIFFSetField( tiff, TIFFTAG_DOCUMENTNAME, temp_buffer ) )
		{
			TIFFClose( tiff );
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Cannot set DocumentName TIFF tag." );
		}

		/*---*/

		snprintf( temp_buffer, sizeof(temp_buffer), "%s/%s",
			ad->datafile_directory, ad->datafile_name );

		if (! TIFFSetField( tiff, TIFFTAG_PAGENAME, temp_buffer ) )
		{
			TIFFClose( tiff );
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Cannot set PageName TIFF tag." );
		}
	}

	/* Write out the hostname and operating system that created
	 * this file.
	 */

	if ( mxp_tiff_hostcomputer[0] == '\0' ) {
		char host_temp_buffer[MXU_HOSTNAME_LENGTH+1];
		struct hostent *host_entry;
		const char *dns_name;
		char os_temp_buffer[100];

		/* First, get the hostname of the computer. */

		mx_status = mx_gethostname( host_temp_buffer,
					sizeof(host_temp_buffer) );

		if ( mx_status.code != MXE_SUCCESS ) {
			TIFFClose( tiff );
			return mx_status;
		}

		/* Attempt to use the hostname that we just got to
		 * get the DNS domain name using gethostbyname().
		 */

		host_entry = gethostbyname( host_temp_buffer );

		if ( host_entry == NULL ) {
			dns_name = host_temp_buffer;
		} else {
			dns_name = host_entry->h_name;
		}

		mx_status = mx_get_os_version_string( os_temp_buffer,
						sizeof(os_temp_buffer) );

		if ( mx_status.code != MXE_SUCCESS ) {
			TIFFClose( tiff );
			return mx_status;
		}

		snprintf( mxp_tiff_hostcomputer, sizeof(mxp_tiff_hostcomputer),
			"(%s) %s", dns_name, os_temp_buffer );
	}

	if (! TIFFSetField(tiff, TIFFTAG_HOSTCOMPUTER, mxp_tiff_hostcomputer) )
	{
			TIFFClose( tiff );
			return mx_error( MXE_FUNCTION_FAILED, fname,
			"Cannot set HostComputer TIFF tag." );
	}

	/* Missing MX fields: binsize, bias offset. */

	/* FIXME: Things we might want to add later.
 	 *
 	 * Extension:
 	 *   SMaxSampleValue, SMinSampleValue, XPosition, YPosition
 	 *
 	 * EXIF:
 	 *   SubjectDistance
 	 */

/*--------------------------- Write the image data -------------------------*/

	/* Write the image data to the file.
	 *
	 * Note: The image data must be written to the file before
	 * the EXIF directory has been created.  If you put the 
	 * image data write after the EXIF operations, then libtiff
	 * will refuse to write out the image data.
	 */

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

/*------------------------------ TIFF EXIF Support -------------------------*/
#if MX_USE_EXIF_TIFF_TAGS

	/* Only LibTIFF 4.0 and above explicitly support EXIF TIFF tags. */

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

	/* Save the exposure time to an EXIF tag designed for it. */

	if (! TIFFSetField( tiff, EXIFTAG_EXPOSURETIME, exposure_seconds ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set EXIFTAG_EXPOSURETIME tag." );
	}

	/* Save the fractional seconds part of the image timestamp. */

	snprintf( temp_buffer, sizeof(temp_buffer), "%f",
		1.0e-9 * MXIF_TIMESTAMP_NSEC( frame ) );

	if (! TIFFSetField( tiff, EXIFTAG_SUBSECTIME, temp_buffer ) ) {
		TIFFClose( tiff );
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Cannot set EXIFTAG_SUBSECTIME tag." );
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

#endif /* MX_USE_EXIF_TIFF_TAGS */
/*-------------------------- End of TIFF EXIF Support -----------------------*/

	/* We are done, so close down everything. */

	TIFFClose( tiff );

	if ( scanline_buffer != NULL ) {
		_TIFFfree( scanline_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/
