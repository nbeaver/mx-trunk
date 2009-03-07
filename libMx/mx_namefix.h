/*
 * Name:     mx_namefix.h
 *
 * Purpose:  The linkers for some operating systems can only handle up to
 *           31 characters in externally visible names.  In most cases, we
 *           have defined names that are unique in the first 31 characters,
 *           but there are a few cases where doing this would result in the
 *           use of names that do not fit into the patterns set by other
 *           function names.  For these cases, we remap the name in the
 *           preprocessor to a unique name.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2007-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_NAMEFIX_H__
#define __MX_NAMEFIX_H__

#if defined(OS_VMS)

/* mx_area_detector_get_bytes_per_pixel() collides with
 * mx_area_detector_get_bytes_per_frame().
 */

#define mx_area_detector_get_bytes_per_pixel      mx_ad_get_bytes_per_pixel

/* mx_area_detector_get_correction_frame() collides with
 * mx_area_detector_get_correction_flags().
 */

#define mx_area_detector_get_correction_frame     mx_ad_get_correction_frame

/* mx_area_detector_get_maximum_frame_number() collides with
 * mx_area_detector_get_maximum_framesize().
 */

#define mx_area_detector_get_maximum_frame_number mx_ad_get_maximum_frame_number

/* mxd_network_area_detector_trigger_exposure() collides with
 * mxd_network_area_detector_trigger().
 */

#define mxd_network_area_detector_trigger_exposure \
						mxd_network_ad_trigger_exposure

/* mx_network_field_get_attribute_number() collides with
 * mx_network_field_get_attribute_by_name().
 */

#define mx_network_field_get_attribute_number     mx_nf_get_attribute_number

/* mx_waveform_output_get_trigger_repeat() collides with
 * mx_waveform_output_get_trigger_mode().
 */

#define mx_waveform_output_get_trigger_repeat     mx_wvo_get_trigger_repeat

/* mx_waveform_output_set_trigger_repeat() collides with
 * mx_waveform_output_set_trigger_mode().
 */

#define mx_waveform_output_set_trigger_repeat     mx_wvo_set_trigger_repeat

/* mxs_area_detector_scan_default_construct_next_datafile_name() collides with
 * mxs_area_detector_scan_default_initialize_datafile_naming().
 */

#define mxs_area_detector_scan_default_initialize_datafile_naming \
				mxs_ad_default_initialize_datafile_naming

#endif /* OS_VMS */

#endif /* __MX_NAMEFIX_H__ */

