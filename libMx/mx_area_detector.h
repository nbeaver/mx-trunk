/*
 * Name:    mx_area_detector.h
 *
 * Purpose: Functions for communicating with area detectors.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_AREA_DETECTOR_H__
#define __MX_AREA_DETECTOR_H__

MX_API mx_status_type mx_area_detector_get_image_format( MX_RECORD *ad_record,
					unsigned long *resolution_in_bits,
					unsigned long *format );

MX_API mx_status_type mx_area_detector_get_framesize( MX_RECORD *ad_record,
						unsigned long *x_framesize,
						unsigned long *y_framesize );

MX_API mx_status_type mx_area_detector_set_framesize( MX_RECORD *ad_record,
						unsigned long x_framesize,
						unsigned long y_framesize );

MX_API mx_status_type mx_area_detector_get_binsize( MX_RECORD *ad_record,
						unsigned long *x_binsize,
						unsigned long *y_binsize );

MX_API mx_status_type mx_area_detector_set_subframe_size( MX_RECORD *ad_record,
						unsigned long num_lines );

/*---*/

MX_API mx_status_type mx_area_detector_set_exposure_time( MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_sequence( MX_RECORD *ad_record,
					MX_SEQUENCE_INFO *sequence_info );

MX_API mx_status_type mx_area_detector_set_continuous_mode(MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_arm( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_enable_external_trigger(
							MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_disable_external_trigger(
							MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_trigger( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_start( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_stop( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_abort( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_is_busy( MX_RECORD *ad_record,
						mx_bool_type *busy );

MX_API mx_status_type mx_area_detector_get_status( MX_RECORD *ad_record,
						long *last_frame_number,
						unsigned long *status_flags );

/*---*/

MX_API mx_status_type mx_area_detector_get_frame( MX_RECORD *ad_record,
						MX_IMAGE_FRAME **frame );

MX_API mx_status_type mx_area_detector_get_sequence( MX_RECORD *ad_record,
						MX_IMAGE_SEQUENCE **sequence );

MX_API mx_status_type mx_area_detector_get_frame_from_sequence(
						MX_RECORD *ad_record,
						long frame_number,
						MX_IMAGE_FRAME **image_frame );

MX_API mx_status_type mx_area_detector_read_1d_pixel_array(
						MX_IMAGE_FRAME *frame,
						long pixel_datatype,
						void *destination_pixel_array,
						size_t max_array_bytes,
						size_t *num_bytes_copied );

MX_API mx_status_type mx_area_detector_read_1d_pixel_sequence(
						MX_IMAGE_SEQUENCE *sequence,
						long pixel_datatype,
						void *destination_pixel_array,
						size_t max_array_bytes,
						size_t *num_bytes_copied );

/*---*/

MX_API mx_status_type mx_area_detector_correct_frame( MX_RECORD *ad_record,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame,
					MX_IMAGE_FRAME *flood_field_frame );


/*---*/

MX_API mx_status_type mx_area_detector_read_image_file( MX_IMAGE_FRAME **frame,
						unsigned long datafile_type,
						char *datafile_name );

MX_API mx_status_type mx_area_detector_write_image_file( MX_IMAGE_FRAME *frame,
						unsigned long datafile_type,
						char *datafile_name );

#endif /* __MX_AREA_DETECTOR_H__ */

