/*
 * Name:    d_xineos_gige.c
 *
 * Purpose: MX driver for the DALSA Xineos GigE series detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_XINEOS_GIGE_DEBUG				FALSE

#define MXD_XINEOS_GIGE_DEBUG_OPEN			FALSE

#define MXD_XINEOS_GIGE_DEBUG_RESYNCHRONIZE		TRUE

#define MXD_XINEOS_GIGE_DEBUG_ARM			FALSE

#define MXD_XINEOS_GIGE_DEBUG_READOUT_TIMING		FALSE

#define MXD_XINEOS_GIGE_DEBUG_MEASURE_CORRECTION	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_inttypes.h"
#include "mx_hrt.h"
#include "mx_hrt_debug.h"
#include "mx_ascii.h"
#include "mx_cfn.h"
#include "mx_motor.h"
#include "mx_image.h"
#include "mx_rs232.h"
#include "mx_pulse_generator.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "mx_area_detector_rdi.h"
#include "d_xineos_gige.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_xineos_gige_record_function_list = {
	mxd_xineos_gige_initialize_driver,
	mxd_xineos_gige_create_record_structures,
	mx_area_detector_finish_record_initialization,
	NULL,
	NULL,
	mxd_xineos_gige_open,
	NULL,
	NULL,
	mxd_xineos_gige_resynchronize
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_xineos_gige_ad_function_list = {
	mxd_xineos_gige_arm,
	mxd_xineos_gige_trigger,
	NULL,
	mxd_xineos_gige_stop,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_xineos_gige_get_extended_status,
	mxd_xineos_gige_readout_frame,
	mxd_xineos_gige_correct_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_xineos_gige_get_parameter,
	mxd_xineos_gige_set_parameter,
	mxd_xineos_gige_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_xineos_gige_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_XINEOS_GIGE_STANDARD_FIELDS
};

long mxd_xineos_gige_num_record_fields
		= sizeof( mxd_xineos_gige_rf_defaults )
		/ sizeof( mxd_xineos_gige_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_xineos_gige_rfield_def_ptr
			= &mxd_xineos_gige_rf_defaults[0];

/*---*/

static mx_status_type
mxd_xineos_gige_get_pointers( MX_AREA_DETECTOR *ad,
			MX_XINEOS_GIGE **xineos_gige,
			const char *calling_fname )
{
	static const char fname[] = "mxd_xineos_gige_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (xineos_gige == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_XINEOS_GIGE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*xineos_gige = (MX_XINEOS_GIGE *)
				ad->record->record_type_struct;

	if ( *xineos_gige == (MX_XINEOS_GIGE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_XINEOS_GIGE pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_xineos_gige_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xineos_gige_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_xineos_gige_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_XINEOS_GIGE *xineos_gige;

	/* Use calloc() here instead of malloc(), so that a bunch of
	 * fields that are never touched will be initialized to 0.
	 */

	ad = (MX_AREA_DETECTOR *) calloc( 1, sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	xineos_gige = (MX_XINEOS_GIGE *)
				calloc( 1, sizeof(MX_XINEOS_GIGE) );

	if ( xineos_gige == (MX_XINEOS_GIGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for an MX_XINEOS_GIGE structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = xineos_gige;
	record->class_specific_function_list = 
			&mxd_xineos_gige_ad_function_list;

	ad->record = record;
	xineos_gige->record = record;

	xineos_gige->image_noir_info = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xineos_gige_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_xineos_gige_open()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_XINEOS_GIGE *xineos_gige = NULL;
	MX_RECORD *video_input_record = NULL;
	MX_RECORD_FIELD *num_frames_to_skip_field = NULL;
	long i;
	unsigned long mask, xineos_flags;
	long *num_frames_to_skip_ptr;
	long num_frames_to_skip;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG_OPEN
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	video_input_record = xineos_gige->video_input_record;

	if ( video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The video_input_record pointer for detector '%s' is NULL.",
			record->name );
	}

	xineos_flags = xineos_gige->xineos_gige_flags;

	/*---*/

	/* ad->correction_frames_to_skip should normally be set to 0, since
	 * the 'sapera_lt_camera' video input driver used by this area
	 * detector takes care of skipping frames itself.
	 */

	ad->correction_frames_to_skip = 0;

	if ( xineos_flags & MXF_XINEOS_GIGE_AUTOMATICALLY_DUMP_PIXEL_VALUES ) {
		xineos_gige->dump_pixel_values = TRUE;
	} else {
		xineos_gige->dump_pixel_values = FALSE;
	}

	/*---*/

	ad->header_length = 0;

	ad->sequence_parameters.sequence_type = MXT_SQ_ONE_SHOT;
	ad->sequence_parameters.num_parameters = 1;
	ad->sequence_parameters.parameter_array[0] = 1.0;

	/* Set the default file formats. */

	ad->datafile_load_format   = MXT_IMAGE_FILE_SMV;
	ad->datafile_save_format   = MXT_IMAGE_FILE_SMV;
	ad->correction_load_format = MXT_IMAGE_FILE_SMV;
	ad->correction_save_format = MXT_IMAGE_FILE_SMV;

	ad->correction_calc_format = MXT_IMAGE_FORMAT_FLOAT;

	ad->correction_measurement_sequence_type = MXT_SQ_MULTIFRAME;

	xineos_gige->saturation_pixel_value = 14000.0;
	xineos_gige->minimum_pixel_value = 5.0;

	/* --- */

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

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						sizeof(ad->image_format_name) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	/* Does this detector have an MX pulse generator to use in generating
	 * "internal" triggers?
	 */

	if ( strlen( xineos_gige->pulse_generator_name ) == 0 ) {
		xineos_gige->pulse_generator_record = NULL;
	} else {
		xineos_gige->pulse_generator_record =
			mx_get_record( record,
				xineos_gige->pulse_generator_name );

		if ( xineos_gige->pulse_generator_record == NULL ) {
			(void) mx_error( MXE_NOT_FOUND, fname,
			"Internal trigger record '%s' for detector '%s' "
			"was not found in the MX database.",
				xineos_gige->pulse_generator_name,
				record->name );

		} else {
			/* Is the trigger record a pulse generator record? */

			unsigned long mx_class;

			mx_class =
			 xineos_gige->pulse_generator_record->mx_class;

			if ( mx_class != MXC_PULSE_GENERATOR ) {
				(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Internal trigger record '%s' for "
				"detector '%s' is NOT a pulse generator "
				"record.  It will be IGNORED!",
				    xineos_gige->pulse_generator_name,
				    record->name );
				
				xineos_gige->pulse_generator_record = NULL;
			}
		}
	}

	if ( xineos_gige->pulse_generator_record == NULL ) {
		xineos_gige->pulse_generator_is_available = FALSE;
	} else {
		xineos_gige->pulse_generator_is_available = TRUE;
	}

	/* Internally triggered one shot and multiframe sequences will
	 * use the pulse generator for exposures greater than or equal
	 * to this time if a pulse generator is present.
	 *
	 * Note: Currently supported versions of this driver use pulse
	 * generators for everything.
	 */

	xineos_gige->pulse_generator_time_threshold = 0.0;
	xineos_gige->minimum_frame_time = MXT_XINEOS_GIGE_MINIMUM_FRAME_TIME;

	/*---*/

	xineos_gige->start_with_pulse_generator = FALSE;
	xineos_gige->using_external_video_duration_mode = FALSE;

	/* The detector will default to internal triggering. */

	mx_status = mx_area_detector_set_trigger_mode( record,
						MXT_IMAGE_INTERNAL_TRIGGER );
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

	ad->mask_image_format = MXT_IMAGE_FORMAT_GREY16;
	ad->bias_image_format = MXT_IMAGE_FORMAT_GREY16;
	ad->dark_current_image_format = MXT_IMAGE_FORMAT_FLOAT;
	ad->flat_field_image_format = MXT_IMAGE_FORMAT_FLOAT;

	ad->measure_dark_current_correction_flags = MXFT_AD_MASK_FRAME;
	ad->measure_flat_field_correction_flags =
			MXFT_AD_MASK_FRAME | MXFT_AD_DARK_CURRENT_FRAME;

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure automatic saving or readout of image frames. */

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_READOUT_FRAME_AFTER_ACQUISITION;

	if ( ad->area_detector_flags & mask ) {
		mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Configure the video input driver to set the value of the
	 * 'num_frames_to_skip' field to the number of junk frames
	 * to be discarded at the start of a sequence.
	 */

	mx_status = mx_find_record_field( video_input_record,
					"num_frames_to_skip",
					&num_frames_to_skip_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_frames_to_skip_ptr = (long *)
		mx_get_field_value_pointer( num_frames_to_skip_field );

#if MXD_XINEOS_GIGE_DEBUG_OPEN
	MX_DEBUG(-2,("%s: num_frames_to_skip_ptr = %p",
		fname, num_frames_to_skip_ptr));
#endif

	num_frames_to_skip = 1;

	*num_frames_to_skip_ptr = num_frames_to_skip;

#if MXD_XINEOS_GIGE_DEBUG_OPEN
	MX_DEBUG(-2,("%s: *num_frames_to_skip_ptr = %ld",
		fname, *num_frames_to_skip_ptr));
#endif

#if MXD_XINEOS_GIGE_DEBUG_OPEN
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxd_xineos_gige_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_xineos_gige_resynchronize()";

	MX_AREA_DETECTOR *ad;
	MX_XINEOS_GIGE *xineos_gige;
	mx_status_type mx_status;

	/* Whether or not resynchronize is successful depends on the
	 * nature of the video capture card's failure.  Some kinds
	 * of failures require restarting the MX server or rebooting
	 * the host computer, which is not something that resynchronize
	 * can handle.
	 */

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG_RESYNCHRONIZE
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	/* Tell the video input card to go resynchronize itself. */

	mx_status = mx_resynchronize_record( xineos_gige->video_input_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG_RESYNCHRONIZE
	MX_DEBUG(-2,("%s complete for record '%s'", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxd_xineos_gige_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_xineos_gige_arm()";

	MX_XINEOS_GIGE *xineos_gige = NULL;
	MX_RECORD *video_input_record = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	MX_SEQUENCE_PARAMETERS vinput_sp;
	double exposure_time;
	double video_pulse_width, video_pulse_period;
	long vinput_trigger_mode, num_frames;
	long video_sequence_type, ad_sequence_type;
	mx_status_type mx_status;

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG_ARM
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	video_input_record = xineos_gige->video_input_record;

	if ( video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The video_input_record pointer for detector '%s' is NULL.",
			ad->record->name );
	}

	/* If we are currently configured to save files using NOIR format,
	 * then make sure we have updated the information needed by the
	 * header of those files.
	 */

	if ( ad->datafile_save_format == MXT_IMAGE_FILE_NOIR ) {

		if ( xineos_gige->image_noir_info == NULL ) {
			char static_header_filename[MXU_FILENAME_LENGTH+1];

			mx_status = mx_cfn_construct_filename( MX_CFN_CONFIG,
						"noir_header.dat",
						static_header_filename,
						sizeof(static_header_filename));

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_image_noir_setup( ad->record,
					"",
					"mx_image_noir_records",
					static_header_filename,
				&(xineos_gige->image_noir_info) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mx_image_noir_update(
				xineos_gige->image_noir_info );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Get the exposure time and number of frames, since they are
	 * all we need from the area detector's sequence parameters
	 * in order to construct the video input card's MX sequence.
	 */

	sp = &(ad->sequence_parameters);

	mx_status = mx_sequence_get_exposure_time( sp, 0, &exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_sequence_get_num_frames( sp, &num_frames );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is this an internally triggered sequence that needs to use a
	 * pulse generator to send triggers to the video input device?
	 */

#if MXD_XINEOS_GIGE_DEBUG_ARM
	MX_DEBUG(-2,("%s: pulse_generator_is_available = %d",
		fname, (int) xineos_gige->pulse_generator_is_available));
	MX_DEBUG(-2,("%s: ad->trigger_mode = %#lx",
		fname, ad->trigger_mode));
	MX_DEBUG(-2,("%s: sp->sequence_type = %ld",
		fname, sp->sequence_type));
	MX_DEBUG(-2,
	("%s: exposure_time = %f, pulse_generator_time_threshold = %f",
		fname, exposure_time,
		xineos_gige->pulse_generator_time_threshold));
#endif
	xineos_gige->start_with_pulse_generator = FALSE;

	if ( xineos_gige->pulse_generator_is_available ) {
	    if ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {
		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_STREAM:
		case MXT_SQ_MULTIFRAME:
		    break;
		default:
		    return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"Internally triggered sequences can only be used "
			"for one-shot and multiframe sequences for "
			"area detector '%s'.  However, you have requested "
			"a sequence type of %ld.",
				ad->record->name, sp->sequence_type );
		    break;
		}

		if ( sp->sequence_type == MXT_SQ_STREAM ) {
		    xineos_gige->start_with_pulse_generator = FALSE;
		} else
		if ( exposure_time >=
				xineos_gige->pulse_generator_time_threshold )
		{
		    xineos_gige->start_with_pulse_generator = TRUE;
		} else {
		    xineos_gige->start_with_pulse_generator = FALSE;
		}
	    } else
	    if ( ad->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) {
		xineos_gige->start_with_pulse_generator = FALSE;
	    } else {
		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		"Neither internal or external trigger are turned on "
		"for area detector '%s'.  trigger_mode = %#lx",
			ad->record->name, ad->trigger_mode );
	    }
	} else {
	    /* A pulse generator is not available. */

	    xineos_gige->start_with_pulse_generator = FALSE;
	}

	/* If we are using a pulse generator to generate "internal" triggers,
	 * then tell the video card to expect an _external_ trigger.
	 * Otherwise, just pass the area detector record's trigger mode
	 * through to the video card.
	 */

	if ( xineos_gige->start_with_pulse_generator ) {
		vinput_trigger_mode = MXT_IMAGE_EXTERNAL_TRIGGER;
	} else {
		vinput_trigger_mode = ad->trigger_mode;
	}

#if MXD_XINEOS_GIGE_DEBUG_ARM
	MX_DEBUG(-2,
	("%s: start_with_pulse_generator = %d, vinput_trigger_mode = %d",
		fname, xineos_gige->start_with_pulse_generator,
		vinput_trigger_mode));
#endif

	mx_status = mx_video_input_set_trigger_mode( video_input_record,
							vinput_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are using a pulse generator to generate "internal" triggers,
	 * then the video card's exposure time will be set by the width of
	 * the pulse generator's pulses or the time between corresponding
	 * edges of the trigger pulse.  That means that we need to force the
	 * video card to expect a DURATION mode sequence.
	 */

	xineos_gige->using_external_video_duration_mode = FALSE;

	if ( xineos_gige->start_with_pulse_generator ) {

		xineos_gige->using_external_video_duration_mode = TRUE;

		/* Set the video input sequence to Duration mode, so that
		 * the video input will be triggered by the pulse generator.
		 */

		video_sequence_type = MXT_SQ_DURATION;
	} else {
		/* If we are not starting the sequence with the pulse
		 * generator, but we _are_ using an external trigger
		 * and the pulse generator is available, then we have
		 * to program the pulse generator, but we do _NOT_
		 * start it, since the sequence will be started by an
		 * external trigger sent to the _pulser_.
		 */

		if ( ( ad->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER )
		  && ( xineos_gige->pulse_generator_is_available ) )
		{
			/* For this detector, there is no gap time, so we
			 * force the pulse period to be the same as the
			 * pulse width.
			 */

			double pulse_width;
			unsigned long num_pulses;

			switch( sp->sequence_type ) {
			case MXT_SQ_ONE_SHOT:
				pulse_width = sp->parameter_array[0];
				num_pulses = 1;
				break;
			case MXT_SQ_MULTIFRAME:
				pulse_width = sp->parameter_array[1];
				num_pulses = sp->parameter_array[0];

				sp->parameter_array[2] = pulse_width;
				break;
			default:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Only one-shot and multiframe sequences can "
				"be used with external triggering for "
				"area detector '%s'.",
					ad->record->name );
				break;
			}

			if ( pulse_width < xineos_gige->minimum_frame_time )
			{
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"An exposure time of %f seconds was requested "
				"for area detector '%s', but the minimum "
				"exposure time for this detector "
				"is %f seconds.",
					pulse_width, ad->record->name,
					xineos_gige->minimum_frame_time );
			}

			video_pulse_width = MXT_XINEOS_GIGE_TRIGGER_WIDTH;

			video_pulse_period = pulse_width;

			mx_status = mx_pulse_generator_setup(
					xineos_gige->pulse_generator_record,
					video_pulse_period,
					video_pulse_width,
					num_pulses,
					0.0,
					MXF_PGN_PULSE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			xineos_gige->using_external_video_duration_mode = TRUE;

			/* Set the video input sequence to Duration mode,
			 * so that the video input will be triggered by
			 * the pulse generator.
			 */

			video_sequence_type = MXT_SQ_DURATION;
		} else {

			/* Otherwise, make the video input sequence
			 * a copy of the area detector's sequence.
			 */

			ad_sequence_type = 
				ad->sequence_parameters.sequence_type;

			if ( ad_sequence_type == MXT_SQ_ONE_SHOT ) {
				video_sequence_type = MXT_SQ_MULTIFRAME;
				num_frames = 1;
			} else {
				video_sequence_type = ad_sequence_type;
			}
		}
	}

	/* Configure the sequence parameters for the video input record. */

	memset( &vinput_sp, 0, sizeof(vinput_sp) );

	vinput_sp.sequence_type = video_sequence_type;

	switch( video_sequence_type ) {
	case MXT_SQ_STREAM:
		vinput_sp.num_parameters = 1;
		vinput_sp.parameter_array[0] = exposure_time;
		break;
	case MXT_SQ_MULTIFRAME:
		vinput_sp.num_parameters = 3;
		vinput_sp.parameter_array[0] = num_frames;
		vinput_sp.parameter_array[1] = exposure_time;
		vinput_sp.parameter_array[2] = exposure_time;
		break;
	case MXT_SQ_DURATION:
		vinput_sp.num_parameters = 1;
		vinput_sp.parameter_array[0] = num_frames;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Video sequence type %lu is not supported for "
		"video input '%s' when used by area detector '%s'.",
			video_sequence_type,
			video_input_record->name,
			ad->record->name );
		break;
	}

	mx_status = mx_video_input_set_sequence_parameters(
					video_input_record, &vinput_sp );

	if ( mx_status.code != MXE_SUCCESS )
 		return mx_status;

	/* Tell the video capture card to get ready for frames. */

#if MXD_XINEOS_GIGE_DEBUG_ARM
	MX_DEBUG(-2,("%s: sequence type = %ld", fname, sp->sequence_type));
#endif

	mx_status = mx_video_input_arm( video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xineos_gige_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_xineos_gige_trigger()";

	MX_XINEOS_GIGE *xineos_gige = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double pulse_width;
	double video_pulse_period, video_pulse_width;
	unsigned long num_pulses;
	mx_status_type mx_status;

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	if ( ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {
		/* If the area detector is using a 'real' external trigger,
		 * then we do not need to do anything in this function.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	if ( xineos_gige->start_with_pulse_generator == FALSE ) {
		/* If we are not using a pulse generator, then we just
		 * pass the request on to the video card.
		 */

		mx_status = mx_video_input_trigger(
				xineos_gige->video_input_record );

		return mx_status;
	}

	/* If we are using the pulse generator, then we need to calculate
	 * the pulse width and the number of pulses.  For this detector,
	 * there is no gap time, so we force the pulse period to be the
	 * same as the pulse width.
	 */

	sp = &(ad->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		pulse_width = sp->parameter_array[0];
		num_pulses = 1;
		break;

	case MXT_SQ_MULTIFRAME:
		pulse_width = sp->parameter_array[1];
		num_pulses = sp->parameter_array[0];

		sp->parameter_array[2] = pulse_width;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only one-shot and multiframe sequences can be used "
		"with internal triggering for area detector '%s'.",
			ad->record->name );
		break;
	}

	MX_DEBUG(-2,("%s: pulse_width = %f, num_pulses = %lu",
		fname, pulse_width, num_pulses));

	if ( pulse_width < xineos_gige->minimum_frame_time )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"An exposure time of %f seconds was requested for "
		"area detector '%s', but the minimum exposure time "
		"for this detector is %f seconds.",
			pulse_width, ad->record->name,
			xineos_gige->minimum_frame_time );
	}

	video_pulse_width = MXT_XINEOS_GIGE_TRIGGER_WIDTH;

	video_pulse_period = pulse_width;

	/* Configure the pulse generator for this sequence. */

	mx_status = mx_pulse_generator_setup(
			xineos_gige->pulse_generator_record,
			video_pulse_period,
			video_pulse_width,
			num_pulses, 0.0,
			MXF_PGN_PULSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Finish by starting the pulse generator. */

	mx_status = mx_pulse_generator_start(
				xineos_gige->pulse_generator_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xineos_gige_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_xineos_gige_stop()";

	MX_XINEOS_GIGE *xineos_gige = NULL;
	mx_status_type mx_status;

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_stop( xineos_gige->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xineos_gige_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_xineos_gige_get_extended_status()";

	MX_XINEOS_GIGE *xineos_gige = NULL;
	unsigned long vinput_status;
	mx_status_type mx_status;

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	if ( ad->correction_frames_to_skip != 0 ) {
		MX_DEBUG(-2,("%s: ad->correction_frames_to_skip = %ld",
			fname, ad->correction_frames_to_skip));
	}
#endif

	mx_status = mx_video_input_get_extended_status(
				xineos_gige->video_input_record,
				&(ad->last_frame_number),
				&(ad->total_num_frames),
				&vinput_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->status = 0;

	if ( vinput_status & MXSF_VIN_IS_BUSY ) {

		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xineos_gige_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_xineos_gige_readout_frame()";

	MX_XINEOS_GIGE *xineos_gige = NULL;
	mx_status_type mx_status;

#if MXD_XINEOS_GIGE_DEBUG_READOUT_TIMING
	MX_HRT_TIMING readout_measurement;
	MX_HRT_TIMING trim_measurement;
	MX_HRT_TIMING save_raw_measurement;
	MX_HRT_TIMING total_measurement;

	MX_HRT_START(total_measurement);
#endif

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_get_frame(
		xineos_gige->video_input_record,
		ad->frame_number, &(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the sequence was started using the pulse generator, then the
	 * video card will be in DURATION mode.  In DURATION mode, the image
	 * frame returned by the video input driver will have a header that
	 * states that the exposure time is 1 second, since the video input
	 * driver does not actually know what the exposure time is supposed
	 * to be in this case.  Therefore, we must manually set the exposure
	 * time here based on the sequence parameters known only by the area
	 * detector record.
	 */

	if ( xineos_gige->using_external_video_duration_mode ) {
		struct timespec exposure_timespec;
		double exposure_time;

		mx_status = mx_area_detector_get_exposure_time( ad->record,
							&exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_timespec =
		    mx_convert_seconds_to_high_resolution_time( exposure_time );

		MXIF_EXPOSURE_TIME_SEC( ad->image_frame ) =
						exposure_timespec.tv_sec;
		MXIF_EXPOSURE_TIME_NSEC( ad->image_frame ) =
						exposure_timespec.tv_nsec;
	}

	/* Save a pointer in the image frame structure that has information
	 * that will be needed if we are going to write a NOIR header.
	 */

	ad->image_frame->application_ptr = xineos_gige->image_noir_info;

	if ( xineos_gige->dump_pixel_values ) {
		char frame_label[20];

		snprintf( frame_label, sizeof(frame_label),
			"Image frame %ld", ad->readout_frame );

		(void ) mx_image_dump_pixel_range( stderr, ad->image_frame,
						frame_label, 0, 10 );
	}

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mxd_xineos_gige_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_xineos_gige_correct_frame()";

	MX_XINEOS_GIGE *xineos_gige = NULL;
	mx_status_type mx_status;

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rdi_correct_frame( ad,
				xineos_gige->minimum_pixel_value,
				xineos_gige->saturation_pixel_value,
				0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xineos_gige_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_xineos_gige_get_parameter()";

	MX_XINEOS_GIGE *xineos_gige = NULL;
	MX_RECORD *video_input_record;
	mx_status_type mx_status;

#if MXD_XINEOS_GIGE_DEBUG
	char buffer[100];
#endif

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, ad->record->name,
		mx_get_parameter_name_from_type( ad->record,
						ad->parameter_type,
						buffer, sizeof(buffer) ),
		ad->parameter_type));
#endif
	video_input_record = xineos_gige->video_input_record;

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

		mx_status = mx_image_get_image_format_name_from_type(
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

	case MXLV_AD_TRIGGER_MODE:

		/* If the detector is started using the pulse generator, then
		 * the video card is guaranteed to be in DURATION mode,
		 * regardless of what mode the area detector thinks it is in.
		 * Thus, we only ask the video card for its trigger mode if
		 * we are _not_ starting the sequence using the pulse
		 * generator.
		 */

		if ( xineos_gige->start_with_pulse_generator == FALSE ) {
			mx_status = mx_video_input_get_trigger_mode(
				video_input_record, &(ad->trigger_mode) );
		}
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
mxd_xineos_gige_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_xineos_gige_set_parameter()";

	MX_XINEOS_GIGE *xineos_gige = NULL;
	MX_RECORD *video_input_record;
	MX_SEQUENCE_PARAMETERS *ad_seq = NULL;
	double *param_array = NULL;
	mx_status_type mx_status;

#if MXD_XINEOS_GIGE_DEBUG
	char buffer[100];
#endif

	static long allowed_binsize[] = { 1, 2 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter '%s' (%ld)",
		fname, ad->record->name,
		mx_get_parameter_name_from_type( ad->record,
						ad->parameter_type,
						buffer, sizeof(buffer) ),
		ad->parameter_type));
#endif
	ad_seq = &(ad->sequence_parameters);

	param_array = ad_seq->parameter_array;

	video_input_record = xineos_gige->video_input_record;

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		/* The Xineos GigE detector does not support binning. */

		ad->binsize[0] = 1;
		ad->binsize[1] = 1;

		mx_status = mx_area_detector_compute_new_binning( ad,
							ad->parameter_type,
							num_allowed_binsizes,
							allowed_binsize );
		break;

	case MXLV_AD_TRIGGER_MODE:

		/* If the detector is started using the pulse generator, then
		 * the video card is guaranteed to be in DURATION mode,
		 * regardless of what mode the area detector thinks it is in.
		 * Thus, we only attempt to set the video card's trigger mode
		 * if we are _not_ starting the sequence using the pulse
		 * generator.
		 */

		if ( xineos_gige->start_with_pulse_generator == FALSE ) {
			mx_status = mx_video_input_set_trigger_mode(
				video_input_record, ad->trigger_mode );
		}
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 

		/* Sending sequence parameters to the video capture card
 		 * is a job better done by the mxd_xineos_gige_arm() methods,
 		 * since it has access to information that is known only at
 		 * the time that we do the arm().  Thus, we defer this job
 		 * to the arm() method.
		 */
		break; 

	case MXLV_AD_MARK_FRAME_AS_SAVED:
		mx_status = mx_video_input_mark_frame_as_saved(
				video_input_record,
				ad->mark_frame_as_saved );
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

/*--------------------------------------------------------------------------*/

/* NOTE: This function is not called when running in an MX server. */

MX_EXPORT mx_status_type
mxd_xineos_gige_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_xineos_gige_measure_correction()";

	MX_XINEOS_GIGE *xineos_gige = NULL;
	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr = NULL;
#if 0
	double gate_time;
#endif
	long last_frame_number, old_last_frame_number;
	long total_num_frames;
	unsigned long i, ad_status;
	mx_status_type mx_status;

	mx_status = mxd_xineos_gige_get_pointers( ad, &xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_XINEOS_GIGE_DEBUG_MEASURE_CORRECTION
	MX_DEBUG(-2,("%s invoked for detector '%s'", fname, ad->record->name ));
#endif

	if ( ad->correction_measurement_type == MXFT_AD_FLAT_FIELD_FRAME ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Direct measurement of the non-uniformity frame "
		"for detector '%s' is not supported.", ad->record->name );
	}

	/* Setup a correction measurement structure to contain the
	 * correction information.
	 */

	mx_status = mx_area_detector_prepare_for_correction( ad, &corr );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( ad, NULL );
		return mx_status;
	}

	/* Put the area detector into the sequence mode appropriate for
	 * the type of triggering they are doing.
	 */

	mx_status = mx_area_detector_get_trigger_mode( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ad->trigger_mode ) {
	case MXT_IMAGE_INTERNAL_TRIGGER:
		mx_status = mx_area_detector_set_multiframe_mode( ad->record,
							corr->num_exposures,
							corr->exposure_time,
							corr->exposure_time );
		break;
	case MXT_IMAGE_EXTERNAL_TRIGGER:
		mx_status = mx_area_detector_set_duration_mode( ad->record,
							corr->num_exposures );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Requested trigger mode %ld is not supported for detector '%s'.",
				ad->trigger_mode, ad->record->name );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( ad, NULL );
		return mx_status;
	}

	/* Since this is a dark current correction, we must disable
	 * the shutter.
	 */

	mx_status = mx_area_detector_set_shutter_enable( ad->record, FALSE );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( ad, NULL );
		return mx_status;
	}

	/* Start the sequence. */

	mx_status = mx_area_detector_start( ad->record );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( ad, NULL );
		return mx_status;
	}

	/* Readout the frames as they appear. */

	old_last_frame_number = -1;

	for(;;) {
		mx_status = mx_area_detector_get_extended_status( ad->record,
							&last_frame_number,
							&total_num_frames,
							&ad_status );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( ad, NULL );
			return mx_status;
		}

		if ( last_frame_number > old_last_frame_number ) {
			for ( i = (old_last_frame_number + 1);
				i <= last_frame_number;
				i++ )
			{
				mx_status =
				    mx_area_detector_process_correction_frame(
					ad, i,
					corr->desired_correction_flags,
					0,
					corr->sum_array );

				if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( ad, NULL );
					return mx_status;
				}

			}

			if ( (last_frame_number + 1) >= corr->num_exposures ) {

				break;    /* Exit the outer for() loop. */

			}

			old_last_frame_number = last_frame_number;
		}

		mx_msleep(10);
	}

	mx_status = mx_area_detector_finish_correction_calculation( ad, corr );

	return mx_status;
}

