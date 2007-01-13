/*
 * Name:    d_pccd_170170.c
 *
 * Purpose: MX driver for the Aviex PCCD-170170 CCD detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PCCD_170170_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_camera_link.h"
#include "mx_area_detector.h"
#include "d_pccd_170170.h"

/*---*/

#if HAVE_CAMERA_LINK

MX_RECORD_FUNCTION_LIST mxd_pccd_170170_record_function_list = {
	mxd_pccd_170170_initialize_type,
	mxd_pccd_170170_create_record_structures,
	mxd_pccd_170170_finish_record_initialization,
	mxd_pccd_170170_delete_record,
	NULL,
	NULL,
	NULL,
	mxd_pccd_170170_open,
	mxd_pccd_170170_close
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_pccd_170170_function_list = {
	mxd_pccd_170170_arm,
	mxd_pccd_170170_trigger,
	mxd_pccd_170170_stop,
	mxd_pccd_170170_abort,
	NULL,
	NULL,
	mxd_pccd_170170_get_extended_status,
	mxd_pccd_170170_readout_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pccd_170170_get_parameter,
	mxd_pccd_170170_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_pccd_170170_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MXD_PCCD_170170_STANDARD_FIELDS
};

long mxd_pccd_170170_num_record_fields
		= sizeof( mxd_pccd_170170_record_field_defaults )
			/ sizeof( mxd_pccd_170170_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pccd_170170_rfield_def_ptr
			= &mxd_pccd_170170_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_pccd_170170_get_pointers( MX_AREA_DETECTOR *ad,
			MX_PCCD_170170 **pccd_170170,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pccd_170170_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (pccd_170170 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PCCD_170170 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pccd_170170 = (MX_PCCD_170170 *)
				ad->record->record_type_struct;

	if ( *pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_PCCD_170170 pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_free_sector_array( uint16_t ****sector_array_ptr )
{
	static const char fname[] = "mxd_pccd_170170_free_sector_array()";

	uint16_t ***sector_array;
	uint16_t **sector_array_row_ptr;

	if ( sector_array_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The sector_array_ptr argument passed was NULL." );
	}

	sector_array = *sector_array_ptr;

	if ( sector_array == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	sector_array_row_ptr = *sector_array;

	if ( sector_array_row_ptr != NULL ) {
		free( sector_array_row_ptr );
	}

	free( sector_array );

	*sector_array_ptr = NULL;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_alloc_sector_array( uint16_t ****sector_array_ptr,
					long sector_height,
					long sector_width,
					uint16_t *image_data )
{
	static const char fname[] = "mxd_pccd_170170_alloc_sector_array()";

	uint16_t ***sector_array;
	uint16_t **sector_array_row_ptr;
	long num_sector_rows, num_sector_columns, num_sectors;
	long n, sector_row, row, sector_column;
	long row_of_sectors_size, row_size;
	long offset;

	if ( sector_array_ptr == (uint16_t ****) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The sector_array_ptr argument passed was NULL." );
	}
	if ( image_data == (uint16_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The image_data pointer passed was NULL." );
	}

	num_sector_rows = 4;
	num_sector_columns = 4;
	num_sectors = num_sector_rows * num_sector_columns;

	*sector_array_ptr = NULL;
	
	sector_array = malloc( num_sectors * sizeof(uint16_t **) );

	if ( sector_array == (uint16_t ***) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"sector array pointer.", num_sectors );
	}

	sector_array_row_ptr = 
		malloc( num_sectors * sector_height * sizeof(uint16_t *) );

	if ( sector_array_row_ptr == (uint16_t **) NULL ) {
		free( sector_array );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of row pointers.", num_sectors * sector_height );
	}

	for ( n = 0; n < num_sectors; n++ ) {
		sector_array[n] = sector_array_row_ptr
			+ n * sector_height * sizeof(sector_array_row_ptr);
	}

	row_of_sectors_size = num_sector_columns * sector_height * sector_width;

	row_size = num_sector_columns * sector_width;

	for ( sector_row = 0; sector_row < num_sector_rows; sector_row++ ) {
	    for ( row = 0; row < sector_width; row++ ) {
		for ( sector_column = 0; sector_column < num_sector_columns;
							sector_column++ )
		{
		    n = sector_column + 4 * sector_row;

		    offset = sector_row * row_of_sectors_size
				+ row * row_size
				+ sector_column;

		    sector_array[n][row] = image_data + offset;
		}
	    }
	}

	*sector_array_ptr = sector_array;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pccd_170170_descramble_image( MX_PCCD_170170 *pccd_170170,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *raw_frame )
{
	static const char fname[] = "mxd_pccd_170170_descramble_image()";

	long bytes_to_copy, raw_length, image_length;
	uint16_t *raw_frame_data;
	uint16_t ***image_sector_array;
	long i, j, i_framesize, j_framesize;
	mx_status_type mx_status;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}
	if ( raw_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The raw_frame pointer passed was NULL." );
	}

	image_length = image_frame->image_length;

	raw_length = raw_frame->image_length;

	if ( raw_length <= image_length ) {
		bytes_to_copy = raw_length;
	} else {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The raw frame of length %ld bytes is too long to fit "
		"into the image frame of length %ld bytes.",
			raw_length, image_length );
	}

	MX_DEBUG(-2,
	("%s: image_length = %ld, raw_length = %ld, bytes_to_copy = %ld",
		fname, image_length, raw_length, bytes_to_copy));

	if ( (raw_frame->framesize[0] != image_frame->framesize[0])
	  || (raw_frame->framesize[1] != image_frame->framesize[1]) )
	{
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"The raw frame (%ld,%ld) and the image frame (%ld,%ld) have "
		"different dimensions.  At present, I do not know what the "
		"correct mapping is for this case.",
			raw_frame->framesize[0], raw_frame->framesize[1],
			image_frame->framesize[0], image_frame->framesize[1] );
	}

	/* For each frame, we overlay the frame with 16 sets of sector array
	 * pointers so that we can treat each of the 16 sectors as independent
	 * two dimensional arrays.
	 */

	i_framesize = raw_frame->framesize[0] / 4;
	j_framesize = raw_frame->framesize[1] / 4;

	/* If the framesize has changed since the last time we descrambled
	 * a frame, we must create new sector arrays to point into the raw
	 * frame data and the image frame data.
	 *
	 * If the framesize is the same, we just leave the existing sector
	 * arrays as they are.
	 */

	if ( ( pccd_170170->old_framesize[0] != raw_frame->framesize[0] )
	  || ( pccd_170170->old_framesize[1] != raw_frame->framesize[1] ) )
	{
		mx_status = mxd_pccd_170170_free_sector_array(
					&(pccd_170170->image_sector_array) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_pccd_170170_alloc_sector_array(
					&(pccd_170170->image_sector_array),
					i_framesize, j_framesize,
					image_frame->image_data );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	raw_frame_data = raw_frame->image_data;

	MX_DEBUG(-2,("%s: raw_frame_data = %p, image_frame->image_data = %p",
		fname, raw_frame_data, image_frame->image_data));

	/* Copy and descramble the pixels from the raw frame
	 * to the image frame.
	 */

	image_sector_array = pccd_170170->image_sector_array;

	for ( i = 0; i < i_framesize; i++ ) {
	    for ( j = 0; j < j_framesize; j++ ) {

		image_sector_array[0][i][j] = raw_frame_data[15];

		image_sector_array[1][i_framesize-i-1][j] = raw_frame_data[14];

		image_sector_array[2][i][j] = raw_frame_data[11];

		image_sector_array[3][i_framesize-i-1][j] = raw_frame_data[10];

		image_sector_array[4][i][j_framesize-j-1] = raw_frame_data[12];

		image_sector_array[5][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[13];

		image_sector_array[6][i][j_framesize-j-1] = raw_frame_data[8];

		image_sector_array[7][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[9];

		image_sector_array[8][i][j] = raw_frame_data[3];

		image_sector_array[9][i_framesize-i-1][j] = raw_frame_data[2];

		image_sector_array[10][i][j] = raw_frame_data[7];

		image_sector_array[11][i_framesize-i-1][j] = raw_frame_data[6];

		image_sector_array[12][i][j_framesize-j-1] = raw_frame_data[0];

		image_sector_array[13][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[1];

		image_sector_array[14][i][j_framesize-j-1] = raw_frame_data[4];

		image_sector_array[15][i_framesize-i-1][j_framesize-j-1]
							= raw_frame_data[5];
	    }
	}

	MX_DEBUG(-2,("%s: Image descrambling complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pccd_170170_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_pccd_170170_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	pccd_170170 = (MX_PCCD_170170 *)
				malloc( sizeof(MX_PCCD_170170) );

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PCCD_170170 structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = pccd_170170;
	record->class_specific_function_list = 
			&mxd_pccd_170170_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	pccd_170170->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_finish_record_initialization( MX_RECORD *record )
{
	return mx_area_detector_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_pccd_170170_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxd_pccd_170170_delete_record()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pccd_170170 != NULL ) {
		if ( pccd_170170->raw_frame != NULL ) {
			mx_free( pccd_170170->raw_frame );
		}
		mx_free( pccd_170170 );
	}

	if ( ad != NULL ) {
		mx_free( ad );
	}

	if ( record != NULL ) {
		mx_free( record );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pccd_170170_open()";

	MX_AREA_DETECTOR *ad;
	MX_PCCD_170170 *pccd_170170;
	MX_RECORD *video_input_record;
	long vinput_framesize[2];
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	flags = pccd_170170->pccd_170170_flags;

	if ( flags & MXF_PCCD_170170_USE_SIMULATOR ) {
		mx_warning( "Area detector '%s' will use "
				"a camera simulator instead of a real camera.",
				record->name );
	}

	video_input_record = pccd_170170->video_input_record;

	/* FIXME: Need to change the file format. */

	ad->frame_file_format = MXT_IMAGE_FILE_PNM;

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	ad->sequence_parameters.sequence_type = MXT_SQ_ONE_SHOT;
	ad->sequence_parameters.num_parameters = 1;
	ad->sequence_parameters.parameter_array[0] = 1.0;

	/* Get the video input's current framesize, which will be used
	 * to compute the area detector's maximum framesize.
	 */

	mx_status = mx_video_input_get_framesize( video_input_record,
					&vinput_framesize[0],
					&vinput_framesize[1] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* At maximum resolution, the Aviex camera is configured such that
	 * each line contains 1024 groups of pixels, with 16 pixels per group.
	 * There are a total of 1024 lines.  This means that at maximum
	 * resolution, the video input is configured for a resolution
	 * of 16384 by 1024.  However, we want this to appear to the user
	 * to have a resolution of 4096 by 4096.  Thus we must rescale the
	 * resolution as reported by the video card by multiplying and
	 * dividing by appropriate factors of 4.
	 */

	ad->maximum_framesize[0] =
		vinput_framesize[0] / MXF_PCCD_170170_HORIZ_SCALE;

	ad->maximum_framesize[1] =
		vinput_framesize[1] * MXF_PCCD_170170_VERT_SCALE;

	/* Copy the maximum framesize to the current framesize. */

	ad->framesize[0] = ad->maximum_framesize[0];
	ad->framesize[1] = ad->maximum_framesize[1];

	/* The pixel clock frequency is 60 MHz. */

	mx_status = mx_video_input_set_pixel_clock_frequency(
				video_input_record, 6.0e7 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the video input's external trigger to trigger on
	 * the rising edge of the trigger pulse.
	 */

	mx_status = mx_video_input_set_external_trigger_polarity(
				video_input_record, MXF_VIN_TRIGGER_RISING );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the video input to use negative Camera Link pulses. */

	mx_status = mx_video_input_set_camera_trigger_polarity(
				video_input_record, MXF_VIN_TRIGGER_LOW );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the video input's initial trigger mode (internal/external/etc) */

	mx_status = mx_video_input_set_trigger_mode( video_input_record,
				pccd_170170->initial_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Load the image correction files. */

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize area detector parameters. */

	ad->pixel_order = MXT_IMAGE_PIXEL_ORDER_STANDARD;
	ad->header_length = 0;

	mx_status = mx_area_detector_get_image_format( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_pixel( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate space for a raw frame buffer.  This buffer will be used to
	 * read in the raw pixels from the imaging board before descrambling.
	 */

#if 1
	MX_DEBUG(-2,("%s: Before final call to mx_image_alloc()", fname));

	MX_DEBUG(-2,("%s: &(pccd_170170->raw_frame) = %p",
			fname, &(pccd_170170->raw_frame) ));

	MX_DEBUG(-2,("%s: vinput_framesize = (%lu,%lu)",
			fname, vinput_framesize[0], vinput_framesize[1]));

	MX_DEBUG(-2,("%s: image_format = %ld", fname, ad->image_format ));
	MX_DEBUG(-2,("%s: pixel_order = %ld", fname, ad->pixel_order ));
	MX_DEBUG(-2,("%s: bytes_per_pixel = %g", fname, ad->bytes_per_pixel));
	MX_DEBUG(-2,("%s: header_length = %ld", fname, ad->header_length));
	MX_DEBUG(-2,("%s: bytes_per_frame = %ld", fname, ad->bytes_per_frame));
#endif

	mx_status = mx_image_alloc( &(pccd_170170->raw_frame),
					MXT_IMAGE_LOCAL_1D_ARRAY,
					vinput_framesize,
					ad->image_format,
					ad->pixel_order,
					ad->bytes_per_pixel,
					ad->header_length,
					ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pccd_170170->old_framesize[0] = -1;
	pccd_170170->old_framesize[1] = -1;

	pccd_170170->image_sector_array = NULL;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_arm()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_arm( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_trigger()";

	MX_PCCD_170170 *pccd_170170;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"area detector '%s'.",
			sp->sequence_type, ad->record->name );
	}

	mx_status = mx_video_input_trigger( pccd_170170->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_stop()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_stop( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_abort()";

	MX_PCCD_170170 *pccd_170170;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_abort( pccd_170170->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_get_extended_status()";

	MX_PCCD_170170 *pccd_170170;
	long last_frame_number;
	unsigned long status_flags;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_get_status( pccd_170170->video_input_record,
						&last_frame_number,
						&status_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->last_frame_number = last_frame_number;

	ad->status = 0;

	if ( status_flags & MXSF_VIN_IS_BUSY ) {
		ad->status |= MXSF_AD_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_readout_frame()";

	MX_PCCD_170170 *pccd_170170;
	unsigned long flags;
	size_t bytes_to_copy, raw_frame_length, image_length;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

#if 0
	mx_status = mx_video_input_get_frame(
		pccd_170170->video_input_record,
		ad->readout_frame, &(ad->image_frame) );
#else
	/* Read in the raw image frame. */

	mx_status = mx_video_input_get_frame(
		pccd_170170->video_input_record,
		ad->readout_frame, &(pccd_170170->raw_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If required, we now descramble the image. */

	flags = pccd_170170->pccd_170170_flags;

	if ( flags & MXF_PCCD_170170_USE_SIMULATOR ) {
		/* If we are using the camera simulator board, the data
		 * stream coming in is not scrambled, so we just copy
		 * directly from the raw frame to the image frame.
		 */

		raw_frame_length = pccd_170170->raw_frame->image_length;

		image_length = ad->image_frame->image_length;

		if ( raw_frame_length < image_length ) {
			bytes_to_copy = raw_frame_length;
		} else {
			bytes_to_copy = image_length;
		}

#if MXD_SOFT_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
	    ("%s: Copying camera simulator image.  Image length = %lu bytes.",
			fname, bytes_to_copy ));
#endif
		memcpy( ad->image_frame->image_data,
			pccd_170170->raw_frame->image_data,
			bytes_to_copy );
	} else {
		/* Descramble the image. */

		mx_status = mxd_pccd_170170_descramble_image( pccd_170170,
							ad->image_frame,
							pccd_170170->raw_frame);
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_get_parameter()";

	MX_PCCD_170170 *pccd_170170;
	MX_RECORD *video_input_record;
	long vinput_horiz_framesize, vinput_vert_framesize;
	mx_status_type mx_status;

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif
	video_input_record = pccd_170170->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		mx_status = mx_video_input_get_framesize( video_input_record,
			     &vinput_horiz_framesize, &vinput_vert_framesize );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* See the comments about the Aviex camera in the function
		 * mxd_pccd_170170_open() for the explanation of where the
		 * factors of 4 come from.
		 */

		ad->framesize[0] = 
			vinput_horiz_framesize / MXF_PCCD_170170_HORIZ_SCALE;

		ad->framesize[1] =
			vinput_vert_framesize * MXF_PCCD_170170_VERT_SCALE;
		break;
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_video_input_get_image_format( video_input_record,
						&(ad->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		mx_status = mx_video_input_get_bytes_per_frame(
				video_input_record, &(ad->bytes_per_frame) );
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		mx_status = mx_video_input_get_bytes_per_pixel(
				video_input_record, &(ad->bytes_per_pixel) );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		break;

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_get_trigger_mode(
				video_input_record, &(ad->trigger_mode) );
		break;

	case MXLV_AD_PROPERTY_NAME:
		break;
	case MXLV_AD_PROPERTY_DOUBLE:

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,("%s: Returning value %g for property '%s'",
			fname, ad->property_double, ad->property_name ));
#endif
		break;
	case MXLV_AD_PROPERTY_LONG:

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,("%s: Returning value %ld for property '%s'",
			fname, ad->property_long, ad->property_name ));
#endif
		break;
	case MXLV_AD_PROPERTY_STRING:

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,("%s: Returning string '%s' for property '%s'",
			fname, ad->property_string, ad->property_name ));
#endif
		break;
	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_pccd_170170_set_parameter()";

	MX_PCCD_170170 *pccd_170170;
	long vinput_horiz_framesize, vinput_vert_framesize;
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2, 4, 8, 16, 32, 64 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	mx_status = mxd_pccd_170170_get_pointers( ad, &pccd_170170, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PCCD_170170_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Tell the video input to change its framesize. */

		/* See the comments about the Aviex camera in the function
		 * mxd_pccd_170170_open() for the explanation of where the
		 * factors of 4 come from.
		 */

		vinput_horiz_framesize =
			ad->framesize[0] * MXF_PCCD_170170_HORIZ_SCALE;

		vinput_vert_framesize =
			ad->framesize[1] / MXF_PCCD_170170_VERT_SCALE;

		mx_status = mx_video_input_set_framesize(
					pccd_170170->video_input_record,
					vinput_horiz_framesize,
					vinput_vert_framesize );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		mx_status = mx_video_input_set_sequence_parameters(
					pccd_170170->video_input_record,
					&(ad->sequence_parameters) );
		break; 

	case MXLV_AD_TRIGGER_MODE:
		mx_status = mx_video_input_set_trigger_mode(
				pccd_170170->video_input_record,
				ad->trigger_mode );
		break;

	case MXLV_AD_PROPERTY_NAME:
		break;
	case MXLV_AD_PROPERTY_DOUBLE:

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,("%s: Setting property '%s' to %g",
			fname, ad->property_name, ad->property_double ));
#endif
		break;
	case MXLV_AD_PROPERTY_LONG:

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,("%s: Setting property '%s' to %ld",
			fname, ad->property_name, ad->property_long ));
#endif
		break;
	case MXLV_AD_PROPERTY_STRING:

#if MXD_PCCD_170170_DEBUG
		MX_DEBUG(-2,("%s: Setting property '%s' to '%s'",
			fname, ad->property_name, ad->property_string ));
#endif
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
		break;

	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_camera_link_command( MX_PCCD_170170 *pccd_170170,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag )
{
	static const char fname[] = "mxd_pccd_170170_camera_link_command()";

	MX_RECORD *camera_link_record;
	size_t command_length, response_length;
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( max_response_length < 1 ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The requested response buffer length %lu for record '%s' "
		"is too short to hold a minimum length response.",
			(unsigned long) max_response_length,
			pccd_170170->record->name );
	}

	camera_link_record = pccd_170170->camera_link_record;

	if ( camera_link_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The camera_link_record pointer for record '%s' is NULL.",
			pccd_170170->record->name );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sent '%s' to '%s'",
			fname, command, camera_link_record->name ));
	}

	command_length = strlen(command);

	mx_status = mx_camera_link_serial_write( camera_link_record,
						command, &command_length, -1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Leave room in the response buffer to null terminate the response. */

	response_length = max_response_length - 1;

	mx_status = mx_camera_link_serial_read( camera_link_record,
						response, &response_length, -1);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	response[response_length] = '\0';

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, camera_link_record->name ));
	}

	if ( response[0] == 'E' ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error response received for command '%s' sent to "
		"Camera Link interface '%s'.",
			command, camera_link_record->name );
	}
	
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_read_register( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long *register_value )
{
	static const char fname[] = "mxd_pccd_170170_read_register()";

	char command[10], response[10];
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( register_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The register_value pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( register_address >= 100 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested register address %lu for record '%s' "
		"is outside the allowed range of 0 to 99.",
			register_address, pccd_170170->record->name );
	}

	snprintf( command, sizeof(command), "R%02lu", register_address );

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, 5,
					MXD_PCCD_170170_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	response[4] = '\0';

	*register_value = atoi( &response[1] );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_write_register( MX_PCCD_170170 *pccd_170170,
				unsigned long register_address,
				unsigned long register_value )
{
	static const char fname[] = "mxd_pccd_170170_write_register()";

	char command[10], response[10];
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( register_address >= 100 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested register address %lu for record '%s' "
		"is outside the allowed range of 0 to 99.",
			register_address, pccd_170170->record->name );
	}
	if ( register_value >= 300 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested register value %lu for record '%s' "
		"is outside the allowed range of 0 to 299.",
			register_value, pccd_170170->record->name );
	}

	snprintf( command, sizeof(command),
		"W%02lu%03lu", register_address, register_value );

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, 2,
					MXD_PCCD_170170_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pccd_170170_read_adc( MX_PCCD_170170 *pccd_170170,
				unsigned long adc_address,
				double *adc_value )
{
	static const char fname[] = "mxd_pccd_170170_read_adc()";

	char command[10], response[10];
	mx_status_type mx_status;

	if ( pccd_170170 == (MX_PCCD_170170 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCCD_170170 pointer passed was NULL." );
	}
	if ( adc_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The adc_value pointer passed for record '%s' was NULL.",
			pccd_170170->record->name );
	}
	if ( adc_address >= 8 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested register address %lu for record '%s' "
		"is outside the allowed range of 0 to 7.",
			adc_address, pccd_170170->record->name );
	}

	snprintf( command, sizeof(command), "R%01lu", adc_address );

	mx_status = mxd_pccd_170170_camera_link_command( pccd_170170,
					command, response, 7,
					MXD_PCCD_170170_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	response[4] = '\0';

	*adc_value = atof( &response[1] );

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_CAMERA_LINK */

