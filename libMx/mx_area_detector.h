/*
 * Name:    mx_area_detector.h
 *
 * Purpose: Functions for communicating with area detectors.
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

#ifndef __MX_AREA_DETECTOR_H__
#define __MX_AREA_DETECTOR_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_callback.h"
#include "mx_image.h"
#include "mx_namefix.h"

#define MXU_AD_EXTENDED_STATUS_STRING_LENGTH	40

#define MXU_AD_DATAFILE_FORMAT_NAME_LENGTH	20

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

#define MXSF_AD_UNSAVED_IMAGE_FRAMES			0x10

#define MXSF_AD_BUFFER_OVERRUN				0x100
#define MXSF_AD_FILE_IO_ERROR				0x200
#define MXSF_AD_PERMISSION_DENIED			0x400
#define MXSF_AD_DISK_FULL				0x800

#define MXSF_AD_EXPOSURE_TIME_CONFLICT			0x1000

#define MXSF_AD_ERROR					0x80000000

#define MXSF_AD_IS_BUSY		( MXSF_AD_ACQUISITION_IN_PROGRESS \
				| MXSF_AD_CORRECTION_IN_PROGRESS \
				| MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS \
				| MXSF_AD_CONTROLLER_ACTION_IN_PROGRESS )

/*-----------------------------------------------------------------*/

/* Bit definitions for the 'area_detector_flags' variable. */

#define MXF_AD_GEOM_CORR_AFTER_FLAT_FIELD	   		0x1
#define MXF_AD_CORRECTION_FRAME_GEOM_CORR_LAST	   		0x2
#define MXF_AD_CORRECTION_FRAME_NO_GEOM_CORR	   		0x4
#define MXF_AD_DEZINGER_CORRECTION_FRAME           		0x8
#define MXF_AD_BIAS_CORR_AFTER_FLAT_FIELD		   		0x10

  /* If MXF_AD_SAVE_FRAME_AFTER_ACQUISITION is set and we are running in an
   * MX server, then the area detector datafile management routines will
   * automatically arrange to write the image frame data out to a file.
   */

#define MXF_AD_SAVE_FRAME_AFTER_ACQUISITION	   		0x1000

  /* For some detectors controlled by an external control system
   * such as MarCCD, the act of reading out and correcting newly
   * acquired frames is what causes the external control system
   * to write out its own native copies of the image.  The flag
   * MXF_AD_READOUT_FRAME_AFTER_ACQUISITION is used to tell the
   * datafile management callback that it should readout and
   * correct the image data, even though the callback may not 
   * itself be responsible for saving the image data to disk.
   */

#define MXF_AD_READOUT_FRAME_AFTER_ACQUISITION			0x2000

  /* If MX is communicating with a vendor provided external
   * control system, then "remote frames" are frames that
   * are loaded in memory belonging to the external control
   * system.
   *
   * For example, if we are controlling MarCCD, the
   * "remote frame" is the data frame buffer belonging
   * to the "marccd" program.
   */

#define MXF_AD_SAVE_REMOTE_FRAME_AFTER_ACQUISITION 		0x4000

  /* If we are debugging correction measurements, then we can turn on
   * the MXF_AD_SAVE_CORRECTION_FRAME_AFTER_ACQUISITION flag to save
   * correction frames as part of the normal sequence of frames.  This
   * will create a separate file for each frame of the correction
   * measurement sequence.
   */

#define MXF_AD_SAVE_CORRECTION_FRAME_AFTER_ACQUISITION		0x8000

  /*
   * MXF_AD_SAVE_AVERAGED_CORRECTION_FRAME tells the detector software
   * to save a copy of the averaged result of a correction sequence to a
   * user-specified location and filename.
   */

#define MXF_AD_SAVE_AVERAGED_CORRECTION_FRAME			0x10000

  /* If one or more of the directories in the pathname of the image files
   * does not exist, then this flag below tells MX to automatically create
   * all of the directories needed to be able to create this file.
   */

#define MXF_AD_AUTOMATICALLY_CREATE_DIRECTORY_HIERARCHY		0x20000

  /* The following flags tells the area detector to alway take
   * correction measurements in a particular trigger mode.  If
   * both the internal and external flags are selected, internal
   * trigger will be used.
   */

#define MXF_AD_CORRECTION_MEASUREMENTS_USE_INTERNAL_TRIGGER	0x100000

#define MXF_AD_CORRECTION_MEASUREMENTS_USE_EXTERNAL_TRIGGER	0x200000

  /* Set the following flag if you want to suppress the check by
   * mx_area_detector_arm() for exposure time conflicts.
   */

#define MXF_AD_BYPASS_DARK_CURRENT_EXPOSURE_TIME_TEST		0x1000000

  /* If MX is running in a single process (list_head->is_server == FALSE)
   * and the following flag is set, then the function
   * mx_area_detector_finish_record_initialization() will force
   * off the MXF_AD_SAVE_FRAME_AFTER_ACQUISITION flag.  This flag
   * exists to make it easier to debug drivers when running in 
   * single process mode by eliminating attempts to do server-style
   * background frame saving.  This flag has no effect if you are
   * running in a real server (list_head->is_server == TRUE).
   */

#define MXF_AD_DO_NOT_SAVE_FRAME_IN_SINGLE_PROCESS_MODE		0x10000000

  /* The following flag requests that a 6-bit ASCII debugging image
   * be written to the log at the end of each call to the function
   * mx_area_detector_readout_frame().
   */

#define MXF_AD_DISPLAY_ASCII_DEBUGGING_IMAGE			0x40000000

  /* The following flag is intended for testing only. */

#define MXF_AD_USE_UNSAFE_OSCILLATION		   		0x80000000

/*-----------------------------------------------------------------*/

/* Values for the 'exposure_mode' field. */

#define MXF_AD_STILL_MODE		1
#define MXF_AD_DARK_MODE		2
#define MXF_AD_EXPOSE_MODE		3

/*-----------------------------------------------------------------*/

/* Frame types for the 'correct_frame', 'transfer_frame', 'load_frame',
 * 'save_frame', and 'copy_frame' fields.
 */

#define MXFT_AD_IMAGE_FRAME		0x0
#define MXFT_AD_MASK_FRAME		0x1
#define MXFT_AD_BIAS_FRAME		0x2
#define MXFT_AD_DARK_CURRENT_FRAME	0x4
#define MXFT_AD_FLAT_FIELD_FRAME	0x8

/* The following are only used by 'save_frame', so that we can save
 * rebinned frames to disk files.  Most of the time it should not
 * be necessary to directly manipulate the rebinned frames.
 */

#define MXFT_AD_REBINNED_MASK_FRAME		0x10
#define MXFT_AD_REBINNED_BIAS_FRAME		0x20
#define MXFT_AD_REBINNED_DARK_CURRENT_FRAME	0x40
#define MXFT_AD_REBINNED_FLAT_FIELD_FRAME	0x80

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

	long num_unread_frames;
	long old_last_frame_number;
	long old_total_num_frames;
	unsigned long old_status;
	unsigned long desired_correction_flags;
	long saved_trigger_mode;

	MX_CALLBACK_MESSAGE *callback_message;
} MX_AREA_DETECTOR_CORRECTION_MEASUREMENT;

typedef struct mx_area_detector_type {
	MX_RECORD *record;

	void *application_ptr;

	long parameter_type;
	long frame_number;

	long maximum_framesize[2];
	long framesize[2];
	long binsize[2];
	char image_format_name[MXU_IMAGE_FORMAT_NAME_LENGTH+1];
	long image_format;
	long byte_order;
	long trigger_mode;
	long exposure_mode;
	long header_length;
	long bytes_per_frame;
	double bytes_per_pixel;
	long bits_per_pixel;

	long mask_image_format;
	long bias_image_format;
	long dark_current_image_format;
	long flat_field_image_format;

	unsigned long measure_dark_current_correction_flags;
	unsigned long measure_flat_field_correction_flags;

	mx_bool_type arm;
	mx_bool_type trigger;
	mx_bool_type start;
	mx_bool_type stop;
	mx_bool_type abort;
	mx_bool_type busy;
	unsigned long maximum_frame_number;
	long last_frame_number;
	long total_num_frames;
	unsigned long status;
	char extended_status[ MXU_AD_EXTENDED_STATUS_STRING_LENGTH + 1 ];

	unsigned long latched_status;

	/* Saving field numbers makes finding the fields quicker for
	 * the mx_area_detector_vctest_extended_status() function and
	 * other related status functions.
	 */

	long extended_status_field_number;
	long last_frame_number_field_number;
	long total_num_frames_field_number;
	long status_field_number;

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

	mx_bool_type correction_measurement_sequence_type;

	mx_bool_type correction_measurement_in_progress;

	mx_bool_type bias_corr_after_flat_field;

	mx_bool_type geom_corr_after_flat_field;
	mx_bool_type correction_frame_geom_corr_last;
	mx_bool_type correction_frame_no_geom_corr;

	/* 'sequence_parameters' contains information like the type of the
	 * sequence, the number of frames in the sequence, and sequence
	 * parameters like the exposure time per frame, and the interval
	 * between frames.
	 */

	MX_SEQUENCE_PARAMETERS sequence_parameters;

	/* The following variables provide a way for MX network clients
	 * to set up certain sequence types in one mx_put() operation.
	 * The received values are copied into the MX_SEQUENCE_PARAMETERS
	 * structure above.
	 */

	double sequence_one_shot[1];
	double sequence_continuous[1];
	double sequence_multiframe[3];
	double sequence_strobe[2];
	double sequence_duration[1];
	double sequence_gated[3];

	/* 'exposure_time' is used by mx_area_detector_get_exposure_time()
	 * to extract the exposure time from the MX_SEQUENCE_PARAMETERS
	 * structure and put it someplace that a remote client can 
	 * request it from.  'num_exposures' does the same for 
	 * mx_area_detector_get_num_exposures().
	 */

	double exposure_time;

	long num_exposures;

	/* If 'image_data_available' is TRUE, then it is possible to get
	 * real image data into the 'image_frame' object using either a
	 * 'readout_frame' or a 'transfer_frame' operation.
	 *
	 * If the 'image_data_available' flag is FALSE, then the 'image_frame'
	 * object _never_ has valid data in it.
	 *
	 * Most area detector drivers will have this flag set to TRUE.
	 * If the area detector hardware is controlled by an external
	 * control system, then it may not be possible to get at the
	 * actual image frames and it will be necessary to set the flag
	 * to FALSE.  In that case, the MX area detector driver is 
	 * merely used to send commands to the external control system.
	 */

	mx_bool_type image_data_available;

	/* The 'image_frame' object normally contains the actual image data 
	 * read from the detector.
	 *
	 * The readout_frame() method expects to read the new frame
	 * into the 'image_frame' object.
	 */

	long readout_frame;
	MX_IMAGE_FRAME *image_frame;

	/* 'image_frame_header_length', 'image_frame_header', and
	 * 'image_frame_data' are used to provide places for MX event
	 * handlers to find the current contents of the 'ad->image_frame'
	 * object.  They must only be modified by the functions in
	 * in libMx/pr_area_detector.c and libMx/mx_area_detector.c.
	 * No other functions should modify them.
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

	/* 'all_mask_pixels_are_set' and 'all_bias_pixels_are_equal'
	 * are used by some detector drivers to optimize the correction
	 * process when they are set.  'constant_bias_pixel_offset' only
	 * has a valid value if 'all_bias_pixels_are_equal' is set.
	 */

	mx_bool_type all_mask_pixels_are_set;
	mx_bool_type all_bias_pixels_are_equal;

	double constant_bias_pixel_offset;

	/*
	 * 'flat_field_scale_max' and 'flat_field_scale_min' are used
	 * in flat field correction to limit the maximum and minimum
	 * values of the flat field scale factor.
	 */

	double flat_field_scale_max;
	double flat_field_scale_min;

	/* 'transfer_frame' tells the server to send one of the frames
	 * to the caller.
	 */

	long transfer_frame;
	MX_IMAGE_FRAME **transfer_destination_frame_ptr;

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

	/* mx_area_detector_save_averaged_correction_frame() uses the
	 * following two fields to get the name of the file to save an
	 * averaged correction_frame to.  These two fields do not have
	 * a process function, so writing to them does not, by itself,
	 * cause anything to happen.
	 */

	char saved_dark_current_filename[MXU_FILENAME_LENGTH+1];
	char saved_flat_field_filename[MXU_FILENAME_LENGTH+1];

	/* The following fields are used for measuring dark current and
	 * flat field image frames.
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

	/* 'mark_frame_as_saved' is used to indicate that the specified
	 * frame number has been saved by some unspecified archival system.
	 * The specified frame number is an absolute frame number that is
	 * compatible with the value of 'total_num_frames'.
	 *
	 * It is intended to be used by buffer overrun detection logic
	 * for area detector devices that maintain the image frames in
	 * some sort of circular buffer.  For other area detector types,
	 * it is probably of no use.
	 */

	long mark_frame_as_saved;

	/* 'show_image_frame' and 'show_image_statistics' are used to
	 * display what is currently in the particular image buffer
	 * specified by the value passed to the fields.
	 */

	long show_image_frame;
	long show_image_statistics;

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

	double flat_field_average_intensity;
	double bias_average_intensity;

	MX_IMAGE_FRAME *flat_field_frame;
	char *flat_field_frame_buffer;
	char flat_field_filename[MXU_FILENAME_LENGTH+1];

	/* If the image frame is smaller than the correction frames by
	 * an integer ratio, rebinned versions of the correction frames
	 * are used to correct the image frame.  The following pointers
	 * point to the rebinned correction frames.
	 */

	MX_IMAGE_FRAME *rebinned_mask_frame;
	MX_IMAGE_FRAME *rebinned_bias_frame;
	MX_IMAGE_FRAME *rebinned_dark_current_frame;
	MX_IMAGE_FRAME *rebinned_flat_field_frame;

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

	mx_bool_type dark_current_offset_can_change;

	/* flat_field_scale_array is recomputed any time that the
	 * bias frame is changed, the flat field frame is changed
	 * or the correction flags are changed.
	 *
	 * flat_field_scale_array uses 'float' rather than 'double'
	 * to save memory.  For a 4096 by 4096 image, this means that
	 * the array uses 64 megabytes rather than 128 megabytes.
	 */

	float *flat_field_scale_array;

	mx_bool_type flat_field_scale_can_change;

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

	unsigned long datafile_load_format;
	unsigned long datafile_save_format;

	char datafile_load_format_name[ MXU_AD_DATAFILE_FORMAT_NAME_LENGTH+1 ];
	char datafile_save_format_name[ MXU_AD_DATAFILE_FORMAT_NAME_LENGTH+1 ];

	unsigned long correction_load_format;
	unsigned long correction_save_format;

	char correction_load_format_name[
				MXU_AD_DATAFILE_FORMAT_NAME_LENGTH+1 ];
	char correction_save_format_name[
				MXU_AD_DATAFILE_FORMAT_NAME_LENGTH+1 ];

	long datafile_total_num_frames;
	long datafile_last_frame_number;
	mx_status_type (*datafile_management_handler)(MX_RECORD *);

	MX_CALLBACK *datafile_management_callback;

	mx_bool_type inhibit_autosave;

	/* The following entries report the total disk space and the
	 * free disk space in bytes available for the disk partition
	 * that contains the directory specified by 'datafile_directory'.
	 */

	uint64_t disk_space[2];

	/* The following entries are used for oscillation exposures that 
	 * are synchronized with a motor and a shutter.
	 */

	char oscillation_motor_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *oscillation_motor_record;
	char last_oscillation_motor_name[MXU_RECORD_NAME_LENGTH+1];

	char shutter_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *shutter_record;
	char last_shutter_name[MXU_RECORD_NAME_LENGTH+1];

	char oscillation_trigger_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *oscillation_trigger_record;
	char last_oscillation_trigger_name[MXU_RECORD_NAME_LENGTH+1];

	double oscillation_distance;
	double shutter_time;
	mx_bool_type setup_oscillation;
	mx_bool_type trigger_oscillation;

	/* The following contains the (calculated ?) motor position
	 * corresponding to the most recent call to 'readout_frame'.
	 *
	 * Sometimes this is generated using the oscillation variables
	 * immediately above.
	 */

	double motor_position;

	/* Support for an optional image log file, which writes
	 * information about currently running sequences to a file 
	 * which can be used by programs that copy image files from
	 * a local directory to an archive directory.  An example
	 * of this is the 'mx_ad_copy' program.  If a filename is
	 * written to the 'image_log_filename' field, any existing
	 * image log is closed and the code switches to appending
	 * to the new log file (which may actually be the same file).
	 */

	FILE *image_log_file;
	char image_log_filename[MXU_FILENAME_LENGTH+1];

	/* 'image_log_error_seen' is used to ensure that a given error
	 * condition is written to the image log only once during a
	 * given sequence.
	 */

	mx_bool_type image_log_error_seen;
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
#define MXLV_AD_EXPOSURE_MODE			12008
#define MXLV_AD_BYTES_PER_FRAME			12009
#define MXLV_AD_BYTES_PER_PIXEL			12010
#define MXLV_AD_BITS_PER_PIXEL			12011
#define MXLV_AD_ARM				12012
#define MXLV_AD_TRIGGER				12013
#define MXLV_AD_START				12014
#define MXLV_AD_STOP				12015
#define MXLV_AD_ABORT				12016
#define MXLV_AD_MAXIMUM_FRAME_NUMBER		12017
#define MXLV_AD_LAST_FRAME_NUMBER		12018
#define MXLV_AD_TOTAL_NUM_FRAMES		12019
#define MXLV_AD_STATUS				12020
#define MXLV_AD_EXTENDED_STATUS			12021
#define MXLV_AD_LATCHED_STATUS			12022
#define MXLV_AD_MAXIMUM_NUM_ROIS		12023
#define MXLV_AD_CURRENT_NUM_ROIS		12024
#define MXLV_AD_ROI_ARRAY			12025
#define MXLV_AD_ROI_NUMBER			12026
#define MXLV_AD_ROI				12027
#define MXLV_AD_ROI_BYTES_PER_FRAME		12028
#define MXLV_AD_GET_ROI_FRAME			12029
#define MXLV_AD_ROI_FRAME_BUFFER		12030
#define MXLV_AD_SEQUENCE_TYPE			12031
#define MXLV_AD_NUM_SEQUENCE_PARAMETERS		12032
#define MXLV_AD_SEQUENCE_PARAMETER_ARRAY	12033
#define MXLV_AD_EXPOSURE_TIME			12034
#define MXLV_AD_NUM_EXPOSURES			12035
#define MXLV_AD_IMAGE_DATA_AVAILABLE		12036
#define MXLV_AD_READOUT_FRAME			12037
#define MXLV_AD_IMAGE_FRAME_HEADER_LENGTH	12038
#define MXLV_AD_IMAGE_FRAME_HEADER		12039
#define MXLV_AD_IMAGE_FRAME_DATA		12040
#define MXLV_AD_CORRECT_FRAME			12041
#define MXLV_AD_CORRECTION_FLAGS		12042
#define MXLV_AD_CONSTANT_BIAS_PIXEL_OFFSET	12043
#define MXLV_AD_TRANSFER_FRAME			12044
#define MXLV_AD_LOAD_FRAME			12045
#define MXLV_AD_SAVE_FRAME			12046
#define MXLV_AD_RAW_LOAD_FRAME			12047
#define MXLV_AD_RAW_SAVE_FRAME			12048
#define MXLV_AD_FRAME_FILENAME			12049
#define MXLV_AD_COPY_FRAME			12050
#define MXLV_AD_SEQUENCE_START_DELAY		12051
#define MXLV_AD_TOTAL_ACQUISITION_TIME		12052
#define MXLV_AD_DETECTOR_READOUT_TIME		12053
#define MXLV_AD_TOTAL_SEQUENCE_TIME		12054
#define MXLV_AD_GEOM_CORR_AFTER_FLAT_FIELD		12055
#define MXLV_AD_BIAS_CORR_AFTER_FLAT_FIELD		12056
#define MXLV_AD_CORRECTION_FRAME_GEOM_CORR_LAST	12057
#define MXLV_AD_CORRECTION_FRAME_NO_GEOM_CORR	12058
#define MXLV_AD_CORRECTION_MEASUREMENT_TYPE	12059
#define MXLV_AD_CORRECTION_MEASUREMENT_TIME	12060
#define MXLV_AD_NUM_CORRECTION_MEASUREMENTS	12061
#define MXLV_AD_CORRECTION_FRAMES_TO_SKIP	12062
#define MXLV_AD_DEZINGER_CORRECTION_FRAME	12063
#define MXLV_AD_DEZINGER_THRESHOLD		12064
#define MXLV_AD_USE_SCALED_DARK_CURRENT		12065
#define MXLV_AD_REGISTER_NAME			12066
#define MXLV_AD_REGISTER_VALUE			12067
#define MXLV_AD_SHUTTER_ENABLE			12068
#define MXLV_AD_TRANSFER_IMAGE_DURING_SCAN	12069
#define MXLV_AD_MARK_FRAME_AS_SAVED		12070
#define MXLV_AD_SHOW_IMAGE_FRAME		12071
#define MXLV_AD_SHOW_IMAGE_STATISTICS		12072

#define MXLV_AD_AREA_DETECTOR_FLAGS		12100
#define MXLV_AD_INITIAL_CORRECTION_FLAGS	12101
#define MXLV_AD_MASK_FILENAME			12102
#define MXLV_AD_BIAS_FILENAME			12103
#define MXLV_AD_DARK_CURRENT_FILENAME		12104
#define MXLV_AD_FLAT_FIELD_FILENAME		12105

#define MXLV_AD_SUBFRAME_SIZE			12201

#define MXLV_AD_DATAFILE_DIRECTORY		12500
#define MXLV_AD_DATAFILE_PATTERN		12501
#define MXLV_AD_DATAFILE_NAME			12502
#define MXLV_AD_DATAFILE_NUMBER			12503
#define MXLV_AD_DATAFILE_ALLOW_OVERWRITE	12504
#define MXLV_AD_DATAFILE_AUTOSELECT_NUMBER	12505
#define MXLV_AD_CONSTRUCT_NEXT_DATAFILE_NAME	12506

#define MXLV_AD_DATAFILE_LOAD_FORMAT		12508
#define MXLV_AD_DATAFILE_SAVE_FORMAT		12509
#define MXLV_AD_DATAFILE_LOAD_FORMAT_NAME	12510
#define MXLV_AD_DATAFILE_SAVE_FORMAT_NAME	12511
#define MXLV_AD_CORRECTION_LOAD_FORMAT		12512
#define MXLV_AD_CORRECTION_SAVE_FORMAT		12513
#define MXLV_AD_CORRECTION_LOAD_FORMAT_NAME	12514
#define MXLV_AD_CORRECTION_SAVE_FORMAT_NAME	12515
#define MXLV_AD_DATAFILE_TOTAL_NUM_FRAMES	12516
#define MXLV_AD_DATAFILE_LAST_FRAME_NUMBER	12517
#define MXLV_AD_DISK_SPACE			12518

#define MXLV_AD_OSCILLATION_MOTOR_NAME		12600
#define MXLV_AD_SHUTTER_NAME			12601
#define MXLV_AD_OSCILLATION_TRIGGER_NAME	12602
#define MXLV_AD_OSCILLATION_DISTANCE		12603
#define MXLV_AD_SHUTTER_TIME			12604
#define MXLV_AD_SETUP_OSCILLATION		12605
#define MXLV_AD_TRIGGER_OSCILLATION		12606

#define MXLV_AD_MOTOR_POSITION			12650

#define MXLV_AD_SEQUENCE_ONE_SHOT		12701
#define MXLV_AD_SEQUENCE_CONTINUOUS		12702
#define MXLV_AD_SEQUENCE_MULTIFRAME		12703
/*...*/
#define MXLV_AD_SEQUENCE_STROBE			12705
#define MXLV_AD_SEQUENCE_DURATION		12706
#define MXLV_AD_SEQUENCE_GATED			12707

#define MXLV_AD_IMAGE_LOG_FILENAME		12800

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
  {MXLV_AD_EXPOSURE_MODE, -1, "exposure_mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, exposure_mode), \
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
  {MXLV_AD_START, -1, "start", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, start), \
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
  {MXLV_AD_LATCHED_STATUS, -1, "latched_status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, latched_status), \
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
  {MXLV_AD_EXPOSURE_TIME, -1, "exposure_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, exposure_time),\
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_NUM_EXPOSURES, -1, "num_exposures", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, num_exposures),\
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AD_IMAGE_DATA_AVAILABLE, -1, "image_data_available", \
					MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_data_available),\
	{0}, NULL, MXFF_READ_ONLY}, \
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
  {MXLV_AD_CONSTANT_BIAS_PIXEL_OFFSET, -1, "constant_bias_pixel_offset", \
		MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, constant_bias_pixel_offset), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "flat_field_scale_max", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, flat_field_scale_max), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "flat_field_scale_min", MXFT_DOUBLE, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, flat_field_scale_min), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "flat_field_average_intensity", MXFT_DOUBLE, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, \
                offsetof(MX_AREA_DETECTOR, flat_field_average_intensity), \
        {0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "bias_average_intensity", MXFT_DOUBLE, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, \
                offsetof(MX_AREA_DETECTOR, bias_average_intensity), \
        {0}, NULL, MXFF_READ_ONLY}, \
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
  {MXLV_AD_RAW_LOAD_FRAME, -1, "raw_load_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, load_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_RAW_SAVE_FRAME, -1, "raw_save_frame", MXFT_LONG, NULL, 0, {0}, \
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
  {-1, -1, "saved_dark_current_filename", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, saved_dark_current_filename), \
	{sizeof(char)}, NULL, 0}, \
  \
  {-1, -1, "saved_flat_field_filename", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, saved_flat_field_filename), \
	{sizeof(char)}, NULL, 0}, \
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
  {MXLV_AD_GEOM_CORR_AFTER_FLAT_FIELD, -1, \
  		"geom_corr_after_flat_field", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, geom_corr_after_flat_field), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_BIAS_CORR_AFTER_FLAT_FIELD, -1, \
  		"bias_corr_after_flat_field", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, bias_corr_after_flat_field), \
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
  {-1, -1, "flat_field_frame_buffer", MXFT_CHAR, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, flat_field_frame_buffer), \
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
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_MARK_FRAME_AS_SAVED, -1, "mark_frame_as_saved", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof( MX_AREA_DETECTOR, mark_frame_as_saved ), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SHOW_IMAGE_FRAME, -1, "show_image_frame", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof( MX_AREA_DETECTOR, show_image_frame ), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SHOW_IMAGE_STATISTICS, -1, "show_image_statistics", \
				MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof( MX_AREA_DETECTOR, show_image_statistics ), \
	{0}, NULL, 0}

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
  {MXLV_AD_FLAT_FIELD_FILENAME, -1, "flat_field_filename", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR, flat_field_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "dark_current_offset_can_change", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, dark_current_offset_can_change), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "flat_field_scale_can_change", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, flat_field_scale_can_change), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_DIRECTORY, -1, "datafile_directory", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, datafile_directory), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_LOAD_FORMAT, -1, "datafile_load_format", \
					MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, datafile_load_format),\
	{0}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_SAVE_FORMAT, -1, "datafile_save_format", \
					MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, datafile_save_format),\
	{0}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_LOAD_FORMAT_NAME, -1, "datafile_load_format_name", \
		MXFT_STRING, NULL, 1, {MXU_AD_DATAFILE_FORMAT_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, datafile_load_format_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_SAVE_FORMAT_NAME, -1, "datafile_save_format_name", \
		MXFT_STRING, NULL, 1, {MXU_AD_DATAFILE_FORMAT_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, datafile_save_format_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_CORRECTION_LOAD_FORMAT, -1, "correction_load_format", \
					MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_load_format), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_CORRECTION_SAVE_FORMAT, -1, "correction_save_format", \
					MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_save_format), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_CORRECTION_LOAD_FORMAT_NAME, -1, "correction_load_format_name", \
		MXFT_STRING, NULL, 1, {MXU_AD_DATAFILE_FORMAT_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_load_format_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_CORRECTION_SAVE_FORMAT_NAME, -1, "correction_save_format_name", \
		MXFT_STRING, NULL, 1, {MXU_AD_DATAFILE_FORMAT_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, correction_save_format_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_TOTAL_NUM_FRAMES, -1, "datafile_total_num_frames", \
		MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, datafile_total_num_frames), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_DATAFILE_LAST_FRAME_NUMBER, -1, "datafile_last_frame_number", \
		MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, datafile_last_frame_number), \
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
  {-1, -1, "datafile_total_num_frames", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_AREA_DETECTOR, datafile_total_num_frames), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "inhibit_autosave", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, inhibit_autosave), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_DISK_SPACE, -1, "disk_space", MXFT_UINT64, NULL, 1, {2},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, disk_space), \
	{sizeof(uint64_t)}, NULL, 0}, \
  \
  {MXLV_AD_OSCILLATION_MOTOR_NAME, -1, "oscillation_motor_name", \
			MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR, oscillation_motor_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_SHUTTER_NAME, -1, "shutter_name", MXFT_STRING, \
  					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, shutter_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_OSCILLATION_TRIGGER_NAME, -1, "oscillation_trigger_name", \
			MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_AREA_DETECTOR, oscillation_trigger_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_AD_OSCILLATION_DISTANCE, -1, "oscillation_distance", MXFT_DOUBLE, \
					NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, oscillation_distance),\
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SHUTTER_TIME, -1, "shutter_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, shutter_time),\
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SETUP_OSCILLATION, -1, "setup_oscillation", \
						MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, setup_oscillation), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_TRIGGER_OSCILLATION, -1, "trigger_oscillation", \
						MXFT_BOOL, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, trigger_oscillation), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_MOTOR_POSITION, -1, "motor_position", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, motor_position), \
	{0}, NULL, 0}, \
  \
  {MXLV_AD_SEQUENCE_ONE_SHOT, -1, \
			"sequence_one_shot", MXFT_DOUBLE, NULL, 1, {1}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, sequence_one_shot), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_AD_SEQUENCE_CONTINUOUS, -1, \
			"sequence_continuous", MXFT_DOUBLE, NULL, 1, {1}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, sequence_continuous), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_AD_SEQUENCE_MULTIFRAME, -1, \
			"sequence_multiframe", MXFT_DOUBLE, NULL, 1, {3}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, sequence_multiframe), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_AD_SEQUENCE_STROBE, -1, \
			"sequence_strobe", MXFT_DOUBLE, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, sequence_strobe), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_AD_SEQUENCE_DURATION, -1, \
			"sequence_duration", MXFT_DOUBLE, NULL, 1, {1}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, sequence_duration), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_AD_SEQUENCE_GATED, -1, \
			"sequence_gated", MXFT_DOUBLE, NULL, 1, {3}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, sequence_gated), \
	{sizeof(double)}, NULL, 0}, \
  \
  {MXLV_AD_IMAGE_LOG_FILENAME, -1, "image_log_filename", \
			MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_AREA_DETECTOR, image_log_filename), \
	{sizeof(char)}, NULL, 0}

typedef struct {
        mx_status_type ( *arm ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *trigger ) ( MX_AREA_DETECTOR *ad );
        mx_status_type ( *start ) ( MX_AREA_DETECTOR *ad );
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
	mx_status_type ( *setup_oscillation ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *trigger_oscillation ) ( MX_AREA_DETECTOR *ad );
	mx_status_type ( *save_averaged_correction_frame )
						 ( MX_AREA_DETECTOR *ad );
} MX_AREA_DETECTOR_FUNCTION_LIST;

MX_API mx_status_type mx_area_detector_get_pointers( MX_RECORD *record,
                                        MX_AREA_DETECTOR **ad,
                                MX_AREA_DETECTOR_FUNCTION_LIST **flist_ptr,
                                        const char *calling_fname );

MX_API mx_status_type mx_area_detector_initialize_driver( MX_DRIVER *driver,
					long *maximum_num_rois_varargs_cookie );

MX_API mx_status_type mx_area_detector_finish_record_initialization(
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

MX_API mx_status_type mx_area_detector_get_bytes_per_image_file(
						MX_RECORD *record,
						unsigned long datafile_type,
						size_t *bytes_per_image_file );

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

MX_API mx_status_type mx_area_detector_get_exposure_time( MX_RECORD *ad_record,
							double *exposure_time );

MX_API mx_status_type mx_area_detector_get_num_exposures( MX_RECORD *ad_record,
							long *num_exposures );

MX_API mx_status_type mx_area_detector_set_one_shot_mode(MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_continuous_mode(MX_RECORD *ad_record,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_multiframe_mode(MX_RECORD *ad_record,
							long num_frames,
							double exposure_time,
							double frame_time );

MX_API mx_status_type mx_area_detector_set_strobe_mode( MX_RECORD *ad_record,
							long num_frames,
							double exposure_time );

MX_API mx_status_type mx_area_detector_set_duration_mode( MX_RECORD *ad_record,
							long num_frames );

MX_API mx_status_type mx_area_detector_set_gated_mode( MX_RECORD *ad_record,
							long num_frames,
							double exposure_time,
							double gate_time );

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

MX_API mx_status_type mx_area_detector_get_exposure_mode( MX_RECORD *ad_record,
							long *exposure_mode );

MX_API mx_status_type mx_area_detector_set_exposure_mode( MX_RECORD *ad_record,
							long exposure_mode );

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

MX_API mx_status_type mx_area_detector_image_log_show_error(
						MX_AREA_DETECTOR *ad,
						mx_status_type mx_status );

/*---*/

MX_API mx_status_type mx_area_detector_setup_oscillation( MX_RECORD *ad_record,
						MX_RECORD *motor_record,
						MX_RECORD *shutter_record,
						MX_RECORD *trigger_record,
						double oscillation_distance,
						double shutter_time );

MX_API mx_status_type mx_area_detector_trigger_oscillation(
						MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_wait_for_image_complete(
							MX_RECORD *record,
							double timeout );

MX_API mx_status_type mx_area_detector_wait_for_oscillation_complete(
							MX_RECORD *record,
							double timeout );

MX_API mx_status_type mx_area_detector_trigger_unsafe_oscillation(
						MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_send_oscillation_trigger_pulse(
						MX_AREA_DETECTOR *ad );

/*---*/

MX_API mx_status_type mx_area_detector_setup_frame( MX_RECORD *ad_record,
						MX_IMAGE_FRAME **frame );

MX_API mx_status_type mx_area_detector_setup_correction_frame(
						MX_RECORD *ad_record,
						long frame_format,
						MX_IMAGE_FRAME **frame );

MX_API mx_status_type mx_area_detector_readout_frame( MX_RECORD *ad_record,
						long frame_number );

MX_API mx_status_type mx_area_detector_correct_frame( MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_transfer_frame( MX_RECORD *ad_record,
						long frame_type,
						MX_IMAGE_FRAME **destination );

MX_API mx_status_type mx_area_detector_load_frame( MX_RECORD *ad_record,
						long frame_type,
						char *frame_filename );

MX_API mx_status_type mx_area_detector_save_frame( MX_RECORD *ad_record,
						long frame_type,
						char *frame_filename );

MX_API mx_status_type mx_area_detector_copy_frame( MX_RECORD *ad_record,
						long source_frame_type,
						long destination_frame_type );

MX_API mx_status_type mx_area_detector_update_frame_pointers(
						MX_AREA_DETECTOR *ad );
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

MX_API mx_status_type mx_area_detector_mark_frame_as_saved(
						MX_RECORD *ad_record,
						long frame_number );

MX_API mx_status_type mx_area_detector_get_parameter( MX_RECORD *ad_record,
						long parameter_type );

MX_API mx_status_type mx_area_detector_set_parameter( MX_RECORD *ad_record,
						long parameter_type );

/*---*/

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

/*---*/

MX_API mx_status_type mx_area_detector_default_get_register(
						MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_set_register(
						MX_AREA_DETECTOR *ad );

/*---*/

MX_API mx_status_type mx_area_detector_copy_and_convert_frame(
					MX_IMAGE_FRAME **destination_frame,
					MX_IMAGE_FRAME *source_frame );

MX_API mx_status_type mx_area_detector_copy_and_convert_image_data(
					MX_IMAGE_FRAME *destination_frame,
					MX_IMAGE_FRAME *source_frame );

/*---*/

MX_API mx_status_type mx_area_detector_get_disk_space( MX_RECORD *ad_record,
	 					uint64_t *total_disk_space,
	 					uint64_t *free_disk_space );

MX_API mx_status_type mx_area_detector_get_local_disk_space(
					MX_RECORD *ad_record,
 					uint64_t *local_total_disk_space,
 					uint64_t *local_free_disk_space );

/*---*/

MX_API mx_status_type mx_area_detector_initialize_datafile_number(
							MX_RECORD *ad_record );

MX_API mx_status_type mx_area_detector_construct_next_datafile_name(
				MX_RECORD *ad_record,
				mx_bool_type autoincrement_datafile_number,
				char *filename_buffer,
				size_t filename_buffer_size );

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
				MX_RECORD_FIELD *, int, mx_bool_type * );

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

/***************************************************************************
 *  Area detector correction functions from mx_area_detector_correction.c  *
 ***************************************************************************/

MX_API mx_status_type mx_area_detector_load_correction_files(
						MX_RECORD *record );

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

#define mx_area_detector_measure_flat_field_frame( r, t, n ) \
	mx_area_detector_measure_correction_frame( (r), \
						MXFT_AD_FLAT_FIELD_FRAME, \
						(t), (n) )

MX_API mx_status_type mx_area_detector_save_averaged_correction_frame(
					MX_RECORD *ad_record,
					long correction_measurement_type );

MX_API mx_status_type mx_area_detector_default_save_averaged_correction_frame(
					MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_get_use_scaled_dark_current_flag(
					MX_RECORD *ad_record,
					mx_bool_type *use_scaled_dark_current );

MX_API mx_status_type mx_area_detector_set_use_scaled_dark_current_flag(
					MX_RECORD *ad_record,
					mx_bool_type use_scaled_dark_current );

/*---*/

MX_API mx_status_type mx_area_detector_get_correction_frame(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					unsigned long frame_type,
					char *frame_name,
					MX_IMAGE_FRAME **correction_frame );

MX_API mx_status_type mx_area_detector_default_correct_frame(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_measure_correction(
                                                MX_AREA_DETECTOR *ad );

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

MX_API mx_status_type mx_area_detector_default_dezinger_correction(
                                                MX_AREA_DETECTOR *ad );

MX_API mx_status_type mx_area_detector_default_geometrical_correction(
						MX_AREA_DETECTOR *ad,
						MX_IMAGE_FRAME *frame );

/*---*/

MX_API mx_status_type mx_area_detector_check_correction_framesize(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *test_frame,
					const char *frame_name );

/*---*/

MX_API mx_status_type mx_area_detector_compute_dark_current_offset(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

MX_API mx_status_type mx_area_detector_compute_flat_field_scale(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flat_field_frame );

MX_API mx_status_type mx_area_detector_check_for_low_memory(
					MX_AREA_DETECTOR *ad,
					mx_bool_type *memory_is_low );

/*---*/

MX_API mx_status_type mx_area_detector_classic_frame_correction(
					MX_RECORD *ad_record,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame,
					MX_IMAGE_FRAME *flat_field_frame );

/* Use precomputed dark_current_offset_array. */

MX_API mx_status_type mx_area_detector_u16_precomp_dark_correction(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

MX_API mx_status_type mx_area_detector_s32_precomp_dark_correction(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

MX_API mx_status_type mx_area_detector_flt_precomp_dark_correction(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

MX_API mx_status_type mx_area_detector_dbl_precomp_dark_correction(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

/* Compute the dark current offset on the fly. */

MX_API mx_status_type mx_area_detector_u16_plain_dark_correction(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

MX_API mx_status_type mx_area_detector_s32_plain_dark_correction(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

MX_API mx_status_type mx_area_detector_flt_plain_dark_correction(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

MX_API mx_status_type mx_area_detector_dbl_plain_dark_correction(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame );

/* Use precomputed flat_field_scale_array. */

MX_API mx_status_type mx_area_detector_u16_precomp_flat_field(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flat_field_frame );

MX_API mx_status_type mx_area_detector_s32_precomp_flat_field(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flat_field_frame );

MX_API mx_status_type mx_area_detector_flt_precomp_flat_field(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flat_field_frame );

MX_API mx_status_type mx_area_detector_dbl_precomp_flat_field(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flat_field_frame );

/* Compute the flat field scale on the fly. */

MX_API mx_status_type mx_area_detector_u16_plain_flat_field(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flat_field_frame );

MX_API mx_status_type mx_area_detector_s32_plain_flat_field(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flat_field_frame );

MX_API mx_status_type mx_area_detector_flt_plain_flat_field(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flat_field_frame );

MX_API mx_status_type mx_area_detector_dbl_plain_flat_field(
					MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flat_field_frame );

#ifdef __cplusplus
}
#endif

#endif /* __MX_AREA_DETECTOR_H__ */

