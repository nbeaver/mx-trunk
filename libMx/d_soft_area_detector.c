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
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_AREA_DETECTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
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
	NULL,
	NULL,
	mxd_soft_area_detector_get_extended_status,
	mxd_soft_area_detector_readout_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_soft_area_detector_get_parameter,
	mxd_soft_area_detector_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_area_detector_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_SOFT_AREA_DETECTOR_STANDARD_FIELDS
};

long mxd_soft_area_detector_num_record_fields
		= sizeof( mxd_soft_area_detector_rf_defaults )
		/ sizeof( mxd_soft_area_detector_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_area_detector_rfield_def_ptr
			= &mxd_soft_area_detector_rf_defaults[0];

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

	/* Use calloc() here instead of malloc(), so that a bunch of
	 * fields that are never touched will be initialized to 0.
	 */

	ad = (MX_AREA_DETECTOR *) calloc( 1, sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	soft_area_detector = (MX_SOFT_AREA_DETECTOR *)
				calloc( 1, sizeof(MX_SOFT_AREA_DETECTOR) );

	if ( soft_area_detector == (MX_SOFT_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_SOFT_AREA_DETECTOR structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = soft_area_detector;
	record->class_specific_function_list = 
			&mxd_soft_area_detector_function_list;

	ad->record = record;
	soft_area_detector->record = record;

	soft_area_detector->start_time = mx_convert_seconds_to_clock_ticks(0);

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
	long i;
	mx_status_type mx_status;

	soft_area_detector = NULL;

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

	ad->header_length = 0;

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

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	/* Copy other needed parameters from the video input record to
	 * the area detector record.
	 */

	mx_status = mx_video_input_get_image_format( video_input_record,
							&(ad->image_format) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_image_get_format_name_from_type( ad->image_format,
						ad->image_format_name,
						sizeof(ad->image_format_name) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->frame_file_format = ad->image_format;  /* Is this used? */

	mx_status = mx_video_input_get_byte_order( video_input_record,
							&(ad->byte_order) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_pixel( video_input_record,
							&(ad->bytes_per_pixel));
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bits_per_pixel( video_input_record,
							&(ad->bits_per_pixel));
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( video_input_record,
							&(ad->bytes_per_frame));
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the video input's initial trigger mode (internal/external/etc) */

	mx_status = mx_video_input_set_trigger_mode( video_input_record,
				soft_area_detector->initial_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Zero out the ROI boundaries. */

	for ( i = 0; i < ad->maximum_num_rois; i++ ) {
		ad->roi_array[i][0] = 0;
		ad->roi_array[i][1] = 0;
		ad->roi_array[i][2] = 0;
		ad->roi_array[i][3] = 0;
	}

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

	soft_area_detector = NULL;

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

	soft_area_detector = NULL;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

	soft_area_detector->start_time = mx_current_clock_tick();

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CONTINUOUS:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
		soft_area_detector->sequence_in_progress = TRUE;
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

	soft_area_detector->sequence_in_progress = TRUE;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Start time = (%ld,%ld)", fname,
		soft_area_detector->start_time.high_order,
		soft_area_detector->start_time.low_order));

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

	soft_area_detector = NULL;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	soft_area_detector->sequence_in_progress = FALSE;

	mx_status = mx_video_input_stop(soft_area_detector->video_input_record);

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_abort()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	mx_status_type mx_status;

	soft_area_detector = NULL;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	soft_area_detector->sequence_in_progress = FALSE;

	mx_status = mx_video_input_abort(
			soft_area_detector->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_soft_area_detector_get_extended_status()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	MX_CLOCK_TICK current_time, clock_tick_difference;
	double seconds_difference;
	long num_frames;
	long total_num_frames;
	double exposure_time, frame_time;
	long sequence_type, num_parameters;
	double *parameter_array;
	mx_status_type mx_status;

	soft_area_detector = NULL;

	mx_status = mxd_soft_area_detector_get_pointers( ad,
						&soft_area_detector, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	ad->status = 0;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: sequence_in_progress = %d",
		fname, (int) soft_area_detector->sequence_in_progress));
#endif

	/* If the soft area detector says that a sequence is not in
	 * progress, then we can stop now.
	 */

	if ( soft_area_detector->sequence_in_progress == FALSE )
		return MX_SUCCESSFUL_RESULT;

#if 0
	/* If the video input says that it is busy, we mark the area
	 * detector as busy.
	 */

	mx_status = mx_video_input_is_busy(
			soft_area_detector->video_input_record, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}
#endif

	/* Get the sequence parameters. */

	sequence_type   = ad->sequence_parameters.sequence_type;
	num_parameters  = ad->sequence_parameters.num_parameters;
	parameter_array = ad->sequence_parameters.parameter_array;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: sequence_type = %ld, num_parameters = %ld",
		fname, sequence_type, num_parameters));
	MX_DEBUG(-2,("%s: array[0] = %g, array[1] = %g, array[2] = %g",
	    fname, parameter_array[0], parameter_array[1], parameter_array[2]));
#endif

	switch( sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
		num_frames    = 1;
		exposure_time = parameter_array[0];
		frame_time    = exposure_time;
		break;
	case MXT_SQ_MULTIFRAME:
	case MXT_SQ_CIRCULAR_MULTIFRAME:
		num_frames    = parameter_array[0];
		exposure_time = parameter_array[1];
		frame_time    = parameter_array[2];
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Sequence type %ld is not supported for software emulated "
		"area detector '%s'.", sequence_type, ad->record->name );
	}

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s: num_frames = %ld, exposure_time = %g, frame_time = %g",
		fname, num_frames, exposure_time, frame_time));
#endif

	/* How long has it been since the trigger time? */

	current_time = mx_current_clock_tick();

	clock_tick_difference = mx_subtract_clock_ticks( current_time,
					soft_area_detector->start_time );
					
	seconds_difference =
		mx_convert_clock_ticks_to_seconds( clock_tick_difference );

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s: current_time = (%lu,%lu), difference = (%lu,%lu), %g seconds",
	    fname, current_time.high_order, current_time.low_order,
	    clock_tick_difference.high_order, clock_tick_difference.low_order,
	    seconds_difference));
#endif

	if ( seconds_difference < 0.0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Somehow, the operating system is claiming that the current "
		"time (%lu,%lu) is before the start time (%lu,%lu) for "
		"area detector '%s'.",
			current_time.high_order,
			current_time.low_order,
			soft_area_detector->start_time.high_order,
			soft_area_detector->start_time.low_order,
			ad->record->name );
	}

	/* If the time difference is less than one exposure time, we
	 * unconditionally declare the area detector to be busy, the
	 * last_frame_number to be -1, and the total number of frames
	 * to be 0.
	 */
	
	if ( seconds_difference < exposure_time ) {

#if MXD_SOFT_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: before first exposure time.", fname));
#endif

		ad->last_frame_number = -1;

		ad->total_num_frames  = 0;

		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;

#if MXD_SOFT_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: (B) last_frame_number = %ld, "
		"total_num_frames = %ld, status = %#lx", fname,
		    ad->last_frame_number, ad->total_num_frames, ad->status));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, the first frame should have finished by now. */

	/* We now subtract the first exposure time from the
	 * 'seconds_difference' variable, since that makes
	 * the calculation of the frame number easier.
	 */

	seconds_difference -= exposure_time;

	/* Now we can calculate the total number of frames acquired. */

	total_num_frames = (long) ( seconds_difference / frame_time );

	/* What happens next depends on the type of sequence
	 * we are running.
	 */

	if ( sequence_type == MXT_SQ_ONE_SHOT ) {

#if MXD_SOFT_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: In One-shot mode.", fname));
#endif
		/* If we are in One-shot mode, the sequence is over. */

		ad->last_frame_number = 0;

		ad->total_num_frames = 1;

		soft_area_detector->sequence_in_progress = FALSE;
	} else
	if ( sequence_type == MXT_SQ_CONTINUOUS ) {

#if MXD_SOFT_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: In Continuous mode.", fname));
#endif
		/* For Continuous mode, the last frame number will always
		 * be zero after the first exposure time.
		 */

		ad->last_frame_number = 0;

		ad->total_num_frames = total_num_frames;

		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	} else
	if ( sequence_type == MXT_SQ_MULTIFRAME ) {

#if MXD_SOFT_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: In Multiframe mode.", fname));
#endif
		/* Multiframe mode. */

		if ( total_num_frames >= num_frames ) {

			ad->last_frame_number = num_frames - 1;

			ad->total_num_frames  = num_frames - 1;

			soft_area_detector->sequence_in_progress = FALSE;
		} else {
			ad->last_frame_number = total_num_frames;

			ad->total_num_frames  = total_num_frames;

			ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
		}
	} else
	if ( sequence_type == MXT_SQ_CIRCULAR_MULTIFRAME ) {

#if MXD_SOFT_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: In Circular Multiframe mode.", fname));
#endif
		/* In Circular Multiframe mode, the frame number wraps
		 * back to the beginning when you get to the end of the
		 * frame buffers.  Thus, the reported last frame number
		 * is the calculated last frame number modulo the number
		 * of frames.
		 */

		ad->last_frame_number = total_num_frames % num_frames;

		ad->total_num_frames  = total_num_frames;

		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	} else {
		/* We should never get here. */

		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Somehow, we got to a part of this function that we "
		"should not be able to get to." );
	}

#if MXD_SOFT_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: last_frame_number = %ld, status = %#lx",
		fname, ad->last_frame_number, ad->status));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_area_detector_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_soft_area_detector_readout_frame()";

	MX_SOFT_AREA_DETECTOR *soft_area_detector;
	mx_status_type mx_status;

	soft_area_detector = NULL;

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

	soft_area_detector = NULL;

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

	case MXLV_AD_BITS_PER_PIXEL:
		mx_status = mx_video_input_get_bits_per_pixel(
				video_input_record, &(ad->bits_per_pixel) );
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
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2, 4, 8, 16, 32, 64 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	soft_area_detector = NULL;

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
	case MXLV_AD_BINSIZE:
		mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

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
	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}

