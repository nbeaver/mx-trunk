/*
 * Name:    mx_area_detector_rdi.h
 *
 * Purpose: Image correction functions for RDI area detectors.
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

#ifndef __MX_AREA_DETECTOR_RDI_H__
#define __MX_AREA_DETECTOR_RDI_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Values for the 'rdi_flags' argument to these functions. */

#define MXF_RDI_USE_DOUBLE_PRECISION_CORRECTION		0x1

#define MXF_RDI_BYPASS_DARK_CURRENT_EXPOSURE_TIME_TEST	0x2

/*---*/

MX_API mx_status_type mx_rdi_correct_frame( MX_AREA_DETECTOR *ad,
					double minimum_pixel_value,
					double saturation_pixel_value,
					unsigned long rdi_flags );

MX_API mx_status_type mx_rdi_dbl_image_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *correction_calc_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame,
					MX_IMAGE_FRAME *non_uniformity_frame,
					double minimum_pixel_value,
					double saturation_pixel_value,
					unsigned long rdi_flags );

#ifdef __cplusplus
}
#endif

#endif /* __MX_AREA_DETECTOR_RDI_H__ */

