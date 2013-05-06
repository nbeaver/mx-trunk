/*
 * Name:    mx_area_detector_rdi.c
 *
 * Purpose: Functions used in correcting images for RDI area detectors.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/*---*/

#define MX_RDI_DEBUG_CORRECTION				FALSE

#define MX_RDI_DEBUG_CORRECTION_STATISTICS		FALSE

#define MX_RDI_DEBUG_CORRECTION_TIMING			TRUE

#define MX_RDI_DEBUG_LOOP_TIMING			TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt_debug.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_area_detector_rdi.h"

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_rdi_correct_frame( MX_AREA_DETECTOR *ad,
			double minimum_pixel_value,
			double saturation_pixel_value,
			unsigned long rdi_flags )
{
	static const char fname[] = "mx_rdi_correct_frame()";

	MX_IMAGE_FRAME *image_frame, *mask_frame, *bias_frame;
	MX_IMAGE_FRAME *dark_current_frame, *non_uniformity_frame;
	MX_IMAGE_FRAME *correction_calc_frame;
	uint16_t *image_array;
	float *corr_float_array;
	unsigned long flags;
	unsigned long i, corr_pixels_per_frame;
	unsigned long image_format, correction_format;
	mx_bool_type correction_measurement_in_progress;
	mx_status_type mx_status;

#if MX_RDI_DEBUG_CORRECTION_TIMING
	MX_HRT_TIMING setup_measurement;
	MX_HRT_TIMING alloc_measurement;
	MX_HRT_TIMING correction_measurement;
	MX_HRT_TIMING convert_measurement;
	MX_HRT_TIMING header_measurement;
	MX_HRT_TIMING total_measurement;
#endif

	flags = ad->correction_flags;

#if MX_RDI_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s', flags = %#lx",
		fname, ad->record->name ));
#endif

	if ( flags == 0 ) {
		/* If there is no correction to be done, then just return. */

#if MX_RDI_DEBUG_CORRECTION
		MX_DEBUG(-2,(
		"%s returning, since no correction is needed for '%s'",
			fname, ad->record->name));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

#if MX_RDI_DEBUG_CORRECTION_TIMING
	MX_HRT_START(total_measurement);
	MX_HRT_START(setup_measurement);
#endif

	image_frame = ad->image_frame;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
	    "Area detector '%s' has not yet read out its first image frame.",
			ad->record->name );
	}

#if MX_RDI_DEBUG_CORRECTION_STATISTICS
	MX_DEBUG(-2,("%s: raw image frame statistics:", fname));
	mx_image_statistics( image_frame );
#endif

	/* Area detector image correction is currently only supported
	 * for 16-bit greyscale images (MXT_IMAGE_FORMAT_GREY16).
	 */

	image_format = MXIF_IMAGE_FORMAT(image_frame);

	if ( image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"the primary image frame is not a 16-bit greyscale image." );
	}

	/*--- Find the frame pointers for the image frames to be used. ---*/

	mask_frame = ad->mask_frame;

#if MX_RDI_DEBUG_CORRECTION_STATISTICS
	MX_DEBUG(-2,("%s: mask frame statistics:", fname));
	if ( mask_frame == NULL ) {
		MX_DEBUG(-2,("%s:    No mask frame loaded.", fname));
	} else {
		mx_image_statistics( mask_frame );
	}
#endif

	bias_frame = ad->bias_frame;

#if MX_RDI_DEBUG_CORRECTION_STATISTICS
	MX_DEBUG(-2,("%s: bias frame statistics:", fname));
	if ( bias_frame == NULL ) {
		MX_DEBUG(-2,("%s:    No bias frame loaded.", fname));
	} else {
		mx_image_statistics( bias_frame );
	}
#endif

	dark_current_frame = ad->dark_current_frame;

#if MX_RDI_DEBUG_CORRECTION_STATISTICS
	MX_DEBUG(-2,("%s: dark current frame statistics:", fname));
	if ( dark_current_frame == NULL ) {
		MX_DEBUG(-2,("%s:    No dark current frame loaded.", fname));
	} else {
		mx_image_statistics( dark_current_frame );
	}
#endif

	non_uniformity_frame = ad->flood_field_frame;

#if MX_RDI_DEBUG_CORRECTION_STATISTICS
	MX_DEBUG(-2,("%s: non-uniformity frame statistics:", fname));
	if ( non_uniformity_frame == NULL ) {
		MX_DEBUG(-2,("%s:    No non-uniformity frame loaded.", fname));
	} else {
		mx_image_statistics( non_uniformity_frame );
	}
#endif

	correction_measurement_in_progress =
		ad->correction_measurement_in_progress;

#if MX_RDI_DEBUG_CORRECTION_TIMING
	MX_HRT_END(setup_measurement);
#endif

	/*---*/

#if MX_RDI_DEBUG_CORRECTION_TIMING
	MX_HRT_START(alloc_measurement);
#endif

	if ( ad->correction_calc_format == MXT_IMAGE_FORMAT_DEFAULT ) {
		ad->correction_calc_format = image_format;
	}

	/*---*/

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

		row_framesize    = MXIF_ROW_FRAMESIZE(image_frame);
		column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);

		mx_status = mx_image_format_get_bytes_per_pixel(
					ad->correction_calc_format,
					&corr_bytes_per_pixel );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		corr_pixels_per_frame = row_framesize * column_framesize;

		corr_image_length =
		    mx_round( corr_bytes_per_pixel * corr_pixels_per_frame );

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

		MXIF_EXPOSURE_TIME_SEC( ad->correction_calc_frame )
			= MXIF_EXPOSURE_TIME_SEC( image_frame );

		MXIF_EXPOSURE_TIME_NSEC( ad->correction_calc_frame )
			= MXIF_EXPOSURE_TIME_NSEC( image_frame );

		image_array = image_frame->image_data;

		switch( ad->correction_calc_format ) {
		case MXT_IMAGE_FORMAT_FLOAT:
			corr_float_array =
				ad->correction_calc_frame->image_data;

			for ( i = 0; i < corr_pixels_per_frame; i++ ) {
				corr_float_array[i] = image_array[i];
			}
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Correction calc frame types other than 'float' "
			"are not implemented here." );
			break;
		}

		correction_calc_frame = ad->correction_calc_frame;
	}

#if MX_RDI_DEBUG_CORRECTION_STATISTICS
	MX_DEBUG(-2,
		("%s: correction_calc_frame before image correction",fname));
	mx_image_statistics(correction_calc_frame);
#endif

#if MX_RDI_DEBUG_CORRECTION_TIMING
	MX_HRT_END(alloc_measurement);

	MX_HRT_START(correction_measurement);
#endif

	correction_format = MXIF_IMAGE_FORMAT(correction_calc_frame);

	switch( correction_format ) {
	case MXT_IMAGE_FORMAT_DOUBLE:
		mx_status = mx_rdi_dbl_image_correction( ad,
				correction_calc_frame,
				mask_frame,
				bias_frame,
				dark_current_frame,
				non_uniformity_frame,
				minimum_pixel_value,
				saturation_pixel_value,
				rdi_flags );
		break;
				
	case MXT_IMAGE_FORMAT_FLOAT:
		mx_status = mx_rdi_flt_image_correction( ad,
				correction_calc_frame,
				mask_frame,
				bias_frame,
				dark_current_frame,
				non_uniformity_frame,
				minimum_pixel_value,
				saturation_pixel_value,
				rdi_flags );
				
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support for image correction using correction format %ld "
		"for detector '%s' is not yet implemented.",
			correction_format,
			ad->record->name );
		break;
	}

#if MX_RDI_DEBUG_CORRECTION_STATISTICS
	MX_DEBUG(-2,("%s: final correction_calc_frame", fname));
	mx_image_statistics(correction_calc_frame);
#endif

#if MX_RDI_DEBUG_CORRECTION_TIMING
	MX_HRT_END(correction_measurement);

	MX_HRT_START(convert_measurement);
#endif

	/*--- If needed, convert the data back to the native format ---*/

	if ( correction_calc_frame != image_frame ) {

		image_array = image_frame->image_data;

		switch( ad->correction_calc_format ) {
		case MXT_IMAGE_FORMAT_FLOAT:

			/* NOTE: This copy operation does not do any tests on
			 * the value before copying it to the 16-bit integer
			 * array to make sure the value fits in a 16-bit
			 * unsigned integer.  This is because the function
			 * mx_rdi_flt_image_correction() above has already
			 * made more stringent tests on the value than we
			 * would here.
			 */

			corr_float_array =
				ad->correction_calc_frame->image_data;

			for ( i = 0; i < corr_pixels_per_frame; i++ ) {
				image_array[i] = (uint16_t) corr_float_array[i];
			}
			break;

		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Correction calc frame types other than 'float' "
			"are not implemented here." );
			break;
		}
	}

#if MX_RDI_DEBUG_CORRECTION_STATISTICS
	MX_DEBUG(-2,("%s: final image_frame", fname));
	mx_image_statistics(image_frame);
#endif

#if MX_RDI_DEBUG_CORRECTION_TIMING
	MX_HRT_END(convert_measurement);

	MX_HRT_START(header_measurement);
#endif

	/*--- Write the bias offset to the header. ---*/

	if ( bias_frame == NULL ) {
		MXIF_BIAS_OFFSET_MILLI_ADUS(image_frame) = 0;
	} else
	if ( (ad->correction_flags & MXFT_AD_BIAS_FRAME) == 0 ) {
		MXIF_BIAS_OFFSET_MILLI_ADUS(image_frame) = 0;
	} else {
		MXIF_BIAS_OFFSET_MILLI_ADUS(image_frame) =
			mx_round( 1000.0 * ad->bias_average_intensity );
	}

#if MX_RDI_DEBUG_CORRECTION_TIMING
	MX_HRT_END(header_measurement);
	MX_HRT_END(total_measurement);

	MX_HRT_RESULTS( setup_measurement, fname, "for setup." );
	MX_HRT_RESULTS( alloc_measurement, fname, "for alloc." );
	MX_HRT_RESULTS( correction_measurement, fname, "for correction." );
	MX_HRT_RESULTS( convert_measurement, fname, "for convert." );
	MX_HRT_RESULTS( header_measurement, fname, "for header." );
	MX_HRT_RESULTS( total_measurement, fname, "for total correction." );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_rdi_dbl_image_correction( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *correction_calc_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *dark_current_frame,
				MX_IMAGE_FRAME *non_uniformity_frame,
				double minimum_pixel_value,
				double saturation_pixel_value,
				unsigned long rdi_flags )
{
	static const char fname[] = "mx_rdi_dbl_image_correction()";

	unsigned long correction_flags;
	unsigned long i, num_pixels_per_frame;
	unsigned long num_mask_pixels, num_bias_pixels;
	unsigned long num_dark_current_pixels, num_non_uniformity_pixels;
	double *dbl_image_data_array;
	double image_pixel;
	uint16_t *u16_mask_data_array;
	uint16_t *u16_bias_data_array;
	float *flt_dark_current_data_array;
	float *flt_non_uniformity_data_array;
	uint16_t mask_pixel;
	double bias_pixel, dark_current_pixel, non_uniformity_pixel;
	long correction_calc_format, mask_format, bias_format;
	long dark_current_format, non_uniformity_format;
	double image_exposure_time, dark_current_exposure_time;
	mx_bool_type abort_if_different_exposure_times;
	mx_status_type mx_status;

	if ( correction_calc_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The correction_calc_frame pointer passed was NULL." );
	}

	correction_flags = ad->correction_flags;

#if 0
	MX_DEBUG(-2,("%s: correction_flags = %lu", fname, correction_flags));
#endif

	correction_calc_format = MXIF_IMAGE_FORMAT(correction_calc_frame);

	if ( correction_calc_format != MXT_IMAGE_FORMAT_DOUBLE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			correction_calc_format, ad->record->name );
	}

	dbl_image_data_array = correction_calc_frame->image_data;

	if ( dbl_image_data_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The image_data array pointer for the image frame "
		"is NULL for detector '%s'", ad->record->name );
	}

	num_pixels_per_frame = MXIF_ROW_FRAMESIZE(correction_calc_frame)
				* MXIF_COLUMN_FRAMESIZE(correction_calc_frame);

	/*--- Check that the mask frame is set up correctly ---*/

	if ( correction_flags & MXFT_AD_MASK_FRAME ) {

	    if ( mask_frame == NULL ) {
		mx_warning( "Mask correction skipped, since no mask frame "
			"is loaded for detector '%s'.", ad->record->name );

		correction_flags |= (~MXFT_AD_MASK_FRAME);
	    } else {
	    	mask_format = MXIF_IMAGE_FORMAT(mask_frame);

		if ( mask_format != MXT_IMAGE_FORMAT_GREY16 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Mask correction format %ld is not supported "
			"by this function for area detector '%s'.",
				mask_format, ad->record->name );
		} else {
		    num_mask_pixels = MXIF_ROW_FRAMESIZE(mask_frame)
				* MXIF_COLUMN_FRAMESIZE(mask_frame);

		    if ( num_pixels_per_frame != num_mask_pixels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The number of pixels in the image frame (%lu) is not "
			"the same as the number of pixels in the mask frame "
			"(%lu) for area detector '%s'.",
				num_pixels_per_frame, num_mask_pixels,
				ad->record->name );
		    } else {
			u16_mask_data_array = mask_frame->image_data;

			if ( u16_mask_data_array == NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The image_data pointer for the mask frame "
				"is NULL for detector '%s'.", ad->record->name);
			}
		    }
		}
	    }
	}

	/*--- Check that the bias frame is set up correctly ---*/

	if ( correction_flags & MXFT_AD_BIAS_FRAME ) {

	    if ( bias_frame == NULL ) {
		mx_warning( "Bias correction skipped, since no bias frame "
			"is loaded for detector '%s'.", ad->record->name );

		correction_flags |= (~MXFT_AD_BIAS_FRAME);
	    } else {
	    	bias_format = MXIF_IMAGE_FORMAT(bias_frame);

		if ( bias_format != MXT_IMAGE_FORMAT_GREY16 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Bias correction format %ld is not supported "
			"by this function for area detector '%s'.",
				bias_format, ad->record->name );
		} else {
		    num_bias_pixels = MXIF_ROW_FRAMESIZE(bias_frame)
				* MXIF_COLUMN_FRAMESIZE(bias_frame);

		    if ( num_pixels_per_frame != num_bias_pixels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The number of pixels in the image frame (%lu) is not "
			"the same as the number of pixels in the bias frame "
			"(%lu) for area detector '%s'.",
				num_pixels_per_frame, num_bias_pixels,
				ad->record->name );
		    } else {
			u16_bias_data_array = bias_frame->image_data;

			if ( u16_bias_data_array == NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The image_data pointer for the bias frame "
				"is NULL for detector '%s'.", ad->record->name);
			}
		    }
		}
	    }
	}

	/*--- Check that the dark current frame is set up correctly ---*/

	if ( correction_flags & MXFT_AD_DARK_CURRENT_FRAME ) {

	    if ( dark_current_frame == NULL ) {
		mx_warning( "Dark current correction skipped, since "
			"no dark current frame is loaded for detector '%s'.",
			ad->record->name );

		correction_flags |= (~MXFT_AD_DARK_CURRENT_FRAME);
	    } else {

		/* See if we need to abort if the image frame was taken for
		 * a different exposure time than the dark current frame.
		 */

		if (rdi_flags & MXF_RDI_BYPASS_DARK_CURRENT_EXPOSURE_TIME_TEST)
		{
			abort_if_different_exposure_times = FALSE;
		} else {
			abort_if_different_exposure_times = TRUE;
		}

		mx_status = mx_image_get_exposure_time( correction_calc_frame,
							&image_exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_exposure_time( dark_current_frame,
						&dark_current_exposure_time );

		if ( mx_difference( image_exposure_time,
				dark_current_exposure_time ) > 0.001 )
		{
		    if ( abort_if_different_exposure_times ) {

			mx_status = mx_area_detector_stop( ad->record );

			ad->latched_status |= MXSF_AD_EXPOSURE_TIME_CONFLICT;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The image frame for detector '%s' was taken for "
			"a different exposure time (%g seconds) than the "
			"exposure time for the dark current image "
			"(%g seconds).", ad->record->name,
				image_exposure_time,
				dark_current_exposure_time );
		    } else {
			mx_warning(
			"The image frame for detector '%s' was taken for "
			"a different exposure time (%g seconds) than the "
			"exposure time for the dark current image "
			"(%g seconds).", ad->record->name,
				image_exposure_time,
				dark_current_exposure_time );
		    }
		}

		/*---*/

	    	dark_current_format = MXIF_IMAGE_FORMAT(dark_current_frame);

		if ( dark_current_format != MXT_IMAGE_FORMAT_FLOAT ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Dark current correction format %ld is not supported "
			"by this function for area detector '%s'.",
				dark_current_format, ad->record->name );
		} else {
		    num_dark_current_pixels =
				MXIF_ROW_FRAMESIZE(dark_current_frame)
				* MXIF_COLUMN_FRAMESIZE(dark_current_frame);

		    if ( num_pixels_per_frame != num_dark_current_pixels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The number of pixels in the image frame (%lu) "
			"is not " "the same as the number of pixels in "
			"the dark current frame (%lu) for area detector '%s'.",
				num_pixels_per_frame, num_dark_current_pixels,
				ad->record->name );
		    } else {
			flt_dark_current_data_array =
					dark_current_frame->image_data;

			if ( flt_dark_current_data_array == NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The image_data pointer for the dark current "
				"frame is NULL for detector '%s'.",
				ad->record->name);
			}
		    }
		}
	    }
	}

	/*--- Check that the non uniformity frame is set up correctly ---*/

	if ( correction_flags & MXFT_AD_FLOOD_FIELD_FRAME ) {

	    if ( non_uniformity_frame == NULL ) {
		mx_warning( "Non-uniformity correction skipped, since "
			"no non-uniformity frame is loaded for detector '%s'.",
			ad->record->name );

		correction_flags |= (~MXFT_AD_FLOOD_FIELD_FRAME);
	    } else {
	    	non_uniformity_format = MXIF_IMAGE_FORMAT(non_uniformity_frame);

		if ( non_uniformity_format != MXT_IMAGE_FORMAT_FLOAT ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Non-uniformity correction format %ld is not supported "
			"by this function for area detector '%s'.",
				non_uniformity_format, ad->record->name );
		} else {
		    num_non_uniformity_pixels =
				MXIF_ROW_FRAMESIZE(non_uniformity_frame)
				* MXIF_COLUMN_FRAMESIZE(non_uniformity_frame);

		    if ( num_pixels_per_frame != num_non_uniformity_pixels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The number of pixels in the image frame (%lu) "
			"is not " "the same as the number of pixels in "
			"the dark current frame (%lu) for area detector '%s'.",
			    num_pixels_per_frame, num_non_uniformity_pixels,
			    ad->record->name );
		    } else {
			flt_non_uniformity_data_array =
					non_uniformity_frame->image_data;

			if ( flt_non_uniformity_data_array == NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The image_data pointer for the dark current "
				"frame is NULL for detector '%s'.",
				ad->record->name);
			}
		    }
		}
	    }
	}

	/*----*/

	/* Loop over all of the pixels and apply the requested corrections. */

	for ( i = 0; i < num_pixels_per_frame; i++ ) {

		image_pixel = dbl_image_data_array[i];

		if ( correction_flags & MXFT_AD_MASK_FRAME ) {
			mask_pixel = u16_mask_data_array[i];

			if ( mask_pixel == 0 ) {
				/* If this pixel is masked off, then set
				 * the pixel value to 0 and skip the rest
				 * of this pass through the loop.
				 */

				dbl_image_data_array[i] = 0.0;
				continue;  /* Go back to the top of the loop. */
			}
		}

		if ( correction_flags & MXFT_AD_DARK_CURRENT_FRAME ) {
			dark_current_pixel = flt_dark_current_data_array[i];

			image_pixel = image_pixel - dark_current_pixel;
		}

		if ( correction_flags & MXFT_AD_FLOOD_FIELD_FRAME ) {
			non_uniformity_pixel = flt_non_uniformity_data_array[i];

			image_pixel = image_pixel * non_uniformity_pixel;
		}

		if ( correction_flags & MXFT_AD_BIAS_FRAME ) {
			bias_pixel = u16_bias_data_array[i];

			image_pixel = image_pixel + bias_pixel;
		}

		if ( image_pixel > saturation_pixel_value ) {
			image_pixel = 65535.0;
		} else
		if ( image_pixel < minimum_pixel_value ) {
			image_pixel = minimum_pixel_value;
		}

		dbl_image_data_array[i] = image_pixel;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_rdi_flt_image_correction( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *correction_calc_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *dark_current_frame,
				MX_IMAGE_FRAME *non_uniformity_frame,
				float minimum_pixel_value,
				float saturation_pixel_value,
				unsigned long rdi_flags )
{
	static const char fname[] = "mx_rdi_flt_image_correction()";

	unsigned long correction_flags;
	unsigned long i, num_pixels_per_frame;
	unsigned long num_mask_pixels, num_bias_pixels;
	unsigned long num_dark_current_pixels, num_non_uniformity_pixels;
	float *flt_image_data_array;
	float image_pixel;
	uint16_t *u16_mask_data_array;
	uint16_t *u16_bias_data_array;
	float *flt_dark_current_data_array;
	float *flt_non_uniformity_data_array;
	uint16_t mask_pixel;
	float bias_pixel, dark_current_pixel, non_uniformity_pixel;
	long correction_calc_format, mask_format, bias_format;
	long dark_current_format, non_uniformity_format;
	double image_exposure_time, dark_current_exposure_time;
	mx_bool_type abort_if_different_exposure_times;
	mx_status_type mx_status;

#if MX_RDI_DEBUG_LOOP_TIMING
	MX_HRT_TIMING total_measurement;
	MX_HRT_TIMING loop_measurement;
	MX_HRT_TIMING setup_measurement;
	MX_HRT_TIMING dark_current_measurement;
	MX_HRT_TIMING non_uniformity_measurement;
	MX_HRT_TIMING bias_measurement;
	MX_HRT_TIMING threshold_measurement;
	MX_HRT_TIMING mask_measurement;

	MX_HRT_START( total_measurement );
	MX_HRT_START( setup_measurement );
#endif

	if ( correction_calc_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The correction_calc_frame pointer passed was NULL." );
	}

	correction_flags = ad->correction_flags;

#if 0
	MX_DEBUG(-2,("%s: correction_flags = %#lx", fname, correction_flags));
#endif

	correction_calc_format = MXIF_IMAGE_FORMAT(correction_calc_frame);

	if ( correction_calc_format != MXT_IMAGE_FORMAT_FLOAT ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image correction calculation format %ld is not supported "
		"by this function for area detector '%s'.",
			correction_calc_format, ad->record->name );
	}

	flt_image_data_array = correction_calc_frame->image_data;

	if ( flt_image_data_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The image_data array pointer for the image frame "
		"is NULL for detector '%s'", ad->record->name );
	}

	num_pixels_per_frame = MXIF_ROW_FRAMESIZE(correction_calc_frame)
				* MXIF_COLUMN_FRAMESIZE(correction_calc_frame);

	/*--- Check that the mask frame is set up correctly ---*/

	if ( correction_flags & MXFT_AD_MASK_FRAME ) {

	    if ( mask_frame == NULL ) {
		mx_warning( "Mask correction skipped, since no mask frame "
			"is loaded for detector '%s'.", ad->record->name );

		correction_flags |= (~MXFT_AD_MASK_FRAME);
	    } else {
	    	mask_format = MXIF_IMAGE_FORMAT(mask_frame);

		if ( mask_format != MXT_IMAGE_FORMAT_GREY16 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Mask correction format %ld is not supported "
			"by this function for area detector '%s'.",
				mask_format, ad->record->name );
		} else {
		    num_mask_pixels = MXIF_ROW_FRAMESIZE(mask_frame)
				* MXIF_COLUMN_FRAMESIZE(mask_frame);

		    if ( num_pixels_per_frame != num_mask_pixels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The number of pixels in the image frame (%lu) is not "
			"the same as the number of pixels in the mask frame "
			"(%lu) for area detector '%s'.",
				num_pixels_per_frame, num_mask_pixels,
				ad->record->name );
		    } else {
			u16_mask_data_array = mask_frame->image_data;

			if ( u16_mask_data_array == NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The image_data pointer for the mask frame "
				"is NULL for detector '%s'.", ad->record->name);
			}
		    }
		}
	    }
	}

	/*--- Check that the bias frame is set up correctly ---*/

	if ( correction_flags & MXFT_AD_BIAS_FRAME ) {

	    if ( bias_frame == NULL ) {
		mx_warning( "Bias correction skipped, since no bias frame "
			"is loaded for detector '%s'.", ad->record->name );

		correction_flags |= (~MXFT_AD_BIAS_FRAME);
	    } else {
	    	bias_format = MXIF_IMAGE_FORMAT(bias_frame);

		if ( bias_format != MXT_IMAGE_FORMAT_GREY16 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Bias correction format %ld is not supported "
			"by this function for area detector '%s'.",
				bias_format, ad->record->name );
		} else {
		    num_bias_pixels = MXIF_ROW_FRAMESIZE(bias_frame)
				* MXIF_COLUMN_FRAMESIZE(bias_frame);

		    if ( num_pixels_per_frame != num_bias_pixels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The number of pixels in the image frame (%lu) is not "
			"the same as the number of pixels in the bias frame "
			"(%lu) for area detector '%s'.",
				num_pixels_per_frame, num_bias_pixels,
				ad->record->name );
		    } else {
			u16_bias_data_array = bias_frame->image_data;

			if ( u16_bias_data_array == NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The image_data pointer for the bias frame "
				"is NULL for detector '%s'.", ad->record->name);
			}
		    }
		}
	    }
	}

	/*--- Check that the dark current frame is set up correctly ---*/

	if ( correction_flags & MXFT_AD_DARK_CURRENT_FRAME ) {

	    if ( dark_current_frame == NULL ) {
		mx_warning( "Dark current correction skipped, since "
			"no dark current frame is loaded for detector '%s'.",
			ad->record->name );

		correction_flags |= (~MXFT_AD_DARK_CURRENT_FRAME);
	    } else {

		/* See if we need to abort if the image frame was taken for
		 * a different exposure time than the dark current frame.
		 */

		if (rdi_flags & MXF_RDI_BYPASS_DARK_CURRENT_EXPOSURE_TIME_TEST)
		{
			abort_if_different_exposure_times = FALSE;
		} else {
			abort_if_different_exposure_times = TRUE;
		}

		mx_status = mx_image_get_exposure_time( correction_calc_frame,
							&image_exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_exposure_time( dark_current_frame,
						&dark_current_exposure_time );

		if ( mx_difference( image_exposure_time,
				dark_current_exposure_time ) > 0.001 )
		{
		    if ( abort_if_different_exposure_times ) {

			mx_status = mx_area_detector_stop( ad->record );

			ad->latched_status |= MXSF_AD_EXPOSURE_TIME_CONFLICT;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The image frame for detector '%s' was taken for "
			"a different exposure time (%g seconds) than the "
			"exposure time for the dark current image "
			"(%g seconds).", ad->record->name,
				image_exposure_time,
				dark_current_exposure_time );
		    } else {
			mx_warning(
			"The image frame for detector '%s' was taken for "
			"a different exposure time (%g seconds) than the "
			"exposure time for the dark current image "
			"(%g seconds).", ad->record->name,
				image_exposure_time,
				dark_current_exposure_time );
		    }
		}

		/*---*/

	    	dark_current_format = MXIF_IMAGE_FORMAT(dark_current_frame);

		if ( dark_current_format != MXT_IMAGE_FORMAT_FLOAT ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Dark current correction format %ld is not supported "
			"by this function for area detector '%s'.",
				dark_current_format, ad->record->name );
		} else {
		    num_dark_current_pixels =
				MXIF_ROW_FRAMESIZE(dark_current_frame)
				* MXIF_COLUMN_FRAMESIZE(dark_current_frame);

		    if ( num_pixels_per_frame != num_dark_current_pixels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The number of pixels in the image frame (%lu) "
			"is not " "the same as the number of pixels in "
			"the dark current frame (%lu) for area detector '%s'.",
				num_pixels_per_frame, num_dark_current_pixels,
				ad->record->name );
		    } else {
			flt_dark_current_data_array =
					dark_current_frame->image_data;

			if ( flt_dark_current_data_array == NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The image_data pointer for the dark current "
				"frame is NULL for detector '%s'.",
				ad->record->name);
			}
		    }
		}
	    }
	}

	/*--- Check that the non uniformity frame is set up correctly ---*/

	if ( correction_flags & MXFT_AD_FLOOD_FIELD_FRAME ) {

	    if ( non_uniformity_frame == NULL ) {
		mx_warning( "Non-uniformity correction skipped, since "
			"no non-uniformity frame is loaded for detector '%s'.",
			ad->record->name );

		correction_flags |= (~MXFT_AD_FLOOD_FIELD_FRAME);
	    } else {
	    	non_uniformity_format = MXIF_IMAGE_FORMAT(non_uniformity_frame);

		if ( non_uniformity_format != MXT_IMAGE_FORMAT_FLOAT ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Non-uniformity correction format %ld is not supported "
			"by this function for area detector '%s'.",
				non_uniformity_format, ad->record->name );
		} else {
		    num_non_uniformity_pixels =
				MXIF_ROW_FRAMESIZE(non_uniformity_frame)
				* MXIF_COLUMN_FRAMESIZE(non_uniformity_frame);

		    if ( num_pixels_per_frame != num_non_uniformity_pixels ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The number of pixels in the image frame (%lu) "
			"is not " "the same as the number of pixels in "
			"the dark current frame (%lu) for area detector '%s'.",
			    num_pixels_per_frame, num_non_uniformity_pixels,
			    ad->record->name );
		    } else {
			flt_non_uniformity_data_array =
					non_uniformity_frame->image_data;

			if ( flt_non_uniformity_data_array == NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The image_data pointer for the dark current "
				"frame is NULL for detector '%s'.",
				ad->record->name);
			}
		    }
		}
	    }
	}

	/*----*/

	/*-----------------------------------------------------------------*/

#if MX_RDI_DEBUG_LOOP_TIMING
	MX_HRT_END( setup_measurement );
	MX_HRT_START( loop_measurement );
#endif

	/* Loop over all of the pixels and apply the requested corrections. */

	for ( i = 0; i < num_pixels_per_frame; i++ ) {

		image_pixel = flt_image_data_array[i];

		if ( correction_flags & MXFT_AD_MASK_FRAME ) {
			mask_pixel = u16_mask_data_array[i];

			if ( mask_pixel == 0 ) {
				/* If this pixel is masked off, then set
				 * the pixel value to 0 and skip the rest
				 * of this pass through the loop.
				 */

				flt_image_data_array[i] = 0.0;
				continue;  /* Go back to the top of the loop. */
			}
		}

		if ( correction_flags & MXFT_AD_DARK_CURRENT_FRAME ) {
			dark_current_pixel = flt_dark_current_data_array[i];

			image_pixel = image_pixel - dark_current_pixel;
		}

		if ( correction_flags & MXFT_AD_FLOOD_FIELD_FRAME ) {
			non_uniformity_pixel = flt_non_uniformity_data_array[i];

			image_pixel = image_pixel * non_uniformity_pixel;
		}

		if ( correction_flags & MXFT_AD_BIAS_FRAME ) {
			bias_pixel = u16_bias_data_array[i];

			image_pixel = image_pixel + bias_pixel;
		}

		if ( image_pixel > saturation_pixel_value ) {
			image_pixel = 65535.0;
		} else
		if ( image_pixel < minimum_pixel_value ) {
			image_pixel = minimum_pixel_value;
		}

		flt_image_data_array[i] = image_pixel;
	}

#if MX_RDI_DEBUG_LOOP_TIMING
	MX_HRT_END( loop_measurement );
	MX_HRT_END( total_measurement );

	MX_HRT_RESULTS( setup_measurement, fname, "setup" );
	MX_HRT_RESULTS( loop_measurement,  fname, "loop"  );
	MX_HRT_RESULTS( total_measurement, fname, "total" );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_rdi_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_rdi_load_frame()";

	MX_IMAGE_FRAME *non_uniformity_frame;
	uint32_t non_uniformity_format;
	float *non_uniformity_array;
	double sum, mean;
	double bytes_per_pixel_dbl;
	unsigned long bytes_per_pixel;
	unsigned long i, image_size_in_bytes, image_size_in_pixels;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	/* Initially, we use the default load frame function. */

	mx_status = mx_area_detector_default_load_frame( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the frame we just loaded was not a non-uniformity frame,
	 * then we are done.
	 */

	if ( ad->load_frame != MXFT_AD_FLOOD_FIELD_FRAME )
		return MX_SUCCESSFUL_RESULT;

	MX_DEBUG(-2,("%s: analyzing non-uniformity frame for detector '%s'.",
		fname, ad->record->name ));

	/* Old RDI non-uniformity frames had an average value around 10000,
	 * while new non-uniformity frames have an average value of 1.0.
	 *
	 * In the MX correction code, it is more convenient that the average
	 * value is around 1, since that reduces the number of multiplications
	 * or divisions that we have to make in the body of the correction 
	 * loop.  Therefore, we compute the mean of the values in the 
	 * non-uniformity frame that we just loaded.  If the mean is greater
	 * than or equal to 100.0, we assume that this is an old style
	 * non-uniformity frame and divide all its values by 10000.0.
	 * Otherwise, we leave the non-uniformity frame alone.
	 */

	non_uniformity_frame = ad->flood_field_frame;

	if ( non_uniformity_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The non-uniformity frame pointer for detector '%s' is NULL.",
			ad->record->name );
	}

	non_uniformity_format = MXIF_IMAGE_FORMAT( non_uniformity_frame );

	MX_DEBUG(-2,("%s: non_uniformity_format = %lu",
		fname, (unsigned long) non_uniformity_format ));

	if ( non_uniformity_format != MXT_IMAGE_FORMAT_FLOAT ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The non-uniformity frame that we just loaded for "
		"detector '%s' is _not_ a 32-bit float image.  "
		"Instead, it is of type %lu.",
			ad->record->name,
			(unsigned long) non_uniformity_format );
	}

	non_uniformity_array = (float *) non_uniformity_frame->image_data;

	if ( non_uniformity_array == (float *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The non-uniformity data array for detector '%s' is NULL.",
			ad->record->name );
	}

	image_size_in_bytes = non_uniformity_frame->image_length;

	bytes_per_pixel_dbl = MXIF_BYTES_PER_PIXEL(non_uniformity_frame);

	MX_DEBUG(-2,("%s: non-uniformity image bytes per pixel = %g",
		fname, bytes_per_pixel_dbl));

	bytes_per_pixel = mx_round( bytes_per_pixel_dbl );

	image_size_in_pixels = mx_divide_safely( image_size_in_bytes,
							bytes_per_pixel );

	MX_DEBUG(-2,("%s: non-uniformity image size = %lu pixels",
		fname, image_size_in_pixels));

	/* Compute the mean value of the non-uniformity pixels. */

	sum = 0.0;

	for ( i = 0; i < image_size_in_pixels; i++ ) {
		sum += non_uniformity_array[i];
	}

	mean = sum / image_size_in_pixels;

	MX_DEBUG(-2,("%s: non-uniformity image mean value = %g", fname, mean));

	/* If the mean is greater than or equal to 100.0, then we must
	 * renormalize the value by dividing it by 10000.0.
	 */

	if ( mean >= 100.0 ) {

		MX_DEBUG(-2,("%s: renormalizing non-uniformity frame.", fname));

		for ( i = 0; i < image_size_in_pixels; i++ ) {
			non_uniformity_array[i] /= 10000.0;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

