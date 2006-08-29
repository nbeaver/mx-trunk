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

/* Status bit definitions for the 'status' field. */

#define MXSF_AD_IS_BUSY		0x1

typedef struct {
	MX_RECORD *record;

	long parameter_type;

	long framesize[2];
	long image_format;
	long trigger_mode;

	mx_bool_type busy;
	unsigned long status;

	MX_SEQUENCE_INFO sequence_info;
} MX_AREA_DETECTOR;

#define MXLV_AD_FRAMESIZE			12001
#define MXLV_AD_FORMAT				12002
#define MXLV_AD_TRIGGER_MODE			12004
#define MXLV_AD_BUSY				12005
#define MXLV_AD_STATUS				12006
#define MXLV_AD_SEQUENCE_TYPE			12007
#define MXLV_AD_NUM_SEQUENCE_PARAMETERS		12008
#define MXLV_AD_SEQUENCE_PARAMETERS		12009

#define MX_AREA_DETECTOR_STANDARD_FIELDS \
  {MXLV_AD_FRAMESIZE, -1, "framesize", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, framesize), \
	{sizeof(long)}, NULL, 0}, \
  \
  {MXLV_AD_FORMAT, -1, "image_format", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_format), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_TRIGGER_MODE, -1, "trigger_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, trigger_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, busy), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, status), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SEQUENCE_TYPE, -1, "sequence_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, sequence_info.sequence_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_NUM_SEQUENCE_PARAMETERS, -1, "num_sequence_parameters", \
						MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
	    offsetof(MX_AREA_DETECTOR, sequence_info.num_sequence_parameters), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SEQUENCE_PARAMETERS, -1, "sequence_parameters", \
			MXFT_DOUBLE, NULL, 1, {MXU_MAX_SEQUENCE_PARAMETERS}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, sequence_info.sequence_parameters), \
	{0}, NULL, 0}


typedef struct {
        mx_status_type ( *arm ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *trigger ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *stop ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *abort ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *busy ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_status ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_frame ) ( MX_AREA_DETECTOR *ad,
                                        MX_IMAGE_FRAME **frame );
        mx_status_type ( *get_sequence ) ( MX_AREA_DETECTOR *ad,
                                        MX_IMAGE_SEQUENCE **sequence );
        mx_status_type ( *get_parameter ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *set_parameter ) ( MX_AREA_DETECTOR *ad );
} MX_AREA_DETECTOR_FUNCTION_LIST;

MX_API mx_status_type mx_area_detector_get_pointers( MX_RECORD *record,
                                        MX_AREA_DETECTOR **ad,
                                MX_AREA_DETECTOR_FUNCTION_LIST **flist_ptr,
                                        const char *calling_fname );

/*---*/


MX_API mx_status_type mx_area_detector_get_image_format( MX_RECORD *ad_record,
						long *format );

MX_API mx_status_type mx_area_detector_get_framesize( MX_RECORD *ad_record,
						long *x_framesize,
						long *y_framesize );

MX_API mx_status_type mx_area_detector_set_framesize( MX_RECORD *ad_record,
						long x_framesize,
						long y_framesize );

MX_API mx_status_type mx_area_detector_get_binsize( MX_RECORD *ad_record,
						long *x_binsize,
						long *y_binsize );

MX_API mx_status_type mx_area_detector_set_binsize( MX_RECORD *ad_record,
						long x_binsize,
						long y_binsize );

MX_API mx_status_type mx_area_detector_set_subframe_size( MX_RECORD *ad_record,
						long num_lines );

/*---*/

MX_API mx_status_type mx_area_detector_set_exposure_time( MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_continuous_mode(MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_sequence( MX_RECORD *ad_record,
					MX_SEQUENCE_INFO *sequence_info );

MX_API mx_status_type mx_area_detector_set_trigger_mode( MX_RECORD *ad_record,
							long trigger_mode );

MX_API mx_status_type mx_area_detector_arm( MX_RECORD *ad_record );

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
						MX_IMAGE_SEQUENCE *sequence,
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

MX_API mx_status_type mx_area_detector_default_get_parameter_handler(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_set_parameter_handler(
                                                MX_AREA_DETECTOR *ad );

/*---*/

MX_API mx_status_type mx_area_detector_correct_frame( MX_RECORD *ad_record,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame,
					MX_IMAGE_FRAME *flood_field_frame );


#endif /* __MX_AREA_DETECTOR_H__ */

