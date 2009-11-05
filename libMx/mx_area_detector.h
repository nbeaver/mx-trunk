/*
 * Name:    mx_area_detector.h
 *
 * Purpose: Functions for communicating with area detectors.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_AREA_DETECTOR_H__
#define __MX_AREA_DETECTOR_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_callback.h"
#include "mx_namefix.h"

#define MXU_AD_EXTENDED_STATUS_STRING_LENGTH	40

/* The datafile pattern char and the datafile pattern string
 * should be identical.
 */

#define MX_AREA_DETECTOR_DATAFILE_PATTERN_CHAR		'#'
#define MX_AREA_DETECTOR_DATAFILE_PATTERN_STRING	"#"

/* Status bit definitions for the 'status' field. */

#define MXSF_AD_ACQUISITION_IN_PROGRESS			0x1
#define MXSF_AD_CORRECTION_IN_PROGRESS			0x2
#define MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS	0x4
#define MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS		0x8

#define MXSF_AD_BUFFER_OVERRUN				0x100

#define MXSF_AD_ERROR					0x80000000

#define MXSF_AD_IS_BUSY		( MXSF_AD_ACQUISITION_IN_PROGRESS \
				| MXSF_AD_CORRECTION_IN_PROGRESS \
				| MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS \
				| MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS )

/*-----------------------------------------------------------------*/

/* Bit definitions for the 'area_detector_flags' variable. */

#define MXF_AD_GEOM_CORR_AFTER_FLOOD		   0x1
#define MXF_AD_CORRECTION_FRAME_GEOM_CORR_LAST	   0x2
#define MXF_AD_CORRECTION_FRAME_NO_GEOM_CORR	   0x4
#define MXF_AD_DEZINGER_CORRECTION_FRAME           0x8
#define MXF_AD_BIAS_CORR_AFTER_FLOOD		   0x10

#define MXF_AD_SAVE_FRAME_AFTER_ACQUISITION	   0x1000
#define MXF_AD_LOAD_FRAME_AFTER_ACQUISITION	   0x2000

  /* If MX is communicating with a vendor provided external
   * control system, then "remote frames" are frames that
   * are loaded in memory belonging to the external control
   * system.
   *
   * For example, if we are controlling MarCCD, the
   * "remote frame" is the data frame buffer belonging
   * to the "marccd" program.
   */

#define MXF_AD_SAVE_REMOTE_FRAME_AFTER_ACQUISITION 0x4000

#define MXF_AD_USE_UNSAFE_EXPOSURE		   0x80000000

/*-----------------------------------------------------------------*/

/* Frame types for the 'correct_frame', 'transfer_frame', 'load_frame',
 * 'save_frame', and 'copy_frame' fields.
 */

#define MXFT_AD_IMAGE_FRAME		0x0
#define MXFT_AD_MASK_FRAME		0x1
#define MXFT_AD_BIAS_FRAME		0x2
#define MXFT_AD_DARK_CURRENT_FRAME	0x4
#define MXFT_AD_FLOOD_FIELD_FRAME	0x8

/* The following are only used by 'save_frame', so that we can save
 * rebinned frames to disk files.  Most of the time it should not
 * be necessary to directly manipulate the rebinned frames.
 */

#define MXFT_AD_REBINNED_MASK_FRAME		0x10
#define MXFT_AD_REBINNED_BIAS_FRAME		0x20
#define MXFT_AD_REBINNED_DARK_CURRENT_FRAME	0x40
#define MXFT_AD_REBINNED_FLOOD_FIELD_FRAME	0x80

/* The following are used only for the 'correction_flags'
 * member of MX_AREA_DETECTOR.
 */

#define MXFT_AD_GEOMETRICAL_CORRECTION	0x1000
#define MXFT_AD_DEZINGER		0x2000

#define MXFT_AD_ALL			0xffff

/* The following bits are used only by 'initial_correction_flags'
 * to force the area detector software to use either the high memory
 * or the low memory frame correction methods.
 */

#define MXFT_AD_USE_LOW_MEMORY_METHODS	0x10000000
#define MXFT_AD_USE_HIGH_MEMORY_METHODS	0x20000000

typedef struct {
	struct mx_area_detector_type *area_detector;
	MX_IMAGE_FRAME *destination_frame;
	double exposure_time;
	long num_exposures;
	long num_frames_read;

	long raw_num_exposures;
	long raw_num_exposures_to_skip;

	MX_IMAGE_FRAME **dezinger_frame_array;

	double *sum_array;
	uint16_t *destination_array;

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

	/* Saving field numbers makes finding the fields quicker for
	 * the mx_area_detector_vctest_extended_status() function and
	 * other related status functions.
	 */

	long extended_status_field_number;
	long last_frame_number_field_number;
	long total_num_frames_field_number;
	long status_field_number;
	long last_datafile_name_field_number;

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

	double sequence_start_delay;
	double total_acquisition_time;
	double detector_readout_time;
	double total_sequence_time;

	mx_bool_type use_multiframe_correction;

	mx_bool_type correction_measurement_in_progress;

	mx_bool_type bias_corr_after_flood;

	mx_bool_type geom_corr_after_flood;
	mx_bool_type correction_frame_geom_corr_last;
	mx_bool_type correction_frame_no_geom_corr;

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

	/* 'image_frame_header_length', 'image_frame_header', and
	 * 'image_frame_data' are used to provide places for MX event
	 * handlers to find the current contents of the 'ad->image_frame'
	 * object.  They must only be modified by the functions in
	 * in libMx/pr_area_detector.c.  No other functions should
	 * modify them.
	 */

	unsigned long image_frame_header_length;
	char *image_frame_header;
	char *image_frame_data;

	/* The individual bits in 'correction_flags' determine which
	 * corrections are made.  The 'correct_frame' field tells the
	 * software to execute the corrections.
	 */

	mx_bool_type correct_frame;
	unsigned long correction_flags;

	mx_bool_type correction_frames_are_unbinned;

	/*
	 * 'flood_field_scale_max' and 'flood_field_scale_min' are used
	 * in flood field correction to limit the maximum and minimum
	 * values of the flood field scale factor.
	 */

	double flood_field_scale_max;
	double flood_field_scale_min;

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

	/* If correction_frames_to_skip is 0, then an MX server configured
	 * for callbacks will take an additional number of frames equal
	 * to the value of 'correction_frames_to_skip' and then throw away
	 * those initial frames when computing the measured correction frame.
	 */

	unsigned long correction_frames_to_skip;

	/* If dezinger_correction_frame is TRUE, then correction images
	 * are dezingered.
	 */

	mx_bool_type dezinger_correction_frame;

	/* dezinger_threshold is use to determine which pixels are to be
	 * thrown away during dezingering.
	 */

	double dezinger_threshold;	/* in units of standard deviation */

	/* Used by mx_area_detector_get_register() and
	 * mx_area_detector_set_register().
	 */

	char register_name[MXU_FIELD_NAME_LENGTH+1];
	long register_value;

	mx_bool_type shutter_enable;

	mx_bool_type transfer_image_during_scan;

	/* 'area_detector_flags' is used to initialize various features
	 * of the area detector.
	 */

	unsigned long area_detector_flags;

	/* 'initial_correction_flags' sets the initial value of the
	 * 'correction_flags' variable for image correction.
	 */

	unsigned long initial_correction_flags;

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
	double bias_average_intensity;

	MX_IMAGE_FRAME *flood_field_frame;
	char *flood_field_frame_buffer;
	char flood_field_filename[MXU_FILENAME_LENGTH+1];

	/* If the image frame is smaller than the correction frames by
	 * an integer ratio, rebinned versions of the correction frames
	 * are used to correct the image frame.  The following pointers
	 * point to the rebinned correction frames.
	 */

	MX_IMAGE_FRAME *rebinned_mask_frame;
	MX_IMAGE_FRAME *rebinned_bias_frame;
	MX_IMAGE_FRAME *rebinned_dark_current_frame;
	MX_IMAGE_FRAME *rebinned_flood_field_frame;

	/* dark_current_offset_array is recomputed any time that
	 * the exposure time is changed, the bias frame is changed,
	 * the dark current frame is changed, or the correction flags
	 * are changed.
	 *
	 * dark_current_offset_array uses 'float' rather than 'double'
	 * to save memory.  For a 4096 by 4096 image, this means that
	 * the array uses 64 megabytes rather than 128 megabytes.
	 */

	float *dark_current_offset_array;

	double old_exposure_time;

	/* flood_field_scale_array is recomputed any time that the
	 * bias frame is changed, the flood field frame is changed
	 * or the correction flags are changed.
	 *
	 * flood_field_scale_array uses 'float' rather than 'double'
	 * to save memory.  For a 4096 by 4096 image, this means that
	 * the array uses 64 megabytes rather than 128 megabytes.
	 */

	float *flood_field_scale_array;

	/* If correction calculations are performed in a format
	 * other than the native format of the image frame, then
	 * correction_calc_frame is where the intermediate
	 * correction frame values are stored.
	 */

	long correction_calc_format;

	MX_IMAGE_FRAME *correction_calc_frame;

	/* The datafile_... fields are used for the implementation
	 * of automatic saving or loading of image frames.
	 */

	char datafile_directory[MXU_FILENAME_LENGTH+1];
	char datafile_pattern[MXU_FILENAME_LENGTH+1];
	char datafile_name[MXU_FILENAME_LENGTH+1];
	unsigned long datafile_number;

	mx_bool_type datafile_allow_overwrite;
	mx_bool_type datafile_autoselect_number;

	mx_bool_type construct_next_datafile_name;

	char last_datafile_name[MXU_FILENAME_LENGTH+1];
	unsigned long datafile_format;

	long datafile_total_num_frames;
	long datafile_last_frame_number;
	mx_status_type (*datafile_management_handler)(MX_RECORD *);

	MX_CALLBACK *datafile_management_callback;

	/* The following entries are used for exposures that 
	 * are synchronized with a motor and a shutter.
	 */

	char exposure_motor_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *exposure_motor_record;
	char last_exposure_motor_name[MXU_RECORD_NAME_LENGTH+1];

	char shutter_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *shutter_record;
	char last_shutter_name[MXU_RECORD_NAME_LENGTH+1];

	char exposure_trigger_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *exposure_trigger_record;
	char last_exposure_trigger_name[MXU_RECORD_NAME_LENGTH+1];

	double exposure_distance;
	double shutter_time;
	mx_bool_type setup_exposure;
	mx_bool_type trigger_exposure;
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
#define MXLV_AD_IMAGE_FRAME_HEADER_LENGTH	12032
#define MXLV_AD_IMAGE_FRAME_HEADER		12033
#define MXLV_AD_IMAGE_FRAME_DATA		12034
#define MXLV_AD_CORRECT_FRAME			12035
#define MXLV_AD_CORRECTION_FLAGS		12036
#define MXLV_AD_TRANSFER_FRAME			12037
#define MXLV_AD_LOAD_FRAME			12038
#define MXLV_AD_SAVE_FRAME			12039
#define MXLV_AD_FRAME_FILENAME			12040
#define MXLV_AD_COPY_FRAME			12041
#define MXLV_AD_SEQUENCE_START_DELAY		12042
#define MXLV_AD_TOTAL_ACQUISITION_TIME		12043
#define MXLV_AD_DETECTOR_READOUT_TIME		12044
#define MXLV_AD_TOTAL_SEQUENCE_TIME		12045
#define MXLV_AD_GEOM_CORR_AFTER_FLOOD		12046
#define MXLV_AD_BIAS_CORR_AFTER_FLOOD		12047
#define MXLV_AD_CORRECTION_FRAME_GEOM_CORR_LAST	12048
#define MXLV_AD_CORRECTION_FRAME_NO_GEOM_CORR	12049
#define MXLV_AD_CORRECTION_MEASUREMENT_TYPE	12050
#define MXLV_AD_CORRECTION_MEASUREMENT_TIME	12051
#define MXLV_AD_NUM_CORRECTION_MEASUREMENTS	12052
#define MXLV_AD_CORRECTION_FRAMES_TO_SKIP	12053
#define MXLV_AD_DEZINGER_CORRECTION_FRAME	12054
#define MXLV_AD_DEZINGER_THRESHOLD		12055
#define MXLV_AD_USE_SCALED_DARK_CURRENT		12056
#define MXLV_AD_REGISTER_NAME			12057
#define MXLV_AD_REGISTER_VALUE			12058
#define MXLV_AD_SHUTTER_ENABLE			12059
#define MXLV_AD_TRANSFER_IMAGE_DURING_SCAN	12060

#define MXLV_AD_AREA_DETECTOR_FLAGS		12100
#define MXLV_AD_INITIAL_CORRECTION_FLAGS	12101
#define MXLV_AD_MASK_FILENAME			12102
#define MXLV_AD_BIAS_FILENAME			12103
#define MXLV_AD_DARK_CURRENT_FILENAME		12104
#define MXLV_AD_FLOOD_FIELD_FILENAME		12105

#define MXLV_AD_SUBFRAME_SIZE			12201

#define MXLV_AD_DATAFILE_DIRECTORY		12500
#define MXLV_AD_DATAFILE_PATTERN		12501
#define MXLV_AD_DATAFILE_NAME			12502
#define MXLV_AD_DATAFILE_NUMBER			12503
#define MXLV_AD_DATAFILE_ALLOW_OVERWRITE	12504
#define MXLV_AD_DATAFILE_AUTOSELECT_NUMBER	12505
#define MXLV_AD_CONSTRUCT_NEXT_DATAFILE_NAME	12506
#define MXLV_AD_LAST_DATAFILE_NAME		12507
#define MXLV_AD_DATAFILE_FORMAT			12508

#define MXLV_AD_EXPOSURE_MOTOR_NAME		12600
#define MXLV_AD_SHUTTER_NAME			12601
#define MXLV_AD_EXPOSURE_TRIGGER_NAME		12602
#define MXLV_AD_EXPOSURE_DISTANCE		12603
#define MXLV_AD_SHUTTER_TIME			12604
#define MXLV_AD_SETUP_EXPOSURE			12605
#define MXLV_AD_TRIGGER_EXPOSURE		12606

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
	{0}, NULL, (MXFF_READ_ONLY | MXFF_POLL), \
	0, 0, mx_area_detector_vctest_extended_status}, \
  \
  {MXLV_AD_TOTAL_NUM_FRAMES, -1, "total_num_frames", MXFT_LONG, NULL, 0, {0},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, total_num_frames), \
	{0}, NULL, (MXFF_READ_ONLY | MXFF_POLL), \
	0, 0, mx_area_detector_vctest_extended_status}, \
  \
  {MXLV_AD_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, status), \
	{0}, NULL, (MXFF_READ_ONLY | MXFF_POLL), \
	0, 0, mx_area_detector_vctest_extended_status}, \
  \
  {MXLV_AD_EXTENDED_STATUS, -1, "extended_status", MXFT_STRING, \
			NULL, 1, {MXU_AD_EXTENDED_STATUS_STRING_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, extended_status), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_POLL), \
	0, 0, mx_area_detector_vctest_extended_status}, \
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
  {MXLV_AD_IMAGE_FRAME_HEADER_LENGTH, -1, "image_frame_header_length", \
						MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, image_frame_header_length),\
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_IMAGE_FRAME_HEADER, -1, "image_frame_header", \
						MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_frame_header),\
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}, \
  \
  {MXLV_AD_IMAGE_FRAME_DATA, -1, "image_frame_data", \
						MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_frame_data),\
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
  {-1, -1, "correction_frames_are_unbinned", MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_frames_are_unbinned), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "flood_field_scale_max", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, flood_field_scale_max), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "flood_field_scale_min", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, flood_field_scale_min), \
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
  {MXLV_AD_SEQUENCE_START_DELAY, -1, \
		"sequence_start_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, sequence_start_delay), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_TOTAL_ACQUISITION_TIME, -1, \
		"total_acquisition_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, total_acquisition_time), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_DETECTOR_READOUT_TIME, -1, \
		"detector_readout_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, detector_readout_time), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_TOTAL_SEQUENCE_TIME, -1, \
		"total_sequence_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, total_sequence_time), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_GEOM_CORR_AFTER_FLOOD, -1, \
  		"geom_corr_after_flood", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, geom_corr_after_flood), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_BIAS_CORR_AFTER_FLOOD, -1, \
  		"bias_corr_after_flood", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, bias_corr_after_flood), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_CORRECTION_FRAME_GEOM_CORR_LAST, -1, \
  		"correction_frame_geom_corr_last", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_frame_geom_corr_last), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_CORRECTION_FRAME_NO_GEOM_CORR, -1, \
  		"correction_frame_no_geom_corr", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_frame_no_geom_corr), \
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
  {MXLV_AD_CORRECTION_FRAMES_TO_SKIP, -1, \
  		"correction_frames_to_skip", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_frames_to_skip), \
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
  {MXLV_AD_DEZINGER_CORRECTION_FRAME, -1, \
  		"dezinger_correction_frame", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, dezinger_correction_frame), \
	{0}, NULL, 0}, \
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
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}, \
  \
  {MXLV_AD_REGISTER_NAME, -1, "register_name", MXFT_STRING, NULL, \
  					1, {MXU_FIELD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, register_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_REGISTER_VALUE, -1, "register_value", \
  					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof( MX_AREA_DETECTOR, register_value ),\
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_SHUTTER_ENABLE, -1, "shutter_enable", \
  					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof( MX_AREA_DETECTOR, shutter_enable ),\
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_TRANSFER_IMAGE_DURING_SCAN, -1, "transfer_image_during_scan", \
  					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof( MX_AREA_DETECTOR, transfer_image_during_scan ),\
	{sizeof(char)}, NULL, 0}

#define MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS \
  {MXLV_AD_AREA_DETECTOR_FLAGS, -1, "area_detector_flags", \
			  			MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, area_detector_flags), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_AD_INITIAL_CORRECTION_FLAGS, -1, "initial_correction_flags", \
			  			MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, initial_correction_flags), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
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
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_AD_DATAFILE_DIRECTORY, -1, "datafile_directory", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, datafile_directory), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_FORMAT, -1, "datafile_format", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, datafile_format), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_PATTERN, -1, "datafile_pattern", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, datafile_pattern), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_NAME, -1, "datafile_name", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, datafile_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_NUMBER, -1, "datafile_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, datafile_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_ALLOW_OVERWRITE, -1, "datafile_allow_overwrite", \
					MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, datafile_allow_overwrite), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_AUTOSELECT_NUMBER, -1, "datafile_autoselect_number", \
					MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, datafile_autoselect_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_CONSTRUCT_NEXT_DATAFILE_NAME, -1, "construct_next_datafile_name", \
  					MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, construct_next_datafile_name), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_LAST_DATAFILE_NAME, -1, "last_datafile_name", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, last_datafile_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {-1, -1, "datafile_total_num_frames", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, datafile_total_num_frames), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_EXPOSURE_MOTOR_NAME, -1, "exposure_motor_name", MXFT_STRING, \
  					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR, exposure_motor_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_SHUTTER_NAME, -1, "shutter_name", MXFT_STRING, \
  					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, shutter_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_EXPOSURE_TRIGGER_NAME, -1, "exposure_trigger_name", MXFT_STRING, \
  					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR, exposure_trigger_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_EXPOSURE_DISTANCE, -1, "exposure_distance", MXFT_DOUBLE, \
					NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, exposure_distance),\
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SHUTTER_TIME, -1, "shutter_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, shutter_time),\
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SETUP_EXPOSURE, -1, "setup_exposure", MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, setup_exposure), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_TRIGGER_EXPOSURE, -1, "trigger_exposure", MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, trigger_exposure), \
	{0}, NULL, 0}

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
	mx_status_type ( *geometrical_correction ) ( MX_AREA_DETECTOR * ad,
							MX_IMAGE_FRAME *frame );
	mx_status_type ( *setup_exposure ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *trigger_exposure ) ( MX_AREA_DETECTOR *ad );
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

MX_API mx_status_type mx_area_detector_get_register( MX_RECORD *record,
							char *register_name,
							long *register_value );

MX_API mx_status_type mx_area_detector_set_register( MX_RECORD *record,
							char *register_name,
							long register_value );

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

MX_API mx_status_type mx_area_detector_get_sequence_start_delay(
						MX_RECORD *ad_record,
						double *sequence_start_delay );

MX_API mx_status_type mx_area_detector_set_sequence_start_delay(
						MX_RECORD *ad_record,
						double sequence_start_delay );

MX_API mx_status_type mx_area_detector_get_total_acquisition_time(
						MX_RECORD *ad_record,
						double *total_acquisition_time);

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
						double exposure_time_per_line,
						double total_time_per_line );

MX_API mx_status_type mx_area_detector_set_subimage_mode(
						MX_RECORD *ad_record,
						long num_lines_per_subimage,
						long num_subimages,
						double exposure_time,
						double subimage_time,
						double exposure_multiplier,
						double gap_multiplier );

/*---*/

MX_API mx_status_type mx_area_detector_get_trigger_mode( MX_RECORD *ad_record,
							long *trigger_mode );

MX_API mx_status_type mx_area_detector_set_trigger_mode( MX_RECORD *ad_record,
							long trigger_mode );

MX_API mx_status_type mx_area_detector_get_shutter_enable( MX_RECORD *ad_record,
						mx_bool_type *shutter_enable );

MX_API mx_status_type mx_area_detector_set_shutter_enable( MX_RECORD *ad_record,
						mx_bool_type shutter_enable );

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

/*---*/

MX_API mx_status_type mx_area_detector_setup_exposure( MX_RECORD *ad_record,
						MX_RECORD *motor_record,
						MX_RECORD *shutter_record,
						MX_RECORD *trigger_record,
						double exposure_distance,
						double shutter_time );

MX_API mx_status_type mx_area_detector_trigger_exposure( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_wait_for_exposure_end( MX_RECORD *record,
							double timeout );

MX_API mx_status_type mx_area_detector_trigger_unsafe_exposure(
						MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_send_exposure_trigger_pulse(
						MX_AREA_DETECTOR *ad );

/*---*/

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
						long source_frame_type,
						long destination_frame_type );
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

MX_API mx_status_type mx_area_detector_get_parameter( MX_RECORD *ad_record,
						long parameter_type );

MX_API mx_status_type mx_area_detector_set_parameter( MX_RECORD *ad_record,
						long parameter_type );

/*---*/

MX_API mx_status_type mx_area_detector_get_correction_frame(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					unsigned long frame_type,
					char *frame_name,
					MX_IMAGE_FRAME **correction_frame );

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

MX_API mx_status_type mx_area_detector_default_geometrical_correction(
						MX_AREA_DETECTOR *ad,
						MX_IMAGE_FRAME *frame );

/*---*/

MX_API void mx_area_detector_cleanup_after_correction( MX_AREA_DETECTOR *ad,
				MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr);

MX_API mx_status_type mx_area_detector_prepare_for_correction(
			MX_AREA_DETECTOR *ad,
			MX_AREA_DETECTOR_CORRECTION_MEASUREMENT **corr_ptr );

MX_API mx_status_type mx_area_detector_process_correction_frame(
					MX_AREA_DETECTOR *ad,
					long frame_number,
					unsigned long desired_correction_flags,
					MX_IMAGE_FRAME **dezinger_frame_ptr,
					double *sum_array );

MX_API mx_status_type mx_area_detector_finish_correction_calculation(
			MX_AREA_DETECTOR *ad,
			MX_AREA_DETECTOR_CORRECTION_MEASUREMENT *corr );

/*---*/

MX_API mx_status_type mx_area_detector_default_get_register(
						MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_set_register(
						MX_AREA_DETECTOR *ad );

/*---*/

MX_API mx_status_type mx_area_detector_copy_and_convert_image_data(
					MX_IMAGE_FRAME *source_frame,
					MX_IMAGE_FRAME *destination_frame );

/*---*/

MX_API mx_status_type mx_area_detector_frame_correction( MX_RECORD *ad_record,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame,
					MX_IMAGE_FRAME *flood_field_frame );

/*---*/

MX_API mx_status_type mx_area_detector_initialize_datafile_number(
							MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_construct_next_datafile_name(
							MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_initialize_remote_datafile_number(
							MX_RECORD *ad_record,
							char *remote_prefix,
							char *local_prefix );

/*---*/

MX_API mx_status_type mx_area_detector_setup_datafile_management(
				MX_RECORD *ad_record,
				mx_status_type (*handler_fn)(MX_RECORD *) );

MX_API mx_status_type mx_area_detector_default_datafile_management_handler(
							MX_RECORD *ad_record );

/*---*/

MX_API_PRIVATE mx_status_type mx_area_detector_vctest_extended_status(
					MX_RECORD_FIELD *, mx_bool_type * );

MX_API_PRIVATE void mx_area_detector_update_extended_status_string(
					MX_AREA_DETECTOR *ad );

/************************************************************************
 * The following functions are intended for use only in device drivers. *
 * They should not be called directly by application programs.          *
 ************************************************************************/

MX_API_PRIVATE mx_status_type mx_area_detector_compute_new_binning(
						MX_AREA_DETECTOR *ad,
						long parameter_type,
						int num_allowed_binsizes,
						long *allowed_binsize_array );

MX_API_PRIVATE mx_status_type mx_area_detector_compute_dark_current_offset(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

MX_API_PRIVATE mx_status_type mx_area_detector_compute_flood_field_scale(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flood_field_frame );

#ifdef __cplusplus
}
#endif

#endif /* __MX_AREA_DETECTOR_H__ */

