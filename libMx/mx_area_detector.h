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
	long frame_number;

	long maximum_framesize[2];
	long framesize[2];
	long binsize[2];
	char image_format_name[MXU_IMAGE_FORMAT_NAME_LENGTH+1];
	long image_format;
	long pixel_order;
	long trigger_mode;
	long bytes_per_frame;

	mx_bool_type arm;
	mx_bool_type trigger;
	mx_bool_type stop;
	mx_bool_type abort;
	mx_bool_type busy;
	unsigned long status;

	long maximum_num_rois;
	long current_num_rois;
	unsigned long **roi_array;

	unsigned long roi_number;
	unsigned long roi[4];
	long roi_bytes_per_frame;

	MX_SEQUENCE_PARAMETERS sequence_parameters;

	/* Note: The get_frame() method expects to read the new frame
	 * into the 'frame' MX_IMAGE_FRAME structure.
	 */

	long get_frame;
	MX_IMAGE_FRAME *frame;

	/* 'frame_buffer' is used to provide a place for MX event handlers
	 * to find the contents of the most recently taken frame.  It must
	 * only be modified by mx_area_dtector_process_function() in
	 * libMx/pr_area_detector.c.  No other functions should modify it.
	 */

	char *frame_buffer;

	/* The following are the analogous data structures for ROI frames. */

	long get_roi_frame;
	MX_IMAGE_FRAME *roi_frame;

	char *roi_frame_buffer;
} MX_AREA_DETECTOR;

#define MXLV_AD_MAXIMUM_FRAMESIZE		12001
#define MXLV_AD_FRAMESIZE			12002
#define MXLV_AD_BINSIZE				12003
#define MXLV_AD_FORMAT_NAME			12004
#define MXLV_AD_FORMAT				12005
#define MXLV_AD_PIXEL_ORDER			12006
#define MXLV_AD_TRIGGER_MODE			12007
#define MXLV_AD_BYTES_PER_FRAME			12008
#define MXLV_AD_ARM				12009
#define MXLV_AD_TRIGGER				12010
#define MXLV_AD_STOP				12011
#define MXLV_AD_ABORT				12012
#define MXLV_AD_BUSY				12013
#define MXLV_AD_STATUS				12014
#define MXLV_AD_MAXIMUM_NUM_ROIS		12014
#define MXLV_AD_CURRENT_NUM_ROIS		12015
#define MXLV_AD_ROI_ARRAY			12016
#define MXLV_AD_ROI_NUMBER			12017
#define MXLV_AD_ROI				12018
#define MXLV_AD_ROI_BYTES_PER_FRAME		12019
#define MXLV_AD_SEQUENCE_TYPE			12020
#define MXLV_AD_NUM_SEQUENCE_PARAMETERS		12021
#define MXLV_AD_SEQUENCE_PARAMETER_ARRAY	12022
#define MXLV_AD_GET_FRAME			12023
#define MXLV_AD_FRAME_BUFFER			12024
#define MXLV_AD_GET_ROI_FRAME			12025
#define MXLV_AD_ROI_FRAME_BUFFER		12026

#define MX_AREA_DETECTOR_STANDARD_FIELDS \
  {MXLV_AD_MAXIMUM_FRAMESIZE, -1, "maximum_framesize", \
					MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, maximum_framesize), \
	{sizeof(long)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_FRAMESIZE, -1, "framesize", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, framesize), \
	{sizeof(long)}, NULL, 0}, \
  \
  {MXLV_AD_BINSIZE, -1, "binsize", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, binsize), \
	{sizeof(long)}, NULL, 0}, \
  \
  {MXLV_AD_FORMAT_NAME, -1, "image_format_name", MXFT_STRING, \
	  	NULL, 1, {MXU_IMAGE_FORMAT_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_format_name),\
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_FORMAT, -1, "image_format", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_format), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_PIXEL_ORDER, -1, "pixel_order", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, pixel_order), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_TRIGGER_MODE, -1, "trigger_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, trigger_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_BYTES_PER_FRAME, -1, "bytes_per_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, bytes_per_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_ARM, -1, "arm", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, arm), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_TRIGGER, -1, "trigger", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, trigger), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, stop), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_ABORT, -1, "abort", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, abort), \
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
  {MXLV_AD_MAXIMUM_NUM_ROIS, -1, "maximum_num_rois", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, maximum_num_rois), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {MXLV_AD_CURRENT_NUM_ROIS, -1, "current_num_rois", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, current_num_rois), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_AD_ROI_ARRAY, -1, "roi_array", \
			MXFT_ULONG, NULL, 2, {MXU_VARARGS_LENGTH, 4}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, roi_array), \
	{sizeof(unsigned long), sizeof(unsigned long *)}, \
					NULL, MXFF_VARARGS}, \
  \
  {MXLV_AD_ROI_NUMBER, -1, "roi_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, roi_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_ROI, -1, "roi", MXFT_ULONG, NULL, 1, {4}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, roi), \
	{sizeof(unsigned long)}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_AD_ROI_BYTES_PER_FRAME, -1, "roi_bytes_per_frame", \
					MXFT_LONG, NULL, 1, {4}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, roi_bytes_per_frame), \
	{sizeof(unsigned long)}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_AD_SEQUENCE_TYPE, -1, "sequence_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, sequence_parameters.sequence_type), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_NUM_SEQUENCE_PARAMETERS, -1, "num_sequence_parameters", \
						MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
	    offsetof(MX_AREA_DETECTOR, sequence_parameters.num_parameters), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SEQUENCE_PARAMETER_ARRAY, -1, "sequence_parameter_array", \
			MXFT_DOUBLE, NULL, 1, {MXU_MAX_SEQUENCE_PARAMETERS}, \
	MXF_REC_CLASS_STRUCT, \
	    offsetof(MX_AREA_DETECTOR, sequence_parameters.parameter_array), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_AD_GET_FRAME, -1, "get_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, get_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_FRAME_BUFFER, -1, "frame_buffer", MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, frame_buffer), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}, \
  \
  {MXLV_AD_GET_ROI_FRAME, -1, "get_roi_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, get_roi_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_ROI_FRAME_BUFFER, -1, "roi_frame_buffer", MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, roi_frame_buffer), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}

typedef struct {
        mx_status_type ( *arm ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *trigger ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *stop ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *abort ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *busy ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_status ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_frame ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_sequence ) ( MX_AREA_DETECTOR *ad,
                                        MX_IMAGE_SEQUENCE **sequence );
        mx_status_type ( *get_roi_frame ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_parameter ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *set_parameter ) ( MX_AREA_DETECTOR *ad );
} MX_AREA_DETECTOR_FUNCTION_LIST;

MX_API mx_status_type mx_area_detector_get_pointers( MX_RECORD *record,
                                        MX_AREA_DETECTOR **ad,
                                MX_AREA_DETECTOR_FUNCTION_LIST **flist_ptr,
                                        const char *calling_fname );

MX_API mx_status_type mx_area_detector_finish_record_initialization(
						MX_RECORD *record );

/*---*/


MX_API mx_status_type mx_area_detector_get_image_format( MX_RECORD *ad_record,
						long *format );

MX_API mx_status_type mx_area_detector_set_image_format( MX_RECORD *ad_record,
						long format );

MX_API mx_status_type mx_area_detector_get_maximum_framesize(
						MX_RECORD *ad_record,
						long *maximum_x_framesize,
						long *maximum_y_framesize );

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

MX_API mx_status_type mx_area_detector_get_roi( MX_RECORD *ad_record,
						long roi_number,
						long *x_minimum,
						long *x_maximum,
						long *y_minimum,
						long *y_maximum );

MX_API mx_status_type mx_area_detector_set_roi( MX_RECORD *ad_record,
						long roi_number,
						long x_minimum,
						long x_maximum,
						long y_minimum,
						long y_maximum );

MX_API mx_status_type mx_area_detector_set_subframe_size( MX_RECORD *ad_record,
						long num_lines );

MX_API mx_status_type mx_area_detector_get_bytes_per_frame( MX_RECORD *record,
						long *bytes_per_frame );

/*---*/

MX_API mx_status_type mx_area_detector_set_exposure_time( MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_continuous_mode(MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_sequence_parameters(
				MX_RECORD *ad_record,
				MX_SEQUENCE_PARAMETERS *sequence_parameters );

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
						long frame_number,
						MX_IMAGE_FRAME **frame );

MX_API mx_status_type mx_area_detector_get_sequence( MX_RECORD *ad_record,
						MX_IMAGE_SEQUENCE **sequence );

MX_API mx_status_type mx_area_detector_get_frame_from_sequence(
						MX_IMAGE_SEQUENCE *sequence,
						long frame_number,
						MX_IMAGE_FRAME **image_frame );

MX_API mx_status_type mx_area_detector_get_roi_frame( MX_RECORD *ad_record,
						MX_IMAGE_FRAME *frame,
						long roi_number,
						MX_IMAGE_FRAME **roi_frame );

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

