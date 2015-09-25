/*
 * Name:    pr_area_detector.c
 *
 * Purpose: Functions used to process MX area detector record events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2006-2009, 2011-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PR_AREA_DETECTOR_DEBUG_CORRECTION		FALSE

#define PR_AREA_DETECTOR_DEBUG_IMAGE_FRAME_DATA		FALSE

#define PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION	FALSE

#define PR_AREA_DETECTOR_DEBUG_PROCESS			FALSE

#define PR_AREA_DETECTOR_DEBUG_ROI			FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_cfn.h"
#include "mx_hrt.h"
#include "mx_hrt_debug.h"
#include "mx_socket.h"
#include "mx_vm_alloc.h"
#include "mx_image.h"
#include "mx_area_detector.h"

#include "mx_process.h"
#include "pr_handlers.h"

/*---------------------------------------------------------------------------*/

#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION

int mx_global_debug_initialized[10] = {FALSE};
void *mx_global_debug_pointer[10] = {NULL};

#endif

/*---------------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_measure_correction_callback_function(
				MX_CALLBACK_MESSAGE *callback_message )
{
	static const char fname[] =
		"mxp_area_detector_measure_correction_callback_function()";

	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr;
	MX_AREA_DETECTOR *ad;
	MX_IMAGE_FRAME **dezinger_frame_ptr;
	long pixels_per_frame;
	long last_frame_number, total_num_frames, num_frames_difference;
	long frame_number_to_read;
	unsigned long ad_status, busy, ad_flags;
	mx_bool_type sequence_complete, start_detector;
	mx_status_type mx_status;

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
	MX_DEBUG(-2,("%s invoked for callback message %p",
		fname, callback_message));
#endif

#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	MX_DEBUG(-2,("%s: MARKER #1", fname));
#endif
	corr = callback_message->u.function.callback_args;

#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	MX_DEBUG(-2,("%s: corr = %p, global[1] = %p",
		fname, corr, mx_global_debug_pointer[1]));
	mx_vm_show_os_info( stderr, corr, sizeof(corr) );
#endif

	ad = corr->area_detector;

#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	MX_DEBUG(-2,("%s: ad = %p, global[0] = %p",
		fname, ad, mx_global_debug_pointer[0]));

	mx_vm_show_os_info( stderr, ad, sizeof(ad) );
	mx_heap_check();
	MX_DEBUG(-2,("%s: ad->record = %p", fname, ad->record));
	mx_vm_show_os_info( stderr, ad->record, sizeof(MX_RECORD *) );
	MX_DEBUG(-2,("%s: ad->record->name = '%s'", fname, ad->record->name));
#endif

	pixels_per_frame = ad->framesize[0] * ad->framesize[1];

	MXW_UNUSED( pixels_per_frame );

	/* See if any new frames are ready to be read out. */

	mx_status = mx_area_detector_get_extended_status( ad->record,
							&last_frame_number,
							&total_num_frames,
							&ad_status );
#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	MX_DEBUG(-2,("%s: MARKER #2", fname));
	mx_heap_check();
	MX_DEBUG(-2,("%s: ad->record = %p", fname, ad->record));
	mx_vm_show_os_info( stderr, ad->record, sizeof(MX_RECORD *) );
	MX_DEBUG(-2,("%s: ad->record->name = '%s'", fname, ad->record->name));
#endif

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( ad, corr );
		return mx_status;
	}

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: last_frame_number = %ld, old_last_frame_number = %ld",
		fname, last_frame_number, corr->old_last_frame_number));
	MX_DEBUG(-2,("%s: total_num_frames = %ld, old_total_num_frames = %ld",
		fname, total_num_frames, corr->old_total_num_frames));
	MX_DEBUG(-2,("%s: ad_status = %#lx, old_status = %#lx",
		fname, ad_status, corr->old_status));
#endif

	num_frames_difference = total_num_frames - corr->old_total_num_frames;

	/* Some sites (SOLEIL) want to skip over the first few frames.  This
	 * is most easily done by adding corr->raw_num_exposures_to_skip to
	 * the value of corr->old_total_num_frames in the function 
	 * mxp_area_detector_measure_correction_frame_handler().
	 *
	 * If corr->raw_num_exposures_to_skip is greater than zero, then
	 * the first few frames returned by the detector will have a
	 * computed value of num_frames_difference that is negative. If
	 * this happens, we set num_frames_difference to 0, which causes
	 * the processing of these early frames to be skipped.
	 */

	if ( num_frames_difference < 0 ) {
		num_frames_difference = 0;
	}

	/*---*/

	corr->num_unread_frames += num_frames_difference;

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: num_frames_difference = %ld",
		fname, num_frames_difference ));
	MX_DEBUG(-2,("%s: num_unread_frames = %ld",
		fname, corr->num_unread_frames ));
#endif

	/* Readout the frames and add them to either sum_array
	 * or dezinger_frame_array (depending on the value of
	 * ad->dezinger_correction_frame).
	 */

	sequence_complete = FALSE;

	while( corr->num_unread_frames > 0 ) {

		/* Readout the frame into ad->image_frame. */

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,("%s: Reading frame %ld",
				fname, corr->num_frames_read));
#endif
		if ( ad->dezinger_correction_frame ) {
		    dezinger_frame_ptr = 
			&(corr->dezinger_frame_array[ corr->num_frames_read ]);
		} else {
		    dezinger_frame_ptr = NULL;
		}

		switch( ad->correction_measurement_sequence_type ) {
		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_GATED:
			frame_number_to_read = corr->num_frames_read
					+ corr->raw_num_exposures_to_skip;
			break;
		case MXT_SQ_ONE_SHOT:
			frame_number_to_read = 0;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported correction measurement sequence type %lu "
			"requested for area detector '%s'.",
		    (unsigned long) ad->correction_measurement_sequence_type,
				ad->record->name );
		}

		mx_status = mx_area_detector_process_correction_frame(
					ad, frame_number_to_read,
					corr->desired_correction_flags,
					dezinger_frame_ptr,
					corr->sum_array );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( ad, corr );
			return mx_status;
		}

		corr->num_frames_read++;
		corr->num_unread_frames--;

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,("%s: corr->num_frame_read = %ld.",
			fname, corr->num_frames_read));
#endif

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
		corr->old_status = ad_status;

		if ( total_num_frames >= corr->old_total_num_frames ) {
			corr->old_total_num_frames = total_num_frames;
		}

		/* If the correction is done as a sequence of one-shot
		 * frames and a new frame was received since the last
		 * pass through this handler, then start the detector again.
		 */

		busy = ad_status & MXSF_AD_ACQUISITION_IN_PROGRESS;

		start_detector = FALSE;

		if (ad->correction_measurement_sequence_type == MXT_SQ_ONE_SHOT)
		{
			if ( num_frames_difference > 0 ) {
				start_detector = TRUE;
			} else
			if ( busy == 0 ) {
				start_detector = TRUE;
			}
		}

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,
		("%s: start_detector = %d, busy = %lu, total_num_frames = %lu",
			fname, (int) start_detector, busy, total_num_frames));
#endif

		if ( start_detector ) {

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
			MX_DEBUG(-2,
			("%s: Before mx_area_detector_start()", fname));
#endif
			mx_status = mx_area_detector_start( ad->record );

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
			MX_DEBUG(-2,
			("%s: After mx_area_detector_start()", fname));
#endif
			if ( mx_status.code != MXE_SUCCESS ) {
				mx_area_detector_abort( ad->record );

				mx_area_detector_cleanup_after_correction(
								ad, corr );
				return mx_status;
			}
		}

		/* Restart the callback virtual timer. */

		mx_status = mx_virtual_timer_start(
			callback_message->u.function.oneshot_timer,
			callback_message->u.function.callback_interval );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( ad, corr );
		}

#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
		MX_DEBUG(-2,("%s: MARKER #900, ad = %p", fname, ad));
		mx_vm_show_os_info( stderr, ad, sizeof(ad) );
#endif

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,("%s: Callback virtual timer restarted.",fname));
		MX_DEBUG(-2,
		("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"));
#endif
		/* Return, knowing that we will be called again soon. */

		return mx_status;
	}

	/* The correction sequence is complete. */

	/* FIXME - Are we leaking memory by not destroying the timer here? */
#if 0
	(void) mx_virtual_timer_destroy(
		callback_message->u.function.oneshot_timer, NULL );
#endif

#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	MX_DEBUG(-2,("%s: MARKER #990, ad = %p", fname, ad));
	mx_vm_show_os_info( stderr, ad, sizeof(ad) );
#endif

	mx_status = mx_area_detector_finish_correction_calculation( ad, corr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad_flags = ad->area_detector_flags;

	if ( ad_flags & MXF_AD_SAVE_AVERAGED_CORRECTION_FRAME ) {
		mx_status = mx_area_detector_save_averaged_correction_frame(
				ad->record, ad->correction_measurement_type );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: Correction sequence complete.", fname));
	MX_DEBUG(-2,
		("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"));
#endif

#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	MX_DEBUG(-2,("%s: MARKER #999, ad = %p", fname, ad));
	mx_vm_show_os_info( stderr, ad, sizeof(ad) );
#endif

	return mx_status;
}

/*---------------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_stop_correction_callback( MX_RECORD *record,
					MX_AREA_DETECTOR *ad )
{
#if 0
	static const char fname[] =
		"mxp_area_detector_stop_correction_callback()";
#endif

	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr;
	MX_VIRTUAL_TIMER *oneshot_timer;
	mx_status_type mx_status;

	corr = ad->correction_measurement;

	if ( corr == NULL ) {
		ad->correction_measurement_in_progress = FALSE;
		return MX_SUCCESSFUL_RESULT;
	}

	/* Stop the callback timer. */

	oneshot_timer = corr->callback_message->u.function.oneshot_timer;

	/* FIXME - Are we leaking memory by not destroying the timer here? */
#if 0
	mx_status = mx_virtual_timer_destroy( oneshot_timer, NULL );
#else
	mx_status = mx_virtual_timer_stop( oneshot_timer, NULL );
#endif

	mx_area_detector_cleanup_after_correction( ad, NULL );

	return mx_status;
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
	double frame_time, modified_frame_time, gate_time;
	double detector_readout_time;
	unsigned long flags;
	mx_status_type mx_status;

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
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

#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	MX_DEBUG(-2,("%s: MARKER #A, ad = %p, global[0] = %p",
		fname, ad, mx_global_debug_pointer[0]));
#endif

	/* See if callbacks are enabled. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head->callback_pipe == NULL ) {
		/* We are not configured to handle callbacks, so we must
		 * wait for the correction measurement to finish.
		 */

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
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

	/* Set up all the data structures for the correction process. */

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: Before mx_area_detector_prepare_for_correction()",
		fname ));
#endif

	mx_status = mx_area_detector_prepare_for_correction( ad, &corr );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	/* Get the detector readout time for the current sequence. */

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: Before mx_area_detector_get_detector_readout_time()",
		fname ));
#endif

	mx_status = mx_area_detector_get_detector_readout_time( record,
							&detector_readout_time);

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: detector_readout_time = %f",
		fname, detector_readout_time ));
#endif

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

	switch( ad->correction_measurement_sequence_type ) {
	case MXT_SQ_GATED:
		gate_time = 5.0 + corr->raw_num_exposures * corr->exposure_time;

		mx_status = mx_area_detector_set_gated_mode( record,
							corr->raw_num_exposures,
							corr->exposure_time,
							gate_time );
		break;

	case MXT_SQ_MULTIFRAME:

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,
		("%s: Setting multiframe mode: raw_num_exposures = %ld, "
		"exposure_time = %f, modified_frame_time = %f",
			fname, corr->raw_num_exposures, corr->exposure_time,
			modified_frame_time ));
#endif
		mx_status = mx_area_detector_set_multiframe_mode( record,
							corr->raw_num_exposures,
							corr->exposure_time,
							modified_frame_time );
		break;

	case MXT_SQ_ONE_SHOT:
#if PR_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,("%s: Setting one-shot mode: exposure_time = %f",
			fname, corr->exposure_time));
#endif
		mx_status = mx_area_detector_set_one_shot_mode( record,
							corr->exposure_time );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported correction measurement sequence type %lu "
		"requested for area detector '%s'.",
			(unsigned long)ad->correction_measurement_sequence_type,
			ad->record->name );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	flags = ad->area_detector_flags;

	/* If requested, put the detector into a specified trigger mode
	 * for correction measurements.  After the correction measurements
	 * are done, the cleanup code will revert the trigger mode back
	 * to whatever it was before.
	 */

	if ( flags & MXF_AD_CORRECTION_MEASUREMENTS_USE_INTERNAL_TRIGGER ) {

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,("%s: Switching to internal trigger mode.", fname));
#endif

		mx_status = mx_area_detector_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( NULL, corr );
			return mx_status;
		}

	} else
	if ( flags & MXF_AD_CORRECTION_MEASUREMENTS_USE_EXTERNAL_TRIGGER ) {

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,("%s: Switching to external trigger mode.", fname));
#endif

		mx_status = mx_area_detector_set_trigger_mode( record,
						MXT_IMAGE_EXTERNAL_TRIGGER );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( NULL, corr );
			return mx_status;
		}
	}

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: Creating callback message.", fname));
#endif

	/* Create a callback message struct. */

	corr->callback_message = malloc( sizeof(MX_CALLBACK_MESSAGE) );

	if ( corr->callback_message == NULL ) {
		mx_area_detector_cleanup_after_correction( NULL, corr );

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

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: Creating callback timer.", fname));
#endif

	mx_status = mx_virtual_timer_create(
				&oneshot_timer,
				list_head->master_timer,
				MXIT_ONE_SHOT_TIMER,
				mx_callback_standard_vtimer_handler,
				corr->callback_message );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( NULL, corr );
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
		mx_area_detector_cleanup_after_correction( NULL, corr );

		return mx_status;
	}

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: old_last_frame_number = %ld, "
	"old_total_num_frames = %ld, old_status = %#lx",
		fname, corr->old_last_frame_number,
		corr->old_total_num_frames, corr->old_status));
#endif

	/* Some sites (SOLEIL) want to skip over the first few frames.  This
	 * is most easily done by adding corr->raw_num_exposures_to_skip to
	 * the value of corr->old_total_num_frames.  Then, the measurement
	 * callback mxp_area_detector_measure_correction_callback_function()
	 * ignores incoming frames until the reported total number of frames
	 * exceeds the _modified_ value of corr->old_total_num_frames.
	 */

	corr->old_total_num_frames += corr->raw_num_exposures_to_skip;

	/*---*/

	corr->num_frames_read = 0;
	corr->num_unread_frames = 0;

	/* Save the correction measurement structure in the
	 * MX_AREA_DETECTOR structure.  This indicates that
	 * a correction measurement is in progress.
	 */

	ad->correction_measurement = corr;

	/* If this is a dark current measurement, then disable the shutter. */

	if ( ad->correction_measurement_type == MXFT_AD_DARK_CURRENT_FRAME ) {

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,("%s: Disabling the shutter.", fname));
#endif

		mx_status = mx_area_detector_set_shutter_enable(record, FALSE);

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction(NULL, corr);

			return mx_status;
		}
	}

	/* Start the measurement sequence. */

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: Before mx_area_detector_start()", fname));
#endif

	mx_status = mx_area_detector_start( record );

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: After mx_area_detector_start()", fname));
#endif

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_abort( record );

		mx_area_detector_cleanup_after_correction( NULL, corr );

		return mx_status;
	}

	/* Start the callback virtual timer. */

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: Starting the callback timer.", fname));
#endif

	mx_status = mx_virtual_timer_start( oneshot_timer,
		corr->callback_message->u.function.callback_interval );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_abort( record );

		mx_area_detector_cleanup_after_correction( NULL, corr );

		return mx_status;
	}

#if PR_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: Callback virtual timer started.", fname));
	MX_DEBUG(-2,("******************************************************"));
#endif

	return MX_SUCCESSFUL_RESULT;
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

#if PR_AREA_DETECTOR_DEBUG_ROI
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

#if PR_AREA_DETECTOR_DEBUG_ROI
	MX_DEBUG(-2,("%s: roi_bytes_per_frame = %ld, roi_frame_buffer = %p",
		fname, ad->roi_bytes_per_frame, ad->roi_frame_buffer));
#endif

#if PR_AREA_DETECTOR_DEBUG_ROI
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

static mx_status_type
mxp_area_detector_initialize_image_frame( MX_RECORD *record,
					MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxp_area_detector_initialize_image_frame()";

	long bytes_per_frame;
	void *image_data_ptr;
	mx_status_type mx_status;

	mx_status = mx_area_detector_setup_frame( record, &(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_update_frame_pointers( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Zero out the image frame buffer. */

	mx_status = mx_area_detector_get_bytes_per_frame( record,
							&bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	image_data_ptr = ad->image_frame->image_data;

	if ( image_data_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The ad->image_frame->image_data pointer is NULL.  "
		"This should not be able to happen." );
	}

	memset( image_data_ptr, 0, bytes_per_frame );

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

mx_status_type
mx_setup_area_detector_process_functions( MX_RECORD *record )
{

#if PR_AREA_DETECTOR_DEBUG || PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	static const char fname[] =
			"mx_setup_area_detector_process_functions()";
#endif

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

#if PR_AREA_DETECTOR_DEBUG_PROCESS
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

#if PR_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	mx_global_debug_initialized[0] = TRUE;
	mx_global_debug_pointer[0] = record->record_class_struct;

	MX_DEBUG(-2,("%s: #1 ad = %p", fname, record->record_class_struct));
	MX_DEBUG(-2,("%s: #2 ad = %p", fname, mx_global_debug_pointer[0]));
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
		case MXLV_AD_CONSTRUCT_NEXT_DATAFILE_NAME:
		case MXLV_AD_COPY_FRAME:
		case MXLV_AD_CORRECT_FRAME:
		case MXLV_AD_CORRECTION_FLAGS:
		case MXLV_AD_CORRECTION_LOAD_FORMAT:
		case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		case MXLV_AD_CORRECTION_SAVE_FORMAT:
		case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		case MXLV_AD_DATAFILE_DIRECTORY:
		case MXLV_AD_DATAFILE_LOAD_FORMAT:
		case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		case MXLV_AD_DATAFILE_NAME:
		case MXLV_AD_DATAFILE_NUMBER:
		case MXLV_AD_DATAFILE_PATTERN:
		case MXLV_AD_DATAFILE_SAVE_FORMAT:
		case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		case MXLV_AD_DETECTOR_READOUT_TIME:
		case MXLV_AD_DISK_SPACE:
		case MXLV_AD_EXPOSURE_MOTOR_NAME:
		case MXLV_AD_EXPOSURE_TIME:
		case MXLV_AD_EXPOSURE_TRIGGER_NAME:
		case MXLV_AD_EXTENDED_STATUS:
		case MXLV_AD_FRAME_FILENAME:
		case MXLV_AD_FRAMESIZE:
		case MXLV_AD_GET_ROI_FRAME:
		case MXLV_AD_IMAGE_FORMAT:
		case MXLV_AD_IMAGE_FORMAT_NAME:
		case MXLV_AD_IMAGE_FRAME_DATA:
		case MXLV_AD_IMAGE_FRAME_HEADER:
		case MXLV_AD_IMAGE_FRAME_HEADER_LENGTH:
		case MXLV_AD_IMAGE_LOG_FILENAME:
		case MXLV_AD_LAST_FRAME_NUMBER:
		case MXLV_AD_LOAD_FRAME:
		case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		case MXLV_AD_MAXIMUM_FRAMESIZE:
		case MXLV_AD_MOTOR_POSITION:
		case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		case MXLV_AD_NUM_EXPOSURES:
		case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		case MXLV_AD_RAW_LOAD_FRAME:
		case MXLV_AD_RAW_SAVE_FRAME:
		case MXLV_AD_READOUT_FRAME:
		case MXLV_AD_REGISTER_VALUE:
		case MXLV_AD_ROI:
		case MXLV_AD_ROI_FRAME_BUFFER:
		case MXLV_AD_ROI_NUMBER:
		case MXLV_AD_SAVE_FRAME:
		case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		case MXLV_AD_SEQUENCE_START_DELAY:
		case MXLV_AD_SEQUENCE_TYPE:
		case MXLV_AD_SEQUENCE_ONE_SHOT:
		case MXLV_AD_SEQUENCE_CONTINUOUS:
		case MXLV_AD_SEQUENCE_MULTIFRAME:
		case MXLV_AD_SEQUENCE_STROBE:
		case MXLV_AD_SEQUENCE_DURATION:
		case MXLV_AD_SEQUENCE_GATED:
		case MXLV_AD_SHOW_IMAGE_FRAME:
		case MXLV_AD_SHOW_IMAGE_STATISTICS:
		case MXLV_AD_SHUTTER_ENABLE:
		case MXLV_AD_SHUTTER_NAME:
		case MXLV_AD_SETUP_EXPOSURE:
		case MXLV_AD_START:
		case MXLV_AD_STATUS:
		case MXLV_AD_STOP:
		case MXLV_AD_TOTAL_ACQUISITION_TIME:
		case MXLV_AD_TOTAL_NUM_FRAMES:
		case MXLV_AD_TOTAL_SEQUENCE_TIME:
		case MXLV_AD_TRANSFER_FRAME:
		case MXLV_AD_TRIGGER:
		case MXLV_AD_TRIGGER_EXPOSURE:
		case MXLV_AD_TRIGGER_MODE:

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

	MX_RECORD *record = NULL;
	MX_RECORD_FIELD *record_field = NULL;
	MX_AREA_DETECTOR *ad = NULL;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist = NULL;
	MX_IMAGE_FRAME *frame = NULL;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * ) = NULL;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * ) = NULL;
	unsigned long flags;
	int fclose_status, saved_errno;
	char new_filename[MXU_FILENAME_LENGTH+1];
	char timestamp_buffer[80];
	mx_status_type mx_status;

#if PR_AREA_DETECTOR_DEBUG_PROCESS
	unsigned long i;
#endif

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	ad = (MX_AREA_DETECTOR *) (record->record_class_struct);
	flist = (MX_AREA_DETECTOR_FUNCTION_LIST *)
			(record->class_specific_function_list);

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	mx_status = MX_SUCCESSFUL_RESULT;

#if PR_AREA_DETECTOR_DEBUG_PROCESS
	MX_DEBUG(-2,("%s: record '%s', field = '%s', operation = %d",
		fname, record->name, record_field->name, operation));
#endif

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {

		case MXLV_AD_CORRECTION_LOAD_FORMAT:
		case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		case MXLV_AD_CORRECTION_SAVE_FORMAT:
		case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
		case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
		case MXLV_AD_DATAFILE_DIRECTORY:
		case MXLV_AD_DATAFILE_LOAD_FORMAT:
		case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		case MXLV_AD_DATAFILE_NAME:
		case MXLV_AD_DATAFILE_NUMBER:
		case MXLV_AD_DATAFILE_PATTERN:
		case MXLV_AD_DATAFILE_SAVE_FORMAT:
		case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		case MXLV_AD_DISK_SPACE:
		case MXLV_AD_FRAME_FILENAME:
		case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
			ad->parameter_type = record_field->label_value;

			mx_status = (flist->get_parameter)( ad );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;

		case MXLV_AD_MOTOR_POSITION:
			/* Just report the value already in the variable. */

			break;
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
		case MXLV_AD_CORRECTION_FLAGS:
			mx_status = mx_area_detector_get_correction_flags(
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

			mx_area_detector_update_extended_status_string( ad );

#if PR_AREA_DETECTOR_DEBUG_PROCESS
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

			mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
			break;
		case MXLV_AD_FRAMESIZE:
			mx_status = mx_area_detector_get_framesize( record,
								NULL, NULL );
			break;
		case MXLV_AD_IMAGE_FRAME_DATA:

#if PR_AREA_DETECTOR_DEBUG_IMAGE_FRAME_DATA
			MX_DEBUG(-2,("%s: mx_pointer_is_valid(%p) = %d",
				fname, ad->image_frame_data,
				mx_pointer_is_valid( ad->image_frame_data,
					sizeof(uint16_t), R_OK | W_OK ) ));

			mx_vm_show_os_info( stderr,
					ad->image_frame_data,
					sizeof(uint16_t) );
#endif
			if ( ad->image_frame_data == NULL ) {
				mx_status = 
				    mxp_area_detector_initialize_image_frame(
								record, ad );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				if ( ad->image_frame_data == NULL ) {
					return mx_error(
					MXE_INITIALIZATION_ERROR, fname,
					"The attempt to initialize the image "
					"frame of area detector '%s' failed.",
						record->name );
				}
			}
			break;
		case MXLV_AD_IMAGE_FRAME_HEADER:
			if ( ad->image_frame_header == NULL ) {
				mx_status = 
				    mxp_area_detector_initialize_image_frame(
								record, ad );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				if ( ad->image_frame_header == NULL ) {
					return mx_error(
					MXE_INITIALIZATION_ERROR, fname,
					"The attempt to initialize the image "
					"frame of area detector '%s' failed.",
						record->name );
				}
			}
			break;
		case MXLV_AD_IMAGE_FRAME_HEADER_LENGTH:
			if ( ad->image_frame_header_length == 0 ) {
				mx_status = 
				    mxp_area_detector_initialize_image_frame(
								record, ad );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				if ( ad->image_frame_header_length == 0 ) {
					return mx_error(
					MXE_INITIALIZATION_ERROR, fname,
					"The attempt to initialize the image "
					"frame of area detector '%s' failed.",
						record->name );
				}
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
		case MXLV_AD_REGISTER_VALUE:
			mx_status = mx_area_detector_get_register( record,
						ad->register_name, NULL );
			break;
		case MXLV_AD_ROI:
			mx_status = mx_area_detector_get_roi( record,
							ad->roi_number, NULL );
#if PR_AREA_DETECTOR_DEBUG_ROI
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
		case MXLV_AD_EXPOSURE_TIME:
			mx_status = mx_area_detector_get_exposure_time(
								record, NULL );
			break;
		case MXLV_AD_NUM_EXPOSURES:
			mx_status = mx_area_detector_get_num_exposures(
								record, NULL );
			break;
		case MXLV_AD_TRIGGER_MODE:
			mx_status = mx_area_detector_get_trigger_mode(
								record, NULL );
			break;

#if PR_AREA_DETECTOR_DEBUG_ROI
		case MXLV_AD_ROI_NUMBER:
			MX_DEBUG(-2,("%s: ROI number = %ld",
				fname, ad->roi_number));
			break;
#endif
		case MXLV_AD_SEQUENCE_TYPE:
		case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
			mx_status = mx_area_detector_get_sequence_parameters(
								record, NULL );
#if PR_AREA_DETECTOR_DEBUG_ROI
			MX_DEBUG(-2,("%s: sequence type = %ld",
				fname, ad->sequence_parameters.sequence_type));
			MX_DEBUG(-2,("%s: num_parameters = %ld",
				fname, ad->sequence_parameters.num_parameters));

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

		case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
		case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		case MXLV_AD_DATAFILE_NUMBER:
		case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		case MXLV_AD_FRAME_FILENAME:
		case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
		case MXLV_AD_RAW_LOAD_FRAME:
		case MXLV_AD_RAW_SAVE_FRAME:
			ad->parameter_type = record_field->label_value;

			mx_status = (flist->set_parameter)( ad );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;

		case MXLV_AD_MOTOR_POSITION:
			/* Just save the value. */

			break;
		case MXLV_AD_ABORT:
			(void) mxp_area_detector_stop_correction_callback(
								record, ad );

			mx_status = mx_area_detector_abort( record );
			break;
		case MXLV_AD_ARM:
			if ( ad->arm != 0 ) {
				mx_status = mx_area_detector_arm( record );
			}
			break;
		case MXLV_AD_BINSIZE:
			mx_status = mx_area_detector_set_binsize( record,
						ad->binsize[0],
						ad->binsize[1] );
			break;
		case MXLV_AD_CONSTRUCT_NEXT_DATAFILE_NAME:
			mx_status =
	mx_area_detector_construct_next_datafile_name( record, TRUE, NULL, 0 );

			break;
		case MXLV_AD_COPY_FRAME:
			mx_status = mx_area_detector_copy_frame( record,
					ad->copy_frame[0], ad->copy_frame[1] );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ad->copy_frame[1] == MXFT_AD_IMAGE_FRAME ) {
				mx_status =
				    mx_area_detector_update_frame_pointers(ad);
			}
			break;
		case MXLV_AD_CORRECT_FRAME:
			mx_status = mx_area_detector_correct_frame( record );

			break;
		case MXLV_AD_CORRECTION_FLAGS:
			mx_status = mx_area_detector_set_correction_flags(
							record,
							ad->correction_flags );
			break;
		case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
			mx_status =
	    mxp_area_detector_measure_correction_frame_handler( record, ad );

			break;
		case MXLV_AD_DATAFILE_DIRECTORY:
		case MXLV_AD_DATAFILE_PATTERN:
			ad->parameter_type = record_field->label_value;

			mx_status = (flist->set_parameter)( ad );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			flags = ad->area_detector_flags;

			if ( flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION )
			{
				mx_status =
			    mx_area_detector_initialize_datafile_number(record);
			}
			break;
		case MXLV_AD_EXPOSURE_MOTOR_NAME:
			/* If the exposure motor name has changed, then
			 * we need to do a lookup of the corresponding
			 * motor record.
			 */

			if ( strcmp( ad->exposure_motor_name,
			    ad->last_exposure_motor_name ) != 0 )
			{
			    ad->exposure_motor_record
				  = mx_get_record( record,
				  	ad->exposure_motor_name );

			    if ( ad->exposure_motor_record == NULL ) {
				ad->last_exposure_motor_name[0] = '\0';

				return mx_error( MXE_NOT_FOUND, fname,
				"exposure motor '%s' was not found "
				"in the MX database.",
					ad->exposure_motor_name );
			    } else {
				/* Is this record a motor record? */

				switch(ad->exposure_motor_record->mx_class) {
				case MXC_MOTOR:
					break;
				default:
					mx_status = mx_error(
					MXE_ILLEGAL_ARGUMENT, fname,
					"The record '%s' specified as the "
					"exposure motor record is not "
					"actually a motor record.",
						ad->exposure_motor_name );

					ad->exposure_motor_record = NULL;
					ad->exposure_motor_name[0] = '\0';
				}

			    	strlcpy( ad->last_exposure_motor_name,
					ad->exposure_motor_name,
				    sizeof(ad->last_exposure_motor_name) );
			    }
			}
			break;
		case MXLV_AD_EXPOSURE_TRIGGER_NAME:
			/* If the exposure trigger name has changed, then
			 * we need to do a lookup of the corresponding
			 * trigger record.
			 */

			if ( strcmp( ad->exposure_trigger_name,
			    ad->last_exposure_trigger_name ) != 0 )
			{
			    ad->exposure_trigger_record
				  = mx_get_record( record,
				  	ad->exposure_trigger_name );

			    if ( ad->exposure_trigger_record == NULL ) {
				ad->last_exposure_trigger_name[0] = '\0';

				return mx_error( MXE_NOT_FOUND, fname,
				"exposure trigger '%s' was not found "
				"in the MX database.",
					ad->exposure_trigger_name );
			    } else {
				/* Is this record a trigger record? */

				switch(ad->exposure_trigger_record->mx_class) {
				case MXC_RELAY:
					break;
				default:
					mx_status = mx_error(
					MXE_ILLEGAL_ARGUMENT, fname,
					"The record '%s' specified as the "
					"exposure trigger record is not "
					"a valid exposure trigger.  "
					"The exposure trigger must be a "
					"relay record.",
						ad->exposure_trigger_name );

					ad->exposure_trigger_record = NULL;
					ad->exposure_trigger_name[0] = '\0';
				}

			    	strlcpy( ad->last_exposure_trigger_name,
					ad->exposure_trigger_name,
				    sizeof(ad->last_exposure_trigger_name) );
			    }
			}
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
			mx_status = mx_image_get_image_format_type_from_name(
				ad->image_format_name, &(ad->image_format) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Fall through to the next case. */
		case MXLV_AD_IMAGE_FORMAT:
			mx_status = mx_area_detector_set_image_format( record,
							ad->image_format );
			break;
		case MXLV_AD_IMAGE_LOG_FILENAME:
			/* If an existing image log file is open, close it. */

			if ( ad->image_log_file != NULL ) {
				fprintf( ad->image_log_file, "%s close\n",
					mx_timestamp( timestamp_buffer,
						sizeof(timestamp_buffer) ) );

				fflush( ad->image_log_file );

				fclose_status = fclose( ad->image_log_file );

				ad->image_log_file = NULL;

				if ( fclose_status != 0 ) {
					saved_errno = errno;

					return mx_error(MXE_FILE_IO_ERROR,fname,
				"The attempt to close image log file '%s' "
				"failed.  Errno = %d, error message = '%s'",
					ad->image_log_filename,
					saved_errno,
					strerror(saved_errno) );
				}
			}

			/* If the length of the log filename is greater
			 * than 0, then interpret this as a request to
			 * append to a log file with the specified name.
			 */

			if ( strlen( ad->image_log_filename ) > 0 ) {
				mx_status = mx_cfn_construct_filename(
						MX_CFN_LOGFILE,
						ad->image_log_filename,
						new_filename,
						sizeof(new_filename) );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				ad->image_log_file = fopen( new_filename, "a" );

				if ( ad->image_log_file == (FILE *) NULL ) {
					saved_errno = errno;

					return mx_error(MXE_FILE_IO_ERROR,fname,
					"The attempt to open image log file "
					"'%s' failed.  "
					"Errno = %d, error message = '%s'",
						new_filename,
						saved_errno,
						strerror(saved_errno) );
				}

				/* Configure the log file to be line buffered,
				 * and write out a message saying that we
				 * have opened the file.
				 */

				setvbuf( ad->image_log_file, (char *) NULL,
					_IOLBF, BUFSIZ );

				fprintf( ad->image_log_file, "%s open %s\n",
					mx_timestamp( timestamp_buffer,
						sizeof(timestamp_buffer) ),
					new_filename );

				fflush( ad->image_log_file );
			}
			break;
		case MXLV_AD_LOAD_FRAME:
			mx_status = mx_area_detector_load_frame( record,
					ad->load_frame, ad->frame_filename );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( ad->load_frame == MXFT_AD_IMAGE_FRAME ) {
				mx_status =
				    mx_area_detector_update_frame_pointers(ad);
			}
			break;
		case MXLV_AD_REGISTER_VALUE:
			mx_status = mx_area_detector_set_register( record,
			    ad->register_name, ad->register_value );
			break;
		case MXLV_AD_ROI:
			mx_status = mx_area_detector_set_roi( record,
						ad->roi_number, ad->roi );
#if PR_AREA_DETECTOR_DEBUG_ROI
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

			mx_status = mx_area_detector_update_frame_pointers(ad);
			break;
		case MXLV_AD_SAVE_FRAME:
			mx_status = mx_area_detector_save_frame( record,
					ad->save_frame, ad->frame_filename );
			break;
		case MXLV_AD_SEQUENCE_START_DELAY:
			mx_status = mx_area_detector_set_sequence_start_delay(
					record, ad->sequence_start_delay );
			break;
		case MXLV_AD_SETUP_EXPOSURE:
			mx_status = mx_area_detector_setup_exposure( record,
						ad->exposure_motor_record,
						ad->shutter_record,
						ad->exposure_trigger_record,
						ad->exposure_distance,
						ad->shutter_time );
			break;
		case MXLV_AD_SHUTTER_ENABLE:
			mx_status = mx_area_detector_set_shutter_enable(
						record, ad->shutter_enable );
			break;
		case MXLV_AD_SHUTTER_NAME:
			/* If the shutter name has changed, then we need to do
			 * a lookup of the corresponding shutter record.
			 */

			if ( strcmp( ad->shutter_name,
			    ad->last_shutter_name ) != 0 )
			{
			    ad->shutter_record = mx_get_record( record,
						  	ad->shutter_name );

			    if ( ad->shutter_record == NULL ) {
				ad->last_shutter_name[0] = '\0';

				return mx_error( MXE_NOT_FOUND, fname,
				"Shutter '%s' was not found "
				"in the MX database.",
					ad->shutter_name );
			    } else {
				/* Is this record a digital output or relay? */

				switch(ad->shutter_record->mx_class) {
				case MXC_DIGITAL_OUTPUT:
				case MXC_RELAY:
					break;
				default:
					mx_status = mx_error(
					MXE_ILLEGAL_ARGUMENT, fname,
					"The record '%s' specified as the "
					"shutter record is not actually a"
					"digital output or relay.",
						ad->shutter_name );

					ad->shutter_record = NULL;
					ad->shutter_name[0] = '\0';
				}

			    	strlcpy( ad->last_shutter_name,
					ad->shutter_name,
				    sizeof(ad->last_shutter_name) );
			    }
			}
			break;
		case MXLV_AD_START:
			if ( ad->start != 0 ) {
				mx_status = mx_area_detector_start( record );
			}
			break;
		case MXLV_AD_STOP:
			(void) mxp_area_detector_stop_correction_callback(
								record, ad );

			mx_status = mx_area_detector_stop( record );
			break;
		case MXLV_AD_TRANSFER_FRAME:
			if ( ad->image_frame == NULL ) {
				return mx_error( MXE_NOT_READY, fname,
				"No image frames have been acquired by area "
				"detector '%s' since the last time that it "
				"was started.",
					record->name );
			}

			mx_status = mx_area_detector_transfer_frame( record,
				ad->transfer_frame, &(ad->image_frame) );
			break;
		case MXLV_AD_TRIGGER:
			if ( ad->trigger != 0 ) {
				mx_status = mx_area_detector_trigger( record );
			}
			break;
		case MXLV_AD_TRIGGER_EXPOSURE:
			mx_status = mx_area_detector_trigger_exposure( record );
			break;
		case MXLV_AD_TRIGGER_MODE:
			mx_status = mx_area_detector_set_trigger_mode(
						record, ad->trigger_mode );
			break;

#if PR_AREA_DETECTOR_DEBUG_ROI
		case MXLV_AD_ROI_NUMBER:
			MX_DEBUG(-2,("%s: ROI number = %ld",
				fname, ad->roi_number));
			break;
#endif

		case MXLV_AD_SEQUENCE_TYPE:
		case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
#if PR_AREA_DETECTOR_DEBUG_PROCESS
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

		case MXLV_AD_SEQUENCE_ONE_SHOT:
#if 0
			MX_DEBUG(-2,("%s: one_shot ... %f", fname,
				ad->sequence_one_shot[0]));
#endif
			mx_status = mx_area_detector_set_one_shot_mode( record,
						ad->sequence_one_shot[0] );
			break;

		case MXLV_AD_SEQUENCE_CONTINUOUS:
			mx_status = mx_area_detector_set_continuous_mode(record,
						ad->sequence_continuous[0] );
			break;

		case MXLV_AD_SEQUENCE_MULTIFRAME:
			mx_status = mx_area_detector_set_multiframe_mode(record,
					mx_round( ad->sequence_multiframe[0] ),
					ad->sequence_multiframe[1],
					ad->sequence_multiframe[2] );
			break;

		case MXLV_AD_SEQUENCE_STROBE:
			mx_status = mx_area_detector_set_strobe_mode( record,
					mx_round( ad->sequence_strobe[0] ),
					ad->sequence_strobe[1] );
			break;

		case MXLV_AD_SEQUENCE_DURATION:
#if 0
			MX_DEBUG(-2,("%s: duration ... %f", fname,
				ad->sequence_duration[0]));
#endif
			mx_status = mx_area_detector_set_duration_mode( record,
					mx_round( ad->sequence_duration[0] ) );
			break;

		case MXLV_AD_SEQUENCE_GATED:
#if 0
			MX_DEBUG(-2,("%s: gated ... %f, %f, %f", fname,
				ad->sequence_gated[0],
				ad->sequence_gated[1],
				ad->sequence_gated[2]));
#endif
			mx_status = mx_area_detector_set_gated_mode( record,
					mx_round( ad->sequence_gated[0] ),
					ad->sequence_gated[1],
					ad->sequence_gated[2] );
			break;

		case MXLV_AD_SHOW_IMAGE_FRAME:
			switch( ad->show_image_frame) {
			case MXFT_AD_IMAGE_FRAME:
				frame = ad->image_frame;
				break;
			case MXFT_AD_MASK_FRAME:
				frame = ad->mask_frame;
				break;
			case MXFT_AD_BIAS_FRAME:
				frame = ad->bias_frame;
				break;
			case MXFT_AD_DARK_CURRENT_FRAME:
				frame = ad->dark_current_frame;
				break;
			case MXFT_AD_FLOOD_FIELD_FRAME:
				frame = ad->flood_field_frame;
				break;
			default:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Illegal frame type %ld requested for "
				"detector '%s'", ad->show_image_frame,
					ad->record->name );
				break;
			}
			if ( frame == NULL ) {
				return mx_error( MXE_NOT_READY, fname,
				"Frame type %ld has not yet been used "
				"by detector '%s'", ad->show_image_frame,
					ad->record->name );
			}
#if 0
			mx_image_display_ascii( stderr, frame, 0, 65535 );
#endif
			break;
		case MXLV_AD_SHOW_IMAGE_STATISTICS:
			switch( ad->show_image_statistics) {
			case MXFT_AD_IMAGE_FRAME:
				frame = ad->image_frame;
				break;
			case MXFT_AD_MASK_FRAME:
				frame = ad->mask_frame;
				break;
			case MXFT_AD_BIAS_FRAME:
				frame = ad->bias_frame;
				break;
			case MXFT_AD_DARK_CURRENT_FRAME:
				frame = ad->dark_current_frame;
				break;
			case MXFT_AD_FLOOD_FIELD_FRAME:
				frame = ad->flood_field_frame;
				break;
			default:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Illegal frame type %ld requested for "
				"detector '%s'", ad->show_image_statistics,
					ad->record->name );
				break;
			}
			if ( frame == NULL ) {
				return mx_error( MXE_NOT_READY, fname,
				"Frame type %ld has not yet been used "
				"by detector '%s'", ad->show_image_statistics,
					ad->record->name );
			}
#if 0
			MX_DEBUG(-2,("%s: frame = %p, format = %lu",
				fname, frame,
				(unsigned long)MXIF_IMAGE_FORMAT(frame) ));
#endif
			mx_image_statistics( frame );
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

