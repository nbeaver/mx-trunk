/*
 * Name:    pr_area_detector.c
 *
 * Purpose: Functions used to process MX area detector record events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PR_AREA_DETECTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_hrt_debug.h"
#include "mx_socket.h"
#include "mx_image.h"
#include "mx_area_detector.h"

#include "mx_process.h"
#include "pr_handlers.h"

/*---------------------------------------------------------------------------*/

static void
mxp_area_detector_cleanup_after_correction( MX_AREA_DETECTOR *ad,
			MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr_ptr )
{
	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr;

	if ( corr_ptr != NULL ) {
		corr = corr_ptr;
	} else
	if ( ad != NULL ) {
		corr = ad->correction_measurement;
	} else {
		ad->correction_measurement_in_progress = FALSE;
		return;
	}

	if ( corr == NULL ) {
		ad->correction_measurement_in_progress = FALSE;
		return;
	}

	if ( ad == NULL ) {
		ad = corr->area_detector;
	}

	/* Enable the area detector shutter. */

	if ( ( ad != NULL )
	  && ( ad->correction_measurement_type == MXFT_AD_DARK_CURRENT_FRAME ) )
	{
		(void) mx_area_detector_set_shutter_enable( ad->record, TRUE );
	}

	/* Free correction measurement data structures. */

#if MX_AREA_DETECTOR_USE_DEZINGER
	if ( corr->dezinger_frame_array != (MX_IMAGE_FRAME **) NULL )
	{
		long n;

		for ( n = 0; n < corr->num_exposures; n++ ) {
			mx_image_free( corr->dezinger_frame_array[n] );
		}

		free( corr->dezinger_frame_array );
	}
#else /* not MX_AREA_DETECTOR_USE_DEZINGER */

	if ( corr->sum_array != NULL ) {
		free( corr->sum_array );
	}

#endif /* MX_AREA_DETECTOR_USE_DEZINGER */

	if ( ad == NULL ) {
		if ( corr->area_detector != NULL ) {
			corr->area_detector->correction_measurement = NULL;
		}
	} else {
		ad->correction_measurement = NULL;
	}

	free( corr );

	ad->correction_measurement_in_progress = FALSE;

	return;
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_measure_correction_callback_function(
				MX_CALLBACK_MESSAGE *callback_message )
{
	static const char fname[] =
		"mxp_area_detector_measure_correction_callback_function()";

	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr;
	MX_AREA_DETECTOR *ad;
	MX_IMAGE_FRAME *dest_frame;
	long pixels_per_frame;
	long last_frame_number, total_num_frames, num_frames_difference;
	unsigned long ad_status, saved_correction_flags;
	time_t time_buffer;
	struct timespec exposure_timespec;
	mx_bool_type sequence_complete;
	mx_status_type mx_status, mx_status2;

#if MX_AREA_DETECTOR_USE_DEZINGER
#   if PR_AREA_DETECTOR_DEBUG
	MX_HRT_TIMING measurement;
#   endif
#else
	void *void_image_data_pointer;
	double *sum_array;
	uint16_t *src_array, *dest_array;
	double num_exposures_as_double, temp_double;
	long i;
	size_t image_length;
#endif

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
	MX_DEBUG(-2,("%s invoked for callback message %p",
		fname, callback_message));
#endif

	corr = callback_message->u.function.callback_args;

	ad = corr->area_detector;

	pixels_per_frame = ad->framesize[0] * ad->framesize[1];

	/* See if any new frames are ready to be read out. */

	mx_status = mx_area_detector_get_extended_status( ad->record,
							&last_frame_number,
							&total_num_frames,
							&ad_status );
	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( ad, corr );
		return mx_status;
	}

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: last_frame_number = %ld, old_last_frame_number = %ld",
		fname, last_frame_number, corr->old_last_frame_number));
	MX_DEBUG(-2,("%s: total_num_frames = %ld, old_total_num_frames = %ld",
		fname, total_num_frames, corr->old_total_num_frames));
	MX_DEBUG(-2,("%s: ad_status = %#lx, old_status = %#lx",
		fname, ad_status, corr->old_status));
#endif

	num_frames_difference = total_num_frames - corr->old_total_num_frames;

	corr->num_unread_frames += num_frames_difference;

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: num_frames_difference = %ld",
		fname, num_frames_difference ));
	MX_DEBUG(-2,("%s: num_unread_frames = %ld",
		fname, corr->num_unread_frames ));
#endif

	/* Readout the frames and add them to either sum_array
	 * or dezinger_frame_array (depending on the value of
	 * the MX_AREA_DETECTOR_USE_DEZINGER macro).
	 */

	sequence_complete = FALSE;

	while( corr->num_unread_frames > 0 ) {

		/* Readout the frame into ad->image_frame. */

#if PR_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Reading frame %ld",
				fname, corr->num_frames_read));
#endif

		mx_status = mx_area_detector_readout_frame(
				ad->record, corr->num_frames_read );

		if ( mx_status.code != MXE_SUCCESS ) {
			mxp_area_detector_cleanup_after_correction( ad, corr );
			return mx_status;
		}

		/* Perform any necessary image corrections. */

		if ( corr->desired_correction_flags != 0 ) {
			mx_status = mx_area_detector_get_correction_flags(
					ad->record, &saved_correction_flags );

			if ( mx_status.code != MXE_SUCCESS ) {
				mxp_area_detector_cleanup_after_correction(
								ad, corr );
				return mx_status;
			}

			mx_status = mx_area_detector_set_correction_flags(
				ad->record, corr->desired_correction_flags );

			if ( mx_status.code != MXE_SUCCESS ) {
				mxp_area_detector_cleanup_after_correction(
								ad, corr );
				return mx_status;
			}

#if PR_AREA_DETECTOR_DEBUG
			MX_DEBUG(-2,("%s: Correcting frame %ld",
					fname, corr->num_frames_read));
#endif

			mx_status = mx_area_detector_correct_frame(ad->record);

			mx_status2 = mx_area_detector_set_correction_flags(
					ad->record, saved_correction_flags );

			if ( mx_status2.code != MXE_SUCCESS ) {
				mxp_area_detector_cleanup_after_correction(
								ad, corr );
				return mx_status2;
			}

			if ( mx_status.code != MXE_SUCCESS ) {
				mxp_area_detector_cleanup_after_correction(
								ad, corr );
				return mx_status;
			}
		}

#if MX_AREA_DETECTOR_USE_DEZINGER
		/* Copy the image frame to the dezinger frame array. */

		mx_status = mx_image_copy_frame( ad->image_frame,
		      &(corr->dezinger_frame_array[ corr->num_frames_read ]) );

		if ( mx_status.code != MXE_SUCCESS ) {
			mxp_area_detector_cleanup_after_correction( ad, corr );
			return mx_status;
		}

#else /* not MX_AREA_DETECTOR_USE_DEZINGER */

		/* Get the image data pointer for this image. */

		mx_status = mx_image_get_image_data_pointer( ad->image_frame,
						&image_length,
						&void_image_data_pointer );

		if ( mx_status.code != MXE_SUCCESS ) {
			mxp_area_detector_cleanup_after_correction( ad, corr );
			return mx_status;
		}

		src_array = void_image_data_pointer;

		/* Add the pixels in this image to the sum array. */

#if PR_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Adding frame %ld pixels to sum array.",
			fname, corr->num_frames_read));
#endif

		sum_array = corr->sum_array;

		for ( i = 0; i < pixels_per_frame; i++ ) {
			sum_array[i] += (double) src_array[i];
		}

#endif /* MX_AREA_DETECTOR_USE_DEZINGER */

		corr->num_frames_read++;
		corr->num_unread_frames--;

		if (corr->num_frames_read >= ad->num_correction_measurements)
		{
			sequence_complete = TRUE;
			break;		/* Exit the while() loop. */
		}
	}

	/* See if the correction sequence is complete. */

	if ( sequence_complete == FALSE ) {

		/* If the correction measurements are not complete, then
		 * we must save the current state and them reschedule the
		 * virtual timer to be called again.
		 */

		corr->old_last_frame_number = last_frame_number;
		corr->old_total_num_frames = total_num_frames;
		corr->old_status = ad_status;

		/* Restart the callback virtual timer. */

		mx_status = mx_virtual_timer_start(
			callback_message->u.function.oneshot_timer,
			callback_message->u.function.callback_interval );

		if ( mx_status.code != MXE_SUCCESS ) {
			mxp_area_detector_cleanup_after_correction( ad, corr );
		}

#if PR_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Callback virtual timer restarted.",fname));
		MX_DEBUG(-2,
		("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"));
#endif
		/* Return, knowing that we will be called again soon. */

		return mx_status;
	}

	/* The correction sequence is complete. */

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
		dest_frame = ad->dark_current_frame;
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		dest_frame = ad->flood_field_frame;
		break;

	default:
		mxp_area_detector_cleanup_after_correction( ad, corr );
			
		return mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type,
			ad->record->name );
	}

#if MX_AREA_DETECTOR_USE_DEZINGER

#if PR_AREA_DETECTOR_DEBUG
	MX_HRT_START( measurement );
#endif

	mx_status = mx_image_dezinger( &(corr->destination_frame),
					corr->num_exposures,
					corr->dezinger_frame_array,
					fabs( ad->dezinger_threshold ) );

#if PR_AREA_DETECTOR_DEBUG
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "Total image dezingering time." );
#endif

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( ad, corr );
		return mx_status;
	}

#else /* not MX_AREA_DETECTOR_USE_DEZINGER */

	/* Get a pointer to the destination array. */

	mx_status = mx_image_get_image_data_pointer( dest_frame,
						&image_length,
						&void_image_data_pointer );
		
	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( ad, corr );
		return mx_status;
	}

	dest_array = void_image_data_pointer;

	/* Copy normalized pixels to the destination array. */

	num_exposures_as_double = (double) corr->num_exposures;

	for ( i = 0; i < pixels_per_frame; i++ ) {
		temp_double = sum_array[i] / num_exposures_as_double;

		dest_array[i] = mx_round( temp_double );
	}

#endif /* not MX_AREA_DETECTOR_USE_DEZINGER */

	/* If requested, perform a delayed geometrical correction on a
	 * flood field frame after averaging or dezingering the source
	 * frames.
	 */

	if ( ( ad->correction_measurement_type == MXFT_AD_FLOOD_FIELD_FRAME )
	  && ( ad->correction_frame_geom_corr_last == TRUE )
	  && ( ad->correction_frame_no_geom_corr == FALSE ) )
	{
		MX_AREA_DETECTOR_FUNCTION_LIST *flist;
		mx_status_type (*geometrical_correction_fn)( MX_AREA_DETECTOR *,
							MX_IMAGE_FRAME * );

		mx_status = mx_area_detector_get_pointers( ad->record,
							NULL, &flist, fname );

		if ( mx_status.code != MXE_SUCCESS ) {
			mxp_area_detector_cleanup_after_correction( ad, corr );
			return mx_status;
		}

		geometrical_correction_fn = flist->geometrical_correction;

		if ( geometrical_correction_fn == NULL ) {
			geometrical_correction_fn =
				mx_area_detector_default_geometrical_correction;
		}

		MX_DEBUG(-2,("%s: Invoking delayed geometrical correction "
				"on measured flood field frame.", fname ));

		mx_status = (*geometrical_correction_fn)( ad,
						corr->destination_frame );

		if ( mx_status.code != MXE_SUCCESS ) {
			mxp_area_detector_cleanup_after_correction( ad, corr );
			return mx_status;
		}
	}

	/* Set the image frame exposure time. */

	exposure_timespec =
	    mx_convert_seconds_to_high_resolution_time( corr->exposure_time );

	MXIF_EXPOSURE_TIME_SEC(corr->destination_frame)
						= exposure_timespec.tv_sec;

	MXIF_EXPOSURE_TIME_NSEC(corr->destination_frame)
						= exposure_timespec.tv_nsec;

	/* Set the image frame timestamp to the current time. */

	MXIF_TIMESTAMP_SEC(corr->destination_frame) = time( &time_buffer );

	MXIF_TIMESTAMP_NSEC(corr->destination_frame) = 0;

	/* Return with the satisfaction of a job well done. */

	mxp_area_detector_cleanup_after_correction( ad, corr );

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Correction sequence complete.", fname));
	MX_DEBUG(-2,
		("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"));
#endif

	return MX_SUCCESSFUL_RESULT;;
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_measure_correction_frame_handler( MX_RECORD *record,
						MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxp_area_detector_measure_correction_frame_handler()";

	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr;
	MX_LIST_HEAD *list_head;
	MX_VIRTUAL_TIMER *oneshot_timer;
	double frame_time, modified_frame_time;
	double detector_readout_time;
	long pixels_per_frame;
	mx_status_type mx_status;

#if (MX_AREA_DETECTOR_USE_DEZINGER == FALSE )
	void *void_image_data_pointer;
	size_t image_length;
#endif

#if PR_AREA_DETECTOR_DEBUG
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

#if PR_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
	   ("%s: Performing synchronous correction frame measurement.", fname));
#endif

		mx_status = mx_area_detector_measure_correction_frame(
				record, ad->correction_measurement_type,
				ad->correction_measurement_time,
				ad->num_correction_measurements );

		return mx_status;
	}

	/* If we get here, the correction measurement will be handled
	 * by a callback.
	 */

	/* Allocate a structure to contain the current state of
	 * the correction measurement process.
	 */

	corr = malloc( sizeof(MX_AREA_DETECTOR_CORRECTION_MEASUREMENT) );

	if ( corr == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an "
		"MX_AREA_DETECTOR_CORRECTION_MEASUREMENT structure "
		"for record '%s'.", record->name );
	}

	memset( corr, 0, sizeof(MX_AREA_DETECTOR_CORRECTION_MEASUREMENT) );

	corr->area_detector = ad;

	corr->exposure_time = ad->correction_measurement_time;

	corr->num_exposures = ad->num_correction_measurements;

	if ( corr->num_exposures <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of exposures (%ld) requested for "
		"area detector '%s'.  The minimum number of exposures "
		"allowed is 1.",  corr->num_exposures, record->name );
	}

	pixels_per_frame = ad->framesize[0] * ad->framesize[1]; 

	/* Set a flag that says a correction frame measurement is in progress.*/

	ad->correction_measurement_in_progress = TRUE;

	/* The correction frames will originally be read into the image frame.
	 * We must make sure that the image frame is big enough to hold
	 * the image data.
	 */

	mx_status = mx_area_detector_setup_frame( record,
						&(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	/* Make sure that the destination image frame is already big enough
	 * to hold the image frame that we are going to put in it.
	 */

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
		mx_status = mx_area_detector_setup_frame( record,
						&(ad->dark_current_frame) );

		corr->destination_frame = ad->dark_current_frame;

		corr->desired_correction_flags =
			MXFT_AD_MASK_FRAME | MXFT_AD_BIAS_FRAME;
		break;
	
	case MXFT_AD_FLOOD_FIELD_FRAME:
		mx_status = mx_area_detector_setup_frame( record,
						&(ad->flood_field_frame) );

		corr->destination_frame = ad->flood_field_frame;

		corr->desired_correction_flags = 
	  MXFT_AD_MASK_FRAME | MXFT_AD_BIAS_FRAME | MXFT_AD_DARK_CURRENT_FRAME;

	  	if ( ( ad->geom_corr_after_flood == FALSE )
		  && ( ad->correction_frame_geom_corr_last == FALSE )
		  && ( ad->correction_frame_no_geom_corr == FALSE ) )
		{
			corr->desired_correction_flags
				|= MXFT_AD_GEOMETRICAL_CORRECTION;
		}
		break;

	default:
		corr->destination_frame = NULL;

		corr->desired_correction_flags = 0;

		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type, record->name );
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

#if MX_AREA_DETECTOR_USE_DEZINGER

	corr->dezinger_frame_array =
		calloc( corr->num_exposures, sizeof(MX_IMAGE_FRAME *) );

	if ( corr->dezinger_frame_array == (MX_IMAGE_FRAME **) NULL ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"a %ld element array of MX_IMAGE_FRAME pointers.", 
			corr->num_exposures );
	}

#else /* not MX_AREA_DETECTOR_USE_DEZINGER */

	/* Get a pointer to the destination array. */

	mx_status = mx_image_get_image_data_pointer( corr->destination_frame,
						&image_length,
						&void_image_data_pointer );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	corr->destination_array = void_image_data_pointer;

	/* Allocate a double precision array to store intermediate sums. */

	corr->sum_array = calloc( pixels_per_frame, sizeof(double) );

	if ( corr->sum_array == (double *) NULL ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"a %ld element array of doubles.", pixels_per_frame );
	}

#endif /* MX_AREA_DETECTOR_USE_DEZINGER */

	/* Get the detector readout time for the current sequence. */

	mx_status = mx_area_detector_get_detector_readout_time( record,
							&detector_readout_time);

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	/* Compute the time between the start of successive frames. */

	frame_time = corr->exposure_time + detector_readout_time;

	/* Add an extra 10 milliseconds to the end of the frame time
	 * to make sure that there is a non-zero time interval between
	 * the end of one frame and the start of the next frame.
	 */

	modified_frame_time = frame_time + 0.01;

	/* Setup a multiframe sequence that will acquire all of
	 * the frames needed to compute the correction frame.
	 */

	mx_status = mx_area_detector_set_multiframe_mode( record,
							corr->num_exposures,
							corr->exposure_time,
							modified_frame_time );
	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	/* Put the detector into internal trigger mode. */

	mx_status = mx_area_detector_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	/* Create a callback message struct. */

	corr->callback_message = malloc( sizeof(MX_CALLBACK_MESSAGE) );

	if ( corr->callback_message == NULL ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_CALLBACK_MESSAGE "
		"structure for record '%s'.", record->name );
	}

	corr->callback_message->callback_type = MXCBT_FUNCTION;
	corr->callback_message->list_head = list_head;

	corr->callback_message->u.function.callback_function = 
		mxp_area_detector_measure_correction_callback_function;

	corr->callback_message->u.function.callback_args = corr;

	/* Specify the callback interval in seconds. */

	corr->callback_message->u.function.callback_interval = 1.0;

	/* Create a one-shot interval timer that will arrange for the
	 * correction measurement callback function to be called later.
	 */

	mx_status = mx_virtual_timer_create(
				&oneshot_timer,
				list_head->master_timer,
				MXIT_ONE_SHOT_TIMER,
				mx_callback_standard_vtimer_handler,
				corr->callback_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	corr->callback_message->u.function.oneshot_timer = oneshot_timer;

	/* Get the current values of 'last_frame_number'
	 * and 'total_num_frames'.
	 */

	mx_status = mx_area_detector_get_extended_status( record,
						&(corr->old_last_frame_number),
						&(corr->old_total_num_frames),
						&(corr->old_status) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_area_detector_cleanup_after_correction( NULL, corr );

		return mx_status;
	}

	corr->num_frames_read = 0;
	corr->num_unread_frames = 0;

	/* Save the correction measurement structure in the
	 * MX_AREA_DETECTOR structure.  This indicates that
	 * a correction measurement is in progress.
	 */

	ad->correction_measurement = corr;

	/* If this is a dark current measurement, then disable the shutter. */

	if ( ad->correction_measurement_type == MXFT_AD_DARK_CURRENT_FRAME ) {

		mx_status = mx_area_detector_set_shutter_enable(record, FALSE);

		if ( mx_status.code != MXE_SUCCESS ) {
			mxp_area_detector_cleanup_after_correction(NULL, corr);

			return mx_status;
		}
	}

	/* Start the measurement sequence. */

	mx_status = mx_area_detector_start( record );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_abort( record );

		mxp_area_detector_cleanup_after_correction( NULL, corr );

		return mx_status;
	}

	/* Start the callback virtual timer. */

	mx_status = mx_virtual_timer_start( oneshot_timer,
		corr->callback_message->u.function.callback_interval );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_abort( record );

		mxp_area_detector_cleanup_after_correction( NULL, corr );

		return mx_status;
	}

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Callback virtual timer started.", fname));
	MX_DEBUG(-2,("******************************************************"));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_update_frame_pointers( MX_AREA_DETECTOR *ad )
{
	MX_IMAGE_FRAME *image_frame;
	mx_status_type mx_status;

	image_frame = ad->image_frame;

	if ( image_frame == NULL ) {
		ad->image_frame_header_length = 0;
		ad->image_frame_header        = NULL;
		ad->image_frame_data          = NULL;
	} else {
		ad->image_frame_header_length = image_frame->header_length;
		ad->image_frame_header = (char *) image_frame->header_data;
		ad->image_frame_data          = image_frame->image_data;
	}

	/* Modify the 'image_frame_header' record field to have
	 * the correct length in bytes.
	 */

	mx_status = mx_set_1d_field_array_length_by_name( ad->record,
				"image_frame_header",
				ad->image_frame_header_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Modify the 'image_frame_data' record field to have
	 * the correct length in bytes.
	 */

	mx_status = mx_set_1d_field_array_length_by_name( ad->record,
				"image_frame_data",
				ad->bytes_per_frame );

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

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', field = '%s'",
			fname, record->name, record_field->name));
#endif

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

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: roi_bytes_per_frame = %ld, roi_frame_buffer = %p",
		fname, ad->roi_bytes_per_frame, ad->roi_frame_buffer));
#endif

#if PR_AREA_DETECTOR_DEBUG
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

#if PR_AREA_DETECTOR_DEBUG
	static const char fname[] =
			"mx_setup_area_detector_process_functions()";
#endif

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

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
		case MXLV_AD_DETECTOR_READOUT_TIME:
		case MXLV_AD_EXTENDED_STATUS:
		case MXLV_AD_FRAMESIZE:
		case MXLV_AD_GET_ROI_FRAME:
		case MXLV_AD_IMAGE_FORMAT:
		case MXLV_AD_IMAGE_FORMAT_NAME:
		case MXLV_AD_IMAGE_FRAME_DATA:
		case MXLV_AD_IMAGE_FRAME_HEADER:
		case MXLV_AD_IMAGE_FRAME_HEADER_LENGTH:
		case MXLV_AD_LAST_FRAME_NUMBER:
		case MXLV_AD_LOAD_FRAME:
		case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		case MXLV_AD_READOUT_FRAME:
		case MXLV_AD_REGISTER_VALUE:
		case MXLV_AD_ROI:
		case MXLV_AD_ROI_FRAME_BUFFER:
		case MXLV_AD_SAVE_FRAME:
		case MXLV_AD_SEQUENCE_START_DELAY:
		case MXLV_AD_SHUTTER_ENABLE:
		case MXLV_AD_STATUS:
		case MXLV_AD_STOP:
		case MXLV_AD_TOTAL_ACQUISITION_TIME:
		case MXLV_AD_TOTAL_NUM_FRAMES:
		case MXLV_AD_TOTAL_SEQUENCE_TIME:
		case MXLV_AD_TRANSFER_FRAME:
		case MXLV_AD_TRIGGER:
		case MXLV_AD_TRIGGER_MODE:

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
		case MXLV_AD_DETECTOR_READOUT_TIME:
			mx_status = mx_area_detector_get_detector_readout_time(
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
		case MXLV_AD_IMAGE_FRAME_DATA:
			if ( ad->image_frame_data == NULL ) {
				return mx_error(MXE_INITIALIZATION_ERROR, fname,
			"Area detector '%s' has not yet taken its first frame.",
					record->name );
			}
			break;
		case MXLV_AD_IMAGE_FRAME_HEADER:
			if ( ad->image_frame_header == NULL ) {
				return mx_error(MXE_INITIALIZATION_ERROR, fname,
			"Area detector '%s' has not yet taken its first frame.",
					record->name );
			}
			break;
		case MXLV_AD_IMAGE_FRAME_HEADER_LENGTH:
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
		case MXLV_AD_REGISTER_VALUE:
			mx_status = mx_area_detector_get_register( record,
						ad->register_name, NULL );
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
		case MXLV_AD_SEQUENCE_START_DELAY:
			mx_status = mx_area_detector_get_sequence_start_delay(
								record, NULL );
			break;
		case MXLV_AD_SHUTTER_ENABLE:
			mx_status = mx_area_detector_get_shutter_enable(
								record, NULL );
			break;
		case MXLV_AD_STATUS:
			mx_status = mx_area_detector_get_status( record, NULL );
			break;
		case MXLV_AD_TOTAL_ACQUISITION_TIME:
			mx_status = mx_area_detector_get_total_acquisition_time(
								record, NULL );
			break;
		case MXLV_AD_TOTAL_NUM_FRAMES:
			mx_status = mx_area_detector_get_total_num_frames(
								record, NULL );
			break;
		case MXLV_AD_TOTAL_SEQUENCE_TIME:
			mx_status = mx_area_detector_get_total_sequence_time(
								record, NULL );
			break;
		case MXLV_AD_TRIGGER_MODE:
			mx_status = mx_area_detector_get_trigger_mode(
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
			MX_DEBUG( 1,(
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

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ad->copy_frame[1] == MXFT_AD_IMAGE_FRAME ) {
				mx_status =
				    mxp_area_detector_update_frame_pointers(ad);
			}
			break;
		case MXLV_AD_CORRECT_FRAME:
			mx_status = mx_area_detector_correct_frame( record );

			break;
		case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
			mx_status =
	    mxp_area_detector_measure_correction_frame_handler( record, ad );

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

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ad->load_frame == MXFT_AD_IMAGE_FRAME ) {
				mx_status =
				    mxp_area_detector_update_frame_pointers(ad);
			}
			break;
		case MXLV_AD_REGISTER_VALUE:
			mx_status = mx_area_detector_set_register( record,
			    ad->register_name, ad->register_value );
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
			mx_status = mx_area_detector_setup_frame( record,
							&(ad->image_frame) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_area_detector_readout_frame( record,
							ad->readout_frame );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxp_area_detector_update_frame_pointers(ad);
			break;
		case MXLV_AD_SAVE_FRAME:
			mx_status = mx_area_detector_save_frame( record,
					ad->save_frame, ad->frame_filename );
			break;
		case MXLV_AD_SEQUENCE_START_DELAY:
			mx_status = mx_area_detector_set_sequence_start_delay(
					record, ad->sequence_start_delay );
			break;
		case MXLV_AD_SHUTTER_ENABLE:
			mx_status = mx_area_detector_set_shutter_enable(
						record, ad->shutter_enable );
			break;
		case MXLV_AD_STOP:
			mx_status = mx_area_detector_stop( record );
			break;
		case MXLV_AD_TRANSFER_FRAME:
			mx_status = mx_area_detector_transfer_frame( record,
					ad->transfer_frame, ad->image_frame );
			break;
		case MXLV_AD_TRIGGER:
			mx_status = mx_area_detector_trigger( record );
			break;
		case MXLV_AD_TRIGGER_MODE:
			mx_status = mx_area_detector_set_trigger_mode(
						record, ad->trigger_mode );
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
								record, NULL );
			break;
		default:
			MX_DEBUG( 1,(
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

