/*
 * Name:    mx_area_detector.h
 *
 * Purpose: Functions for communicating with area detectors.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_AREA_DETECTOR_H__
#define __MX_AREA_DETECTOR_H__

#include "mx_callback.h"
#include "mx_namefix.h"

#define MXU_AD_EXTENDED_STATUS_STRING_LENGTH	40

/* Status bit definitions for the 'status' field. */

#define MXSF_AD_IS_BUSY					0x1
#define MXSF_AD_CORRECTION_IN_PROGRESS			0x2
#define MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS	0x4

/* Frame types for the 'correct_frame', 'transfer_frame', 'load_frame',
 * 'save_frame', and 'copy_frame' fields.
 */

#define MXFT_AD_IMAGE_FRAME		0x1
#define MXFT_AD_MASK_FRAME		0x2
#define MXFT_AD_BIAS_FRAME		0x4
#define MXFT_AD_DARK_CURRENT_FRAME	0x8
#define MXFT_AD_FLOOD_FIELD_FRAME	0x10

#define MXFT_AD_ALL			0xffffffff

typedef struct {
	struct mx_area_detector_type *area_detector;
	MX_IMAGE_FRAME *destination_frame;
	uint16_t *destination_array;
	long num_frames_summed;
	double *sum_array;
	long num_unread_frames;
	long old_last_frame_number;
	long old_total_num_frames;
	unsigned long old_status;
	unsigned long desired_correction_flags;
	MX_CALLBACK_MESSAGE *callback_message;
} MX_AREA_DETECTOR_CORRECTION_MEASUREMENT;

typedef struct mx_area_detector_type {
	MX_RECORD *record;

	long ad_state;

	long parameter_type;
	long frame_number;

	long maximum_framesize[2];
	long framesize[2];
	long binsize[2];
	char image_format_name[MXU_IMAGE_FORMAT_NAME_LENGTH+1];
	long image_format;
	long byte_order;
	long trigger_mode;
	long header_length;
	long bytes_per_frame;
	double bytes_per_pixel;
	long bits_per_pixel;

	mx_bool_type arm;
	mx_bool_type trigger;
	mx_bool_type stop;
	mx_bool_type abort;
	mx_bool_type busy;
	unsigned long maximum_frame_number;
	long last_frame_number;
	long total_num_frames;
	unsigned long status;
	char extended_status[ MXU_AD_EXTENDED_STATUS_STRING_LENGTH + 1 ];

	long subframe_size;	/* Not all detectors support this. */

	long maximum_num_rois;
	long current_num_rois;
	unsigned long **roi_array;

	unsigned long roi_number;
	unsigned long roi[4];
	long roi_bytes_per_frame;

	/* The following are used to store ROI frames. */

	long get_roi_frame;
	MX_IMAGE_FRAME *roi_frame;

	char *roi_frame_buffer;

	double detector_readout_time;
	double total_sequence_time;

	/* 'sequence_parameters' contains information like the type of the
	 * sequence, the number of frames in the sequence, and sequence
	 * parameters like the exposure time per frame, and the interval
	 * between frames.
	 */

	MX_SEQUENCE_PARAMETERS sequence_parameters;

	/* Note: The readout_frame() method expects to read the new frame
	 * into the 'image_frame' MX_IMAGE_FRAME structure.
	 */

	long readout_frame;
	MX_IMAGE_FRAME *image_frame;

	/* 'image_frame_buffer' is used to provide a place for MX event
	 * handlers to find the contents of the most recently taken frame.
	 * It must only be modified by mx_area_detector_process_function()
	 * in libMx/pr_area_detector.c.  No other functions should modify it.
	 */

	char *image_frame_buffer;

	/* The individual bits in 'correction_flags' determine which
	 * corrections are made.  The 'correct_frame' field tells the
	 * software to execute the corrections.
	 */

	mx_bool_type correct_frame;
	unsigned long correction_flags;

	/* 'transfer_frame' tells the server to send one of the frames
	 * to the caller.
	 */

	long transfer_frame;
	MX_IMAGE_FRAME *transfer_destination_frame;

	/* 'frame_file_format' is the file format that is used for load and
	 * save operations below.
	 */

	long frame_file_format;

	/* 'load_frame' and 'save_frame' are used to tell the software
	 * what kind of frame to load or save.  'frame_filename' specifies
	 * the name of the file to load or save.  The specified file _must_
	 * be on the computer that has the frame buffer.
	 */

	long load_frame;
	long save_frame;
	char frame_filename[MXU_FILENAME_LENGTH+1];

	/* The following field is used to identify the source and destination
	 * frames in a copy operation.  The valid values are found at the
	 * top of this file and include MXFT_AD_IMAGE_FRAME, MXFT_AD_MASK_FRAME,
	 * and so forth.
	 */

	long copy_frame[2];

	/* The following fields are used for measuring dark current and
	 * flood field image frames.
	 */

	MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *correction_measurement;

	long correction_measurement_type;
	double correction_measurement_time;
	long num_correction_measurements;

	/* dezinger_threshold is use to determine which pixels are to be
	 * thrown away during dezingering.
	 */

	double dezinger_threshold;

	/* The following are the image frames and frame buffer pointers
	 * used for image correction.
	 */

	MX_IMAGE_FRAME *mask_frame;
	char *mask_frame_buffer;
	char mask_filename[MXU_FILENAME_LENGTH+1];

	MX_IMAGE_FRAME *bias_frame;
	char *bias_frame_buffer;
	char bias_filename[MXU_FILENAME_LENGTH+1];

	mx_bool_type use_scaled_dark_current;
	double dark_current_exposure_time;

	MX_IMAGE_FRAME *dark_current_frame;
	char *dark_current_frame_buffer;
	char dark_current_filename[MXU_FILENAME_LENGTH+1];

	double flood_field_average_intensity;

	MX_IMAGE_FRAME *flood_field_frame;
	char *flood_field_frame_buffer;
	char flood_field_filename[MXU_FILENAME_LENGTH+1];

} MX_AREA_DETECTOR;

/* Warning: Do not rely on the following numbers remaining the same
 * between releases of MX.
 */

#define MXLV_AD_MAXIMUM_FRAMESIZE		12001
#define MXLV_AD_FRAMESIZE			12002
#define MXLV_AD_BINSIZE				12003
#define MXLV_AD_IMAGE_FORMAT_NAME		12004
#define MXLV_AD_IMAGE_FORMAT			12005
#define MXLV_AD_BYTE_ORDER			12006
#define MXLV_AD_TRIGGER_MODE			12007
#define MXLV_AD_BYTES_PER_FRAME			12008
#define MXLV_AD_BYTES_PER_PIXEL			12009
#define MXLV_AD_BITS_PER_PIXEL			12010
#define MXLV_AD_ARM				12011
#define MXLV_AD_TRIGGER				12012
#define MXLV_AD_STOP				12013
#define MXLV_AD_ABORT				12014
#define MXLV_AD_MAXIMUM_FRAME_NUMBER		12015
#define MXLV_AD_LAST_FRAME_NUMBER		12016
#define MXLV_AD_TOTAL_NUM_FRAMES		12017
#define MXLV_AD_STATUS				12018
#define MXLV_AD_EXTENDED_STATUS			12019
#define MXLV_AD_MAXIMUM_NUM_ROIS		12020
#define MXLV_AD_CURRENT_NUM_ROIS		12021
#define MXLV_AD_ROI_ARRAY			12022
#define MXLV_AD_ROI_NUMBER			12023
#define MXLV_AD_ROI				12024
#define MXLV_AD_ROI_BYTES_PER_FRAME		12025
#define MXLV_AD_GET_ROI_FRAME			12026
#define MXLV_AD_ROI_FRAME_BUFFER		12027
#define MXLV_AD_SEQUENCE_TYPE			12028
#define MXLV_AD_NUM_SEQUENCE_PARAMETERS		12029
#define MXLV_AD_SEQUENCE_PARAMETER_ARRAY	12030
#define MXLV_AD_READOUT_FRAME			12031
#define MXLV_AD_IMAGE_FRAME_BUFFER		12032
#define MXLV_AD_CORRECT_FRAME			12033
#define MXLV_AD_CORRECTION_FLAGS		12034
#define MXLV_AD_TRANSFER_FRAME			12035
#define MXLV_AD_LOAD_FRAME			12036
#define MXLV_AD_SAVE_FRAME			12037
#define MXLV_AD_FRAME_FILENAME			12038
#define MXLV_AD_COPY_FRAME			12039
#define MXLV_AD_DETECTOR_READOUT_TIME		12040
#define MXLV_AD_TOTAL_SEQUENCE_TIME		12041

#define MXLV_AD_CORRECTION_MEASUREMENT_TYPE	12044
#define MXLV_AD_CORRECTION_MEASUREMENT_TIME	12045
#define MXLV_AD_NUM_CORRECTION_MEASUREMENTS	12046
#define MXLV_AD_DEZINGER_THRESHOLD		12047
#define MXLV_AD_USE_SCALED_DARK_CURRENT		12048

#define MXLV_AD_MASK_FILENAME			12101
#define MXLV_AD_BIAS_FILENAME			12102
#define MXLV_AD_DARK_CURRENT_FILENAME		12103
#define MXLV_AD_FLOOD_FIELD_FILENAME		12104

#define MXLV_AD_SUBFRAME_SIZE			12201

#define MX_AREA_DETECTOR_STANDARD_FIELDS \
  {MXLV_AD_MAXIMUM_FRAMESIZE, -1, "maximum_framesize", \
					MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, maximum_framesize), \
	{sizeof(long)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_FRAMESIZE, -1, "framesize", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, framesize), \
	{sizeof(long)}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_AD_BINSIZE, -1, "binsize", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, binsize), \
	{sizeof(long)}, NULL, 0}, \
  \
  {MXLV_AD_IMAGE_FORMAT_NAME, -1, "image_format_name", MXFT_STRING, \
	  	NULL, 1, {MXU_IMAGE_FORMAT_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_format_name),\
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_IMAGE_FORMAT, -1, "image_format", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_format), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_BYTE_ORDER, -1, "byte_order", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, byte_order), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_TRIGGER_MODE, -1, "trigger_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, trigger_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_BYTES_PER_FRAME, -1, "bytes_per_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, bytes_per_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_BYTES_PER_PIXEL, -1, "bytes_per_pixel", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, bytes_per_pixel), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_BITS_PER_PIXEL, -1, "bits_per_pixel", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, bits_per_pixel), \
	{0}, NULL, MXFF_READ_ONLY}, \
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
  {MXLV_AD_MAXIMUM_FRAME_NUMBER, -1, "maximum_frame_number", \
  						MXFT_LONG, NULL, 0, {0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, maximum_frame_number),\
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_LAST_FRAME_NUMBER, -1, "last_frame_number", MXFT_LONG, NULL, 0, {0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, last_frame_number), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_TOTAL_NUM_FRAMES, -1, "total_num_frames", MXFT_LONG, NULL, 0, {0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, total_num_frames), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, status), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_EXTENDED_STATUS, -1, "extended_status", MXFT_STRING, \
			NULL, 1, {MXU_AD_EXTENDED_STATUS_STRING_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, extended_status), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
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
  {MXLV_AD_GET_ROI_FRAME, -1, "get_roi_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, get_roi_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_ROI_FRAME_BUFFER, -1, "roi_frame_buffer", MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, roi_frame_buffer), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}, \
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
  {MXLV_AD_READOUT_FRAME, -1, "readout_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, readout_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_IMAGE_FRAME_BUFFER, -1, "image_frame_buffer", \
						MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_frame_buffer),\
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}, \
  \
  {MXLV_AD_CORRECT_FRAME, -1, "correct_frame", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, correct_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_CORRECTION_FLAGS, -1, "correction_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, correction_flags), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_TRANSFER_FRAME, -1, "transfer_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, transfer_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_LOAD_FRAME, -1, "load_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, load_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SAVE_FRAME, -1, "save_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, save_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_FRAME_FILENAME, -1, "frame_filename", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, frame_filename), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_COPY_FRAME, -1, "copy_frame", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, copy_frame), \
	{sizeof(long)}, NULL, 0}, \
  \
  {MXLV_AD_DETECTOR_READOUT_TIME, -1, \
		"detector_readout_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, detector_readout_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_TOTAL_SEQUENCE_TIME, -1, \
		"total_sequence_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, total_sequence_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_CORRECTION_MEASUREMENT_TIME, -1, \
  		"correction_measurement_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_measurement_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_NUM_CORRECTION_MEASUREMENTS, -1, \
  		"num_correction_measurements", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, num_correction_measurements), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_CORRECTION_MEASUREMENT_TYPE, -1, \
  		"correction_measurement_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_measurement_type), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "mask_frame_buffer", MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, mask_frame_buffer),\
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}, \
  \
  {-1, -1, "bias_frame_buffer", MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, bias_frame_buffer),\
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}, \
  \
  {MXLV_AD_DEZINGER_THRESHOLD, -1, "dezinger_threshold", \
  						MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, dezinger_threshold), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_USE_SCALED_DARK_CURRENT, -1, "use_scaled_dark_current", \
  						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR, use_scaled_dark_current), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "dark_current_exposure_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, dark_current_exposure_time), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "dark_current_frame_buffer", MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, dark_current_frame_buffer),\
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}, \
  \
  {-1, -1, "flood_field_frame_buffer", MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, flood_field_frame_buffer), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}

#define MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS \
  {MXLV_AD_MASK_FILENAME, -1, "mask_filename", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, mask_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_AD_BIAS_FILENAME, -1, "bias_filename", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, bias_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_AD_DARK_CURRENT_FILENAME, -1, "dark_current_filename", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR, dark_current_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_AD_FLOOD_FIELD_FILENAME, -1, "flood_field_filename", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR, flood_field_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}

typedef struct {
        mx_status_type ( *arm ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *trigger ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *stop ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *abort ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *get_last_frame_number ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *get_total_num_frames ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_status ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_extended_status ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *readout_frame ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *correct_frame ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *transfer_frame ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *load_frame ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *save_frame ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *copy_frame ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_roi_frame ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *get_parameter ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *set_parameter ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *measure_correction ) ( MX_AREA_DETECTOR *ad );
} MX_AREA_DETECTOR_FUNCTION_LIST;

MX_API mx_status_type mx_area_detector_get_pointers( MX_RECORD *record,
                                        MX_AREA_DETECTOR **ad,
                                MX_AREA_DETECTOR_FUNCTION_LIST **flist_ptr,
                                        const char *calling_fname );

MX_API mx_status_type mx_area_detector_initialize_type(
			long record_type,
			long *num_record_fields,
			MX_RECORD_FIELD_DEFAULTS **record_field_defaults,
			long *maximum_num_rois_varargs_cookie );

MX_API mx_status_type mx_area_detector_finish_record_initialization(
						MX_RECORD *record );

MX_API mx_status_type mx_area_detector_load_correction_files(
						MX_RECORD *record );

/*---*/

MX_API mx_status_type mx_area_detector_get_long_parameter( MX_RECORD *record,
							char *parameter_name,
							long *parameter_value );

MX_API mx_status_type mx_area_detector_set_long_parameter( MX_RECORD *record,
							char *parameter_name,
							long *parameter_value );

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
						unsigned long roi_number,
						unsigned long *roi );

MX_API mx_status_type mx_area_detector_set_roi( MX_RECORD *ad_record,
						unsigned long roi_number,
						unsigned long *roi );

MX_API mx_status_type mx_area_detector_get_subframe_size( MX_RECORD *ad_record,
						unsigned long *num_columns );

MX_API mx_status_type mx_area_detector_set_subframe_size( MX_RECORD *ad_record,
						unsigned long num_columns );

MX_API mx_status_type mx_area_detector_get_bytes_per_frame( MX_RECORD *record,
						long *bytes_per_frame );

MX_API mx_status_type mx_area_detector_get_bytes_per_pixel( MX_RECORD *record,
						double *bytes_per_pixel );

MX_API mx_status_type mx_area_detector_get_bits_per_pixel( MX_RECORD *record,
						long *bits_per_pixel );

MX_API mx_status_type mx_area_detector_get_correction_flags( MX_RECORD *record,
					unsigned long *correction_flags );

MX_API mx_status_type mx_area_detector_set_correction_flags( MX_RECORD *record,
					unsigned long correction_flags );

MX_API mx_status_type mx_area_detector_measure_correction_frame(
					MX_RECORD *record,
					long correction_measurement_type,
					double correction_measurement_time,
					long num_correction_measurements );

#define mx_area_detector_measure_dark_current_frame( r, t, n ) \
	mx_area_detector_measure_correction_frame( (r), \
						MXFT_AD_DARK_CURRENT_FRAME, \
						(t), (n) )

#define mx_area_detector_measure_flood_field_frame( r, t, n ) \
	mx_area_detector_measure_correction_frame( (r), \
						MXFT_AD_FLOOD_FIELD_FRAME, \
						(t), (n) )

MX_API mx_status_type mx_area_detector_get_use_scaled_dark_current_flag(
						MX_RECORD *ad_record,
					mx_bool_type *use_scaled_dark_current );

MX_API mx_status_type mx_area_detector_set_use_scaled_dark_current_flag(
						MX_RECORD *ad_record,
					mx_bool_type use_scaled_dark_current );

/*---*/

MX_API mx_status_type mx_area_detector_get_detector_readout_time(
						MX_RECORD *ad_record,
						double *detector_readout_time );

MX_API mx_status_type mx_area_detector_get_total_sequence_time(
						MX_RECORD *ad_record,
						double *total_sequence_time );

MX_API mx_status_type mx_area_detector_get_sequence_parameters(
				MX_RECORD *ad_record,
				MX_SEQUENCE_PARAMETERS *sequence_parameters );

MX_API mx_status_type mx_area_detector_set_sequence_parameters(
				MX_RECORD *ad_record,
				MX_SEQUENCE_PARAMETERS *sequence_parameters );

MX_API mx_status_type mx_area_detector_set_one_shot_mode(MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_continuous_mode(MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_multiframe_mode(MX_RECORD *ad_record,
							long num_frames,
							double exposure_time,
							double frame_time );

MX_API mx_status_type mx_area_detector_set_circular_multiframe_mode(
							MX_RECORD *ad_record,
							long num_frames,
							double exposure_time,
							double frame_time );

MX_API mx_status_type mx_area_detector_set_strobe_mode( MX_RECORD *ad_record,
							long num_frames,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_bulb_mode( MX_RECORD *ad_record,
							long num_frames );

/*---*/

MX_API mx_status_type mx_area_detector_set_geometrical_mode(
						MX_RECORD *ad_record,
						long num_frames,
						double exposure_time,
						double frame_time,
						double exposure_multiplier,
						double gap_multiplier );

MX_API mx_status_type mx_area_detector_set_streak_camera_mode(
						MX_RECORD *ad_record,
						long num_lines,
						double exposure_time_per_line );

MX_API mx_status_type mx_area_detector_set_subimage_mode(
						MX_RECORD *ad_record,
						long num_lines_per_subimage,
						long num_subimages,
						double exposure_time,
						double subimage_time );

/*---*/

MX_API mx_status_type mx_area_detector_get_trigger_mode( MX_RECORD *ad_record,
							long *trigger_mode );

MX_API mx_status_type mx_area_detector_set_trigger_mode( MX_RECORD *ad_record,
							long trigger_mode );

/*---*/

MX_API mx_status_type mx_area_detector_arm( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_trigger( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_start( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_stop( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_abort( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_is_busy( MX_RECORD *ad_record,
						mx_bool_type *busy );

MX_API mx_status_type mx_area_detector_get_maximum_frame_number(
					MX_RECORD *ad_record,
					unsigned long *maximum_frame_number );

MX_API mx_status_type mx_area_detector_get_last_frame_number(
						MX_RECORD *ad_record,
						long *frame_number );

MX_API mx_status_type mx_area_detector_get_total_num_frames(
						MX_RECORD *ad_record,
						long *total_num_frames );

MX_API mx_status_type mx_area_detector_get_status( MX_RECORD *ad_record,
						unsigned long *status_flags );

MX_API mx_status_type mx_area_detector_get_extended_status(
						MX_RECORD *ad_record,
						long *last_frame_number,
						long *total_num_frames,
						unsigned long *status_flags );

MX_API mx_status_type mx_area_detector_setup_frame( MX_RECORD *ad_record,
						MX_IMAGE_FRAME **frame );

MX_API mx_status_type mx_area_detector_readout_frame( MX_RECORD *ad_record,
						long frame_number );

MX_API mx_status_type mx_area_detector_correct_frame( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_transfer_frame( MX_RECORD *ad_record,
						long frame_type,
						MX_IMAGE_FRAME *destination );

MX_API mx_status_type mx_area_detector_load_frame( MX_RECORD *ad_record,
						long frame_type,
						char *frame_filename );

MX_API mx_status_type mx_area_detector_save_frame( MX_RECORD *ad_record,
						long frame_type,
						char *frame_filename );

MX_API mx_status_type mx_area_detector_copy_frame( MX_RECORD *ad_record,
						long destination_frame_type,
						long source_frame_type );
/*---*/

MX_API mx_status_type mx_area_detector_get_frame( MX_RECORD *ad_record,
						long frame_number,
						MX_IMAGE_FRAME **frame );

MX_API mx_status_type mx_area_detector_get_sequence( MX_RECORD *ad_record,
						long num_frames,
						MX_IMAGE_SEQUENCE **sequence );

MX_API mx_status_type mx_area_detector_get_roi_frame( MX_RECORD *ad_record,
						MX_IMAGE_FRAME *frame,
						unsigned long roi_number,
						MX_IMAGE_FRAME **roi_frame );

/*---*/

MX_API mx_status_type mx_area_detector_default_correct_frame(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_transfer_frame(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_load_frame(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_save_frame(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_copy_frame(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_get_parameter_handler(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_set_parameter_handler(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_measure_correction(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_dezinger_correction(
                                                MX_AREA_DETECTOR *ad );

/*---*/

MX_API mx_status_type mx_area_detector_frame_correction( MX_RECORD *ad_record,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame,
					MX_IMAGE_FRAME *flood_field_frame );

/************************************************************************
 * The following functions are intended for use only in device drivers. *
 * They should not be called directly by application programs.          *
 ************************************************************************/

MX_API_PRIVATE mx_status_type mx_area_detector_compute_new_binning(
						MX_AREA_DETECTOR *ad,
						long parameter_type,
						int num_allowed_binsizes,
						long *allowed_binsize_array );

#endif /* __MX_AREA_DETECTOR_H__ */

