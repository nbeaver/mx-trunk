/*
 * Name:    pr_area_detector.c
 *
 * Purpose: Functions used to process MX area detector record events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PR_AREA_DETECTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_socket.h"
#include "mx_image.h"
#include "mx_area_detector.h"

#include "mx_process.h"
#include "pr_handlers.h"

/*---------------------------------------------------------------------------*/

static void
mxp_area_detector_free_correction_struct( MX_AREA_DETECTOR *ad,
			MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *cm_ptr )
{
	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *cm;

	if ( cm_ptr != NULL ) {
		cm = cm_ptr;
	} else
	if ( ad != NULL ) {
		cm = ad->correction_measurement;
	} else {
		return;
	}

	if ( cm == NULL ) {
		return;
	}
	if ( cm->sum_array != NULL ) {
		free( cm->sum_array );
	}
	free( cm );

	if ( ad != NULL ) {
		ad->correction_measurement = NULL;
	}

	return;
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_measure_correction_frame_handler( MX_RECORD *record,
						MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxp_area_detector_measure_correction_frame_handler()";

	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *cm;
	MX_LIST_HEAD *list_head;
	MX_VIRTUAL_TIMER *oneshot_timer;
	void *void_image_data_pointer;
	double exposure_time, frame_time, detector_readout_time;
	long num_exposures, pixels_per_frame;
	size_t image_length;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, record->name ));

	MX_DEBUG(-2,("%s: type = %ld, time = %g, num measurements = %ld",
		fname, ad->correction_measurement_type,
		ad->correction_measurement_time,
		ad->num_correction_measurements ));
#endif
	if ( ad->image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The area detector is currently using an image format of %ld.  "
		"At present, only GREY16 image format is supported.",
			ad->image_format );
	}

	/* See if callbacks are enabled. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head->callback_pipe == NULL ) {
		/* We are not configured to handle callbacks, so we must
		 * wait for the correction measurement to finish.
		 */

		MX_DEBUG(-2,
	   ("%s: Performing synchronous correction frame measurement.", fname));

		mx_status = mx_area_detector_measure_correction_frame(
				record, ad->correction_measurement_type,
				ad->correction_measurement_time,
				ad->num_correction_measurements );

		return mx_status;
	}

	/* If we get here, the correction measurement will be handled
	 * by a callback.
	 */

	pixels_per_frame = ad->framesize[0] * ad->framesize[1]; 

	exposure_time = ad->correction_measurement_time;

	num_exposures = ad->num_correction_measurements;

	if ( num_exposures <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of exposures (%ld) requested for "
		"area detector '%s'.  The minimum number of exposures "
		"allowed is 1.",  num_exposures, record->name );
	}

	/* Allocate a structure to contain the current state of
	 * the correction measurement process.
	 */

	cm = malloc( sizeof(MX_AREA_DETECTOR_CORRECTION_MEASUREMENT) );

	if ( cm == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an "
		"MX_AREA_DETECTOR_CORRECTION_MEASUREMENT structure "
		"for record '%s'.", record->name );
	}

	memset( cm, 0, sizeof(MX_AREA_DETECTOR_CORRECTION_MEASUREMENT) );

	cm->area_detector = ad;

	/* The correction frames will originally be read into the image frame.
	 * We must make sure that the image frame is big enough to hold
	 * the image data.
	 */

	mx_status = mx_area_detector_setup_frame( record,
						&(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_free_correction_struct( NULL, cm );
		return mx_status;
	}

	/* Make sure that the destination image frame is already big enough
	 * to hold the image frame that we are going to put in it.
	 */

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
		mx_status = mx_area_detector_setup_frame( record,
						&(ad->dark_current_frame) );

		cm->destination_frame = ad->dark_current_frame;

		cm->desired_correction_flags = 0;
		break;
	
	case MXFT_AD_FLOOD_FIELD_FRAME:
		mx_status = mx_area_detector_setup_frame( record,
						&(ad->flood_field_frame) );

		cm->destination_frame = ad->flood_field_frame;

		cm->desired_correction_flags = MXFT_AD_DARK_CURRENT_FRAME;
		break;

	default:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type, record->name );
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_free_correction_struct( NULL, cm );
		return mx_status;
	}

	/* Get a pointer to the destination array. */

	mx_status = mx_image_get_image_data_pointer( cm->destination_frame,
						&image_length,
						&void_image_data_pointer );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_free_correction_struct( NULL, cm );
		return mx_status;
	}

	cm->destination_array = void_image_data_pointer;

	/* Allocate a double precision array to store intermediate sums. */

	cm->sum_array = calloc( pixels_per_frame, sizeof(double) );

	if ( cm->sum_array == (double *) NULL ) {
		mxp_area_detector_free_correction_struct( NULL, cm );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"a %ld element array of doubles.", pixels_per_frame );
	}

	/* Compute the time between the start of successive frames. */

	mx_status = mx_area_detector_get_detector_readout_time( record,
							&detector_readout_time);

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_free_correction_struct( NULL, cm );
		return mx_status;
	}

	frame_time = exposure_time + detector_readout_time;

	/* Setup a multiframe sequence that will acquire all of
	 * the frames needed to compute the correction frame.
	 */

	mx_status = mx_area_detector_set_multiframe_mode( record,
							num_exposures,
							exposure_time,
							frame_time );
	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_free_correction_struct( NULL, cm );
		return mx_status;
	}

	/* Put the detector into internal trigger mode. */

	mx_status = mx_area_detector_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_free_correction_struct( NULL, cm );
		return mx_status;
	}

	/* Create a callback message struct. */

	cm->callback = malloc( sizeof(MX_CALLBACK_MESSAGE) );

	if ( cm->callback == NULL ) {
		mxp_area_detector_free_correction_struct( NULL, cm );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_CALLBACK_MESSAGE "
		"structure for record '%s'.", record->name );
	}

	cm->callback->callback_type = MXCBT_FUNCTION;
	cm->callback->list_head = list_head;

	cm->callback->u.function.callback_function = NULL;
	cm->callback->u.function.callback_args = NULL;

	/* Create a one-shot interval timer that will arrange for the
	 * correction measurement callback function to be called later.
	 */

	mx_status = mx_virtual_timer_create(
				&oneshot_timer,
				list_head->master_timer,
				MXIT_ONE_SHOT_TIMER,
				mx_callback_standard_vtimer_handler,
				cm->callback );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_free_correction_struct( NULL, cm );
		return mx_status;
	}

	cm->callback->u.function.oneshot_timer = oneshot_timer;

	/* Save the correction measurement structure in the
	 * MX_AREA_DETECTOR structure.  This indicates that
	 * a correction measurement is in progress.
	 */

	ad->correction_measurement = cm;

	/* Start the measurement sequence. */

	mx_status = mx_area_detector_start( record );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_abort( record );

		mxp_area_detector_free_correction_struct( NULL, cm );

		return mx_status;
	}

	/* Start the callback virtual timer. */

	mx_status = mx_virtual_timer_start( oneshot_timer, 1.0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_abort( record );

		mxp_area_detector_free_correction_struct( NULL, cm );

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_readout_frame_handler( MX_RECORD *record,
					MX_RECORD_FIELD *record_field,
					MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxp_area_detector_readout_frame_handler()";

	MX_IMAGE_FRAME *image_frame;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s', field = '%s'",
			fname, record->name, record_field->name));

	mx_status = mx_area_detector_setup_frame( record, &(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_readout_frame( record, ad->readout_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the image_frame buffer pointer to point to the buffer in
	 * the frame that we just read in.
	 */

	image_frame = ad->image_frame;

	ad->image_frame_buffer = image_frame->image_data;

	MX_DEBUG( 2,("%s: bytes_per_frame = %ld, frame_buffer = %p",
		fname, ad->bytes_per_frame, ad->image_frame_buffer));

#if 0
	{
		int i;
		unsigned char c;

		for ( i = 0; i < 10; i++ ) {
			c = ad->image_frame_buffer[i];

			MX_DEBUG(-2,
			("%s: image_frame_buffer[%d] = %u", fname, i, c));
		}
	}
#endif

	/* Modify the 'image_frame_buffer' record field to have
	 * the correct number of array elements.
	 */

	mx_status = mx_set_1d_field_array_length_by_name( record,
				"image_frame_buffer", ad->bytes_per_frame );

	return mx_status;
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_get_roi_frame_handler( MX_RECORD *record,
				MX_RECORD_FIELD *record_field,
				MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxp_area_detector_get_roi_frame_handler()";

	MX_IMAGE_FRAME *roi_frame;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s', field = '%s'",
			fname, record->name, record_field->name));

	/* Check to see if a full image frame has been read from 
	 * the camera hardware yet.
	 */

	if ( ad->image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No image frame has been read from the camera hardware yet "
		"for area detector '%s', so we are not yet ready to return "
		"an ROI frame.", record->name );
	}

	/* Get the ROI frame from the full image frame. */

	mx_status = mx_area_detector_get_roi_frame( record, ad->image_frame,
						ad->get_roi_frame,
						&(ad->roi_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the ROI frame buffer pointer to point to the buffer in
	 * the ROI frame that we just copied in.
	 */

	roi_frame = ad->roi_frame;

	ad->roi_frame_buffer = roi_frame->image_data;

	MX_DEBUG( 2,("%s: roi_bytes_per_frame = %ld, roi_frame_buffer = %p",
		fname, ad->roi_bytes_per_frame, ad->roi_frame_buffer));

#if 0
	{
		int i;
		unsigned char c;

		for ( i = 0; i < 10; i++ ) {
			c = ad->roi_frame_buffer[i];

			MX_DEBUG(-2,
			("%s: roi_frame_buffer[%d] = %u", fname, i, c));
		}
	}
#endif

	/* Modify the 'roi_frame_buffer' record field to have the
	 * correct number of array elements.
	 */

	mx_status = mx_set_1d_field_array_length_by_name( record,
						"roi_frame_buffer",
						ad->roi_bytes_per_frame );
	return mx_status;
}

/*---------------------------------------------------------------------------*/

mx_status_type
mx_setup_area_detector_process_functions( MX_RECORD *record )
{
	static const char fname[] =
			"mx_setup_area_detector_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_AD_ABORT:
		case MXLV_AD_ARM:
		case MXLV_AD_BINSIZE:
		case MXLV_AD_BITS_PER_PIXEL:
		case MXLV_AD_BYTES_PER_FRAME:
		case MXLV_AD_BYTES_PER_PIXEL:
		case MXLV_AD_COPY_FRAME:
		case MXLV_AD_CORRECT_FRAME:
		case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		case MXLV_AD_EXTENDED_STATUS:
		case MXLV_AD_FRAMESIZE:
		case MXLV_AD_GET_ROI_FRAME:
		case MXLV_AD_IMAGE_FORMAT:
		case MXLV_AD_IMAGE_FORMAT_NAME:
		case MXLV_AD_IMAGE_FRAME_BUFFER:
		case MXLV_AD_LAST_FRAME_NUMBER:
		case MXLV_AD_LOAD_FRAME:
		case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		case MXLV_AD_READOUT_FRAME:
		case MXLV_AD_ROI:
		case MXLV_AD_ROI_FRAME_BUFFER:
		case MXLV_AD_SAVE_FRAME:
		case MXLV_AD_STATUS:
		case MXLV_AD_STOP:
		case MXLV_AD_TOTAL_NUM_FRAMES:
		case MXLV_AD_TRANSFER_FRAME:
		case MXLV_AD_TRIGGER:

#if PR_AREA_DETECTOR_DEBUG
		case MXLV_AD_ROI_NUMBER:
		case MXLV_AD_FRAME_FILENAME:
		case MXLV_AD_SEQUENCE_TYPE:
		case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
#endif
		case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
			record_field->process_function
					= mx_area_detector_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_area_detector_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_area_detector_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AREA_DETECTOR *ad;
	mx_status_type mx_status;

#if PR_AREA_DETECTOR_DEBUG
	unsigned long i;
#endif

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	ad = (MX_AREA_DETECTOR *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', field = '%s', operation = %d",
		fname, record->name, record_field->name, operation));
#endif

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_AD_BINSIZE:
			mx_status = mx_area_detector_get_binsize( record,
								NULL, NULL );
			break;
		case MXLV_AD_BITS_PER_PIXEL:
			mx_status = mx_area_detector_get_bits_per_pixel(
								record, NULL );
			break;
		case MXLV_AD_BYTES_PER_FRAME:
			mx_status = mx_area_detector_get_bytes_per_frame(
								record, NULL );
			break;
		case MXLV_AD_BYTES_PER_PIXEL:
			mx_status = mx_area_detector_get_bytes_per_pixel(
								record, NULL );
			break;
		case MXLV_AD_EXTENDED_STATUS:
			mx_status = mx_area_detector_get_extended_status(
						record, NULL, NULL, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			snprintf(
			    ad->extended_status, sizeof(ad->extended_status),
			    "%ld %ld %#lx",
					ad->last_frame_number,
					ad->total_num_frames,
					ad->status );

#if PR_AREA_DETECTOR_DEBUG
			MX_DEBUG(-2,("%s: extended_status = %s",
				fname, ad->extended_status));
#endif
			break;
		case MXLV_AD_IMAGE_FORMAT:
		case MXLV_AD_IMAGE_FORMAT_NAME:
			mx_status = 
			   mx_area_detector_get_image_format( record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_image_get_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
			break;
		case MXLV_AD_FRAMESIZE:
			mx_status = mx_area_detector_get_framesize( record,
								NULL, NULL );
			break;
		case MXLV_AD_IMAGE_FRAME_BUFFER:
			if ( ad->image_frame_buffer == NULL ) {
				return mx_error(MXE_INITIALIZATION_ERROR, fname,
			"Area detector '%s' has not yet taken its first frame.",
					record->name );
			}
			break;
		case MXLV_AD_LAST_FRAME_NUMBER:
			mx_status = mx_area_detector_get_last_frame_number(
								record, NULL );
			break;
		case MXLV_AD_MAXIMUM_FRAME_NUMBER:
			mx_status = mx_area_detector_get_maximum_frame_number(
								record, NULL );
			break;
		case MXLV_AD_MAXIMUM_FRAMESIZE:
			mx_status = mx_area_detector_get_maximum_framesize(
							record, NULL, NULL );
			break;
		case MXLV_AD_ROI:
			mx_status = mx_area_detector_get_roi( record,
							ad->roi_number, NULL );
#if PR_AREA_DETECTOR_DEBUG
			MX_DEBUG(-2,("%s: get ROI(%lu) = (%lu,%lu,%lu,%lu)",
				fname, ad->roi_number,
				ad->roi[0], ad->roi[1],
				ad->roi[2], ad->roi[3]));
#endif
			break;
		case MXLV_AD_ROI_FRAME_BUFFER:
			if ( ad->roi_frame_buffer == NULL ) {
				return mx_error(MXE_INITIALIZATION_ERROR, fname,
		"Area detector '%s' has not yet initialized its first ROI.",
					record->name );
			}
			break;
		case MXLV_AD_STATUS:
			mx_status = mx_area_detector_get_status( record, NULL );
			break;
		case MXLV_AD_TOTAL_NUM_FRAMES:
			mx_status = mx_area_detector_get_total_num_frames(
								record, NULL );
			break;

#if PR_AREA_DETECTOR_DEBUG
		case MXLV_AD_ROI_NUMBER:
			MX_DEBUG(-2,("%s: ROI number = %ld",
				fname, ad->roi_number));
			break;
		case MXLV_AD_SEQUENCE_TYPE:
			MX_DEBUG(-2,("%s: sequence type = %ld",
				fname, ad->sequence_parameters.sequence_type));
			break;
		case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
			MX_DEBUG(-2,("%s: num_parameters = %ld",
				fname, ad->sequence_parameters.num_parameters));
			break;
#endif
		case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
#if PR_AREA_DETECTOR_DEBUG
			for ( i = 0;
			    i < ad->sequence_parameters.num_parameters;
			    i++ )
			{
				MX_DEBUG(-2,("%s: parameter_array[%lu] = %g",
					    fname, i,
				   ad->sequence_parameters.parameter_array[i]));
			}
#endif
			break;
		default:
			MX_DEBUG(-1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_AD_ABORT:
			mx_status = mx_area_detector_abort( record );
			break;
		case MXLV_AD_ARM:
			mx_status = mx_area_detector_arm( record );
			break;
		case MXLV_AD_BINSIZE:
			mx_status = mx_area_detector_set_binsize( record,
						ad->binsize[0],
						ad->binsize[1] );
			break;
		case MXLV_AD_COPY_FRAME:
			mx_status = mx_area_detector_copy_frame( record,
					ad->copy_frame[0], ad->copy_frame[1] );
			break;
		case MXLV_AD_CORRECT_FRAME:
			mx_status = mx_area_detector_correct_frame( record );
			break;
		case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
#if 0
			mx_status = mx_area_detector_measure_correction_frame(
					record, ad->correction_measurement_type,
					ad->correction_measurement_time,
					ad->num_correction_measurements );
#else
			mx_status =
	    mxp_area_detector_measure_correction_frame_handler( record, ad );
#endif
			break;
		case MXLV_AD_FRAMESIZE:
			mx_status = mx_area_detector_set_framesize( record,
						ad->framesize[0],
						ad->framesize[1] );
			break;
		case MXLV_AD_GET_ROI_FRAME:
			mx_status = mxp_area_detector_get_roi_frame_handler(
					record, record_field, ad );
			break;
		case MXLV_AD_IMAGE_FORMAT_NAME:
			mx_status = mx_image_get_format_type_from_name(
				ad->image_format_name, &(ad->image_format) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Fall through to the next case. */
		case MXLV_AD_IMAGE_FORMAT:
			mx_status = mx_area_detector_set_image_format( record,
							ad->image_format );
			break;
		case MXLV_AD_LOAD_FRAME:
			mx_status = mx_area_detector_load_frame( record,
					ad->load_frame, ad->frame_filename );
			break;
		case MXLV_AD_ROI:
			mx_status = mx_area_detector_set_roi( record,
						ad->roi_number, ad->roi );
#if PR_AREA_DETECTOR_DEBUG
			MX_DEBUG(-2,("%s: set ROI(%lu) = (%lu,%lu,%lu,%lu)",
				fname, ad->roi_number,
				ad->roi[0], ad->roi[1],
				ad->roi[2], ad->roi[3]));
#endif
			break;
		case MXLV_AD_READOUT_FRAME:
			mx_status = mxp_area_detector_readout_frame_handler(
					record, record_field, ad );
			break;
		case MXLV_AD_SAVE_FRAME:
			mx_status = mx_area_detector_save_frame( record,
					ad->save_frame, ad->frame_filename );
			break;
		case MXLV_AD_STOP:
			mx_status = mx_area_detector_stop( record );
			break;
		case MXLV_AD_TRANSFER_FRAME:
			mx_status = mx_area_detector_transfer_frame( record,
						ad->transfer_frame, NULL );
			break;
		case MXLV_AD_TRIGGER:
			mx_status = mx_area_detector_trigger( record );
			break;

#if PR_AREA_DETECTOR_DEBUG
		case MXLV_AD_ROI_NUMBER:
			MX_DEBUG(-2,("%s: ROI number = %ld",
				fname, ad->roi_number));
			break;
		case MXLV_AD_FRAME_FILENAME:
			MX_DEBUG(-2,("%s: Frame filename = '%s'",
				fname, ad->frame_filename));
			break;
		case MXLV_AD_SEQUENCE_TYPE:
			MX_DEBUG(-2,("%s: sequence type = %ld",
				fname, ad->sequence_parameters.sequence_type));
			break;
		case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
			MX_DEBUG(-2,("%s: num_parameters = %ld",
				fname, ad->sequence_parameters.num_parameters));
			break;
#endif
		case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
#if PR_AREA_DETECTOR_DEBUG
			for ( i = 0;
			    i < ad->sequence_parameters.num_parameters;
			    i++ )
			{
				MX_DEBUG(-2,("%s: parameter_array[%lu] = %g",
					    fname, i,
				   ad->sequence_parameters.parameter_array[i]));
			}
#endif
			mx_status = mx_area_detector_set_sequence_parameters(
					record, &(ad->sequence_parameters) );
			break;
		default:
			MX_DEBUG(-1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

