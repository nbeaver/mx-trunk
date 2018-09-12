/*
 * Name:    d_pilatus.h
 *
 * Purpose: Header file for Dectris Pilatus detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015-2016, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PILATUS_H__
#define __D_PILATUS_H__

#define MXU_PILATUS_COMMAND_LENGTH	(MXU_FILENAME_LENGTH+200)

#define MXU_PILATUS_CAMERA_NAME_LENGTH		80
#define MXU_PILATUS_THRESHOLD_STRING_LENGTH	80
#define MXU_PILATUS_TVX_VERSION_LENGTH		80

/* Values for the 'pilatus_flags' field. */

#define MXF_PILATUS_DEBUG_SERIAL			0x1
#define MXF_PILATUS_LOAD_FRAME_AFTER_ACQUISITION	0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long pilatus_flags;
	unsigned long acknowledgement_interval;
	char detector_server_image_directory[MXU_FILENAME_LENGTH+1];
	char detector_server_image_root[MXU_FILENAME_LENGTH+1];
	char local_image_root[MXU_FILENAME_LENGTH+1];

	double delay_time;
	double exposure_period;
	double gap_time;
	unsigned long exposures_per_frame;

	char command[MXU_PILATUS_COMMAND_LENGTH+1];
	char response[MXU_PILATUS_COMMAND_LENGTH+1];
	double set_energy;
	char set_threshold[MXU_PILATUS_THRESHOLD_STRING_LENGTH+1];

	/* For reading temperature and humidity sensors. */
	double th[2];
	unsigned long th_channel;

	char tvx_version[MXU_PILATUS_TVX_VERSION_LENGTH+1];
	char camera_name[MXU_PILATUS_CAMERA_NAME_LENGTH+1];
	unsigned long camserver_pid;

	mx_bool_type exposure_in_progress;
	unsigned long old_total_num_frames;
	unsigned long old_datafile_number;
} MX_PILATUS;

#define MXLV_PILATUS_DETECTOR_SERVER_IMAGE_DIRECTORY	87801
#define MXLV_PILATUS_DETECTOR_SERVER_IMAGE_ROOT		87802
#define MXLV_PILATUS_LOCAL_IMAGE_ROOT			87803
#define MXLV_PILATUS_COMMAND				87804
#define MXLV_PILATUS_RESPONSE				87805
#define MXLV_PILATUS_SET_ENERGY				87806
#define MXLV_PILATUS_SET_THRESHOLD			87807
#define MXLV_PILATUS_TH					87808


#define MXD_PILATUS_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "pilatus_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, pilatus_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "acknowledgement_interval", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, acknowledgement_interval), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_PILATUS_DETECTOR_SERVER_IMAGE_DIRECTORY, \
			 -1, "detector_server_image_directory", \
			MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PILATUS, detector_server_image_directory), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PILATUS_DETECTOR_SERVER_IMAGE_ROOT, -1, "detector_server_image_root", \
			MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, detector_server_image_root), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_PILATUS_LOCAL_IMAGE_ROOT, -1, "local_image_root", \
			MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, local_image_root), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "delay_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, delay_time), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "exposure_period", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, exposure_period), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "gap_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, gap_time), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "exposures_per_frame", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, exposures_per_frame), \
	{0}, NULL, 0}, \
  \
  {MXLV_PILATUS_COMMAND, -1, "command", \
		MXFT_STRING, NULL, 1, {MXU_PILATUS_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, command), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PILATUS_RESPONSE, -1, "response", \
		MXFT_STRING, NULL, 1, {MXU_PILATUS_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, response), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PILATUS_SET_ENERGY, -1, "set_energy", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, set_energy), \
	{0}, NULL, 0}, \
  \
  {MXLV_PILATUS_SET_THRESHOLD, -1, "set_threshold", \
		MXFT_STRING, NULL, 1, {MXU_PILATUS_THRESHOLD_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, set_threshold), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PILATUS_TH, -1, "th", MXFT_DOUBLE, NULL, 1, {2}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, th), \
	{sizeof(double)}, NULL, 0}, \
  \
  {-1, -1, "th_channel", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, th_channel), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "tvx_version", MXFT_STRING, NULL, \
			1, {MXU_PILATUS_TVX_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, tvx_version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "camera_name", MXFT_STRING, NULL, \
			1, {MXU_PILATUS_CAMERA_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, camera_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "camserver_pid", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PILATUS, camserver_pid), \
	{0}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxd_pilatus_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_pilatus_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_pilatus_open( MX_RECORD *record );
MX_API mx_status_type mxd_pilatus_special_processing_setup( MX_RECORD *record );

MX_API mx_status_type mxd_pilatus_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_extended_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_load_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_save_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_copy_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_set_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_measure_correction( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_setup_oscillation( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_trigger_oscillation( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_pilatus_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST
			mxd_pilatus_ad_function_list;

extern long mxd_pilatus_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pilatus_rfield_def_ptr;

MX_API mx_status_type mxd_pilatus_command( MX_PILATUS *pilatus,
					char *command,
					char *response,
					size_t response_buffer_length,
					unsigned long *pilatus_return_code );

#endif /* __D_PILATUS_H__ */

