/*
 * Name:    d_radicon_xineos_gige.c
 *
 * Purpose: MX driver for the Radicon Xineos GigE series detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_RADICON_XINEOS_GIGE_DEBUG				TRUE

#define MXD_RADICON_XINEOS_GIGE_DEBUG_READOUT_TIMING		FALSE

#define MXD_RADICON_XINEOS_GIGE_DEBUG_MEASURE_CORRECTION	FALSE

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
#include "d_radicon_xineos_gige.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_radicon_xineos_gige_record_function_list = {
	mxd_radicon_xineos_gige_initialize_driver,
	mxd_radicon_xineos_gige_create_record_structures,
	mx_area_detector_finish_record_initialization,
	NULL,
	NULL,
	mxd_radicon_xineos_gige_open
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_xineos_gige_ad_function_list = {
	mxd_radicon_xineos_gige_arm,
	mxd_radicon_xineos_gige_trigger,
	mxd_radicon_xineos_gige_stop,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_radicon_xineos_gige_get_extended_status,
	mxd_radicon_xineos_gige_readout_frame,
	mxd_radicon_xineos_gige_correct_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_radicon_xineos_gige_get_parameter,
	mxd_radicon_xineos_gige_set_parameter,
	mxd_radicon_xineos_gige_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_radicon_xineos_gige_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_RADICON_XINEOS_GIGE_STANDARD_FIELDS
};

long mxd_radicon_xineos_gige_num_record_fields
		= sizeof( mxd_radicon_xineos_gige_rf_defaults )
		/ sizeof( mxd_radicon_xineos_gige_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_radicon_xineos_gige_rfield_def_ptr
			= &mxd_radicon_xineos_gige_rf_defaults[0];

/*---*/

static mx_status_type
mxd_radicon_xineos_gige_get_pointers( MX_AREA_DETECTOR *ad,
			MX_RADICON_XINEOS_GIGE **radicon_xineos_gige,
			const char *calling_fname )
{
	static const char fname[] = "mxd_radicon_xineos_gige_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (radicon_xineos_gige == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RADICON_XINEOS_GIGE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*radicon_xineos_gige = (MX_RADICON_XINEOS_GIGE *)
				ad->record->record_type_struct;

	if ( *radicon_xineos_gige == (MX_RADICON_XINEOS_GIGE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_RADICON_XINEOS_GIGE pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_radicon_xineos_gige_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige;

	/* Use calloc() here instead of malloc(), so that a bunch of
	 * fields that are never touched will be initialized to 0.
	 */

	ad = (MX_AREA_DETECTOR *) calloc( 1, sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	radicon_xineos_gige = (MX_RADICON_XINEOS_GIGE *)
				calloc( 1, sizeof(MX_RADICON_XINEOS_GIGE) );

	if ( radicon_xineos_gige == (MX_RADICON_XINEOS_GIGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for an MX_RADICON_XINEOS_GIGE structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = radicon_xineos_gige;
	record->class_specific_function_list = 
			&mxd_radicon_xineos_gige_ad_function_list;

	ad->record = record;
	radicon_xineos_gige->record = record;

	radicon_xineos_gige->image_noir_info = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_radicon_xineos_gige_open()";

	MX_AREA_DETECTOR *ad;
	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	MX_RECORD *video_input_record;
	char non_uniformity_filename[MXU_FILENAME_LENGTH+1];
	long vinput_framesize[2];
	long video_framesize[2];
	long i;
	char c;
	unsigned long mask, num_bytes_available;
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	video_input_record = radicon_xineos_gige->video_input_record;

	if ( video_input_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The video_input_record pointer for detector '%s' is NULL.",
			record->name );
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

	ad->correction_calc_format = MXT_IMAGE_FORMAT_DOUBLE;

	ad->correction_measurement_sequence_type = MXT_SQ_MULTIFRAME;

	radicon_xineos_gige->saturation_pixel_value = 14000.0;
	radicon_xineos_gige->minimum_pixel_value = 5.0;

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

	if ( strlen( radicon_xineos_gige->pulse_generator_name ) == 0 ) {
		radicon_xineos_gige->pulse_generator_record = NULL;
	} else {
		radicon_xineos_gige->pulse_generator_record =
			mx_get_record( record,
				radicon_xineos_gige->pulse_generator_name );

		if ( radicon_xineos_gige->pulse_generator_record == NULL ) {
			(void) mx_error( MXE_NOT_FOUND, fname,
			"Internal trigger record '%s' for detector '%s' "
			"was not found in the MX database.",
				radicon_xineos_gige->pulse_generator_name,
				record->name );

		} else {
			/* Is the trigger record a pulse generator record? */

			unsigned long mx_class;

			mx_class =
			 radicon_xineos_gige->pulse_generator_record->mx_class;

			if ( mx_class != MXC_PULSE_GENERATOR ) {
				(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Internal trigger record '%s' for "
				"detector '%s' is NOT a pulse generator "
				"record.  It will be IGNORED!",
				    radicon_xineos_gige->pulse_generator_name,
				    record->name );
				
				radicon_xineos_gige->pulse_generator_record
							= NULL;
			}
		}
	}

	radicon_xineos_gige->use_pulse_generator = FALSE;

	/* Internally triggered one shot and multiframe sequences will
	 * use the pulse generator for exposures greater than or equal
	 * to this time if a pulse generator is present.
	 */

	radicon_xineos_gige->pulse_generator_time_threshold = 1.0;

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
	ad->flood_field_image_format = MXT_IMAGE_FORMAT_FLOAT;

	ad->measure_dark_current_correction_flags = MXFT_AD_MASK_FRAME;
	ad->measure_flood_field_correction_flags =
			MXFT_AD_MASK_FRAME | MXFT_AD_DARK_CURRENT_FRAME;

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure automatic saving or loading of image frames. */

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_LOAD_FRAME_AFTER_ACQUISITION;

	if ( ad->area_detector_flags & mask ) {
		mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXD_RADICON_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_xineos_gige_arm()";

	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	MX_RECORD *video_input_record = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	double exposure_time;
	long vinput_trigger_mode;
	mx_status_type mx_status;

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	video_input_record = radicon_xineos_gige->video_input_record;

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

		if ( radicon_xineos_gige->image_noir_info == NULL ) {
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
				&(radicon_xineos_gige->image_noir_info) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mx_image_noir_update(
				radicon_xineos_gige->image_noir_info );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Get the exposure time, since we will need it in a moment. */

	sp = &(ad->sequence_parameters);

	mx_status = mx_sequence_get_exposure_time( sp, 0, &exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Is this an internally triggered sequence that needs to
	 * use a pulse generator?
	 */

	radicon_xineos_gige->use_pulse_generator = FALSE;

	if ( ( radicon_xineos_gige->pulse_generator_record != NULL )
	  && ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER )
	  && ( exposure_time >=
		radicon_xineos_gige->pulse_generator_time_threshold ) )
	{
		radicon_xineos_gige->use_pulse_generator = TRUE;
	}

	/* If we are using a pulse generator to generate "internal" triggers,
	 * then tell the video card to expect an _external_ trigger.
	 * Otherwise, just pass the area detector record's trigger mode
	 * through to the video card.
	 */

	if ( radicon_xineos_gige->use_pulse_generator ) {
		vinput_trigger_mode = MXT_IMAGE_EXTERNAL_TRIGGER;
	} else {
		vinput_trigger_mode = ad->trigger_mode;
	}

	mx_status = mx_video_input_set_trigger_mode( video_input_record,
							vinput_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the video capture card to get ready for frames. */

	MX_DEBUG(-2,("%s: sequence type = %d", fname, sp->sequence_type));

	mx_status = mx_video_input_arm( video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_xineos_gige_trigger()";

	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double pulse_period, pulse_width;
	unsigned long num_pulses;
	mx_status_type mx_status;

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	if ( ( ad->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) == 0 ) {
		/* If the area detector is using a 'real' external trigger,
		 * then we do not need to do anything in this function.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	if ( radicon_xineos_gige->use_pulse_generator == FALSE ) {
		/* If we are not using a pulse generator, then we just
		 * pass the request on to the video card.
		 */

		mx_status = mx_video_input_trigger(
				radicon_xineos_gige->video_input_record );

		return mx_status;
	}

	/* If we are using the pulse generator, then we need to calculate
	 * the pulse width, pulse period, and number of pulses.
	 */

	sp = &(ad->sequence_parameters);

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		pulse_width = sp->parameter_array[0];
		pulse_period = 1.001 * pulse_width;
		num_pulses = 1;
		break;

	case MXT_SQ_MULTIFRAME:
		pulse_width = sp->parameter_array[1];
		pulse_period = sp->parameter_array[2];
		num_pulses = sp->parameter_array[0];
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only one-shot and multiframe sequences can be used "
		"with internal triggering for area detector '%s'.",
			ad->record->name );
		break;
	}

	/* Configure the pulse generator for this sequence. */

	mx_status = mx_pulse_generator_set_mode(
				radicon_xineos_gige->pulse_generator_record,
				MXF_PGN_PULSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_pulse_period(
				radicon_xineos_gige->pulse_generator_record,
				pulse_period );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_pulse_width(
				radicon_xineos_gige->pulse_generator_record,
				pulse_width );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_set_num_pulses(
				radicon_xineos_gige->pulse_generator_record,
				num_pulses );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Finish by starting the pulse generator. */

	mx_status = mx_pulse_generator_start(
				radicon_xineos_gige->pulse_generator_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_xineos_gige_stop()";

	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	mx_status_type mx_status;

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	mx_status = mx_video_input_stop(
				radicon_xineos_gige->video_input_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_radicon_xineos_gige_get_extended_status()";

	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	long vinput_last_frame_number;
	long vinput_total_num_frames;
	unsigned long vinput_status;
	mx_status_type mx_status;

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_extended_status(
				radicon_xineos_gige->video_input_record,
				&vinput_last_frame_number,
				&vinput_total_num_frames,
				&vinput_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->last_frame_number = vinput_last_frame_number;
	ad->total_num_frames = vinput_total_num_frames;

	ad->status = 0;

	if ( vinput_status & MXSF_VIN_IS_BUSY ) {

		ad->status |= MXSF_AD_ACQUISITION_IN_PROGRESS;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_xineos_gige_readout_frame()";

	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	mx_status_type mx_status;

#if MXD_RADICON_XINEOS_GIGE_DEBUG_READOUT_TIMING
	MX_HRT_TIMING readout_measurement;
	MX_HRT_TIMING trim_measurement;
	MX_HRT_TIMING save_raw_measurement;
	MX_HRT_TIMING total_measurement;

	MX_HRT_START(total_measurement);
#endif

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_video_input_get_frame(
		radicon_xineos_gige->video_input_record,
		ad->readout_frame, &(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save a pointer in the image frame structure that has information
	 * that will be needed if we are going to write a NOIR header.
	 */

	ad->image_frame->application_ptr = radicon_xineos_gige->image_noir_info;

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_xineos_gige_correct_frame()";

	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	unsigned long gige_flags, rdi_flags;
	mx_status_type mx_status;

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	gige_flags = radicon_xineos_gige->radicon_xineos_gige_flags;

	if (gige_flags & MXF_RADICON_XINEOS_GIGE_BYPASS_DC_EXPOSURE_TIME_TEST)
	{
		rdi_flags = MXF_RDI_BYPASS_DARK_CURRENT_EXPOSURE_TIME_TEST;
	} else {
		rdi_flags = 0;
	}

	mx_status = mx_rdi_correct_frame( ad,
				radicon_xineos_gige->minimum_pixel_value,
				radicon_xineos_gige->saturation_pixel_value,
				rdi_flags );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_xineos_gige_get_parameter()";

	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	MX_RECORD *video_input_record;
	mx_status_type mx_status;

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif
	video_input_record = radicon_xineos_gige->video_input_record;

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
		mx_status = mx_video_input_get_trigger_mode(
				video_input_record, &(ad->trigger_mode) );
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
mxd_radicon_xineos_gige_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_radicon_xineos_gige_set_parameter()";

	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	MX_RECORD *video_input_record;
	unsigned long sro_mode, trigger_mask;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	static long allowed_binsize[] = { 1, 2 };

	static int num_allowed_binsizes = sizeof( allowed_binsize )
						/ sizeof( allowed_binsize[0] );

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_XINEOS_GIGE_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif
	video_input_record = radicon_xineos_gige->video_input_record;

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
		mx_status = mx_video_input_set_trigger_mode(
				video_input_record, ad->trigger_mode );
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		mx_status = mx_video_input_set_sequence_parameters(
					video_input_record,
					&(ad->sequence_parameters) );
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

MX_EXPORT mx_status_type
mxd_radicon_xineos_gige_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxd_radicon_xineos_gige_measure_correction()";

	MX_RADICON_XINEOS_GIGE *radicon_xineos_gige = NULL;
	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr = NULL;
#if 0
	double gate_time;
#endif
	long last_frame_number, old_last_frame_number;
	unsigned long i, total_num_frames, ad_status;
	mx_status_type mx_status;

	mx_status = mxd_radicon_xineos_gige_get_pointers( ad,
						&radicon_xineos_gige, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_RADICON_XINEOS_GIGE_DEBUG_MEASURE_CORRECTION
	MX_DEBUG(-2,("%s invoked for detector '%s'", fname, ad->record->name ));
#endif

	if ( ad->correction_measurement_type == MXFT_AD_FLOOD_FIELD_FRAME ) {
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
		"Requested trigger mode %d is not supported for detector '%s'.",
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

