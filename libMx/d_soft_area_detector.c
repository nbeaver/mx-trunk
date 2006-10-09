/*
 * Name:    d_soft_area_detector.c
 *
 * Purpose: MX driver for a software emulated area detector.  It uses an
 *          MX video input record as the source of the frame images.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_AREA_DETECTOR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "d_soft_area_detector.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_soft_area_detector_record_function_list = {
	mxd_soft_area_detector_initialize_type,
	mxd_soft_area_detector_create_record_structures,
	mxd_soft_area_detector_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_soft_area_detector_open,
	mxd_soft_area_detector_close
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_soft_area_detector_function_list = {
	mxd_soft_area_detector_arm,
	mxd_soft_area_detector_trigger,
	mxd_soft_area_detector_stop,
	mxd_soft_area_detector_abort,
	NULL,
	mxd_soft_area_detector_get_status,
	NULL,
	mxd_soft_area_detector_readout_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_soft_area_detector_get_parameter,
	mxd_soft_area_detector_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_area_detector_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_SOFT_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_soft_area_detector_num_record_fields
		= sizeof( mxd_soft_area_detector_record_field_defaults )
			/ sizeof( mxd_soft_area_detector_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_area_detector_rfield_def_ptr
			= &mxd_soft_area_detector_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_soft_area_detector_get_pointers( MX_AREA_DETECTOR *ad,
			MX_SOFT_AREA_DETECTOR **soft_area_detector,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_area_detector_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (soft_area_detector == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_SOFT_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*soft_area_detector = (MX_SOFT_AREA_DETECTOR *)
				ad->record->record_type_struct;

	if ( *soft_area_detector == (MX_SOFT_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_SOFT_AREA_DETECTOR pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_soft_area_detector_initialize_type( long record_type )
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
mxd_soft_area_detector_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_soft_area_detector_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_SOFT_AREA_DETECTOR *soft_area_detector;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	soft_area_detector = (MX_SOFT_AREA_DETECTOR *)
				malloc( sizeof(MX_SOFT_AREA_DETECTOR) );

	if ( soft_area_detector == (MX_SOFT_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_SOFT_AREA_DETECTOR structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = soft_area_detector;
	record->class_specific_function_list = 
			&mxd_soft_area_detector_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	soft_area_detector->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_finish_record_initialization( MX_RECORD *record )
{
	return mx_area_detector_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_area_detector_open()";

	MX_AREA_DETECTOR *ad;
	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	MX_RECORD *video_input_record;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	video_input_record = soft_area_detector->video_input_record;

	/* FIXME: Need to change the file format. */

	ad->frame_file_format = MXT_IMAGE_FILE_PNM;

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	ad->sequence_parameters.sequence_type = MXT_SQ_ONE_SHOT;
	ad->sequence_parameters.num_parameters = 1;
	ad->sequence_parameters.parameter_array[0] = 1.0;

	/* Set the maximum framesize to the initial framesize of the
	 * video input.
	 */

	mx_status = mx_video_input_get_framesize( video_input_record,
					&(ad->maximum_framesize[0]),
					&(ad->maximum_framesize[1]) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->framesize[0] = ad->maximum_framesize[0];
	ad->framesize[1] = ad->maximum_framesize[1];

	/* Set the video input's initial trigger mode (internal/external/etc) */

	mx_status = mx_video_input_set_trigger_mode( video_input_record,
				soft_area_detector->initial_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Load the image correction files. */

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_arm()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_arm(soft_area_detector->video_input_record);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_trigger()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		break;

	case MXT_SQ_CONTINUOUS:
		break;

	case MXT_SQ_MULTI:
		break;

	case MXT_SQ_CONTINUOUS_MULTI:
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"area detector '%s'.",
			sp->sequence_type, ad->record->name );
	}

	mx_status = mx_video_input_trigger(
			soft_area_detector->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_stop()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_stop(soft_area_detector->video_input_record);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_abort()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_abort(
			soft_area_detector->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_get_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_get_status()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_is_busy(
			soft_area_detector->video_input_record, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->status = 0;

	if ( busy ) {
		ad->status |= MXSF_AD_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_readout_frame()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_get_frame(
		soft_area_detector->video_input_record,
		ad->readout_frame, &(ad->image_frame) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_get_parameter()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	MX_RECORD *video_input_record;
	mx_status_type mx_status;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif
	video_input_record = soft_area_detector->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		mx_status = mx_video_input_get_framesize( video_input_record,
				&(ad->framesize[0]), &(ad->framesize[1]) );
		break;
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_video_input_get_image_format( video_input_record,
						&(ad->image_format) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get_image_format_name_from_type(
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
	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_set_parameter()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	double x_binsize, y_binsize, x_framesize, y_framesize;
	int i;
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2, 4, 8, 16, 32, 64 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		/* Compute the closest integer binsize. */

		x_binsize = mx_divide_safely( ad->maximum_framesize[0],
						ad->framesize[0] );

		y_binsize = mx_divide_safely( ad->maximum_framesize[1],
						ad->framesize[1] );

		ad->binsize[0] = mx_round( x_binsize );
		ad->binsize[1] = mx_round( y_binsize );

		/* Fall through to the next case of changing the binsize. */
	case MXLV_AD_BINSIZE:
		for ( i = 0; i < num_allowed_binsizes; i++ ) {
			if ( ad->binsize[0] == allowed_binsize[i] )
				break;
		}

		if ( i >= num_allowed_binsizes ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested X binsize of %ld for area detector '%s' "
			"is not supported.  The allowed binsizes are "
			"1, 2, 4, 8, 16, 32, and 64.",
				ad->binsize[0], ad->record->name );
		}

		for ( i = 0; i < num_allowed_binsizes; i++ ) {
			if ( ad->binsize[1] == allowed_binsize[i] )
				break;
		}

		if ( i >= num_allowed_binsizes ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested Y binsize of %ld for area detector '%s' "
			"is not supported.  The allowed binsizes are "
			"1, 2, 4, 8, 16, 32, and 64.",
				ad->binsize[1], ad->record->name );
		}

		/* Compute the framesize from the binsize. */

		x_framesize = mx_divide_safely( ad->maximum_framesize[0],
							ad->binsize[0] );

		y_framesize = mx_divide_safely( ad->maximum_framesize[1],
							ad->binsize[1] );

		ad->framesize[0] = x_framesize;
		ad->framesize[1] = y_framesize;

		/* Tell the video input to change its framesize. */

		mx_status = mx_video_input_set_framesize(
					soft_area_detector->video_input_record,
					ad->framesize[0], ad->framesize[1] );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		mx_status = mx_video_input_set_sequence_parameters(
					soft_area_detector->video_input_record,
					&(ad->sequence_parameters) );
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

