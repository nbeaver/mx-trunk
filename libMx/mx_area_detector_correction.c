/*
 * Name:    mx_area_detector_correction.c
 *
 * Purpose: Functions for correcting images acquired by an area detector.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_AREA_DETECTOR_DEBUG    			FALSE

#define MX_AREA_DETECTOR_DEBUG_DEZINGER			FALSE

#define MX_AREA_DETECTOR_DEBUG_FRAME_TIMING		FALSE

#define MX_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION	FALSE

#define MX_AREA_DETECTOR_DEBUG_CORRECTION		FALSE

#define MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING	FALSE

#define MX_AREA_DETECTOR_DEBUG_CORRECTION_FLAGS		FALSE

#define MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME	FALSE

#define MX_AREA_DETECTOR_DEBUG_CORRECTION_FILENAMES	FALSE

#define MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD	FALSE

#define MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE	FALSE

/*---*/

#define MX_AREA_DETECTOR_ENABLE_DATAFILE_AUTOSAVE	TRUE /* Leave this on */

/*---*/

#define MX_RDI_DEBUG_CORRECTION				FALSE

#define MX_RDI_DEBUG_CORRECTION_STATISTICS		FALSE

#define MX_RDI_DEBUG_CORRECTION_TIMING			FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_cfn.h"
#include "mx_hrt_debug.h"
#include "mx_memory.h"
#include "mx_image.h"
#include "mx_area_detector.h"

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_load_correction_files( MX_RECORD *record )
{
	static const char fname[] = "mx_area_detector_load_correction_files()";

	MX_AREA_DETECTOR *ad;
	char correction_filename[ MXU_FILENAME_LENGTH+1 ];
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name));

	MX_DEBUG(-2,("%s: ad->image_format = %ld", fname, ad->image_format));
	MX_DEBUG(-2,("%s: ad->framesize = (%ld,%ld)",
		fname, ad->framesize[0], ad->framesize[1]));
#endif

	if ( strlen(ad->mask_filename) > 0 ) {
		mx_status = mx_cfn_construct_filename( MX_CFN_CONFIG,
						ad->mask_filename,
						correction_filename,
						sizeof(correction_filename) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_FILENAMES
		MX_DEBUG(-2,("%s: mask filename = '%s'",
			fname, correction_filename));
#endif

		mx_status = mx_area_detector_load_frame( record,
							MXFT_AD_MASK_FRAME,
							correction_filename );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_NOT_FOUND) )
		{
			return mx_status;
		}
	}

	if ( strlen(ad->bias_filename) > 0 ) {
		mx_status = mx_cfn_construct_filename( MX_CFN_CONFIG,
						ad->bias_filename,
						correction_filename,
						sizeof(correction_filename) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_FILENAMES
		MX_DEBUG(-2,("%s: bias filename = '%s'",
			fname, correction_filename));
#endif

		mx_status = mx_area_detector_load_frame( record,
							MXFT_AD_BIAS_FRAME,
							correction_filename );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_NOT_FOUND) )
		{
			return mx_status;
		}
	}

	if ( strlen(ad->dark_current_filename) > 0 ) {
		mx_status = mx_cfn_construct_filename( MX_CFN_CONFIG,
						ad->dark_current_filename,
						correction_filename,
						sizeof(correction_filename) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_FILENAMES
		MX_DEBUG(-2,("%s: dark current filename = '%s'",
			fname, correction_filename));
#endif

		mx_status = mx_area_detector_load_frame( record,
						MXFT_AD_DARK_CURRENT_FRAME,
						correction_filename );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_NOT_FOUND) )
		{
			return mx_status;
		}
	}

	if ( strlen(ad->flood_field_filename) > 0 ) {
		mx_status = mx_cfn_construct_filename( MX_CFN_CONFIG,
						ad->flood_field_filename,
						correction_filename,
						sizeof(correction_filename) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_FILENAMES
		MX_DEBUG(-2,("%s: flood field filename = '%s'",
			fname, correction_filename));
#endif

		mx_status = mx_area_detector_load_frame( record,
						MXFT_AD_FLOOD_FIELD_FRAME,
						correction_filename );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_NOT_FOUND) )
		{
			return mx_status;
		}
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s complete for area detector '%s'.",
		fname, ad->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_get_correction_flags( MX_RECORD *record,
					unsigned long *correction_flags )
{
	static const char fname[] = "mx_area_detector_get_correction_flags()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_CORRECTION_FLAGS;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( correction_flags != NULL ) {
		*correction_flags = ad->correction_flags;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_correction_flags( MX_RECORD *record,
					unsigned long correction_flags )
{
	static const char fname[] = "mx_area_detector_set_correction_flags()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	if ( ad->dark_current_offset_can_change ) {
		mx_free( ad->dark_current_offset_array );
	}

	if ( ad->flood_field_scale_can_change ) {
		mx_free( ad->flood_field_scale_array );
	}

	ad->parameter_type = MXLV_AD_CORRECTION_FLAGS;
	ad->correction_flags = correction_flags;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_measure_correction_frame( MX_RECORD *record,
					long correction_measurement_type,
					double correction_measurement_time,
					long num_correction_measurements )
{
	static const char fname[] =
		"mx_area_detector_measure_correction_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *measure_correction_fn ) ( MX_AREA_DETECTOR * );
	unsigned long ad_flags;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
		if ( ad->dark_current_offset_can_change == FALSE ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The dark current for detector '%s' cannot be changed.",
				record->name );
		}
		break;
	case MXFT_AD_FLOOD_FIELD_FRAME:
		if ( ad->flood_field_scale_can_change == FALSE ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The flood field for detector '%s' cannot be changed.",
				record->name );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal measurement type %ld requested for detector '%s'",
			ad->correction_measurement_type,
			record->name );
		break;
	}

	measure_correction_fn = flist->measure_correction;

	if ( measure_correction_fn == NULL ) {
		measure_correction_fn =
			mx_area_detector_default_measure_correction;
	}

	ad->correction_measurement_type = correction_measurement_type;
	ad->correction_measurement_time = correction_measurement_time;
	ad->num_correction_measurements = num_correction_measurements;

	ad->correction_measurement_in_progress = TRUE;

	mx_status = (*measure_correction_fn)( ad );

	ad->correction_measurement_in_progress = FALSE;

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Some frame types need special things to happen after
	 * they are measured.
	 */

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
		if ( ad->dark_current_frame != NULL ) {
			mx_status = mx_image_get_exposure_time(
					ad->dark_current_frame,
					&(ad->dark_current_exposure_time) );
		}

		mx_free( ad->dark_current_offset_array );
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		if ( ad->flood_field_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->flood_field_frame, ad->mask_frame,
					&(ad->flood_field_average_intensity) );
		}

		mx_free( ad->flood_field_scale_array );
		break;
	}

	ad_flags = ad->area_detector_flags;

	if ( ad_flags & MXF_AD_SAVE_AVERAGED_CORRECTION_FRAME ) {
		mx_status = mx_area_detector_save_averaged_correction_frame(
				ad->record, correction_measurement_type );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_save_averaged_correction_frame( MX_RECORD *record,
					long correction_measurement_type )
{
	static const char fname[] =
			"mx_area_detector_save_averaged_correction_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *save_fn ) ( MX_AREA_DETECTOR * );

	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->correction_measurement_type = correction_measurement_type;

	/* If a custom save function is available, then use that. */

	save_fn = flist->save_averaged_correction_frame;

	if ( save_fn == NULL ) {
		save_fn =
		    mx_area_detector_default_save_averaged_correction_frame;
	}

	mx_status = (*save_fn)( ad );

	return mx_status;
}

	/* Otherwise, use this generic code. */

MX_EXPORT mx_status_type
mx_area_detector_default_save_averaged_correction_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_save_averaged_correction_frame()";

	MX_IMAGE_FRAME *averaged_frame;
	char *averaged_frame_filename;
	long averaged_frame_image_format;
	char frame_type_string[20];
	mx_status_type mx_status;

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
		averaged_frame = ad->dark_current_frame;
		averaged_frame_filename = ad->saved_dark_current_filename;
		averaged_frame_image_format = ad->correction_save_format;
		strlcpy( frame_type_string, "dark current",
					sizeof(frame_type_string) );
		break;
	case MXFT_AD_FLOOD_FIELD_FRAME:
		averaged_frame = ad->flood_field_frame;
		averaged_frame_filename = ad->saved_flood_field_filename;
		averaged_frame_image_format = ad->correction_save_format;
		strlcpy( frame_type_string, "flood field",
					sizeof(frame_type_string) );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector '%s' can only save dark current "
			"and flood field frames via this function.",
			ad->record->name );
		break;
	}

	if ( strlen( averaged_frame_filename ) == 0 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No saved %s filename has been specified yet for "
		"this function for area detector '%s'.",
			frame_type_string, ad->record->name );
	}

	mx_status = mx_image_write_file( averaged_frame,
					averaged_frame_image_format,
					averaged_frame_filename );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_use_scaled_dark_current_flag( MX_RECORD *record,
					mx_bool_type *use_scaled_dark_current )
{
	static const char fname[] =
			"mx_area_detector_get_use_scaled_dark_current_flag()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_USE_SCALED_DARK_CURRENT;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( use_scaled_dark_current != NULL ) {
		*use_scaled_dark_current = ad->use_scaled_dark_current;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_use_scaled_dark_current_flag( MX_RECORD *record,
					mx_bool_type use_scaled_dark_current )
{
	static const char fname[] =
			"mx_area_detector_set_use_scaled_dark_current_flag()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	/* Discard any existing dark current offset array, since
	 * turning on or off the use_scaled_dark_current flag can
	 * affect the computed exposure scale factor.
	 */

	mx_free( ad->dark_current_offset_array );

	ad->parameter_type = MXLV_AD_USE_SCALED_DARK_CURRENT;
	ad->use_scaled_dark_current = use_scaled_dark_current;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

/*---*/

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_get_correction_frame( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					unsigned long frame_type,
					char *frame_name,
					MX_IMAGE_FRAME **correction_frame )
{
	static const char fname[] = "mx_area_detector_get_correction_frame()";

	MX_IMAGE_FRAME **rebinned_frame = NULL;
	unsigned long image_width, image_height;
	unsigned long correction_width, correction_height;
	unsigned long rebinned_width, rebinned_height;
	mx_bool_type configuration_conflict;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_TIMING rebin_timing;
#endif

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,("\n----------------------------------------------------"));
	MX_DEBUG(-2,
	("%s invoked, image_frame = %p, frame_type = %lu, frame_name = '%s'",
		fname, image_frame, frame_type, frame_name));
#endif

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AREA_DETECTOR pointer passed was NULL." );
	}
	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The image_frame pointer passed was NULL." );
	}
	if ( correction_frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The correction_frame pointer passed was NULL." );
	}

	/* If the requested type of correction is not currently enabled
	 * for this area detector, then return a NULL correction frame.
	 */

	if ( ( ad->correction_flags & frame_type ) == 0 ) {
		*correction_frame = NULL;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the requested correction frame. */

	switch( frame_type ) {
	case MXFT_AD_MASK_FRAME:
		*correction_frame = ad->mask_frame;
		break;

	case MXFT_AD_BIAS_FRAME:
		*correction_frame = ad->bias_frame;
		break;

	case MXFT_AD_DARK_CURRENT_FRAME:
		*correction_frame = ad->dark_current_frame;
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		*correction_frame = ad->flood_field_frame;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image frame type %lu is not supported for area detector '%s'.",
			frame_type, ad->record->name );
		break;
	}

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,("%s: initial (*correction_frame) = %p",
		fname, *correction_frame));
#endif

	if ( *correction_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No %s frame has been loaded for area detector '%s'.",
			frame_name, ad->record->name );
	}

	image_width  = MXIF_ROW_FRAMESIZE(image_frame);
	image_height = MXIF_COLUMN_FRAMESIZE(image_frame);

	correction_width  = MXIF_ROW_FRAMESIZE(*correction_frame);
	correction_height = MXIF_COLUMN_FRAMESIZE(*correction_frame);

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,("%s: image_width = %lu, image_height = %lu",
		fname, image_width, image_height));

	MX_DEBUG(-2,("%s: correction_width = %lu, correction_height = %lu",
		fname, correction_width, correction_height));
#endif

	/* If the image frame and the default correction frame have
	 * the same dimensions, then we can return the correction frame
	 * as is.
	 */

	if ( (image_width == correction_width)
	  && (image_height == correction_height) )
	{

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
		MX_DEBUG(-2,
		("%s: Returning since the image sizes match.", fname));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, we check to see if the correction frame dimensions
	 * are an integer multiple or integer divisor of the image frame
	 * dimensions.
	 */

	configuration_conflict = FALSE;

#if 0
	/* FIXME: Ratios that are "close" to an integer _must_ be supported! */

	if ( correction_width >= image_width ) {
		if ( (correction_width % image_width) != 0 ) {
			configuration_conflict = TRUE;
		}
	} else {
		if ( (image_width % correction_width) != 0 ) {
			configuration_conflict = TRUE;
		}
	}

	if ( correction_height >= image_height ) {
		if ( (correction_height % image_height) != 0 ) {
			configuration_conflict = TRUE;
		}
	} else {
		if ( (image_height % correction_height) != 0 ) {
			configuration_conflict = TRUE;
		}
	}
#endif

	if ( configuration_conflict ) {
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"The dimensions of the %s frame (%lu,%lu) are not an integer "
		"multiple or integer divisor of the dimensions of the image "
		"frame (%lu,%lu) for area detector '%s'.  The %s frame cannot "
		"be used for image correction as long as this is the case.",
			frame_name, correction_width, correction_height,
			image_width, image_height, ad->record->name,
			frame_name );
	}

	/* Do we already have a rebinned correction frame with the right
	 * dimensions?  If so, we can reuse that frame.
	 */

	switch( frame_type ) {
	case MXFT_AD_MASK_FRAME:
		rebinned_frame = &(ad->rebinned_mask_frame);
		break;

	case MXFT_AD_BIAS_FRAME:
		rebinned_frame = &(ad->rebinned_bias_frame);
		break;

	case MXFT_AD_DARK_CURRENT_FRAME:
		rebinned_frame = &(ad->rebinned_dark_current_frame);
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		rebinned_frame = &(ad->rebinned_flood_field_frame);
		break;
	}

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,("%s: initial (*rebinned_frame) = %p",
		fname, *rebinned_frame));
#endif

	if ( *rebinned_frame != (MX_IMAGE_FRAME *) NULL ) {

		/* See if we can reuse the rebinned frame. */

		rebinned_width = MXIF_ROW_FRAMESIZE(*rebinned_frame);
		rebinned_height = MXIF_COLUMN_FRAMESIZE(*rebinned_frame);

		if ( (rebinned_width == image_width)
		  && (rebinned_height == image_height) )
		{

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
			MX_DEBUG(-2,
		    ("%s: Returning since we are reusing the rebinned frame.",
		    		fname));
#endif
			/* Yes, we can reuse the existing rebinned frame. */

			*correction_frame = *rebinned_frame;

			return MX_SUCCESSFUL_RESULT;
		}

		/* No, we cannot reuse the existing rebinned frame as is. */
	}

	/* Rebin the correction frame using 'rebinned_frame'
	 * as the destination.
	 */

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,
  ("%s: Rebinning the correction frame.  Width scale = %lu, Height scale = %lu",
  		fname, correction_width / image_width,
		correction_height / image_height));
#endif

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_START( rebin_timing );
#endif
	mx_status = mx_image_rebin( rebinned_frame, *correction_frame,
					image_width, image_height );

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( rebin_timing );
	MX_HRT_RESULTS( rebin_timing, fname, "Rebinning correction image." );
#endif

	if ( mx_status.code != MXE_SUCCESS ) 
		return mx_status;

	/* Return the rebinned frame. */

	*correction_frame = *rebinned_frame;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_correct_frame()";

	MX_IMAGE_FRAME *image_frame, *mask_frame, *bias_frame;
	MX_IMAGE_FRAME *dark_current_frame, *flood_field_frame;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	image_frame = ad->image_frame;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
	    "Area detector '%s' has not yet read out its first image frame.",
			ad->record->name );
	}

	/* Find the frame pointers for the image frames to be used. */

	mx_status = mx_area_detector_get_correction_frame( ad, image_frame,
							MXFT_AD_MASK_FRAME,
							"mask",
							&mask_frame );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_correction_frame( ad, image_frame,
							MXFT_AD_BIAS_FRAME,
							"bias",
							&bias_frame );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_correction_frame( ad, image_frame,
						MXFT_AD_DARK_CURRENT_FRAME,
						"dark current",
						&dark_current_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_correction_frame( ad, image_frame,
						MXFT_AD_FLOOD_FIELD_FRAME,
						"flood field",
						&flood_field_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_classic_frame_correction( ad->record,
				image_frame, mask_frame, bias_frame,
				dark_current_frame, flood_field_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	{
		uint16_t *image_data_array;
		long i;

		image_data_array = image_frame->image_data;

		for ( i = 0; i < 10; i++ ) {
			MX_DEBUG(-2,("%s: image_data_array[%ld] = %d",
				fname, i, image_data_array[i]));
		}
	}
#endif
	/* Write the bias offset to the header. */

	if ( bias_frame == NULL ) {
		MXIF_BIAS_OFFSET_MILLI_ADUS(image_frame) = 0;
	} else {
		MXIF_BIAS_OFFSET_MILLI_ADUS(image_frame) =
			mx_round( 1000.0 * ad->bias_average_intensity );
	}

	return mx_status;
}

/*=======================================================================*/

#define MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION \
do {                                                                          \
	if ( ad->dezinger_correction_frame ) {                                             \
		for ( i = 0; i < num_exposures; i++ ) {			      \
			if ( dezinger_frame_array[i] != NULL ) {	      \
				mx_image_free( dezinger_frame_array[i] );     \
			}						      \
		}							      \
		mx_free( dezinger_frame_array );                              \
	} else {                                                              \
		mx_free( sum_array );	                                      \
	}                                                                     \
	(void) mx_area_detector_set_shutter_enable(ad->record, TRUE);         \
} while (0)


/*---*/

MX_EXPORT mx_status_type
mx_area_detector_default_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_measure_correction()";

	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr;
	long n;
	unsigned long ad_status;
	mx_status_type mx_status;
	MX_IMAGE_FRAME **dezinger_frame_ptr;

#if MX_AREA_DETECTOR_DEBUG_DEZINGER
	MX_HRT_TIMING measurement;
#endif

	mx_status = mx_area_detector_get_pointers( ad->record,
						NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));

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

	/* Setup a correction measurement structure to contain the
	 * correction information.
	 */

	mx_status = mx_area_detector_prepare_for_correction( ad, &corr );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( ad, NULL );
		return mx_status;
	}

	/* Put the area detector in One-shot mode. */

	mx_status = mx_area_detector_set_one_shot_mode( ad->record,
							corr->exposure_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	/* If this is a dark current correction, disable the shutter. */

	if ( ad->correction_measurement_type == MXFT_AD_DARK_CURRENT_FRAME ) {
		mx_status = mx_area_detector_set_shutter_enable(
							ad->record, FALSE );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( NULL, corr );
			return mx_status;
		}
	}

	/* Take the requested number of exposures and sum together
	 * the pixels from each exposure.
	 */

	for ( n = 0; n < corr->num_exposures; n++ ) {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Exposure %ld", fname, n));
#endif

		/* Start the exposure. */

		mx_status = mx_area_detector_start( ad->record );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( NULL, corr );
			return mx_status;
		}

		/* Wait for the exposure to end. */

		for(;;) {
			mx_status = mx_area_detector_get_status( ad->record,
								&ad_status );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_area_detector_cleanup_after_correction(
								NULL, corr );
				return mx_status;
			}

			if ((ad_status & MXSF_AD_ACQUISITION_IN_PROGRESS) == 0)
			{

				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}

		/* Readout the frame, process it, and copy it to the
		 * dezinger array.
		 */

		if ( ad->dezinger_correction_frame ) {
			dezinger_frame_ptr = &(corr->dezinger_frame_array[n]);
		} else {
			dezinger_frame_ptr = NULL;
		}

		mx_status = mx_area_detector_process_correction_frame(
						ad, 0,
						corr->desired_correction_flags,
						dezinger_frame_ptr,
						corr->sum_array );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( NULL, corr );
			return mx_status;
		}

		mx_msleep(10);
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Calculating normalized pixels.", fname));
#endif

	mx_status = mx_area_detector_finish_correction_calculation( ad, corr );

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: correction measurement complete.", fname));
#endif
	
	return mx_status;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT void
mx_area_detector_cleanup_after_correction( MX_AREA_DETECTOR *ad,
			MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr_ptr )
{
	static const char fname[] =
		"mx_area_detector_cleanup_after_correction()";

	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr;

	if ( corr_ptr != NULL ) {
		corr = corr_ptr;
	} else
	if ( ad != NULL ) {
		corr = ad->correction_measurement;
	} else {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"Both the MX_AREA_DETECTOR_CORRECTION_MEASUREMENT and "
		"the MX_AREA_DETECTOR pointers are NULL!" );

		return;
	}

	if ( corr == NULL ) {
		ad->correction_measurement_in_progress = FALSE;
		return;
	}

	if ( ad == NULL ) {
		ad = corr->area_detector;
	}

	/* Restore the trigger mode. */

	(void) mx_area_detector_set_trigger_mode( ad->record,
						corr->saved_trigger_mode );

	/* Enable the area detector shutter. */

	if ( ( ad != NULL )
	  && ( ad->correction_measurement_type == MXFT_AD_DARK_CURRENT_FRAME ) )
	{
		(void) mx_area_detector_set_shutter_enable( ad->record, TRUE );
	}

	/* Free correction measurement data structures. */

	if ( corr->dezinger_frame_array != (MX_IMAGE_FRAME **) NULL ) {
		long n;

		for ( n = 0; n < corr->num_exposures; n++ ) {
			mx_image_free( corr->dezinger_frame_array[n] );
		}

		mx_free( corr->dezinger_frame_array );
	}

	if ( corr->sum_array != NULL ) {
		mx_free( corr->sum_array );
	}

	if ( ad == NULL ) {
		if ( corr->area_detector != NULL ) {
			corr->area_detector->correction_measurement = NULL;
		}
	} else {
		ad->correction_measurement = NULL;
	}

#if MX_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	MX_DEBUG(-2,("%s: FREE-ing corr = %p", fname, corr));
#endif

	mx_free( corr );

	ad->correction_measurement_in_progress = FALSE;

	return;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_prepare_for_correction( MX_AREA_DETECTOR *ad,
			MX_AREA_DETECTOR_CORRECTION_MEASUREMENT **corr_ptr )
{
	static const char fname[] = "mx_area_detector_prepare_for_correction()";

	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr;
	long pixels_per_frame, saved_trigger_mode;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}
	if ( corr_ptr == (MX_AREA_DETECTOR_CORRECTION_MEASUREMENT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The pointer to an MX_AREA_DETECTOR_CORRECTION_MEASUREMENT "
		"pointer is NULL." );
	}
	if ( ad->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_AREA_DETECTOR %p is NULL.", ad );
	}

	/* Read the existing trigger mode. */

	mx_status = mx_area_detector_get_trigger_mode( ad->record,
						&saved_trigger_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate a structure to contain the current state of
	 * the correction measurement process.
	 */

	corr = malloc( sizeof(MX_AREA_DETECTOR_CORRECTION_MEASUREMENT) );

#if MX_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	mx_global_debug_pointer[1] = corr;

	MX_DEBUG(-2,("%s: MALLOC: corr = %p, global[1] = %p",
		fname, corr, mx_global_debug_pointer[1]));
#endif

	if ( corr == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an "
		"MX_AREA_DETECTOR_CORRECTION_MEASUREMENT structure "
		"for record '%s'.", ad->record->name );
	}

	*corr_ptr = corr;

	memset( corr, 0, sizeof(MX_AREA_DETECTOR_CORRECTION_MEASUREMENT) );

	corr->area_detector = ad;

#if MX_AREA_DETECTOR_DEBUG_MEMORY_CORRUPTION
	mx_global_debug_pointer[0] = ad;

	MX_DEBUG(-2,("%s: ad = %p, corr->area_detector = %p, global[0] = %p",
		fname, ad, corr->area_detector, mx_global_debug_pointer[0]));
#endif

	corr->exposure_time = ad->correction_measurement_time;

	corr->num_exposures = ad->num_correction_measurements;

	if ( corr->num_exposures <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of exposures (%ld) requested for "
		"area detector '%s'.  The minimum number of exposures "
		"allowed is 1.",  corr->num_exposures, ad->record->name );
	}

	pixels_per_frame = ad->framesize[0] * ad->framesize[1]; 

	/* If ad->correction_frames_to_skip is non-zero, then we have to 
	 * add additional frames to the measurement sequence and then skip
	 * the matching number of frames at the start of the exposure.
	 */

	corr->raw_num_exposures_to_skip = (long) ad->correction_frames_to_skip;

	corr->raw_num_exposures =
		corr->num_exposures + corr->raw_num_exposures_to_skip;

	/* Save the existing trigger mode for later restoration. */

	corr->saved_trigger_mode = saved_trigger_mode;

	/* Set a flag that says a correction frame measurement is in progress.*/

	ad->correction_measurement_in_progress = TRUE;

	/* The correction frames will originally be read into the image frame.
	 * We must make sure that the image frame is big enough to hold
	 * the image data.
	 */

	mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	/* Make sure that the destination image frame is already big enough
	 * to hold the image frame that we are going to put in it.
	 */

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:

		corr->destination_frame = ad->dark_current_frame;

		corr->desired_correction_flags =
			ad->measure_dark_current_correction_flags;
		break;
	
	case MXFT_AD_FLOOD_FIELD_FRAME:

		corr->destination_frame = ad->flood_field_frame;

		corr->desired_correction_flags = 
			ad->measure_flood_field_correction_flags;

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
			ad->correction_measurement_type, ad->record->name );
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_area_detector_cleanup_after_correction( NULL, corr );
		return mx_status;
	}

	corr->dezinger_frame_array = NULL;
	corr->sum_array = NULL;

	if ( ad->dezinger_correction_frame ) {

		if ( corr->num_exposures < 2 ) {
			mx_area_detector_cleanup_after_correction( NULL, corr );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Area detector '%s' cannot perform dezingering "
			"of correction frames if only 1 measurement "
			"is to be performed.", ad->record->name );
		}

		corr->dezinger_frame_array =
		    calloc( corr->num_exposures, sizeof(MX_IMAGE_FRAME *) );

		if ( corr->dezinger_frame_array == (MX_IMAGE_FRAME **) NULL )
		{
			mx_area_detector_cleanup_after_correction( NULL, corr );

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"a %ld element array of MX_IMAGE_FRAME pointers.", 
				corr->num_exposures );
		}
	} else {
		/* Do not dezinger */

		/* Allocate a double precision array to store
		 * intermediate sums.
		 */

		corr->sum_array =
			calloc( pixels_per_frame, sizeof(double) );

		if ( corr->sum_array == (double *) NULL ) {
			mx_area_detector_cleanup_after_correction( NULL, corr );

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"a %ld element array of doubles.", pixels_per_frame );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_process_correction_frame( MX_AREA_DETECTOR *ad,
					long frame_number,
					unsigned long desired_correction_flags,
					MX_IMAGE_FRAME **dezinger_frame_ptr,
					double *sum_array )
{
	static const char fname[] =
		"mx_area_detector_process_correction_frame()";

	long i, pixels_per_frame;
	void *void_image_data_pointer;
	uint16_t *src_array;
	size_t image_length;

	unsigned long saved_correction_flags;
	mx_bool_type saved_bias_corr_after_flood;
	mx_status_type mx_status, mx_status2;

	/* Readout the frame into ad->image_frame. */

	mx_status = mx_area_detector_readout_frame( ad->record, frame_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Perform any necessary image corrections. */

	if ( desired_correction_flags != 0 ) {

		mx_status = mx_area_detector_get_correction_flags(
				ad->record, &saved_correction_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_area_detector_set_correction_flags(
				ad->record, desired_correction_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		saved_bias_corr_after_flood = ad->bias_corr_after_flood;
		ad->bias_corr_after_flood = FALSE;

		mx_status = mx_area_detector_correct_frame(ad->record);

		ad->bias_corr_after_flood = saved_bias_corr_after_flood;

		mx_status2 = mx_area_detector_set_correction_flags(
				ad->record, saved_correction_flags );

		if ( mx_status2.code != MXE_SUCCESS )
			return mx_status2;

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( ad->dezinger_correction_frame ) {
		/* Copy the image frame to the dezinger frame array. */

		mx_status = mx_image_copy_frame( ad->image_frame,
						dezinger_frame_ptr );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		if ( sum_array == (double *) NULL ) {
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"sum_array pointer was NULL when dezinger was FALSE." );
		}

		/* Get the image data pointer. */

		mx_status = mx_image_get_image_data_pointer( ad->image_frame,
					&image_length,
					&void_image_data_pointer );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		src_array = void_image_data_pointer;

		/* Add the pixels in this image to the sum array. */

		pixels_per_frame = ad->framesize[0] * ad->framesize[1];

		for ( i = 0; i < pixels_per_frame; i++ ) {
			sum_array[i] += (double) src_array[i];
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_finish_correction_calculation( MX_AREA_DETECTOR *ad,
				MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr )
{
	static const char fname[] =
		"mx_area_detector_finish_correction_calculation()";

	MX_IMAGE_FRAME *dest_frame;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *geometrical_correction_fn ) ( MX_AREA_DETECTOR *,
							MX_IMAGE_FRAME * );
	uint16_t *u16_dest_array;
	float *flt_dest_array;
	unsigned long dest_format;
	struct timespec exposure_timespec;
	time_t time_buffer;
	long i, pixels_per_frame;
	double temp_double;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_DEZINGER
	MX_HRT_TIMING measurement;
#endif

	mx_status = mx_area_detector_get_pointers( ad->record,
						NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	geometrical_correction_fn = flist->geometrical_correction;

	if ( geometrical_correction_fn == NULL ) {
		geometrical_correction_fn =
			mx_area_detector_default_geometrical_correction;
	}

	dest_frame = corr->destination_frame;

	if ( ad->dezinger_correction_frame ) {

#if MX_AREA_DETECTOR_DEBUG_DEZINGER
		MX_HRT_START( measurement );
#endif

		mx_status = mx_image_dezinger( &dest_frame,
					corr->num_exposures,
					corr->dezinger_frame_array,
					fabs(ad->dezinger_threshold) );

#if MX_AREA_DETECTOR_DEBUG_DEZINGER
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement,
			fname, "Total image dezingering time." );
#endif

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( NULL, corr );
			return mx_status;
		}
	} else {
		/* not dezinger */

		/* Copy normalized pixels to the destination array. */

		pixels_per_frame = ad->framesize[0] * ad->framesize[1];

		dest_format = MXIF_IMAGE_FORMAT(corr->destination_frame);

		switch( dest_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
		    u16_dest_array = corr->destination_frame->image_data;

		    for ( i = 0; i < pixels_per_frame; i++ ) {
			temp_double = corr->sum_array[i] / corr->num_exposures;

			u16_dest_array[i] = (uint16_t) mx_round( temp_double );
		    }
		    break;

		case MXT_IMAGE_FORMAT_FLOAT:
		    flt_dest_array = corr->destination_frame->image_data;

		    for ( i = 0; i < pixels_per_frame; i++ ) {
			temp_double = corr->sum_array[i] / corr->num_exposures;

#if 0
			flt_dest_array[i] = (uint16_t) mx_round( temp_double );
#else
			flt_dest_array[i] = (float) temp_double;
#endif
		    }
		    break;

		default:
		    return mx_error( MXE_UNSUPPORTED, fname,
			"Correction image format %lu is not supported "
			"for detector '%s'", dest_format, ad->record->name );
		    break;
		}
	}

	/* If requested, perform a delayed geometrical correction on a
	 * flood field frame after averaging or dezingering the source
	 * frames.
	 */

	if ( ( ad->correction_measurement_type == MXFT_AD_FLOOD_FIELD_FRAME )
	  && ( ad->correction_frame_geom_corr_last == TRUE )
	  && ( ad->correction_frame_no_geom_corr == FALSE ) )
	{
		MX_DEBUG(-2,("%s: Invoking delayed geometrical correction "
				"on measured flood field frame", fname ));

		mx_status = (*geometrical_correction_fn)( ad, dest_frame );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_area_detector_cleanup_after_correction( NULL, corr );
			return mx_status;
		}
	}

	/* Set the image frame exposure time. */

	exposure_timespec =
	    mx_convert_seconds_to_high_resolution_time( corr->exposure_time );

	MXIF_EXPOSURE_TIME_SEC(dest_frame)  = exposure_timespec.tv_sec;
	MXIF_EXPOSURE_TIME_NSEC(dest_frame) = exposure_timespec.tv_nsec;

	/* Set the image frame timestamp to the current time. */

	MXIF_TIMESTAMP_SEC(dest_frame) = time( &time_buffer );
	MXIF_TIMESTAMP_NSEC(dest_frame) = 0;

	/* Return with the satisfaction of a job well done. */

	mx_area_detector_cleanup_after_correction( NULL, corr );

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: correction measurement complete.", fname));
#endif
	
	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

#define FREE_DEZINGER_ARRAYS \
	do {								\
		if ( image_frame_array != NULL ) {			\
			for ( z = 0; z < num_exposures; z++ ) {		\
				mx_image_free( image_frame_array[z] );	\
			}						\
		}							\
	} while(0)

MX_EXPORT mx_status_type
mx_area_detector_default_dezinger_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_dezinger_correction()";

	MX_IMAGE_FRAME *dest_frame;
	MX_IMAGE_FRAME **image_frame_array;
	unsigned long saved_correction_flags, desired_correction_flags;
	long i, n, z, num_exposures;
	double exposure_time;
	unsigned long ad_status;
	mx_status_type mx_status, mx_status2;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));

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

	exposure_time = ad->correction_measurement_time;

	num_exposures = ad->num_correction_measurements;

	if ( num_exposures <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of exposures (%ld) requested for "
		"area detector '%s'.  The minimum number of exposures "
		"allowed is 1.",  num_exposures, ad->record->name );
	}

	/* The correction frames will originally be read into the image frame.
	 * We must make sure that the image frame is big enough to hold
	 * the image data.
	 */

	mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure that the destination image frame is already big enough
	 * to hold the image frame that we are going to put in it.
	 */

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
		mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->dark_current_frame) );

		dest_frame = ad->dark_current_frame;

		desired_correction_flags = 0;
		break;
	
	case MXFT_AD_FLOOD_FIELD_FRAME:
		mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->flood_field_frame) );

		dest_frame = ad->flood_field_frame;

		desired_correction_flags = MXFT_AD_DARK_CURRENT_FRAME;
		break;

	default:
		desired_correction_flags = 0;

		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type, ad->record->name );

		dest_frame = NULL;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate an array of images to store the images to be used
	 * for dezingering.
	 */

	image_frame_array = malloc( num_exposures * sizeof(MX_IMAGE_FRAME *) );

	if ( image_frame_array == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Could not allocate a %lu element array of MX_IMAGE_FRAME pointers.",
			num_exposures );
	}

	for ( i = 0; i < num_exposures; i++ ) {
		mx_status = mx_area_detector_setup_frame( ad->record,
						&(image_frame_array[i]) );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_DEZINGER_ARRAYS;
			return mx_status;
		}
	}

	/* Put the area detector in One-shot mode. */

	mx_status = mx_area_detector_set_one_shot_mode( ad->record,
							exposure_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_DEZINGER_ARRAYS;
		return mx_status;
	}

	/* Take the requested number of exposures. */

	for ( n = 0; n < num_exposures; n++ ) {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Exposure %ld", fname, n));
#endif

		/* Start the exposure. */

		mx_status = mx_area_detector_start( ad->record );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_DEZINGER_ARRAYS;
			return mx_status;
		}

		/* Wait for the exposure to end. */

		for(;;) {
			mx_status = mx_area_detector_get_status( ad->record,
								&ad_status );

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status;
			}

#if 0
			if ( mx_kbhit() ) {
				(void) mx_getch();

				MX_DEBUG(-2,("%s: INTERRUPTED", fname));

				FREE_DEZINGER_ARRAYS;
				return mx_area_detector_stop( ad->record );
			}
#endif

			if ((ad_status & MXSF_AD_ACQUISITION_IN_PROGRESS) == 0)
			{

				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}

		/* Readout the frame into ad->image_frame. */

		mx_status = mx_area_detector_readout_frame( ad->record, 0 );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_DEZINGER_ARRAYS;
			return mx_status;
		}

		/* Perform any necessary image corrections. */

		if ( desired_correction_flags != 0 ) {
			mx_status = mx_area_detector_get_correction_flags(
					ad->record, &saved_correction_flags );

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status;
			}

			mx_status = mx_area_detector_set_correction_flags(
					ad->record, desired_correction_flags );

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status;
			}

			mx_status = mx_area_detector_correct_frame( ad->record);

			mx_status2 = mx_area_detector_set_correction_flags(
					ad->record, saved_correction_flags );

			if ( mx_status2.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status2;
			}

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status;
			}
		}

		/* Transfer the frame to the image frame array. */

		mx_status = mx_area_detector_transfer_frame( ad->record,
						MXFT_AD_IMAGE_FRAME,
						&(image_frame_array[n]) );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_DEZINGER_ARRAYS;
			return mx_status;
		}
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Dezingering images.", fname));
#endif

	mx_status = mx_image_dezinger( &dest_frame,
					num_exposures,
					image_frame_array,
					ad->dezinger_threshold );
	FREE_DEZINGER_ARRAYS;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: correction measurement complete.", fname));
#endif
	
	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_default_geometrical_correction( MX_AREA_DETECTOR *ad,
						MX_IMAGE_FRAME *frame )
{
	static const char fname[] =
		"mx_area_detector_default_geometrical_correction()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Geometrical correction has not yet "
	"been implemented for area detector '%s'.", ad->record->name );
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_check_correction_framesize( MX_AREA_DETECTOR *ad,
						MX_IMAGE_FRAME *image_frame,
						MX_IMAGE_FRAME *test_frame,
						const char *frame_name )
{
	static const char fname[] =
		"mx_area_detector_check_correction_framesize()";

	unsigned long image_row_framesize, image_column_framesize;
	unsigned long test_row_framesize, test_column_framesize;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	/* It is OK for test_frame to be NULL. */

	if ( test_frame == (MX_IMAGE_FRAME *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	image_row_framesize = MXIF_ROW_FRAMESIZE(image_frame);
	image_column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);

	test_row_framesize = MXIF_ROW_FRAMESIZE(test_frame);
	test_column_framesize = MXIF_COLUMN_FRAMESIZE(test_frame);

	if ( (image_row_framesize != test_row_framesize)
	  || (image_column_framesize != test_column_framesize) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The current %s framesize (%lu,%lu) for area detector '%s' is "
		"different from the framesize (%lu,%lu) of the image frame.",
			frame_name, test_row_framesize,
			test_column_framesize, ad->record->name,
			image_row_framesize, image_column_framesize );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_compute_dark_current_offset( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mx_area_detector_compute_dark_current_offset()";

	uint8_t  *dark_current_data8  = NULL;
	uint16_t *dark_current_data16 = NULL;
	uint32_t *dark_current_data32 = NULL;
	uint8_t  *bias_data8  = NULL;
	uint16_t *bias_data16 = NULL;
	uint32_t *bias_data32 = NULL;
	double raw_dark_current = 0.0;
	double bias_offset      = 0.0;
	unsigned long i, num_pixels, image_format;
	double scaled_dark_current, exposure_time_ratio, exposure_time;
	float *dark_current_offset_array;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_TIMING compute_dark_current_timing;
#endif

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	/* If present, discard the old version
	 * of the dark current offset array.
	 */

	if ( ad->dark_current_offset_array != NULL ) {
		mx_free( ad->dark_current_offset_array );
	}

	/* If no dark current frame is currently loaded, then we just
	 * skip over the computation of the dark current offset.
	 */

	if ( dark_current_frame == NULL ) {
		MX_DEBUG(-2,
		("%s: dark_current_frame is NULL.  Skipping...", fname));

		return MX_SUCCESSFUL_RESULT;
	}

	image_format = MXIF_IMAGE_FORMAT(dark_current_frame);

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		dark_current_data8 = dark_current_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		dark_current_data16 = dark_current_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_GREY32:
		dark_current_data32 = dark_current_frame->image_data;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Dark current correction is not supported for image format %lu "
		"used by area detector '%s'.", image_format, ad->record->name );
	}

	if ( bias_frame != NULL ) {
		if ( image_format != MXIF_IMAGE_FORMAT(bias_frame) ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The bias frame image format %lu for area detector '%s'"
			" does not match the dark current image format %lu.",
				(unsigned long) MXIF_IMAGE_FORMAT(bias_frame),
				ad->record->name,
				image_format );
		}

		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			bias_data8 = bias_frame->image_data;
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			bias_data16 = bias_frame->image_data;
			break;
		case MXT_IMAGE_FORMAT_GREY32:
			bias_data32 = bias_frame->image_data;
			break;
		}
	}

	/* Compute the exposure time ratio. */

	if ( ad->use_scaled_dark_current == FALSE ) {
		exposure_time_ratio = 1.0;
	} else {
		/* Get the exposure time for the currently selected sequence. */

		sp = &(ad->sequence_parameters);

		mx_status = mx_sequence_get_exposure_time( sp, 0,
							&exposure_time );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_time_ratio = mx_divide_safely( exposure_time,
					ad->dark_current_exposure_time );
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: exposure_time_ratio = %g",
		fname, exposure_time_ratio));
#endif

	/* Allocate memory for the dark current offset array. */

	num_pixels = MXIF_ROW_FRAMESIZE(dark_current_frame)
			* MXIF_COLUMN_FRAMESIZE(dark_current_frame);
	
	dark_current_offset_array = malloc( num_pixels * sizeof(float) );

	if ( dark_current_offset_array == (float *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu-element array "
		"of dark current offset values for area detector '%s'.",
			num_pixels, ad->record->name );
	}

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_START(compute_dark_current_timing);
#endif

	for ( i = 0; i < num_pixels; i++ ) {

		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			raw_dark_current = dark_current_data8[i];
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			raw_dark_current = dark_current_data16[i];
			break;
		case MXT_IMAGE_FORMAT_GREY32:
			raw_dark_current = dark_current_data32[i];
			break;
		}

		if ( ad->bias_corr_after_flood ) {
			bias_offset = 0;
		} else
		if ( bias_frame == NULL ) {
			bias_offset = 0;
		} else {
			switch( image_format ) {
			case MXT_IMAGE_FORMAT_GREY8:
				bias_offset = bias_data8[i];
				break;
			case MXT_IMAGE_FORMAT_GREY16:
				bias_offset = bias_data16[i];
				break;
			case MXT_IMAGE_FORMAT_GREY32:
				bias_offset = bias_data32[i];
				break;
			}
		}

		scaled_dark_current = exposure_time_ratio
			* ( raw_dark_current - bias_offset ) + bias_offset;

		dark_current_offset_array[i] =
			bias_offset - scaled_dark_current;
			
#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr,
	    "i = %lu, scaled_dark_current = %g = %g * ( %g - %g ) + %g, ",
		    		i, scaled_dark_current,
				exposure_time_ratio, raw_dark_current,
				bias_offset, bias_offset );
			fprintf( stderr,
			"dark_current_offset_array[%lu] = %g\n",
				i, dark_current_offset_array[i] );
		}
#endif
	}

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_END(compute_dark_current_timing);
	MX_HRT_RESULTS(compute_dark_current_timing, fname,
			"for computing new dark current offset array.");
#endif

	/* Save a pointer to the new dark current offset array. */

	ad->dark_current_offset_array = dark_current_offset_array;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	fprintf(stderr, "ad->dark_current_offset_array = ");

	for ( i = 0; i < 10; i++ ) {
		fprintf(stderr, "%g ", ad->dark_current_offset_array[i]);
	}
	fprintf(stderr, "\n");
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_compute_flood_field_scale( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] =
		"mx_area_detector_compute_flood_field_scale()";

	uint8_t  *flood_field_data_u8  = NULL;
	uint16_t *flood_field_data_u16 = NULL;
	uint32_t *flood_field_data_u32 = NULL;
	float    *flood_field_data_flt = NULL;
	double   *flood_field_data_dbl = NULL;
	uint8_t  *bias_data_u8  = NULL;
	uint16_t *bias_data_u16 = NULL;
	uint32_t *bias_data_u32 = NULL;
	double raw_flood_field = 0.0;
	double bias_offset     = 0.0;
	unsigned long i, num_pixels, flood_image_format, bias_image_format;
	double ffs_numerator, ffs_denominator, bias_average;
	float *flood_field_scale_array;
	mx_bool_type all_pixels_underflowed;

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_TIMING compute_flood_field_timing;
#endif

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	/* If present, discard the old version
	 * of the flood field scale array.
	 */

	if ( ad->flood_field_scale_array != NULL ) {
		mx_free( ad->flood_field_scale_array );
	}

	/* If no flood field frame is currently loaded, then we just
	 * skip over the computation of the flood field scale.
	 */

	if ( flood_field_frame == NULL ) {
		MX_DEBUG(-2,
		("%s: flood_field_frame is NULL.  Skipping...", fname));

		return MX_SUCCESSFUL_RESULT;
	}

	flood_image_format = MXIF_IMAGE_FORMAT(flood_field_frame);

	switch( flood_image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		flood_field_data_u8 = flood_field_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		flood_field_data_u16 = flood_field_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_GREY32:
		flood_field_data_u32 = flood_field_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_FLOAT:
		flood_field_data_flt = flood_field_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_DOUBLE:
		flood_field_data_dbl = flood_field_frame->image_data;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Flood field correction is not supported for image format %lu "
		"used by area detector '%s'.",
			flood_image_format, ad->record->name );
	}

	bias_image_format = 0;

	if ( bias_frame == NULL ) {
		all_pixels_underflowed = FALSE;
	} else {
		all_pixels_underflowed = TRUE;

		bias_image_format = MXIF_IMAGE_FORMAT(bias_frame);

		switch( bias_image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			bias_data_u8 = bias_frame->image_data;
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			bias_data_u16 = bias_frame->image_data;
			break;
		case MXT_IMAGE_FORMAT_GREY32:
			bias_data_u32 = bias_frame->image_data;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Bias offset correction is not supported for "
			"image format %lu used by area detector '%s'.",
				bias_image_format, ad->record->name );
		}
	}

	num_pixels = MXIF_ROW_FRAMESIZE(flood_field_frame)
			* MXIF_COLUMN_FRAMESIZE(flood_field_frame);
	
	flood_field_scale_array = malloc( num_pixels * sizeof(float) );

	if ( flood_field_scale_array == (float *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu-element array "
		"of flood field scale factors for area detector '%s'.",
			num_pixels, ad->record->name );
	}

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_START(compute_flood_field_timing);
#endif

	for ( i = 0; i < num_pixels; i++ ) {

		switch( flood_image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			raw_flood_field = flood_field_data_u8[i];
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			raw_flood_field = flood_field_data_u16[i];
			break;
		case MXT_IMAGE_FORMAT_GREY32:
			raw_flood_field = flood_field_data_u32[i];
			break;
		case MXT_IMAGE_FORMAT_FLOAT:
			raw_flood_field = flood_field_data_flt[i];
			break;
		case MXT_IMAGE_FORMAT_DOUBLE:
			raw_flood_field = flood_field_data_dbl[i];
			break;
		}

		if ( bias_frame == NULL ) {
			bias_offset = 0;
			bias_average = 0;
		} else {
			switch( bias_image_format ) {
			case MXT_IMAGE_FORMAT_GREY8:
				bias_offset = bias_data_u8[i];
				break;
			case MXT_IMAGE_FORMAT_GREY16:
				bias_offset = bias_data_u16[i];
				break;
			case MXT_IMAGE_FORMAT_GREY32:
				bias_offset = bias_data_u32[i];
				break;
			}

			bias_average = ad->bias_average_intensity;
		}

		/* We _must_ _not_ use mx_divide_safely() here since
		 * function calls have too high of an overhead to
		 * be used in this loop.
		 */

		if ( raw_flood_field <= bias_offset ) {
			flood_field_scale_array[i] = 0.0;
			continue;	/* Skip to the next pixel. */
		} else {
			all_pixels_underflowed = FALSE;
		}

		ffs_denominator = raw_flood_field - bias_offset;

		ffs_numerator = ad->flood_field_average_intensity
					- bias_average;

		flood_field_scale_array[i]
				= ffs_numerator / ffs_denominator;

		if ( flood_field_scale_array[i] > ad->flood_field_scale_max ){
			flood_field_scale_array[i] = 0.0;
		} else
		if ( flood_field_scale_array[i] < ad->flood_field_scale_min ){
			flood_field_scale_array[i] = 0.0;
		}
	}

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_END(compute_flood_field_timing);
	MX_HRT_RESULTS(compute_flood_field_timing, fname,
			"for computing new flood field scale array.");
#endif

	if ( all_pixels_underflowed ) {
		mx_free( flood_field_scale_array );

		return mx_error(MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"Every pixel in the flood field frame '%s' for "
		"area detector '%s' is smaller than the value of the "
		"corresponding pixel in the bias frame '%s'.",
			ad->flood_field_filename,
			ad->record->name,
			ad->bias_filename );
	}

	/* Save a pointer to the new flood field scale array. */

	ad->flood_field_scale_array = flood_field_scale_array;

#if 0
	fprintf(stderr, "ad->flood_field_scale_array = ");

	for ( i = 0; i < 100; i++ ) {
		if ( (i % 10) == 0 ) {
			fprintf(stderr, "\n");
		}

		fprintf(stderr, "%g ", ad->flood_field_scale_array[i]);
	}
	fprintf(stderr, "...\n");
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_check_for_low_memory( MX_AREA_DETECTOR *ad,
				mx_bool_type *memory_is_low )
{
#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	static const char fname[] =
		"mx_area_detector_check_for_low_memory()";
#endif

	MX_SYSTEM_MEMINFO system_meminfo;
	MX_PROCESS_MEMINFO process_meminfo;
	long row_framesize, column_framesize;
	mx_memsize_type bytes_per_float_array;
	mx_memsize_type process_memory_needed, extra_memory_needed;
	double process_to_system_ratio;
	mx_status_type mx_status;

	mx_status = mx_get_system_meminfo( &system_meminfo );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	mx_info("--------------------------------");
	mx_display_system_meminfo( &system_meminfo );
	mx_info("--------------------------------");
#endif

	mx_status = mx_get_process_meminfo( MXF_PROCESS_ID_SELF,
						&process_meminfo );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	mx_display_process_meminfo( &process_meminfo );
	mx_info("--------------------------------");
#endif

	/* If requested in the MX database file, we force the use
	 * of a particular method here.
	 */

	if ( ad->initial_correction_flags & MXFT_AD_USE_LOW_MEMORY_METHODS ) {

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
		MX_DEBUG(-2,
		("%s: forced *memory_is_low to TRUE", fname));
#endif
		*memory_is_low = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( ad->initial_correction_flags & MXFT_AD_USE_HIGH_MEMORY_METHODS ) {

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
		MX_DEBUG(-2,
		("%s: forced *memory_is_low to FALSE", fname));
#endif
		*memory_is_low = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* See if we will need to have more memory. */

	mx_status = mx_area_detector_get_framesize( ad->record,
					&row_framesize, &column_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bytes_per_float_array =
		row_framesize * column_framesize * sizeof(float);

	extra_memory_needed = 0;

	if ( ad->dark_current_offset_array == NULL ) {
		extra_memory_needed += bytes_per_float_array;
	}
	if ( ad->flood_field_scale_array == NULL ) {
		extra_memory_needed += bytes_per_float_array;
	}

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	MX_DEBUG(-2,("%s: extra_memory_needed = %lu",
		fname, extra_memory_needed));
#endif

	process_memory_needed = process_meminfo.total_bytes
					+ extra_memory_needed;

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	MX_DEBUG(-2,("%s: process_memory_needed = %lu, system_memory = %lu",
		fname, process_memory_needed, system_meminfo.total_bytes ));
#endif

	process_to_system_ratio = mx_divide_safely( process_memory_needed,
					system_meminfo.total_bytes );

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	MX_DEBUG(-2,("%s: process_to_system_ratio = %g",
		fname, process_to_system_ratio));
#endif

	/*---*/

	if ( process_to_system_ratio < 0.8 ) {
		*memory_is_low = FALSE;
	} else {
		*memory_is_low = TRUE;
	}

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	MX_DEBUG(-2,("%s: *memory_is_low = %d",
		fname, (int) *memory_is_low));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

/* mx_area_detector_u16_precomp_dark_correction() is for use when enough
 * free memory is available that page swapping will not be required.
 */

MX_EXPORT mx_status_type
mx_area_detector_u16_precomp_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mx_area_detector_u16_precomp_dark_correction()";

	unsigned long i, num_pixels;
	double image_pixel, image_exposure_time;
	float *dark_current_offset_array;
	uint16_t *mask_data_array;
	uint16_t *u16_image_data_array;
	long image_format;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	u16_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Discard the old dark current offset array if the exposure time
	 * has changed significantly.
	 */

	mx_status = mx_image_get_exposure_time( image_frame,
						&image_exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,
	("%s: image_exposure_time = %g", fname, image_exposure_time));
#endif

	if ( mx_difference( image_exposure_time,
				ad->old_exposure_time ) > 0.001 )
	{
		mx_free( ad->dark_current_offset_array );
	}

	ad->old_exposure_time = image_exposure_time;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	/*---*/

	/* Get the dark current offset array, creating a new one if necessary.*/

	if ( dark_current_frame == NULL ) {
		dark_current_offset_array = NULL;
	} else {
		if ( ad->dark_current_offset_array == NULL ) {
		    mx_status = mx_area_detector_compute_dark_current_offset(
					ad, bias_frame, dark_current_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		dark_current_offset_array = ad->dark_current_offset_array;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p", fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_offset_array = %p",
			fname, dark_current_offset_array));
#endif
	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				u16_image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"u16_image_data_array[%lu] = %ld\n",
					i, (long) u16_image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* If requested, apply the dark current correction. */

		if ( dark_current_offset_array != NULL ) {

			/* Get the raw image pixel. */

			image_pixel = (double) u16_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "BEFORE image_pixel = %g, ",
						image_pixel );
				fprintf( stderr,
				"dark_current_offset_array[%lu] = %g, ",
					i, dark_current_offset_array[i] );
			}
#endif
			image_pixel = image_pixel
					+ dark_current_offset_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "AFTER image_pixel = %g, ",
						image_pixel );
			}
#endif

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			if ( image_pixel < 0.0 ) {
				u16_image_data_array[i] = 0;
			} else {
				u16_image_data_array[i] = image_pixel + 0.5;
			}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"Final u16_image_data_array[%lu] = %ld, ",
					i, (long) u16_image_data_array[i]);
			}
#endif
		}
#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "\n" );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mx_area_detector_s32_precomp_dark_correction() is for use when enough
 * free memory is available that page swapping will not be required.
 */

MX_EXPORT mx_status_type
mx_area_detector_s32_precomp_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mx_area_detector_s32_precomp_dark_correction()";

	unsigned long i, num_pixels;
	double image_pixel, image_exposure_time;
	float *dark_current_offset_array;
	uint16_t *mask_data_array;
	int32_t *s32_image_data_array;
	long image_format;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_INT32 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	s32_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Discard the old dark current offset array if the exposure time
	 * has changed significantly.
	 */

	mx_status = mx_image_get_exposure_time( image_frame,
						&image_exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,
	("%s: image_exposure_time = %g", fname, image_exposure_time));
#endif

	if ( mx_difference( image_exposure_time,
				ad->old_exposure_time ) > 0.001 )
	{
		mx_free( ad->dark_current_offset_array );
	}

	ad->old_exposure_time = image_exposure_time;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	/*---*/

	/* Get the dark current offset array, creating a new one if necessary.*/

	if ( dark_current_frame == NULL ) {
		dark_current_offset_array = NULL;
	} else {
		if ( ad->dark_current_offset_array == NULL ) {
		    mx_status = mx_area_detector_compute_dark_current_offset(
					ad, bias_frame, dark_current_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		dark_current_offset_array = ad->dark_current_offset_array;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p", fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_offset_array = %p",
			fname, dark_current_offset_array));
#endif
	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				s32_image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"s32_image_data_array[%lu] = %ld\n",
					i, (long) s32_image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* If requested, apply the dark current correction. */

		if ( dark_current_offset_array != NULL ) {

			/* Get the raw image pixel. */

			image_pixel = (double) s32_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "BEFORE image_pixel = %g, ",
						image_pixel );
				fprintf( stderr,
				"dark_current_offset_array[%lu] = %g, ",
					i, dark_current_offset_array[i] );
			}
#endif
			image_pixel = image_pixel
					+ dark_current_offset_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "AFTER image_pixel = %g, ",
						image_pixel );
			}
#endif

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			if ( image_pixel < 0.0 ) {
				s32_image_data_array[i] = image_pixel - 0.5;
			} else {
				s32_image_data_array[i] = image_pixel + 0.5;
			}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"Final image_data_array[%lu] = %ld, ",
					i, (long) s32_image_data_array[i]);
			}
#endif
		}
#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "\n" );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mx_area_detector_flt_precomp_dark_correction() is for use when enough
 * free memory is available that page swapping will not be required.
 */

MX_EXPORT mx_status_type
mx_area_detector_flt_precomp_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mx_area_detector_flt_precomp_dark_correction()";

	unsigned long i, num_pixels;
	float image_pixel;
	double image_exposure_time;
	float *dark_current_offset_array;
	uint16_t *mask_data_array;
	float *flt_image_data_array;
	long image_format;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_FLOAT ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	flt_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Discard the old dark current offset array if the exposure time
	 * has changed significantly.
	 */

	mx_status = mx_image_get_exposure_time( image_frame,
						&image_exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,
	("%s: image_exposure_time = %g", fname, image_exposure_time));
#endif

	if ( mx_difference( image_exposure_time,
				ad->old_exposure_time ) > 0.001 )
	{
		mx_free( ad->dark_current_offset_array );
	}

	ad->old_exposure_time = image_exposure_time;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	/*---*/

	/* Get the dark current offset array, creating a new one if necessary.*/

	if ( dark_current_frame == NULL ) {
		dark_current_offset_array = NULL;
	} else {
		if ( ad->dark_current_offset_array == NULL ) {
		    mx_status = mx_area_detector_compute_dark_current_offset(
					ad, bias_frame, dark_current_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		dark_current_offset_array = ad->dark_current_offset_array;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p", fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_offset_array = %p",
			fname, dark_current_offset_array));
#endif
	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				flt_image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"flt_image_data_array[%lu] = %f\n",
					i, flt_image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* If requested, apply the dark current correction. */

		if ( dark_current_offset_array != NULL ) {

			/* Get the raw image pixel. */

			image_pixel = flt_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "BEFORE image_pixel = %g, ",
						image_pixel );
				fprintf( stderr,
				"dark_current_offset_array[%lu] = %g, ",
					i, dark_current_offset_array[i] );
			}
#endif
			image_pixel = image_pixel
					+ dark_current_offset_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "AFTER image_pixel = %g, ",
						image_pixel );
			}
#endif

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			flt_image_data_array[i] = image_pixel;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"Final flt_image_data_array[%lu] = %f, ",
					i, flt_image_data_array[i]);
			}
#endif
		}
#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "\n" );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mx_area_detector_dbl_precomp_dark_correction() is for use when enough
 * free memory is available that page swapping will not be required.
 */

MX_EXPORT mx_status_type
mx_area_detector_dbl_precomp_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mx_area_detector_dbl_precomp_dark_correction()";

	unsigned long i, num_pixels;
	double image_pixel, image_exposure_time;
	float *dark_current_offset_array;
	uint16_t *mask_data_array;
	double *dbl_image_data_array;
	long image_format;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_DOUBLE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	dbl_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Discard the old dark current offset array if the exposure time
	 * has changed significantly.
	 */

	mx_status = mx_image_get_exposure_time( image_frame,
						&image_exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,
	("%s: image_exposure_time = %g", fname, image_exposure_time));
#endif

	if ( mx_difference( image_exposure_time,
				ad->old_exposure_time ) > 0.001 )
	{
		mx_free( ad->dark_current_offset_array );
	}

	ad->old_exposure_time = image_exposure_time;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	/*---*/

	/* Get the dark current offset array, creating a new one if necessary.*/

	if ( dark_current_frame == NULL ) {
		dark_current_offset_array = NULL;
	} else {
		if ( ad->dark_current_offset_array == NULL ) {
		    mx_status = mx_area_detector_compute_dark_current_offset(
					ad, bias_frame, dark_current_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		dark_current_offset_array = ad->dark_current_offset_array;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p", fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_offset_array = %p",
			fname, dark_current_offset_array));
#endif
	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				dbl_image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"dbl_image_data_array[%lu] = %f\n",
					i, dbl_image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* If requested, apply the dark current correction. */

		if ( dark_current_offset_array != NULL ) {

			/* Get the raw image pixel. */

			image_pixel = (double) dbl_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "BEFORE image_pixel = %g, ",
						image_pixel );
				fprintf( stderr,
				"dark_current_offset_array[%lu] = %g, ",
					i, dark_current_offset_array[i] );
			}
#endif
			image_pixel = image_pixel
					+ dark_current_offset_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "AFTER image_pixel = %g, ",
						image_pixel );
			}
#endif

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			dbl_image_data_array[i] = image_pixel;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"Final dbl_image_data_array[%lu] = %f, ",
					i, dbl_image_data_array[i]);
			}
#endif
		}
#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "\n" );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

/* mx_area_detector_u16_plain_dark_correction() is for use to avoid
 * swapping when the amount of free memory available is small.
 */

MX_EXPORT mx_status_type
mx_area_detector_u16_plain_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mxp_area_detector_u16_plain_dark_correction()";

	unsigned long i, num_pixels;
	uint16_t *mask_data_array, *bias_data_array, *dark_current_data_array;
	uint16_t *u16_image_data_array;
	long image_format;
	double image_exposure_time, dark_current_exposure_time;
	double exposure_time_ratio;
	double image_pixel, raw_dark_current, scaled_dark_current;
	unsigned long bias_offset;
	mx_bool_type use_scaled_dark_current;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	u16_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Discard the dark current offset array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->dark_current_offset_array );

	/* Get pointers to the mask, bias, and dark current image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: bias_data_array = %p", fname, bias_data_array));
#endif

	if ( dark_current_frame == NULL ) {
		dark_current_data_array = NULL;
	} else {
		dark_current_data_array = dark_current_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p",
			fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_data_array = %p",
			fname, dark_current_data_array));
#endif

	/* Compute the exposure time ratio. */

	if ( ad->use_scaled_dark_current == FALSE ) {
		use_scaled_dark_current = FALSE;
	} else
	if ( dark_current_frame == NULL ) {
		use_scaled_dark_current = FALSE;
	} else {
		use_scaled_dark_current = TRUE;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: use_scaled_dark_current = %d",
		fname, (int) use_scaled_dark_current));
#endif

	if ( use_scaled_dark_current == FALSE ) {
		exposure_time_ratio = 1.0;
	} else {
		mx_status = mx_image_get_exposure_time( image_frame,
							&image_exposure_time );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_exposure_time( dark_current_frame,
						&dark_current_exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_time_ratio = mx_divide_safely( image_exposure_time,
						dark_current_exposure_time );
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: exposure_time_ratio = %g",
		fname, exposure_time_ratio));
#endif

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	/* Do the mask, bias, and dark current corrections. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				u16_image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"u16_image_data_array[%lu] = %d\n",
					i, (int) u16_image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* Get the bias offset for this pixel. */

		if ( ad->bias_corr_after_flood ) {
			bias_offset = 0;
		} else
		if ( bias_data_array == NULL ) {
			bias_offset = 0;
		} else {
			bias_offset = bias_data_array[i];
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "bias_offset = %d, ",
					(int) bias_offset );
		}
#endif
		/* Get the raw image pixel. */

		image_pixel = (double) u16_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "BEFORE image_pixel = %g, ",
				image_pixel );
		}
#endif

		/* If requested, apply the dark current correction. */

		if ( dark_current_data_array != NULL ) {
			raw_dark_current = (double) dark_current_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "raw_dark_current = %g, ",
					raw_dark_current );
			}
#endif
			scaled_dark_current = exposure_time_ratio
			    * (raw_dark_current - bias_offset) + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "scaled_dark_current = %g, ",
					scaled_dark_current );
			}
#endif
			image_pixel = image_pixel + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"AFTER adding bias, image_pixel = %g, ",
					image_pixel );
			}
#endif
			image_pixel = image_pixel - scaled_dark_current;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
			"AFTER subtracting dark current, image_pixel = %g, ",
					image_pixel );
			}
#endif
		}

		/* Round to the nearest integer by adding 0.5 and
		 * then truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function
		 * calls have too high of an overhead to be used
		 * in this loop.
		 */

		if ( image_pixel < 0.0 ) {
			u16_image_data_array[i] = 0;
		} else {
			u16_image_data_array[i] = image_pixel + 0.5;
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "u16_image_data_array[%lu] = %ld\n",
				i, (long) u16_image_data_array[i] );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mx_area_detector_s32_plain_dark_correction() is for use to avoid
 * swapping when the amount of free memory available is small.
 */

MX_EXPORT mx_status_type
mx_area_detector_s32_plain_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mx_area_detector_s32_plain_dark_correction()";

	unsigned long i, num_pixels;
	uint16_t *mask_data_array, *bias_data_array, *dark_current_data_array;
	int32_t *s32_image_data_array;
	long image_format;
	double image_exposure_time, dark_current_exposure_time;
	double exposure_time_ratio;
	double image_pixel, raw_dark_current, scaled_dark_current;
	unsigned long bias_offset;
	mx_bool_type use_scaled_dark_current;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_INT32 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	s32_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Discard the dark current offset array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->dark_current_offset_array );

	/* Get pointers to the mask, bias, and dark current image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: bias_data_array = %p", fname, bias_data_array));
#endif

	if ( dark_current_frame == NULL ) {
		dark_current_data_array = NULL;
	} else {
		dark_current_data_array = dark_current_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p",
			fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_data_array = %p",
			fname, dark_current_data_array));
#endif

	/* Compute the exposure time ratio. */

	if ( ad->use_scaled_dark_current == FALSE ) {
		use_scaled_dark_current = FALSE;
	} else
	if ( dark_current_frame == NULL ) {
		use_scaled_dark_current = FALSE;
	} else {
		use_scaled_dark_current = TRUE;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: use_scaled_dark_current = %d",
		fname, (int) use_scaled_dark_current));
#endif

	if ( use_scaled_dark_current == FALSE ) {
		exposure_time_ratio = 1.0;
	} else {
		mx_status = mx_image_get_exposure_time( image_frame,
							&image_exposure_time );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_exposure_time( dark_current_frame,
						&dark_current_exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_time_ratio = mx_divide_safely( image_exposure_time,
						dark_current_exposure_time );
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: exposure_time_ratio = %g",
		fname, exposure_time_ratio));
#endif

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	/* Do the mask, bias, and dark current corrections. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				s32_image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"s32_image_data_array[%lu] = %ld\n",
					i, (long) s32_image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* Get the bias offset for this pixel. */

		if ( ad->bias_corr_after_flood ) {
			bias_offset = 0;
		} else
		if ( bias_data_array == NULL ) {
			bias_offset = 0;
		} else {
			bias_offset = bias_data_array[i];
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "bias_offset = %d, ",
					(int) bias_offset );
		}
#endif
		/* Get the raw image pixel. */

		image_pixel = (double) s32_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "BEFORE image_pixel = %g, ",
				image_pixel );
		}
#endif

		/* If requested, apply the dark current correction. */

		if ( dark_current_data_array != NULL ) {
			raw_dark_current = (double) dark_current_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "raw_dark_current = %g, ",
					raw_dark_current );
			}
#endif
			scaled_dark_current = exposure_time_ratio
			    * (raw_dark_current - bias_offset) + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "scaled_dark_current = %g, ",
					scaled_dark_current );
			}
#endif
			image_pixel = image_pixel + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"AFTER adding bias, image_pixel = %g, ",
					image_pixel );
			}
#endif
			image_pixel = image_pixel - scaled_dark_current;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
			"AFTER subtracting dark current, image_pixel = %g, ",
					image_pixel );
			}
#endif
		}

		/* Round to the nearest integer by adding 0.5 and
		 * then truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function
		 * calls have too high of an overhead to be used
		 * in this loop.
		 */

		if ( image_pixel < 0.0 ) {
			s32_image_data_array[i] = image_pixel - 0.5;
		} else {
			s32_image_data_array[i] = image_pixel + 0.5;
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "image_data_array[%lu] = %ld\n",
				i, (long) s32_image_data_array[i] );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mx_area_detector_flt_plain_dark_correction() is for use to avoid
 * swapping when the amount of free memory available is small.
 */

MX_EXPORT mx_status_type
mx_area_detector_flt_plain_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mx_area_detector_flt_plain_dark_correction()";

	unsigned long i, num_pixels;
	uint16_t *mask_data_array, *bias_data_array, *dark_current_data_array;
	float *flt_image_data_array;
	long image_format;
	double image_exposure_time, dark_current_exposure_time;
	double exposure_time_ratio;
	float image_pixel;
	double raw_dark_current, scaled_dark_current;
	unsigned long bias_offset;
	mx_bool_type use_scaled_dark_current;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_FLOAT ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	flt_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Discard the dark current offset array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->dark_current_offset_array );

	/* Get pointers to the mask, bias, and dark current image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: bias_data_array = %p", fname, bias_data_array));
#endif

	if ( dark_current_frame == NULL ) {
		dark_current_data_array = NULL;
	} else {
		dark_current_data_array = dark_current_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p",
			fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_data_array = %p",
			fname, dark_current_data_array));
#endif

	/* Compute the exposure time ratio. */

	if ( ad->use_scaled_dark_current == FALSE ) {
		use_scaled_dark_current = FALSE;
	} else
	if ( dark_current_frame == NULL ) {
		use_scaled_dark_current = FALSE;
	} else {
		use_scaled_dark_current = TRUE;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: use_scaled_dark_current = %d",
		fname, (int) use_scaled_dark_current));
#endif

	if ( use_scaled_dark_current == FALSE ) {
		exposure_time_ratio = 1.0;
	} else {
		mx_status = mx_image_get_exposure_time( image_frame,
							&image_exposure_time );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_exposure_time( dark_current_frame,
						&dark_current_exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_time_ratio = mx_divide_safely( image_exposure_time,
						dark_current_exposure_time );
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: exposure_time_ratio = %g",
		fname, exposure_time_ratio));
#endif

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	/* Do the mask, bias, and dark current corrections. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				flt_image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"flt_image_data_array[%lu] = %f\n",
					i, flt_image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* Get the bias offset for this pixel. */

		if ( ad->bias_corr_after_flood ) {
			bias_offset = 0;
		} else
		if ( bias_data_array == NULL ) {
			bias_offset = 0;
		} else {
			bias_offset = bias_data_array[i];
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "bias_offset = %d, ",
					(int) bias_offset );
		}
#endif
		/* Get the raw image pixel. */

		image_pixel = flt_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "BEFORE image_pixel = %g, ",
				image_pixel );
		}
#endif

		/* If requested, apply the dark current correction. */

		if ( dark_current_data_array != NULL ) {
			raw_dark_current = (double) dark_current_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "raw_dark_current = %g, ",
					raw_dark_current );
			}
#endif
			scaled_dark_current = exposure_time_ratio
			    * (raw_dark_current - bias_offset) + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "scaled_dark_current = %g, ",
					scaled_dark_current );
			}
#endif
			image_pixel = image_pixel + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"AFTER adding bias, image_pixel = %g, ",
					image_pixel );
			}
#endif
			image_pixel = image_pixel - scaled_dark_current;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
			"AFTER subtracting dark current, image_pixel = %g, ",
					image_pixel );
			}
#endif
		}

		/* Round to the nearest integer by adding 0.5 and
		 * then truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function
		 * calls have too high of an overhead to be used
		 * in this loop.
		 */

		flt_image_data_array[i] = image_pixel;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "image_data_array[%lu] = %ld\n",
				i, (long) flt_image_data_array[i] );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mx_area_detector_dbl_plain_dark_correction() is for use to avoid
 * swapping when the amount of free memory available is small.
 */

MX_EXPORT mx_status_type
mx_area_detector_dbl_plain_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mx_area_detector_dbl_plain_dark_correction()";

	unsigned long i, num_pixels;
	uint16_t *mask_data_array, *bias_data_array, *dark_current_data_array;
	double *dbl_image_data_array;
	long image_format;
	double image_exposure_time, dark_current_exposure_time;
	double exposure_time_ratio;
	double image_pixel, raw_dark_current, scaled_dark_current;
	unsigned long bias_offset;
	mx_bool_type use_scaled_dark_current;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_DOUBLE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	dbl_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Discard the dark current offset array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->dark_current_offset_array );

	/* Get pointers to the mask, bias, and dark current image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: bias_data_array = %p", fname, bias_data_array));
#endif

	if ( dark_current_frame == NULL ) {
		dark_current_data_array = NULL;
	} else {
		dark_current_data_array = dark_current_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p",
			fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_data_array = %p",
			fname, dark_current_data_array));
#endif

	/* Compute the exposure time ratio. */

	if ( ad->use_scaled_dark_current == FALSE ) {
		use_scaled_dark_current = FALSE;
	} else
	if ( dark_current_frame == NULL ) {
		use_scaled_dark_current = FALSE;
	} else {
		use_scaled_dark_current = TRUE;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: use_scaled_dark_current = %d",
		fname, (int) use_scaled_dark_current));
#endif

	if ( use_scaled_dark_current == FALSE ) {
		exposure_time_ratio = 1.0;
	} else {
		mx_status = mx_image_get_exposure_time( image_frame,
							&image_exposure_time );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_exposure_time( dark_current_frame,
						&dark_current_exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_time_ratio = mx_divide_safely( image_exposure_time,
						dark_current_exposure_time );
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: exposure_time_ratio = %g",
		fname, exposure_time_ratio));
#endif

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	/* Do the mask, bias, and dark current corrections. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				dbl_image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"dbl_image_data_array[%lu] = %f\n",
					i, dbl_image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* Get the bias offset for this pixel. */

		if ( ad->bias_corr_after_flood ) {
			bias_offset = 0;
		} else
		if ( bias_data_array == NULL ) {
			bias_offset = 0;
		} else {
			bias_offset = bias_data_array[i];
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "bias_offset = %d, ",
					(int) bias_offset );
		}
#endif
		/* Get the raw image pixel. */

		image_pixel = dbl_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "BEFORE image_pixel = %g, ",
				image_pixel );
		}
#endif

		/* If requested, apply the dark current correction. */

		if ( dark_current_data_array != NULL ) {
			raw_dark_current = (double) dark_current_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "raw_dark_current = %g, ",
					raw_dark_current );
			}
#endif
			scaled_dark_current = exposure_time_ratio
			    * (raw_dark_current - bias_offset) + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "scaled_dark_current = %g, ",
					scaled_dark_current );
			}
#endif
			image_pixel = image_pixel + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"AFTER adding bias, image_pixel = %g, ",
					image_pixel );
			}
#endif
			image_pixel = image_pixel - scaled_dark_current;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
			"AFTER subtracting dark current, image_pixel = %g, ",
					image_pixel );
			}
#endif
		}

		/* Round to the nearest integer by adding 0.5 and
		 * then truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function
		 * calls have too high of an overhead to be used
		 * in this loop.
		 */

		dbl_image_data_array[i] = image_pixel;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "image_data_array[%lu] = %ld\n",
				i, (long) dbl_image_data_array[i] );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_u16_precomp_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] =
		"mx_area_detector_u16_precomp_flood_field()";

	unsigned long i, num_pixels;
	double image_pixel, bias_offset;
	float *flood_field_scale_array;
	uint16_t *mask_data_array, *bias_data_array;
	uint16_t *u16_image_data_array;
	long image_format;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	u16_image_data_array = image_frame->image_data;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	/*---*/

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	/*---*/

	/* Get the flood field scale array, creating a new one if necessary. */

	if ( flood_field_frame == NULL ) {
		flood_field_scale_array = NULL;
	} else {
		if ( ad->flood_field_scale_array == NULL ) {
		    mx_status = mx_area_detector_compute_flood_field_scale(
					ad, bias_frame, flood_field_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		flood_field_scale_array = ad->flood_field_scale_array;
	}

	/* If requested, do the flood field correction. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	if ( flood_field_scale_array != NULL ) {

		num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
				* MXIF_COLUMN_FRAMESIZE(image_frame);

		for ( i = 0; i < num_pixels; i++ ) {

			if ( mask_data_array != NULL ) {
				if ( mask_data_array[i] == 0 ) {

					/* If the pixel is masked off, skip
					 * this pixel and return to the top
					 * of the loop for the next pixel.
					 */

					continue;
				}
			}

			if ( ad->bias_corr_after_flood ) {
				bias_offset = 0;
			} else
			if ( bias_data_array == NULL ) {
				bias_offset = 0;
			} else {
				bias_offset = bias_data_array[i];
			}

			image_pixel = (double) u16_image_data_array[i];

			image_pixel = image_pixel - bias_offset;

			image_pixel = image_pixel * flood_field_scale_array[i];

			image_pixel = image_pixel + bias_offset;

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			if ( image_pixel < 0.0 ) {
				u16_image_data_array[i] = 0;
			} else {
				u16_image_data_array[i] = image_pixel + 0.5;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_s32_precomp_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] =
		"mx_area_detector_s32_precomp_flood_field()";

	unsigned long i, num_pixels;
	double image_pixel, bias_offset;
	float *flood_field_scale_array;
	uint16_t *mask_data_array, *bias_data_array;
	int32_t *s32_image_data_array;
	long image_format;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_INT32 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	s32_image_data_array = image_frame->image_data;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	/*---*/

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	/*---*/

	/* Get the flood field scale array, creating a new one if necessary. */

	if ( flood_field_frame == NULL ) {
		flood_field_scale_array = NULL;
	} else {
		if ( ad->flood_field_scale_array == NULL ) {
		    mx_status = mx_area_detector_compute_flood_field_scale(
					ad, bias_frame, flood_field_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		flood_field_scale_array = ad->flood_field_scale_array;
	}

	/* If requested, do the flood field correction. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	if ( flood_field_scale_array != NULL ) {

		num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
				* MXIF_COLUMN_FRAMESIZE(image_frame);

		for ( i = 0; i < num_pixels; i++ ) {

			if ( mask_data_array != NULL ) {
				if ( mask_data_array[i] == 0 ) {

					/* If the pixel is masked off, skip
					 * this pixel and return to the top
					 * of the loop for the next pixel.
					 */

					continue;
				}
			}

			if ( ad->bias_corr_after_flood ) {
				bias_offset = 0;
			} else
			if ( bias_data_array == NULL ) {
				bias_offset = 0;
			} else {
				bias_offset = bias_data_array[i];
			}

			image_pixel = (double) s32_image_data_array[i];

			image_pixel = image_pixel - bias_offset;

			image_pixel = image_pixel * flood_field_scale_array[i];

			image_pixel = image_pixel + bias_offset;

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			if ( image_pixel < 0.0 ) {
				s32_image_data_array[i] = image_pixel - 0.5;
			} else {
				s32_image_data_array[i] = image_pixel + 0.5;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_flt_precomp_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] =
		"mx_area_detector_flt_precomp_flood_field()";

	unsigned long i, num_pixels;
	float image_pixel, bias_offset;
	float *flood_field_scale_array;
	uint16_t *mask_data_array, *bias_data_array;
	float *flt_image_data_array;
	long image_format;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_FLOAT ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	flt_image_data_array = image_frame->image_data;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	/*---*/

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	/*---*/

	/* Get the flood field scale array, creating a new one if necessary. */

	if ( flood_field_frame == NULL ) {
		flood_field_scale_array = NULL;
	} else {
		if ( ad->flood_field_scale_array == NULL ) {
		    mx_status = mx_area_detector_compute_flood_field_scale(
					ad, bias_frame, flood_field_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		flood_field_scale_array = ad->flood_field_scale_array;
	}

	/* If requested, do the flood field correction. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	if ( flood_field_scale_array != NULL ) {

		num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
				* MXIF_COLUMN_FRAMESIZE(image_frame);

		for ( i = 0; i < num_pixels; i++ ) {

			if ( mask_data_array != NULL ) {
				if ( mask_data_array[i] == 0 ) {

					/* If the pixel is masked off, skip
					 * this pixel and return to the top
					 * of the loop for the next pixel.
					 */

					continue;
				}
			}

			if ( ad->bias_corr_after_flood ) {
				bias_offset = 0;
			} else
			if ( bias_data_array == NULL ) {
				bias_offset = 0;
			} else {
				bias_offset = bias_data_array[i];
			}

			image_pixel = flt_image_data_array[i];

			image_pixel = image_pixel - bias_offset;

			image_pixel = image_pixel * flood_field_scale_array[i];

			image_pixel = image_pixel + bias_offset;

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			flt_image_data_array[i] = image_pixel;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_dbl_precomp_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] =
		"mx_area_detector_dbl_precomp_flood_field()";

	unsigned long i, num_pixels;
	double image_pixel, bias_offset;
	float *flood_field_scale_array;
	uint16_t *mask_data_array, *bias_data_array;
	double *dbl_image_data_array;
	long image_format;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_DOUBLE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	dbl_image_data_array = image_frame->image_data;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	/*---*/

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	/*---*/

	/* Get the flood field scale array, creating a new one if necessary. */

	if ( flood_field_frame == NULL ) {
		flood_field_scale_array = NULL;
	} else {
		if ( ad->flood_field_scale_array == NULL ) {
		    mx_status = mx_area_detector_compute_flood_field_scale(
					ad, bias_frame, flood_field_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		flood_field_scale_array = ad->flood_field_scale_array;
	}

	/* If requested, do the flood field correction. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	if ( flood_field_scale_array != NULL ) {

		num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
				* MXIF_COLUMN_FRAMESIZE(image_frame);

		for ( i = 0; i < num_pixels; i++ ) {

			if ( mask_data_array != NULL ) {
				if ( mask_data_array[i] == 0 ) {

					/* If the pixel is masked off, skip
					 * this pixel and return to the top
					 * of the loop for the next pixel.
					 */

					continue;
				}
			}

			if ( ad->bias_corr_after_flood ) {
				bias_offset = 0;
			} else
			if ( bias_data_array == NULL ) {
				bias_offset = 0;
			} else {
				bias_offset = bias_data_array[i];
			}

			image_pixel = dbl_image_data_array[i];

			image_pixel = image_pixel - bias_offset;

			image_pixel = image_pixel * flood_field_scale_array[i];

			image_pixel = image_pixel + bias_offset;

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			dbl_image_data_array[i] = image_pixel;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_u16_plain_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] = "mx_area_detector_u16_plain_flood_field()";

	unsigned long i, num_pixels;
	uint16_t *mask_data_array, *bias_data_array;
	uint16_t *flood_field_data_array;
	uint16_t *u16_image_data_array;
	long image_format;
	unsigned long raw_flood_field, bias_offset;
	double bias_average;
	double image_pixel, flood_field_scale_factor;
	double ffs_numerator, ffs_denominator;
	double flood_field_scale_max, flood_field_scale_min;
	mx_bool_type all_pixels_underflowed;

	/* Return now if we have not been provided with a flood field frame. */

	if ( flood_field_frame == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	flood_field_data_array = flood_field_frame->image_data;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	u16_image_data_array = image_frame->image_data;

	/* Discard the flood field scale array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->flood_field_scale_array );

	/* Get pointers to the mask and bias image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	if ( bias_data_array == NULL ) {
		all_pixels_underflowed = FALSE;
	} else {
		all_pixels_underflowed = TRUE;
	}

	flood_field_scale_max = ad->flood_field_scale_max;
	flood_field_scale_min = ad->flood_field_scale_min;

	/* Now do the flood field correction. */

	for ( i = 0; i < num_pixels; i++ ) {

		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				u16_image_data_array[i] = 0;
				continue;
			}
		}

		image_pixel = (double) u16_image_data_array[i];

		raw_flood_field = flood_field_data_array[i];

		if ( ad->bias_corr_after_flood ) {
			bias_offset = 0;
			bias_average = 0;
		} else
		if ( bias_data_array == NULL ) {
			bias_offset = 0;
			bias_average = 0;
		} else {
			bias_offset = bias_data_array[i];
			bias_average = ad->bias_average_intensity;
		}

		/* We _must_ _not_ use mx_divide_safely() here since function
		 * calls have too high of an overhead to be used in this loop.
		 */

		if ( raw_flood_field <= bias_offset ) {
			u16_image_data_array[i] = 0;
			continue;	/* Skip to the next pixel. */
		} else {
			all_pixels_underflowed = FALSE;
		}

		ffs_denominator = raw_flood_field - bias_offset;

		ffs_numerator = ad->flood_field_average_intensity
					- bias_average;

		flood_field_scale_factor = ffs_numerator / ffs_denominator;

		if ( flood_field_scale_factor > flood_field_scale_max ) {
			u16_image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}
		if ( flood_field_scale_factor < flood_field_scale_min ) {
			u16_image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}

		image_pixel = image_pixel - bias_offset;

		image_pixel = image_pixel * flood_field_scale_factor;

		image_pixel = image_pixel + bias_offset;

		/* Round to the nearest integer by adding 0.5 and then
		 * truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function calls
		 * have too high of an overhead to be used in this loop.
		 */

		if ( image_pixel < 0.0 ) {
			u16_image_data_array[i] = 0;
		} else {
			u16_image_data_array[i] = image_pixel + 0.5;
		}
	}

	if ( all_pixels_underflowed ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The entire contents of the corrected image frame for "
		"area detector '%s' have been set to 0 since each pixel in "
		"the flood field frame '%s' is less than or equal to the "
		"value of the corresponding pixel in the bias frame '%s'.",
			ad->record->name,
			ad->flood_field_filename,
			ad->bias_filename );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_s32_plain_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] = "mx_area_detector_s32_plain_flood_field()";

	unsigned long i, num_pixels;
	uint16_t *mask_data_array, *bias_data_array;
	uint16_t *flood_field_data_array;
	int32_t *s32_image_data_array;
	long image_format;
	unsigned long raw_flood_field, bias_offset;
	double bias_average;
	double image_pixel, flood_field_scale_factor;
	double ffs_numerator, ffs_denominator;
	double flood_field_scale_max, flood_field_scale_min;

	/* Return now if we have not been provided with a flood field frame. */

	if ( flood_field_frame == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	flood_field_data_array = flood_field_frame->image_data;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_INT32 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	s32_image_data_array = image_frame->image_data;

	/* Discard the flood field scale array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->flood_field_scale_array );

	/* Get pointers to the mask and bias image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	flood_field_scale_max = ad->flood_field_scale_max;
	flood_field_scale_min = ad->flood_field_scale_min;

	/* Now do the flood field correction. */

	for ( i = 0; i < num_pixels; i++ ) {

		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				s32_image_data_array[i] = 0;
				continue;
			}
		}

		image_pixel = (double) s32_image_data_array[i];

		raw_flood_field = flood_field_data_array[i];

		if ( ad->bias_corr_after_flood ) {
			bias_offset = 0;
			bias_average = 0;
		} else
		if ( bias_data_array == NULL ) {
			bias_offset = 0;
			bias_average = 0;
		} else {
			bias_offset = bias_data_array[i];
			bias_average = ad->bias_average_intensity;
		}

		/* Skip pixels for which the denominator would be zero. */

		if ( raw_flood_field == bias_offset ) {
			s32_image_data_array[i]  = 0;
			continue;
		}

		ffs_denominator = raw_flood_field - bias_offset;

		ffs_numerator = ad->flood_field_average_intensity
					- bias_average;

		flood_field_scale_factor = ffs_numerator / ffs_denominator;

		if ( flood_field_scale_factor > flood_field_scale_max ) {
			s32_image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}
		if ( flood_field_scale_factor < flood_field_scale_min ) {
			s32_image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}

		image_pixel = image_pixel - bias_offset;

		image_pixel = image_pixel * flood_field_scale_factor;

		image_pixel = image_pixel + bias_offset;

		/* Round to the nearest integer by adding 0.5 and then
		 * truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function calls
		 * have too high of an overhead to be used in this loop.
		 */

		if ( image_pixel < 0.0 ) {
			s32_image_data_array[i] = image_pixel - 0.5;
		} else {
			s32_image_data_array[i] = image_pixel + 0.5;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_flt_plain_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] = "mx_area_detector_flt_plain_flood_field()";

	unsigned long i, num_pixels;
	uint16_t *mask_data_array, *bias_data_array;
	uint16_t *flood_field_data_array;
	float *flt_image_data_array;
	long image_format;
	unsigned long raw_flood_field, bias_offset;
	float bias_average;
	float image_pixel, flood_field_scale_factor;
	float ffs_numerator, ffs_denominator;
	float flood_field_scale_max, flood_field_scale_min;

	/* Return now if we have not been provided with a flood field frame. */

	if ( flood_field_frame == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	flood_field_data_array = flood_field_frame->image_data;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_FLOAT ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	flt_image_data_array = image_frame->image_data;

	/* Discard the flood field scale array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->flood_field_scale_array );

	/* Get pointers to the mask and bias image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	flood_field_scale_max = ad->flood_field_scale_max;
	flood_field_scale_min = ad->flood_field_scale_min;

	/* Now do the flood field correction. */

	for ( i = 0; i < num_pixels; i++ ) {

		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				flt_image_data_array[i] = 0;
				continue;
			}
		}

		image_pixel = flt_image_data_array[i];

		raw_flood_field = flood_field_data_array[i];

		if ( ad->bias_corr_after_flood ) {
			bias_offset = 0;
			bias_average = 0;
		} else
		if ( bias_data_array == NULL ) {
			bias_offset = 0;
			bias_average = 0;
		} else {
			bias_offset = bias_data_array[i];
			bias_average = ad->bias_average_intensity;
		}

		/* Skip pixels for which the denominator would be zero. */

		if ( raw_flood_field == bias_offset ) {
			flt_image_data_array[i]  = 0;
			continue;
		}

		ffs_denominator = raw_flood_field - bias_offset;

		ffs_numerator = ad->flood_field_average_intensity
					- bias_average;

		flood_field_scale_factor = ffs_numerator / ffs_denominator;

		if ( flood_field_scale_factor > flood_field_scale_max ) {
			flt_image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}
		if ( flood_field_scale_factor < flood_field_scale_min ) {
			flt_image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}

		image_pixel = image_pixel - bias_offset;

		image_pixel = image_pixel * flood_field_scale_factor;

		image_pixel = image_pixel + bias_offset;

		/* Round to the nearest integer by adding 0.5 and then
		 * truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function calls
		 * have too high of an overhead to be used in this loop.
		 */

		flt_image_data_array[i] = image_pixel;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_dbl_plain_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] = "mx_area_detector_dbl_plain_flood_field()";

	unsigned long i, num_pixels;
	uint16_t *mask_data_array, *bias_data_array;
	uint16_t *flood_field_data_array;
	double *dbl_image_data_array;
	long image_format;
	unsigned long raw_flood_field, bias_offset;
	double bias_average;
	double image_pixel, flood_field_scale_factor;
	double ffs_numerator, ffs_denominator;
	double flood_field_scale_max, flood_field_scale_min;

	/* Return now if we have not been provided with a flood field frame. */

	if ( flood_field_frame == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	flood_field_data_array = flood_field_frame->image_data;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_DOUBLE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			image_format, ad->record->name );
	}

	dbl_image_data_array = image_frame->image_data;

	/* Discard the flood field scale array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->flood_field_scale_array );

	/* Get pointers to the mask and bias image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	flood_field_scale_max = ad->flood_field_scale_max;
	flood_field_scale_min = ad->flood_field_scale_min;

	/* Now do the flood field correction. */

	for ( i = 0; i < num_pixels; i++ ) {

		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				dbl_image_data_array[i] = 0;
				continue;
			}
		}

		image_pixel = dbl_image_data_array[i];

		raw_flood_field = flood_field_data_array[i];

		if ( ad->bias_corr_after_flood ) {
			bias_offset = 0;
			bias_average = 0;
		} else
		if ( bias_data_array == NULL ) {
			bias_offset = 0;
			bias_average = 0;
		} else {
			bias_offset = bias_data_array[i];
			bias_average = ad->bias_average_intensity;
		}

		/* Skip pixels for which the denominator would be zero. */

		if ( raw_flood_field == bias_offset ) {
			dbl_image_data_array[i]  = 0;
			continue;
		}

		ffs_denominator = raw_flood_field - bias_offset;

		ffs_numerator = ad->flood_field_average_intensity
					- bias_average;

		flood_field_scale_factor = ffs_numerator / ffs_denominator;

		if ( flood_field_scale_factor > flood_field_scale_max ) {
			dbl_image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}
		if ( flood_field_scale_factor < flood_field_scale_min ) {
			dbl_image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}

		image_pixel = image_pixel - bias_offset;

		image_pixel = image_pixel * flood_field_scale_factor;

		image_pixel = image_pixel + bias_offset;

		/* Round to the nearest integer by adding 0.5 and then
		 * truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function calls
		 * have too high of an overhead to be used in this loop.
		 */

		dbl_image_data_array[i] = image_pixel;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

static mx_status_type
mxp_area_detector_u16_delayed_bias_offset( MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *bias_frame )
{
	static const char fname[] =
		"mxp_area_detector_u16_delayed_bias_offset()";

	unsigned long i, num_pixels;
	uint16_t *u16_image_data_array;
	uint16_t *bias_data_array;
	double image_pixel;
	unsigned long bias_offset;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	u16_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Get a pointer to the bias image data. */

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: bias_data_array = %p", fname, bias_data_array));
#endif

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	/* Do the bias offset corrections. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* Get the bias offset for this pixel. */

		bias_offset = bias_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "bias_offset = %d, ",
					(int) bias_offset );
		}
#endif
		/* Get the raw image pixel. */

		image_pixel = (double) u16_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "BEFORE image_pixel = %g, ",
				image_pixel );
		}
#endif

		image_pixel = image_pixel + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr,
			"AFTER adding bias, image_pixel = %g, ", image_pixel );
		}
#endif
		/* Round to the nearest integer by adding 0.5 and
		 * then truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function
		 * calls have too high of an overhead to be used
		 * in this loop.
		 */

		if ( image_pixel < 0.0 ) {
			u16_image_data_array[i] = 0;
		} else {
			u16_image_data_array[i] = image_pixel + 0.5;
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "image_data_array[%lu] = %ld\n",
				i, (long) u16_image_data_array[i] );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_s32_delayed_bias_offset( MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *bias_frame )
{
	static const char fname[] =
		"mxp_area_detector_s32_delayed_bias_offset()";

	unsigned long i, num_pixels;
	int32_t *s32_image_data_array;
	uint16_t *bias_data_array;
	double image_pixel;
	unsigned long bias_offset;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	s32_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Get a pointer to the bias image data. */

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: bias_data_array = %p", fname, bias_data_array));
#endif

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	/* Do the bias offset corrections. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* Get the bias offset for this pixel. */

		bias_offset = bias_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "bias_offset = %d, ",
					(int) bias_offset );
		}
#endif
		/* Get the raw image pixel. */

		image_pixel = (double) s32_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "BEFORE image_pixel = %g, ",
				image_pixel );
		}
#endif

		image_pixel = image_pixel + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr,
			"AFTER adding bias, image_pixel = %g, ", image_pixel );
		}
#endif
		/* Round to the nearest integer by adding 0.5 and
		 * then truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function
		 * calls have too high of an overhead to be used
		 * in this loop.
		 */

		if ( image_pixel < 0.0 ) {
			s32_image_data_array[i] = image_pixel - 0.5;
		} else {
			s32_image_data_array[i] = image_pixel + 0.5;
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "s32_image_data_array[%lu] = %ld\n",
				i, (long) s32_image_data_array[i] );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_dbl_delayed_bias_offset( MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *bias_frame )
{
	static const char fname[] =
		"mxp_area_detector_dbl_delayed_bias_offset()";

	unsigned long i, num_pixels;
	double *dbl_image_data_array;
	uint16_t *bias_data_array;
	double image_pixel;
	unsigned long bias_offset;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	dbl_image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_frame->image_data = %p",
		fname, image_frame->image_data));
#endif

	/* Get a pointer to the bias image data. */

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: bias_data_array = %p", fname, bias_data_array));
#endif

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	/* Do the bias offset corrections. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* Get the bias offset for this pixel. */

		bias_offset = bias_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "bias_offset = %d, ",
					(int) bias_offset );
		}
#endif
		/* Get the raw image pixel. */

		image_pixel = dbl_image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "BEFORE image_pixel = %g, ",
				image_pixel );
		}
#endif

		image_pixel = image_pixel + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr,
			"AFTER adding bias, image_pixel = %g, ", image_pixel );
		}
#endif
		/* Round to the nearest integer by adding 0.5 and
		 * then truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function
		 * calls have too high of an overhead to be used
		 * in this loop.
		 */

		dbl_image_data_array[i] = image_pixel;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "image_data_array[%lu] = %ld\n",
				i, (long) dbl_image_data_array[i] );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_delayed_bias_offset( MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *bias_frame )
{
	static const char fname[] = "mxp_area_detector_delayed_bias_offset()";

	long image_format;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_GREY16:
		mx_status = mxp_area_detector_u16_delayed_bias_offset(
						image_frame, bias_frame );
		break;
	case MXT_IMAGE_FORMAT_INT32:
		mx_status = mxp_area_detector_s32_delayed_bias_offset(
						image_frame, bias_frame );
		break;
	case MXT_IMAGE_FORMAT_DOUBLE:
		mx_status = mxp_area_detector_dbl_delayed_bias_offset(
						image_frame, bias_frame );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported.",
			image_format );
		break;
	}

	return mx_status;
}

/*=======================================================================*/

/* mx_area_detector_classic_frame_correction() requires that all of the frames
 * have the same framesize.
 */

MX_EXPORT mx_status_type
mx_area_detector_classic_frame_correction( MX_RECORD *record,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *dark_current_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] =
		"mx_area_detector_classic_frame_correction()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *geometrical_correction_fn ) ( MX_AREA_DETECTOR *,
							MX_IMAGE_FRAME * );
	MX_IMAGE_FRAME *correction_calc_frame;
	unsigned long flags;
	unsigned long image_format, correction_format;
	mx_bool_type memory_is_low = FALSE;
	mx_bool_type geom_corr_before_flood;
	mx_bool_type geom_corr_after_flood;
	mx_bool_type correction_measurement_in_progress;
	mx_bool_type geometrical_correction_requested;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_TIMING initial_timing = MX_HRT_ZERO;
	MX_HRT_TIMING geometrical_timing = MX_HRT_ZERO;
	MX_HRT_TIMING flood_timing = MX_HRT_ZERO;
	MX_HRT_TIMING initial_convert_timing = MX_HRT_ZERO;
	MX_HRT_TIMING final_convert_timing = MX_HRT_ZERO;
	MX_HRT_TIMING delayed_bias_timing = MX_HRT_ZERO;
#endif

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("\n%s invoked for area detector '%s'.",
		fname, record->name ));
#endif

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"No primary image frame was provided." );
	}

	flags = ad->correction_flags;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_FLAGS
	MX_DEBUG(-2,("%s: ad->correction_flags = %#lx",
		fname, ad->correction_flags));
#endif

	if ( flags == 0 ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,("%s: No corrections requested.", fname));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* Under what circumstances will we perform a geometrical correction? */

	correction_measurement_in_progress =
		ad->correction_measurement_in_progress;

	if ( ad->correction_flags & MXFT_AD_GEOMETRICAL_CORRECTION ) {
		geometrical_correction_requested = TRUE;
	} else {
		geometrical_correction_requested = FALSE;
	}

	if ( correction_measurement_in_progress ) {
		/* We are measuring a correction_frame. */

		if ( ad->correction_frame_no_geom_corr ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = FALSE;
		} else
		if ( ad->correction_frame_geom_corr_last ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = FALSE;
		} else
		if ( ad->geom_corr_after_flood ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = TRUE;
		} else {
			geom_corr_before_flood = TRUE;
			geom_corr_after_flood = FALSE;
		}
	} else {
		/* We are _not_ measuring a correction_frame. */

		if ( geometrical_correction_requested == FALSE ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = FALSE;
		} else
		if ( ad->geom_corr_after_flood ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = TRUE;
		} else {
			geom_corr_before_flood = TRUE;
			geom_corr_after_flood = FALSE;
		}
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_FLAGS
	MX_DEBUG(-2,("%s: correction_measurement_in_progress = %d",
		fname, (int) correction_measurement_in_progress));
	MX_DEBUG(-2,("%s: geometrical_correction_requested = %d",
		fname, (int) geometrical_correction_requested));
	MX_DEBUG(-2,("%s: ad->correction_frame_no_geom_corr = %d",
		fname, (int) ad->correction_frame_no_geom_corr));
	MX_DEBUG(-2,("%s: ad->correction_frame_geom_corr_last = %d",
		fname, (int) ad->correction_frame_geom_corr_last));
	MX_DEBUG(-2,("%s: ad->geom_corr_after_flood = %d",
		fname, (int) ad->geom_corr_after_flood));
	MX_DEBUG(-2,
	("%s: geom_corr_before_flood = %d, geom_corr_after_flood = %d",
		fname, geom_corr_before_flood, geom_corr_after_flood));
#endif

	/*---*/

	/* If we have a lot of free memory available, then we can use
	 * floating point arrays of correction values to speed up the
	 * corrections.  We call mx_area_detector_check_for_low_memory()
	 * to see if enough memory is available.
	 */

	mx_status = mx_area_detector_check_for_low_memory( ad, &memory_is_low );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	geometrical_correction_fn = flist->geometrical_correction;

	if ( geometrical_correction_fn == NULL ) {
		geometrical_correction_fn =
			mx_area_detector_default_geometrical_correction;
	}

	/* Area detector image correction is currently only supported
	 * for 16-bit greyscale images (MXT_IMAGE_FORMAT_GREY16).
	 */

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The primary image frame is not a 16-bit greyscale image." );
	}

	if ( ad->correction_calc_format == MXT_IMAGE_FORMAT_DEFAULT ) {
		ad->correction_calc_format = image_format;
	}

	/*----*/

	if ( ( ad->correction_calc_format != image_format )
	  && geom_corr_before_flood )
	{
		char correction_fmt_name[40];
		char image_fmt_name[40];

		(void) mx_image_get_image_format_name_from_type(
				ad->correction_calc_format,
				correction_fmt_name,
				sizeof(correction_fmt_name) );

		(void) mx_image_get_image_format_name_from_type(
				image_format,
				image_fmt_name,
				sizeof(image_fmt_name) );

		return mx_error( MXE_UNSUPPORTED, fname,
"\nThe following combination of settings for area detector '%s' is unsupported:"
"\n"
"\n1.  The image correction format '%s' (%ld) is different from the "
"\n        image format '%s' %ld."
"\n2.  Geometrical correction is configured to take place before the flood."
"\n"
"\nThe recommended solution is to configure geometrical correction to take"
"\nplace after the flood.", record->name,
		correction_fmt_name, ad->correction_calc_format,
		image_fmt_name, image_format );
	}

	/*----*/

	/* If the correction calculation format is not the same as the
	 * native image format, then we must create an image frame to
	 * contain the intermediate results of the correction calculation.
	 */

	if ( image_format == ad->correction_calc_format ) {
		correction_calc_frame = image_frame;
	} else {
		double corr_bytes_per_pixel;
		size_t corr_image_length;
		long row_framesize, column_framesize;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_START( initial_convert_timing );
#endif

		row_framesize    = MXIF_ROW_FRAMESIZE(image_frame);
		column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);

		mx_status = mx_image_format_get_bytes_per_pixel(
					ad->correction_calc_format,
					&corr_bytes_per_pixel );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		corr_image_length = mx_round( corr_bytes_per_pixel
			* (double)( row_framesize * column_framesize ) );

		mx_status = mx_image_alloc( &(ad->correction_calc_frame),
					row_framesize,
					column_framesize,
					ad->correction_calc_format,
					MXIF_BYTE_ORDER(image_frame),
					corr_bytes_per_pixel,
					MXIF_HEADER_BYTES(image_frame),
					corr_image_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_area_detector_copy_and_convert_image_data(
					ad->correction_calc_frame, image_frame);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		correction_calc_frame = ad->correction_calc_frame;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( initial_convert_timing );
#endif
	}

	correction_format = MXIF_IMAGE_FORMAT(correction_calc_frame);

	/*---*/

	mx_status = mx_area_detector_check_correction_framesize( ad,
					    image_frame, mask_frame, "mask" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_check_correction_framesize( ad,
					    image_frame, bias_frame, "bias" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_check_correction_framesize( ad,
			    image_frame, dark_current_frame, "dark current" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_check_correction_framesize( ad,
			    image_frame, flood_field_frame, "flood field" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_START( initial_timing );
#endif

	/******* Dark current correction *******/

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: memory_is_low = %d", fname, (int) memory_is_low));
#endif

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_FLAGS
	MX_DEBUG(-2,("%s: dark_current_frame = %p", fname, dark_current_frame));
#endif

	if ( memory_is_low ) {

		/* Do not use a precomputed dark current offset array.
		 * Instead, save memory by computing the necessary
		 * offset on the fly.
		 */

		switch( correction_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
			mx_status =
			    mx_area_detector_u16_plain_dark_correction( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
			break;
		case MXT_IMAGE_FORMAT_INT32:
			mx_status =
			    mx_area_detector_s32_plain_dark_correction( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
			break;
		case MXT_IMAGE_FORMAT_FLOAT:
			mx_status =
			    mx_area_detector_flt_plain_dark_correction( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
			break;
		case MXT_IMAGE_FORMAT_DOUBLE:
			mx_status =
			    mx_area_detector_dbl_plain_dark_correction( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Dark current correction requested for illegal "
			"correction calculation frame format %ld "
			"for area detector '%s'.",
				correction_format, ad->record->name );
			break;
		}
	} else {
		/* Use precomputed ad->dark_current_offset_array. */

		switch( correction_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
			mx_status =
			    mx_area_detector_u16_precomp_dark_correction( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
			break;
		case MXT_IMAGE_FORMAT_INT32:
			mx_status =
			    mx_area_detector_s32_precomp_dark_correction( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
			break;
		case MXT_IMAGE_FORMAT_FLOAT:
			mx_status =
			    mx_area_detector_flt_precomp_dark_correction( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
			break;
		case MXT_IMAGE_FORMAT_DOUBLE:
			mx_status =
			    mx_area_detector_dbl_precomp_dark_correction( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Dark current correction requested for illegal "
			"correction calculation frame format %ld "
			"for area detector '%s'.",
				correction_format, ad->record->name );
			break;
		}
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( initial_timing );

	MX_HRT_START( geometrical_timing );
#endif

	/******* Geometrical correction *******/

	if ( geom_corr_before_flood ) {

		/* If requested, do the geometrical correction. */

		if ( flags & MXFT_AD_GEOMETRICAL_CORRECTION ) {
			mx_status = (*geometrical_correction_fn)( ad,
								image_frame );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( geometrical_timing );

	MX_HRT_START( flood_timing );
#endif

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_FLAGS
	MX_DEBUG(-2,("%s: flood_field_frame = %p", fname, flood_field_frame));
#endif

	/******* Flood field correction *******/

	if ( memory_is_low ) {
		switch( correction_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
			mx_status =
			    mx_area_detector_u16_plain_flood_field( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
			break;
		case MXT_IMAGE_FORMAT_INT32:
			mx_status =
			    mx_area_detector_s32_plain_flood_field( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
			break;
		case MXT_IMAGE_FORMAT_FLOAT:
			mx_status =
			    mx_area_detector_flt_plain_flood_field( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
			break;
		case MXT_IMAGE_FORMAT_DOUBLE:
			mx_status =
			    mx_area_detector_dbl_plain_flood_field( ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Flood field correction requested for illegal "
			"correction calculation frame format %ld "
			"for area detector '%s'.",
				correction_format, ad->record->name );
			break;
		}
	} else {
		switch( correction_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
			mx_status = mx_area_detector_u16_precomp_flood_field(
							ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
			break;
		case MXT_IMAGE_FORMAT_INT32:
			mx_status = mx_area_detector_s32_precomp_flood_field(
							ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
			break;
		case MXT_IMAGE_FORMAT_FLOAT:
			mx_status = mx_area_detector_flt_precomp_flood_field(
							ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
			break;
		case MXT_IMAGE_FORMAT_DOUBLE:
			mx_status = mx_area_detector_dbl_precomp_flood_field(
							ad,
							correction_calc_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Flood field correction requested for illegal "
			"correction calculation frame format %ld "
			"for area detector '%s'.",
				correction_format, ad->record->name );
			break;
		}
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( flood_timing );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/******* Delayed bias offset (if requested) *******/

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_START( delayed_bias_timing );
#endif

	if ( ad->bias_corr_after_flood && ( bias_frame != NULL ) ) {

		mx_status = mxp_area_detector_delayed_bias_offset(
					correction_calc_frame, bias_frame );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( delayed_bias_timing );
#endif

	/******* If needed, convert the data back to the native format *******/

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_START( final_convert_timing );
#endif

	if ( correction_calc_frame != image_frame ) {
		mx_status = mx_area_detector_copy_and_convert_image_data(
					image_frame, ad->correction_calc_frame);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( final_convert_timing );
#endif

	/******* Delayed geometrical correction (if requested) *******/

	if ( geom_corr_after_flood ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
		/* If invoked, this code overwrites the earlier
		 * measured value for the timing.
		 */

		MX_HRT_START( geometrical_timing );
#endif
		/* If requested, do the geometrical correction. */

		if ( flags & MXFT_AD_GEOMETRICAL_CORRECTION ) {
			mx_status = (*geometrical_correction_fn)( ad,
								image_frame );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
		MX_HRT_END( geometrical_timing );
#endif
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_DEBUG(-2,(" "));	/* Print an empty line. */

	if ( correction_calc_frame != image_frame ) {
		MX_HRT_RESULTS( initial_convert_timing,
			fname, "Initial type conversion time." );
	}

	MX_HRT_RESULTS( initial_timing, fname,
				"Mask, bias, and dark correction time." );

	if ( geom_corr_after_flood ) {
		MX_HRT_RESULTS( flood_timing, fname, "Flood correction time." );

		if ( ad->bias_corr_after_flood ) {
			MX_HRT_RESULTS( delayed_bias_timing, fname,
				"Delayed bias offset time." );
		}

		MX_HRT_RESULTS( geometrical_timing, fname,
				"Geometrical correction time." );
	} else {
		MX_HRT_RESULTS( geometrical_timing, fname,
				"Geometrical correction time." );
		MX_HRT_RESULTS( flood_timing, fname, "Flood correction time." );

		if ( ad->bias_corr_after_flood ) {
			MX_HRT_RESULTS( delayed_bias_timing, fname,
				"Delayed bias offset time." );
		}
	}

	if ( correction_calc_frame != image_frame ) {
		MX_HRT_RESULTS( final_convert_timing,
			fname, "Final type conversion time." );
	}

#endif

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

